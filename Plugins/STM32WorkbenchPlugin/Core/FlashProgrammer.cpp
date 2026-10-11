#include "FlashProgrammer.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>
#include <QDebug>

FlashProgrammer::FlashProgrammer(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FlashProgrammer::onProcessReadyReadStd);
    connect(m_process, &QProcess::readyReadStandardError, this, &FlashProgrammer::onProcessReadyReadErr);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &FlashProgrammer::onProcessFinished);
}

FlashProgrammer::~FlashProgrammer()
{
    cancelFlash();
}

bool FlashProgrammer::isBusy() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void FlashProgrammer::cancelFlash()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) {
            m_process->kill();
        }
        emit flashOutput(QStringLiteral("\n[已手动中止固件烧录]\n"), true);
    }

    if (!m_tempScriptPath.isEmpty() && QFile::exists(m_tempScriptPath)) {
        QFile::remove(m_tempScriptPath);
        m_tempScriptPath.clear();
    }
    if (!m_tempFirmwarePath.isEmpty()) {
        QFile::remove(m_tempFirmwarePath);
        m_tempFirmwarePath.clear();
    }
}

bool FlashProgrammer::flashFirmware(const QString &firmwarePath, const FlashOptions &options)
{
    if (isBusy()) {
        emit flashOutput(QStringLiteral("错误: 已有烧录任务正在进行中!\n"), true);
        return false;
    }

    if (!QFile::exists(firmwarePath)) {
        emit flashOutput(QString("错误: 固件文件不存在: %1\n请先执行编译构建!\n").arg(firmwarePath), true);
        return false;
    }
    if (!validTargetAddress(options.targetAddress)) {
        emit flashOutput(QStringLiteral("错误: Flash 起始地址必须是有效的 32 位十六进制或十进制数。\n"), true);
        return false;
    }
    if (options.eraseAll && !driverSupportsMassErase(options.driver)) {
        emit flashOutput(QStringLiteral("错误: 当前烧录驱动不支持全片擦除，请改用 J-Link 官方工具或 STM32CubeProgrammer。\n"), true);
        return false;
    }

    static const QRegularExpression serialPattern("^[A-Za-z0-9._:-]+$");
    if (!options.probeSerial.isEmpty() && !serialPattern.match(options.probeSerial).hasMatch()) {
        emit flashOutput(QStringLiteral("错误: 下载器序列号包含不支持的字符。\n"), true);
        return false;
    }
    if (!options.probeInstanceId.isEmpty()) {
        QString probeError;
        if (!validateProbeSelection(options, ProbeDetector::scan(), &probeError)) {
            emit flashOutput(QStringLiteral("错误: ") + probeError + "\n", true);
            return false;
        }
    }

    m_startTime = QDateTime::currentMSecsSinceEpoch();

    switch (options.driver) {
        case FlashDriverType::OpenOCD_STLink:
        case FlashDriverType::OpenOCD_CMSISDAP:
        case FlashDriverType::OpenOCD_JLink:
            return flashViaOpenOCD(firmwarePath, options);

        case FlashDriverType::JLink_Native:
            return flashViaJLink(firmwarePath, options);

        case FlashDriverType::STM32CubeProg:
            return flashViaSTM32Cube(firmwarePath, options);

        case FlashDriverType::StFlash:
            return flashViaStFlash(firmwarePath, options);
    }

    return false;
}

bool FlashProgrammer::validTargetAddress(const QString &address)
{
    static const QRegularExpression addressPattern("^(?:0[xX][0-9a-fA-F]+|[0-9]+)$");
    if (!addressPattern.match(address).hasMatch()) return false;
    bool ok = false;
    const qulonglong value = address.toULongLong(&ok, address.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10);
    return ok && value <= 0xffffffffULL;
}

bool FlashProgrammer::driverSupportsMassErase(FlashDriverType driver)
{
    return driver == FlashDriverType::JLink_Native || driver == FlashDriverType::STM32CubeProg;
}

bool FlashProgrammer::driverSupportsProbe(FlashDriverType driver, ProbeType probe)
{
    switch (probe) {
    case ProbeType::STLink:
        return driver == FlashDriverType::OpenOCD_STLink ||
               driver == FlashDriverType::STM32CubeProg || driver == FlashDriverType::StFlash;
    case ProbeType::CmsisDap:
        return driver == FlashDriverType::OpenOCD_CMSISDAP;
    case ProbeType::JLink:
        return driver == FlashDriverType::OpenOCD_JLink || driver == FlashDriverType::JLink_Native;
    default:
        return false;
    }
}

bool FlashProgrammer::validateProbeSelection(const FlashOptions &options,
                                              const QList<ProbeInfo> &connectedProbes,
                                              QString *error)
{
    if (options.probeInstanceId.isEmpty()) return true;
    const ProbeInfo *selected = nullptr;
    int compatibleCount = 0;
    for (const ProbeInfo &probe : connectedProbes) {
        if (driverSupportsProbe(options.driver, probe.type)) ++compatibleCount;
        if (probe.instanceId.compare(options.probeInstanceId, Qt::CaseInsensitive) == 0)
            selected = &probe;
    }
    if (!selected) {
        if (error) *error = QStringLiteral("选定的下载器已断开，请刷新烧录设置。");
        return false;
    }
    if (!driverSupportsProbe(options.driver, selected->type)) {
        if (error) *error = QStringLiteral("当前烧录驱动与选定下载器类型不匹配。");
        return false;
    }
    if (options.probeSerial.isEmpty() && compatibleCount > 1) {
        if (error) *error = QStringLiteral("检测到多台同类下载器，必须指定序列号才能选择设备。");
        return false;
    }
    if (!selected->serial.isEmpty() && !options.probeSerial.isEmpty() &&
        selected->serial.compare(options.probeSerial, Qt::CaseInsensitive) != 0) {
        if (error) *error = QStringLiteral("当前设备序列号与已保存的序列号不一致，请刷新烧录设置。");
        return false;
    }
    return true;
}

QString FlashProgrammer::resolveOpenOcdScriptsDir(const QString &openocdPath)
{
    const QDir binDir = QFileInfo(openocdPath).absoluteDir();
    const QString standard = QDir::cleanPath(binDir.absoluteFilePath("../share/openocd/scripts"));
    if (QDir(standard).exists()) return standard;
    const QString compact = QDir::cleanPath(binDir.absoluteFilePath("../scripts"));
    if (QDir(compact).exists()) return compact;
    return standard;
}

bool FlashProgrammer::validateOpenOcdScripts(const FlashOptions &options,
                                              const QString &targetCfg,
                                              const QString &scriptsDir,
                                              QString *error)
{
    const QString interfaceCfg = !options.customOpenocdInterfaceCfg.isEmpty()
        ? options.customOpenocdInterfaceCfg
        : options.driver == FlashDriverType::OpenOCD_CMSISDAP ? "interface/cmsis-dap.cfg"
        : options.driver == FlashDriverType::OpenOCD_JLink ? "interface/jlink.cfg"
        : "interface/stlink.cfg";
    if (targetCfg.isEmpty()) {
        if (error) *error = QStringLiteral("此芯片没有已知的 OpenOCD target 配置，请手动指定受支持的 target 脚本。");
        return false;
    }
    if (!QDir(scriptsDir).exists()) {
        if (error) *error = QStringLiteral("OpenOCD 脚本目录不存在: ") + scriptsDir;
        return false;
    }
    for (const QString &cfg : {interfaceCfg, targetCfg}) {
        const QString resolved = QFileInfo(cfg).isAbsolute()
            ? cfg : QDir(scriptsDir).filePath(cfg);
        if (!QFileInfo(resolved).isFile()) {
            if (error) *error = QStringLiteral("OpenOCD 脚本不存在: ") + cfg;
            return false;
        }
    }
    return true;
}

QStringList FlashProgrammer::openOcdArguments(const QString &firmwarePath,
                                               const FlashOptions &options,
                                               const QString &targetCfg)
{
    const QString interfaceCfg = !options.customOpenocdInterfaceCfg.isEmpty() ?
        options.customOpenocdInterfaceCfg :
        options.driver == FlashDriverType::OpenOCD_CMSISDAP ? "interface/cmsis-dap.cfg" :
        options.driver == FlashDriverType::OpenOCD_JLink ? "interface/jlink.cfg" :
        "interface/stlink.cfg";
    QStringList args = {"-f", interfaceCfg};
    if (!options.probeSerial.isEmpty()) args << "-c" << ("adapter serial " + options.probeSerial);
    args << "-c" << ("adapter speed " + QString::number(options.swdSpeedKhz));
    args << "-f" << targetCfg;
    QString path = firmwarePath;
    path.replace('\\', '/');
    QString command = QString("program \"%1\" %2").arg(path, options.targetAddress);
    if (options.verify) command += " verify";
    if (options.resetAndRun) command += " reset";
    args << "-c" << (command + " exit");
    return args;
}

QStringList FlashProgrammer::cubeProgrammerArguments(const QString &firmwarePath,
                                                     const FlashOptions &options)
{
    QStringList args = {"-c", "port=SWD"};
    if (!options.probeSerial.isEmpty()) args << ("sn=" + options.probeSerial);
    args << ("freq=" + QString::number(options.swdSpeedKhz));
    if (options.eraseAll) args << "-e" << "all";
    QString path = firmwarePath;
    path.replace('\\', '/');
    args << "-w" << path << options.targetAddress;
    if (options.verify) args << "-v";
    if (options.resetAndRun) args << "-rst";
    return args;
}

QStringList FlashProgrammer::jLinkArguments(const FlashOptions &options,
                                            const QString &device,
                                            const QString &scriptPath)
{
    QStringList args = {"-device", device, "-if", "SWD", "-speed",
                        QString::number(options.swdSpeedKhz), "-autoconnect", "1",
                        "-NoGui", "1", "-ExitOnError", "1"};
    if (!options.probeSerial.isEmpty()) args << "-USB" << options.probeSerial;
    args << "-CommandFile" << scriptPath;
    return args;
}

QString FlashProgrammer::jLinkCommandScript(const QString &firmwarePath,
                                             const FlashOptions &options,
                                             const QString &device)
{
    QString path = firmwarePath;
    path.replace('\\', '/');
    QString script = QString("si 1\nspeed %1\ndevice %2\nr\nh\n")
        .arg(options.swdSpeedKhz).arg(device);
    if (options.eraseAll) script += "erase\n";
    // SEGGER documents LoadFile <FileName> <Addr> [noreset|reset]. It verifies flash itself.
    script += QString("loadfile \"%1\" %2 noreset\n")
        .arg(path, firmwarePath.endsWith(".bin", Qt::CaseInsensitive) ? options.targetAddress : "0");
    if (options.resetAndRun) script += "r\ng\n";
    return script + "q\n";
}

bool FlashProgrammer::flashViaOpenOCD(const QString &firmwarePath, const FlashOptions &options)
{
    if (m_paths.openocdPath.isEmpty() || !QFile::exists(m_paths.openocdPath)) {
        emit flashOutput(QStringLiteral("错误: 未配置有效的 OpenOCD 路径!\n"), true);
        return false;
    }

    QString targetCfg;
    if (!options.customOpenocdTargetCfg.isEmpty()) {
        targetCfg = options.customOpenocdTargetCfg;
    } else {
        targetCfg = m_profile.openocdTargetCfg.isEmpty() ?
                    STM32ProfileManager::getOpenocdTarget(m_profile.family) : m_profile.openocdTargetCfg;
    }

    QString scriptsDir = m_paths.openocdScriptsDir;
    if (scriptsDir.isEmpty()) {
        scriptsDir = resolveOpenOcdScriptsDir(m_paths.openocdPath);
    }
    QString preflightError;
    if (!validateOpenOcdScripts(options, targetCfg, scriptsDir, &preflightError)) {
        emit flashOutput(QStringLiteral("错误: ") + preflightError + "\n", true);
        return false;
    }

    emit flashStarted(QStringLiteral("OpenOCD"));
    emit flashOutput(QString("====== 启动 OpenOCD 烧录: %1 ======\n").arg(QFileInfo(firmwarePath).fileName()), false);

    // 先指定接口及设备序列号，再加载目标脚本。
    QStringList args;
    args << "-s" << scriptsDir;

    args << openOcdArguments(firmwarePath, options, targetCfg);

    m_process->setWorkingDirectory(QFileInfo(firmwarePath).absolutePath());
    emit flashOutput(QString("执行: %1 %2\n\n").arg(m_paths.openocdPath, args.join(" ")), false);
    m_process->start(m_paths.openocdPath, args);

    return m_process->waitForStarted(3000);
}

bool FlashProgrammer::flashViaJLink(const QString &firmwarePath, const FlashOptions &options)
{
    if (m_paths.jlinkPath.isEmpty() || !QFile::exists(m_paths.jlinkPath)) {
        emit flashOutput(QStringLiteral("错误: 未配置有效的 J-Link (JLink.exe) 路径!\n"), true);
        return false;
    }

    emit flashStarted(QStringLiteral("SEGGER J-Link"));
    emit flashOutput(QString("====== 启动 SEGGER J-Link 烧录: %1 ======\n").arg(QFileInfo(firmwarePath).fileName()), false);

    QString device = m_profile.jlinkDeviceName.isEmpty() ? "STM32F103C8" : m_profile.jlinkDeviceName;

    // J-Link Commander on Windows reads command files using the system code page.
    // Stage firmware in a short temporary path so Chinese project paths and long paths work.
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString tempDir = QDir::tempPath();
    const QString suffix = QFileInfo(firmwarePath).suffix().toLower();
    m_tempFirmwarePath = QDir(tempDir).filePath("stm32_flash_" + id + "." + suffix);
    if (!QFile::copy(firmwarePath, m_tempFirmwarePath)) {
        emit flashOutput(QStringLiteral("错误: 无法暂存固件到临时目录，请检查磁盘空间和访问权限。\n"), true);
        m_tempFirmwarePath.clear();
        return false;
    }
    m_tempScriptPath = QDir(tempDir).filePath("jlink_flash_" + id + ".jlink");

    QFile scriptFile(m_tempScriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit flashOutput("无法创建 J-Link 临时控制脚本: " + scriptFile.errorString() + "\n", true);
        QFile::remove(m_tempFirmwarePath);
        m_tempFirmwarePath.clear();
        m_tempScriptPath.clear();
        return false;
    }

    const QByteArray script = jLinkCommandScript(m_tempFirmwarePath, options, device).toLocal8Bit();
    if (scriptFile.write(script) != script.size()) {
        emit flashOutput(QStringLiteral("错误: 写入 J-Link 控制脚本失败。\n"), true);
        scriptFile.close();
        QFile::remove(m_tempScriptPath);
        QFile::remove(m_tempFirmwarePath);
        m_tempScriptPath.clear();
        m_tempFirmwarePath.clear();
        return false;
    }
    scriptFile.close();

    const QStringList args = jLinkArguments(options, device, m_tempScriptPath);

    m_process->setWorkingDirectory(QFileInfo(firmwarePath).absolutePath());
    emit flashOutput(QString("执行: %1 %2\n\n").arg(m_paths.jlinkPath, args.join(" ")), false);
    m_process->start(m_paths.jlinkPath, args);

    if (m_process->waitForStarted(3000)) return true;
    emit flashOutput(QString("错误: 无法启动 J-Link: %1\n").arg(m_process->errorString()), true);
    QFile::remove(m_tempScriptPath);
    QFile::remove(m_tempFirmwarePath);
    m_tempScriptPath.clear();
    m_tempFirmwarePath.clear();
    return false;
}

bool FlashProgrammer::flashViaSTM32Cube(const QString &firmwarePath, const FlashOptions &options)
{
    if (m_paths.stm32ProgPath.isEmpty() || !QFile::exists(m_paths.stm32ProgPath)) {
        emit flashOutput(QStringLiteral("错误: 未配置有效的 STM32_Programmer_CLI 路径!\n"), true);
        return false;
    }

    emit flashStarted(QStringLiteral("STM32CubeProgrammer"));
    emit flashOutput(QString("====== 启动 STM32CubeProgrammer 烧录 ======\n"), false);

    const QStringList args = cubeProgrammerArguments(firmwarePath, options);

    m_process->start(m_paths.stm32ProgPath, args);
    return m_process->waitForStarted(3000);
}

bool FlashProgrammer::flashViaStFlash(const QString &firmwarePath, const FlashOptions &options)
{
    QString stflash = "st-flash";
    QStringList args;
    if (!options.probeSerial.isEmpty()) args << "--serial" << options.probeSerial;
    args << ("--freq=" + QString::number(options.swdSpeedKhz) + "k");
    if (options.resetAndRun) {
        args << "--reset";
    }
    args << "write" << firmwarePath << options.targetAddress;

    m_process->start(stflash, args);
    return m_process->waitForStarted(3000);
}

void FlashProgrammer::onProcessReadyReadStd()
{
    QByteArray bytes = m_process->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(bytes);
    emit flashOutput(text, false);

    // 进度启发式匹配
    if (text.contains("Programming Finished", Qt::CaseInsensitive) || text.contains("** Verified OK **")) {
        emit flashProgress(100, QStringLiteral("烧录校验完成"));
    } else if (text.contains("Writing build/", Qt::CaseInsensitive) || text.contains("Downloading file")) {
        emit flashProgress(60, QStringLiteral("正在下载固件..."));
    } else if (text.contains("Erasing", Qt::CaseInsensitive)) {
        emit flashProgress(20, QStringLiteral("正在擦除 Flash 扇区..."));
    }
}

void FlashProgrammer::onProcessReadyReadErr()
{
    QByteArray bytes = m_process->readAllStandardError();
    QString text = QString::fromLocal8Bit(bytes);
    emit flashOutput(text, true);

    if (text.contains("** Verified OK **")) {
        emit flashProgress(100, QStringLiteral("烧录校验完成"));
    }
}

void FlashProgrammer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    int duration = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime);
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);

    // 清理临时脚本
    if (!m_tempScriptPath.isEmpty() && QFile::exists(m_tempScriptPath)) {
        QFile::remove(m_tempScriptPath);
        m_tempScriptPath.clear();
    }
    if (!m_tempFirmwarePath.isEmpty()) {
        QFile::remove(m_tempFirmwarePath);
        m_tempFirmwarePath.clear();
    }

    if (success) {
        emit flashProgress(100, QStringLiteral("烧录成功"));
        emit flashOutput(QString("\n🎉 固件下载与烧录成功! 耗时: %1 秒\n\n").arg(duration / 1000.0, 0, 'f', 2), false);
        emit flashFinished(true, QStringLiteral("烧录成功完成"));
    } else {
        emit flashOutput(QString("\n❌ 固件烧录失败! 退出码: %1, 耗时: %2 秒\n\n").arg(exitCode).arg(duration / 1000.0, 0, 'f', 2), true);
        emit flashFinished(false, QString("烧录失败，退出代码: %1").arg(exitCode));
    }
}
