#include "FlashConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QFileInfo>

FlashConfigDialog::FlashConfigDialog(QWidget *parent, QSettings *settings)
    : QDialog(parent), m_settings(settings)
{
    setWindowTitle(QStringLiteral("固件烧录与下载参数配置 (Flash Settings)"));
    resize(650, 660);
    setupUI();
    applyTheme();
    loadFromSettings();
    updateDriverOptions();
    scanProbes();
}

void FlashConfigDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // 1. 烧录器驱动选择
    QGroupBox *driverGroup = new QGroupBox(QStringLiteral("1. 烧录下载器类型 (Programmer Driver)"), this);
    QVBoxLayout *dgLayout = new QVBoxLayout(driverGroup);
    m_driverCombo = new QComboBox(driverGroup);
    m_driverCombo->addItem(QStringLiteral("OpenOCD (ST-Link V2/V3)"), static_cast<int>(FlashDriverType::OpenOCD_STLink));
    m_driverCombo->addItem(QStringLiteral("OpenOCD (CMSIS-DAP / DAPLink)"), static_cast<int>(FlashDriverType::OpenOCD_CMSISDAP));
    m_driverCombo->addItem(QStringLiteral("OpenOCD (SEGGER J-Link)"), static_cast<int>(FlashDriverType::OpenOCD_JLink));
    m_driverCombo->addItem(QStringLiteral("SEGGER J-Link 官方 (JLink.exe SWD)"), static_cast<int>(FlashDriverType::JLink_Native));
    m_driverCombo->addItem(QStringLiteral("STMicroelectronics STM32CubeProgrammer CLI"), static_cast<int>(FlashDriverType::STM32CubeProg));
    m_driverCombo->addItem(QStringLiteral("开源 st-flash (stlink-tools)"), static_cast<int>(FlashDriverType::StFlash));
    connect(m_driverCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateDriverOptions(); });
    dgLayout->addWidget(m_driverCombo);
    mainLayout->addWidget(driverGroup);

    QGroupBox *probeGroup = new QGroupBox(QStringLiteral("2. 当前接入的下载器"), this);
    QGridLayout *probeLayout = new QGridLayout(probeGroup);
    m_probeCombo = new QComboBox(probeGroup);
    m_probeCombo->setObjectName("probeCombo");
    QPushButton *refreshButton = new QPushButton(QStringLiteral("刷新设备"), probeGroup);
    connect(refreshButton, &QPushButton::clicked, this, &FlashConfigDialog::scanProbes);
    connect(m_probeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FlashConfigDialog::onProbeChanged);
    probeLayout->addWidget(new QLabel(QStringLiteral("设备:"), probeGroup), 0, 0);
    probeLayout->addWidget(m_probeCombo, 0, 1);
    probeLayout->addWidget(refreshButton, 0, 2);
    m_probeInfoLabel = new QLabel(probeGroup);
    m_probeInfoLabel->setWordWrap(true);
    m_probeInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    probeLayout->addWidget(m_probeInfoLabel, 1, 0, 1, 3);
    m_probeSerialEdit = new QLineEdit(probeGroup);
    m_probeSerialEdit->setObjectName("probeSerialEdit");
    m_probeSerialEdit->setPlaceholderText(QStringLiteral("自动读取；无法读取时可手动填写"));
    probeLayout->addWidget(new QLabel(QStringLiteral("序列号:"), probeGroup), 2, 0);
    probeLayout->addWidget(m_probeSerialEdit, 2, 1, 1, 2);
    mainLayout->addWidget(probeGroup);

    // 烧录控制参数
    QGroupBox *paramGroup = new QGroupBox(QStringLiteral("3. 烧录与动作选项 (Flash Options)"), this);
    QGridLayout *pgLayout = new QGridLayout(paramGroup);

    m_targetAddrEdit = new QLineEdit("0x08000000", paramGroup);
    pgLayout->addWidget(new QLabel(QStringLiteral("Flash 起始基地址:"), paramGroup), 0, 0);
    pgLayout->addWidget(m_targetAddrEdit, 0, 1);

    m_speedSpin = new QSpinBox(paramGroup);
    m_speedSpin->setRange(100, 20000);
    m_speedSpin->setSingleStep(500);
    m_speedSpin->setValue(4000);
    m_speedSpin->setSuffix(" kHz");
    pgLayout->addWidget(new QLabel(QStringLiteral("SWD 时钟速率:"), paramGroup), 1, 0);
    pgLayout->addWidget(m_speedSpin, 1, 1);

    m_verifyCheck = new QCheckBox(QStringLiteral("烧录后执行校验 (Verify Flash)"), paramGroup);
    m_verifyCheck->setChecked(true);
    pgLayout->addWidget(m_verifyCheck, 2, 0, 1, 2);

    m_resetCheck = new QCheckBox(QStringLiteral("烧录完成后自动复位并运行 (Reset and Run)"), paramGroup);
    m_resetCheck->setChecked(true);
    pgLayout->addWidget(m_resetCheck, 3, 0, 1, 2);

    m_eraseAllCheck = new QCheckBox(QStringLiteral("全片擦除 (Mass Erase, 默认扇区擦除)"), paramGroup);
    m_eraseAllCheck->setChecked(false);
    pgLayout->addWidget(m_eraseAllCheck, 4, 0, 1, 2);

    mainLayout->addWidget(paramGroup);

    // 3. 高级 OpenOCD 覆盖参数 (可选)
    QGroupBox *advGroup = new QGroupBox(QStringLiteral("4. 自定义 OpenOCD 脚本 (可选)"), this);
    QGridLayout *advLayout = new QGridLayout(advGroup);

    m_customInterfaceEdit = new QLineEdit(advGroup);
    m_customInterfaceEdit->setPlaceholderText(QStringLiteral("默认按上方类型自动加载 (如 interface/stlink.cfg)"));
    advLayout->addWidget(new QLabel(QStringLiteral("Interface 脚本:"), advGroup), 0, 0);
    advLayout->addWidget(m_customInterfaceEdit, 0, 1);

    m_customTargetEdit = new QLineEdit(advGroup);
    m_customTargetEdit->setPlaceholderText(QStringLiteral("默认根据芯片系列自动匹配 (如 target/stm32f1x.cfg)"));
    advLayout->addWidget(new QLabel(QStringLiteral("Target 脚本:"), advGroup), 1, 0);
    advLayout->addWidget(m_customTargetEdit, 1, 1);

    mainLayout->addWidget(advGroup);

    // 4. 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_saveBtn = new QPushButton(QStringLiteral("确定并保存"), this);
    m_saveBtn->setStyleSheet("background-color: #20bf6b; color: white; font-weight: bold; padding: 6px 18px; border-radius: 4px;");
    connect(m_saveBtn, &QPushButton::clicked, this, &FlashConfigDialog::onSaveClicked);

    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void FlashConfigDialog::applyTheme()
{
    setStyleSheet(
        "QDialog { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Segoe UI', 'Microsoft YaHei'; }"
        "QGroupBox { font-weight: bold; border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding-top: 14px; color: #89b4fa; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QLabel { color: #bac2de; font-size: 12px; }"
        "QLineEdit, QSpinBox { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #f5e0dc; }"
        "QComboBox { background-color: #181825; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; color: #cdd6f4; }"
        "QCheckBox { color: #cdd6f4; spacing: 6px; }"
        "QPushButton { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 14px; }"
        "QPushButton:hover { background-color: #45475a; }"
    );
}

void FlashConfigDialog::loadFromSettings()
{
    if (!m_settings) return;

    m_settings->beginGroup("STM32Flash");
    int driverVal = m_settings->value("driver", static_cast<int>(FlashDriverType::OpenOCD_STLink)).toInt();
    int idx = m_driverCombo->findData(driverVal);
    if (idx >= 0) m_driverCombo->setCurrentIndex(idx);

    m_targetAddrEdit->setText(m_settings->value("targetAddr", "0x08000000").toString());
    m_speedSpin->setValue(m_settings->value("speedKhz", 4000).toInt());
    m_verifyCheck->setChecked(m_settings->value("verify", true).toBool());
    m_resetCheck->setChecked(m_settings->value("reset", true).toBool());
    m_eraseAllCheck->setChecked(m_settings->value("eraseAll", false).toBool());
    m_customInterfaceEdit->setText(m_settings->value("customInterface", "").toString());
    m_customTargetEdit->setText(m_settings->value("customTarget", "").toString());
    m_hasSavedProbeChoice = m_settings->contains("probeInstanceId");
    m_savedProbeId = m_settings->value("probeInstanceId").toString();
    m_probeSerialEdit->setText(m_settings->value("probeSerial").toString());
    m_settings->endGroup();
    updateDriverOptions();
}

void FlashConfigDialog::updateDriverOptions()
{
    const auto driver = static_cast<FlashDriverType>(m_driverCombo->currentData().toInt());
    m_eraseAllCheck->setEnabled(FlashProgrammer::driverSupportsMassErase(driver));
    if (!m_eraseAllCheck->isEnabled()) m_eraseAllCheck->setChecked(false);
    m_eraseAllCheck->setToolTip(m_eraseAllCheck->isEnabled() ? QString() :
        QStringLiteral("全片擦除目前由 J-Link 官方工具和 STM32CubeProgrammer 支持"));
    const bool alwaysVerifies = driver == FlashDriverType::StFlash || driver == FlashDriverType::JLink_Native;
    if (alwaysVerifies) m_verifyCheck->setChecked(true);
    m_verifyCheck->setEnabled(!alwaysVerifies);
    m_verifyCheck->setToolTip(alwaysVerifies ? QStringLiteral("该工具写入后会自动校验") : QString());
}

void FlashConfigDialog::scanProbes()
{
    const bool initialScan = m_probeCombo->count() == 0;
    const QString wantedId = initialScan ? m_savedProbeId : m_probeCombo->currentData().toString();
    const QString currentSerial = m_probeSerialEdit->text().trimmed();
    m_probes = ProbeDetector::scan();
    QSignalBlocker blocked(m_probeCombo);
    m_probeCombo->clear();
    m_probeCombo->addItem(QStringLiteral("自动选择（不指定设备）"), QString());
    int selected = 0;
    for (const ProbeInfo &probe : m_probes) {
        const QString label = QString("%1 · %2%3").arg(ProbeDetector::typeName(probe.type), probe.name,
            probe.serial.isEmpty() ? QString() : QString(" · %1").arg(probe.serial));
        m_probeCombo->addItem(label, probe.instanceId);
        if (!wantedId.isEmpty() && probe.instanceId.compare(wantedId, Qt::CaseInsensitive) == 0)
            selected = m_probeCombo->count() - 1;
    }
    if (!wantedId.isEmpty() && selected == 0) {
        m_probeCombo->addItem(QStringLiteral("已保存设备（当前未连接）"), wantedId);
        selected = m_probeCombo->count() - 1;
    }
    const bool autoSelectSingle = initialScan && !m_hasSavedProbeChoice && m_probes.size() == 1;
    if (autoSelectSingle) selected = 1;
    m_probeCombo->setCurrentIndex(selected);
    QString selectedSerial = currentSerial;
    if (selected > 0 && selected <= m_probes.size() && !m_probes[selected - 1].serial.isEmpty())
        selectedSerial = m_probes[selected - 1].serial;
    m_probeSerialEdit->setText(selectedSerial);
    if (autoSelectSingle) onProbeChanged(selected);
    else updateProbeInfo();
}

void FlashConfigDialog::updateProbeInfo()
{
    const QString id = m_probeCombo->currentData().toString();
    for (const ProbeInfo &probe : m_probes) {
        if (probe.instanceId.compare(id, Qt::CaseInsensitive) != 0 || id.isEmpty()) continue;
        m_probeInfoLabel->setText(QString("名称: %1\n厂商: %2  ·  VID/PID: %3:%4\nUSB 序列号: %5\n设备 ID: %6")
            .arg(probe.name, probe.manufacturer.isEmpty() ? QStringLiteral("未知") : probe.manufacturer,
                 probe.vendorId, probe.productId,
                 probe.serial.isEmpty() ? QStringLiteral("未读取到，请手动填写") : probe.serial,
                 probe.instanceId));
        return;
    }
    m_probeInfoLabel->setText(id.isEmpty() ?
        QString("当前识别到 %1 个下载器；自动模式由烧录工具选择设备。多设备时请指定一台。").arg(m_probes.size()) :
        QStringLiteral("所选设备当前未连接；烧录前请刷新设备列表。"));
}

void FlashConfigDialog::onProbeChanged(int)
{
    const QString id = m_probeCombo->currentData().toString();
    m_probeSerialEdit->clear();
    for (const ProbeInfo &probe : m_probes) {
        if (probe.instanceId.compare(id, Qt::CaseInsensitive) != 0 || id.isEmpty()) continue;
        m_probeSerialEdit->setText(probe.serial);
        FlashDriverType suggested = FlashDriverType::OpenOCD_STLink;
        if (probe.type == ProbeType::CmsisDap) suggested = FlashDriverType::OpenOCD_CMSISDAP;
        if (probe.type == ProbeType::JLink) {
            const QString path = m_settings ? m_settings->value("STM32Toolchain/jlinkPath").toString() : QString();
            suggested = QFileInfo::exists(path) ? FlashDriverType::JLink_Native : FlashDriverType::OpenOCD_JLink;
        }
        const int driverIndex = m_driverCombo->findData(static_cast<int>(suggested));
        if (driverIndex >= 0) m_driverCombo->setCurrentIndex(driverIndex);
        break;
    }
    updateProbeInfo();
}

void FlashConfigDialog::onSaveClicked()
{
    if (!FlashProgrammer::validTargetAddress(m_targetAddrEdit->text().trimmed())) {
        QMessageBox::warning(this, QStringLiteral("地址无效"),
                             QStringLiteral("Flash 起始地址必须是有效的 32 位十六进制或十进制数。"));
        return;
    }
    const QString serial = m_probeSerialEdit->text().trimmed();
    static const QRegularExpression serialPattern("^[A-Za-z0-9._:-]*$");
    if (!serialPattern.match(serial).hasMatch()) {
        QMessageBox::warning(this, QStringLiteral("序列号无效"),
                             QStringLiteral("下载器序列号只能包含字母、数字、点、下划线、冒号和连字符。"));
        return;
    }
    if (!m_probeCombo->currentData().toString().isEmpty() && serial.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少序列号"),
                             QStringLiteral("选定设备没有可用序列号，请手动填写后保存。"));
        return;
    }
    const QString selectedId = m_probeCombo->currentData().toString();
    for (const ProbeInfo &probe : m_probes) {
        if (selectedId.isEmpty() || probe.instanceId.compare(selectedId, Qt::CaseInsensitive) != 0) continue;
        const auto driver = static_cast<FlashDriverType>(m_driverCombo->currentData().toInt());
        if (!FlashProgrammer::driverSupportsProbe(driver, probe.type)) {
            QMessageBox::warning(this, QStringLiteral("驱动不匹配"),
                                 QStringLiteral("请选择与当前下载器类型匹配的烧录驱动。"));
            return;
        }
        break;
    }
    if (m_settings) {
        m_settings->beginGroup("STM32Flash");
        m_settings->setValue("driver", m_driverCombo->currentData().toInt());
        m_settings->setValue("targetAddr", m_targetAddrEdit->text().trimmed());
        m_settings->setValue("speedKhz", m_speedSpin->value());
        m_settings->setValue("verify", m_verifyCheck->isChecked());
        m_settings->setValue("reset", m_resetCheck->isChecked());
        m_settings->setValue("eraseAll", m_eraseAllCheck->isEnabled() && m_eraseAllCheck->isChecked());
        m_settings->setValue("customInterface", m_customInterfaceEdit->text().trimmed());
        m_settings->setValue("customTarget", m_customTargetEdit->text().trimmed());
        m_settings->setValue("probeInstanceId", m_probeCombo->currentData().toString());
        m_settings->setValue("probeSerial", serial);
        m_settings->endGroup();
        m_settings->sync();
    }
    accept();
}

FlashOptions FlashConfigDialog::getOptions() const
{
    FlashOptions opt;
    opt.driver = static_cast<FlashDriverType>(m_driverCombo->currentData().toInt());
    opt.targetAddress = m_targetAddrEdit->text().trimmed();
    opt.swdSpeedKhz = m_speedSpin->value();
    opt.verify = m_verifyCheck->isChecked();
    opt.resetAndRun = m_resetCheck->isChecked();
    opt.eraseAll = m_eraseAllCheck->isEnabled() && m_eraseAllCheck->isChecked();
    opt.customOpenocdInterfaceCfg = m_customInterfaceEdit->text().trimmed();
    opt.customOpenocdTargetCfg = m_customTargetEdit->text().trimmed();
    opt.probeInstanceId = m_probeCombo->currentData().toString();
    opt.probeSerial = m_probeSerialEdit->text().trimmed();
    return opt;
}
