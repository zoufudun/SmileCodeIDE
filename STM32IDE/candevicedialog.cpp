#include "candevicedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "caninterface.h"
#include "USBCANFD/zlgcan.h"

CanDeviceDialog::CanDeviceDialog(CanInterface *can, QWidget *parent)
    : QDialog(parent), m_can(can) {
  setupUi();

  connect(m_can, &CanInterface::connected, this,
          &CanDeviceDialog::onDeviceConnected);
  connect(m_can, &CanInterface::disconnected, this,
          &CanDeviceDialog::onDeviceDisconnected);
  connect(m_can, &CanInterface::errorOccurred, this,
          &CanDeviceDialog::onDeviceError);

  refreshState();
}

void CanDeviceDialog::setupUi() {
  setWindowTitle("设备管理 - ZLG USBCANFD");
  setMinimumWidth(420);

  QGroupBox *cfgGroup = new QGroupBox("设备配置");
  QGridLayout *grid = new QGridLayout(cfgGroup);

  m_deviceTypeCombo = new QComboBox();
  m_deviceTypeCombo->addItem("USBCANFD-200U", ZCAN_USBCANFD_200U);
  m_deviceTypeCombo->addItem("USBCANFD-100U", ZCAN_USBCANFD_100U);
  m_deviceTypeCombo->addItem("USBCANFD-MINI", ZCAN_USBCANFD_MINI);
  m_deviceTypeCombo->addItem("USBCANFD-800U", ZCAN_USBCANFD_800U);

  m_deviceIndexSpin = new QSpinBox();
  m_deviceIndexSpin->setRange(0, 7);

  m_channelCombo = new QComboBox();
  m_channelCombo->addItems({"通道 0", "通道 1"});

  m_baudCombo = new QComboBox();
  m_baudCombo->addItem("1M", 1000000);
  m_baudCombo->addItem("800K", 800000);
  m_baudCombo->addItem("500K", 500000);
  m_baudCombo->addItem("250K", 250000);
  m_baudCombo->addItem("125K", 125000);
  m_baudCombo->addItem("100K", 100000);
  m_baudCombo->addItem("50K", 50000);
  m_baudCombo->setCurrentText("500K");

  m_fdCheck = new QCheckBox("启用 CAN FD");
  m_dataBaudCombo = new QComboBox();
  m_dataBaudCombo->addItem("5M", 5000000);
  m_dataBaudCombo->addItem("4M", 4000000);
  m_dataBaudCombo->addItem("2M", 2000000);
  m_dataBaudCombo->addItem("1M", 1000000);
  m_dataBaudCombo->setCurrentText("2M");
  m_dataBaudCombo->setEnabled(false);
  connect(m_fdCheck, &QCheckBox::toggled, m_dataBaudCombo,
          &QWidget::setEnabled);

  m_modeCombo = new QComboBox();
  m_modeCombo->addItems({"正常模式", "静默监听"});

  m_termResCheck = new QCheckBox("内部终端电阻");
  m_termResCheck->setChecked(true);

  grid->addWidget(new QLabel("设备型号:"), 0, 0);
  grid->addWidget(m_deviceTypeCombo, 0, 1);
  grid->addWidget(new QLabel("设备索引:"), 0, 2);
  grid->addWidget(m_deviceIndexSpin, 0, 3);

  grid->addWidget(new QLabel("通道:"), 1, 0);
  grid->addWidget(m_channelCombo, 1, 1);
  grid->addWidget(new QLabel("工作模式:"), 1, 2);
  grid->addWidget(m_modeCombo, 1, 3);

  grid->addWidget(new QLabel("仲裁波特率:"), 2, 0);
  grid->addWidget(m_baudCombo, 2, 1);
  grid->addWidget(m_fdCheck, 2, 2);
  grid->addWidget(m_dataBaudCombo, 2, 3);

  grid->addWidget(m_termResCheck, 3, 0, 1, 2);

  m_diagButton = new QPushButton("诊断驱动/设备");
  m_openButton = new QPushButton("启动设备");
  connect(m_diagButton, &QPushButton::clicked, this,
          &CanDeviceDialog::onDiagnoseClicked);
  connect(m_openButton, &QPushButton::clicked, this,
          &CanDeviceDialog::onOpenCloseClicked);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addWidget(m_diagButton);
  btnLayout->addStretch();
  btnLayout->addWidget(m_openButton);

  m_statusLabel = new QLabel();
  m_statusLabel->setWordWrap(true);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(cfgGroup);
  mainLayout->addLayout(btnLayout);
  mainLayout->addWidget(m_statusLabel);
}

void CanDeviceDialog::onOpenCloseClicked() {
  if (m_can->isOpen()) {
    m_can->close();
    return;
  }

  if (!m_can->libraryLoaded()) {
    QMessageBox::warning(
        this, "提示",
        QStringLiteral("未能加载 zlgcan.dll：%1\n请将 ZLG 驱动的 zlgcan.dll 及 "
                       "kerneldlls 目录放到程序目录下。")
            .arg(m_can->libraryError()));
    return;
  }

  const quint32 deviceType =
      static_cast<quint32>(m_deviceTypeCombo->currentData().toUInt());
  const int abitBaud = m_baudCombo->currentData().toInt();
  const int dbitBaud = m_dataBaudCombo->currentData().toInt();
  const CanMode mode =
      m_modeCombo->currentIndex() == 1 ? CanMode::ListenOnly : CanMode::Normal;

  m_can->open(deviceType, m_deviceIndexSpin->value(),
              m_channelCombo->currentIndex(), abitBaud, m_fdCheck->isChecked(),
              dbitBaud, mode, m_termResCheck->isChecked());
}

void CanDeviceDialog::onDiagnoseClicked() {
  if (!m_can->libraryLoaded()) {
    m_statusLabel->setText(
        QStringLiteral("zlgcan.dll 未加载：%1").arg(m_can->libraryError()));
    m_statusLabel->setStyleSheet("color:#c0392b;");
    return;
  }
  QString driverVer, deviceName;
  bool online = false;
  if (m_can->diagnoseDriver(&driverVer, &deviceName, &online)) {
    m_statusLabel->setText(QStringLiteral("驱动版本: %1\n设备: %2 (%3)")
                               .arg(driverVer)
                               .arg(deviceName)
                               .arg(online ? "在线" : "离线"));
    m_statusLabel->setStyleSheet("color:#27ae60;");
  } else {
    m_statusLabel->setText(
        QStringLiteral("诊断失败：未检测到设备，请检查连接与驱动。"));
    m_statusLabel->setStyleSheet("color:#c0392b;");
  }
}

void CanDeviceDialog::onDeviceConnected() { refreshState(); }

void CanDeviceDialog::onDeviceDisconnected() { refreshState(); }

void CanDeviceDialog::onDeviceError(const QString &message) {
  m_statusLabel->setText(message);
  m_statusLabel->setStyleSheet("color:#c0392b;");
}

void CanDeviceDialog::refreshState() {
  const bool open = m_can->isOpen();
  m_openButton->setText(open ? "关闭设备" : "启动设备");

  m_deviceTypeCombo->setEnabled(!open);
  m_deviceIndexSpin->setEnabled(!open);
  m_channelCombo->setEnabled(!open);
  m_baudCombo->setEnabled(!open);
  m_fdCheck->setEnabled(!open);
  m_dataBaudCombo->setEnabled(!open && m_fdCheck->isChecked());
  m_modeCombo->setEnabled(!open);
  m_termResCheck->setEnabled(!open);

  if (open) {
    m_statusLabel->setText("设备已启动，通道已打开。");
    m_statusLabel->setStyleSheet("color:#27ae60;");
  } else {
    m_statusLabel->setText("设备未启动。");
    m_statusLabel->setStyleSheet("color:#7f8c8d;");
  }
}
