#include <QApplication>
#include "idetheme.h"
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
    resize(760, 680);
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
    QPushButton *browseJlink = new QPushButton(QStringLiteral("浏览..."), hardwareGroup);
    connect(browseJlink, &QPushButton::clicked, this, &ToolchainConfigDialog::onBrowseJlink);

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
    hwLayout->addWidget(browseJlink, 2, 2);

    hwLayout->addWidget(new QLabel(QStringLiteral("STM32Prog CLI:"), hardwareGroup), 3, 0);
    hwLayout->addWidget(m_stm32ProgEdit, 3, 1);
    hwLayout->addWidget(browseStm32Prog, 3, 2);

    mainLayout->addWidget(hardwareGroup);

    // 4. 环境自检报告与诊断输出
    QGroupBox *reportGroup = new QGroupBox(QStringLiteral("4. 环境健康度自检报告"), this);
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
