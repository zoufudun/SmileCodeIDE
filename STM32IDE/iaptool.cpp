#include "iaptool.h"
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>
#include <QVBoxLayout>

IAPTool::IAPTool(QWidget *parent)
    : QWidget(parent), m_isConnected(false), m_isTransferring(false) {
  m_serialPort = new QSerialPort(this);
  m_udpSocket = new QUdpSocket(this);
  m_tcpSocket = new QTcpSocket(this);

  setupUi();
  setupConnections();

  refreshSerialPorts();

  // Default to Serial
  m_protocolCombo->setCurrentIndex(0);
  onProtocolChanged(0);
}

IAPTool::~IAPTool() {
  if (m_serialPort->isOpen())
    m_serialPort->close();
  if (m_tcpSocket->isOpen())
    m_tcpSocket->close();
  if (m_udpSocket->isOpen())
    m_udpSocket->close();
}

void IAPTool::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Top Control Area
  QGroupBox *configGroup = new QGroupBox("通信配置", this);
  QVBoxLayout *configLayout = new QVBoxLayout(configGroup);

  // Protocol Selector
  QHBoxLayout *protoLayout = new QHBoxLayout();
  protoLayout->addWidget(new QLabel("通信协议:"));
  m_protocolCombo = new QComboBox();
  m_protocolCombo->addItems(
      {"串口 (UART/USB-CDC)", "USB (HID/DFU)", "WIFI (TCP)", "UDP"});
  protoLayout->addWidget(m_protocolCombo);
  protoLayout->addStretch();
  configLayout->addLayout(protoLayout);

  // Stacked Settings
  m_settingsStack = new QStackedWidget();
  m_settingsStack->addWidget(createSerialPage()); // Index 0
  m_settingsStack->addWidget(createUsbPage());    // Index 1
  m_settingsStack->addWidget(createWifiPage());   // Index 2
  m_settingsStack->addWidget(createUdpPage());    // Index 3

  configLayout->addWidget(m_settingsStack);

  // Connect Button
  m_btnConnect = new QPushButton("打开连接");
  configLayout->addWidget(m_btnConnect);

  mainLayout->addWidget(configGroup);

  // File Selection
  QGroupBox *fileGroup = new QGroupBox("固件文件", this);
  QHBoxLayout *fileLayout = new QHBoxLayout(fileGroup);
  m_filePathEdit = new QLineEdit();
  m_filePathEdit->setPlaceholderText("请选择 .bin 或 .hex 文件");
  m_btnBrowse = new QPushButton("浏览");
  fileLayout->addWidget(m_filePathEdit);
  fileLayout->addWidget(m_btnBrowse);
  mainLayout->addWidget(fileGroup);

  // Progress
  m_progressBar = new QProgressBar();
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  mainLayout->addWidget(m_progressBar);

  // Actions
  QHBoxLayout *actionLayout = new QHBoxLayout();
  m_btnStart = new QPushButton("开始升级");
  m_btnStart->setEnabled(false); // Disabled until connected and file selected
  actionLayout->addWidget(m_btnStart);
  mainLayout->addLayout(actionLayout);

  // Log
  m_logText = new QTextEdit();
  m_logText->setReadOnly(true);
  mainLayout->addWidget(m_logText);
}

QWidget *IAPTool::createSerialPage() {
  QWidget *page = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new QLabel("端口:"));
  m_serialPortCombo = new QComboBox();
  layout->addWidget(m_serialPortCombo);

  QPushButton *btnRefresh = new QPushButton("刷新");
  connect(btnRefresh, &QPushButton::clicked, this,
          &IAPTool::refreshSerialPorts);
  layout->addWidget(btnRefresh);

  layout->addWidget(new QLabel("波特率:"));
  m_baudRateCombo = new QComboBox();
  m_baudRateCombo->addItems({"9600", "19200", "38400", "57600", "115200",
                             "230400", "460800", "921600"});
  m_baudRateCombo->setCurrentText("115200");
  layout->addWidget(m_baudRateCombo);

  layout->addStretch();
  return page;
}

QWidget *IAPTool::createUsbPage() {
  QWidget *page = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new QLabel("USB 模式:"));
  QComboBox *usbMode = new QComboBox();
  usbMode->addItems({"HID 设备", "Mass Storage", "DFU"});
  layout->addWidget(usbMode);

  layout->addStretch();
  return page;
}

QWidget *IAPTool::createWifiPage() {
  QWidget *page = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new QLabel("设备IP:"));
  m_ipEdit = new QLineEdit("192.168.1.100");
  layout->addWidget(m_ipEdit);

  layout->addWidget(new QLabel("端口:"));
  m_portScan = new QSpinBox();
  m_portScan->setRange(1, 65535);
  m_portScan->setValue(8080);
  layout->addWidget(m_portScan);

  layout->addStretch();
  return page;
}

QWidget *IAPTool::createUdpPage() {
  // Reusing the same structure as Wifi for IP/Port
  return createWifiPage();
}

void IAPTool::setupConnections() {
  connect(m_protocolCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onProtocolChanged(int)));
  connect(m_btnBrowse, &QPushButton::clicked, this, &IAPTool::onBrowseFile);
  connect(m_btnConnect, &QPushButton::clicked, this,
          &IAPTool::onConnectClicked);
  connect(m_btnStart, &QPushButton::clicked, this, &IAPTool::onStartUpgrade);

  // Serial
  connect(m_serialPort, &QSerialPort::readyRead, this,
          &IAPTool::onSerialDataReceived);

  // TCP
  connect(m_tcpSocket, &QTcpSocket::connected, this, &IAPTool::onTcpConnected);
  connect(m_tcpSocket, &QTcpSocket::disconnected, this,
          &IAPTool::onTcpDisconnected);
  connect(m_tcpSocket, &QTcpSocket::readyRead, this,
          &IAPTool::onTcpDataReceived);

  // UDP
  connect(m_udpSocket, &QUdpSocket::readyRead, this,
          &IAPTool::onUdpDataReceived);
}

void IAPTool::onProtocolChanged(int index) {
  m_settingsStack->setCurrentIndex(index);
  // Reset connection state when protocol changes
  if (m_isConnected) {
    onConnectClicked(); // Will disconnect
  }
}

void IAPTool::refreshSerialPorts() {
  m_serialPortCombo->clear();
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
    m_serialPortCombo->addItem(info.portName() + ": " + info.description(),
                               info.portName());
  }
}

void IAPTool::onBrowseFile() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "选择固件", "", "Firmware (*.bin *.hex)"); //  *.s19 *.dfu
  if (!fileName.isEmpty()) {
    m_filePathEdit->setText(fileName);
    log("已选择文件: " + fileName);
  }
}

void IAPTool::onConnectClicked() {
  int protocol = m_protocolCombo->currentIndex();

  if (m_isConnected) {
    // Disconnect
    switch (protocol) {
    case 0: // Serial
      m_serialPort->close();
      break;
    case 2: // Network
      m_tcpSocket->disconnectFromHost();
      break;
    case 3: // UDP
      m_udpSocket->close();
      break;
    }
    m_isConnected = false;
    m_btnConnect->setText("打开连接");
    m_btnStart->setEnabled(false);
    m_protocolCombo->setEnabled(true);
    log("已断开连接");
  } else {
    // Connect
    bool success = false;

    if (protocol == 0) { // Serial
      QString portName = m_serialPortCombo->currentData().toString();
      m_serialPort->setPortName(portName);
      m_serialPort->setBaudRate(m_baudRateCombo->currentText().toInt());
      if (m_serialPort->open(QIODevice::ReadWrite)) {
        success = true;
      } else {
        log("串口打开失败: " + m_serialPort->errorString());
      }
    } else if (protocol == 2) { // TCP
      m_tcpSocket->connectToHost(m_ipEdit->text(), m_portScan->value());
      // Wait for connected signal
      m_protocolCombo->setEnabled(false);
      return;                   // Success handled in slot
    } else if (protocol == 3) { // UDP
      // UDP is connectionless, but we bind or just prepare
      // Check IP format
      success = true; // Always "connected"
    }

    if (success) {
      m_isConnected = true;
      m_btnConnect->setText("关闭连接");
      m_btnStart->setEnabled(true);
      m_protocolCombo->setEnabled(false);
      log("连接成功");
    }
  }
}

void IAPTool::onTcpConnected() {
  m_isConnected = true;
  m_btnConnect->setText("关闭连接");
  m_btnStart->setEnabled(true);
  log("TCP连接成功");
}

void IAPTool::onTcpDisconnected() {
  m_isConnected = false;
  m_btnConnect->setText("打开连接");
  m_btnStart->setEnabled(false);
  m_protocolCombo->setEnabled(true);
  log("TCP连接断开");
}

void IAPTool::onStartUpgrade() {
  if (!m_isConnected && m_protocolCombo->currentIndex() != 3) { // UDP exception
    QMessageBox::warning(this, "警告", "请先连接设备");
    return;
  }

  QString fileName = m_filePathEdit->text();
  if (fileName.isEmpty()) {
    QMessageBox::warning(this, "警告", "请选择固件文件");
    return;
  }

  m_firmwareFile.setFileName(fileName);
  if (!m_firmwareFile.open(QIODevice::ReadOnly)) {
    log("无法打开文件");
    return;
  }

  m_totalBytes = m_firmwareFile.size();
  m_bytesSent = 0;
  m_progressBar->setMaximum(100);
  m_progressBar->setValue(0);

  log("开始升级...");

  // Simulate/Start sending - This is where the specific protocol (Ymodem etc)
  // would go For now, we'll just read and fake a send or send raw bytes

  QByteArray data = m_firmwareFile.readAll(); // Read small chunks in real app
  m_firmwareFile.close();                     // For this simple demo

  int protocol = m_protocolCombo->currentIndex();
  if (protocol == 0) {
    m_serialPort->write(data);
  } else if (protocol == 2) {
    m_tcpSocket->write(data);
  } else if (protocol == 3) {
    m_udpSocket->writeDatagram(data, QHostAddress(m_ipEdit->text()),
                               m_portScan->value());
  }

  // In a real IAP, we'd wait for ACKs. Here we just assume success for the
  // demo.
  m_progressBar->setValue(100);
  log("发送完成 (Raw Data)");
}

void IAPTool::onSerialDataReceived() {
  QByteArray data = m_serialPort->readAll();
  log("RX: " + data.toHex());
}

void IAPTool::onTcpDataReceived() {
  QByteArray data = m_tcpSocket->readAll();
  log("RX TCP: " + data.toHex());
}

void IAPTool::onUdpDataReceived() {
  while (m_udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(int(m_udpSocket->pendingDatagramSize()));
    m_udpSocket->readDatagram(datagram.data(), datagram.size());
    log("RX UDP: " + datagram.toHex());
  }
}

void IAPTool::log(const QString &msg) {
  QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
  m_logText->append(QString("[%1] %2").arg(time).arg(msg));
}
