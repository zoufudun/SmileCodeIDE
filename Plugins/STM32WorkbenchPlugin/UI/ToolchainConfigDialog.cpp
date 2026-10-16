#include "ToolchainConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QCompleter>
#include "TargetChipDialog.h"

ToolchainConfigDialog::ToolchainConfigDialog(QWidget *parent, QSettings *settings)
    : QDialog(parent), m_settings(settings), m_detector(new ToolchainDetector(this))
{
    setWindowTitle(QStringLiteral("嵌入式工具链与硬件调试器配置 (Toolchain Settings)"));
    resize(760, 680);
    setupUI();
    applyTheme();

    // 读取已保存设置或执行一次自检
    ToolchainPaths current = m_detector->loadSettings(m_settings);
    updateFormFromPaths(current);

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
    QGridLayout *tgLayout = new QGridLayout(toolchainGroup);
    tgLayout->setColumnStretch(1, 1);

    m_gccEdit = new QLineEdit(toolchainGroup);
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
    m_diagnosticReportEdit->setReadOnly(true);
    m_diagnosticReportEdit->setFont(QFont("Consolas", 9));
    reportLayout->addWidget(m_diagnosticReportEdit);
    mainLayout->addWidget(reportGroup, 1);

    // 5. 底部动作按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_autoDetectBtn = new QPushButton(QStringLiteral("🔍 自动智能扫描环境"), this);
    m_autoDetectBtn->setStyleSheet("background-color: #3867d6; color: white; font-weight: bold; padding: 6px 16px; border-radius: 4px;");
    connect(m_autoDetectBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onAutoDetectClicked);

    m_saveBtn = new QPushButton(QStringLiteral("保存配置"), this);
    m_saveBtn->setStyleSheet("background-color: #20bf6b; color: white; font-weight: bold; padding: 6px 20px; border-radius: 4px;");
    connect(m_saveBtn, &QPushButton::clicked, this, &ToolchainConfigDialog::onSaveClicked);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setStyleSheet("padding: 6px 16px; border-radius: 4px;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_autoDetectBtn);
    btnLayout->addStretch(1);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void ToolchainConfigDialog::applyTheme()
{
    setStyleSheet(
        "QDialog { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Segoe UI', 'Microsoft YaHei'; }"
        "QGroupBox { font-weight: bold; border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding-top: 14px; color: #89b4fa; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QLabel { color: #bac2de; font-size: 12px; }"
        "QLineEdit { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #f5e0dc; selection-background-color: #585b70; }"
        "QLineEdit:focus { border: 1px solid #89b4fa; }"
        "QComboBox { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #cdd6f4; }"
        "QTextEdit { background-color: #11111b; border: 1px solid #313244; border-radius: 4px; color: #a6adc8; }"
        "QPushButton { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #45475a; }"
    );
}

void ToolchainConfigDialog::updateFormFromPaths(const ToolchainPaths &paths)
{
    m_gccEdit->setText(paths.gccPath);
    m_gdbEdit->setText(paths.gdbPath);
    m_makeEdit->setText(paths.makePath);
    m_openocdEdit->setText(paths.openocdPath);
    m_openocdScriptsEdit->setText(paths.openocdScriptsDir);
    m_jlinkEdit->setText(paths.jlinkPath);
    m_stm32ProgEdit->setText(paths.stm32ProgPath);

    m_diagnosticReportEdit->setPlainText(m_detector->generateDiagnosticReport(paths));
}

ToolchainPaths ToolchainConfigDialog::getPaths() const
{
    ToolchainPaths paths;
    paths.gccPath = m_gccEdit->text().trimmed();
    paths.gdbPath = m_gdbEdit->text().trimmed();
    paths.makePath = m_makeEdit->text().trimmed();
    paths.openocdPath = m_openocdEdit->text().trimmed();
    paths.openocdScriptsDir = m_openocdScriptsEdit->text().trimmed();
    paths.jlinkPath = m_jlinkEdit->text().trimmed();
    paths.stm32ProgPath = m_stm32ProgEdit->text().trimmed();

    if (!paths.gccPath.isEmpty()) {
        ToolchainPaths companions;
        if (ToolchainDetector::validateArmGccDir(QFileInfo(paths.gccPath).absolutePath(), companions)) {
            paths.gppPath = companions.gppPath;
            paths.objcopyPath = companions.objcopyPath;
            paths.sizePath = companions.sizePath;
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
    ToolchainPaths detected = m_detector->autoDetect();
    const ToolchainPaths current = getPaths();
    if (detected.gccPath.isEmpty()) detected.gccPath = current.gccPath;
    if (detected.gdbPath.isEmpty()) detected.gdbPath = current.gdbPath;
    if (detected.makePath.isEmpty()) detected.makePath = current.makePath;
    if (detected.openocdPath.isEmpty()) detected.openocdPath = current.openocdPath;
    if (detected.openocdScriptsDir.isEmpty()) detected.openocdScriptsDir = current.openocdScriptsDir;
    if (detected.jlinkPath.isEmpty()) detected.jlinkPath = current.jlinkPath;
    if (detected.stm32ProgPath.isEmpty()) detected.stm32ProgPath = current.stm32ProgPath;
    updateFormFromPaths(detected);
    QMessageBox::information(this, QStringLiteral("扫描完成"),
                             QStringLiteral("已扫描 PATH、环境变量和常见安装目录。结果已填入表单，请检查报告后保存。"));
}

void ToolchainConfigDialog::onBrowseGcc()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 arm-none-eabi-gcc 可执行文件"),
                                               m_gccEdit->text(), "GCC Compiler (arm-none-eabi-gcc*.exe arm-none-eabi-gcc);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_gccEdit->setText(QDir::toNativeSeparators(file));
        ToolchainPaths p = getPaths();
        m_diagnosticReportEdit->setPlainText(m_detector->generateDiagnosticReport(p));
    }
}

void ToolchainConfigDialog::onBrowseGdb()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 arm-none-eabi-gdb 可执行文件"),
                                               m_gdbEdit->text(), "GDB Debugger (arm-none-eabi-gdb*.exe arm-none-eabi-gdb);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_gdbEdit->setText(QDir::toNativeSeparators(file));
    }
}

void ToolchainConfigDialog::onBrowseMake()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 make 或 mingw32-make 可执行文件"),
                                               m_makeEdit->text(), "Make (*make*.exe make);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_makeEdit->setText(QDir::toNativeSeparators(file));
    }
}

void ToolchainConfigDialog::onBrowseOpenocd()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 openocd 可执行文件"),
                                               m_openocdEdit->text(), "OpenOCD (openocd*.exe openocd);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_openocdEdit->setText(QDir::toNativeSeparators(file));
    }
}

void ToolchainConfigDialog::onBrowseOpenocdScripts()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 OpenOCD scripts 文件夹"),
                                                    m_openocdScriptsEdit->text());
    if (!dir.isEmpty()) {
        m_openocdScriptsEdit->setText(QDir::toNativeSeparators(dir));
    }
}

void ToolchainConfigDialog::onBrowseJlink()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 JLink 可执行文件"),
                                               m_jlinkEdit->text(), "J-Link (JLink*.exe JLinkExe);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_jlinkEdit->setText(QDir::toNativeSeparators(file));
    }
}

void ToolchainConfigDialog::onBrowseStm32Prog()
{
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 STM32_Programmer_CLI 可执行文件"),
                                               m_stm32ProgEdit->text(), "Programmer (STM32_Programmer_CLI*.exe);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_stm32ProgEdit->setText(QDir::toNativeSeparators(file));
    }
}

void ToolchainConfigDialog::onSaveClicked()
{
    ToolchainPaths paths = getPaths();
    m_detector->saveSettings(m_settings, paths);

    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        m_settings->setValue("selectedChip", getSelectedChip());
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
