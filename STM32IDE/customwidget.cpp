#include "customwidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QApplication>
#include <QDrag>

// ============ CustomProtocolButton 实现 ============

CustomProtocolButton::CustomProtocolButton(QWidget *parent)
    : QPushButton(parent) {
  setText("自定义协议");
  setMinimumSize(100, 60);
  setKeyboardStyle(0);

  connect(this, &QPushButton::clicked, this,
          &CustomProtocolButton::onButtonClicked);
}

void CustomProtocolButton::setKeyboardStyle(int styleIndex) {
  // 精美的键盘类按钮样式
  QString styles[] = {
      // 样式0: 蓝色渐变
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #667eea, stop:1 #764ba2);"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "  padding: 10px;"
      "  box-shadow: 0 4px 6px rgba(0,0,0,0.1);"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #7b8ff0, stop:1 #8a5cb8);"
      "  transform: translateY(-2px);"
      "}"
      "QPushButton:pressed {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #5a6fd8, stop:1 #6a4390);"
      "  transform: translateY(1px);"
      "}",

      // 样式1: 绿色渐变
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #11998e, stop:1 #38ef7d);"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "  padding: 10px;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #1aaa9e, stop:1 #48ff8d);"
      "}"
      "QPushButton:pressed {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #0e887e, stop:1 #28df6d);"
      "}",

      // 样式2: 橙色渐变
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #f093fb, stop:1 #f5576c);"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "  padding: 10px;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #ffa3fb, stop:1 #ff677c);"
      "}"
      "QPushButton:pressed {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
      "    stop:0 #e083eb, stop:1 #e5475c);"
      "}"};

  if (styleIndex >= 0 && styleIndex < 3) {
    setStyleSheet(styles[styleIndex]);
  }
}

void CustomProtocolButton::startAutoSend() {
  if (!m_autoSendTimer) {
    m_autoSendTimer = new QTimer(this);
    connect(m_autoSendTimer, &QTimer::timeout, this,
            &CustomProtocolButton::onAutoSendTimeout);
  }

  m_autoSendTimer->start(m_config.loopInterval);

  // 更新按钮样式显示正在循环发送
  setStyleSheet(styleSheet() + "\nQPushButton { border: 3px solid #FF5722; }");
}

void CustomProtocolButton::stopAutoSend() {
  if (m_autoSendTimer) {
    m_autoSendTimer->stop();
  }

  // 恢复原始样式
  setKeyboardStyle(0);
}

void CustomProtocolButton::onAutoSendTimeout() {
  QByteArray frame = buildFrame();
  emit sendData(frame);
}

void CustomProtocolButton::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragStartPos = event->pos();
  }
  QPushButton::mousePressEvent(event);
}

void CustomProtocolButton::mouseMoveEvent(QMouseEvent *event) {
  if (!(event->buttons() & Qt::LeftButton)) {
    return;
  }

  if ((event->pos() - m_dragStartPos).manhattanLength() <
      QApplication::startDragDistance()) {
    return;
  }

  // 在父控件内拖动
  QPoint newPos = parentWidget()->mapFromGlobal(event->globalPos()) -
                  QPoint(width() / 2, height() / 2);
  move(newPos);
}

void CustomProtocolButton::setProtocolConfig(const ProtocolConfig &config) {
  m_config = config;
  setText(config.name);
}

void CustomProtocolButton::onButtonClicked() {
  QByteArray frame = buildFrame();
  emit sendData(frame);
}

void CustomProtocolButton::showConfigDialog() {
  ProtocolConfigDialog dialog(this);
  dialog.setConfig(m_config);
  if (dialog.exec() == QDialog::Accepted) {
    m_config = dialog.getConfig();
    setText(m_config.name);
  }
}

void CustomProtocolButton::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  QAction *configAction = menu.addAction("配置协议");

  QAction *autoSendAction = nullptr;
  if (isAutoSending()) {
    autoSendAction = menu.addAction("停止循环发送");
  } else {
    autoSendAction = menu.addAction("开始循环发送");
  }

  menu.addSeparator();
  QAction *styleAction = menu.addAction("更换样式");
  QAction *deleteAction = menu.addAction("删除控件");

  QAction *selected = menu.exec(event->globalPos());
  if (selected == configAction) {
    showConfigDialog();
  } else if (selected == autoSendAction) {
    if (isAutoSending()) {
      stopAutoSend();
    } else {
      if (m_config.autoSendLoop) {
        startAutoSend();
      } else {
        QMessageBox::information(this, "提示",
          "请先在协议配置中启用循环发送功能");
      }
    }
  } else if (selected == styleAction) {
    // 循环切换样式
    static int currentStyle = 0;
    currentStyle = (currentStyle + 1) % 3;
    setKeyboardStyle(currentStyle);
  } else if (selected == deleteAction) {
    deleteLater();
  }
}

QByteArray CustomProtocolButton::buildFrame() {
  QByteArray frame;

  // 1. 添加帧头
  if (m_config.useFrameHeader) {
    frame.append(m_config.frameHeader);
  }

  // 2. 预留长度字段位置
  int lengthFieldPos = -1;
  if (m_config.useLength) {
    lengthFieldPos = frame.size();
    for (int i = 0; i < m_config.lengthSize; ++i) {
      frame.append(char(0));
    }
  }

  // 3. 添加帧序号
  if (m_config.useSequence) {
    if (m_config.sequenceSize == 1) {
      frame.append(char(m_sequenceNumber & 0xFF));
    } else if (m_config.sequenceSize == 2) {
      frame.append(char((m_sequenceNumber >> 8) & 0xFF));
      frame.append(char(m_sequenceNumber & 0xFF));
    }
    m_sequenceNumber++;
  }

  // 4. 添加功能码
  if (m_config.useFunctionCode) {
    frame.append(char(m_config.functionCode));
  }

  // 5. 添加数据区
  frame.append(m_config.dataPayload);

  // 6. 填充长度字段
  if (m_config.useLength && lengthFieldPos >= 0) {
    int length = m_config.lengthIncludesHeader
                     ? frame.size()
                     : (frame.size() - lengthFieldPos - m_config.lengthSize);

    if (m_config.lengthSize == 1) {
      frame[lengthFieldPos] = char(length & 0xFF);
    } else if (m_config.lengthSize == 2) {
      frame[lengthFieldPos] = char((length >> 8) & 0xFF);
      frame[lengthFieldPos + 1] = char(length & 0xFF);
    } else if (m_config.lengthSize == 4) {
      frame[lengthFieldPos] = char((length >> 24) & 0xFF);
      frame[lengthFieldPos + 1] = char((length >> 16) & 0xFF);
      frame[lengthFieldPos + 2] = char((length >> 8) & 0xFF);
      frame[lengthFieldPos + 3] = char(length & 0xFF);
    }
  }

  // 7. 计算并添加校验码
  if (m_config.checksumType != ChecksumType::None) {
    QByteArray checksumData = frame;
    if (m_config.useFrameHeader) {
      checksumData = frame.mid(m_config.frameHeader.size());
    }

    switch (m_config.checksumType) {
    case ChecksumType::XOR: {
      quint8 checksum = calculateXOR(checksumData);
      frame.append(char(checksum));
      break;
    }
    case ChecksumType::CRC8: {
      quint8 checksum = calculateCRC8(checksumData);
      frame.append(char(checksum));
      break;
    }
    case ChecksumType::CRC16: {
      quint16 checksum = calculateCRC16(checksumData);
      frame.append(char((checksum >> 8) & 0xFF));
      frame.append(char(checksum & 0xFF));
      break;
    }
    case ChecksumType::CRC32: {
      quint32 checksum = calculateCRC32(checksumData);
      frame.append(char((checksum >> 24) & 0xFF));
      frame.append(char((checksum >> 16) & 0xFF));
      frame.append(char((checksum >> 8) & 0xFF));
      frame.append(char(checksum & 0xFF));
      break;
    }
    case ChecksumType::Sum8: {
      quint8 checksum = calculateSum8(checksumData);
      frame.append(char(checksum));
      break;
    }
    case ChecksumType::Sum16: {
      quint16 checksum = calculateSum16(checksumData);
      frame.append(char((checksum >> 8) & 0xFF));
      frame.append(char(checksum & 0xFF));
      break;
    }
    default:
      break;
    }
  }

  // 8. 添加帧尾
  if (m_config.useFrameTail) {
    frame.append(m_config.frameTail);
  }

  return frame;
}

// 校验算法实现
quint8 CustomProtocolButton::calculateXOR(const QByteArray &data) {
  quint8 result = 0;
  for (char byte : data) {
    result ^= static_cast<quint8>(byte);
  }
  return result;
}

quint8 CustomProtocolButton::calculateCRC8(const QByteArray &data) {
  quint8 crc = 0;
  for (char byte : data) {
    crc ^= static_cast<quint8>(byte);
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

quint16 CustomProtocolButton::calculateCRC16(const QByteArray &data) {
  quint16 crc = 0xFFFF;
  for (char byte : data) {
    crc ^= static_cast<quint8>(byte);
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

quint32 CustomProtocolButton::calculateCRC32(const QByteArray &data) {
  quint32 crc = 0xFFFFFFFF;
  for (char byte : data) {
    crc ^= static_cast<quint8>(byte);
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x00000001) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }
  return ~crc;
}

quint8 CustomProtocolButton::calculateSum8(const QByteArray &data) {
  quint8 sum = 0;
  for (char byte : data) {
    sum += static_cast<quint8>(byte);
  }
  return sum;
}

quint16 CustomProtocolButton::calculateSum16(const QByteArray &data) {
  quint16 sum = 0;
  for (char byte : data) {
    sum += static_cast<quint8>(byte);
  }
  return sum;
}

// ============ ProtocolConfigDialog 实现 ============

ProtocolConfigDialog::ProtocolConfigDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("协议配置");
  setMinimumWidth(500);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // 基本信息
  QGroupBox *basicGroup = new QGroupBox("基本信息");
  QFormLayout *basicLayout = new QFormLayout(basicGroup);

  m_editName = new QLineEdit();
  basicLayout->addRow("协议名称:", m_editName);

  mainLayout->addWidget(basicGroup);

  // 帧头配置
  QGroupBox *headerGroup = new QGroupBox("帧头配置");
  QVBoxLayout *headerLayout = new QVBoxLayout(headerGroup);

  m_chkUseHeader = new QCheckBox("使用帧头");
  headerLayout->addWidget(m_chkUseHeader);

  QHBoxLayout *headerEditLayout = new QHBoxLayout();
  headerEditLayout->addWidget(new QLabel("帧头 (HEX):"));
  m_editFrameHeader = new QLineEdit();
  m_editFrameHeader->setPlaceholderText("例如: AA 55");
  headerEditLayout->addWidget(m_editFrameHeader);
  headerLayout->addLayout(headerEditLayout);

  mainLayout->addWidget(headerGroup);

  // 数据字段配置
  QGroupBox *dataGroup = new QGroupBox("数据字段配置");
  QFormLayout *dataLayout = new QFormLayout(dataGroup);

  // 长度字段
  m_chkUseLength = new QCheckBox("使用长度字段");
  dataLayout->addRow(m_chkUseLength);

  m_spinLengthPos = new QSpinBox();
  m_spinLengthPos->setRange(0, 255);
  dataLayout->addRow("长度字段位置:", m_spinLengthPos);

  m_spinLengthSize = new QSpinBox();
  m_spinLengthSize->setRange(1, 4);
  m_spinLengthSize->setValue(1);
  dataLayout->addRow("长度字段字节数:", m_spinLengthSize);

  m_chkLengthIncludesHeader = new QCheckBox("长度包含帧头");
  dataLayout->addRow(m_chkLengthIncludesHeader);

  // 帧序号
  m_chkUseSequence = new QCheckBox("使用帧序号");
  dataLayout->addRow(m_chkUseSequence);

  m_spinSequencePos = new QSpinBox();
  m_spinSequencePos->setRange(0, 255);
  dataLayout->addRow("序号字段位置:", m_spinSequencePos);

  // 功能码
  m_chkUseFunctionCode = new QCheckBox("使用功能码");
  dataLayout->addRow(m_chkUseFunctionCode);

  m_spinFunctionCodePos = new QSpinBox();
  m_spinFunctionCodePos->setRange(0, 255);
  dataLayout->addRow("功能码位置:", m_spinFunctionCodePos);

  m_editFunctionCode = new QLineEdit();
  m_editFunctionCode->setPlaceholderText("例如: 01");
  dataLayout->addRow("功能码 (HEX):", m_editFunctionCode);

  mainLayout->addWidget(dataGroup);

  // 数据区
  QGroupBox *payloadGroup = new QGroupBox("数据区");
  QVBoxLayout *payloadLayout = new QVBoxLayout(payloadGroup);

  m_editDataPayload = new QTextEdit();
  m_editDataPayload->setPlaceholderText("输入数据 (HEX格式，例如: 01 02 03 04)");
  m_editDataPayload->setMaximumHeight(80);
  payloadLayout->addWidget(m_editDataPayload);

  mainLayout->addWidget(payloadGroup);

  // 校验配置
  QGroupBox *checksumGroup = new QGroupBox("校验配置");
  QFormLayout *checksumLayout = new QFormLayout(checksumGroup);

  m_comboChecksumType = new QComboBox();
  m_comboChecksumType->addItem("无校验", int(ChecksumType::None));
  m_comboChecksumType->addItem("异或校验 (XOR)", int(ChecksumType::XOR));
  m_comboChecksumType->addItem("CRC-8", int(ChecksumType::CRC8));
  m_comboChecksumType->addItem("CRC-16", int(ChecksumType::CRC16));
  m_comboChecksumType->addItem("CRC-32", int(ChecksumType::CRC32));
  m_comboChecksumType->addItem("8位累加和", int(ChecksumType::Sum8));
  m_comboChecksumType->addItem("16位累加和", int(ChecksumType::Sum16));
  checksumLayout->addRow("校验算法:", m_comboChecksumType);

  m_spinChecksumPos = new QSpinBox();
  m_spinChecksumPos->setRange(-1, 255);
  m_spinChecksumPos->setValue(-1);
  m_spinChecksumPos->setSpecialValueText("帧尾之前");
  checksumLayout->addRow("校验码位置:", m_spinChecksumPos);

  mainLayout->addWidget(checksumGroup);

  // 帧尾配置
  QGroupBox *tailGroup = new QGroupBox("帧尾配置");
  QVBoxLayout *tailLayout = new QVBoxLayout(tailGroup);

  m_chkUseTail = new QCheckBox("使用帧尾");
  tailLayout->addWidget(m_chkUseTail);

  QHBoxLayout *tailEditLayout = new QHBoxLayout();
  tailEditLayout->addWidget(new QLabel("帧尾 (HEX):"));
  m_editFrameTail = new QLineEdit();
  m_editFrameTail->setPlaceholderText("例如: 0D 0A");
  tailEditLayout->addWidget(m_editFrameTail);
  tailLayout->addLayout(tailEditLayout);

  mainLayout->addWidget(tailGroup);

  // 循环发送配置
  QGroupBox *loopGroup = new QGroupBox("循环发送");
  QFormLayout *loopLayout = new QFormLayout(loopGroup);

  m_chkAutoSendLoop = new QCheckBox("启用循环发送");
  loopLayout->addRow(m_chkAutoSendLoop);

  m_spinLoopInterval = new QSpinBox();
  m_spinLoopInterval->setRange(100, 60000);
  m_spinLoopInterval->setValue(1000);
  m_spinLoopInterval->setSuffix(" ms");
  loopLayout->addRow("发送间隔:", m_spinLoopInterval);

  mainLayout->addWidget(loopGroup);

  // 按钮
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  QPushButton *btnOk = new QPushButton("确定");
  QPushButton *btnCancel = new QPushButton("取消");
  buttonLayout->addWidget(btnOk);
  buttonLayout->addWidget(btnCancel);
  mainLayout->addLayout(buttonLayout);

  connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
  connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ProtocolConfigDialog::setConfig(const ProtocolConfig &config) {
  m_editName->setText(config.name);

  m_chkUseHeader->setChecked(config.useFrameHeader);
  m_editFrameHeader->setText(config.frameHeader.toHex(' ').toUpper());

  m_chkUseTail->setChecked(config.useFrameTail);
  m_editFrameTail->setText(config.frameTail.toHex(' ').toUpper());

  m_chkUseLength->setChecked(config.useLength);
  m_spinLengthPos->setValue(config.lengthPosition);
  m_spinLengthSize->setValue(config.lengthSize);
  m_chkLengthIncludesHeader->setChecked(config.lengthIncludesHeader);

  m_chkUseSequence->setChecked(config.useSequence);
  m_spinSequencePos->setValue(config.sequencePosition);

  m_chkUseFunctionCode->setChecked(config.useFunctionCode);
  m_spinFunctionCodePos->setValue(config.functionCodePosition);
  m_editFunctionCode->setText(QString::number(config.functionCode, 16).toUpper());

  m_editDataPayload->setPlainText(config.dataPayload.toHex(' ').toUpper());

  for (int i = 0; i < m_comboChecksumType->count(); ++i) {
    if (m_comboChecksumType->itemData(i).toInt() ==
        int(config.checksumType)) {
      m_comboChecksumType->setCurrentIndex(i);
      break;
    }
  }
  m_spinChecksumPos->setValue(config.checksumPosition);

  m_chkAutoSendLoop->setChecked(config.autoSendLoop);
  m_spinLoopInterval->setValue(config.loopInterval);
}

ProtocolConfig ProtocolConfigDialog::getConfig() const {
  ProtocolConfig config;

  config.name = m_editName->text();

  config.useFrameHeader = m_chkUseHeader->isChecked();
  config.frameHeader =
      QByteArray::fromHex(m_editFrameHeader->text().toLatin1());

  config.useFrameTail = m_chkUseTail->isChecked();
  config.frameTail = QByteArray::fromHex(m_editFrameTail->text().toLatin1());

  config.useLength = m_chkUseLength->isChecked();
  config.lengthPosition = m_spinLengthPos->value();
  config.lengthSize = m_spinLengthSize->value();
  config.lengthIncludesHeader = m_chkLengthIncludesHeader->isChecked();

  config.useSequence = m_chkUseSequence->isChecked();
  config.sequencePosition = m_spinSequencePos->value();
  config.sequenceSize = 2;

  config.useFunctionCode = m_chkUseFunctionCode->isChecked();
  config.functionCodePosition = m_spinFunctionCodePos->value();
  config.functionCode = m_editFunctionCode->text().toUInt(nullptr, 16);

  config.dataPayload =
      QByteArray::fromHex(m_editDataPayload->toPlainText().toLatin1());

  config.checksumType = ChecksumType(
      m_comboChecksumType->currentData().toInt());
  config.checksumPosition = m_spinChecksumPos->value();

  config.autoSendLoop = m_chkAutoSendLoop->isChecked();
  config.loopInterval = m_spinLoopInterval->value();

  return config;
}
