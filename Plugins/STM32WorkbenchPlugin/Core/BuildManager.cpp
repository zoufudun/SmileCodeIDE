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
    return QDir(projectDir).absoluteFilePath(m_profile.buildDirectory() + "/" + m_profile.artifactBaseName() + ".elf");
}

QString BuildManager::getBinPath(const QString &projectDir) const
{
    return QDir(projectDir).absoluteFilePath(m_profile.buildDirectory() + "/" + m_profile.artifactBaseName() + ".bin");
}

QString BuildManager::getHexPath(const QString &projectDir) const
{
    return QDir(projectDir).absoluteFilePath(m_profile.buildDirectory() + "/" + m_profile.artifactBaseName() + ".hex");
}

bool BuildManager::startBuild(const QString &projectDir,
                             BuildConfiguration config,
                             int parallelJobs,
                             const CompilerBuildOptions &buildOptions)
{
    if (isBusy()) {
        emit buildOutput(QStringLiteral("错误: 已有构建任务正在运行中!\n"), true);
        return false;
    }

    CompilerBuildOptions effectiveOpts = buildOptions;
    if (effectiveOpts.optLevel.trimmed().isEmpty()) {
        if (!m_buildOptions.optLevel.trimmed().isEmpty()) {
            effectiveOpts = m_buildOptions;
        } else {
            effectiveOpts = MakefileGenerator::defaultOptionsForConfig(config);
        }
    }

    beginSession(projectDir, QStringLiteral("编译构建"));

    // 1. 生成最新 Makefile
    QString genErr;
    if (!MakefileGenerator::generate(projectDir, m_profile, m_paths, config, QString(), QStringList(), QStringList(), &genErr, effectiveOpts)) {
        consumeOutput("错误: Makefile 生成失败: " + genErr + "\n", true);
        finishSession(false, -1, 0);
        if (genErr.contains(QStringLiteral("旧版自动生成的链接脚本")))
            emit legacyLinkerMigrationRequired(
                QDir(projectDir).absoluteFilePath(m_profile.defaultLdFile));
        return false;
    }

    consumeOutput(QString("====== 开始构建工程 [%1] (%2 / 优化: %3) ======\n")
                     .arg(m_profile.chipName)
                     .arg(config == BuildConfiguration::Debug ? "Debug" : "Release")
                     .arg(effectiveOpts.toOptimizationFlag()), false);

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

    consumeOutput(QString("工程目录: %1\nGCC: %2\nMake: %3\n执行构建命令: %4 %5\n\n")
                     .arg(projectDir, m_paths.gccPath.isEmpty() ? QStringLiteral("PATH 中的 arm-none-eabi-gcc") : m_paths.gccPath,
                          makeExec, makeExec, args.join(" ")), false);
    m_process->start(makeExec, args);

    if (!m_process->waitForStarted(3000)) {
        consumeOutput("错误: 无法启动 make 进程: " + m_process->errorString() + "\n", true);
        finishSession(false, -1, 0);
        return false;
    }

    return true;
}

bool BuildManager::migrateLegacyLinkerScript(const QString &projectDir,
                                             QString *outBackupPath,
                                             QString *outError)
{
    return MakefileGenerator::migrateLegacyLinkerScript(
        projectDir, m_profile, outBackupPath, outError);
}

bool BuildManager::startClean(const QString &projectDir)
{
    if (isBusy()) return false;
    beginSession(projectDir, QStringLiteral("清理工程"));
    if (m_profile.chipName.isEmpty()) {
        consumeOutput(QStringLiteral("错误: 请先选择有效目标芯片。\n"), true);
        finishSession(false, -1, 0);
        return false;
    }
    consumeOutput(QStringLiteral("====== 开始清理构建中间文件 ======\n"), false);

    // The project root has one generated Makefile. After switching H7 cores it may
    // still describe the other core, so clean the selected core directly.
    if (!m_profile.linkerContext.isEmpty()) {
        QDir buildDir(QDir(projectDir).filePath(m_profile.buildDirectory()));
        const bool cleaned = !buildDir.exists() || buildDir.removeRecursively();
        consumeOutput(cleaned ? QStringLiteral("已清理当前核心构建目录\n")
                                 : QStringLiteral("无法清理当前核心构建目录\n"), !cleaned);
        if (cleaned && m_isRebuilding) {
            m_isRebuilding = false;
            return startBuild(projectDir, m_pendingConfig, m_pendingJobs, m_pendingBuildOptions);
        }
        finishSession(cleaned, cleaned ? 0 : -1, 0);
        return cleaned;
    }

    QString makeExec = m_paths.makePath.isEmpty() ? "make" : m_paths.makePath;
    QStringList args;
    args << "clean";

    m_process->setWorkingDirectory(projectDir);
    m_process->start(makeExec, args);

    if (!m_process->waitForStarted(3000)) {
        // 如果 make clean 无法运行，只清理当前核心的构建目录。
        QDir buildDir(QDir(projectDir).filePath(m_profile.buildDirectory()));
        const bool cleaned = !buildDir.exists() || buildDir.removeRecursively();
        consumeOutput(cleaned ? QStringLiteral("已通过原生方式清理当前构建目录\n")
                              : QStringLiteral("错误: 无法清理当前构建目录\n"), !cleaned);
        if (cleaned && m_isRebuilding) {
            m_isRebuilding = false;
            return startBuild(projectDir, m_pendingConfig, m_pendingJobs, m_pendingBuildOptions);
        }
        finishSession(cleaned, cleaned ? 0 : -1, 0);
        return cleaned;
    }

    return true;
}

bool BuildManager::startRebuild(const QString &projectDir,
                               BuildConfiguration config,
                               int parallelJobs,
                               const CompilerBuildOptions &buildOptions)
{
    if (isBusy()) return false;

    m_isRebuilding = true;
    m_pendingConfig = config;
    m_pendingJobs = parallelJobs;
    m_pendingBuildOptions = buildOptions;

    return startClean(projectDir);
}

void BuildManager::stopBuild()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_cancelled = true;
        m_isRebuilding = false;
        consumeOutput(QStringLiteral("[已手动终止构建进程]\n"), true);
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
    }
}

void BuildManager::onProcessReadyReadStd()
{
    QByteArray bytes = m_process->readAllStandardOutput();
    QString text = m_stdoutDecoder.decode(bytes);
    consumeOutput(text, false);
}

void BuildManager::onProcessReadyReadErr()
{
    QByteArray bytes = m_process->readAllStandardError();
    QString text = m_stderrDecoder.decode(bytes);
    consumeOutput(text, true);
}

void BuildManager::beginSession(const QString &projectDir, const QString &action)
{
    m_currentProjectDir = projectDir;
    m_currentAction = action;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_cancelled = false;
    m_stdoutDecoder.resetState();
    m_stderrDecoder.resetState();
    m_log.reset(projectDir, action);
    emit buildSessionStarted(action);
    emit buildStarted(action);
}

void BuildManager::publishDiagnostics(const QVector<BuildDiagnostic> &diagnostics)
{
    for (const auto &diagnostic : diagnostics) emit diagnosticFound(diagnostic);
}

void BuildManager::consumeOutput(const QString &text, bool isError)
{
    if (text.isEmpty()) return;
    const qsizetype firstLine = m_log.detailedEntries().size();
    publishDiagnostics(m_log.append(text, isError));
    const auto &lines = m_log.detailedEntries();
    for (qsizetype i = firstLine; i < lines.size(); ++i) parseSizeOutput(lines.at(i).text);
    emit buildOutput(text, isError);
    emit buildLogChanged();
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

        // Legacy consumers still receive this report; the workbench renders the
        // model's compact size entry from the original size-tool output.
        emit buildOutput(memInfo, false);
    }
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    onProcessReadyReadStd();
    onProcessReadyReadErr();
    const qsizetype firstLine = m_log.detailedEntries().size();
    publishDiagnostics(m_log.flush());
    const auto &lines = m_log.detailedEntries();
    for (qsizetype i = firstLine; i < lines.size(); ++i) parseSizeOutput(lines.at(i).text);
    int duration = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime);
    bool success = !m_cancelled && exitCode == 0 && exitStatus == QProcess::NormalExit && m_log.errorCount() == 0;

    if (m_isRebuilding) {
        m_isRebuilding = false;
        if (success) {
            // 清理成功后，紧接着触发编译构建
            startBuild(m_currentProjectDir, m_pendingConfig, m_pendingJobs, m_pendingBuildOptions);
            return;
        }
    }

    finishSession(success, exitCode, duration);
}

void BuildManager::finishSession(bool success, int exitCode, int durationMs)
{
    m_isRebuilding = false;
    const qsizetype previousDiagnostics = m_log.diagnostics().size();
    const auto result = m_log.finish(success && !m_cancelled, exitCode, durationMs);
    const auto &diagnostics = m_log.diagnostics();
    for (qsizetype i = previousDiagnostics; i < diagnostics.size(); ++i)
        emit diagnosticFound(diagnostics.at(i));
    emit buildOutput(result.summary + '\n', !result.success);
    emit buildLogChanged();
    emit buildFinished(result.success, exitCode, durationMs);
}
