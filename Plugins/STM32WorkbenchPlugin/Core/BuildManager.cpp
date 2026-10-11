#include "BuildManager.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

BuildManager::BuildManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessReadyReadStd);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessReadyReadErr);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildManager::onProcessFinished);
}

BuildManager::~BuildManager()
{
    stopBuild();
}

bool BuildManager::isBusy() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

QString BuildManager::getElfPath(const QString &projectDir) const
{
    QString target = m_profile.chipName.isEmpty() ? "firmware" : m_profile.chipName.toLower();
    return QDir(projectDir).absoluteFilePath("build/" + target + ".elf");
}

QString BuildManager::getBinPath(const QString &projectDir) const
{
    QString target = m_profile.chipName.isEmpty() ? "firmware" : m_profile.chipName.toLower();
    return QDir(projectDir).absoluteFilePath("build/" + target + ".bin");
}

QString BuildManager::getHexPath(const QString &projectDir) const
{
    QString target = m_profile.chipName.isEmpty() ? "firmware" : m_profile.chipName.toLower();
    return QDir(projectDir).absoluteFilePath("build/" + target + ".hex");
}

bool BuildManager::startBuild(const QString &projectDir,
                             BuildConfiguration config,
                             int parallelJobs)
{
    if (isBusy()) {
        emit buildOutput(QStringLiteral("错误: 已有构建任务正在运行中!\n"), true);
        return false;
    }

    m_currentProjectDir = projectDir;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_pendingStdout.clear();
    m_pendingStderr.clear();
    m_errorCount = 0;
    m_warningCount = 0;

    // 1. 生成最新 Makefile
    QString genErr;
    if (!MakefileGenerator::generate(projectDir, m_profile, m_paths, config, QString(), QStringList(), QStringList(), &genErr)) {
        emit buildOutput("Makefile 生成失败: " + genErr + "\n", true);
        emit buildFinished(false, -1, 0);
        return false;
    }

    emit buildStarted(QStringLiteral("编译构建"));
    emit buildOutput(QString("====== 开始构建工程 [%1] (%2) ======\n")
                     .arg(m_profile.chipName)
                     .arg(config == BuildConfiguration::Debug ? "Debug" : "Release"), false);

    // 2. 准备 make 命令与参数
    QString makeExec = m_paths.makePath;
    if (makeExec.isEmpty()) {
        makeExec = "make";
    }

    int jobs = parallelJobs > 0 ? parallelJobs : QThread::idealThreadCount();
    if (jobs <= 0) jobs = 4;

    QStringList args;
    args << QString("-j%1").arg(jobs);

    m_process->setWorkingDirectory(projectDir);

    // 设置必要的系统环境变量 PATH (将 gcc 与 make 所在目录追加至 PATH)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathVar = env.value("PATH");
    if (!m_paths.gccPath.isEmpty()) {
        pathVar = QFileInfo(m_paths.gccPath).absolutePath() + QDir::listSeparator() + pathVar;
    }
    if (!m_paths.makePath.isEmpty()) {
        pathVar = QFileInfo(m_paths.makePath).absolutePath() + QDir::listSeparator() + pathVar;
    }
    env.insert("PATH", pathVar);
    m_process->setProcessEnvironment(env);

    emit buildOutput(QString("工程目录: %1\nGCC: %2\nMake: %3\n执行构建命令: %4 %5\n\n")
                     .arg(projectDir, m_paths.gccPath.isEmpty() ? QStringLiteral("PATH 中的 arm-none-eabi-gcc") : m_paths.gccPath,
                          makeExec, makeExec, args.join(" ")), false);
    m_process->start(makeExec, args);

    if (!m_process->waitForStarted(3000)) {
        emit buildOutput("无法启动 make 进程: " + m_process->errorString() + "\n", true);
        emit buildFinished(false, -1, 0);
        return false;
    }

    return true;
}

bool BuildManager::startClean(const QString &projectDir)
{
    if (isBusy()) return false;

    m_currentProjectDir = projectDir;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_pendingStdout.clear();
    m_pendingStderr.clear();
    m_errorCount = 0;
    m_warningCount = 0;

    emit buildStarted(QStringLiteral("清理工程"));
    emit buildOutput(QStringLiteral("====== 开始清理构建中间文件 ======\n"), false);

    QString makeExec = m_paths.makePath.isEmpty() ? "make" : m_paths.makePath;
    QStringList args;
    args << "clean";

    m_process->setWorkingDirectory(projectDir);
    m_process->start(makeExec, args);

    if (!m_process->waitForStarted(3000)) {
        // 如果 make clean 无法运行，使用安全原生 Qt 删除 build 目录
        QDir buildDir(projectDir + "/build");
        if (buildDir.exists()) {
            buildDir.removeRecursively();
        }
        emit buildOutput(QStringLiteral("已通过原生方式清理 build 目录\n"), false);
        emit buildFinished(true, 0, 100);
        return true;
    }

    return true;
}

bool BuildManager::startRebuild(const QString &projectDir,
                               BuildConfiguration config,
                               int parallelJobs)
{
    if (isBusy()) return false;

    m_isRebuilding = true;
    m_pendingConfig = config;
    m_pendingJobs = parallelJobs;

    return startClean(projectDir);
}

void BuildManager::stopBuild()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) {
            m_process->kill();
        }
        emit buildOutput(QStringLiteral("\n[已手动终止构建进程]\n"), true);
    }
}

void BuildManager::onProcessReadyReadStd()
{
    QByteArray bytes = m_process->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(bytes);
    emit buildOutput(text, false);
    consumeOutput(text, false);
}

void BuildManager::onProcessReadyReadErr()
{
    QByteArray bytes = m_process->readAllStandardError();
    QString text = QString::fromLocal8Bit(bytes);
    emit buildOutput(text, true);
    consumeOutput(text, true);
}

void BuildManager::consumeOutput(const QString &text, bool isError)
{
    QString &pending = isError ? m_pendingStderr : m_pendingStdout;
    pending += text;
    int newline = pending.indexOf('\n');
    while (newline >= 0) {
        parseOutputLine(pending.left(newline).trimmed(), isError);
        pending.remove(0, newline + 1);
        newline = pending.indexOf('\n');
    }
}

void BuildManager::parseOutputLine(const QString &line, bool isError)
{
    Q_UNUSED(isError);
    if (line.isEmpty()) return;

    // 1. GCC 报错与警告提取正则
    // 示例: Src/main.c:45:10: error: 'HAL_StatusTypeDef' undeclared
    static QRegularExpression diagRegex(R"(^(.+?):(\d+)(?::(\d+))?:\s+(fatal error|error|warning|note):\s+(.*)$)");
    QRegularExpressionMatch match = diagRegex.match(line);
    if (match.hasMatch()) {
        BuildDiagnostic diag;
        QString rawPath = match.captured(1);
        if (QFileInfo(rawPath).isRelative()) {
            diag.filePath = QDir(m_currentProjectDir).absoluteFilePath(rawPath);
        } else {
            diag.filePath = rawPath;
        }
        diag.line = match.captured(2).toInt();
        diag.column = match.captured(3).toInt();

        QString sev = match.captured(4).toLower();
        if (sev.contains("error")) {
            diag.severity = sev.contains("fatal") ? DiagnosticSeverity::Fatal : DiagnosticSeverity::Error;
        } else if (sev.contains("warning")) {
            diag.severity = DiagnosticSeverity::Warning;
        } else {
            diag.severity = DiagnosticSeverity::Info;
        }

        diag.message = match.captured(5);
        if (diag.severity == DiagnosticSeverity::Warning) ++m_warningCount;
        if (diag.severity == DiagnosticSeverity::Error || diag.severity == DiagnosticSeverity::Fatal) ++m_errorCount;
        emit diagnosticFound(diag);
    }

    // 2. 解析 arm-none-eabi-size 尺寸统计输出
    // 典型行格式:   text    data     bss     dec     hex filename
    //              12420     108    1568   14096    3710 build/stm32f103c8.elf
    parseSizeOutput(line);
}

void BuildManager::parseSizeOutput(const QString &line)
{
    static QRegularExpression sizeRegex(R"(^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([0-9a-fA-F]+)\s+(.*\.elf.*)$)");
    QRegularExpressionMatch match = sizeRegex.match(line);
    if (match.hasMatch()) {
        quint32 text = match.captured(1).toUInt();
        quint32 data = match.captured(2).toUInt();
        quint32 bss  = match.captured(3).toUInt();

        quint32 flashUsed = text + data;
        quint32 ramUsed   = data + bss;

        quint32 flashTotal = m_profile.flashSizeBytes > 0 ? m_profile.flashSizeBytes : 64 * 1024;
        quint32 ramTotal   = m_profile.ramSizeBytes > 0 ? m_profile.ramSizeBytes : 20 * 1024;

        emit memoryUsageUpdated(flashUsed, flashTotal, ramUsed, ramTotal);

        QString memInfo = QString("\n========== 芯片内存资源占用报告 (%1) ==========\n"
                                  "  Flash 占用: %2 字节 (%3 KB) / %4 KB [%5%]\n"
                                  "  RAM 占用:   %6 字节 (%7 KB) / %8 KB [%9%]\n"
                                  "  (text=%10, data=%11, bss=%12)\n"
                                  "====================================================\n")
                          .arg(m_profile.chipName)
                          .arg(flashUsed).arg(flashUsed / 1024.0, 0, 'f', 2).arg(flashTotal / 1024)
                          .arg(QString::number((double)flashUsed / flashTotal * 100.0, 'f', 1))
                          .arg(ramUsed).arg(ramUsed / 1024.0, 0, 'f', 2).arg(ramTotal / 1024)
                          .arg(QString::number((double)ramUsed / ramTotal * 100.0, 'f', 1))
                          .arg(text).arg(data).arg(bss);

        emit buildOutput(memInfo, false);
    }
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    onProcessReadyReadStd();
    onProcessReadyReadErr();
    if (!m_pendingStdout.isEmpty()) parseOutputLine(m_pendingStdout.trimmed(), false);
    if (!m_pendingStderr.isEmpty()) parseOutputLine(m_pendingStderr.trimmed(), true);
    m_pendingStdout.clear();
    m_pendingStderr.clear();
    int duration = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime);
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    if (m_isRebuilding) {
        m_isRebuilding = false;
        if (success) {
            // 清理成功后，紧接着触发编译构建
            startBuild(m_currentProjectDir, m_pendingConfig, m_pendingJobs);
            return;
        }
    }

    if (success) {
        emit buildOutput(QString("\n✅ 构建成功! 错误 %1，警告 %2，耗时 %3 秒\n\n")
                         .arg(m_errorCount).arg(m_warningCount).arg(duration / 1000.0, 0, 'f', 2), false);
    } else {
        emit buildOutput(QString("\n❌ 构建失败! 退出代码: %1，错误 %2，警告 %3，耗时 %4 秒\n\n")
                         .arg(exitCode).arg(m_errorCount).arg(m_warningCount).arg(duration / 1000.0, 0, 'f', 2), true);
    }

    emit buildFinished(success, exitCode, duration);
}
