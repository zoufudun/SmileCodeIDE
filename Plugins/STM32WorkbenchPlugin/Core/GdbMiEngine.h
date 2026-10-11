#ifndef GDBMIENGINE_H
#define GDBMIENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QMap>
#include <QVector>

/**
 * @brief GDB 调试会话状态
 */
enum class GdbTargetState {
    Disconnected,   // 未连接
    Connecting,     // 连接中
    Connected,      // 已连接到目标硬件
    Running,        // 目标硬件运行中
    Stopped,        // 目标硬件已暂停 (命中断点或单步执行停住)
    Exited          // 调试会话结束
};

/**
 * @brief 断点信息
 */
struct GdbBreakpoint {
    int id = 0;
    QString file;
    int line = 0;
    QString address;
    QString func;
    bool enabled = true;
    int hitCount = 0;
};

/**
 * @brief 调用栈帧信息
 */
struct GdbStackFrame {
    int level = 0;
    QString func;
    QString file;
    QString fullname;
    int line = 0;
    QString address;
};

/**
 * @brief 局部变量与监视变量
 */
struct GdbVariable {
    QString name;
    QString value;
    QString type;
};

/**
 * @brief CPU 核心寄存器
 */
struct GdbRegister {
    int number = 0;
    QString name;
    QString value;
};

/**
 * @brief 基于 GDB/MI (Machine Interface) 协议的现代嵌入式异步调试引擎
 */
class GdbMiEngine : public QObject
{
    Q_OBJECT

public:
    explicit GdbMiEngine(QObject *parent = nullptr);
    ~GdbMiEngine() override;

    GdbTargetState state() const { return m_state; }
    bool isConnected() const { return m_state == GdbTargetState::Connected || m_state == GdbTargetState::Running || m_state == GdbTargetState::Stopped; }

    // 启动 GDB 客户端并连接到远程 GDB Server
    bool startSession(const QString &gdbPath,
                      const QString &elfPath,
                      const QString &host = "localhost",
                      int port = 3333);

    // 终止调试会话
    void stopSession();

    // 调试核心控制动作
    void continueExec();
    void pauseExec();
    void stepOver();
    void stepInto();
    void stepOut();
    void resetHalt();
    void loadFirmware();

    // 断点管理
    void insertBreakpoint(const QString &file, int line);
    void removeBreakpoint(int id);
    void toggleBreakpoint(const QString &file, int line);
    QVector<GdbBreakpoint> breakpoints() const { return m_breakpoints.values().toVector(); }

    // 查询指令
    void requestStackTrace();
    void requestLocals();
    void requestRegisters();
    void evaluateExpression(const QString &expr);

    // 发送原生 GDB 命令或 CLI 表达式
    void sendRawCommand(const QString &cmd);

signals:
    void stateChanged(GdbTargetState state);
    void consoleOutput(const QString &text);
    void targetRunning();
    void targetStopped(const QString &file, int line, const QString &func, const QString &reason);
    void breakpointChanged(const GdbBreakpoint &bkpt, bool added);
    void stackTraceReceived(const QVector<GdbStackFrame> &frames);
    void localsReceived(const QVector<GdbVariable> &variables);
    void registersReceived(const QVector<GdbRegister> &registers);
    void expressionEvaluated(const QString &expr, const QString &result);
    void sessionFinished();

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void sendMiCommand(const QString &cmd);
    void parseMiLine(const QString &line);
    void handleStoppedNotification(const QString &payload);
    void handleBreakpointCreated(const QString &payload);
    void handleStackTraceResult(const QString &payload);
    void handleLocalsResult(const QString &payload);
    void handleRegistersResult(const QString &payload);
    QString extractFieldValue(const QString &src, const QString &key);

    QProcess *m_process = nullptr;
    GdbTargetState m_state = GdbTargetState::Disconnected;
    QString m_elfPath;
    QString m_host;
    int m_port = 3333;
    QMap<int, GdbBreakpoint> m_breakpoints; // id -> Breakpoint
    int m_tokenCounter = 1;
};

#endif // GDBMIENGINE_H
