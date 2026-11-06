#include <QApplication>
#include "idetheme.h"
#include "ideworkflowicons.h"
#include "ToolchainConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QCompleter>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include "TargetChipDialog.h"

namespace {
struct ToolchainCheckResult {
    ToolchainPaths paths;
    QString report;
};

ToolchainPaths savedPaths(QSettings *settings)
{
    ToolchainPaths paths;
    if (!settings) return paths;
    settings->beginGroup("STM32Toolchain");
    paths.gccPath = settings->value("gccPath").toString();
    paths.gppPath = settings->value("gppPath").toString();
    paths.gdbPath = settings->value("gdbPath").toString();
    paths.objcopyPath = settings->value("objcopyPath").toString();
    paths.sizePath = settings->value("sizePath").toString();
    paths.makePath = settings->value("makePath").toString();
    paths.openocdPath = settings->value("openocdPath").toString();
    paths.openocdScriptsDir = settings->value("openocdScriptsDir").toString();
    paths.jlinkPath = settings->value("jlinkPath").toString();
    paths.stm32ProgPath = settings->value("stm32ProgPath").toString();
    settings->endGroup();
    return paths;
}

ToolchainPaths fillMissingPaths(ToolchainPaths current, const ToolchainPaths &detected)
{
    // An unavailable user path may belong to an unplugged drive. Keep it until
    // the user explicitly changes it; scanning fills only unconfigured tools.
    const auto fields = {&ToolchainPaths::gccPath, &ToolchainPaths::gppPath,
        &ToolchainPaths::gdbPath, &ToolchainPaths::objcopyPath, &ToolchainPaths::sizePath,
        &ToolchainPaths::makePath, &ToolchainPaths::openocdPath,
        &ToolchainPaths::openocdScriptsDir, &ToolchainPaths::jlinkPath,
        &ToolchainPaths::stm32ProgPath};
    for (auto field : fields) {
        if ((current.*field).isEmpty()) current.*field = detected.*field;
    }
    return current;
}
}

ToolchainConfigDialog::ToolchainConfigDialog(QWidget *parent, QSettings *settings,
                                             const QString &themeId)
    : QDialog(parent), m_settings(settings), m_detector(new ToolchainDetector(this))
{
    setWindowTitle(QStringLiteral("嵌入式工具链与硬件调试器配置 (Toolchain Settings)"));
    setWindowIcon(IDEWorkflowIcons::icon(IDEWorkflowIcons::Kind::ToolchainConfig));
    resize(800, 760);
    setupUI();
    QString resolvedTheme = themeId;
    if (resolvedTheme.isEmpty() && m_settings) {
        m_settings->beginGroup("MainWindow");
        resolvedTheme = m_settings->value("currentTheme").toString();
        m_settings->endGroup();
        if (resolvedTheme.isEmpty()) {
            resolvedTheme = m_settings->value("theme").toString();
        }
    }
    if (resolvedTheme.isEmpty()) {
        resolvedTheme = IdeTheme::currentThemeId();
    }
    applyTheme(resolvedTheme);

    // Loading the dialog must never run discovery or executable version checks.
    setPaths(savedPaths(m_settings));

    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        QString savedChip = m_settings->value("selectedChip", "STM32F103C8").toString();
        int idx = m_chipCombo->findText(savedChip);
        if (idx >= 0) m_chipCombo->setCurrentIndex(idx);
        else if (!STM32ProfileManager::instance().getProfile(savedChip).chipName.isEmpty())
            m_chipCombo->setCurrentText(savedChip);

        CompilerBuildOptions opts;
        opts.optLevel = m_settings->value("optLevel", "-O1").toString();
        opts.debugLevel = m_settings->value("debugLevel", "-g3 -gdwarf-2").toString();
        opts.enableToleranceFlags = m_settings->value("enableToleranceFlags", true).toBool();
        opts.customCFlags = m_settings->value("customCFlags", "").toString();
        setBuildOptions(opts);
        m_settings->endGroup();
    }
}

void ToolchainConfigDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // 1. 目标芯片规格快速选择
    QGroupBox *chipGroup = new QGroupBox(QStringLiteral("1. 目标芯片型号 (Target MCU Profile)"), this);
    m_chipGroup = chipGroup;
    chipGroup->setObjectName("toolchainChipGroup");
    QHBoxLayout *chipLayout = new QHBoxLayout(chipGroup);
    QLabel *chipLabel = new QLabel(QStringLiteral("默认芯片:"), chipGroup);
    m_chipCombo = new QComboBox(chipGroup);
    m_chipCombo->addItems(STM32ProfileManager::instance().supportedChips());
    m_chipCombo->setEditable(true);
    m_chipCombo->setInsertPolicy(QComboBox::NoInsert);
    m_chipCombo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_chipCombo->completer()->setFilterMode(Qt::MatchContains);
    chipLayout->addWidget(chipLabel);
    chipLayout->addWidget(m_chipCombo, 1);
    m_selectChipBtn = new QPushButton(QStringLiteral("芯片选择..."), chipGroup);
    m_selectChipBtn->setToolTip(QStringLiteral("打开目标芯片分类级联选择对话框 (厂商/系列/器件)"));
    connect(m_selectChipBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onOpenChipSelectDialog);
    chipLayout->addWidget(m_selectChipBtn);
    mainLayout->addWidget(chipGroup);

    // 2. 交叉编译与构建工具链
    QGroupBox *toolchainGroup = new QGroupBox(QStringLiteral("2. GNU Arm 交叉编译工具链 (Toolchain Paths)"), this);
    m_toolchainGroup = toolchainGroup;
    QGridLayout *tgLayout = new QGridLayout(toolchainGroup);
    tgLayout->setColumnStretch(1, 1);

    m_gccEdit = new QLineEdit(toolchainGroup);
    m_gccEdit->setObjectName("toolchainGccPath");
    QPushButton *browseGcc = new QPushButton(QStringLiteral("浏览..."), toolchainGroup);
    connect(browseGcc, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseGcc);

    m_gdbEdit = new QLineEdit(toolchainGroup);
    QPushButton *browseGdb = new QPushButton(QStringLiteral("浏览..."), toolchainGroup);
    connect(browseGdb, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseGdb);

    m_makeEdit = new QLineEdit(toolchainGroup);
    QPushButton *browseMake = new QPushButton(QStringLiteral("浏览..."), toolchainGroup);
    connect(browseMake, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseMake);

    tgLayout->addWidget(new QLabel(QStringLiteral("GCC 编译器:"), toolchainGroup), 0, 0);
    tgLayout->addWidget(m_gccEdit, 0, 1);
    tgLayout->addWidget(browseGcc, 0, 2);

    tgLayout->addWidget(new QLabel(QStringLiteral("GDB 调试器:"), toolchainGroup), 1, 0);
    tgLayout->addWidget(m_gdbEdit, 1, 1);
    tgLayout->addWidget(browseGdb, 1, 2);

    tgLayout->addWidget(new QLabel(QStringLiteral("Make 工具:"), toolchainGroup), 2, 0);
    tgLayout->addWidget(m_makeEdit, 2, 1);
    tgLayout->addWidget(browseMake, 2, 2);

    mainLayout->addWidget(toolchainGroup);

    // 3. 硬件调试器与烧录工具
    QGroupBox *hardwareGroup = new QGroupBox(QStringLiteral("3. 硬件调试与烧录器 (Flash & Debug Servers)"), this);
    m_hardwareGroup = hardwareGroup;
    QGridLayout *hwLayout = new QGridLayout(hardwareGroup);
    hwLayout->setColumnStretch(1, 1);

    m_openocdEdit = new QLineEdit(hardwareGroup);
    QPushButton *browseOpenocd = new QPushButton(QStringLiteral("浏览..."), hardwareGroup);
    connect(browseOpenocd, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseOpenocd);

    m_openocdScriptsEdit = new QLineEdit(hardwareGroup);
    QPushButton *browseScripts = new QPushButton(QStringLiteral("浏览..."), hardwareGroup);
    connect(browseScripts, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseOpenocdScripts);

    m_jlinkEdit = new QLineEdit(hardwareGroup);
    m_jlinkEdit->setObjectName("toolchainJlinkPath");
    m_jlinkEdit->setPlaceholderText(QStringLiteral("支持自动查找 SEGGER J-Link 或 Keil 内置 JLink.exe"));
    m_autoFindJlinkBtn = new QPushButton(QStringLiteral("自动查找"), hardwareGroup);
    m_autoFindJlinkBtn->setObjectName("autoFindJlinkBtn");
    m_autoFindJlinkBtn->setToolTip(QStringLiteral("自动查找系统中安装的 SEGGER J-Link 或 Keil 内置 J-Link 环境"));
    connect(m_autoFindJlinkBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onAutoFindJlink);
    QPushButton *browseJlink = new QPushButton(QStringLiteral("浏览..."), hardwareGroup);
    connect(browseJlink, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseJlink);
    QWidget *jlinkActionBox = new QWidget(hardwareGroup);
    QHBoxLayout *jlinkActionLayout = new QHBoxLayout(jlinkActionBox);
    jlinkActionLayout->setContentsMargins(0, 0, 0, 0);
    jlinkActionLayout->setSpacing(6);
    jlinkActionLayout->addWidget(m_autoFindJlinkBtn);
    jlinkActionLayout->addWidget(browseJlink);

    m_stm32ProgEdit = new QLineEdit(hardwareGroup);
    QPushButton *browseStm32Prog = new QPushButton(QStringLiteral("浏览..."), hardwareGroup);
    connect(browseStm32Prog, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseStm32Prog);

    hwLayout->addWidget(new QLabel(QStringLiteral("OpenOCD 路径:"), hardwareGroup), 0, 0);
    hwLayout->addWidget(m_openocdEdit, 0, 1);
    hwLayout->addWidget(browseOpenocd, 0, 2);

    hwLayout->addWidget(new QLabel(QStringLiteral("OpenOCD Scripts:"), hardwareGroup), 1, 0);
    hwLayout->addWidget(m_openocdScriptsEdit, 1, 1);
    hwLayout->addWidget(browseScripts, 1, 2);

    hwLayout->addWidget(new QLabel(QStringLiteral("J-Link 路径:"), hardwareGroup), 2, 0);
    hwLayout->addWidget(m_jlinkEdit, 2, 1);
    hwLayout->addWidget(jlinkActionBox, 2, 2);

    hwLayout->addWidget(new QLabel(QStringLiteral("STM32Prog CLI:"), hardwareGroup), 3, 0);
    hwLayout->addWidget(m_stm32ProgEdit, 3, 1);
    hwLayout->addWidget(browseStm32Prog, 3, 2);

    mainLayout->addWidget(hardwareGroup);

    // 4. 编译器优化等级与构建选项
    QGroupBox *optGroup = new QGroupBox(QStringLiteral("4. 编译器优化等级与构建选项 (Optimization & Build Options)"), this);
    m_optGroup = optGroup;
    optGroup->setObjectName("toolchainOptGroup");
    QGridLayout *optLayout = new QGridLayout(optGroup);
    optLayout->setColumnStretch(1, 1);

    m_optLevelCombo = new QComboBox(optGroup);
    m_optLevelCombo->setObjectName("optLevelCombo");
    m_optLevelCombo->addItem(QStringLiteral("-O1 (默认推荐: 延时精准、对齐 Keil、硬件外设驱动最佳兼容)"), QStringLiteral("-O1"));
    m_optLevelCombo->addItem(QStringLiteral("-O0 (无优化: 关闭优化生成直白代码，注意裸机软件延时严重放大)"), QStringLiteral("-O0"));
    m_optLevelCombo->addItem(QStringLiteral("-O2 (性能优先: 指令重排优化，算法速度优先，适合正式发布)"), QStringLiteral("-O2"));
    m_optLevelCombo->addItem(QStringLiteral("-O3 (极限性能: 激进内联与循环展开，适合算力密集型任务)"), QStringLiteral("-O3"));
    m_optLevelCombo->addItem(QStringLiteral("-Os (体积优先: 极限精简代码尺寸，适合低容量 Flash 芯片)"), QStringLiteral("-Os"));
    m_optLevelCombo->addItem(QStringLiteral("-Og (调试体验: GCC 官方标准调试优化，兼顾调试与基本时序)"), QStringLiteral("-Og"));

    m_optDescriptionLabel = new QLabel(optGroup);
    m_optDescriptionLabel->setObjectName("toolchainOptDescription");
    m_optDescriptionLabel->setWordWrap(true);

    m_debugLevelCombo = new QComboBox(optGroup);
    m_debugLevelCombo->setObjectName("debugLevelCombo");
    m_debugLevelCombo->addItem(QStringLiteral("-g3 -gdwarf-2 (全量 DWARF2 符号，支持宏展开与行级精确定位，推荐)"), QStringLiteral("-g3 -gdwarf-2"));
    m_debugLevelCombo->addItem(QStringLiteral("-g (标准 DWARF 调试符号)"), QStringLiteral("-g"));
    m_debugLevelCombo->addItem(QStringLiteral("-g0 (无调试符号，生成固件最小)"), QStringLiteral("-g0"));

    m_tolerantFlagsCheck = new QCheckBox(QStringLiteral("启用 Keil 旧工程兼容模式 (-Wno-error=implicit-function-declaration)"), optGroup);
    m_tolerantFlagsCheck->setObjectName("tolerantFlagsCheck");
    m_tolerantFlagsCheck->setChecked(true);
    m_tolerantFlagsCheck->setToolTip(QStringLiteral("避免因旧工程中的函数隐式声明或类型微小差异导致 GCC 14 报错终止，建议保持开启"));

    m_customFlagsEdit = new QLineEdit(optGroup);
    m_customFlagsEdit->setObjectName("customFlagsEdit");
    m_customFlagsEdit->setPlaceholderText(QStringLiteral("可选自定义额外编译参数，如 -fstack-usage 或 -DDEBUG_PRINT"));

    optLayout->addWidget(new QLabel(QStringLiteral("优化等级 (-O):"), optGroup), 0, 0);
    optLayout->addWidget(m_optLevelCombo, 0, 1, 1, 2);

    optLayout->addWidget(new QLabel(QStringLiteral("等级说明:"), optGroup), 1, 0, Qt::AlignTop);
    optLayout->addWidget(m_optDescriptionLabel, 1, 1, 1, 2);

    optLayout->addWidget(new QLabel(QStringLiteral("调试信息 (-g):"), optGroup), 2, 0);
    optLayout->addWidget(m_debugLevelCombo, 2, 1);
    optLayout->addWidget(m_tolerantFlagsCheck, 2, 2);

    optLayout->addWidget(new QLabel(QStringLiteral("额外 CFLAGS:"), optGroup), 3, 0);
    optLayout->addWidget(m_customFlagsEdit, 3, 1, 1, 2);

    connect(m_optLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolchainConfigDialog::onOptLevelChanged);
    updateOptDescription();

    mainLayout->addWidget(optGroup);

    // 5. 环境自检报告与诊断输出
    QGroupBox *reportGroup = new QGroupBox(QStringLiteral("5. 环境健康度自检报告"), this);
    QVBoxLayout *reportLayout = new QVBoxLayout(reportGroup);
    m_diagnosticReportEdit = new QTextEdit(reportGroup);
    m_diagnosticReportEdit->setObjectName("toolchainDiagnosticReport");
    m_diagnosticReportEdit->setReadOnly(true);
    m_diagnosticReportEdit->setFont(QFont("Consolas", 9));
    reportLayout->addWidget(m_diagnosticReportEdit);
    mainLayout->addWidget(reportGroup, 1);

    // 5. 底部动作按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_autoDetectBtn = new QPushButton(QStringLiteral("🔍 自动智能扫描环境"), this);
    m_autoDetectBtn->setObjectName("toolchainAutoDetect");
    // auto detect btn style handled by applyTheme
    connect(m_autoDetectBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onAutoDetectClicked);

    m_saveBtn = new QPushButton(QStringLiteral("保存配置"), this);
    m_saveBtn->setObjectName("toolchainSave");
    // save btn style handled by applyTheme
    connect(m_saveBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onSaveClicked);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    // cancel btn style handled by applyTheme
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_autoDetectBtn);
    btnLayout->addStretch(1);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void ToolchainConfigDialog::applyTheme(const QString &themeId)
{
    QString tid = themeId;
    if (tid.isEmpty()) {
        QWidget *w = parentWidget();
        while (w) {
            QVariant v = w->property("currentTheme");
            if (v.isValid() && !v.toString().isEmpty()) {
                tid = v.toString();
                break;
            }
            w = w->parentWidget();
        }
    }
    if (tid.isEmpty() && qApp) {
        QVariant v = qApp->property("currentTheme");
        if (v.isValid() && !v.toString().isEmpty()) {
            tid = v.toString();
        }
    }
    if (tid.isEmpty() && m_settings) {
        m_settings->beginGroup("MainWindow");
        tid = m_settings->value("currentTheme").toString();
        m_settings->endGroup();
        if (tid.isEmpty()) {
            tid = m_settings->value("theme").toString();
        }
    }
    if (tid.isEmpty()) {
        tid = IdeTheme::currentThemeId();
    }
    if (tid.isEmpty()) {
        tid = QStringLiteral("dark");
    }
    const IdeTheme::ThemePalette p = IdeTheme::paletteFor(tid);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(p.windowBg));
    pal.setColor(QPalette::WindowText, QColor(p.textMain));
    pal.setColor(QPalette::Base, QColor(p.inputBg));
    pal.setColor(QPalette::AlternateBase, QColor(p.baseAltBg));
    pal.setColor(QPalette::Text, QColor(p.textMain));
    pal.setColor(QPalette::Button, QColor(p.cardBg));
    pal.setColor(QPalette::ButtonText, QColor(p.textMain));
    setPalette(pal);
    setAutoFillBackground(true);

    setStyleSheet(QString(
        "QDialog {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "}"
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid %3;"
        "  border-radius: 6px;"
        "  margin-top: 8px;"
        "  padding-top: 14px;"
        "  color: %4;"
        "  background-color: %5;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 4px;"
        "  color: %4;"
        "  background: transparent;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  background-color: transparent;"
        "  color: %6;"
        "  font-size: 12px;"
        "}"
        "QLineEdit, QComboBox, QTextEdit {"
        "  background-color: %7;"
        "  border: 1px solid %8;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  color: %2;"
        "  selection-background-color: %9;"
        "  selection-color: %10;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QTextEdit:focus { border: 1px solid %4; }"

        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; border: none; width: 22px; background: transparent; }"

        "QComboBox::down-arrow { image: url(\"%14\"); width: 10px; height: 10px; margin-right: 6px; }"

        "QComboBox::down-arrow:hover { image: url(\"%15\"); }"
        "QCheckBox {"
        "  background: transparent;"
        "  background-color: transparent;"
        "  color: %2;"
        "  spacing: 8px;"
        "  font-size: 12px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 15px;"
        "  height: 15px;"
        "  border: 1px solid %8;"
        "  border-radius: 3px;"
        "  background-color: %7;"
        "}"
        "QCheckBox::indicator:hover { border-color: %4; }"
        "QCheckBox::indicator:checked { background-color: %4; border-color: %4; }"
        "QPushButton {"
        "  background: %11;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton:hover {"
        "  background: %12;"
        "  color: %13;"
        "  border: 1px solid %4;"
        "}"
        "QPushButton#toolchainAutoDetect {"
        "  background: %11;"
        "  color: %4;"
        "  border: 1px solid %4;"
        "  font-weight: bold;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton#toolchainAutoDetect:hover {"
        "  background: %12;"
        "  color: %13;"
        "}"
        "QPushButton#toolchainSave {"
        "  background: %12;"
        "  color: %13;"
        "  font-weight: bold;"
        "  padding: 6px 20px;"
        "  border: 1px solid %4;"
        "}").arg(p.windowBg, p.textMain, p.border, p.accent, p.cardBg,
                 p.textSub, p.inputBg, p.inputBorder, p.selectionBg, p.selectionText,
                 p.panelBgGrad, p.accentGrad, p.accentText, IdeTheme::ensureArrowIcon(p.textSub), IdeTheme::ensureArrowIcon(p.accent)));

    if (m_optDescriptionLabel) {
        m_optDescriptionLabel->setStyleSheet(QString(
            "QLabel#toolchainOptDescription {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 4px;"
            "  padding: 6px 10px;"
            "  color: %3;"
            "  font-size: 11px;"
            "}").arg(p.cardBg, p.border, p.textMain));
    }
}

void ToolchainConfigDialog::updateFormFromPaths(const ToolchainPaths &paths)
{
    m_paths = paths;
    m_gccEdit->setText(paths.gccPath);
    m_gdbEdit->setText(paths.gdbPath);
    m_makeEdit->setText(paths.makePath);
    m_openocdEdit->setText(paths.openocdPath);
    m_openocdScriptsEdit->setText(paths.openocdScriptsDir);
    m_jlinkEdit->setText(paths.jlinkPath);
    m_stm32ProgEdit->setText(paths.stm32ProgPath);

}

void ToolchainConfigDialog::setPaths(const ToolchainPaths &paths)
{
    updateFormFromPaths(paths);
    startBackgroundCheck(false);
}

void ToolchainConfigDialog::setChipSelectionVisible(bool visible)
{
    m_chipSelectionVisible = visible;
    m_chipGroup->setVisible(visible);
}

void ToolchainConfigDialog::setChecking(bool checking)
{
    m_checking = checking;
    m_autoDetectBtn->setEnabled(!checking);
    m_saveBtn->setEnabled(!checking);
    m_toolchainGroup->setEnabled(!checking);
    m_hardwareGroup->setEnabled(!checking);
}

void ToolchainConfigDialog::startBackgroundCheck(bool scan)
{
    const ToolchainPaths current = getPaths();
    const quint64 generation = ++m_checkGeneration;
    setChecking(true);
    m_diagnosticReportEdit->setPlainText(scan
        ? QStringLiteral("正在后台扫描 PATH、环境变量和常见安装目录，随后检查工具版本…\n已配置的路径将被保留。")
        : QStringLiteral("正在后台检查工具路径和版本…"));
    auto *watcher = new QFutureWatcher<ToolchainCheckResult>(this);
    connect(watcher, &QFutureWatcher<ToolchainCheckResult>::finished, this,
            [this, watcher, generation, scan]() {
        const ToolchainCheckResult result = watcher->result();
        watcher->deleteLater();
        if (generation != m_checkGeneration) return;
        updateFormFromPaths(result.paths);
        m_diagnosticReportEdit->setPlainText((scan
            ? QStringLiteral("扫描完成。已填入缺失工具，请检查结果后保存。\n\n")
            : QString()) + result.report);
        setChecking(false);
    });
    // The worker owns its detector and values. It never touches the dialog,
    // widgets, or the GUI thread's QSettings, and may safely finish after close.
    watcher->setFuture(QtConcurrent::run([current, scan]() {
        ToolchainDetector detector;
        ToolchainCheckResult result;
        result.paths = current;
        QStringList progress;
        if (scan) {
            QObject::connect(&detector, &ToolchainDetector::detectionProgress,
                &detector, [&progress](const QString &message) { progress.append(message); });
            ToolchainPaths detected = detector.autoDetect();
            // Do not pair an explicitly selected GCC with another installation's
            // companion tools discovered elsewhere on PATH.
            if (!current.gccPath.isEmpty() && current.gccPath != detected.gccPath) {
                ToolchainPaths companions;
                ToolchainDetector::validateArmGccDir(QFileInfo(current.gccPath).absolutePath(), companions);
                detected.gppPath = companions.gppPath;
                detected.gdbPath = companions.gdbPath;
                detected.objcopyPath = companions.objcopyPath;
                detected.sizePath = companions.sizePath;
            }
            result.paths = fillMissingPaths(current, detected);
        }
        result.report = progress.join('\n');
        if (!result.report.isEmpty()) result.report += "\n\n";
        result.report += detector.generateDiagnosticReport(result.paths);
        return result;
    }));
}

void ToolchainConfigDialog::done(int result)
{
    ++m_checkGeneration;
    setChecking(false);
    QDialog::done(result);
}

ToolchainPaths ToolchainConfigDialog::getPaths() const
{
    ToolchainPaths paths = m_paths;
    paths.gccPath = m_gccEdit->text().trimmed();
    paths.gdbPath = m_gdbEdit->text().trimmed();
    paths.makePath = m_makeEdit->text().trimmed();
    paths.openocdPath = m_openocdEdit->text().trimmed();
    paths.openocdScriptsDir = m_openocdScriptsEdit->text().trimmed();
    paths.jlinkPath = m_jlinkEdit->text().trimmed();
    paths.stm32ProgPath = m_stm32ProgEdit->text().trimmed();

    if (paths.gccPath != m_paths.gccPath) {
        paths.gppPath.clear();
        paths.objcopyPath.clear();
        paths.sizePath.clear();
    }
    if (!paths.gccPath.isEmpty()) {
        ToolchainPaths companions;
        if (ToolchainDetector::validateArmGccDir(QFileInfo(paths.gccPath).absolutePath(), companions)) {
            if (paths.gppPath.isEmpty()) paths.gppPath = companions.gppPath;
            if (paths.gdbPath.isEmpty()) paths.gdbPath = companions.gdbPath;
            if (paths.objcopyPath.isEmpty()) paths.objcopyPath = companions.objcopyPath;
            if (paths.sizePath.isEmpty()) paths.sizePath = companions.sizePath;
        }
    }

    return paths;
}

QString ToolchainConfigDialog::getSelectedChip() const
{
    return m_chipCombo->currentText();
}

void ToolchainConfigDialog::onAutoDetectClicked()
{
    if (!m_checking) startBackgroundCheck(true);
}

void ToolchainConfigDialog::onBrowseGcc()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 arm-none-eabi-gcc 可执行文件"),
                                               m_gccEdit->text(), "GCC Compiler (arm-none-eabi-gcc*.exe arm-none-eabi-gcc);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_gccEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onBrowseGdb()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 arm-none-eabi-gdb 可执行文件"),
                                               m_gdbEdit->text(), "GDB Debugger (arm-none-eabi-gdb*.exe arm-none-eabi-gdb);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_gdbEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onBrowseMake()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 make 或 mingw32-make 可执行文件"),
                                               m_makeEdit->text(), "Make (*make*.exe make);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_makeEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onBrowseOpenocd()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 openocd 可执行文件"),
                                               m_openocdEdit->text(), "OpenOCD (openocd*.exe openocd);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_openocdEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onBrowseOpenocdScripts()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 OpenOCD scripts 文件夹"),
                                                    m_openocdScriptsEdit->text());
    if (!dir.isEmpty()) {
        m_openocdScriptsEdit->setText(QDir::toNativeSeparators(dir));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onBrowseJlink()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 JLink 可执行文件"),
                                               m_jlinkEdit->text(), "J-Link (JLink*.exe JLinkExe);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_jlinkEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onAutoFindJlink()
{
    const QString detected = m_detector->detectJLinkPath();
    if (!detected.isEmpty()) {
        m_paths.jlinkPath = QDir::toNativeSeparators(detected);
        m_jlinkEdit->setText(m_paths.jlinkPath);
        startBackgroundCheck(false);
    } else if (isVisible()) {
        QMessageBox::information(this, QStringLiteral("J-Link 自动查找"),
            QStringLiteral("未在系统默认安装目录、注册表或 Keil 目录中找到 JLink.exe，请点击“浏览...”手动选择。"));
    }
}

void ToolchainConfigDialog::onBrowseStm32Prog()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 STM32_Programmer_CLI 可执行文件"),
                                               m_stm32ProgEdit->text(), "Programmer (STM32_Programmer_CLI*.exe);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_stm32ProgEdit->setText(QDir::toNativeSeparators(file));
        startBackgroundCheck(false);
    }
}

void ToolchainConfigDialog::onSaveClicked()
{
    if (m_checking) return;
    ToolchainPaths paths = getPaths();
    m_detector->saveSettings(m_settings, paths);

    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        if (m_chipSelectionVisible) m_settings->setValue("selectedChip", getSelectedChip());
        CompilerBuildOptions opts = getBuildOptions();
        m_settings->setValue("optLevel", opts.optLevel);
        m_settings->setValue("debugLevel", opts.debugLevel);
        m_settings->setValue("enableToleranceFlags", opts.enableToleranceFlags);
        m_settings->setValue("customCFlags", opts.customCFlags);
        m_settings->endGroup();
        m_settings->sync();
    }

    accept();
}

void ToolchainConfigDialog::onOpenChipSelectDialog()
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
        }
    }
}

void ToolchainConfigDialog::onOptLevelChanged(int)
{
    updateOptDescription();
}

void ToolchainConfigDialog::updateOptDescription()
{
    if (!m_optDescriptionLabel || !m_optLevelCombo) return;
    const QString opt = m_optLevelCombo->currentData().toString();
    if (opt == "-O1") {
        m_optDescriptionLabel->setText(QStringLiteral("💡 【默认推荐】对齐 Keil MDK 默认的 1 级优化。精确保障软件模拟延时（模拟 I2C、触摸屏、单总线、屏幕驱动），杜绝外设握手超时死锁黑屏，同时兼顾源码单步断点调试体验。"));
    } else if (opt == "-O0") {
        m_optDescriptionLabel->setText(QStringLiteral("⚠️ 【无优化警告】禁用所有函数内联与寄存器变量分配。在 400MHz Cortex-M7/M4 裸机程序下，空循环软件延时会被放大 10 倍以上，易导致触摸屏/外设握手超时死锁（板卡假死黑屏）。"));
    } else if (opt == "-O2") {
        m_optDescriptionLabel->setText(QStringLiteral("🚀 【性能优先】执行深度指令重排、循环展开与函数内联，显著提升算力密集型任务的执行效率，适合成熟产品的正式 Release 发布。部分变量在调试时可能被优化消除。"));
    } else if (opt == "-O3") {
        m_optDescriptionLabel->setText(QStringLiteral("⚡ 【极限性能】启用最高强度的激进优化（包含高级向量化）。编译时间相对较长，二进制体积有所增加，适合图像处理、DSP 算法或实时性极高的场景。"));
    } else if (opt == "-Os") {
        m_optDescriptionLabel->setText(QStringLiteral("📦 【体积优先】在保证功能正常的前提下极限压缩编译后的机器码尺寸，生成最小的 .bin/.hex 固件，特别适合 Flash 存储空间紧张的中小容量 MCU。"));
    } else if (opt == "-Og") {
        m_optDescriptionLabel->setText(QStringLiteral("🐞 【标准调试】GCC 官方专为单步调试设计的优化等级。在消除严重延时失真的同时，尽可能保证调用栈、局部变量与 C 源码行号的精准一致性。"));
    } else {
        m_optDescriptionLabel->setText(QStringLiteral("使用用户自定义的编译器优化级别配置。"));
    }
}

CompilerBuildOptions ToolchainConfigDialog::getBuildOptions() const
{
    CompilerBuildOptions opts;
    if (m_optLevelCombo) {
        opts.optLevel = m_optLevelCombo->currentData().toString();
    }
    if (m_debugLevelCombo) {
        opts.debugLevel = m_debugLevelCombo->currentData().toString();
    }
    if (m_tolerantFlagsCheck) {
        opts.enableToleranceFlags = m_tolerantFlagsCheck->isChecked();
    }
    if (m_customFlagsEdit) {
        opts.customCFlags = m_customFlagsEdit->text().trimmed();
    }
    return opts;
}

void ToolchainConfigDialog::setBuildOptions(const CompilerBuildOptions &options)
{
    m_buildOptions = options;
    if (m_optLevelCombo) {
        int idx = m_optLevelCombo->findData(options.optLevel);
        if (idx >= 0) m_optLevelCombo->setCurrentIndex(idx);
    }
    if (m_debugLevelCombo) {
        int idx = m_debugLevelCombo->findData(options.debugLevel);
        if (idx >= 0) m_debugLevelCombo->setCurrentIndex(idx);
    }
    if (m_tolerantFlagsCheck) {
        m_tolerantFlagsCheck->setChecked(options.enableToleranceFlags);
    }
    if (m_customFlagsEdit) {
        m_customFlagsEdit->setText(options.customCFlags);
    }
    updateOptDescription();
}
