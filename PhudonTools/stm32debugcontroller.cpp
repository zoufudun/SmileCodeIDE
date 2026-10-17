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
    stopDebug();
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
    if (chipName.isEmpty() || chipName == m_currentProfile.chipName) return;
    STM32ChipProfile profile = STM32ProfileManager::instance().getProfile(chipName);
    if (!profile.chipName.isEmpty()) {
        m_currentProfile = profile;
        applyToolchainAndChip();
        saveProjectConfig();
        emit chipChanged(m_currentProfile.chipName);
    }
}

void STM32DebugController::setProbeName(const QString &probeName)
{
    if (m_probeName != probeName) {
        m_probeName = probeName;
        saveProjectConfig();
        emit probeChanged(m_probeName);
    }
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
    m_projectDir = QFileInfo(projectDir).canonicalFilePath();
    if (m_projectDir.isEmpty()) {
        m_projectDir = QFileInfo(projectDir).absoluteFilePath();
    }

    if (!loadProjectConfig(m_projectDir)) {
        QString detectedChip = autoDetectChipFromProject(m_projectDir);
        if (!detectedChip.isEmpty()) {
            setTargetChip(detectedChip);
        }
        saveProjectConfig();
    }

    setState(STM32SessionState::Idle);
}

void STM32DebugController::closeProject()
{
    if (m_state != STM32SessionState::Idle) {
        stopDebug();
        m_buildManager->stopBuild();
    }
    m_projectDir.clear();
    setState(STM32SessionState::Idle);
}

bool STM32DebugController::buildProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) {
        return false;
    }
    setState(STM32SessionState::Building);
    return m_buildManager->startBuild(m_projectDir, BuildConfiguration::Debug, 4);
}

bool STM32DebugController::cleanProject()
{
    if (m_projectDir.isEmpty()) return false;
    return m_buildManager->startClean(m_projectDir);
}

bool STM32DebugController::rebuildProject()
{
    if (m_projectDir.isEmpty() || m_state != STM32SessionState::Idle) {
        return false;
    }
    setState(STM32SessionState::Building);
    return m_buildManager->startRebuild(m_projectDir, BuildConfiguration::Debug, 4);
}

void STM32DebugController::stopBuild()
{
    if (m_state == STM32SessionState::Building) {
        m_buildManager->stopBuild();
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
    FlashOptions opt;
    if (m_probeName.contains("JLINK", Qt::CaseInsensitive)) {
        opt.driver = FlashDriverType::JLink_Native;
    } else if (m_probeName.contains("DAP", Qt::CaseInsensitive)) {
        opt.driver = FlashDriverType::OpenOCD_CMSISDAP;
    } else {
        opt.driver = FlashDriverType::OpenOCD_STLink;
    }
    opt.targetAddress = "0x08000000";
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

    setState(STM32SessionState::Flashing);
    emit flashStarted();

    FlashOptions opt = resolveFlashOptions();
    bool started = m_flashProgrammer->flashFirmware(binPath, opt);
    if (!started) {
        setState(STM32SessionState::Idle);
        emit flashFinished(false, tr("启动烧录程序失败！"));
        return false;
    }

    // FlashProgrammer 异步执行，完成时复位状态
    connect(m_flashProgrammer, &FlashProgrammer::flashFinished, this, [this](bool success, const QString &msg) {
        if (m_state == STM32SessionState::Flashing) {
            setState(STM32SessionState::Idle);
        }
        emit flashFinished(success, msg);
    }, Qt::UniqueConnection);

    connect(m_flashProgrammer, &FlashProgrammer::flashOutput, this, [this](const QString &text, bool isErr) {
        emit flashOutput(text, isErr);
    }, Qt::UniqueConnection);

    return true;
}

QString STM32DebugController::resolveElfPath() const
{
    QString elf = m_buildManager->getElfPath(m_projectDir);
    if (QFile::exists(elf)) return elf;

    // 备用：搜索 build 目录下的任意 .elf
    QDir buildDir(m_projectDir + "/build");
    QStringList elfs = buildDir.entryList(QStringList() << "*.elf", QDir::Files);
    if (!elfs.isEmpty()) {
        return buildDir.filePath(elfs.first());
    }
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

    GdbServerType srvType = m_probeName.contains("JLINK", Qt::CaseInsensitive)
                                ? GdbServerType::JLinkGdbServer
                                : GdbServerType::OpenOCD;
    m_targetGdbPort = (srvType == GdbServerType::JLinkGdbServer) ? 2331 : 3333;

    // 启动 GDB Server
    bool ok = m_gdbServerManager->startServer(srvType, QString(),
                                              m_currentProfile.openocdTargetCfg, m_targetGdbPort);
    if (!ok) {
        setState(STM32SessionState::Idle);
        emit gdbConsoleOutput(tr("错误: 无法启动 GDB Server 后台进程！"));
        return false;
    }

    // 启动非阻塞探活定时器
    m_probeAttempts = 0;
    m_probeTimer->start();
    return true;
}

void STM32DebugController::onProbePollTimeout()
{
    m_probeAttempts++;
    if (GdbServerManager::isPortInUse(m_targetGdbPort, 80)) {
        m_probeTimer->stop();
        emit gdbConsoleOutput(tr(">>> GDB Server 端口 %1 探活就绪，启动 GDB Client 握手...").arg(m_targetGdbPort));
        startGdbClient(m_targetGdbPort);
        return;
    }

    if (m_probeAttempts >= 50) { // 5秒超时
        m_probeTimer->stop();
        m_gdbServerManager->stopServer();
        setState(STM32SessionState::Idle);
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
    m_gdbEngine->insertBreakpoint("main", 0);
    m_gdbEngine->continueExec();
}

void STM32DebugController::stopDebug()
{
    m_probeTimer->stop();
    m_gdbEngine->stopSession();
    m_gdbServerManager->stopServer();
    setState(STM32SessionState::Idle);
    emit debugSessionEnded();
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
        if (m_gdbEngine->isConnected()) {
            m_gdbEngine->toggleBreakpoint(canonical, line);
        }
    }
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
        setTargetChip(chip);
    }
    QString probe = obj.value("probe").toString();
    if (!probe.isEmpty()) {
        m_probeName = probe;
    }
    return true;
}

bool STM32DebugController::saveProjectConfig()
{
    if (m_projectDir.isEmpty()) return false;
    QString dirPath = m_projectDir + "/.smilecode";
    QDir().mkpath(dirPath);

    QJsonObject obj;
    obj["projectType"] = "STM32";
    obj["targetChip"] = m_currentProfile.chipName;
    obj["series"] = m_currentProfile.family;
    obj["core"] = m_currentProfile.core;
    obj["probe"] = m_probeName;
    obj["openocdTarget"] = m_currentProfile.openocdTargetCfg;
    obj["flashAddress"] = "0x08000000";

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
