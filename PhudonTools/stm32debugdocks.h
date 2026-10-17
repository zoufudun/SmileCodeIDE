#ifndef STM32DEBUGDOCKS_H
#define STM32DEBUGDOCKS_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMap>
#include <QSet>
#include "GdbMiEngine.h"

/**
 * @brief 调用栈帧展示面板
 */
class CallStackDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit CallStackDock(QWidget *parent = nullptr);

    void setFrames(const QVector<GdbStackFrame> &frames);
    void clear();

signals:
    void frameSelected(int level, const QString &file, int line);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *m_treeWidget;
    QVector<GdbStackFrame> m_frames;
};

/**
 * @brief 局部变量与监视变量面板
 */
class VariablesDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit VariablesDock(QWidget *parent = nullptr);

    void setLocals(const QVector<GdbVariable> &variables);
    void updateWatchResult(const QString &expr, const QString &value);
    void clear();
    QStringList watchList() const { return m_watchExpressions; }

signals:
    void watchRequested(const QString &expr);
    void watchRemoved(const QString &expr);

private slots:
    void onAddWatchClicked();
    void onWatchContextMenu(const QPoint &pos);

private:
    QTabWidget *m_tabWidget;
    QTableWidget *m_localsTable;
    QTableWidget *m_watchTable;
    QLineEdit *m_watchInput;
    QPushButton *m_addWatchBtn;

    QMap<QString, QString> m_prevLocals;
    QStringList m_watchExpressions;
};

/**
 * @brief CPU 核心寄存器面板
 */
class RegistersDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit RegistersDock(QWidget *parent = nullptr);

    void setRegisters(const QVector<GdbRegister> &registers);
    void clear();

private:
    QTableWidget *m_table;
    QMap<QString, QString> m_prevRegisters;
};

/**
 * @brief 断点集中管理面板
 */
class BreakpointsDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit BreakpointsDock(QWidget *parent = nullptr);

    void updateBreakpoints(const QMap<QString, QSet<int>> &breakpoints);
    void clear();

signals:
    void breakpointJumpRequested(const QString &file, int line);
    void clearAllBreakpointsRequested();

private slots:
    void onItemDoubleClicked(int row, int col);

private:
    QTableWidget *m_table;
    QPushButton *m_clearAllBtn;
    struct BreakpointEntry {
        QString file;
        int line;
    };
    QList<BreakpointEntry> m_entries;
};

#endif // STM32DEBUGDOCKS_H
