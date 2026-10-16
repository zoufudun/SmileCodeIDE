#include "STM32WorkbenchWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QUrlQuery>
#include <QCompleter>
#include <QSignalBlocker>
#include <QHeaderView>
#include <QScrollBar>
#include <QShortcut>
#include "SourceNavigator.h"
#include "ToolchainConfigDialog.h"
#include "FlashConfigDialog.h"
#include "TargetChipDialog.h"

STM32WorkbenchWidget::STM32WorkbenchWidget(QWidget *parent, QSettings *settings)
    : QWidget(parent), m_settings(settings)
{
    // 初始化核心调度引擎
    m_detector = new ToolchainDetector(this);
    m_buildManager = new BuildManager(this);
    m_flashProgrammer = new FlashProgrammer(this);
    m_serverManager = new GdbServerManager(this);
    m_gdbEngine = new GdbMiEngine(this);

    setupUI();
    applyTheme();
    loadSettings();

    // 默认绑定当前芯片规格
    onChipChanged(m_chipCombo->currentText());

    // 绑定构建与烧录信号
    connect(m_buildManager, &BuildManager::buildOutput, this, &STM32WorkbenchWidget::onBuildOutput);
    connect(m_buildManager, &BuildManager::buildSessionStarted, this, &STM32WorkbenchWidget::beginBuildLog);
    connect(m_buildManager, &BuildManager::buildLogChanged, this, [this] {
        refreshBuildProblems();
        if (m_buildViewActive) renderBuildLog();
    });
    connect(m_buildManager, &BuildManager::buildStarted, this, [this](const QString &action) {
        if (!m_usingBuildLogModel) leaveFlashLog();
        m_taskStatusLabel->setText(action + QStringLiteral("..."));
        m_taskProgressBar->setValue(30);
    });
    connect(m_buildManager, &BuildManager::buildFinished, this, [this](bool success, int, int) {
        if (m_usingBuildLogModel && !m_buildViewActive) return;
        flushLogText();
        const bool clean = m_buildManager->currentAction().contains(QStringLiteral("清理"));
        m_taskStatusLabel->setText(clean ? (success ? QStringLiteral("清理完成") : QStringLiteral("清理失败"))
                                         : (success ? QStringLiteral("构建完成") : QStringLiteral("构建失败")));
        m_taskProgressBar->setValue(success ? 100 : 0);
        if (m_usingBuildLogModel && (!m_buildHasSize || !success)) {
            const QString result = clean ? QStringLiteral("已清理") : !success ? QStringLiteral("构建失败") : QStringLiteral("本次无尺寸数据");
            m_flashMemLabel->setText(QStringLiteral("Flash: ") + result);
            m_ramMemLabel->setText(QStringLiteral("RAM: ") + result);
        }
    });
    connect(m_buildManager, &BuildManager::legacyLinkerMigrationRequired, this,
            [this](const QString &linkerPath) {
        const int answer = QMessageBox::warning(
            this, QStringLiteral("迁移旧版链接脚本"),
            QStringLiteral("检测到旧版 SmileCodeIDE 自动生成的链接脚本：\n%1\n\n"
                           "继续将先创建 .bak 备份，再安装经过 SHA-256 校验的 CubeMX 原版脚本。"
                           "若备份或校验失败，原文件不会被替换。")
                .arg(linkerPath),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            m_taskStatusLabel->setText(QStringLiteral("已取消链接脚本迁移"));
            m_taskProgressBar->setValue(0);
            return;
        }
        QString backupPath;
        QString error;
        if (!m_buildManager->migrateLegacyLinkerScript(
                m_projectPath, &backupPath, &error)) {
            QMessageBox::critical(this, QStringLiteral("迁移失败"), error);
            m_taskStatusLabel->setText(QStringLiteral("链接脚本迁移失败"));
            m_taskProgressBar->setValue(0);
            return;
        }
        appendLogText(QStringLiteral("旧链接脚本已备份到: %1\n已安装 CubeMX 原版脚本，重新开始构建。\n")
                          .arg(backupPath), false);
        const BuildConfiguration cfg =
            static_cast<BuildConfiguration>(m_configCombo->currentData().toInt());
        if (m_retryRebuildAfterMigration)
            m_buildManager->startRebuild(m_projectPath, cfg);
        else
            m_buildManager->startBuild(m_projectPath, cfg);
    });
    connect(m_buildManager, &BuildManager::memoryUsageUpdated, this, &STM32WorkbenchWidget::onMemoryUsageUpdated);
    connect(m_flashProgrammer, &FlashProgrammer::flashOutput, this, &STM32WorkbenchWidget::onFlashOutput);
    connect(m_flashProgrammer, &FlashProgrammer::flashProgress, this, &STM32WorkbenchWidget::onFlashProgress);
    connect(m_flashProgrammer, &FlashProgrammer::flashSessionStarted,
            this, &STM32WorkbenchWidget::beginFlashLog);
    connect(m_flashProgrammer, &FlashProgrammer::flashStarted, this, [this](const QString &) {
        if (!m_flashActive) beginFlashLog(QString(), FlashOptions{});
    });
    connect(m_flashProgrammer, &FlashProgrammer::flashFinished,
            this, &STM32WorkbenchWidget::finishFlashLog);

    // 绑定调试服务器与客户端联动
    connect(m_serverManager, &GdbServerManager::serverOutput, this, [this](const QString &text, bool isError) {
        appendLogText("[GDB Server] " + text, isError);
    });
    connect(m_serverManager, &GdbServerManager::serverReady, this, &STM32WorkbenchWidget::onServerReady);
    connect(m_gdbEngine, &GdbMiEngine::stateChanged, this, &STM32WorkbenchWidget::onGdbStateChanged);
    connect(m_debugPanel, &GdbDebugPanel::jumpToSourceLocation, this, &STM32WorkbenchWidget::jumpToSourceLine);
}

STM32WorkbenchWidget::~STM32WorkbenchWidget()
{
    saveSettings();
}

void STM32WorkbenchWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. 顶部集成主工具栏
    QWidget *toolBar = new QWidget(this);
    toolBar->setStyleSheet("background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px;");
    QVBoxLayout *tbMainLayout = new QVBoxLayout(toolBar);
    tbMainLayout->setContentsMargins(8, 6, 8, 6);
    tbMainLayout->setSpacing(6);

    // 第一行：工程路径与芯片规格选择
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(8);

    QLabel *projLabel = new QLabel(QStringLiteral("📁 工程目录:"), toolBar);
    m_projectPathEdit = new QLineEdit(toolBar);
    m_projectPathEdit->setPlaceholderText(QStringLiteral("选择或打开包含 Makefile/Src 的 STM32 工程目录..."));
    m_browseProjectBtn = new QPushButton(QStringLiteral("浏览..."), toolBar);
    connect(m_browseProjectBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onBrowseProjectDir);

    QLabel *chipLabel = new QLabel(QStringLiteral("🎯 目标芯片:"), toolBar);
    m_chipCombo = new QComboBox(toolBar);
    m_chipCombo->addItems(STM32ProfileManager::instance().supportedChips());
    m_chipCombo->setEditable(true);
    m_chipCombo->setInsertPolicy(QComboBox::NoInsert);
    m_chipCombo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_chipCombo->completer()->setFilterMode(Qt::MatchContains);
    connect(m_chipCombo, &QComboBox::currentTextChanged, this, &STM32WorkbenchWidget::onChipChanged);

    m_selectChipBtn = new QPushButton(QStringLiteral("芯片选择..."), toolBar);
    m_selectChipBtn->setToolTip(QStringLiteral("打开目标芯片分类级联选择对话框 (厂商/系列/器件)"));
    m_selectChipBtn->setStyleSheet("background-color: #2f3542; color: #f1f2f6; padding: 4px 8px; border: 1px solid #57606f; border-radius: 4px; font-weight: bold;");
    connect(m_selectChipBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onOpenChipSelectDialog);

    QLabel *cfgLabel = new QLabel(QStringLiteral("⚙ 构建模式:"), toolBar);
    m_configCombo = new QComboBox(toolBar);
    m_configCombo->addItem(QStringLiteral("Debug (带调试符号 -g3 -O0)"), static_cast<int>(BuildConfiguration::Debug));
    m_configCombo->addItem(QStringLiteral("Release (高性能优化 -O2)"), static_cast<int>(BuildConfiguration::Release));
    m_configCombo->addItem(QStringLiteral("SizeOpt (极限代码体积 -Os)"), static_cast<int>(BuildConfiguration::SizeOpt));

    row1->addWidget(projLabel);
    row1->addWidget(m_projectPathEdit, 2);
    row1->addWidget(m_browseProjectBtn);
    row1->addWidget(chipLabel);
    row1->addWidget(m_chipCombo, 1);
    row1->addWidget(m_selectChipBtn);
    row1->addWidget(cfgLabel);
    row1->addWidget(m_configCombo, 1);
    tbMainLayout->addLayout(row1);

    // 第二行：构建与调试一键操作按钮组
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(6);

    m_buildBtn = new QPushButton(QStringLiteral("🔨 一键编译 (Build)"), toolBar);
    m_buildBtn->setStyleSheet("background-color: #3867d6; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_buildBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onBuildClicked);

    m_cleanBtn = new QPushButton(QStringLiteral("🧹 清理 (Clean)"), toolBar);
    connect(m_cleanBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onCleanClicked);

    m_rebuildBtn = new QPushButton(QStringLiteral("🔄 全量重新构建"), toolBar);
    connect(m_rebuildBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onRebuildClicked);

    m_flashBtn = new QPushButton(QStringLiteral("⚡ 一键烧录/下载 (Flash)"), toolBar);
    m_flashBtn->setStyleSheet("background-color: #fa8231; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_flashBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onFlashClicked);

    m_debugBtn = new QPushButton(QStringLiteral("🐞 启动 GDB 硬件调试"), toolBar);
    m_debugBtn->setStyleSheet("background-color: #20bf6b; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_debugBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onToggleDebugSession);

    m_toolchainConfigBtn = new QPushButton(QStringLiteral("🛠️ 工具链配置"), toolBar);
    connect(m_toolchainConfigBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onOpenToolchainSettings);

    m_flashConfigBtn = new QPushButton(QStringLiteral("🔥 烧录设置"), toolBar);
    connect(m_flashConfigBtn, &QPushButton::clicked, this, &STM32WorkbenchWidget::onOpenFlashSettings);

    row2->addWidget(m_buildBtn);
    row2->addWidget(m_cleanBtn);
    row2->addWidget(m_rebuildBtn);
    row2->addWidget(m_flashBtn);
    row2->addWidget(m_debugBtn);
    row2->addStretch(1);
    row2->addWidget(m_toolchainConfigBtn);
    row2->addWidget(m_flashConfigBtn);
    tbMainLayout->addLayout(row2);

    mainLayout->addWidget(toolBar);

    // 2. 主区域垂直分栏 (上：构建终端与内存消耗仪表盘；下：GDB 调试多维面板)
    m_mainSplitter = new QSplitter(Qt::Vertical, this);

    // 上半部分：构建输出控制台与硬件内存占用条
    QWidget *terminalContainer = new QWidget(m_mainSplitter);
    QVBoxLayout *tcLayout = new QVBoxLayout(terminalContainer);
    tcLayout->setContentsMargins(0, 0, 0, 0);
    tcLayout->setSpacing(4);

    auto *logBar = new QHBoxLayout;
    logBar->setContentsMargins(4, 0, 4, 0);
    logBar->addWidget(new QLabel(QStringLiteral("输出日志"), terminalContainer));
    logBar->addStretch();
    m_flashDetailedLog = new QCheckBox(QStringLiteral("详细日志"), terminalContainer);
    m_flashDetailedLog->setObjectName(QStringLiteral("flashDetailedLog"));
    m_flashDetailedLog->setToolTip(QStringLiteral("显示下载工具的完整输出和执行命令"));
    m_flashDetailedLog->setEnabled(false);
    connect(m_flashDetailedLog, &QCheckBox::toggled, this, [this] {
        if (m_showingFlashLog) renderFlashLog();
        else if (m_buildViewActive) renderBuildLog(true);
    });
    logBar->addWidget(m_flashDetailedLog);
    tcLayout->addLayout(logBar);

    m_outputTabs = new QTabWidget(terminalContainer);
    m_outputTabs->setObjectName(QStringLiteral("buildOutputTabs"));
    m_buildTerminal = new QTextBrowser(m_outputTabs);
    m_buildTerminal->setReadOnly(true);
    m_buildTerminal->setOpenLinks(false);
    connect(m_buildTerminal, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        if (!url.isLocalFile()) return;
        const int line = QUrlQuery(url).queryItemValue(QStringLiteral("line")).toInt();
        if (line > 0) navigateToSource(url.toLocalFile(), line,
            QUrlQuery(url).queryItemValue(QStringLiteral("column")).toInt());
    });
    m_buildTerminal->setFont(QFont("Consolas", 10));
    m_buildTerminal->setPlaceholderText(QStringLiteral("SmileCodeIDE 嵌入式构建与烧录日志终端已就绪..."));
    m_outputTabs->addTab(m_buildTerminal, QStringLiteral("编译 / 下载输出"));
    auto *problemsPage = new QWidget(m_outputTabs);
    auto *problemsLayout = new QVBoxLayout(problemsPage);
    problemsLayout->setContentsMargins(6, 6, 6, 6);
    auto *problemBar = new QHBoxLayout;
    m_problemFilter = new QComboBox(problemsPage);
    m_problemFilter->setObjectName(QStringLiteral("buildProblemFilter"));
    m_problemFilter->addItems({QStringLiteral("全部问题"), QStringLiteral("仅错误"), QStringLiteral("仅警告")});
    m_problemCounts = new QLabel(QStringLiteral("错误 0 · 警告 0"), problemsPage);
    m_previousProblem = new QPushButton(QStringLiteral("上一个"), problemsPage);
    m_previousProblem->setObjectName(QStringLiteral("previousBuildProblem"));
    m_nextProblem = new QPushButton(QStringLiteral("下一个"), problemsPage);
    m_nextProblem->setObjectName(QStringLiteral("nextBuildProblem"));
    m_previousProblem->setToolTip(QStringLiteral("上一个问题 (Shift+F8)"));
    m_nextProblem->setToolTip(QStringLiteral("下一个问题 (F8)"));
    m_previousProblem->setEnabled(false);
    m_nextProblem->setEnabled(false);
    problemBar->addWidget(m_problemFilter);
    problemBar->addWidget(m_problemCounts);
    problemBar->addStretch();
    problemBar->addWidget(m_previousProblem);
    problemBar->addWidget(m_nextProblem);
    problemsLayout->addLayout(problemBar);
    m_buildProblems = new QTreeWidget(problemsPage);
    m_buildProblems->setObjectName(QStringLiteral("buildProblems"));
    m_buildProblems->setHeaderLabels({QStringLiteral("级别"), QStringLiteral("文件"), QStringLiteral("行:列"), QStringLiteral("说明")});
    m_buildProblems->setRootIsDecorated(false);
    m_buildProblems->setAlternatingRowColors(true);
    m_buildProblems->setSelectionMode(QAbstractItemView::SingleSelection);
    m_buildProblems->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_buildProblems->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_buildProblems->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_buildProblems->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    problemsLayout->addWidget(m_buildProblems);
    m_outputTabs->addTab(problemsPage, QStringLiteral("问题 (0)"));
    connect(m_problemFilter, &QComboBox::currentIndexChanged, this, &STM32WorkbenchWidget::refreshBuildProblems);
    connect(m_buildProblems, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) { activateBuildProblem(item); });
    auto *activateProblem = new QShortcut(QKeySequence(Qt::Key_Return), m_buildProblems);
    activateProblem->setContext(Qt::WidgetShortcut);
    connect(activateProblem, &QShortcut::activated, this, [this] { activateBuildProblem(m_buildProblems->currentItem()); });
    connect(m_previousProblem, &QPushButton::clicked, this, [this] { navigateBuildProblem(-1); });
    connect(m_nextProblem, &QPushButton::clicked, this, [this] { navigateBuildProblem(1); });
    auto *nextProblem = new QShortcut(QKeySequence(Qt::Key_F8), this);
    connect(nextProblem, &QShortcut::activated, this, [this] { navigateBuildProblem(1); });
    auto *previousProblem = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F8), this);
    connect(previousProblem, &QShortcut::activated, this, [this] { navigateBuildProblem(-1); });
    tcLayout->addWidget(m_outputTabs, 1);

    // 内存与任务进度条状态行
    QWidget *meterWidget = new QWidget(terminalContainer);
    QHBoxLayout *meterLayout = new QHBoxLayout(meterWidget);
    meterLayout->setContentsMargins(4, 2, 4, 2);
    meterLayout->setSpacing(12);

    m_flashMemLabel = new QLabel(QStringLiteral("Flash: 0 / 64 KB (0%)"), meterWidget);
    m_flashMemBar = new QProgressBar(meterWidget);
    m_flashMemBar->setRange(0, 100);
    m_flashMemBar->setValue(0);
    m_flashMemBar->setFixedWidth(140);
    m_flashMemBar->setStyleSheet("QProgressBar::chunk { background-color: #3867d6; border-radius: 2px; }");

    m_ramMemLabel = new QLabel(QStringLiteral("RAM: 0 / 20 KB (0%)"), meterWidget);
    m_ramMemBar = new QProgressBar(meterWidget);
    m_ramMemBar->setRange(0, 100);
    m_ramMemBar->setValue(0);
    m_ramMemBar->setFixedWidth(140);
    m_ramMemBar->setStyleSheet("QProgressBar::chunk { background-color: #20bf6b; border-radius: 2px; }");

    m_taskStatusLabel = new QLabel(QStringLiteral("就绪"), meterWidget);
    m_taskProgressBar = new QProgressBar(meterWidget);
    m_taskProgressBar->setRange(0, 100);
    m_taskProgressBar->setValue(0);
    m_taskProgressBar->setFixedWidth(120);

    meterLayout->addWidget(new QLabel(QStringLiteral("💾 内存资源占用:"), meterWidget));
    meterLayout->addWidget(m_flashMemLabel);
    meterLayout->addWidget(m_flashMemBar);
    meterLayout->addWidget(m_ramMemLabel);
    meterLayout->addWidget(m_ramMemBar);
    meterLayout->addStretch(1);
    meterLayout->addWidget(m_taskStatusLabel);
    meterLayout->addWidget(m_taskProgressBar);

    tcLayout->addWidget(meterWidget);
    m_mainSplitter->addWidget(terminalContainer);

    // 下半部分：GDB 调试控制面板
    m_debugPanel = new GdbDebugPanel(m_gdbEngine, m_mainSplitter);
    m_mainSplitter->addWidget(m_debugPanel);

    // 设置分栏初始比例 45 : 55
    m_mainSplitter->setStretchFactor(0, 4);
    m_mainSplitter->setStretchFactor(1, 5);
    mainLayout->addWidget(m_mainSplitter, 1);
}

void STM32WorkbenchWidget::applyTheme()
{
    setStyleSheet(
        "QWidget { background-color: #11111b; color: #cdd6f4; font-family: 'Segoe UI', 'Microsoft YaHei'; }"
        "QLabel { color: #bac2de; font-size: 12px; }"
        "QLineEdit { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #f5e0dc; }"
        "QComboBox { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #cdd6f4; }"
        "QTextBrowser, QTextEdit { background-color: #11111b; border: 1px solid #313244; color: #cdd6f4; }"
        "QTreeWidget { background-color: #11111b; alternate-background-color: #181825; color: #cdd6f4; border: 1px solid #313244; }"
        "QTreeWidget::item:selected { background-color: #313244; }"
        "QHeaderView::section { background-color: #1e1e2e; color: #bac2de; border: 0; padding: 5px; }"
        "QTabWidget::pane { border: 1px solid #313244; }"
        "QTabBar::tab { background-color: #181825; color: #bac2de; padding: 5px 12px; }"
        "QTabBar::tab:selected { background-color: #313244; color: #cdd6f4; }"
        "QProgressBar { background-color: #181825; border: 1px solid #45475a; border-radius: 3px; text-align: center; color: #ffffff; font-size: 10px; }"
        "QPushButton { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #45475a; }"
    );
}

void STM32WorkbenchWidget::setProjectPath(const QString &path)
{
    m_projectPath = QDir::toNativeSeparators(path);
    m_projectPathEdit->setText(m_projectPath);
    emit projectPathChanged(m_projectPath);
}

void STM32WorkbenchWidget::loadSettings()
{
    m_paths = m_detector->loadSettings(m_settings);
    m_buildManager->setToolchainPaths(m_paths);
    m_flashProgrammer->setToolchainPaths(m_paths);
    m_serverManager->setToolchainPaths(m_paths);

    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        QString savedChip = m_settings->value("selectedChip", "STM32F103C8").toString();
        int idx = m_chipCombo->findText(savedChip);
        if (idx >= 0) m_chipCombo->setCurrentIndex(idx);
        else if (!STM32ProfileManager::instance().getProfile(savedChip).chipName.isEmpty())
            m_chipCombo->setCurrentText(savedChip);

        QString savedProj = m_settings->value("lastProject", "").toString();
        if (!savedProj.isEmpty() && QDir(savedProj).exists()) {
            setProjectPath(savedProj);
        }
        m_settings->endGroup();
    }
}

void STM32WorkbenchWidget::saveSettings()
{
    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        m_settings->setValue("selectedChip", m_chipCombo->currentText());
        m_settings->setValue("lastProject", m_projectPath);
        m_settings->endGroup();
        m_settings->sync();
    }
}

void STM32WorkbenchWidget::onBrowseProjectDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 STM32 工程根目录"),
                                                   m_projectPath.isEmpty() ? QDir::homePath() : m_projectPath);
    if (!dir.isEmpty()) {
        setProjectPath(dir);
        saveSettings();
    }
}

void STM32WorkbenchWidget::onChipChanged(const QString &chipName)
{
    m_currentProfile = STM32ProfileManager::instance().getProfile(chipName);
    m_buildManager->setChipProfile(m_currentProfile);
    m_flashProgrammer->setChipProfile(m_currentProfile);
    m_serverManager->setChipProfile(m_currentProfile);

    // 刷新内存条总上限提示
    quint32 flashK = m_currentProfile.flashSizeBytes / 1024;
    quint32 ramK = m_currentProfile.ramSizeBytes / 1024;
    m_flashMemLabel->setText(QString("Flash: 0 / %1 KB (0%)").arg(flashK));
    m_ramMemLabel->setText(QString("RAM: 0 / %1 KB (0%)").arg(ramK));
}

void STM32WorkbenchWidget::onBuildClicked()
{
    if (m_buildManager->isBusy()) return;
    m_retryRebuildAfterMigration = false;
    if (m_currentProfile.chipName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("目标芯片无效"),
                             QStringLiteral("请选择目录中的准确 STM32 型号或输入有效订货号。"));
        return;
    }
    if (m_projectPath.isEmpty() || !QDir(m_projectPath).exists()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先指定有效的 STM32 工程根目录!"));
        return;
    }

    m_buildTerminal->clear();
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
    BuildConfiguration cfg = static_cast<BuildConfiguration>(m_configCombo->currentData().toInt());
    m_taskStatusLabel->setText(QStringLiteral("正在构建..."));
    m_taskProgressBar->setValue(30);

    m_buildManager->startBuild(m_projectPath, cfg);
}

void STM32WorkbenchWidget::onCleanClicked()
{
    if (m_buildManager->isBusy()) return;
    if (m_projectPath.isEmpty()) return;
    m_buildTerminal->append(QStringLiteral("正在清理...\n"));
    m_buildManager->startClean(m_projectPath);
}

void STM32WorkbenchWidget::onRebuildClicked()
{
    if (m_buildManager->isBusy()) return;
    m_retryRebuildAfterMigration = true;
    if (m_currentProfile.chipName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("目标芯片无效"),
                             QStringLiteral("请选择目录中的准确 STM32 型号或输入有效订货号。"));
        return;
    }
    if (m_projectPath.isEmpty()) return;
    m_buildTerminal->clear();
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
    BuildConfiguration cfg = static_cast<BuildConfiguration>(m_configCombo->currentData().toInt());
    m_buildManager->startRebuild(m_projectPath, cfg);
}

void STM32WorkbenchWidget::onFlashClicked()
{
    if (m_flashProgrammer->isBusy()) return;
    if (m_projectPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先指定 STM32 工程目录!"));
        return;
    }

    QString binPath = m_buildManager->getBinPath(m_projectPath);
    if (!QFile::exists(binPath)) {
        // 如果固件还未生成，提示并引导自动构建
        int ret = QMessageBox::question(this, QStringLiteral("未检测到固件"),
                                       QString("尚未在 build/ 目录找到目标固件:\n%1\n是否立即执行一键构建？").arg(binPath),
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            onBuildClicked();
        }
        return;
    }

    FlashOptions opt;
    if (m_settings) {
        FlashConfigDialog dlg(this, m_settings, m_currentProfile);
        opt = dlg.getOptions();
    }

    m_taskStatusLabel->setText(QStringLiteral("正在烧录固件..."));
    m_taskProgressBar->setValue(10);
    if (!m_flashProgrammer->flashFirmware(binPath, opt) && m_flashActive)
        finishFlashLog(false, QStringLiteral("下载未启动，请检查上方错误信息。"));
}

void STM32WorkbenchWidget::onToggleDebugSession()
{
    if (m_gdbEngine->isConnected()) {
        // 如果已经在调试中，停止调试
        m_gdbEngine->stopSession();
        m_serverManager->stopServer();
        return;
    }

    if (m_projectPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先指定 STM32 工程目录!"));
        return;
    }

    QString elfPath = m_buildManager->getElfPath(m_projectPath);
    if (!QFile::exists(elfPath)) {
        QMessageBox::warning(this, QStringLiteral("未找到 ELF 文件"),
                             QString("未在 build 目录下找到调试目标符号文件: %1\n请先点击[一键编译]构建工程!").arg(elfPath));
        return;
    }

    m_buildViewActive = false;
    leaveFlashLog();
    m_buildTerminal->append(QStringLiteral("\n====== 启动嵌入式硬件调试流程 ======\n"));
    m_taskStatusLabel->setText(QStringLiteral("正在启动 GDB Server..."));

    // 启动 OpenOCD 服务端 (默认 3333 端口)
    bool ok = m_serverManager->startServer(GdbServerType::OpenOCD, QString(), QString(), 3333);
    if (!ok) {
        m_buildTerminal->append(QStringLiteral("❌ 无法启动 GDB Server，请检查 OpenOCD 配置!\n"));
    }
}

void STM32WorkbenchWidget::onServerReady(int port)
{
    QString elfPath = m_buildManager->getElfPath(m_projectPath);
    m_buildTerminal->append(QString("GDB Server 就绪，启动 GDB 客户端连接 127.0.0.1:%1...\n").arg(port));

    m_gdbEngine->startSession(m_paths.gdbPath, elfPath, "localhost", port);
}

void STM32WorkbenchWidget::onGdbStateChanged(GdbTargetState state)
{
    if (state == GdbTargetState::Connected || state == GdbTargetState::Running || state == GdbTargetState::Stopped) {
        m_debugBtn->setText(QStringLiteral("⏹ 结束调试会话"));
        m_debugBtn->setStyleSheet("background-color: #eb3b5a; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    } else {
        m_debugBtn->setText(QStringLiteral("🐞 启动 GDB 硬件调试"));
        m_debugBtn->setStyleSheet("background-color: #20bf6b; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    }
}

void STM32WorkbenchWidget::toggleBreakpoint(const QString &file, int line)
{
    m_gdbEngine->toggleBreakpoint(file, line);
}

void STM32WorkbenchWidget::onBuildOutput(const QString &text, bool isError)
{
    if (m_usingBuildLogModel) return;
    leaveFlashLog();
    appendLogText(text, isError);
}

void STM32WorkbenchWidget::onFlashOutput(const QString &text, bool isError)
{
    if (!m_flashActive) return;
    m_flashLog.append(text, isError);
    if (m_showingFlashLog) renderFlashLog();
}

void STM32WorkbenchWidget::beginFlashLog(const QString &firmwarePath, const FlashOptions &options)
{
    m_buildViewActive = false;
    m_outputTabs->setCurrentIndex(0);
    m_flashOptions = options;
    m_flashLog.reset(options.verify, options.resetAndRun);
    m_flashLogHeader.clear();
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
    m_showingFlashLog = true;
    m_flashActive = true;
    m_flashTimer.start();
    const QSignalBlocker blocker(m_flashDetailedLog);
    m_flashDetailedLog->setChecked(false);
    m_flashDetailedLog->setEnabled(true);
    m_flashDetailedLog->setToolTip(QStringLiteral("显示下载工具的完整输出和执行命令"));

    m_flashLogHeader.append({QStringLiteral("固件下载"), FlashLogSeverity::Info});
    if (!firmwarePath.isEmpty()) {
        const QFileInfo file(firmwarePath);
        m_flashLogHeader.append({QStringLiteral("文件: %1  (%2 字节)").arg(file.fileName()).arg(file.size()), FlashLogSeverity::Info});
    }
    if (!m_currentProfile.chipName.isEmpty())
        m_flashLogHeader.append({QStringLiteral("目标: %1    地址: %2")
            .arg(m_currentProfile.chipName, options.targetAddress), FlashLogSeverity::Info});
    QString driver;
    switch (options.driver) {
    case FlashDriverType::OpenOCD_STLink: driver = QStringLiteral("OpenOCD / ST-Link"); break;
    case FlashDriverType::OpenOCD_CMSISDAP: driver = QStringLiteral("OpenOCD / CMSIS-DAP"); break;
    case FlashDriverType::OpenOCD_JLink: driver = QStringLiteral("OpenOCD / J-Link"); break;
    case FlashDriverType::JLink_Native: driver = QStringLiteral("SEGGER J-Link"); break;
    case FlashDriverType::STM32CubeProg: driver = QStringLiteral("STM32CubeProgrammer"); break;
    case FlashDriverType::StFlash: driver = QStringLiteral("st-flash / ST-Link"); break;
    }
    QString probe = QStringLiteral("下载器: %1").arg(driver);
    if (!options.probeSerial.isEmpty()) probe += QStringLiteral("    序列号: %1").arg(options.probeSerial);
    m_flashLogHeader.append({probe, FlashLogSeverity::Info});
    m_flashLogHeader.append({QString(), FlashLogSeverity::Info});
    m_flashLogHeader.append({QStringLiteral("正在连接目标…"), FlashLogSeverity::Info});
    m_taskStatusLabel->setText(QStringLiteral("正在连接…"));
    m_taskProgressBar->setValue(10);
    renderFlashLog();
}

void STM32WorkbenchWidget::finishFlashLog(bool success, const QString &message)
{
    if (!m_flashActive) return;
    QString summary = message;
    if (success) {
        QStringList result{QStringLiteral("下载完成")};
        if (m_flashOptions.verify) result << QStringLiteral("已校验");
        if (m_flashOptions.resetAndRun) result << QStringLiteral("已复位运行");
        result << QStringLiteral("耗时 %1 秒").arg(m_flashTimer.elapsed() / 1000.0, 0, 'f', 2);
        summary = result.join(QStringLiteral(" · "));
    }
    const auto finalEntry = m_flashLog.finish(success, summary);
    success = finalEntry.severity == FlashLogSeverity::Success;
    m_flashActive = false;
    if (!m_showingFlashLog) return;
    m_taskStatusLabel->setText(success ? QStringLiteral("下载完成") : QStringLiteral("下载失败"));
    m_taskProgressBar->setValue(success ? 100 : 0);
    renderFlashLog();
}

void STM32WorkbenchWidget::renderFlashLog()
{
    m_buildTerminal->clear();
    QTextCursor cursor(m_buildTerminal->document());
    auto insert = [&cursor](const FlashLogEntry &entry) {
        QTextCharFormat format;
        format.setForeground(FlashLogModel::color(entry.severity));
        cursor.insertText(entry.text + '\n', format);
    };
    for (const auto &entry : m_flashLogHeader) insert(entry);
    const auto &entries = m_flashDetailedLog->isChecked()
        ? m_flashLog.detailedEntries() : m_flashLog.conciseEntries();
    for (const auto &entry : entries) insert(entry);
    m_buildTerminal->setTextCursor(cursor);
    m_buildTerminal->ensureCursorVisible();
}

void STM32WorkbenchWidget::leaveFlashLog()
{
    m_showingFlashLog = false;
    m_flashDetailedLog->setEnabled(false);
}

void STM32WorkbenchWidget::beginBuildLog(const QString &action)
{
    leaveFlashLog();
    m_usingBuildLogModel = true;
    m_buildHasSize = false;
    m_buildViewActive = true;
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
    const QSignalBlocker blocker(m_flashDetailedLog);
    m_flashDetailedLog->setChecked(false);
    m_flashDetailedLog->setEnabled(true);
    m_flashDetailedLog->setToolTip(QStringLiteral("显示完整编译命令和诊断上下文"));
    m_outputTabs->setCurrentIndex(0);
    m_problemFilter->setCurrentIndex(0);
    m_buildLogHeading = QStringLiteral("%1: %2  (%3)\n输出目录: %4\n")
        .arg(action, m_buildManager->chipProfile().chipName,
             m_configCombo->currentData().toInt() == int(BuildConfiguration::Debug) ? QStringLiteral("Debug") : QStringLiteral("Release"),
             m_buildManager->getElfPath(m_projectPath));
    m_flashMemBar->setValue(0);
    m_ramMemBar->setValue(0);
    m_flashMemLabel->setText(QStringLiteral("Flash: 等待本次构建结果"));
    m_ramMemLabel->setText(QStringLiteral("RAM: 等待本次构建结果"));
    renderBuildLog(true);
    refreshBuildProblems();
}

void STM32WorkbenchWidget::renderBuildLog(bool reset)
{
    if (!m_buildViewActive) return;
    const auto &model = m_buildManager->logModel();
    const auto &entries = m_flashDetailedLog->isChecked() ? model.detailedEntries() : model.conciseEntries();
    auto *scroll = m_buildTerminal->verticalScrollBar();
    const bool following = scroll->value() >= scroll->maximum() - 2;
    const int previousScroll = scroll->value();
    if (reset || m_renderedBuildEntries > entries.size()) {
        m_buildTerminal->clear();
        m_renderedBuildEntries = 0;
        QTextCursor heading(m_buildTerminal->document());
        QTextCharFormat format;
        format.setForeground(QColor("#cdd6f4"));
        heading.insertText(m_buildLogHeading + '\n', format);
    }
    QTextCursor cursor(m_buildTerminal->document());
    cursor.movePosition(QTextCursor::End);
    for (; m_renderedBuildEntries < entries.size(); ++m_renderedBuildEntries) {
        const auto &entry = entries.at(m_renderedBuildEntries);
        QTextCharFormat format;
        format.setForeground(entry.success ? QColor("#a6e3a1") : BuildLogModel::color(entry.severity));
        if (!entry.filePath.isEmpty() && entry.line > 0) {
            QUrl url = QUrl::fromLocalFile(entry.filePath);
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("line"), QString::number(entry.line));
            query.addQueryItem(QStringLiteral("column"), QString::number(entry.column));
            url.setQuery(query);
            format.setAnchor(true);
            format.setAnchorHref(url.toString());
            format.setFontUnderline(true);
        }
        cursor.insertText(entry.text + '\n', format);
    }
    if (following || reset) {
        m_buildTerminal->setTextCursor(cursor);
        m_buildTerminal->ensureCursorVisible();
    } else scroll->setValue(previousScroll);
}

void STM32WorkbenchWidget::refreshBuildProblems()
{
    int selectedDiagnostic = -1;
    if (m_buildProblems->currentItem()) selectedDiagnostic = m_buildProblems->currentItem()->data(0, Qt::UserRole).toInt();
    m_buildProblems->clear();
    const auto &model = m_buildManager->logModel();
    const int filter = m_problemFilter->currentIndex();
    const auto &diagnostics = model.diagnostics();
    for (qsizetype i = 0; i < diagnostics.size(); ++i) {
        const auto &d = diagnostics.at(i);
        if (d.severity == DiagnosticSeverity::Info) continue;
        const bool warning = d.severity == DiagnosticSeverity::Warning;
        if ((filter == 1 && warning) || (filter == 2 && !warning)) continue;
        auto *item = new QTreeWidgetItem(m_buildProblems);
        item->setText(0, warning ? QStringLiteral("警告") : QStringLiteral("错误"));
        item->setText(1, d.filePath.isEmpty() ? QStringLiteral("链接器 / 构建工具") : QFileInfo(d.filePath).fileName());
        item->setText(2, d.line > 0 ? QString::number(d.line) + (d.column > 0 ? ":" + QString::number(d.column) : QString()) : QStringLiteral("—"));
        item->setText(3, d.message);
        item->setData(0, Qt::UserRole, int(i));
        for (int column = 0; column < 4; ++column) {
            item->setForeground(column, BuildLogModel::color(d.severity));
            item->setToolTip(column, d.filePath.isEmpty() ? d.message + QStringLiteral("\n无源文件位置，请查看详细日志。") : d.filePath + '\n' + d.message);
        }
        if (i == selectedDiagnostic) m_buildProblems->setCurrentItem(item);
    }
    m_problemCounts->setText(QStringLiteral("错误 %1 · 警告 %2").arg(model.errorCount()).arg(model.warningCount()));
    m_outputTabs->setTabText(1, QStringLiteral("问题 (%1)").arg(model.errorCount() + model.warningCount()));
    m_previousProblem->setEnabled(m_buildProblems->topLevelItemCount() > 0);
    m_nextProblem->setEnabled(m_buildProblems->topLevelItemCount() > 0);
}

void STM32WorkbenchWidget::activateBuildProblem(QTreeWidgetItem *item)
{
    if (!item) return;
    const int index = item->data(0, Qt::UserRole).toInt();
    const auto &diagnostics = m_buildManager->logModel().diagnostics();
    if (index < 0 || index >= diagnostics.size()) return;
    const auto &d = diagnostics.at(index);
    if (!d.filePath.isEmpty() && d.line > 0) navigateToSource(d.filePath, d.line, d.column);
}

void STM32WorkbenchWidget::navigateBuildProblem(int direction)
{
    const int count = m_buildProblems->topLevelItemCount();
    if (!count) return;
    int current = m_buildProblems->indexOfTopLevelItem(m_buildProblems->currentItem());
    current = current < 0 ? (direction > 0 ? 0 : count - 1) : (current + direction + count) % count;
    auto *item = m_buildProblems->topLevelItem(current);
    m_buildProblems->setCurrentItem(item);
    m_buildProblems->scrollToItem(item);
    activateBuildProblem(item);
}

void STM32WorkbenchWidget::navigateToSource(const QString &file, int line, int column)
{
    emit jumpToSourceLine(file, line);
    SourceNavigator::open(this, m_sourceNavigationHost, file, line, column);
}

void STM32WorkbenchWidget::appendLogText(const QString &text, bool isError)
{
    QString &pending = isError ? m_pendingLogErr : m_pendingLogOut;
    pending += text;
    int newline = pending.indexOf('\n');
    while (newline >= 0) {
        appendLogLine(pending.left(newline), isError);
        pending.remove(0, newline + 1);
        newline = pending.indexOf('\n');
    }
}

void STM32WorkbenchWidget::flushLogText()
{
    if (!m_pendingLogOut.isEmpty()) appendLogLine(m_pendingLogOut, false);
    if (!m_pendingLogErr.isEmpty()) appendLogLine(m_pendingLogErr, true);
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
}

void STM32WorkbenchWidget::appendLogLine(const QString &line, bool isError)
{
    static const QRegularExpression diagnostic(
        R"(^(.+?):(\d+)(?::\d+)?:\s+(fatal error|error|warning|note):)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = diagnostic.match(line.trimmed());
    QTextCharFormat format;
    if (match.hasMatch()) {
        const QString severity = match.captured(3).toLower();
        format.setForeground(QColor(severity.contains(QStringLiteral("error")) ? "#f38ba8" :
                                    severity == QStringLiteral("warning") ? "#f9e2af" : "#89b4fa"));
        QString path = match.captured(1);
        if (QFileInfo(path).isRelative()) path = QDir(m_projectPath).absoluteFilePath(path);
        QUrl url = QUrl::fromLocalFile(path);
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("line"), match.captured(2));
        url.setQuery(query);
        format.setAnchor(true);
        format.setAnchorHref(url.toString());
        format.setFontUnderline(true);
    } else if (line.contains(QStringLiteral("warning:"), Qt::CaseInsensitive) ||
               line.contains(QStringLiteral("警告"))) {
        format.setForeground(QColor("#f9e2af"));
    } else if (line.contains(QStringLiteral("error:"), Qt::CaseInsensitive) ||
               line.contains(QStringLiteral("fatal error"), Qt::CaseInsensitive) ||
               line.contains(QStringLiteral("构建失败")) || isError) {
        format.setForeground(QColor("#f38ba8"));
    } else if (line.startsWith(QStringLiteral("执行构建命令:")) ||
               line.startsWith(QStringLiteral("GCC:")) ||
               line.startsWith(QStringLiteral("Make:"))) {
        format.setForeground(QColor("#89b4fa"));
    } else if (line.contains(QStringLiteral("构建成功"))) {
        format.setForeground(QColor("#a6e3a1"));
    } else {
        format.setForeground(QColor("#cdd6f4"));
    }
    QTextCursor cursor = m_buildTerminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line + '\n', format);
    m_buildTerminal->setTextCursor(cursor);
    m_buildTerminal->ensureCursorVisible();
}

void STM32WorkbenchWidget::onFlashProgress(int percent, const QString &text)
{
    if (!m_showingFlashLog) return;
    m_taskProgressBar->setValue(percent);
    m_taskStatusLabel->setText(text);
}

void STM32WorkbenchWidget::onMemoryUsageUpdated(quint32 flashUsed, quint32 flashTotal, quint32 ramUsed, quint32 ramTotal)
{
    m_buildHasSize = true;
    double flashPercent = flashTotal > 0 ? ((double)flashUsed / flashTotal * 100.0) : 0;
    double ramPercent = ramTotal > 0 ? ((double)ramUsed / ramTotal * 100.0) : 0;

    m_flashMemBar->setValue(static_cast<int>(flashPercent));
    m_flashMemLabel->setText(QString("Flash: %1 KB / %2 KB (%3%)")
                             .arg(QString::number(flashUsed / 1024.0, 'f', 1))
                             .arg(flashTotal / 1024)
                             .arg(QString::number(flashPercent, 'f', 1)));

    m_ramMemBar->setValue(static_cast<int>(ramPercent));
    m_ramMemLabel->setText(QString("RAM: %1 KB / %2 KB (%3%)")
                           .arg(QString::number(ramUsed / 1024.0, 'f', 1))
                           .arg(ramTotal / 1024)
                           .arg(QString::number(ramPercent, 'f', 1)));
}

void STM32WorkbenchWidget::onOpenToolchainSettings()
{
    ToolchainConfigDialog dlg(this, m_settings);
    if (dlg.exec() == QDialog::Accepted) {
        m_paths = dlg.getPaths();
        m_buildManager->setToolchainPaths(m_paths);
        m_flashProgrammer->setToolchainPaths(m_paths);
        m_serverManager->setToolchainPaths(m_paths);

        QString chip = dlg.getSelectedChip();
        int idx = m_chipCombo->findText(chip);
        if (idx >= 0) m_chipCombo->setCurrentIndex(idx);

        m_buildTerminal->append(QStringLiteral("✅ 工具链与硬件配置已成功更新\n"));
    }
}

void STM32WorkbenchWidget::onOpenFlashSettings()
{
    FlashConfigDialog dlg(this, m_settings, m_currentProfile);
    if (dlg.exec() == QDialog::Accepted) {
        m_buildTerminal->append(QStringLiteral("✅ 固件烧录参数配置已更新\n"));
    }
}

void STM32WorkbenchWidget::onOpenChipSelectDialog()
{
    TargetChipDialog dlg(this, m_settings, m_chipCombo->currentText());
    if (dlg.exec() == QDialog::Accepted) {
        QString chip = dlg.getSelectedChip();
        if (!chip.isEmpty()) {
            int idx = m_chipCombo->findText(chip);
            if (idx >= 0) {
                m_chipCombo->setCurrentIndex(idx);
            } else {
                m_chipCombo->addItem(chip);
                m_chipCombo->setCurrentText(chip);
            }
            onChipChanged(chip);
            if (m_settings) {
                m_settings->beginGroup("STM32Toolchain");
                m_settings->setValue("selectedChip", chip);
                m_settings->endGroup();
            }
            m_buildTerminal->append(QStringLiteral("🎯 当前工程目标芯片已更新为: %1\n").arg(chip));
        }
    }
}
