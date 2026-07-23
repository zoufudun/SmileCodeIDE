#include "normalsenddialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabBar>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QRegularExpression>
#include <QPainter>

// -------------------------------------------------------------------------
// RowSender Implementation
// -------------------------------------------------------------------------
RowSender::RowSender(CanInterface *can, int channel, int rowId, quint32 id, bool ext, bool remote, bool fd, bool brs,
                     const QByteArray &data, int framesPerSend, int sendCount, int intervalMs,
                     bool incId, bool incData, double speedMultiplier, int transmitType, QObject *parent)
    : QObject(parent), m_can(can), m_channel(channel), m_rowId(rowId), m_id(id), m_ext(ext), m_remote(remote),
      m_fd(fd), m_brs(brs), m_data(data), m_framesPerSend(framesPerSend), m_sendCount(sendCount),
      m_intervalMs(intervalMs), m_incId(incId), m_incData(incData), m_speedMultiplier(speedMultiplier),
      m_transmitType(transmitType) {
  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, &RowSender::onTimeout);
}

void RowSender::start() {
  m_currentSend = 0;
  int interval = m_intervalMs;
  if (m_speedMultiplier > 0.0) {
    interval = qRound(interval / m_speedMultiplier);
  }
  if (interval <= 0) interval = 1; // 最小间隔为 1ms

  sendOnce();
  m_currentSend++;

  if (m_currentSend < m_sendCount) {
    m_timer->start(interval);
  } else {
    emit finished(m_rowId, true);
  }
}

void RowSender::stop() {
  m_timer->stop();
}

void RowSender::onTimeout() {
  sendOnce();
  m_currentSend++;
  if (m_currentSend >= m_sendCount) {
    m_timer->stop();
    emit finished(m_rowId, true);
  }
}

void RowSender::sendOnce() {
  for (int f = 0; f < m_framesPerSend; ++f) {
    CanFrame frame;
    frame.id = m_id;
    frame.extended = m_ext;
    frame.remote = m_remote;
    frame.fd = m_fd;
    frame.brs = m_brs;
    frame.data = m_data;
    frame.transmitType = m_transmitType;

    bool ok = m_can->sendFrame(m_channel, frame);
    if (!ok) {
      emit statusChanged(m_rowId, "失败");
      emit finished(m_rowId, false);
      m_timer->stop();
      return;
    } else {
      emit statusChanged(m_rowId, "发送中");
    }

    if (m_incId) {
      m_id++;
    }
    if (m_incData && !m_data.isEmpty()) {
      quint8 last = static_cast<quint8>(m_data.at(m_data.size() - 1));
      last++;
      m_data[m_data.size() - 1] = static_cast<char>(last);
    }
  }
}

// -------------------------------------------------------------------------
// NormalSendPage Implementation
// -------------------------------------------------------------------------
NormalSendPage::NormalSendPage(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  setupUi();

  // 单帧即时发送定时器
  m_immTimer = new QTimer(this);
  connect(m_immTimer, &QTimer::timeout, this, &NormalSendPage::onImmediateSendTimerTick);

  // 界面刷新及总发送时间定时器
  m_uiRefreshTimer = new QTimer(this);
  connect(m_uiRefreshTimer, &QTimer::timeout, this, &NormalSendPage::onUIRefreshTimer);

  // 列表发送大循环定时器
  m_listIntervalTimer = new QTimer(this);
  connect(m_listIntervalTimer, &QTimer::timeout, this, &NormalSendPage::onListSendTick);

  if (m_can) {
    connect(m_can, &CanInterface::connected, this, &NormalSendPage::refreshChannels);
    connect(m_can, &CanInterface::disconnected, this, &NormalSendPage::refreshChannels);
  }
  refreshChannels();
}

NormalSendPage::~NormalSendPage() {
  cleanRowSenders();
}

void NormalSendPage::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 8, 8, 8);
  mainLayout->setSpacing(8);

  // 1. 帧发送 GroupBox
  QGroupBox *sendGroup = new QGroupBox("帧发送");
  QGridLayout *gridLayout = new QGridLayout(sendGroup);
  gridLayout->setSpacing(8);

  // Row 0: 通道, 协议, 加速
  gridLayout->addWidget(new QLabel("通道:"), 0, 0);
  m_channelCombo = new QComboBox();
  m_channelCombo->addItems({"通道 0", "通道 1"});
  if (m_can && m_can->isOpen()) {
    m_channelCombo->setCurrentIndex(m_can->channel());
  }
  gridLayout->addWidget(m_channelCombo, 0, 1);

  gridLayout->addWidget(new QLabel("协议:"), 0, 2);
  m_protocolCombo = new QComboBox();
  m_protocolCombo->addItems({"CAN FD", "CAN"});
  gridLayout->addWidget(m_protocolCombo, 0, 3);

  gridLayout->addWidget(new QLabel("加速:"), 0, 4);
  m_brsCheck = new QCheckBox("开启加速");
  m_brsCheck->setChecked(true);
  gridLayout->addWidget(m_brsCheck, 0, 5);

  // Row 1: 帧类型, 帧格式, 帧ID:0x
  gridLayout->addWidget(new QLabel("帧类型:"), 1, 0);
  m_frameTypeCombo = new QComboBox();
  m_frameTypeCombo->addItems({"标准帧", "扩展帧"});
  gridLayout->addWidget(m_frameTypeCombo, 1, 1);

  gridLayout->addWidget(new QLabel("帧格式:"), 1, 2);
  m_frameFormatCombo = new QComboBox();
  m_frameFormatCombo->addItems({"数据帧", "远程帧"});
  gridLayout->addWidget(m_frameFormatCombo, 1, 3);

  gridLayout->addWidget(new QLabel("帧ID:0x"), 1, 4);
  QHBoxLayout *idLayout = new QHBoxLayout();
  m_idEdit = new QLineEdit("100");
  idLayout->addWidget(m_idEdit, 1);
  QPushButton *idBtn = new QPushButton("...");
  idBtn->setFixedWidth(24);
  idLayout->addWidget(idBtn);
  gridLayout->addLayout(idLayout, 1, 5);

  // Row 2: 数据长度, 数据:0x
  gridLayout->addWidget(new QLabel("数据长度:"), 2, 0);
  m_lengthCombo = new QComboBox();
  gridLayout->addWidget(m_lengthCombo, 2, 1);

  gridLayout->addWidget(new QLabel("数据:0x"), 2, 2);
  QHBoxLayout *dataLayout = new QHBoxLayout();
  m_dataEdit = new QLineEdit("00 11 22 33 44 55 66 77");
  dataLayout->addWidget(m_dataEdit, 1);
  QPushButton *dataBtn = new QPushButton("...");
  dataBtn->setFixedWidth(24);
  dataLayout->addWidget(dataBtn);
  gridLayout->addLayout(dataLayout, 2, 3, 1, 3); // span 3 columns

  // Row 3: 每次发送帧数, 发送次数, 每次间隔(ms)
  gridLayout->addWidget(new QLabel("每次发送帧数:"), 3, 0);
  m_framesPerSendEdit = new QLineEdit("1");
  gridLayout->addWidget(m_framesPerSendEdit, 3, 1);

  gridLayout->addWidget(new QLabel("发送次数:"), 3, 2);
  m_sendCountEdit = new QLineEdit("1");
  gridLayout->addWidget(m_sendCountEdit, 3, 3);

  gridLayout->addWidget(new QLabel("每次间隔(ms):"), 3, 4);
  m_intervalEdit = new QLineEdit("0");
  gridLayout->addWidget(m_intervalEdit, 3, 5);

  // Row 4: 发送方式, 名称(可选), ID/数据递增
  gridLayout->addWidget(new QLabel("发送方式:"), 4, 0);
  m_sendTypeCombo = new QComboBox();
  m_sendTypeCombo->addItems({"正常发送", "单次发送", "自发自收", "单次自发只收"});
  gridLayout->addWidget(m_sendTypeCombo, 4, 1);

  gridLayout->addWidget(new QLabel("名称(可选):"), 4, 2);
  m_nameEdit = new QLineEdit();
  gridLayout->addWidget(m_nameEdit, 4, 3);

  QHBoxLayout *checkLayout = new QHBoxLayout();
  m_incIdCheck = new QCheckBox("ID递增");
  m_incDataCheck = new QCheckBox("数据递增");
  checkLayout->addWidget(m_incIdCheck);
  checkLayout->addWidget(m_incDataCheck);
  gridLayout->addLayout(checkLayout, 4, 4, 1, 2);

  // Row 5: 添加到列表, 立即发送, 发送时间
  QHBoxLayout *btnLayout = new QHBoxLayout();
  m_addToListButton = new QPushButton("添加到列表");
  m_immediateSendButton = new QPushButton("立即发送");
  m_immediateSendTimeLabel = new QLabel("发送时间(s): 0.000");
  m_immediateSendTimeLabel->setStyleSheet("font-weight: bold; color: #abb2bf; margin-left: 10px;");

  btnLayout->addWidget(m_addToListButton);
  btnLayout->addWidget(m_immediateSendButton);
  btnLayout->addWidget(m_immediateSendTimeLabel);
  btnLayout->addStretch();
  gridLayout->addLayout(btnLayout, 5, 0, 1, 6);

  // Protocol/Length setup
  auto updateLenFunc = [this]() {
    m_lengthCombo->clear();
    if (m_protocolCombo->currentText() == "CAN FD") {
      const QVector<int> fdLens = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
      for (int l : fdLens) {
        m_lengthCombo->addItem(QString::number(l), l);
      }
      m_lengthCombo->setCurrentText("64");
      m_brsCheck->setEnabled(true);
    } else {
      for (int i = 0; i <= 8; ++i) {
        m_lengthCombo->addItem(QString::number(i), i);
      }
      m_lengthCombo->setCurrentText("8");
      m_brsCheck->setChecked(false);
      m_brsCheck->setEnabled(false);
    }
  };
  connect(m_protocolCombo, &QComboBox::currentTextChanged, this, updateLenFunc);
  updateLenFunc();

  mainLayout->addWidget(sendGroup);

  // 2. 列表发送 GroupBox
  QGroupBox *listGroup = new QGroupBox("列表发送");
  QVBoxLayout *listLayout = new QVBoxLayout(listGroup);
  listLayout->setContentsMargins(6, 6, 6, 6);

  m_tableWidget = new QTableWidget();
  m_tableWidget->setColumnCount(12);
  m_tableWidget->setHorizontalHeaderLabels({
      "选择", "状态", "ID(0x)", "协议", "长度", "名称",
      "数据", "帧类型", "单次发送帧数", "发送次数", "单次间隔(ms)", "发送方式"
  });
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  m_tableWidget->horizontalHeader()->setStretchLastSection(true);
  m_tableWidget->setAlternatingRowColors(true);
  m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  listLayout->addWidget(m_tableWidget, 1);

  // 列表操作按钮行
  QHBoxLayout *opLayout = new QHBoxLayout();
  m_selectAllBtn = new QPushButton("全选");
  m_invertBtn = new QPushButton("反选");
  m_moveUpBtn = new QPushButton("上移");
  m_moveDownBtn = new QPushButton("下移");
  m_deleteBtn = new QPushButton("删除");
  m_clearBtn = new QPushButton("清空");
  m_importBtn = new QPushButton("导入");
  m_exportBtn = new QPushButton("导出");

  // 使用扁平紧凑风格
  QString btnStyle = "QPushButton { padding: 4px 10px; font-size: 12px; }";
  m_selectAllBtn->setStyleSheet(btnStyle);
  m_invertBtn->setStyleSheet(btnStyle);
  m_moveUpBtn->setStyleSheet(btnStyle);
  m_moveDownBtn->setStyleSheet(btnStyle);
  m_deleteBtn->setStyleSheet(btnStyle);
  m_clearBtn->setStyleSheet(btnStyle);
  m_importBtn->setStyleSheet(btnStyle);
  m_exportBtn->setStyleSheet(btnStyle);

  opLayout->addWidget(m_selectAllBtn);
  opLayout->addWidget(m_invertBtn);
  opLayout->addWidget(m_moveUpBtn);
  opLayout->addWidget(m_moveDownBtn);
  opLayout->addWidget(m_deleteBtn);
  opLayout->addWidget(m_clearBtn);
  opLayout->addWidget(m_importBtn);
  opLayout->addWidget(m_exportBtn);
  opLayout->addStretch();
  listLayout->addLayout(opLayout);

  // 列表发送控制面板行
  QHBoxLayout *ctrlLayout = new QHBoxLayout();
  m_sendModeCombo = new QComboBox();
  m_sendModeCombo->addItems({"并行发送模式", "串行发送模式"});
  m_sendModeCombo->setCurrentIndex(1); // 默认串行

  m_failBehaviorCombo = new QComboBox();
  m_failBehaviorCombo->addItems({"失败继续", "失败停止"});

  ctrlLayout->addWidget(m_sendModeCombo);
  ctrlLayout->addWidget(m_failBehaviorCombo);

  ctrlLayout->addWidget(new QLabel("列表发送次数:"));
  m_listSendCountEdit = new QLineEdit("1");
  m_listSendCountEdit->setFixedWidth(50);
  ctrlLayout->addWidget(m_listSendCountEdit);

  ctrlLayout->addWidget(new QLabel("发送间隔(ms):"));
  m_listIntervalEdit = new QLineEdit("0");
  m_listIntervalEdit->setFixedWidth(50);
  ctrlLayout->addWidget(m_listIntervalEdit);

  ctrlLayout->addWidget(new QLabel("发送速度:"));
  m_speedCombo = new QComboBox();
  m_speedCombo->addItems({"0.1", "0.2", "0.5", "1", "2", "5", "10", "20"});
  m_speedCombo->setCurrentText("1");
  ctrlLayout->addWidget(m_speedCombo);
  ctrlLayout->addWidget(new QLabel("倍"));

  m_listSendButton = new QPushButton("列表发送");
  m_listSendButton->setStyleSheet("QPushButton { background-color: #27ae60; color: white; padding: 6px 20px; font-weight: bold; } QPushButton:hover { background-color: #2ece76; }");
  ctrlLayout->addWidget(m_listSendButton);

  listLayout->addLayout(ctrlLayout);
  mainLayout->addWidget(listGroup, 1);

  // 绑定信号
  connect(m_immediateSendButton, &QPushButton::clicked, this, &NormalSendPage::onImmediateSend);
  connect(m_addToListButton, &QPushButton::clicked, this, &NormalSendPage::onAddToList);

  connect(m_selectAllBtn, &QPushButton::clicked, this, &NormalSendPage::onSelectAll);
  connect(m_invertBtn, &QPushButton::clicked, this, &NormalSendPage::onInvertSelection);
  connect(m_moveUpBtn, &QPushButton::clicked, this, &NormalSendPage::onMoveUp);
  connect(m_moveDownBtn, &QPushButton::clicked, this, &NormalSendPage::onMoveDown);
  connect(m_deleteBtn, &QPushButton::clicked, this, &NormalSendPage::onDeleteSelected);
  connect(m_clearBtn, &QPushButton::clicked, this, &NormalSendPage::onClearList);
  connect(m_importBtn, &QPushButton::clicked, this, &NormalSendPage::onImportList);
  connect(m_exportBtn, &QPushButton::clicked, this, &NormalSendPage::onExportList);

  connect(m_listSendButton, &QPushButton::clicked, this, [this]() {
    if (m_sending) {
      onListSendStop();
    } else {
      onListSendStart();
    }
  });

  // 当长度改变时，若是标准帧更新数据输入框默认占位符长度
  connect(m_lengthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    int len = m_lengthCombo->itemData(idx).toInt();
    QString dataStr = m_dataEdit->text().trimmed();
    QStringList bytes = dataStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    while (bytes.size() < len) {
      bytes.append("00");
    }
    while (bytes.size() > len) {
      bytes.removeLast();
    }
    m_dataEdit->setText(bytes.join(" "));
  });
}

void NormalSendPage::applyThemeStyle(const QString &qss) {
  setStyleSheet(qss);
}

void NormalSendPage::setControlsEnabled(bool enabled) {
  m_channelCombo->setEnabled(enabled);
  m_frameTypeCombo->setEnabled(enabled);
  m_frameFormatCombo->setEnabled(enabled);
  m_idEdit->setEnabled(enabled);
  m_lengthCombo->setEnabled(enabled);
  m_dataEdit->setEnabled(enabled);
  m_framesPerSendEdit->setEnabled(enabled);
  m_sendCountEdit->setEnabled(enabled);
  m_intervalEdit->setEnabled(enabled);
  m_incIdCheck->setEnabled(enabled);
  m_incDataCheck->setEnabled(enabled);
  m_sendTypeCombo->setEnabled(enabled);
  m_nameEdit->setEnabled(enabled);
  m_addToListButton->setEnabled(enabled);
  
  m_selectAllBtn->setEnabled(enabled);
  m_invertBtn->setEnabled(enabled);
  m_moveUpBtn->setEnabled(enabled);
  m_moveDownBtn->setEnabled(enabled);
  m_deleteBtn->setEnabled(enabled);
  m_clearBtn->setEnabled(enabled);
  m_importBtn->setEnabled(enabled);
  m_exportBtn->setEnabled(enabled);

  m_sendModeCombo->setEnabled(enabled);
  m_failBehaviorCombo->setEnabled(enabled);
  m_listSendCountEdit->setEnabled(enabled);
  m_listIntervalEdit->setEnabled(enabled);
  m_speedCombo->setEnabled(enabled);
}

// --------------------------- 即时发送 -----------------------------------
void NormalSendPage::onImmediateSend() {
  if (m_immTimer->isActive()) {
    // 正在以循环定时发送单帧，点击则停止
    m_immTimer->stop();
    m_uiRefreshTimer->stop();
    m_immediateSendButton->setText("立即发送");
    m_immediateSendButton->setStyleSheet("");
    setControlsEnabled(true);
    return;
  }

  if (!m_can || !m_can->isOpen()) {
    QMessageBox::warning(this, "提示", "CAN 设备通道未打开，请先连接设备！");
    return;
  }

  bool ok = false;
  m_immId = m_idEdit->text().trimmed().toUInt(&ok, 16);
  if (!ok) {
    QMessageBox::warning(this, "格式错误", "帧 ID 必须为十六进制数值！");
    return;
  }

  m_immFramesPerSend = m_framesPerSendEdit->text().trimmed().toInt(&ok);
  if (!ok || m_immFramesPerSend <= 0) {
    QMessageBox::warning(this, "格式错误", "每次发送帧数必须为正整数！");
    return;
  }

  m_immRemainingCount = m_sendCountEdit->text().trimmed().toInt(&ok);
  if (!ok || m_immRemainingCount <= 0) {
    QMessageBox::warning(this, "格式错误", "发送次数必须为正整数！");
    return;
  }

  m_immInterval = m_intervalEdit->text().trimmed().toInt(&ok);
  if (!ok || m_immInterval < 0) {
    QMessageBox::warning(this, "格式错误", "每次间隔必须为非负整数！");
    return;
  }

  QString hexData = m_dataEdit->text().trimmed();
  hexData.remove(' ');
  m_immData = QByteArray::fromHex(hexData.toLatin1());

  m_immIncId = m_incIdCheck->isChecked();
  m_immIncData = m_incDataCheck->isChecked();

  m_immStartTime = QDateTime::currentDateTime();

  // 如果发送次数等于 1，或间隔为 0，则同步直接发送完成
  if (m_immRemainingCount == 1 || m_immInterval == 0) {
    double totalSeconds = 0;
    QDateTime start = QDateTime::currentDateTime();

    while (m_immRemainingCount > 0) {
      int channel = m_channelCombo->currentIndex();
      for (int i = 0; i < m_immFramesPerSend; ++i) {
        CanFrame frame;
        frame.id = m_immId;
        frame.extended = (m_frameTypeCombo->currentIndex() == 1);
        frame.remote = (m_frameFormatCombo->currentIndex() == 1);
        frame.fd = (m_protocolCombo->currentText() == "CAN FD");
        frame.brs = (frame.fd && m_brsCheck->isChecked());
        frame.data = m_immData;
        frame.transmitType = m_sendTypeCombo->currentIndex();
        
        m_can->sendFrame(channel, frame);

        if (m_immIncId) m_immId++;
        if (m_immIncData && !m_immData.isEmpty()) {
          quint8 last = static_cast<quint8>(m_immData.at(m_immData.size() - 1));
          last++;
          m_immData[m_immData.size() - 1] = static_cast<char>(last);
        }
      }
      m_immRemainingCount--;
    }
    totalSeconds = start.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    m_immediateSendTimeLabel->setText(QString("发送时间(s): %1").arg(totalSeconds, 0, 'f', 3));
  } else {
    // 异步循环发送
    setControlsEnabled(false);
    m_immediateSendButton->setText("停止发送");
    m_immediateSendButton->setStyleSheet("QPushButton { background-color: #c0392b; color: white; }");
    m_uiRefreshTimer->start(50);
    
    // 执行第一次
    onImmediateSendTimerTick();
    m_immTimer->start(m_immInterval);
  }
}

void NormalSendPage::onImmediateSendTimerTick() {
  if (m_immRemainingCount <= 0) {
    m_immTimer->stop();
    m_uiRefreshTimer->stop();
    m_immediateSendButton->setText("立即发送");
    m_immediateSendButton->setStyleSheet("");
    setControlsEnabled(true);
    return;
  }

  int channel = m_channelCombo->currentIndex();
  for (int i = 0; i < m_immFramesPerSend; ++i) {
    CanFrame frame;
    frame.id = m_immId;
    frame.extended = (m_frameTypeCombo->currentIndex() == 1);
    frame.remote = (m_frameFormatCombo->currentIndex() == 1);
    frame.fd = (m_protocolCombo->currentText() == "CAN FD");
    frame.brs = (frame.fd && m_brsCheck->isChecked());
    frame.data = m_immData;
    frame.transmitType = m_sendTypeCombo->currentIndex();

    m_can->sendFrame(channel, frame);

    if (m_immIncId) m_immId++;
    if (m_immIncData && !m_immData.isEmpty()) {
      quint8 last = static_cast<quint8>(m_immData.at(m_immData.size() - 1));
      last++;
      m_immData[m_immData.size() - 1] = static_cast<char>(last);
    }
  }

  m_immRemainingCount--;
  if (m_immRemainingCount <= 0) {
    m_immTimer->stop();
    m_uiRefreshTimer->stop();
    m_immediateSendButton->setText("立即发送");
    m_immediateSendButton->setStyleSheet("");
    setControlsEnabled(true);
  }
}

// --------------------------- 列表管理 -----------------------------------
void NormalSendPage::onAddToList() {
  bool ok = false;
  QString idText = m_idEdit->text().trimmed();
  idText.toUInt(&ok, 16);
  if (!ok) {
    QMessageBox::warning(this, "错误", "帧 ID 必须是十六进制数值！");
    return;
  }

  int row = m_tableWidget->rowCount();
  m_tableWidget->insertRow(row);

  // 0: Checkbox + Index
  QTableWidgetItem *chkItem = new QTableWidgetItem(QString::number(row + 1));
  chkItem->setCheckState(Qt::Checked);
  m_tableWidget->setItem(row, 0, chkItem);

  // 1: Status
  m_tableWidget->setItem(row, 1, new QTableWidgetItem("未发送"));

  // 2: ID(0x)
  m_tableWidget->setItem(row, 2, new QTableWidgetItem(idText.toUpper()));

  // 3: Protocol
  QString protoStr = m_protocolCombo->currentText();
  if (protoStr == "CAN FD" && m_brsCheck->isChecked()) {
    protoStr += " (BRS)";
  }
  m_tableWidget->setItem(row, 3, new QTableWidgetItem(protoStr));

  // 4: Length
  m_tableWidget->setItem(row, 4, new QTableWidgetItem(m_lengthCombo->currentText()));

  // 5: Name
  m_tableWidget->setItem(row, 5, new QTableWidgetItem(m_nameEdit->text()));

  // 6: Data
  m_tableWidget->setItem(row, 6, new QTableWidgetItem(m_dataEdit->text()));

  // 7: FrameType
  QString typeText = m_frameTypeCombo->currentText() + "/" + m_frameFormatCombo->currentText();
  m_tableWidget->setItem(row, 7, new QTableWidgetItem(typeText));

  // 8: Single send count
  m_tableWidget->setItem(row, 8, new QTableWidgetItem(m_framesPerSendEdit->text()));

  // 9: Send cycles
  m_tableWidget->setItem(row, 9, new QTableWidgetItem(m_sendCountEdit->text()));

  // 10: Interval (ms)
  m_tableWidget->setItem(row, 10, new QTableWidgetItem(m_intervalEdit->text()));

  // 11: 发送方式
  m_tableWidget->setItem(row, 11, new QTableWidgetItem(m_sendTypeCombo->currentText()));
}

void NormalSendPage::onSelectAll() {
  for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
    if (m_tableWidget->item(i, 0)) {
      m_tableWidget->item(i, 0)->setCheckState(Qt::Checked);
    }
  }
}

void NormalSendPage::onInvertSelection() {
  for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
    QTableWidgetItem *item = m_tableWidget->item(i, 0);
    if (item) {
      item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
  }
}

void NormalSendPage::onMoveUp() {
  int curRow = m_tableWidget->currentRow();
  if (curRow <= 0) return;

  m_tableWidget->setUpdatesEnabled(false);
  for (int c = 0; c < m_tableWidget->columnCount(); ++c) {
    QTableWidgetItem *item1 = m_tableWidget->takeItem(curRow, c);
    QTableWidgetItem *item2 = m_tableWidget->takeItem(curRow - 1, c);
    
    // 如果包含复选框，需要特判文本
    if (c == 0) {
      // 保持序号文字递增
      QString t1 = item1->text();
      QString t2 = item2->text();
      item1->setText(t2);
      item2->setText(t1);
    }

    m_tableWidget->setItem(curRow - 1, c, item1);
    m_tableWidget->setItem(curRow, c, item2);
  }
  m_tableWidget->setCurrentCell(curRow - 1, m_tableWidget->currentColumn());
  m_tableWidget->setUpdatesEnabled(true);
}

void NormalSendPage::onMoveDown() {
  int curRow = m_tableWidget->currentRow();
  if (curRow < 0 || curRow >= m_tableWidget->rowCount() - 1) return;

  m_tableWidget->setUpdatesEnabled(false);
  for (int c = 0; c < m_tableWidget->columnCount(); ++c) {
    QTableWidgetItem *item1 = m_tableWidget->takeItem(curRow, c);
    QTableWidgetItem *item2 = m_tableWidget->takeItem(curRow + 1, c);

    if (c == 0) {
      QString t1 = item1->text();
      QString t2 = item2->text();
      item1->setText(t2);
      item2->setText(t1);
    }

    m_tableWidget->setItem(curRow + 1, c, item1);
    m_tableWidget->setItem(curRow, c, item2);
  }
  m_tableWidget->setCurrentCell(curRow + 1, m_tableWidget->currentColumn());
  m_tableWidget->setUpdatesEnabled(true);
}

void NormalSendPage::onDeleteSelected() {
  int curRow = m_tableWidget->currentRow();
  if (curRow >= 0) {
    m_tableWidget->removeRow(curRow);
    // 重排序号
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
      if (m_tableWidget->item(i, 0)) {
        m_tableWidget->item(i, 0)->setText(QString::number(i + 1));
      }
    }
  }
}

void NormalSendPage::onClearList() {
  m_tableWidget->setRowCount(0);
}

void NormalSendPage::onImportList() {
  QString file = QFileDialog::getOpenFileName(this, "导入发送列表", "", "JSON Files (*.json)");
  if (file.isEmpty()) return;

  QFile f(file);
  if (!f.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "错误", "无法打开文件！");
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull() || !doc.isArray()) {
    QMessageBox::warning(this, "格式错误", "无效的列表配置文件！");
    return;
  }

  m_tableWidget->setRowCount(0);
  QJsonArray arr = doc.array();
  for (int i = 0; i < arr.size(); ++i) {
    QJsonObject obj = arr.at(i).toObject();
    int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);

    QTableWidgetItem *chk = new QTableWidgetItem(QString::number(row + 1));
    chk->setCheckState(obj.value("selected").toBool() ? Qt::Checked : Qt::Unchecked);
    m_tableWidget->setItem(row, 0, chk);

    m_tableWidget->setItem(row, 1, new QTableWidgetItem("未发送"));
    m_tableWidget->setItem(row, 2, new QTableWidgetItem(obj.value("id").toString()));
    m_tableWidget->setItem(row, 3, new QTableWidgetItem(obj.value("protocol").toString()));
    m_tableWidget->setItem(row, 4, new QTableWidgetItem(obj.value("length").toString()));
    m_tableWidget->setItem(row, 5, new QTableWidgetItem(obj.value("name").toString()));
    m_tableWidget->setItem(row, 6, new QTableWidgetItem(obj.value("data").toString()));
    m_tableWidget->setItem(row, 7, new QTableWidgetItem(obj.value("frameType").toString()));
    m_tableWidget->setItem(row, 8, new QTableWidgetItem(obj.value("framesPerSend").toString()));
    m_tableWidget->setItem(row, 9, new QTableWidgetItem(obj.value("sendCount").toString()));
    m_tableWidget->setItem(row, 10, new QTableWidgetItem(obj.value("intervalMs").toString()));
    m_tableWidget->setItem(row, 11, new QTableWidgetItem(obj.value("transmitType").toString().isEmpty() ? "正常发送" : obj.value("transmitType").toString()));
  }
}

void NormalSendPage::onExportList() {
  QString file = QFileDialog::getSaveFileName(this, "导出发送列表", "", "JSON Files (*.json)");
  if (file.isEmpty()) return;

  QJsonArray arr;
  for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
    QJsonObject obj;
    obj["selected"] = m_tableWidget->item(i, 0) && (m_tableWidget->item(i, 0)->checkState() == Qt::Checked);
    obj["id"] = m_tableWidget->item(i, 2) ? m_tableWidget->item(i, 2)->text() : "";
    obj["protocol"] = m_tableWidget->item(i, 3) ? m_tableWidget->item(i, 3)->text() : "";
    obj["length"] = m_tableWidget->item(i, 4) ? m_tableWidget->item(i, 4)->text() : "";
    obj["name"] = m_tableWidget->item(i, 5) ? m_tableWidget->item(i, 5)->text() : "";
    obj["data"] = m_tableWidget->item(i, 6) ? m_tableWidget->item(i, 6)->text() : "";
    obj["frameType"] = m_tableWidget->item(i, 7) ? m_tableWidget->item(i, 7)->text() : "";
    obj["framesPerSend"] = m_tableWidget->item(i, 8) ? m_tableWidget->item(i, 8)->text() : "";
    obj["sendCount"] = m_tableWidget->item(i, 9) ? m_tableWidget->item(i, 9)->text() : "";
    obj["intervalMs"] = m_tableWidget->item(i, 10) ? m_tableWidget->item(i, 10)->text() : "";
    obj["transmitType"] = m_tableWidget->item(i, 11) ? m_tableWidget->item(i, 11)->text() : "正常发送";
    arr.append(obj);
  }

  QFile f(file);
  if (!f.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, "错误", "导出文件失败！");
    return;
  }

  f.write(QJsonDocument(arr).toJson());
}

// --------------------------- 列表发送 -----------------------------------
void NormalSendPage::onListSendStart() {
  if (!m_can || !m_can->isOpen()) {
    QMessageBox::warning(this, "提示", "CAN 通道未连接，无法执行列表发送！");
    return;
  }

  m_checkedRows.clear();
  for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
    QTableWidgetItem *chkItem = m_tableWidget->item(i, 0);
    if (chkItem && chkItem->checkState() == Qt::Checked) {
      m_checkedRows.append(i);
      m_tableWidget->setItem(i, 1, new QTableWidgetItem("等待"));
    } else {
      m_tableWidget->setItem(i, 1, new QTableWidgetItem("未发送"));
    }
  }

  if (m_checkedRows.isEmpty()) {
    QMessageBox::warning(this, "提示", "请先选择需要发送的列表行报文！");
    return;
  }

  bool ok = false;
  m_totalListCycles = m_listSendCountEdit->text().trimmed().toInt(&ok);
  if (!ok || m_totalListCycles <= 0) m_totalListCycles = 1;

  m_sending = true;
  m_currentListCycle = 0;
  m_listSendButton->setText("停止发送");
  m_listSendButton->setStyleSheet("QPushButton { background-color: #c0392b; color: white; padding: 6px 20px; font-weight: bold; }");
  setControlsEnabled(false);

  m_listSendStartTime = QDateTime::currentDateTime();
  m_uiRefreshTimer->start(50);

  onListSendTick();
}

void NormalSendPage::onListSendStop() {
  m_sending = false;
  m_listIntervalTimer->stop();
  m_uiRefreshTimer->stop();
  cleanRowSenders();

  m_listSendButton->setText("列表发送");
  m_listSendButton->setStyleSheet("QPushButton { background-color: #27ae60; color: white; padding: 6px 20px; font-weight: bold; }");
  setControlsEnabled(true);
}

void NormalSendPage::onListSendTick() {
  if (!m_sending) return;

  if (m_currentListCycle >= m_totalListCycles) {
    onListSendStop();
    return;
  }

  cleanRowSenders();

  bool parallel = (m_sendModeCombo->currentIndex() == 0);

  if (parallel) {
    startParallelSend();
  } else {
    m_serialIndex = 0;
    executeNextSerial();
  }
}

void NormalSendPage::executeNextSerial() {
  if (!m_sending) return;

  if (m_serialIndex >= m_checkedRows.size()) {
    // 当前周期结束，开启下个周期
    m_currentListCycle++;
    int repeatInterval = m_listIntervalEdit->text().trimmed().toInt();
    if (repeatInterval > 0) {
      m_listIntervalTimer->start(repeatInterval);
    } else {
      QTimer::singleShot(1, this, &NormalSendPage::onListSendTick);
    }
    return;
  }

  int row = m_checkedRows.at(m_serialIndex);

  // 解析此行的具体数据
  bool ok = false;
  quint32 id = m_tableWidget->item(row, 2)->text().toUInt(&ok, 16);
  QString rawData = m_tableWidget->item(row, 6)->text();
  rawData.remove(' ');
  QByteArray data = QByteArray::fromHex(rawData.toLatin1());

  QString frameType = m_tableWidget->item(row, 7)->text();
  bool ext = frameType.contains("扩展");
  bool remote = frameType.contains("远程");

  int framesPerSend = m_tableWidget->item(row, 8)->text().toInt();
  int sendCount = m_tableWidget->item(row, 9)->text().toInt();
  int interval = m_tableWidget->item(row, 10)->text().toInt();

  double multiplier = m_speedCombo->currentText().toDouble();

  m_tableWidget->setItem(row, 1, new QTableWidgetItem("发送中"));

  int channel = m_channelCombo->currentIndex();
  QString protoStr = m_tableWidget->item(row, 3)->text();
  bool fd = protoStr.startsWith("CAN FD");
  bool brs = protoStr.contains("BRS");

  QString sendTypeStr = m_tableWidget->item(row, 11) ? m_tableWidget->item(row, 11)->text() : "正常发送";
  int transmitType = 0;
  if (sendTypeStr == "单次发送") transmitType = 1;
  else if (sendTypeStr == "自发自收") transmitType = 2;
  else if (sendTypeStr == "单次自发只收" || sendTypeStr == "单次自发自收") transmitType = 3;

  RowSender *sender = new RowSender(m_can, channel, row, id, ext, remote, fd, brs,
                                    data, framesPerSend, sendCount, interval,
                                    m_incIdCheck->isChecked(), m_incDataCheck->isChecked(),
                                    multiplier, transmitType, this);
  m_activeSenders.append(sender);

  connect(sender, &RowSender::statusChanged, this, &NormalSendPage::onRowSenderStatus);
  connect(sender, &RowSender::finished, this, &NormalSendPage::onRowSenderFinished);

  sender->start();
}

void NormalSendPage::startParallelSend() {
  m_activeSendersCount = m_checkedRows.size();

  for (int i = 0; i < m_checkedRows.size(); ++i) {
    int row = m_checkedRows.at(i);

    bool ok = false;
    quint32 id = m_tableWidget->item(row, 2)->text().toUInt(&ok, 16);
    QByteArray data = QByteArray::fromHex(m_tableWidget->item(row, 6)->text().remove(' ').toLatin1());

    QString frameType = m_tableWidget->item(row, 7)->text();
    bool ext = frameType.contains("扩展");
    bool remote = frameType.contains("远程");

    int framesPerSend = m_tableWidget->item(row, 8)->text().toInt();
    int sendCount = m_tableWidget->item(row, 9)->text().toInt();
    int interval = m_tableWidget->item(row, 10)->text().toInt();

    double multiplier = m_speedCombo->currentText().toDouble();

    m_tableWidget->setItem(row, 1, new QTableWidgetItem("发送中"));

    int channel = m_channelCombo->currentIndex();
    QString protoStr = m_tableWidget->item(row, 3)->text();
    bool fd = protoStr.startsWith("CAN FD");
    bool brs = protoStr.contains("BRS");

    QString sendTypeStr = m_tableWidget->item(row, 11) ? m_tableWidget->item(row, 11)->text() : "正常发送";
    int transmitType = 0;
    if (sendTypeStr == "单次发送") transmitType = 1;
    else if (sendTypeStr == "自发自收") transmitType = 2;
    else if (sendTypeStr == "单次自发只收" || sendTypeStr == "单次自发自收") transmitType = 3;

    RowSender *sender = new RowSender(m_can, channel, row, id, ext, remote, fd, brs,
                                      data, framesPerSend, sendCount, interval,
                                      m_incIdCheck->isChecked(), m_incDataCheck->isChecked(),
                                      multiplier, transmitType, this);
    m_activeSenders.append(sender);

    connect(sender, &RowSender::statusChanged, this, &NormalSendPage::onRowSenderStatus);
    connect(sender, &RowSender::finished, this, &NormalSendPage::onRowSenderFinished);

    sender->start();
  }
}

void NormalSendPage::onRowSenderStatus(int rowId, const QString &statusText) {
  m_tableWidget->setItem(rowId, 1, new QTableWidgetItem(statusText));
}

void NormalSendPage::onRowSenderFinished(int rowId, bool success) {
  RowSender *rowSender = qobject_cast<RowSender*>(QObject::sender());
  if (rowSender) {
    rowSender->stop();
    m_activeSenders.removeOne(rowSender);
    rowSender->deleteLater();
  }

  m_tableWidget->setItem(rowId, 1, new QTableWidgetItem(success ? "成功" : "失败"));

  bool stopOnFail = (m_failBehaviorCombo->currentIndex() == 1);
  if (!success && stopOnFail) {
    onListSendStop();
    QMessageBox::warning(this, "终止发送", QString("第 %1 行报文发送失败，已终止列表发送。").arg(rowId + 1));
    return;
  }

  bool parallel = (m_sendModeCombo->currentIndex() == 0);
  if (parallel) {
    m_activeSendersCount--;
    if (m_activeSendersCount <= 0) {
      // 当前周期结束，开启下个周期
      m_currentListCycle++;
      int repeatInterval = m_listIntervalEdit->text().trimmed().toInt();
      if (repeatInterval > 0) {
        m_listIntervalTimer->start(repeatInterval);
      } else {
        QTimer::singleShot(1, this, &NormalSendPage::onListSendTick);
      }
    }
  } else {
    // 串行执行下一行
    m_serialIndex++;
    executeNextSerial();
  }
}

void NormalSendPage::onUIRefreshTimer() {
  if (m_sending) {
    double secs = m_listSendStartTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    m_immediateSendTimeLabel->setText(QString("发送时间(s): %1").arg(secs, 0, 'f', 3));
  } else if (m_immTimer->isActive()) {
    double secs = m_immStartTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    m_immediateSendTimeLabel->setText(QString("发送时间(s): %1").arg(secs, 0, 'f', 3));
  }
}

void NormalSendPage::cleanRowSenders() {
  for (auto s : m_activeSenders) {
    if (s) {
      s->stop();
      s->deleteLater();
    }
  }
  m_activeSenders.clear();
}

// -------------------------------------------------------------------------
// NormalSendDialog Implementation
// -------------------------------------------------------------------------
NormalSendDialog::NormalSendDialog(CanInterface *can, QWidget *parent)
    : QDialog(parent), m_can(can) {
  setWindowTitle("普通发送");
  resize(960, 680);
  setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint);
  setAttribute(Qt::WA_DeleteOnClose);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);

  m_tabWidget = new QTabWidget(this);
  m_tabWidget->setTabsClosable(true);
  m_tabWidget->setDocumentMode(true);
  layout->addWidget(m_tabWidget);

  // 初始化添加 '+' Tab
  m_tabWidget->addTab(new QWidget(), "+");

  connect(m_tabWidget, &QTabWidget::currentChanged, this, &NormalSendDialog::onTabChanged);
  connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &NormalSendDialog::onCloseTab);

  // 自动新建第一个 Tab
  createNewTab();
}

NormalSendDialog::~NormalSendDialog() = default;

void NormalSendDialog::applyThemeStyle(const QString &qss) {
  m_currentQss = qss;
  setStyleSheet(qss);
  for (int i = 0; i < m_tabWidget->count() - 1; ++i) {
    NormalSendPage *page = qobject_cast<NormalSendPage*>(m_tabWidget->widget(i));
    if (page) {
      page->applyThemeStyle(qss);
    }
  }
}

void NormalSendDialog::onTabChanged(int index) {
  // 如果选中了最后一个 Tab (即 '+')，添加新页面
  if (index == m_tabWidget->count() - 1) {
    createNewTab();
  }
}

void NormalSendDialog::onCloseTab(int index) {
  // 禁止关闭 '+' 按钮 Tab
  if (index == m_tabWidget->count() - 1) return;

  // 保证至少保留一个工作 Tab
  if (m_tabWidget->count() <= 2) {
    QMessageBox::information(this, "提示", "至少保留一个发送页面工作区！");
    return;
  }

  QWidget *w = m_tabWidget->widget(index);
  m_tabWidget->removeTab(index);
  delete w;
}

void NormalSendDialog::createNewTab() {
  m_tabWidget->blockSignals(true);
  NormalSendPage *page = new NormalSendPage(m_can, this);
  if (!m_currentQss.isEmpty()) {
    page->applyThemeStyle(m_currentQss);
  }
  
  int newIdx = m_tabWidget->count() - 1;
  m_tabWidget->insertTab(newIdx, page, QString("通道名称 %1").arg(m_tabCounter++));
  m_tabWidget->setCurrentIndex(newIdx);
  m_tabWidget->blockSignals(false);
}

void NormalSendPage::refreshChannels() {
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
  } else if (m_can && m_can->isOpen()) {
    m_channelCombo->setCurrentIndex(m_can->channel());
  }
}
