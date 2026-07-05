#include "cantool.h"

#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "candevicedialog.h"
#include "cantheme.h"
#include "USBCANFD/zlgcan.h"

CANTool::CANTool(QWidget *parent) : QDialog(parent) {
  m_can = new CanInterface(this);
  m_co = new CanOpenMaster(m_can, this);
  setupUi();

  // 后端信号
  connect(m_can, &CanInterface::connected, this, &CANTool::onCanConnected);
  connect(m_can, &CanInterface::disconnected, this,
          &CANTool::onCanDisconnected);
  connect(m_can, &CanInterface::errorOccurred, this, &CANTool::onCanError);
  connect(m_can, &CanInterface::frameReceived, this,
          &CANTool::onFrameReceived);
  connect(m_can, &CanInterface::frameSent, this, &CANTool::onFrameSent);

  // CANopen 主站接收到的帧需同时喂给协议解析
  connect(m_can, &CanInterface::frameReceived, m_co,
          &CanOpenMaster::processFrame);

  connect(m_co, &CanOpenMaster::heartbeatReceived, this,
          &CANTool::onHeartbeat);
  connect(m_co, &CanOpenMaster::emcyReceived, this, &CANTool::onEmcy);
  connect(m_co, &CanOpenMaster::pdoReceived, this, &CANTool::onPdo);
  connect(m_co, &CanOpenMaster::sdoReadFinished, this,
          &CANTool::onSdoReadFinished);
  connect(m_co, &CanOpenMaster::sdoWriteFinished, this,
          &CANTool::onSdoWriteFinished);
  connect(m_co, &CanOpenMaster::logMessage, this, &CANTool::onCanOpenLog);

  setControlsEnabled(false);

  if (!m_can->libraryLoaded()) {
    m_statusLabel->setText("未加载 zlgcan.dll");
    appendCanOpenLog(QStringLiteral("[警告] 未能加载 zlgcan 动态库：%1，请将 "
                                    "zlgcan.dll 及 kerneldlls 放到程序目录")
                         .arg(m_can->libraryError()));
  } else {
    // 驱动诊断
    QString driverVer, deviceName;
    bool online = false;
    if (m_can->diagnoseDriver(&driverVer, &deviceName, &online)) {
      m_statusLabel->setText(QStringLiteral("驱动就绪：%1, 设备: %2 (%3)")
                                 .arg(driverVer)
                                 .arg(deviceName)
                                 .arg(online ? "在线" : "离线"));
    } else {
      m_statusLabel->setText("驱动已加载，诊断失败");
    }
  }
}

CANTool::~CANTool() = default;

void CANTool::setupUi() {
  setWindowTitle("CAN / CAN FD / CANopen 测试工具");
  setMinimumSize(900, 660);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 12, 12, 12);
  mainLayout->setSpacing(10);
  mainLayout->addWidget(createConnectionPanel());

  QTabWidget *tabs = new QTabWidget();
  tabs->setDocumentMode(true);
  tabs->addTab(createMonitorPanel(), "报文收发 (CAN / CAN FD)");
  tabs->addTab(createCanOpenPanel(), "CANopen 测试");
  mainLayout->addWidget(tabs, 1);

  setAttribute(Qt::WA_DeleteOnClose);

  // 默认应用深色主题
  applyTheme(QStringLiteral("深色 (Dark)"));
}

QWidget *CANTool::createConnectionPanel() {
  QGroupBox *group = new QGroupBox("设备 (ZLG USBCANFD)");
  QHBoxLayout *layout = new QHBoxLayout(group);

  m_deviceButton = new QPushButton("设备管理...");
  connect(m_deviceButton, &QPushButton::clicked, this,
          &CANTool::onDeviceManage);

  m_statusLabel = new QLabel("未连接");
  m_statusLabel->setStyleSheet("color:#c0392b;");

  m_themeCombo = new QComboBox();
  m_themeCombo->addItems(CanTheme::names());
  m_themeCombo->setCurrentText(QStringLiteral("深色 (Dark)"));
  connect(m_themeCombo, &QComboBox::currentTextChanged, this,
          [this](const QString &name) { applyTheme(name); });

  layout->addWidget(m_deviceButton);
  layout->addSpacing(12);
  layout->addWidget(new QLabel("状态:"));
  layout->addWidget(m_statusLabel, 1);
  layout->addWidget(new QLabel("主题:"));
  layout->addWidget(m_themeCombo);

  return group;
}

void CANTool::applyTheme(const QString &name) {
  m_currentStyle = CanTheme::styleSheet(name);
  setStyleSheet(m_currentStyle);
  if (m_deviceDialog) {
    m_deviceDialog->setStyleSheet(m_currentStyle);
  }
}

void CANTool::onDeviceManage() {
  if (!m_deviceDialog) {
    m_deviceDialog = new CanDeviceDialog(m_can, this);
    if (!m_currentStyle.isEmpty()) {
      m_deviceDialog->setStyleSheet(m_currentStyle);
    }
  }
  m_deviceDialog->show();
  m_deviceDialog->raise();
  m_deviceDialog->activateWindow();
}

QWidget *CANTool::createMonitorPanel() {
  QWidget *panel = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(panel);

  m_receiveTreeWidget = new QTreeWidget();
  m_receiveTreeWidget->setHeaderLabels(
      {"时间", "方向", "帧ID", "帧类型", "长度", "数据"});
  m_receiveTreeWidget->setColumnWidth(0, 110);
  m_receiveTreeWidget->setColumnWidth(1, 50);
  m_receiveTreeWidget->setColumnWidth(2, 90);
  m_receiveTreeWidget->setColumnWidth(3, 130);
  m_receiveTreeWidget->setColumnWidth(4, 50);
  m_receiveTreeWidget->setRootIsDecorated(false);
  m_receiveTreeWidget->setAlternatingRowColors(true);
  m_receiveTreeWidget->setFont(
      QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_receiveTreeWidget->header()->setStretchLastSection(true);
  layout->addWidget(m_receiveTreeWidget, 1);

  QGroupBox *sendGroup = new QGroupBox("报文发送");
  QGridLayout *sendLayout = new QGridLayout(sendGroup);

  m_idLineEdit = new QLineEdit("123");
  m_dataLineEdit = new QLineEdit("00 11 22 33 44 55 66 77");
  m_sendButton = new QPushButton("发送");
  m_clearButton = new QPushButton("清空");

  m_stdFrameBtn = new QRadioButton("标准帧");
  m_extFrameBtn = new QRadioButton("扩展帧");
  m_stdFrameBtn->setChecked(true);
  m_remoteCheckBox = new QCheckBox("远程帧");
  m_fdFrameCheckBox = new QCheckBox("FD 帧");
  m_brsCheckBox = new QCheckBox("加速(BRS)");
  m_brsCheckBox->setEnabled(false);
  connect(m_fdFrameCheckBox, &QCheckBox::toggled, this, [this](bool on) {
    m_brsCheckBox->setEnabled(on);
    if (on)
      m_remoteCheckBox->setChecked(false);
    m_remoteCheckBox->setEnabled(!on);
  });

  QHBoxLayout *typeLayout = new QHBoxLayout();
  typeLayout->addWidget(m_stdFrameBtn);
  typeLayout->addWidget(m_extFrameBtn);
  typeLayout->addWidget(m_remoteCheckBox);
  typeLayout->addWidget(m_fdFrameCheckBox);
  typeLayout->addWidget(m_brsCheckBox);
  typeLayout->addStretch();

  connect(m_sendButton, &QPushButton::clicked, this, &CANTool::onSendClicked);
  connect(m_clearButton, &QPushButton::clicked, this, &CANTool::onClearReceive);

  sendLayout->addWidget(new QLabel("帧ID(Hex):"), 0, 0);
  sendLayout->addWidget(m_idLineEdit, 0, 1);
  sendLayout->addLayout(typeLayout, 0, 2, 1, 2);
  sendLayout->addWidget(new QLabel("数据(Hex):"), 1, 0);
  sendLayout->addWidget(m_dataLineEdit, 1, 1, 1, 2);
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addWidget(m_sendButton);
  btnLayout->addWidget(m_clearButton);
  sendLayout->addLayout(btnLayout, 1, 3);

  layout->addWidget(sendGroup);
  return panel;
}

QWidget *CANTool::createCanOpenPanel() {
  QWidget *panel = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(panel);

  // 节点 + NMT
  QGroupBox *nmtGroup = new QGroupBox("节点管理 (NMT)");
  QHBoxLayout *nmtLayout = new QHBoxLayout(nmtGroup);
  m_nodeIdSpin = new QSpinBox();
  m_nodeIdSpin->setRange(0, 127);
  m_nodeIdSpin->setValue(1);
  m_nodeIdSpin->setPrefix("节点 ");
  connect(m_nodeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int v) { m_co->setNodeId(static_cast<quint8>(v)); });

  m_nmtCommandCombo = new QComboBox();
  m_nmtCommandCombo->addItem("启动节点 (Start)", 0x01);
  m_nmtCommandCombo->addItem("停止节点 (Stop)", 0x02);
  m_nmtCommandCombo->addItem("进入预运行 (Pre-op)", 0x80);
  m_nmtCommandCombo->addItem("复位节点 (Reset Node)", 0x81);
  m_nmtCommandCombo->addItem("复位通信 (Reset Comm)", 0x82);
  m_nmtSendButton = new QPushButton("发送 NMT");
  m_syncButton = new QPushButton("发送 SYNC");
  connect(m_nmtSendButton, &QPushButton::clicked, this, &CANTool::onNmtSend);
  connect(m_syncButton, &QPushButton::clicked, this, &CANTool::onSyncSend);

  nmtLayout->addWidget(m_nodeIdSpin);
  nmtLayout->addWidget(m_nmtCommandCombo, 1);
  nmtLayout->addWidget(m_nmtSendButton);
  nmtLayout->addWidget(m_syncButton);

  // SDO
  QGroupBox *sdoGroup = new QGroupBox("SDO 服务数据对象 (快速传输)");
  QGridLayout *sdoLayout = new QGridLayout(sdoGroup);
  m_sdoIndexEdit = new QLineEdit("6040");
  m_sdoSubIndexEdit = new QLineEdit("00");
  m_sdoValueEdit = new QLineEdit("000F");
  m_sdoSizeCombo = new QComboBox();
  m_sdoSizeCombo->addItem("1 字节", 1);
  m_sdoSizeCombo->addItem("2 字节", 2);
  m_sdoSizeCombo->addItem("3 字节", 3);
  m_sdoSizeCombo->addItem("4 字节", 4);
  m_sdoSizeCombo->setCurrentIndex(1);
  m_sdoReadButton = new QPushButton("读取 (Upload)");
  m_sdoWriteButton = new QPushButton("写入 (Download)");
  connect(m_sdoReadButton, &QPushButton::clicked, this, &CANTool::onSdoRead);
  connect(m_sdoWriteButton, &QPushButton::clicked, this, &CANTool::onSdoWrite);

  sdoLayout->addWidget(new QLabel("索引(Hex):"), 0, 0);
  sdoLayout->addWidget(m_sdoIndexEdit, 0, 1);
  sdoLayout->addWidget(new QLabel("子索引(Hex):"), 0, 2);
  sdoLayout->addWidget(m_sdoSubIndexEdit, 0, 3);
  sdoLayout->addWidget(new QLabel("数值(Hex):"), 1, 0);
  sdoLayout->addWidget(m_sdoValueEdit, 1, 1);
  sdoLayout->addWidget(new QLabel("长度:"), 1, 2);
  sdoLayout->addWidget(m_sdoSizeCombo, 1, 3);
  sdoLayout->addWidget(m_sdoReadButton, 2, 0, 1, 2);
  sdoLayout->addWidget(m_sdoWriteButton, 2, 2, 1, 2);

  // 节点状态表
  QGroupBox *nodeGroup = new QGroupBox("节点状态 / 心跳监控");
  QVBoxLayout *nodeLayout = new QVBoxLayout(nodeGroup);
  m_nodeTreeWidget = new QTreeWidget();
  m_nodeTreeWidget->setHeaderLabels({"节点ID", "状态", "最近更新"});
  m_nodeTreeWidget->setColumnWidth(0, 80);
  m_nodeTreeWidget->setColumnWidth(1, 160);
  m_nodeTreeWidget->setRootIsDecorated(false);
  nodeLayout->addWidget(m_nodeTreeWidget);

  // 日志
  QGroupBox *logGroup = new QGroupBox("CANopen 日志");
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
  m_canOpenLog = new QPlainTextEdit();
  m_canOpenLog->setReadOnly(true);
  m_canOpenLog->setMaximumBlockCount(500);
  m_canOpenLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  logLayout->addWidget(m_canOpenLog);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addWidget(nodeGroup, 1);
  bottomLayout->addWidget(logGroup, 1);

  layout->addWidget(nmtGroup);
  layout->addWidget(sdoGroup);
  layout->addLayout(bottomLayout, 1);
  return panel;
}

void CANTool::onCanConnected() {
  m_statusLabel->setText("已连接");
  m_statusLabel->setStyleSheet("color:#27ae60;");
  setControlsEnabled(true);
}

void CANTool::onCanDisconnected() {
  m_statusLabel->setText("未连接");
  m_statusLabel->setStyleSheet("color:#c0392b;");
  setControlsEnabled(false);
}

void CANTool::onCanError(const QString &message) {
  appendCanOpenLog(QStringLiteral("[错误] %1").arg(message));
  m_statusLabel->setText(message);
  m_statusLabel->setStyleSheet("color:#c0392b;");
}

void CANTool::setControlsEnabled(bool connected) {
  m_sendButton->setEnabled(connected);
  m_nmtSendButton->setEnabled(connected);
  m_syncButton->setEnabled(connected);
  m_sdoReadButton->setEnabled(connected);
  m_sdoWriteButton->setEnabled(connected);
}

void CANTool::onSendClicked() {
  bool ok = false;
  const quint32 id = m_idLineEdit->text().trimmed().toUInt(&ok, 16);
  if (!ok) {
    QMessageBox::warning(this, "提示", "帧 ID 格式错误，请输入十六进制。");
    return;
  }

  CanFrame frame;
  frame.id = id;
  frame.extended = m_extFrameBtn->isChecked();
  frame.fd = m_fdFrameCheckBox->isChecked();
  frame.brs = frame.fd && m_brsCheckBox->isChecked();
  frame.remote = !frame.fd && m_remoteCheckBox->isChecked();

  if (!frame.remote) {
    QString hex = m_dataLineEdit->text();
    hex.remove(' ');
    frame.data = QByteArray::fromHex(hex.toLatin1());
    const int maxLen = frame.fd ? 64 : 8;
    if (frame.data.size() > maxLen) {
      QMessageBox::warning(this, "提示",
                           QStringLiteral("数据长度超过 %1 字节上限。").arg(maxLen));
      return;
    }
  }

  m_can->sendFrame(frame);
}

void CANTool::onClearReceive() { m_receiveTreeWidget->clear(); }

void CANTool::appendFrameRow(const CanFrame &frame, bool tx) {
  QString type;
  if (frame.fd) {
    type = frame.extended ? "FD扩展" : "FD标准";
    if (frame.brs)
      type += "+BRS";
  } else if (frame.remote) {
    type = frame.extended ? "扩展远程" : "标准远程";
  } else {
    type = frame.extended ? "扩展数据" : "标准数据";
  }

  const QString time =
      QDateTime::fromMSecsSinceEpoch(frame.timestamp).toString("HH:mm:ss.zzz");
  const int idWidth = frame.extended ? 8 : 3;
  const QString idText =
      QString("%1").arg(frame.id, idWidth, 16, QChar('0')).toUpper();

  QStringList cols;
  cols << time << (tx ? "发送" : "接收") << idText << type
       << QString::number(frame.data.size())
       << QString::fromLatin1(frame.data.toHex(' ').toUpper());

  QTreeWidgetItem *item = new QTreeWidgetItem(cols);
  item->setForeground(1, tx ? QBrush(QColor("#2980b9"))
                            : QBrush(QColor("#16a085")));
  m_receiveTreeWidget->addTopLevelItem(item);
  m_receiveTreeWidget->scrollToBottom();

  // 限制条目数量，避免内存无限增长
  while (m_receiveTreeWidget->topLevelItemCount() > 2000) {
    delete m_receiveTreeWidget->takeTopLevelItem(0);
  }
}

void CANTool::onFrameReceived(const CanFrame &frame) {
  appendFrameRow(frame, false);
}

void CANTool::onFrameSent(const CanFrame &frame) { appendFrameRow(frame, true); }

void CANTool::onNmtSend() {
  const NmtCommand cmd =
      static_cast<NmtCommand>(m_nmtCommandCombo->currentData().toInt());
  m_co->sendNmt(cmd, static_cast<quint8>(m_nodeIdSpin->value()));
}

void CANTool::onSyncSend() { m_co->sendSync(); }

void CANTool::onSdoRead() {
  bool ok1 = false, ok2 = false;
  const quint16 index =
      static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub =
      static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  if (!ok1 || !ok2) {
    QMessageBox::warning(this, "提示", "索引/子索引格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoRead(index, sub);
}

void CANTool::onSdoWrite() {
  bool ok1 = false, ok2 = false, ok3 = false;
  const quint16 index =
      static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub =
      static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  const quint32 value = m_sdoValueEdit->text().trimmed().toUInt(&ok3, 16);
  if (!ok1 || !ok2 || !ok3) {
    QMessageBox::warning(this, "提示", "索引/子索引/数值格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoWrite(index, sub, value, m_sdoSizeCombo->currentData().toInt());
}

void CANTool::onHeartbeat(quint8 nodeId, NmtState state) {
  const QString idText = QString::number(nodeId);
  QTreeWidgetItem *item = nullptr;
  for (int i = 0; i < m_nodeTreeWidget->topLevelItemCount(); ++i) {
    if (m_nodeTreeWidget->topLevelItem(i)->text(0) == idText) {
      item = m_nodeTreeWidget->topLevelItem(i);
      break;
    }
  }
  if (!item) {
    item = new QTreeWidgetItem(QStringList{idText});
    m_nodeTreeWidget->addTopLevelItem(item);
  }
  item->setText(1, CanOpenMaster::stateText(state));
  item->setText(2, QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void CANTool::onEmcy(quint8 nodeId, quint16 errorCode, quint8 errorRegister,
                     const QByteArray &manufacturer) {
  appendCanOpenLog(
      QStringLiteral("[EMCY] 节点 %1 错误码=0x%2 错误寄存器=0x%3 厂商=%4")
          .arg(nodeId)
          .arg(errorCode, 4, 16, QChar('0'))
          .arg(errorRegister, 2, 16, QChar('0'))
          .arg(QString::fromLatin1(manufacturer.toHex(' ').toUpper())));
}

void CANTool::onPdo(quint8 nodeId, int pdoNumber, bool isTpdo,
                    const QByteArray &data) {
  appendCanOpenLog(QStringLiteral("[%1PDO%2] 节点 %3 数据=%4")
                       .arg(isTpdo ? "T" : "R")
                       .arg(pdoNumber)
                       .arg(nodeId)
                       .arg(QString::fromLatin1(data.toHex(' ').toUpper())));
}

void CANTool::onSdoReadFinished(bool success, quint16 index, quint8 subIndex,
                                quint32 value, quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 读] [%1:%2] = 0x%3 (%4)")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(value, 0, 16)
                         .arg(value));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 读失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CANTool::onSdoWriteFinished(bool success, quint16 index, quint8 subIndex,
                                 quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 写] [%1:%2] 成功")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 写失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CANTool::onCanOpenLog(const QString &message) { appendCanOpenLog(message); }

void CANTool::appendCanOpenLog(const QString &text) {
  const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_canOpenLog->appendPlainText(QStringLiteral("%1  %2").arg(time, text));
}
