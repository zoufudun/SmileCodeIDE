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
#include "ToolchainConfigDialog.h"
#include "FlashConfigDialog.h"

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
    connect(m_buildManager, &BuildManager::buildFinished, this, [this](bool, int, int) { flushLogText(); });
    connect(m_buildManager, &BuildManager::memoryUsageUpdated, this, &STM32WorkbenchWidget::onMemoryUsageUpdated);
    connect(m_flashProgrammer, &FlashProgrammer::flashOutput, this, &STM32WorkbenchWidget::onFlashOutput);
    connect(m_flashProgrammer, &FlashProgrammer::flashProgress, this, &STM32WorkbenchWidget::onFlashProgress);

    // 绑定调试服务器与客户端联动
    connect(m_serverManager, &GdbServerManager::serverOutput, this, [this](const QString &text, bool isError) {
        onBuildOutput("[GDB Server] " + text, isError);
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
    connect(m_chipCombo, &QComboBox::currentTextChanged, this, &STM32WorkbenchWidget::onChipChanged);

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

    m_buildTerminal = new QTextBrowser(terminalContainer);
    m_buildTerminal->setReadOnly(true);
    m_buildTerminal->setOpenLinks(false);
    connect(m_buildTerminal, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        if (!url.isLocalFile()) return;
        const int line = QUrlQuery(url).queryItemValue(QStringLiteral("line")).toInt();
        if (line > 0) emit jumpToSourceLine(url.toLocalFile(), line);
    });
    m_buildTerminal->setFont(QFont("Consolas", 10));
    m_buildTerminal->setPlaceholderText(QStringLiteral("SmileCodeIDE 嵌入式构建与烧录日志终端已就绪..."));
    tcLayout->addWidget(m_buildTerminal, 1);

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
    if (m_projectPath.isEmpty()) return;
    m_buildTerminal->append(QStringLiteral("正在清理...\n"));
    m_buildManager->startClean(m_projectPath);
}

void STM32WorkbenchWidget::onRebuildClicked()
{
    if (m_projectPath.isEmpty()) return;
    m_buildTerminal->clear();
    m_pendingLogOut.clear();
    m_pendingLogErr.clear();
    BuildConfiguration cfg = static_cast<BuildConfiguration>(m_configCombo->currentData().toInt());
    m_buildManager->startRebuild(m_projectPath, cfg);
}

void STM32WorkbenchWidget::onFlashClicked()
{
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
        FlashConfigDialog dlg(this, m_settings);
        opt = dlg.getOptions();
    }

    m_taskStatusLabel->setText(QStringLiteral("正在烧录固件..."));
    m_taskProgressBar->setValue(10);
    m_flashProgrammer->flashFirmware(binPath, opt);
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
    appendLogText(text, isError);
}

void STM32WorkbenchWidget::onFlashOutput(const QString &text, bool isError)
{
    appendLogText(text, isError);
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
    m_taskProgressBar->setValue(percent);
    m_taskStatusLabel->setText(text);
}

void STM32WorkbenchWidget::onMemoryUsageUpdated(quint32 flashUsed, quint32 flashTotal, quint32 ramUsed, quint32 ramTotal)
{
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
    FlashConfigDialog dlg(this, m_settings);
    if (dlg.exec() == QDialog::Accepted) {
        m_buildTerminal->append(QStringLiteral("✅ 固件烧录参数配置已更新\n"));
    }
}
