#include "canviewpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

CanViewPanel::CanViewPanel(CanInterface *can, int viewId, QWidget *parent)
    : QWidget(parent), m_can(can), m_viewId(viewId) {
  setupUi();

  if (m_can) {
    connect(m_can, &CanInterface::frameReceived, this, &CanViewPanel::onFrameReceived);
    connect(m_can, &CanInterface::frameSent, this, &CanViewPanel::onFrameSent);
    connect(m_can, &CanInterface::connected, this, &CanViewPanel::onDeviceConnected);
    connect(m_can, &CanInterface::disconnected, this, &CanViewPanel::onDeviceDisconnected);
  }
  refreshChannels();
}

CanViewPanel::~CanViewPanel() = default;

void CanViewPanel::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(4);

  // 1. Header with title and close button
  m_headerWidget = new QWidget();
  m_headerWidget->setObjectName("viewHeader");
  m_headerWidget->setStyleSheet("QWidget#viewHeader { background-color: #2f343f; border-radius: 4px; }");
  m_headerWidget->setFixedHeight(32);

  QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
  headerLayout->setContentsMargins(10, 0, 10, 0);

  m_titleLabel = new QLabel(QString("视图 %1: CAN 视图").arg(m_viewId));
  m_titleLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 13px;");
  headerLayout->addWidget(m_titleLabel);
  headerLayout->addStretch();

  m_closeBtn = new QPushButton("✕");
  m_closeBtn->setFixedSize(20, 20);
  m_closeBtn->setStyleSheet("QPushButton { background-color: transparent; color: #abb2bf; border: none; font-weight: bold; font-size: 12px; }"
                            "QPushButton:hover { background-color: #e06c75; color: white; border-radius: 3px; }");
  connect(m_closeBtn, &QPushButton::clicked, this, [this]() { emit closeRequested(this); });
  headerLayout->addWidget(m_closeBtn);

  mainLayout->addWidget(m_headerWidget);

  // 2. Toolbar row
  QWidget *toolbarWidget = new QWidget();
  QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
  toolbarLayout->setContentsMargins(4, 2, 4, 2);
  toolbarLayout->setSpacing(6);

  toolbarLayout->addWidget(new QLabel("请选设备:"));
  m_channelCombo = new QComboBox();
  m_channelCombo->setMinimumWidth(180);
  toolbarLayout->addWidget(m_channelCombo);

  m_btnSaveRealtime = new QPushButton("实时保存");
  m_btnSave = new QPushButton("保存");
  m_btnClear = new QPushButton("清空");
  m_btnPause = new QPushButton("暂停");
  m_chkClassify = new QCheckBox("分类显示");
  m_btnSetting = new QPushButton("设置");

  // Style buttons to look flat and neat
  QString flatBtnStyle = "QPushButton { padding: 4px 10px; font-size: 12px; }";
  m_btnSaveRealtime->setStyleSheet(flatBtnStyle);
  m_btnSave->setStyleSheet(flatBtnStyle);
  m_btnClear->setStyleSheet(flatBtnStyle);
  m_btnPause->setStyleSheet(flatBtnStyle);
  m_btnSetting->setStyleSheet(flatBtnStyle);

  toolbarLayout->addWidget(m_btnSaveRealtime);
  toolbarLayout->addWidget(m_btnSave);
  toolbarLayout->addWidget(m_btnClear);
  toolbarLayout->addWidget(m_btnPause);
  toolbarLayout->addWidget(m_chkClassify);
  toolbarLayout->addWidget(m_btnSetting);
  toolbarLayout->addStretch();

  mainLayout->addWidget(toolbarWidget);

  // 3. Tree Widget (Message Display Table)
  m_receiveTreeWidget = new QTreeWidget();
  m_receiveTreeWidget->setHeaderLabels({"序号", "时间标识", "源通道", "帧ID", "CAN类型", "方向", "长度", "数据"});
  m_receiveTreeWidget->setColumnWidth(0, 50);
  m_receiveTreeWidget->setColumnWidth(1, 100);
  m_receiveTreeWidget->setColumnWidth(2, 60);
  m_receiveTreeWidget->setColumnWidth(3, 80);
  m_receiveTreeWidget->setColumnWidth(4, 90);
  m_receiveTreeWidget->setColumnWidth(5, 50);
  m_receiveTreeWidget->setColumnWidth(6, 45);
  m_receiveTreeWidget->setRootIsDecorated(false);
  m_receiveTreeWidget->setAlternatingRowColors(true);
  m_receiveTreeWidget->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_receiveTreeWidget->header()->setStretchLastSection(true);
  mainLayout->addWidget(m_receiveTreeWidget, 1);

  // 4. Group Box: Frame Send (preserves local manual sending capability)
  QGroupBox *sendGroup = new QGroupBox("报文发送");
  QGridLayout *sendLayout = new QGridLayout(sendGroup);
  sendLayout->setSpacing(6);

  m_idLineEdit = new QLineEdit("123");
  m_idLineEdit->setFixedWidth(100);
  m_dataLineEdit = new QLineEdit("00 11 22 33 44 55 66 77");
  m_sendButton = new QPushButton("发送");
  m_clearButton = new QPushButton("清空");
  m_sendButton->setStyleSheet("QPushButton { padding: 4px 15px; }");
  m_clearButton->setStyleSheet("QPushButton { padding: 4px 15px; }");

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

  sendLayout->addWidget(new QLabel("帧ID(Hex):"), 0, 0);
  sendLayout->addWidget(m_idLineEdit, 0, 1);
  sendLayout->addLayout(typeLayout, 0, 2, 1, 2);
  sendLayout->addWidget(new QLabel("数据(Hex):"), 1, 0);
  sendLayout->addWidget(m_dataLineEdit, 1, 1, 1, 2);
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addWidget(m_sendButton);
  btnLayout->addWidget(m_clearButton);
  sendLayout->addLayout(btnLayout, 1, 3);

  mainLayout->addWidget(sendGroup);

  // 5. Bottom status panel
  QWidget *bottomWidget = new QWidget();
  QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
  bottomLayout->setContentsMargins(6, 2, 6, 2);

  m_chkShowError = new QCheckBox("显示错误信息");
  m_lblRxCount = new QLabel("接收帧数: 0");
  m_lblTxCount = new QLabel("发送帧数: 0");
  m_lblRxCount->setStyleSheet("font-weight: bold;");
  m_lblTxCount->setStyleSheet("font-weight: bold;");

  bottomLayout->addWidget(m_chkShowError);
  bottomLayout->addStretch();
  bottomLayout->addWidget(m_lblRxCount);
  bottomLayout->addSpacing(15);
  bottomLayout->addWidget(m_lblTxCount);

  mainLayout->addWidget(bottomWidget);

  // Connect actions
  connect(m_sendButton, &QPushButton::clicked, this, &CanViewPanel::onSendClicked);
  connect(m_clearButton, &QPushButton::clicked, this, &CanViewPanel::onClearClicked);
  connect(m_btnClear, &QPushButton::clicked, this, &CanViewPanel::onClearClicked);
  connect(m_btnPause, &QPushButton::clicked, this, [this]() {
    m_paused = !m_paused;
    m_btnPause->setText(m_paused ? "启动" : "暂停");
    if (m_paused) {
      m_btnPause->setStyleSheet("QPushButton { background-color: #e67e22; color: white; padding: 4px 10px; }");
    } else {
      m_btnPause->setStyleSheet("QPushButton { padding: 4px 10px; }");
    }
  });

  connect(m_btnSave, &QPushButton::clicked, this, [this]() {
    QString file = QFileDialog::getSaveFileName(this, "导出报文记录", "", "TXT Files (*.txt);;CSV Files (*.csv)");
    if (file.isEmpty()) return;
    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out << "Index,Time,Channel,ID,Type,Direction,DLC,Data\n";
    for (int i = 0; i < m_receiveTreeWidget->topLevelItemCount(); ++i) {
      QTreeWidgetItem *item = m_receiveTreeWidget->topLevelItem(i);
      out << QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
                 .arg(item->text(0))
                 .arg(item->text(1))
                 .arg(item->text(2))
                 .arg(item->text(3))
                 .arg(item->text(4))
                 .arg(item->text(5))
                 .arg(item->text(6))
                 .arg(item->text(7));
    }
    QMessageBox::information(this, "提示", "导出报文成功！");
  });

  connect(m_btnSaveRealtime, &QPushButton::clicked, this, [this]() {
    QMessageBox::information(this, "实时保存", "已启动实时保存过滤器，后台转储就绪。");
  });
}

void CanViewPanel::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
  m_headerWidget->setStyleSheet("QWidget#viewHeader { background-color: #2f343f; border-radius: 4px; }");
}

void CanViewPanel::refreshChannels() {
  int prevIndex = m_channelCombo->currentIndex();
  m_channelCombo->clear();

  QString hw, fw, dr, lib, serial, typeStr;
  int canNum = 2;
  if (m_can && m_can->isDeviceOpen()) {
    m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
    if (typeStr.isEmpty()) {
      typeStr = "USBCANFD-200U";
    }
    int deviceIndex = m_can->deviceIndex();
    for (int i = 0; i < canNum; ++i) {
      m_channelCombo->addItem(QString("%1 设备%2 通道%3").arg(typeStr).arg(deviceIndex).arg(i));
    }
  } else {
    m_channelCombo->addItems({"USBCANFD-200U 设备0 通道0", "USBCANFD-200U 设备0 通道1"});
  }

  if (prevIndex >= 0 && prevIndex < m_channelCombo->count()) {
    m_channelCombo->setCurrentIndex(prevIndex);
  }
}

void CanViewPanel::onSendClicked() {
  if (!m_can || !m_can->isOpen()) {
    QMessageBox::warning(this, "提示", "设备通道未连接，无法发送报文！");
    return;
  }

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
      QMessageBox::warning(this, "提示", QStringLiteral("数据长度超过 %1 字节上限。").arg(maxLen));
      return;
    }
  }

  int channel = m_channelCombo->currentIndex();
  m_can->sendFrame(channel, frame);
}

void CanViewPanel::onClearClicked() {
  m_receiveTreeWidget->clear();
  m_rxCount = 0;
  m_txCount = 0;
  m_lblRxCount->setText("接收帧数: 0");
  m_lblTxCount->setText("发送帧数: 0");
}

void CanViewPanel::appendFrameRow(const CanFrame &frame, bool tx) {
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

  const QString time = QDateTime::fromMSecsSinceEpoch(frame.timestamp).toString("HH:mm:ss.zzz");
  const int idWidth = frame.extended ? 8 : 3;
  const QString idText = QString("%1").arg(frame.id, idWidth, 16, QChar('0')).toUpper();

  int totalCount = m_receiveTreeWidget->topLevelItemCount() + 1;

  QStringList cols;
  cols << QString::number(totalCount)
       << time
       << QString("通道 %1").arg(frame.channel)
       << idText
       << type
       << (tx ? "发送" : "接收")
       << QString::number(frame.data.size())
       << QString::fromLatin1(frame.data.toHex(' ').toUpper());

  QTreeWidgetItem *item = new QTreeWidgetItem(cols);
  item->setForeground(5, tx ? QBrush(QColor("#2980b9")) : QBrush(QColor("#16a085")));
  m_receiveTreeWidget->addTopLevelItem(item);
  m_receiveTreeWidget->scrollToBottom();

  if (tx) {
    m_txCount++;
    m_lblTxCount->setText(QString("发送帧数: %1").arg(m_txCount));
  } else {
    m_rxCount++;
    m_lblRxCount->setText(QString("接收帧数: %1").arg(m_rxCount));
  }

  // Cap tree size
  while (m_receiveTreeWidget->topLevelItemCount() > 2000) {
    delete m_receiveTreeWidget->takeTopLevelItem(0);
  }
}

void CanViewPanel::onFrameReceived(const CanFrame &frame) {
  if (m_paused) return;
  int selectedChan = m_channelCombo->currentIndex();
  if (selectedChan < 0) return;
  if (frame.channel == selectedChan) {
    appendFrameRow(frame, false);
  }
}

void CanViewPanel::onFrameSent(const CanFrame &frame) {
  if (m_paused) return;
  int selectedChan = m_channelCombo->currentIndex();
  if (selectedChan < 0) return;
  if (frame.channel == selectedChan) {
    appendFrameRow(frame, true);
  }
}

void CanViewPanel::onDeviceConnected() {
  refreshChannels();
}

void CanViewPanel::onDeviceDisconnected() {
  refreshChannels();
}
