#ifndef GDBDEBUGPANEL_H
#define GDBDEBUGPANEL_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../Core/GdbMiEngine.h"

/**
 * @brief 现代化专业嵌入式 GDB 调试控制与观测面板
 */
class GdbDebugPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GdbDebugPanel(GdbMiEngine *engine, QWidget *parent = nullptr);
    ~GdbDebugPanel() override = default;

signals:
    void jumpToSourceLocation(const QString &file, int line);

private slots:
    void onTargetRunning();
    void onTargetStopped(const QString &file, int line, const QString &func, const QString &reason);
    void onStackTraceReceived(const QVector<GdbStackFrame> &frames);
    void onLocalsReceived(const QVector<GdbVariable> &variables);
    void onRegistersReceived(const QVector<GdbRegister> &registers);
    void onBreakpointChanged(const GdbBreakpoint &bkpt, bool added);
    void onConsoleOutput(const QString &text);
    void onEvaluateExpression();
    void onSendConsoleCommand();
    void onStackItemDoubleClicked(QTableWidgetItem *item);
    void onBreakpointItemDoubleClicked(QTableWidgetItem *item);

private:
    void setupUI();
    void applyTheme();
    void updateActionButtons(GdbTargetState state);

    GdbMiEngine *m_engine;

    // 动作按钮
    QPushButton *m_continueBtn;
    QPushButton *m_pauseBtn;
    QPushButton *m_stepOverBtn;
    QPushButton *m_stepIntoBtn;
    QPushButton *m_stepOutBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_downloadBtn;
    QPushButton *m_stopBtn;
    QLabel *m_statusLabel;

    // 观测视窗选项卡
    QTabWidget *m_tabWidget;
    QTableWidget *m_stackTable;
    QTreeWidget *m_localsTree;
    QTableWidget *m_registersTable;
    QTableWidget *m_breakpointsTable;

    // 监视表达式与控制台
    QLineEdit *m_exprInput;
    QLabel *m_exprResultLabel;
    QTextEdit *m_consoleOutput;
    QLineEdit *m_consoleInput;

    QStringList m_commandHistory;
    int m_historyIndex = 0;
};

#endif // GDBDEBUGPANEL_H
