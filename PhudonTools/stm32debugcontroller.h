#ifndef STM32DEBUGCONTROLLER_H
#define STM32DEBUGCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QTimer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include "ToolchainDetector.h"
#include "STM32Profile.h"
#include "BuildManager.h"
#include "BuildDiagnostic.h"
#include "FlashProgrammer.h"
#include "ProbeDetector.h"
#include "GdbServerManager.h"
#include "GdbMiEngine.h"

/**
 * @brief STM32 全生命周期会话状态
 */
enum class STM32SessionState {
    Idle,               // 空闲
    Building,           // 构建中
    Flashing,           // 烧录中
    StartingDebug,      // 调试服务启动与探活中
    Stopped,            // 调试停机 (命中断点或单步停下)
    Running             // 调试全速运行中
};

/**
 * @brief STM32 编译构建与 GDB 硬件在线调试总控中心
 */
class STM32DebugController : public QObject
{
    Q_OBJECT

public:
    explicit STM32DebugController(QObject *parent = nullptr);
    ~STM32DebugController() override;

    static STM32DebugController *instance();

    // 项目工程生命周期
    void openProject(const QString &projectDir);
    void closeProject();
    QString currentProjectDir() const { return m_projectDir; }
    bool hasProject() const { return !m_projectDir.isEmpty(); }

    // 状态与配置获取
    STM32SessionState state() const { return m_state; }
    STM32ChipProfile currentProfile() const { return m_currentProfile; }
    QString targetChip() const { return m_currentProfile.chipName; }
    QString probeName() const { return m_probeName; }
    ToolchainPaths toolchainPaths() const { return m_toolchainPaths; }
    BuildManager *buildManager() const { return m_buildManager; }

    void setTargetChip(const QString &chipName);
    void setProbeName(const QString &probeName);
    void setCustomGccPath(const QString &path);
    void setCustomOpenocdPath(const QString &path);
    FlashOptions flashOptions() const { return resolveFlashOptions(); }
    void setFlashOptions(const FlashOptions &options);

    void setProjectPath(const QString &path) { openProject(path); }
    void setProbeType(ProbeType type) {
        setProbeName(type == ProbeType::JLink ? "J-Link" :
                     type == ProbeType::CmsisDap ? "CMSIS-DAP" : "ST-Link");
    }
    void setToolchainPaths(const ToolchainPaths &paths) {
        m_toolchainPaths = paths;
        setCustomGccPath(paths.gccPath);
        setCustomOpenocdPath(paths.openocdPath);
    }
    const QMap<QString, QSet<int>>& allBreakpoints() const { return m_breakpoints; }
    const QMap<QString, QSet<int>>& breakpoints() const { return m_breakpoints; }
    void addBreakpoint(const QString &file, int line) { toggleBreakpoint(file, line, true); }
    void removeBreakpoint(const QString &file, int line) { toggleBreakpoint(file, line, false); }

    // 构建与烧录
    bool buildProject();
    bool cleanProject();
    bool rebuildProject();
    bool flashProject();
    void stopBuild();
    void cancelCurrentTask();

    // 硬件 GDB 调试控制
    bool startDebug();
    void stopDebug();
    void continueDebug();
    void pauseDebug();
    void stepOver();
    void stepInto();
    void stepOut();
    void resetHalt();
    void executeGdbCommand(const QString &cmd);

    // 断点双向同步
    void setAllBreakpoints(const QMap<QString, QSet<int>> &breakpoints);
    void toggleBreakpoint(const QString &file, int line, bool added);

    // 查询刷新
    void requestLocals();
    void requestStackTrace();
    void requestRegisters();
    void evaluateWatchExpression(const QString &expr);

    // 项目配置持久化
    bool loadProjectConfig(const QString &projectDir);
    bool saveProjectConfig();
    QString autoDetectChipFromProject(const QString &projectDir);

signals:
    void stateChanged(STM32SessionState state);
    void chipChanged(const QString &chipName);
    void probeChanged(const QString &probeName);
    void breakpointsChanged(const QMap<QString, QSet<int>> &breakpoints);

    // 构建信号
    void buildStarted(const QString &action);
    void buildOutput(const QString &text, bool isError);
    void buildLogChanged();
    void diagnosticFound(const BuildDiagnostic &diag);
    void memoryUsageUpdated(quint32 flashUsed, quint32 flashTotal,
                            quint32 ramUsed, quint32 ramTotal);
    void buildFinished(bool success, int exitCode, int durationMs);

    // 烧录信号
    void flashStarted();
    void flashOutput(const QString &text, bool isError);
    void flashFinished(bool success, const QString &message);

    // 调试信号
    void targetStopped(const QString &file, int line, const QString &func, const QString &reason);
    void targetRunning();
    void stackTraceReceived(const QVector<GdbStackFrame> &frames);
    void localsReceived(const QVector<GdbVariable> &variables);
    void registersReceived(const QVector<GdbRegister> &registers);
    void expressionEvaluated(const QString &expr, const QString &result);
    void gdbConsoleOutput(const QString &text);
    void debugSessionEnded();

private slots:
    void onBuildFinished(bool success, int exitCode, int durationMs);
    void onServerReady(int port);
    void onServerStopped(int exitCode);
    void onServerError(const QString &error);
    void onGdbStateChanged(GdbTargetState state);
    void onGdbTargetStopped(const QString &file, int line, const QString &func, const QString &reason);
    void onGdbTargetRunning();
    void onGdbSessionFinished();
    void onProbePollTimeout();

private:
    void setState(STM32SessionState state);
    void applyToolchainAndChip();
    QString resolveElfPath() const;
    void startGdbClient(int port);
    FlashOptions resolveFlashOptions() const;

    static STM32DebugController *s_instance;

    STM32SessionState m_state = STM32SessionState::Idle;
    QString m_projectDir;
    QString m_probeName = "STLINK";

    ToolchainDetector *m_toolchainDetector = nullptr;
    ToolchainPaths m_toolchainPaths;
    STM32ChipProfile m_currentProfile;

    BuildManager *m_buildManager = nullptr;
    FlashProgrammer *m_flashProgrammer = nullptr;
    GdbServerManager *m_gdbServerManager = nullptr;
    GdbMiEngine *m_gdbEngine = nullptr;

    // 探活定时器
    QTimer *m_probeTimer = nullptr;
    int m_targetGdbPort = 3333;
    int m_probeAttempts = 0;
    bool m_stoppingDebug = false;
    bool m_startingGdbClient = false;
    FlashOptions m_flashOptions;
    bool m_hasFlashOptions = false;

    // 断点记录 (规范化绝对路径 -> 1-based 行号集合)
    QMap<QString, QSet<int>> m_breakpoints;
};

#endif // STM32DEBUGCONTROLLER_H
