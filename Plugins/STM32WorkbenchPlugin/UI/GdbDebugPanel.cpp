#include "idetheme.h"
#include "GdbDebugPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

GdbDebugPanel::GdbDebugPanel(GdbMiEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine)
{
    setupUI();
    applyTheme();

    if (m_engine) {
        connect(m_engine, &GdbMiEngine::stateChanged, this, &GdbDebugPanel::updateActionButtons);
        connect(m_engine, &GdbMiEngine::targetRunning, this, &GdbDebugPanel::onTargetRunning);
        connect(m_engine, &GdbMiEngine::targetStopped, this, &GdbDebugPanel::onTargetStopped);
        connect(m_engine, &GdbMiEngine::stackTraceReceived, this, &GdbDebugPanel::onStackTraceReceived);
        connect(m_engine, &GdbMiEngine::localsReceived, this, &GdbDebugPanel::onLocalsReceived);
        connect(m_engine, &GdbMiEngine::registersReceived, this, &GdbDebugPanel::onRegistersReceived);
        connect(m_engine, &GdbMiEngine::breakpointChanged, this, &GdbDebugPanel::onBreakpointChanged);
        connect(m_engine, &GdbMiEngine::consoleOutput, this, &GdbDebugPanel::onConsoleOutput);
        connect(m_engine, &GdbMiEngine::expressionEvaluated, this, [this](const QString &, const QString &result) {
            m_exprResultLabel->setText(QString("求值结果: %1").arg(result));
        });
    }

    updateActionButtons(m_engine ? m_engine->state() : GdbTargetState::Disconnected);
}

void GdbDebugPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // 1. 顶部调试控制工具条
    QWidget *toolBarWidget = new QWidget(this);
    QHBoxLayout *tbLayout = new QHBoxLayout(toolBarWidget);
    tbLayout->setContentsMargins(4, 4, 4, 4);
    tbLayout->setSpacing(6);

    m_continueBtn = new QPushButton(QStringLiteral("▶ 继续 (F5)"), toolBarWidget);
    m_pauseBtn = new QPushButton(QStringLiteral("⏸ 暂停"), toolBarWidget);
    m_stepOverBtn = new QPushButton(QStringLiteral("↷ 单步跳过 (F10)"), toolBarWidget);
    m_stepIntoBtn = new QPushButton(QStringLiteral("⇊ 单步进入 (F11)"), toolBarWidget);
    m_stepOutBtn = new QPushButton(QStringLiteral("⇈ 单步跳出"), toolBarWidget);
    m_resetBtn = new QPushButton(QStringLiteral("🔄 复位停止"), toolBarWidget);
    m_downloadBtn = new QPushButton(QStringLiteral("⬇ 载入固件"), toolBarWidget);
    m_stopBtn = new QPushButton(QStringLiteral("⏹ 结束调试"), toolBarWidget);

    m_statusLabel = new QLabel(QStringLiteral("状态: 未连接"), toolBarWidget);
    m_statusLabel->setStyleSheet("color: #fab387; font-weight: bold; padding-left: 8px;");

    connect(m_continueBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::continueExec);
    connect(m_pauseBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::pauseExec);
    connect(m_stepOverBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::stepOver);
    connect(m_stepIntoBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::stepInto);
    connect(m_stepOutBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::stepOut);
    connect(m_resetBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::resetHalt);
    connect(m_downloadBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::loadFirmware);
    connect(m_stopBtn, &QPushButton::clicked, m_engine, &GdbMiEngine::stopSession);

    tbLayout->addWidget(m_continueBtn);
    tbLayout->addWidget(m_pauseBtn);
    tbLayout->addWidget(m_stepOverBtn);
    tbLayout->addWidget(m_stepIntoBtn);
    tbLayout->addWidget(m_stepOutBtn);
    tbLayout->addWidget(m_resetBtn);
    tbLayout->addWidget(m_downloadBtn);
    tbLayout->addWidget(m_stopBtn);
    tbLayout->addWidget(m_statusLabel);
    tbLayout->addStretch(1);

    mainLayout->addWidget(toolBarWidget);

    // 2. 多选项卡观测窗格
    m_tabWidget = new QTabWidget(this);

    // 2.1 调用栈表格
    m_stackTable = new QTableWidget(this);
    m_stackTable->setColumnCount(5);
    m_stackTable->setHorizontalHeaderLabels({QStringLiteral("栈帧"), QStringLiteral("函数名称"), QStringLiteral("源文件"), QStringLiteral("行号"), QStringLiteral("指令地址")});
    m_stackTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_stackTable->horizontalHeader()->setStretchLastSection(true);
    m_stackTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stackTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_stackTable, &QTableWidget::itemDoubleClicked, this, &GdbDebugPanel::onStackItemDoubleClicked);
    m_tabWidget->addTab(m_stackTable, QStringLiteral("调用栈 (Call Stack)"));

    // 2.2 局部变量与监视
    QWidget *localsContainer = new QWidget(this);
    QVBoxLayout *localsLayout = new QVBoxLayout(localsContainer);
    localsLayout->setContentsMargins(2, 2, 2, 2);

    m_localsTree = new QTreeWidget(localsContainer);
    m_localsTree->setHeaderLabels({QStringLiteral("变量名"), QStringLiteral("类型"), QStringLiteral("实时数值")});
    m_localsTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_localsTree->header()->setStretchLastSection(true);
    localsLayout->addWidget(m_localsTree);

    // 快速表达式求值条
    QHBoxLayout *exprLayout = new QHBoxLayout();
    m_exprInput = new QLineEdit(localsContainer);
    m_exprInput->setPlaceholderText(QStringLiteral("输入 C 表达式求值 (如: huart1.Instance->CR1, my_var)"));
    connect(m_exprInput, &QLineEdit::returnPressed, this, &GdbDebugPanel::onEvaluateExpression);
    QPushButton *evalBtn = new QPushButton(QStringLiteral("求值"), localsContainer);
    connect(evalBtn, &QPushButton::clicked, this, &GdbDebugPanel::onEvaluateExpression);
    m_exprResultLabel = new QLabel(QStringLiteral("结果: -"), localsContainer);
    exprLayout->addWidget(new QLabel(QStringLiteral("监视表达式:"), localsContainer));
    exprLayout->addWidget(m_exprInput, 1);
    exprLayout->addWidget(evalBtn);
    exprLayout->addWidget(m_exprResultLabel, 1);
    localsLayout->addLayout(exprLayout);

    m_tabWidget->addTab(localsContainer, QStringLiteral("局部变量与监视 (Locals & Watch)"));

    // 2.3 CPU 寄存器表格
    m_registersTable = new QTableWidget(this);
    m_registersTable->setColumnCount(3);
    m_registersTable->setHorizontalHeaderLabels({QStringLiteral("编号"), QStringLiteral("寄存器名称"), QStringLiteral("当前十六进制值")});
    m_registersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_registersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_registersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabWidget->addTab(m_registersTable, QStringLiteral("CPU 寄存器 (Registers)"));

    // 2.4 断点列表
    m_breakpointsTable = new QTableWidget(this);
    m_breakpointsTable->setColumnCount(5);
    m_breakpointsTable->setHorizontalHeaderLabels({QStringLiteral("编号"), QStringLiteral("源文件"), QStringLiteral("行号"), QStringLiteral("地址"), QStringLiteral("函数")});
    m_breakpointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_breakpointsTable->horizontalHeader()->setStretchLastSection(true);
    m_breakpointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_breakpointsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_breakpointsTable, &QTableWidget::itemDoubleClicked, this, &GdbDebugPanel::onBreakpointItemDoubleClicked);
    m_tabWidget->addTab(m_breakpointsTable, QStringLiteral("断点列表 (Breakpoints)"));

    // 2.5 GDB 交互控制台
    QWidget *consoleContainer = new QWidget(this);
    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleContainer);
    consoleLayout->setContentsMargins(2, 2, 2, 2);

    m_consoleOutput = new QTextEdit(consoleContainer);
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setFont(QFont("Consolas", 9));
    consoleLayout->addWidget(m_consoleOutput);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_consoleInput = new QLineEdit(consoleContainer);
    m_consoleInput->setPlaceholderText(QStringLiteral("输入原生 GDB 命令 (如: info registers, print my_var, x/10x 0x20000000)"));
    connect(m_consoleInput, &QLineEdit::returnPressed, this, &GdbDebugPanel::onSendConsoleCommand);
    QPushButton *sendBtn = new QPushButton(QStringLiteral("发送"), consoleContainer);
    connect(sendBtn, &QPushButton::clicked, this, &GdbDebugPanel::onSendConsoleCommand);

    inputLayout->addWidget(m_consoleInput, 1);
    inputLayout->addWidget(sendBtn);
    consoleLayout->addLayout(inputLayout);

    m_tabWidget->addTab(consoleContainer, QStringLiteral("GDB 控制台 (Console)"));

    mainLayout->addWidget(m_tabWidget, 1);
}

void GdbDebugPanel::applyTheme(const QString &themeId)
{
    if (!themeId.isEmpty()) {
        m_currentTheme = themeId;
    }
    const IdeTheme::ThemePalette p = IdeTheme::paletteFor(m_currentTheme);

    setStyleSheet(QString(
        "QWidget { background-color: %1; color: %2; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
        "QTabWidget::pane { border: 1px solid %3; background: %4; }"
        "QTabBar::tab { background: %5; color: %6; padding: 6px 14px; border: 1px solid %3; border-bottom: none; }"
        "QTabBar::tab:selected { background: %4; color: %2; font-weight: bold; border-top: 2px solid %7; }"
        "QTableWidget, QTreeWidget, QTextEdit { background-color: %4; border: 1px solid %3; color: %2; gridline-color: %3; }"
        "QHeaderView::section { background-color: %8; color: %6; border: 1px solid %3; padding: 4px; font-weight: bold; }"
        "QLineEdit { background-color: %9; border: 1px solid %10; border-radius: 4px; padding: 4px 8px; color: %2; }"
        "QPushButton { background: %11; color: %2; border: 1px solid %3; border-radius: 4px; padding: 4px 10px; font-size: 11px; }"
        "QPushButton:hover { background: %12; color: %13; border: 1px solid %7; }"
        "QPushButton:disabled { background-color: %1; color: %14; border-color: %3; }"
    ).arg(p.windowBg, p.textMain, p.border, p.cardBg, p.panelBg,
         p.textSub, p.accent, p.headerBg, p.inputBg, p.inputBorder,
         p.panelBgGrad, p.accentGrad, p.accentText, p.textDisabled));
}

void GdbDebugPanel::updateActionButtons(GdbTargetState state)
{
    bool isConnected = (state == GdbTargetState::Connected || state == GdbTargetState::Running || state == GdbTargetState::Stopped);
    bool isStopped = (state == GdbTargetState::Stopped || state == GdbTargetState::Connected);
    bool isRunning = (state == GdbTargetState::Running);

    m_continueBtn->setEnabled(isStopped);
    m_pauseBtn->setEnabled(isRunning);
    m_stepOverBtn->setEnabled(isStopped);
    m_stepIntoBtn->setEnabled(isStopped);
    m_stepOutBtn->setEnabled(isStopped);
    m_resetBtn->setEnabled(isConnected);
    m_downloadBtn->setEnabled(isStopped);
    m_stopBtn->setEnabled(isConnected);

    const IdeTheme::ThemePalette p = IdeTheme::paletteFor(m_currentTheme);
    switch (state) {
        case GdbTargetState::Disconnected:
            m_statusLabel->setText(QStringLiteral("状态: 未连接"));
            m_statusLabel->setStyleSheet(QString("color: %1;").arg(p.textDisabled));
            break;
        case GdbTargetState::Connecting:
            m_statusLabel->setText(QStringLiteral("状态: 连接硬件中..."));
            m_statusLabel->setStyleSheet(QString("color: %1;").arg(p.warning));
            break;
        case GdbTargetState::Connected:
            m_statusLabel->setText(QStringLiteral("状态: 已连接目标 (就绪)"));
            m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(p.success));
            break;
        case GdbTargetState::Running:
            m_statusLabel->setText(QStringLiteral("状态: 目标硬件运行中 ▶"));
            m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(p.accent));
            break;
        case GdbTargetState::Stopped:
            m_statusLabel->setText(QStringLiteral("状态: 已暂停 (断点/单步) ⏸"));
            m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(p.warning));
            break;
        case GdbTargetState::Exited:
            m_statusLabel->setText(QStringLiteral("状态: 会话已结束"));
            m_statusLabel->setStyleSheet(QString("color: %1;").arg(p.danger));
            break;
    }
}

void GdbDebugPanel::onTargetRunning()
{
    updateActionButtons(GdbTargetState::Running);
}

void GdbDebugPanel::onTargetStopped(const QString &file, int line, const QString &func, const QString &reason)
{
    Q_UNUSED(reason);
    updateActionButtons(GdbTargetState::Stopped);
    if (!file.isEmpty() && line > 0) {
        emit jumpToSourceLocation(file, line);
    }
}

void GdbDebugPanel::onStackTraceReceived(const QVector<GdbStackFrame> &frames)
{
    m_stackTable->setRowCount(0);
    for (int i = 0; i < frames.size(); ++i) {
        const auto &f = frames.at(i);
        int row = m_stackTable->rowCount();
        m_stackTable->insertRow(row);

        m_stackTable->setItem(row, 0, new QTableWidgetItem(QString("#%1").arg(f.level)));
        m_stackTable->setItem(row, 1, new QTableWidgetItem(f.func));
        m_stackTable->setItem(row, 2, new QTableWidgetItem(f.fullname.isEmpty() ? f.file : f.fullname));
        m_stackTable->setItem(row, 3, new QTableWidgetItem(QString::number(f.line)));
        m_stackTable->setItem(row, 4, new QTableWidgetItem(f.address));
    }
}

void GdbDebugPanel::onLocalsReceived(const QVector<GdbVariable> &variables)
{
    m_localsTree->clear();
    for (const auto &v : variables) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_localsTree);
        item->setText(0, v.name);
        item->setText(1, v.type);
        item->setText(2, v.value);
    }
}

void GdbDebugPanel::onRegistersReceived(const QVector<GdbRegister> &registers)
{
    m_registersTable->setRowCount(0);
    for (const auto &r : registers) {
        int row = m_registersTable->rowCount();
        m_registersTable->insertRow(row);

        m_registersTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.number)));
        m_registersTable->setItem(row, 1, new QTableWidgetItem(r.name));
        m_registersTable->setItem(row, 2, new QTableWidgetItem(r.value));
    }
}

void GdbDebugPanel::onBreakpointChanged(const GdbBreakpoint &bkpt, bool added)
{
    Q_UNUSED(added);
    // 重新从 engine 读取全部断点更新列表
    auto allBkpts = m_engine->breakpoints();
    m_breakpointsTable->setRowCount(0);
    for (const auto &b : allBkpts) {
        int row = m_breakpointsTable->rowCount();
        m_breakpointsTable->insertRow(row);

        m_breakpointsTable->setItem(row, 0, new QTableWidgetItem(QString::number(b.id)));
        m_breakpointsTable->setItem(row, 1, new QTableWidgetItem(b.file));
        m_breakpointsTable->setItem(row, 2, new QTableWidgetItem(QString::number(b.line)));
        m_breakpointsTable->setItem(row, 3, new QTableWidgetItem(b.address));
        m_breakpointsTable->setItem(row, 4, new QTableWidgetItem(b.func));
    }
}

void GdbDebugPanel::onConsoleOutput(const QString &text)
{
    m_consoleOutput->moveCursor(QTextCursor::End);
    m_consoleOutput->insertPlainText(text);
    m_consoleOutput->moveCursor(QTextCursor::End);
}

void GdbDebugPanel::onEvaluateExpression()
{
    QString expr = m_exprInput->text().trimmed();
    if (!expr.isEmpty() && m_engine) {
        m_engine->evaluateExpression(expr);
    }
}

void GdbDebugPanel::onSendConsoleCommand()
{
    QString cmd = m_consoleInput->text().trimmed();
    if (!cmd.isEmpty() && m_engine) {
        m_commandHistory.append(cmd);
        m_historyIndex = m_commandHistory.size();
        m_engine->sendRawCommand(cmd);
        m_consoleInput->clear();
    }
}

void GdbDebugPanel::onStackItemDoubleClicked(QTableWidgetItem *item)
{
    int row = item->row();
    QTableWidgetItem *fileItem = m_stackTable->item(row, 2);
    QTableWidgetItem *lineItem = m_stackTable->item(row, 3);
    if (fileItem && lineItem) {
        emit jumpToSourceLocation(fileItem->text(), lineItem->text().toInt());
    }
}

void GdbDebugPanel::onBreakpointItemDoubleClicked(QTableWidgetItem *item)
{
    int row = item->row();
    QTableWidgetItem *fileItem = m_breakpointsTable->item(row, 1);
    QTableWidgetItem *lineItem = m_breakpointsTable->item(row, 2);
    if (fileItem && lineItem) {
        emit jumpToSourceLocation(fileItem->text(), lineItem->text().toInt());
    }
}
