#include "protocolconfigdialog.h"
#include <QMessageBox>

CommProtocolConfigDialog::CommProtocolConfigDialog(const CommProtocolConfig &config, QWidget *parent)
    : QDialog(parent), m_config(config) {
  setWindowTitle(QStringLiteral("通信协议与数据源配置"));
  setMinimumWidth(420);
  setStyleSheet(
      "QDialog { background: #111827; color: #E2E8F0; font-family: 'Microsoft YaHei'; }"
      "QLabel { color: #94A3B8; font-size: 12px; }"
      "QLineEdit, QComboBox, QSpinBox { background: #1E293B; color: #E2E8F0; "
      "border: 1px solid #334155; padding: 6px; border-radius: 4px; font-size: 12px; }"
      "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #00D4FF; }"
      "QGroupBox { border: 1px solid #1E293B; border-radius: 6px; margin-top: 10px; font-size: 12px; font-weight: bold; color: #00D4FF; padding-top: 15px; }"
      "QPushButton { background: #1E3A5F; color: #00D4FF; border: 1px solid #00D4FF; padding: 6px 18px; border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #00D4FF; color: #111827; }");

  setupUi();
}

void CommProtocolConfigDialog::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(16);

  // 1. 协议选择 Header
  auto *topGroup = new QGroupBox(QStringLiteral("通信协议类型选择"), this);
  auto *topLayout = new QFormLayout(topGroup);
  topLayout->setContentsMargins(15, 15, 15, 15);

  m_protocolTypeCombo = new QComboBox(topGroup);
  m_protocolTypeCombo->addItem(QStringLiteral("🚗 CAN 总线 (USBCAN / CANFD)"), static_cast<int>(ProtocolType::CAN_Bus));
  m_protocolTypeCombo->addItem(QStringLiteral("🔌 Modbus RTU 协议 (RS232/RS485)"), static_cast<int>(ProtocolType::Modbus_RTU));
  m_protocolTypeCombo->addItem(QStringLiteral("🌐 Modbus TCP 协议 (以太网)"), static_cast<int>(ProtocolType::Modbus_TCP));
  m_protocolTypeCombo->addItem(QStringLiteral("💻 RS232 / RS485 串口直连"), static_cast<int>(ProtocolType::RS232_RS485));
  m_protocolTypeCombo->addItem(QStringLiteral("📡 TCP / UDP 网络 Socket"), static_cast<int>(ProtocolType::TCP_UDP_Socket));

  int pIdx = m_protocolTypeCombo->findData(static_cast<int>(m_config.protocolType));
  if (pIdx >= 0) m_protocolTypeCombo->setCurrentIndex(pIdx);

  connect(m_protocolTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &CommProtocolConfigDialog::onProtocolChanged);
  topLayout->addRow(QStringLiteral("当前通信协议:"), m_protocolTypeCombo);

  mainLayout->addWidget(topGroup);

  // 2. 堆栈参数卡片
  m_stackedPages = new QStackedWidget(this);

  // ---- Page 0: CAN 总线 ----
  auto *canPage = new QWidget();
  auto *canLayout = new QFormLayout(canPage);
  canLayout->setContentsMargins(15, 15, 15, 15);

  m_canChannelCombo = new QComboBox(canPage);
  m_canChannelCombo->addItem(QStringLiteral("任意通道 (全通道混合接收)"), -1);
  m_canChannelCombo->addItem(QStringLiteral("CAN 通道 0 (仅接收 0 通道数据)"), 0);
  m_canChannelCombo->addItem(QStringLiteral("CAN 通道 1 (仅接收 1 通道数据)"), 1);
  int chIdx = m_canChannelCombo->findData(m_config.canChannel);
  if (chIdx >= 0) m_canChannelCombo->setCurrentIndex(chIdx);
  canLayout->addRow(QStringLiteral("数据源接收通道:"), m_canChannelCombo);

  m_canDeviceEdit = new QLineEdit(m_config.canDeviceName, canPage);
  m_canDeviceEdit->setReadOnly(true);
  canLayout->addRow(QStringLiteral("检测到的硬件适配器:"), m_canDeviceEdit);

  m_stackedPages->addWidget(canPage);

  // ---- Page 1: Modbus RTU / 串口 ----
  auto *serialPage = new QWidget();
  auto *serialLayout = new QFormLayout(serialPage);
  serialLayout->setContentsMargins(15, 15, 15, 15);

  m_serialPortEdit = new QLineEdit(m_config.serialPort, serialPage);
  serialLayout->addRow(QStringLiteral("串口号 (COM Port):"), m_serialPortEdit);

  m_baudRateCombo = new QComboBox(serialPage);
  m_baudRateCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400"});
  m_baudRateCombo->setCurrentText(QString::number(m_config.baudRate));
  serialLayout->addRow(QStringLiteral("波特率 (Baud Rate):"), m_baudRateCombo);

  m_slaveIdSpin = new QSpinBox(serialPage);
  m_slaveIdSpin->setRange(1, 247);
  m_slaveIdSpin->setValue(m_config.slaveId);
  serialLayout->addRow(QStringLiteral("Modbus 从机地址 (Slave ID):"), m_slaveIdSpin);

  m_stackedPages->addWidget(serialPage);

  // ---- Page 2: Modbus TCP / Socket ----
  auto *netPage = new QWidget();
  auto *netLayout = new QFormLayout(netPage);
  netLayout->setContentsMargins(15, 15, 15, 15);

  m_ipAddressEdit = new QLineEdit(m_config.ipAddress, netPage);
  netLayout->addRow(QStringLiteral("目标 IP 地址:"), m_ipAddressEdit);

  m_portSpin = new QSpinBox(netPage);
  m_portSpin->setRange(1, 65535);
  m_portSpin->setValue(m_config.port);
  netLayout->addRow(QStringLiteral("端口号 (Port):"), m_portSpin);

  m_stackedPages->addWidget(netPage);

  mainLayout->addWidget(m_stackedPages);

  // 初始页面联动
  onProtocolChanged(m_protocolTypeCombo->currentIndex());

  // 3. 按钮
  auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(btnBox);
}

void CommProtocolConfigDialog::onProtocolChanged(int index) {
  int pTypeVal = m_protocolTypeCombo->itemData(index).toInt();
  if (pTypeVal == static_cast<int>(ProtocolType::CAN_Bus)) {
    m_stackedPages->setCurrentIndex(0);
  } else if (pTypeVal == static_cast<int>(ProtocolType::Modbus_RTU) ||
             pTypeVal == static_cast<int>(ProtocolType::RS232_RS485)) {
    m_stackedPages->setCurrentIndex(1);
  } else {
    m_stackedPages->setCurrentIndex(2);
  }
}

CommProtocolConfig CommProtocolConfigDialog::config() const {
  CommProtocolConfig res = m_config;
  res.protocolType = static_cast<ProtocolType>(m_protocolTypeCombo->currentData().toInt());

  // CAN
  res.canChannel = m_canChannelCombo->currentData().toInt();
  res.canDeviceName = m_canDeviceEdit->text().trimmed();

  // Serial / Modbus RTU
  res.serialPort = m_serialPortEdit->text().trimmed();
  res.baudRate = m_baudRateCombo->currentText().toInt();
  res.slaveId = m_slaveIdSpin->value();

  // Network / Modbus TCP
  res.ipAddress = m_ipAddressEdit->text().trimmed();
  res.port = m_portSpin->value();

  return res;
}
