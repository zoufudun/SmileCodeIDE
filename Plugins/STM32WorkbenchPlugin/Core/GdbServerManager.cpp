#include "GdbServerManager.h"
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

GdbServerManager::GdbServerManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &GdbServerManager::onProcessReadyReadStd);
    connect(m_process, &QProcess::readyReadStandardError, this, &GdbServerManager::onProcessReadyReadErr);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &GdbServerManager::onProcessFinished);
}

GdbServerManager::~GdbServerManager()
{
    stopServer();
}

bool GdbServerManager::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool GdbServerManager::isPortInUse(int port, int timeoutMs)
{
    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", static_cast<quint16>(port));
    bool connected = socket.waitForConnected(timeoutMs);
    if (connected) {
        socket.disconnectFromHost();
        return true;
    }
    return false;
}

bool GdbServerManager::startServer(GdbServerType type,
                                   const QString &customInterface,
                                   const QString &customTarget,
                                   int port)
{
    if (isRunning()) {
        emit serverOutput(QStringLiteral("GDB Server 已在运行中\n"), false);
        return true;
    }

    m_gdbPort = port;
    m_isReady = false;

    // 先检查端口是否已被外部现有进程监听（若已有，直接复用并报告就绪）
    if (isPortInUse(port, 200)) {
        emit serverOutput(QString("检测到端口 %1 已处于监听状态，直接复用现有 GDB Server 服务\n").arg(port), false);
        m_isReady = true;
        emit serverReady(port);
        return true;
    }

    if (type == GdbServerType::OpenOCD) {
        if (m_paths.openocdPath.isEmpty() || !QFile::exists(m_paths.openocdPath)) {
            emit serverError(QStringLiteral("未配置有效的 OpenOCD 路径"));
            return false;
        }

        QString interfaceCfg = customInterface.isEmpty() ? "interface/stlink.cfg" : customInterface;
        QString targetCfg = customTarget.isEmpty() ?
            (m_profile.openocdTargetCfg.isEmpty() ? STM32ProfileManager::getOpenocdTarget(m_profile.family) : m_profile.openocdTargetCfg) :
            customTarget;

        QStringList args;
        if (!m_paths.openocdScriptsDir.isEmpty() && QDir(m_paths.openocdScriptsDir).exists()) {
            args << "-s" << m_paths.openocdScriptsDir;
        }
        args << "-f" << interfaceCfg;
        args << "-f" << targetCfg;
        args << "-c" << QString("gdb_port %1").arg(port);

        emit serverOutput(QString("启动 OpenOCD 服务端: %1 %2\n").arg(m_paths.openocdPath, args.join(" ")), false);
        m_process->start(m_paths.openocdPath, args);

        if (!m_process->waitForStarted(2000)) {
            emit serverError("无法启动 OpenOCD 进程: " + m_process->errorString());
            return false;
        }
    } else {
        // J-Link GDB Server
        QString jlinkServer = "JLinkGDBServerCL.exe";
        QString jlinkDir;
        if (!m_paths.jlinkPath.isEmpty()) {
            jlinkDir = QFileInfo(m_paths.jlinkPath).absolutePath();
            QString directExe = QDir(jlinkDir).absoluteFilePath("JLinkGDBServerCL.exe");
            if (QFile::exists(directExe)) jlinkServer = directExe;
        }

        QStringList args;
        args << "-select" << "USB"
             << "-device" << (m_profile.jlinkDeviceName.isEmpty() ? "STM32F103C8" : m_profile.jlinkDeviceName)
             << "-endian" << "little"
             << "-if" << "SWD"
             << "-speed" << "4000"
             << "-port" << QString::number(port);

        emit serverOutput(QString("启动 J-Link GDB Server: %1 %2\n").arg(jlinkServer, args.join(" ")), false);
        m_process->start(jlinkServer, args);

        if (!m_process->waitForStarted(2000)) {
            emit serverError("无法启动 J-Link GDB Server: " + m_process->errorString());
            return false;
        }
    }

    emit serverStarted();

    // 设定心跳保底探测定时器，每 300ms 检测端口是否就绪
    QTimer *readyCheckTimer = new QTimer(this);
    connect(readyCheckTimer, &QTimer::timeout, this, [this, readyCheckTimer, port]() {
        if (!isRunning() || m_isReady) {
            readyCheckTimer->stop();
            readyCheckTimer->deleteLater();
            return;
        }
        if (isPortInUse(port, 100)) {
            m_isReady = true;
            readyCheckTimer->stop();
            readyCheckTimer->deleteLater();
            emit serverReady(port);
            emit serverOutput(QString("✅ GDB Server 调试端口 %1 监听就绪!\n").arg(port), false);
        }
    });
    readyCheckTimer->start(300);

    return true;
}

void GdbServerManager::stopServer()
{
    if (isRunning()) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
        emit serverOutput(QStringLiteral("\n[GDB Server 服务端已终止]\n"), false);
    }
    m_isReady = false;
}

void GdbServerManager::onProcessReadyReadStd()
{
    QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    emit serverOutput(text, false);
    checkServerReadyOutput(text);
}

void GdbServerManager::onProcessReadyReadErr()
{
    QString text = QString::fromLocal8Bit(m_process->readAllStandardError());
    emit serverOutput(text, true);
    checkServerReadyOutput(text);
}

void GdbServerManager::checkServerReadyOutput(const QString &text)
{
    if (!m_isReady) {
        if (text.contains("Listening on port", Qt::CaseInsensitive) ||
            text.contains("Waiting for GDB connection", Qt::CaseInsensitive)) {
            m_isReady = true;
            emit serverReady(m_gdbPort);
            emit serverOutput(QString("✅ GDB Server 服务已就绪 (端口 %1)\n").arg(m_gdbPort), false);
        }
    }
}

void GdbServerManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    m_isReady = false;
    emit serverStopped(exitCode);
}
