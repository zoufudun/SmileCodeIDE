#include "stm32debugdocks.h"
#include <QFileInfo>
#include <QFont>
#include <QColor>

// -------------------------------------------------------------
// CallStackDock 实现
// -------------------------------------------------------------
CallStackDock::CallStackDock(QWidget *parent)
    : QDockWidget(tr("调用栈 (Call Stack)"), parent)
{
    setObjectName("CallStackDock");
    setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << tr("层级") << tr("函数") << tr("文件") << tr("行号") << tr("地址"));
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_treeWidget->header()->setStretchLastSection(true);

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &CallStackDock::onItemDoubleClicked);

    setWidget(m_treeWidget);
}

void CallStackDock::setFrames(const QVector<GdbStackFrame> &frames)
{
    m_frames = frames;
    m_treeWidget->clear();

    for (const auto &f : frames) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, QString::number(f.level));
        item->setText(1, f.func);
        item->setText(2, QFileInfo(f.file).fileName());
        item->setText(3, QString::number(f.line));
        item->setText(4, f.address);
        item->setData(0, Qt::UserRole, f.level);
        item->setData(1, Qt::UserRole, f.fullname.isEmpty() ? f.file : f.fullname);
        item->setData(2, Qt::UserRole, f.line);
    }
}

void CallStackDock::clear()
{
    m_frames.clear();
    m_treeWidget->clear();
}

void CallStackDock::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    int level = item->data(0, Qt::UserRole).toInt();
    QString file = item->data(1, Qt::UserRole).toString();
    int line = item->data(2, Qt::UserRole).toInt();
    emit frameSelected(level, file, line);
}

// -------------------------------------------------------------
// VariablesDock 实现
// -------------------------------------------------------------
VariablesDock::VariablesDock(QWidget *parent)
    : QDockWidget(tr("变量与监视 (Variables & Watch)"), parent)
{
    setObjectName("VariablesDock");
    setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_tabWidget = new QTabWidget(this);

    // Tab 1: 局部变量
    m_localsTable = new QTableWidget(this);
    m_localsTable->setColumnCount(3);
    m_localsTable->setHorizontalHeaderLabels(QStringList() << tr("变量名") << tr("类型") << tr("值"));
    m_localsTable->horizontalHeader()->setStretchLastSection(true);
    m_localsTable->setAlternatingRowColors(true);
    m_localsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_localsTable->verticalHeader()->setVisible(false);

    // Tab 2: 表达式监视
    QWidget *watchContainer = new QWidget(this);
    QVBoxLayout *watchLayout = new QVBoxLayout(watchContainer);
    watchLayout->setContentsMargins(4, 4, 4, 4);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_watchInput = new QLineEdit(watchContainer);
    m_watchInput->setPlaceholderText(tr("输入监视表达式，例如: s_rxCount 或 huart1.gState"));
    m_addWatchBtn = new QPushButton(tr("添加监视"), watchContainer);
    inputLayout->addWidget(m_watchInput);
    inputLayout->addWidget(m_addWatchBtn);
    watchLayout->addLayout(inputLayout);

    m_watchTable = new QTableWidget(watchContainer);
    m_watchTable->setColumnCount(2);
    m_watchTable->setHorizontalHeaderLabels(QStringList() << tr("表达式") << tr("计算值"));
    m_watchTable->horizontalHeader()->setStretchLastSection(true);
    m_watchTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_watchTable->verticalHeader()->setVisible(false);
    watchLayout->addWidget(m_watchTable);

    connect(m_addWatchBtn, &QPushButton::clicked, this, &VariablesDock::onAddWatchClicked);
    connect(m_watchInput, &QLineEdit::returnPressed, this, &VariablesDock::onAddWatchClicked);
    connect(m_watchTable, &QTableWidget::customContextMenuRequested, this, &VariablesDock::onWatchContextMenu);

    m_tabWidget->addTab(m_localsTable, tr("局部变量 (Locals)"));
    m_tabWidget->addTab(watchContainer, tr("表达式监视 (Watch)"));

    setWidget(m_tabWidget);
}

void VariablesDock::setLocals(const QVector<GdbVariable> &variables)
{
    m_localsTable->setRowCount(variables.size());
    for (int i = 0; i < variables.size(); ++i) {
        const auto &v = variables[i];
        QTableWidgetItem *nameItem = new QTableWidgetItem(v.name);
        QTableWidgetItem *typeItem = new QTableWidgetItem(v.type);
        QTableWidgetItem *valItem = new QTableWidgetItem(v.value);

        // 如果数值发生变动，亮橙色突出显示
        if (m_prevLocals.contains(v.name) && m_prevLocals[v.name] != v.value) {
            valItem->setForeground(QColor("#FF9800"));
            QFont f = valItem->font();
            f.setBold(true);
            valItem->setFont(f);
        }

        m_prevLocals[v.name] = v.value;

        m_localsTable->setItem(i, 0, nameItem);
        m_localsTable->setItem(i, 1, typeItem);
        m_localsTable->setItem(i, 2, valItem);
    }

    // 触发所有监视表达式自动求值
    for (const QString &expr : m_watchExpressions) {
        emit watchRequested(expr);
    }
}

void VariablesDock::updateWatchResult(const QString &expr, const QString &value)
{
    for (int row = 0; row < m_watchTable->rowCount(); ++row) {
        if (m_watchTable->item(row, 0) && m_watchTable->item(row, 0)->text() == expr) {
            if (m_watchTable->item(row, 1)) {
                m_watchTable->item(row, 1)->setText(value);
            }
            return;
        }
    }
}

void VariablesDock::clear()
{
    m_localsTable->setRowCount(0);
    m_prevLocals.clear();
}

void VariablesDock::onAddWatchClicked()
{
    QString expr = m_watchInput->text().trimmed();
    if (expr.isEmpty() || m_watchExpressions.contains(expr)) return;

    m_watchExpressions.append(expr);
    int row = m_watchTable->rowCount();
    m_watchTable->insertRow(row);
    m_watchTable->setItem(row, 0, new QTableWidgetItem(expr));
    m_watchTable->setItem(row, 1, new QTableWidgetItem(tr("求值中...")));
    m_watchInput->clear();

    emit watchRequested(expr);
}

void VariablesDock::onWatchContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_watchTable->itemAt(pos);
    if (!item) return;
    int row = item->row();
    QString expr = m_watchTable->item(row, 0)->text();

    QMenu menu(this);
    QAction *delAct = menu.addAction(tr("删除监视项: %1").arg(expr));
    QAction *clearAct = menu.addAction(tr("清除全部监视"));

    QAction *selected = menu.exec(m_watchTable->viewport()->mapToGlobal(pos));
    if (selected == delAct) {
        m_watchExpressions.removeAll(expr);
        m_watchTable->removeRow(row);
        emit watchRemoved(expr);
    } else if (selected == clearAct) {
        m_watchExpressions.clear();
        m_watchTable->setRowCount(0);
    }
}

// -------------------------------------------------------------
// RegistersDock 实现
// -------------------------------------------------------------
RegistersDock::RegistersDock(QWidget *parent)
    : QDockWidget(tr("CPU 核心寄存器 (Registers)"), parent)
{
    setObjectName("RegistersDock");
    setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels(QStringList() << tr("寄存器") << tr("当前值 (Hex)"));
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);

    setWidget(m_table);
}

void RegistersDock::setRegisters(const QVector<GdbRegister> &registers)
{
    m_table->setRowCount(registers.size());
    for (int i = 0; i < registers.size(); ++i) {
        const auto &reg = registers[i];
        QTableWidgetItem *nameItem = new QTableWidgetItem(reg.name.toUpper());
        QTableWidgetItem *valItem = new QTableWidgetItem(reg.value);

        // 如果数值发生变动，亮青色/金黄高亮
        if (m_prevRegisters.contains(reg.name) && m_prevRegisters[reg.name] != reg.value) {
            valItem->setForeground(QColor("#00E5FF"));
            QFont f = valItem->font();
            f.setBold(true);
            valItem->setFont(f);
        }

        m_prevRegisters[reg.name] = reg.value;

        m_table->setItem(i, 0, nameItem);
        m_table->setItem(i, 1, valItem);
    }
}

void RegistersDock::clear()
{
    m_table->setRowCount(0);
    m_prevRegisters.clear();
}

// -------------------------------------------------------------
// BreakpointsDock 实现
// -------------------------------------------------------------
BreakpointsDock::BreakpointsDock(QWidget *parent)
    : QDockWidget(tr("断点列表 (Breakpoints)"), parent)
{
    setObjectName("BreakpointsDock");
    setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    m_table = new QTableWidget(container);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(QStringList() << tr("序号") << tr("源文件") << tr("行号"));
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    layout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_clearAllBtn = new QPushButton(tr("清空所有断点"), container);
    btnLayout->addStretch();
    btnLayout->addWidget(m_clearAllBtn);
    layout->addLayout(btnLayout);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &BreakpointsDock::onItemDoubleClicked);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &BreakpointsDock::clearAllBreakpointsRequested);

    setWidget(container);
}

void BreakpointsDock::updateBreakpoints(const QMap<QString, QSet<int>> &breakpoints)
{
    m_entries.clear();
    m_table->setRowCount(0);

    int idx = 1;
    for (auto it = breakpoints.begin(); it != breakpoints.end(); ++it) {
        const QString &file = it.key();
        for (int line : it.value()) {
            BreakpointEntry e{file, line};
            m_entries.append(e);

            int row = m_table->rowCount();
            m_table->insertRow(row);
            m_table->setItem(row, 0, new QTableWidgetItem(QString::number(idx++)));
            m_table->setItem(row, 1, new QTableWidgetItem(QFileInfo(file).fileName()));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::number(line)));
        }
    }
}

void BreakpointsDock::clear()
{
    m_entries.clear();
    m_table->setRowCount(0);
}

void BreakpointsDock::onItemDoubleClicked(int row, int col)
{
    Q_UNUSED(col);
    if (row >= 0 && row < m_entries.size()) {
        emit breakpointJumpRequested(m_entries[row].file, m_entries[row].line);
    }
}
