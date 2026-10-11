#ifndef GDBSERVERMANAGER_H
#define GDBSERVERMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTcpSocket>
#include "ToolchainDetector.h"
#include "STM32Profile.h"

enum class GdbServerType {
    OpenOCD,
    JLinkGdbServer
};

/**
 * @brief GDB 调试服务器 (OpenOCD / J-Link GDB Server) 守护进程管理器
 */
class GdbServerManager : public QObject
{
    Q_OBJECT

public:
    explicit GdbServerManager(QObject *parent = nullptr);
    ~GdbServerManager() override;

    void setToolchainPaths(const ToolchainPaths &paths) { m_paths = paths; }
    void setChipProfile(const STM32ChipProfile &profile) { m_profile = profile; }

    bool isRunning() const;
    int gdbPort() const { return m_gdbPort; }

    // 启动 GDB Server 守护进程并监听 3333 端口
    bool startServer(GdbServerType type = GdbServerType::OpenOCD,
                     const QString &customInterface = QString(),
                     const QString &customTarget = QString(),
                     int port = 3333);

    // 停止 GDB Server
    void stopServer();

    // 检测指定端口是否已有监听服务存活
    static bool isPortInUse(int port, int timeoutMs = 400);

signals:
    void serverStarted();
    void serverReady(int port);
    void serverOutput(const QString &text, bool isError);
    void serverStopped(int exitCode);
    void serverError(const QString &error);

private slots:
    void onProcessReadyReadStd();
    void onProcessReadyReadErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void checkServerReadyOutput(const QString &text);

    ToolchainPaths m_paths;
    STM32ChipProfile m_profile;
    QProcess *m_process = nullptr;
    int m_gdbPort = 3333;
    bool m_isReady = false;
};

#endif // GDBSERVERMANAGER_H
