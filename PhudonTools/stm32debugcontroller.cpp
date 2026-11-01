#include "stm32debugcontroller.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

STM32DebugController *STM32DebugController::s_instance = nullptr;

STM32DebugController *STM32DebugController::instance()
{
    return s_instance;
}

STM32DebugController::STM32DebugController(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    m_toolchainDetector = new ToolchainDetector(this);
    m_toolchainPaths = m_toolchainDetector->autoDetect();

    m_currentProfile = STM32ProfileManager::instance().getProfile("STM32F103C8");

    m_buildManager = new BuildManager(this);
    m_flashProgrammer = new FlashProgrammer(this);
    m_gdbServerManager = new GdbServerManager(this);
    m_gdbEngine = new GdbMiEngine(this);

    m_probeTimer = new QTimer(this);
    m_probeTimer->setInterval(100);

    applyToolchainAndChip();

    // 构建系统信号转发
    connect(m_buildManager, &BuildManager::buildStarted, this, &STM32DebugController::buildStarted);
    connect(m_buildManager, &BuildManager::buildOutput, this, &STM32DebugController::buildOutput);
    connect(m_buildManager, &BuildManager::diagnosticFound, this, &STM32DebugController::diagnosticFound);
    connect(m_buildManager, &BuildManager::memoryUsageUpdated, this, &STM32DebugController::memoryUsageUpdated);
    connect(m_buildManager, &BuildManager::buildFinished, this, &STM32DebugController::onBuildFinished);

    // Connect before preflight checks and only once per controller lifetime.
    connect(m_flashProgrammer, &FlashProgrammer::flashOutput,
            this, &STM32DebugController::flashOutput);
    connect(m_flashProgrammer, &FlashProgrammer::flashFinished, this,
            [this](bool success, const QString &message) {
        if (m_state == STM32SessionState::Flashing) {
            setState(STM32SessionState::Idle);
        }
        emit flashFinished(success, message);
    });

    // GDB Server 信号
    connect(m_gdbServerManager, &GdbServerManager::serverOutput, this, [this](const QString &text, bool isError) {
        Q_UNUSED(isError);
        emit gdbConsoleOutput(QString("[OpenOCD] %1").arg(text));
    });
    connect(m_gdbServerManager, &GdbServerManager::serverReady, this, &STM32DebugController::onServerReady);
    connect(m_gdbServerManager, &GdbServerManager::serverStopped, this, &STM32DebugController::onServerStopped);
    connect(m_gdbServerManager, &GdbServerManager::serverError, this, &STM32DebugController::onServerError);

    // GDB/MI 引擎信号
    connect(m_gdbEngine, &GdbMiEngine::stateChanged, this, &STM32DebugController::onGdbStateChanged);
    connect(m_gdbEngine, &GdbMiEngine::targetStopped, this, &STM32DebugController::onGdbTargetStopped);
    connect(m_gdbEngine, &GdbMiEngine::targetRunning, this, &STM32DebugController::onGdbTargetRunning);
    connect(m_gdbEngine, &GdbMiEngine::stackTraceReceived, this, &STM32DebugController::stackTraceReceived);
    connect(m_gdbEngine, &GdbMiEngine::localsReceived, this, &STM32DebugController::localsReceived);
    connect(m_gdbEngine, &GdbMiEngine::registersReceived, this, &STM32DebugController::registersReceived);
    connect(m_gdbEngine, &GdbMiEngine::expressionEvaluated, this, &STM32DebugController::expressionEvaluated);
    connect(m_gdbEngine, &GdbMiEngine::consoleOutput, this, &STM32DebugController::gdbConsoleOutput);
    connect(m_gdbEngine, &GdbMiEngine::sessionFinished, this, &STM32DebugController::onGdbSessionFinished);

    connect(m_probeTimer, &QTimer::timeout, this, &STM32DebugController::onProbePollTimeout);
}

STM32DebugController::~STM32DebugController()
{
    closeProject();
    // Child engine destructors emit completion signals during QObject teardown.
    disconnect(m_gdbEngine, nullptr, this, nullptr);
    disconnect(m_gdbServerManager, nullptr, this, nullptr);
    disconnect(m_buildManager, nullptr, this, nullptr);
    disconnect(m_flashProgrammer, nullptr, this, nullptr);
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void STM32DebugController::setState(STM32SessionState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void STM32DebugController::applyToolchainAndChip()
{
    m_buildManager->setToolchainPaths(m_toolchainPaths);
    m_buildManager->setChipProfile(m_currentProfile);

    m_flashProgrammer->setToolchainPaths(m_toolchainPaths);
    m_flashProgrammer->setChipProfile(m_currentProfile);

    m_gdbServerManager->setToolchainPaths(m_toolchainPaths);
    m_gdbServerManager->setChipProfile(m_currentProfile);
}

void STM32DebugController::setTargetChip(const QString &chipName)
{
    if (m_state != STM32SessionState::Idle) return;
    if (chipName.isEmpty() || chipName == m_currentProfile.chipName) return;
    STM32ChipProfile profile = STM32ProfileManager::instance().getProfile(chipName);
    if (!profile.chipName.isEmpty()) {
        m_currentProfile = profile;
        if (m_hasFlashOptions) {
            m_flashOptions.targetAddress = profile.flashOrigin;
            m_flashOptions.customOpenocdTargetCfg.clear();
        }
        applyToolchainAndChip();
        saveProjectConfig();
        emit chipChanged(m_currentProfile.chipName);
    }
}

void STM32DebugController::setProbeName(const QString &probeName)
{
    if (m_state != STM32SessionState::Idle) return;
    if (m_probeName != probeName) {
        auto category = [](QString name) {
            name = name.toUpper().remove('-').remove(' ');
            return name.contains("JLINK") ? 1 : name.contains("DAP") ? 2 : 0;
        };
        if (m_hasFlashOptions && category(m_probeName) != category(probeName)) {
            m_flashOptions.driver = category(probeName) == 1 ? FlashDriverType::JLink_Native
                : category(probeName) == 2 ? FlashDriverType::OpenOCD_CMSISDAP
                                         : FlashDriverType::OpenOCD_STLink;
            m_flashOptions.probeSerial.clear();
            m_flashOptions.probeInstanceId.clear();
        }
        m_probeName = probeName;
        saveProjectConfig();
        emit probeChanged(m_probeName);
    }
}

void STM32DebugController::setFlashOptions(const FlashOptions &options)
{
    if (m_state != STM32SessionState::Idle) return;
    const int driver = static_cast<int>(options.driver);
    if (driver < 0 || driver > static_cast<int>(FlashDriverType::StFlash) ||
        !FlashProgrammer::validTargetAddress(options.targetAddress) ||
        options.swdSpeedKhz < 100 || options.swdSpeedKhz > 20000) return;
    m_flashOptions = options;
    m_hasFlashOptions = true;
    m_probeName = options.driver == FlashDriverType::JLink_Native ||
                  options.driver == FlashDriverType::OpenOCD_JLink ? QStringLiteral("J-Link")
                : options.driver == FlashDriverType::OpenOCD_CMSISDAP ? QStringLiteral("CMSIS-DAP")
                                                                     : QStringLiteral("ST-Link");
    saveProjectConfig();
    emit probeChanged(m_probeName);
}

void STM32DebugController::setCustomGccPath(const QString &path)
{
    m_toolchainPaths.gccPath = path;
    applyToolchainAndChip();
    saveProjectConfig();
}

void STM32DebugController::setCustomOpenocdPath(const QString &path)
{
    m_toolchainPaths.openocdPath = path;
    applyToolchainAndChip();
    saveProjectConfig();
}

void STM32DebugController::openProject(const QString &projectDir)
{
    if (projectDir.isEmpty()) return;
    QString resolved = QFileInfo(projectDir).canonicalFilePath();
    if (resolved.isEmpty()) resolved = QFileInfo(projectDir).absoluteFilePath();
    if (!QFileInfo(resolved).isDir() || resolved == m_projectDir) return;
    closeProject();
    m_projectDir = resolved;
    m_hasFlashOptions = false;
    m_flashOptions = FlashOptions();
    m_probeName = QStringLiteral("ST-Link");
    m_currentProfile = STM32ProfileManager::instance().getProfile("STM32F103C8");
    applyToolchainAndChip();

    if (!loadProjectConfig(m_projectDir)) {
        QString detectedChip = autoDetectChipFromProject(m_projectDir);
        if (!detectedChip.isEmpty()) {
            setTargetChip(detectedChip);
        }
        saveProjectConfig();
    }

    setState(STM32SessionState::Idle);
    emit chipChanged(m_currentProfile.chipName);
    emit probeChanged(m_probeName);
}

void STM32DebugController::closeProject()
{
    cancelCurrentTask();
    if (hasProject()) saveProjectConfig();
    m_projectDir.clear();
    m_breakpoints.clear();
    emit breakpointsChanged(m_breakpoints);
    setState(STM32SessionState::Idle);
}

bool STM32DebugController::buildProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) {
        return false;
    }
    setState(STM32SessionState::Building);
    const bool started = m_buildManager->startBuild(m_projectDir, BuildConfiguration::Debug, 4);
    if (!started) setState(STM32SessionState::Idle);
    return started;
}

bool STM32DebugController::cleanProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) return false;
    setState(STM32SessionState::Building);
    const bool started = m_buildManager->startClean(m_projectDir);
    if (!started) setState(STM32SessionState::Idle);
    return started;
}

bool STM32DebugController::rebuildProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) {
        return false;
    }
    setState(STM32SessionState::Building);
    const bool started = m_buildManager->startRebuild(m_projectDir, BuildConfiguration::Debug, 4);
    if (!started) setState(STM32SessionState::Idle);
    return started;
}

void STM32DebugController::stopBuild()
{
    if (m_state == STM32SessionState::Building) {
        m_buildManager->stopBuild();
        if (!m_buildManager->isBusy()) setState(STM32SessionState::Idle);
    }
}

void STM32DebugController::cancelCurrentTask()
{
    if (m_state == STM32SessionState::Building) {
        stopBuild();
    } else if (m_state == STM32SessionState::Flashing) {
        m_flashProgrammer->cancelFlash();
        if (!m_flashProgrammer->isBusy()) setState(STM32SessionState::Idle);
    } else {
        stopDebug();
    }
}

void STM32DebugController::onBuildFinished(bool success, int exitCode, int durationMs)
{
    if (m_state == STM32SessionState::Building) {
        setState(STM32SessionState::Idle);
    }
    emit buildFinished(success, exitCode, durationMs);
}

FlashOptions STM32DebugController::resolveFlashOptions() const
{
    if (m_hasFlashOptions) return m_flashOptions;
    FlashOptions opt;
    QString probe = m_probeName.toUpper();
    probe.remove('-').remove(' ');
    if (probe.contains("JLINK")) {
        opt.driver = FlashDriverType::JLink_Native;
    } else if (m_probeName.contains("DAP", Qt::CaseInsensitive)) {
        opt.driver = FlashDriverType::OpenOCD_CMSISDAP;
    } else {
        opt.driver = FlashDriverType::OpenOCD_STLink;
    }
    opt.targetAddress = m_currentProfile.flashOrigin;
    opt.verify = true;
    opt.resetAndRun = true;
    opt.eraseAll = false;
    opt.customOpenocdTargetCfg = m_currentProfile.openocdTargetCfg;
    return opt;
}

bool STM32DebugController::flashProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) {
        return false;
    }
    QString binPath = m_buildManager->getBinPath(m_projectDir);
    if (binPath.isEmpty() || !QFile::exists(binPath)) {
        QString elfPath = m_buildManager->getElfPath(m_projectDir);
        if (elfPath.isEmpty() || !QFile::exists(elfPath)) {
            emit flashFinished(false, tr("找不到可烧录的固件文件 (bin/elf)，请先编译项目！"));
            return false;
        }
        binPath = elfPath;
    }

    stopDebug();
    if (m_gdbServerManager->isRunning()) {
        emit flashFinished(false, tr("调试服务尚未退出，请稍后重试下载。"));
        return false;
    }
    setState(STM32SessionState::Flashing);
    emit flashStarted();

    FlashOptions opt = resolveFlashOptions();
    bool started = m_flashProgrammer->flashFirmware(binPath, opt);
    if (!started) {
        setState(STM32SessionState::Idle);
        emit flashFinished(false, tr("启动烧录程序失败！"));
        return false;
    }

    return true;
}

QString STM32DebugController::resolveElfPath() const
{
    QString elf = m_buildManager->getElfPath(m_projectDir);
    if (QFile::exists(elf)) return elf;
    return QString();
}

bool STM32DebugController::startDebug()
{
    if (m_projectDir.isEmpty()) return false;
    if (m_state == STM32SessionState::Stopped || m_state == STM32SessionState::Running) {
        continueDebug();
        return true;
    }
    if (m_state != STM32SessionState::Idle) return false;

    QString elfPath = resolveElfPath();
    if (elfPath.isEmpty()) {
        emit gdbConsoleOutput(tr("错误: 目标 ELF 文件不存在，请先执行 F7 编译构建！"));
        return false;
    }

    setState(STM32SessionState::StartingDebug);
    emit gdbConsoleOutput(tr(">>> 启动硬件在线调试会话..."));

    const FlashOptions flashOptions = resolveFlashOptions();
    GdbServerType srvType = flashOptions.driver == FlashDriverType::JLink_Native
                                ? GdbServerType::JLinkGdbServer
                                : GdbServerType::OpenOCD;
    m_targetGdbPort = (srvType == GdbServerType::JLinkGdbServer) ? 2331 : 3333;

    // 启动 GDB Server
    m_startingGdbClient = false;
    QString interfaceCfg = flashOptions.customOpenocdInterfaceCfg;
    if (interfaceCfg.isEmpty()) {
        interfaceCfg = flashOptions.driver == FlashDriverType::OpenOCD_CMSISDAP
            ? QStringLiteral("interface/cmsis-dap.cfg")
            : flashOptions.driver == FlashDriverType::OpenOCD_JLink
                ? QStringLiteral("interface/jlink.cfg") : QStringLiteral("interface/stlink.cfg");
    }
    const QString targetCfg = flashOptions.customOpenocdTargetCfg.isEmpty()
        ? m_currentProfile.openocdTargetCfg : flashOptions.customOpenocdTargetCfg;
    bool ok = m_gdbServerManager->startServer(srvType, interfaceCfg,
                                              targetCfg, m_targetGdbPort,
                                              flashOptions.probeSerial, flashOptions.swdSpeedKhz);
    if (!ok) {
        setState(STM32SessionState::Idle);
        emit gdbConsoleOutput(tr("错误: 无法启动 GDB Server 后台进程！"));
        return false;
    }

    // 启动非阻塞探活定时器
    if (m_state == STM32SessionState::StartingDebug && !m_startingGdbClient) {
        m_probeAttempts = 0;
        m_probeTimer->start();
    }
    return true;
}

void STM32DebugController::onProbePollTimeout()
{
    if (m_state != STM32SessionState::StartingDebug || m_startingGdbClient) {
        m_probeTimer->stop();
        return;
    }
    m_probeAttempts++;
    if (GdbServerManager::isPortInUse(m_targetGdbPort, 80)) {
        m_probeTimer->stop();
        emit gdbConsoleOutput(tr(">>> GDB Server 端口 %1 探活就绪，启动 GDB Client 握手...").arg(m_targetGdbPort));
        startGdbClient(m_targetGdbPort);
        return;
    }

    if (m_probeAttempts >= 50) { // 5秒超时
        m_probeTimer->stop();
        stopDebug();
        emit gdbConsoleOutput(tr("错误: GDB Server 启动超时（5000ms 未监听端口），调试中止！"));
    }
}

void STM32DebugController::onServerReady(int port)
{
    if (m_state == STM32SessionState::StartingDebug) {
        m_probeTimer->stop();
        startGdbClient(port);
    }
}

void STM32DebugController::startGdbClient(int port)
{
    if (m_state != STM32SessionState::StartingDebug || m_startingGdbClient) return;
    m_startingGdbClient = true;
    QString gdbPath = m_toolchainPaths.gdbPath;
    if (gdbPath.isEmpty()) gdbPath = "arm-none-eabi-gdb";

    QString elfPath = resolveElfPath();

    bool ok = m_gdbEngine->startSession(gdbPath, elfPath, "127.0.0.1", port);
    if (!ok) {
        m_gdbServerManager->stopServer();
        setState(STM32SessionState::Idle);
        emit gdbConsoleOutput(tr("错误: 无法启动 arm-none-eabi-gdb 客户端！"));
        return;
    }

    // 握手并同步断点
    m_gdbEngine->resetHalt();

    // 下发全部已记录断点
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it) {
        const QString &file = it.key();
        for (int line : it.value()) {
            m_gdbEngine->insertBreakpoint(file, line);
        }
    }

    // 在 main 设置临时断点并运行
    m_gdbEngine->sendRawCommand("-break-insert -t main");
    m_gdbEngine->continueExec();
}

void STM32DebugController::stopDebug()
{
    if (m_stoppingDebug) return;
    const bool active = m_state == STM32SessionState::StartingDebug ||
                        m_state == STM32SessionState::Stopped ||
                        m_state == STM32SessionState::Running;
    if (!active && !m_gdbServerManager->isRunning() && !m_gdbEngine->isConnected()) return;
    m_stoppingDebug = true;
    m_probeTimer->stop();
    m_gdbEngine->stopSession();
    m_gdbServerManager->stopServer();
    m_startingGdbClient = false;
    setState(STM32SessionState::Idle);
    emit debugSessionEnded();
    m_stoppingDebug = false;
}

void STM32DebugController::continueDebug()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->continueExec();
    }
}

void STM32DebugController::pauseDebug()
{
    if (m_state == STM32SessionState::Running) {
        m_gdbEngine->pauseExec();
    }
}

void STM32DebugController::stepOver()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->stepOver();
    }
}

void STM32DebugController::stepInto()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->stepInto();
    }
}

void STM32DebugController::stepOut()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->stepOut();
    }
}

void STM32DebugController::resetHalt()
{
    if (m_gdbEngine->isConnected()) {
        m_gdbEngine->resetHalt();
    }
}

void STM32DebugController::executeGdbCommand(const QString &cmd)
{
    if (m_gdbEngine->isConnected()) {
        m_gdbEngine->sendRawCommand(cmd);
    }
}

void STM32DebugController::setAllBreakpoints(const QMap<QString, QSet<int>> &breakpoints)
{
    m_breakpoints = breakpoints;
    emit breakpointsChanged(m_breakpoints);
}

void STM32DebugController::toggleBreakpoint(const QString &file, int line, bool added)
{
    QString canonical = QFileInfo(file).canonicalFilePath();
    if (canonical.isEmpty()) canonical = QFileInfo(file).absoluteFilePath();

    if (added) {
        m_breakpoints[canonical].insert(line);
        if (m_gdbEngine->isConnected()) {
            m_gdbEngine->insertBreakpoint(canonical, line);
        }
    } else {
        m_breakpoints[canonical].remove(line);
        if (m_breakpoints[canonical].isEmpty()) {
            m_breakpoints.remove(canonical);
        }
        if (m_gdbEngine->isConnected()) {
            m_gdbEngine->toggleBreakpoint(canonical, line);
        }
    }
    emit breakpointsChanged(m_breakpoints);
}

void STM32DebugController::requestLocals()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->requestLocals();
    }
}

void STM32DebugController::requestStackTrace()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->requestStackTrace();
    }
}

void STM32DebugController::requestRegisters()
{
    if (m_state == STM32SessionState::Stopped) {
        m_gdbEngine->requestRegisters();
    }
}

void STM32DebugController::evaluateWatchExpression(const QString &expr)
{
    if (m_gdbEngine->isConnected() && !expr.trimmed().isEmpty()) {
        m_gdbEngine->evaluateExpression(expr);
    }
}

void STM32DebugController::onGdbStateChanged(GdbTargetState state)
{
    if (state == GdbTargetState::Stopped) {
        setState(STM32SessionState::Stopped);
    } else if (state == GdbTargetState::Running) {
        setState(STM32SessionState::Running);
    } else if (state == GdbTargetState::Disconnected || state == GdbTargetState::Exited) {
        if (m_state == STM32SessionState::StartingDebug ||
            m_state == STM32SessionState::Stopped ||
            m_state == STM32SessionState::Running) {
            stopDebug();
        }
    }
}

void STM32DebugController::onGdbTargetStopped(const QString &file, int line, const QString &func, const QString &reason)
{
    setState(STM32SessionState::Stopped);
    emit targetStopped(file, line, func, reason);
    // 停机时自动请求上下文数据刷新
    m_gdbEngine->requestStackTrace();
    m_gdbEngine->requestLocals();
    m_gdbEngine->requestRegisters();
}

void STM32DebugController::onGdbTargetRunning()
{
    setState(STM32SessionState::Running);
    emit targetRunning();
}

void STM32DebugController::onGdbSessionFinished()
{
    stopDebug();
}

void STM32DebugController::onServerStopped(int exitCode)
{
    Q_UNUSED(exitCode);
    if (m_state == STM32SessionState::StartingDebug ||
        m_state == STM32SessionState::Stopped ||
        m_state == STM32SessionState::Running) {
        stopDebug();
    }
}

void STM32DebugController::onServerError(const QString &error)
{
    emit gdbConsoleOutput(QString("[OpenOCD Error] %1").arg(error));
}

bool STM32DebugController::loadProjectConfig(const QString &projectDir)
{
    QString configPath = projectDir + "/.smilecode/project.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject obj = doc.object();
    QString chip = obj.value("targetChip").toString();
    if (!chip.isEmpty()) {
        const STM32ChipProfile profile = STM32ProfileManager::instance().getProfile(chip);
        if (!profile.chipName.isEmpty()) {
            m_currentProfile = profile;
            applyToolchainAndChip();
        }
    }
    QString probe = obj.value("probe").toString();
    if (!probe.isEmpty()) {
        m_probeName = probe;
    }
    const QJsonObject flash = obj.value("flash").toObject();
    if (!flash.isEmpty()) {
        FlashOptions options;
        options.driver = static_cast<FlashDriverType>(flash.value("driver").toInt());
        options.targetAddress = flash.value("address").toString(m_currentProfile.flashOrigin);
        options.swdSpeedKhz = flash.value("speedKhz").toInt(4000);
        options.verify = flash.value("verify").toBool(true);
        options.resetAndRun = flash.value("resetAndRun").toBool(true);
        options.eraseAll = flash.value("eraseAll").toBool(false);
        options.probeSerial = flash.value("probeSerial").toString();
        options.probeInstanceId = flash.value("probeInstanceId").toString();
        options.customOpenocdInterfaceCfg = flash.value("interfaceCfg").toString();
        options.customOpenocdTargetCfg = flash.value("targetCfg").toString();
        if (static_cast<int>(options.driver) >= 0 && static_cast<int>(options.driver) <= 5 &&
            options.swdSpeedKhz >= 100 && options.swdSpeedKhz <= 20000 &&
            FlashProgrammer::validTargetAddress(options.targetAddress)) {
            m_flashOptions = options;
            m_hasFlashOptions = true;
        }
    }
    return true;
}

bool STM32DebugController::saveProjectConfig()
{
    if (m_projectDir.isEmpty()) return false;
    QString dirPath = m_projectDir + "/.smilecode";
    QDir().mkpath(dirPath);

    QJsonObject obj;
    QFile previous(dirPath + "/project.json");
    if (previous.open(QIODevice::ReadOnly)) {
        obj = QJsonDocument::fromJson(previous.readAll()).object();
    }
    obj["projectType"] = "STM32";
    obj["targetChip"] = m_currentProfile.chipName;
    obj["series"] = m_currentProfile.family;
    obj["core"] = m_currentProfile.core;
    obj["probe"] = m_probeName;
    obj["openocdTarget"] = m_currentProfile.openocdTargetCfg;
    const FlashOptions flash = resolveFlashOptions();
    obj["flashAddress"] = flash.targetAddress;
    QJsonObject flashJson;
    flashJson["driver"] = static_cast<int>(flash.driver);
    flashJson["address"] = flash.targetAddress;
    flashJson["speedKhz"] = flash.swdSpeedKhz;
    flashJson["verify"] = flash.verify;
    flashJson["resetAndRun"] = flash.resetAndRun;
    flashJson["eraseAll"] = flash.eraseAll;
    flashJson["probeSerial"] = flash.probeSerial;
    flashJson["probeInstanceId"] = flash.probeInstanceId;
    flashJson["interfaceCfg"] = flash.customOpenocdInterfaceCfg;
    flashJson["targetCfg"] = flash.customOpenocdTargetCfg;
    obj["flash"] = flashJson;

    QJsonDocument doc(obj);
    QFile file(dirPath + "/project.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
    return false;
}

QString STM32DebugController::autoDetectChipFromProject(const QString &projectDir)
{
    QDir dir(projectDir);

    // 1. 扫描 CubeMX .ioc 文件
    QStringList iocs = dir.entryList(QStringList() << "*.ioc", QDir::Files);
    if (!iocs.isEmpty()) {
        QFile file(dir.filePath(iocs.first()));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            QRegularExpression re(R"(Mcu\.Name\s*=\s*(STM32[A-Z0-9]+))");
            auto match = re.match(content);
            if (match.hasMatch()) {
                QString name = match.captured(1);
                STM32ChipProfile p = STM32ProfileManager::instance().getProfile(name);
                if (!p.chipName.isEmpty()) return p.chipName;
                for (const QString &chip : STM32ProfileManager::instance().supportedChips()) {
                    if (name.startsWith(chip, Qt::CaseInsensitive)) {
                        return chip;
                    }
                }
            }
        }
    }

    // 2. 扫描启动汇编文件 (startup_stm32f103xb.s 等)
    QDir startupDir(projectDir + "/Startup");
    if (!startupDir.exists()) startupDir = dir;
    QStringList asmFiles = startupDir.entryList(QStringList() << "startup_stm32*.s" << "startup_stm32*.S", QDir::Files);
    if (!asmFiles.isEmpty()) {
        QString asmName = asmFiles.first().toLower();
        if (asmName.contains("f103")) return "STM32F103C8";
        if (asmName.contains("f407")) return "STM32F407VG";
        if (asmName.contains("f429")) return "STM32F429ZI";
        if (asmName.contains("h743")) return "STM32H743VI";
    }

    // 默认回退
    return "STM32F103C8";
}
