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

#include <QScrollArea>
#include <QStackedWidget>

// ============ ProtocolPartWidget 实现 ============

ProtocolPartWidget::ProtocolPartWidget(const ProtocolDynamicSegment &seg, QWidget *parent) : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_comboType = new QComboBox();
  m_comboType->addItems({"帧头", "帧尾", "数据段长度", "帧序号", "功能码", "数据区", "校验", "固定字节/数值"});
  m_comboType->setCurrentIndex((int)seg.type);
  
  m_stackedParams = new QStackedWidget();
  
  m_editValue = new QLineEdit();
  m_editValue->setPlaceholderText("用空格分隔HEX,如 AA 55");
  m_stackedParams->addWidget(m_editValue); // page 0
  
  QWidget *page1 = new QWidget();
  QHBoxLayout *page1Layout = new QHBoxLayout(page1);
  page1Layout->setContentsMargins(0, 0, 0, 0);
  m_comboSize = new QComboBox();
  m_comboSize->addItems({"1 字节", "2 字节", "4 字节"});
  page1Layout->addWidget(m_comboSize);
  
  m_comboLengthRange = new QComboBox();
  m_comboLengthRange->addItems({"全部包括(全帧长度)", "随后所有数据", "之后除校验和尾部的数据", "仅包括数据区(DataPayload)"});
  page1Layout->addWidget(m_comboLengthRange);
  m_stackedParams->addWidget(page1); // page 1
  
  m_comboChecksumAlgo = new QComboBox();
  m_comboChecksumAlgo->addItem("无校验", int(ChecksumType::None));
  m_comboChecksumAlgo->addItem("异或校验 (XOR)", int(ChecksumType::XOR));
  m_comboChecksumAlgo->addItem("CRC-8", int(ChecksumType::CRC8));
  m_comboChecksumAlgo->addItem("CRC-16", int(ChecksumType::CRC16));
  m_comboChecksumAlgo->addItem("CRC-32", int(ChecksumType::CRC32));
  m_comboChecksumAlgo->addItem("8位累加和", int(ChecksumType::Sum8));
  m_comboChecksumAlgo->addItem("16位累加和", int(ChecksumType::Sum16));
  m_stackedParams->addWidget(m_comboChecksumAlgo); // page 2

  QPushButton *btnUp = new QPushButton("↑");
  btnUp->setFixedWidth(30);
  QPushButton *btnDown = new QPushButton("↓");
  btnDown->setFixedWidth(30);
  QPushButton *btnRemove = new QPushButton("-");
  btnRemove->setFixedWidth(30);
  
  layout->addWidget(m_comboType);
  layout->addWidget(m_stackedParams, 1);
  layout->addWidget(btnUp);
  layout->addWidget(btnDown);
  layout->addWidget(btnRemove);
  
  connect(m_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProtocolPartWidget::onTypeChanged);
  connect(btnUp, &QPushButton::clicked, this, [this](){ emit moveUpRequested(this); });
  connect(btnDown, &QPushButton::clicked, this, [this](){ emit moveDownRequested(this); });
  connect(btnRemove, &QPushButton::clicked, this, [this](){ emit removeRequested(this); });
  
  // Init values
  m_editValue->setText(seg.value.toHex(' '));
  if (seg.size == 1) m_comboSize->setCurrentIndex(0);
  else if (seg.size == 2) m_comboSize->setCurrentIndex(1);
  else if (seg.size == 4) m_comboSize->setCurrentIndex(2);
  
  if (seg.type == SegmentType::DataLength) {
    m_comboLengthRange->setCurrentIndex(seg.config);
  } else if (seg.type == SegmentType::Checksum) {
    for (int i = 0; i < m_comboChecksumAlgo->count(); ++i) {
      if (m_comboChecksumAlgo->itemData(i).toInt() == seg.config) {
        m_comboChecksumAlgo->setCurrentIndex(i);
        break;
      }
    }
  }
  
  onTypeChanged((int)seg.type);
}

void ProtocolPartWidget::onTypeChanged(int index) {
  SegmentType t = (SegmentType)index;
  if (t == SegmentType::DataLength || t == SegmentType::Sequence) {
    m_stackedParams->setCurrentIndex(1);
    m_comboLengthRange->setVisible(t == SegmentType::DataLength);
  } else if (t == SegmentType::Checksum) {
    m_stackedParams->setCurrentIndex(2);
  } else {
    m_stackedParams->setCurrentIndex(0);
  }
}

ProtocolDynamicSegment ProtocolPartWidget::getSegment() const {
  ProtocolDynamicSegment seg;
  seg.type = (SegmentType)m_comboType->currentIndex();
  seg.value = QByteArray::fromHex(m_editValue->text().toLatin1());
  int sizeIdx = m_comboSize->currentIndex();
  if (sizeIdx == 0) seg.size = 1;
  else if (sizeIdx == 1) seg.size = 2;
  else if (sizeIdx == 2) seg.size = 4;
  
  if (seg.type == SegmentType::Checksum) {
    seg.config = m_comboChecksumAlgo->currentData().toInt();
  } else if (seg.type == SegmentType::DataLength) {
    seg.config = m_comboLengthRange->currentIndex();
  } else {
    seg.config = 0;
  }
  return seg;
}

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
  m_buttonStyle = styleIndex;
  
  if (m_buttonStyle == 8) {
    if (!m_borderTimer) {
      m_borderTimer = new QTimer(this);
      connect(m_borderTimer, &QTimer::timeout, this, &CustomProtocolButton::onBorderTimerTimeout);
    }
    m_borderTimer->start(30);
  } else {
    if (m_borderTimer) {
      m_borderTimer->stop();
    }
  }

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
      "}",
      
      // 样式3: 平面样式 (Flat style)
      "QPushButton {"
      "  background: #3498db;"
      "  color: white;"
      "  border: none;"
      "  border-radius: 4px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:hover { background: #2980b9; }"
      "QPushButton:pressed { background: #1c598a; }",
      
      // 样式4: 立体样式 (3D Neumorphism)
      "QPushButton {"
      "  background: #e0e5ec;"
      "  color: #333;"
      "  border-top: 2px solid #ffffff;"
      "  border-left: 2px solid #ffffff;"
      "  border-bottom: 2px solid #a3b1c6;"
      "  border-right: 2px solid #a3b1c6;"
      "  border-radius: 8px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:hover { background: #d3d9e3; }"
      "QPushButton:pressed {"
      "  border-bottom: 2px solid #ffffff;"
      "  border-right: 2px solid #ffffff;"
      "  border-top: 2px solid #a3b1c6;"
      "  border-left: 2px solid #a3b1c6;"
      "}",

      // 样式5: 金属质感 (Metal texture)
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f5f5f5, stop:0.4 #e0e0e0, stop:0.5 #c8c8c8, stop:1 #e0e0e0);"
      "  color: #333;"
      "  border: 1px solid #707070;"
      "  border-radius: 6px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:0.4 #ebebeb, stop:0.5 #d3d3d3, stop:1 #ebebeb);"
      "}"
      "QPushButton:pressed {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #b8b8b8, stop:0.4 #c8c8c8, stop:0.5 #e0e0e0, stop:1 #c8c8c8);"
      "}",

      // 样式6: 磨砂质感 (Matte texture)
      "QPushButton {"
      "  background: #3c3c3c;"
      "  color: #eeeeee;"
      "  border: 1px solid #222;"
      "  border-radius: 8px;"
      "  font-weight: normal;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:hover { background: #4a4a4a; }"
      "QPushButton:pressed { background: #2b2b2b; }",

      // 样式7: 键盘按键样式 (Keyboard Key)
      "QPushButton {"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fafafa, stop:1 #d4d4d4);"
      "  color: #222;"
      "  border-top: 3px solid #fff;"
      "  border-left: 3px solid #fff;"
      "  border-bottom: 3px solid #b3b3b3;"
      "  border-right: 3px solid #b3b3b3;"
      "  border-radius: 6px;"
      "  font-family: monospace;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:pressed {"
      "  border-top: 3px solid #b3b3b3;"
      "  border-left: 3px solid #b3b3b3;"
      "  border-bottom: 3px solid #fff;"
      "  border-right: 3px solid #fff;"
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #d4d4d4, stop:1 #fafafa);"
      "  padding-top: 2px;"
      "  padding-left: 2px;"
      "}",
      
      // 样式8: RGB 流光边框 (Animated border overlay)
      "QPushButton {"
      "  background: #1e1e1e;"
      "  color: white;"
      "  border: none;"
      "  border-radius: 8px;"
      "  font-weight: bold;"
      "  font-size: 12pt;"
      "}"
      "QPushButton:hover { background: #2d2d2d; }"
      "QPushButton:pressed { background: #111111; }"
  };

  if (styleIndex >= 0 && styleIndex < 9) {
    setStyleSheet(styles[styleIndex]);
  }
}

void CustomProtocolButton::onBorderTimerTimeout() {
  m_borderAngle = (m_borderAngle + 5) % 360;
  update();
}

#include <QPainter>
#include <QPainterPath>
void CustomProtocolButton::paintEvent(QPaintEvent *event) {
  QPushButton::paintEvent(event);

  if (m_buttonStyle == 8) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QConicalGradient grad(rect().center(), m_borderAngle);
    grad.setColorAt(0.0, Qt::red);
    grad.setColorAt(0.16, Qt::magenta);
    grad.setColorAt(0.33, Qt::blue);
    grad.setColorAt(0.5, Qt::cyan);
    grad.setColorAt(0.66, Qt::green);
    grad.setColorAt(0.83, Qt::yellow);
    grad.setColorAt(1.0, Qt::red);
    
    QPen pen(grad, 4);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 6, 6);
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
    m_buttonStyle = (m_buttonStyle + 1) % 9;
    setKeyboardStyle(m_buttonStyle);
  } else if (selected == deleteAction) {
    deleteLater();
  }
}

QByteArray CustomProtocolButton::buildFrame() {
  if (m_config.configMode == 1) {
    QByteArray frame;
    QList<int> lengthPositions;
    QList<int> checksumPositions;
    
    // First pass: placeholder and fixed values
    for (const auto &seg : m_config.dynamicSegments) {
      switch (seg.type) {
      case SegmentType::FrameHeader:
      case SegmentType::FrameTail:
      case SegmentType::FixedValue:
      case SegmentType::FunctionCode:
      case SegmentType::DataPayload:
        frame.append(seg.value);
        break;
      case SegmentType::Sequence: {
        int seqSize = seg.size;
        if (seqSize == 1) {
          frame.append(char(m_sequenceNumber & 0xFF));
        } else if (seqSize == 2) {
          frame.append(char((m_sequenceNumber >> 8) & 0xFF));
          frame.append(char(m_sequenceNumber & 0xFF));
        } else if (seqSize == 4) {
          frame.append(char((m_sequenceNumber >> 24) & 0xFF));
          frame.append(char((m_sequenceNumber >> 16) & 0xFF));
          frame.append(char((m_sequenceNumber >> 8) & 0xFF));
          frame.append(char(m_sequenceNumber & 0xFF));
        }
        m_sequenceNumber++;
        break;
      }
      case SegmentType::DataLength: {
        lengthPositions.append(frame.size());
        frame.append(QByteArray(seg.size, '\0'));
        break;
      }
      case SegmentType::Checksum: {
        checksumPositions.append(frame.size());
        int checksumSize = 1;
        if (seg.config == int(ChecksumType::CRC16) || seg.config == int(ChecksumType::Sum16)) checksumSize = 2;
        else if (seg.config == int(ChecksumType::CRC32)) checksumSize = 4;
        frame.append(QByteArray(checksumSize, '\0'));
        break;
      }
      }
    }
    
    // Second pass: lengths (using whole frame size)
    int totalSize = frame.size();
    for (int i = 0; i < m_config.dynamicSegments.size(); ++i) {
      const auto &seg = m_config.dynamicSegments[i];
      if (seg.type == SegmentType::DataLength) {
        int pos = lengthPositions.takeFirst();
        int val = totalSize; 
        
        // m_comboLengthRange settings handling:
        // 0: 全帧长度
        // 1: 随后所有数据
        // 2: 之后除校验和尾部的数据
        // 3: 仅包括数据区(DataPayload)
        if (seg.config == 1) {
            val = totalSize - (pos + seg.size);
        } else if (seg.config == 2) {
            int excludedSize = 0;
            for (int j = i + 1; j < m_config.dynamicSegments.size(); ++j) {
                if (m_config.dynamicSegments[j].type == SegmentType::Checksum) {
                    int cs = 1;
                    if (m_config.dynamicSegments[j].config == int(ChecksumType::CRC16) || m_config.dynamicSegments[j].config == int(ChecksumType::Sum16)) cs = 2;
                    else if (m_config.dynamicSegments[j].config == int(ChecksumType::CRC32)) cs = 4;
                    excludedSize += cs;
                } else if (m_config.dynamicSegments[j].type == SegmentType::FrameTail) {
                    excludedSize += m_config.dynamicSegments[j].value.size();
                }
            }
            val = totalSize - (pos + seg.size) - excludedSize;
        } else if (seg.config == 3) {
            int dataPayloadSize = 0;
            for (int j = 0; j < m_config.dynamicSegments.size(); ++j) {
                if (m_config.dynamicSegments[j].type == SegmentType::DataPayload) {
                    dataPayloadSize += m_config.dynamicSegments[j].value.size();
                }
            }
            val = dataPayloadSize;
        }
        
        if (seg.size == 1) { frame[pos] = char(val & 0xFF); }
        else if (seg.size == 2) { frame[pos] = char((val >> 8) & 0xFF); frame[pos + 1] = char(val & 0xFF); }
        else if (seg.size == 4) { frame[pos] = char((val >> 24) & 0xFF); frame[pos + 1] = char((val >> 16) & 0xFF); frame[pos + 2] = char((val >> 8) & 0xFF); frame[pos + 3] = char(val & 0xFF); }
      }
    }
    
    // Third pass: checksums
    for (int i = 0; i < m_config.dynamicSegments.size(); ++i) {
      const auto &seg = m_config.dynamicSegments[i];
      if (seg.type == SegmentType::Checksum) {
        int pos = checksumPositions.takeFirst();
        QByteArray dataToSum = frame.mid(0, pos);
        QByteArray checksumBytes;
        switch (ChecksumType(seg.config)) {
        case ChecksumType::XOR:   checksumBytes.append(char(calculateXOR(dataToSum))); break;
        case ChecksumType::CRC8:  checksumBytes.append(char(calculateCRC8(dataToSum))); break;
        case ChecksumType::CRC16: { quint16 checksum = calculateCRC16(dataToSum); checksumBytes.append(char((checksum >> 8) & 0xFF)); checksumBytes.append(char(checksum & 0xFF)); break; }
        case ChecksumType::CRC32: { quint32 checksum = calculateCRC32(dataToSum); checksumBytes.append(char((checksum >> 24) & 0xFF)); checksumBytes.append(char((checksum >> 16) & 0xFF)); checksumBytes.append(char((checksum >> 8) & 0xFF)); checksumBytes.append(char(checksum & 0xFF)); break; }
        case ChecksumType::Sum8:  checksumBytes.append(char(calculateSum8(dataToSum))); break;
        case ChecksumType::Sum16: { quint16 checksum = calculateSum16(dataToSum); checksumBytes.append(char((checksum >> 8) & 0xFF)); checksumBytes.append(char(checksum & 0xFF)); break; }
        default: break;
        }
        if (checksumBytes.size() <= frame.size() - pos) {
          for (int b = 0; b < checksumBytes.size(); ++b) {
            frame[pos + b] = checksumBytes[b];
          }
        }
      }
    }
    return frame;
  }
  
  QByteArray frame;

  // 1. Calculate minimum required base size for configurable headers
  int baseSize = 0;
  if (m_config.useFrameHeader) {
    baseSize = qMax(baseSize, (int)m_config.frameHeader.size());
  }
  if (m_config.useLength) {
    baseSize = qMax(baseSize, m_config.lengthPosition + m_config.lengthSize);
  }
  if (m_config.useSequence) {
    baseSize = qMax(baseSize, m_config.sequencePosition + m_config.sequenceSize);
  }
  if (m_config.useFunctionCode) {
    baseSize = qMax(baseSize, m_config.functionCodePosition + 1);
  }

  // Initialize frame with zeros up to baseSize
  frame.fill(0, baseSize);

  // 2. Put Header
  if (m_config.useFrameHeader) {
    frame.replace(0, m_config.frameHeader.size(), m_config.frameHeader);
  }

  // 3. Put Frame Sequence
  if (m_config.useSequence) {
    if (m_config.sequenceSize == 1) {
      frame[m_config.sequencePosition] = char(m_sequenceNumber & 0xFF);
    } else if (m_config.sequenceSize == 2) {
      frame[m_config.sequencePosition] = char((m_sequenceNumber >> 8) & 0xFF);
      frame[m_config.sequencePosition + 1] = char(m_sequenceNumber & 0xFF);
    }
    m_sequenceNumber++;
  }

  // 4. Put Function Code
  if (m_config.useFunctionCode) {
    frame[m_config.functionCodePosition] = char(m_config.functionCode);
  }

  // 5. Append Data Payload
  frame.append(m_config.dataPayload);

  // 6. Calculate Length
  if (m_config.useLength) {
    int length = m_config.lengthIncludesHeader
                     ? frame.size()
                     : (frame.size() - m_config.lengthPosition - m_config.lengthSize);

    int pos = m_config.lengthPosition;
    if (m_config.lengthSize == 1) {
      frame[pos] = char(length & 0xFF);
    } else if (m_config.lengthSize == 2) {
      frame[pos] = char((length >> 8) & 0xFF);
      frame[pos + 1] = char(length & 0xFF);
    } else if (m_config.lengthSize == 4) {
      frame[pos] = char((length >> 24) & 0xFF);
      frame[pos + 1] = char((length >> 16) & 0xFF);
      frame[pos + 2] = char((length >> 8) & 0xFF);
      frame[pos + 3] = char(length & 0xFF);
    }
  }

  // 7. Calculate and Add Checksum
  if (m_config.checksumType != ChecksumType::None) {
    QByteArray checksumData = frame;
    // Calculate checksum typically bypasses the frame header
    if (m_config.useFrameHeader && checksumData.size() >= m_config.frameHeader.size()) {
      checksumData = frame.mid(m_config.frameHeader.size());
    } else if (m_config.useFrameHeader) {
      checksumData.clear();
    }

    QByteArray checksumBytes;
    switch (m_config.checksumType) {
    case ChecksumType::XOR: {
      checksumBytes.append(char(calculateXOR(checksumData)));
      break;
    }
    case ChecksumType::CRC8: {
      checksumBytes.append(char(calculateCRC8(checksumData)));
      break;
    }
    case ChecksumType::CRC16: {
      quint16 checksum = calculateCRC16(checksumData);
      checksumBytes.append(char((checksum >> 8) & 0xFF));
      checksumBytes.append(char(checksum & 0xFF));
      break;
    }
    case ChecksumType::CRC32: {
      quint32 checksum = calculateCRC32(checksumData);
      checksumBytes.append(char((checksum >> 24) & 0xFF));
      checksumBytes.append(char((checksum >> 16) & 0xFF));
      checksumBytes.append(char((checksum >> 8) & 0xFF));
      checksumBytes.append(char(checksum & 0xFF));
      break;
    }
    case ChecksumType::Sum8: {
      checksumBytes.append(char(calculateSum8(checksumData)));
      break;
    }
    case ChecksumType::Sum16: {
      quint16 checksum = calculateSum16(checksumData);
      checksumBytes.append(char((checksum >> 8) & 0xFF));
      checksumBytes.append(char(checksum & 0xFF));
      break;
    }
    default:
      break;
    }

    if (m_config.checksumPosition == -1) {
      // End of the frame before the tail
      frame.append(checksumBytes);
    } else {
      // Specific position
      int csPos = m_config.checksumPosition;
      if (csPos + checksumBytes.size() > frame.size()) {
        QByteArray padding(csPos + checksumBytes.size() - frame.size(), char(0));
        frame.append(padding);
      }
      frame.replace(csPos, checksumBytes.size(), checksumBytes);
    }
  }

  // 8. Add Frame Tail
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
  setMinimumWidth(550);
  setMinimumHeight(450);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  // Common Top UI
  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->addWidget(new QLabel("协议名称:"));
  m_editNameCommon = new QLineEdit("自定义协议");
  topLayout->addWidget(m_editNameCommon);
  topLayout->addSpacing(20);
  topLayout->addWidget(new QLabel("配置模式:"));
  m_comboMode = new QComboBox();
  m_comboMode->addItems({"按功能块配置 (模式1)", "按字节段自由组合 (模式2)"});
  topLayout->addWidget(m_comboMode);
  mainLayout->addLayout(topLayout);
  
  m_mainStackedWidget = new QStackedWidget();
  
  // --- Mode 1 UI ---
  QWidget *page1 = new QWidget();
  QVBoxLayout *page1Layout = new QVBoxLayout(page1);
  page1Layout->setContentsMargins(0,0,0,0);
  QScrollArea *sa1 = new QScrollArea();
  sa1->setWidgetResizable(true);
  QWidget *sa1w = new QWidget();
  QVBoxLayout *psa1 = new QVBoxLayout(sa1w);
  
  m_editName = new QLineEdit(); // kept for compatibility but not displayed or synced mostly
  
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
  psa1->addWidget(headerGroup);

  // 数据字段配置
  QGroupBox *dataGroup = new QGroupBox("数据字段配置");
  QFormLayout *dataLayout = new QFormLayout(dataGroup);
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

  m_chkUseSequence = new QCheckBox("使用帧序号");
  dataLayout->addRow(m_chkUseSequence);
  m_spinSequencePos = new QSpinBox();
  m_spinSequencePos->setRange(0, 255);
  dataLayout->addRow("序号字段位置:", m_spinSequencePos);

  m_chkUseFunctionCode = new QCheckBox("使用功能码");
  dataLayout->addRow(m_chkUseFunctionCode);
  m_spinFunctionCodePos = new QSpinBox();
  m_spinFunctionCodePos->setRange(0, 255);
  dataLayout->addRow("功能码位置:", m_spinFunctionCodePos);
  m_editFunctionCode = new QLineEdit();
  m_editFunctionCode->setPlaceholderText("例如: 01");
  dataLayout->addRow("功能码 (HEX):", m_editFunctionCode);
  psa1->addWidget(dataGroup);

  // 数据区
  QGroupBox *payloadGroup = new QGroupBox("数据区");
  QVBoxLayout *payloadLayout = new QVBoxLayout(payloadGroup);
  m_editDataPayload = new QTextEdit();
  m_editDataPayload->setPlaceholderText("输入数据 (HEX格式，例如: 01 02 03 04)");
  m_editDataPayload->setMaximumHeight(80);
  payloadLayout->addWidget(m_editDataPayload);
  psa1->addWidget(payloadGroup);

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
  psa1->addWidget(checksumGroup);

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
  psa1->addWidget(tailGroup);
  
  psa1->addStretch();
  sa1->setWidget(sa1w);
  page1Layout->addWidget(sa1);
  m_mainStackedWidget->addWidget(page1);

  // --- Mode 2 UI ---
  QWidget *page2 = new QWidget();
  QVBoxLayout *page2Layout = new QVBoxLayout(page2);
  page2Layout->setContentsMargins(0,0,0,0);
  QScrollArea *sa2 = new QScrollArea();
  sa2->setWidgetResizable(true);
  QWidget *sa2w = new QWidget();
  m_dynamicListLayout = new QVBoxLayout(sa2w);
  m_dynamicListLayout->addStretch(); // push everything to top
  sa2->setWidget(sa2w);
  page2Layout->addWidget(sa2);
  
  QPushButton *btnAddSegment = new QPushButton("+ 添加字节段");
  page2Layout->addWidget(btnAddSegment);
  m_mainStackedWidget->addWidget(page2);
  
  mainLayout->addWidget(m_mainStackedWidget, 1);

  // Common Bottom (Loop config & Buttons)
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

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  QPushButton *btnOk = new QPushButton("确定");
  QPushButton *btnCancel = new QPushButton("取消");
  buttonLayout->addWidget(btnOk);
  buttonLayout->addWidget(btnCancel);
  mainLayout->addLayout(buttonLayout);

  connect(m_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProtocolConfigDialog::onModeChanged);
  connect(btnAddSegment, &QPushButton::clicked, this, &ProtocolConfigDialog::onAddSegmentClicked);
  connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
  connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ProtocolConfigDialog::onModeChanged(int index) {
  m_mainStackedWidget->setCurrentIndex(index);
}

void ProtocolConfigDialog::onAddSegmentClicked() {
  ProtocolPartWidget *pw = new ProtocolPartWidget(ProtocolDynamicSegment());
  m_dynamicListLayout->insertWidget(m_partWidgets.size(), pw);
  m_partWidgets.append(pw);
  
  connect(pw, &ProtocolPartWidget::removeRequested, this, [this](ProtocolPartWidget *w){
    m_partWidgets.removeOne(w);
    m_dynamicListLayout->removeWidget(w);
    delete w;
  });
  connect(pw, &ProtocolPartWidget::moveUpRequested, this, [this](ProtocolPartWidget *w){
    int idx = m_partWidgets.indexOf(w);
    if (idx > 0) {
      m_partWidgets.swapItemsAt(idx, idx - 1);
      m_dynamicListLayout->removeWidget(w);
      // Because there is a stretch item at the end, insert at correct layout index
      // Layout structure: pw1, pw2 ... pwN, stretch
      m_dynamicListLayout->insertWidget(idx - 1, w);
    }
  });
  connect(pw, &ProtocolPartWidget::moveDownRequested, this, [this](ProtocolPartWidget *w){
    int idx = m_partWidgets.indexOf(w);
    if (idx >= 0 && idx < m_partWidgets.size() - 1) {
      m_partWidgets.swapItemsAt(idx, idx + 1);
      m_dynamicListLayout->removeWidget(w);
      m_dynamicListLayout->insertWidget(idx + 1, w);
    }
  });
}

void ProtocolConfigDialog::setConfig(const ProtocolConfig &config) {
  m_editNameCommon->setText(config.name);
  m_comboMode->setCurrentIndex(config.configMode);
  
  // Handle Mode 2 load
  for (const auto &seg : config.dynamicSegments) {
    ProtocolPartWidget *pw = new ProtocolPartWidget(seg);
    m_dynamicListLayout->insertWidget(m_partWidgets.size(), pw);
    m_partWidgets.append(pw);
    
    connect(pw, &ProtocolPartWidget::removeRequested, this, [this](ProtocolPartWidget *w){
      m_partWidgets.removeOne(w);
      m_dynamicListLayout->removeWidget(w);
      delete w;
    });
    connect(pw, &ProtocolPartWidget::moveUpRequested, this, [this](ProtocolPartWidget *w){
      int idx = m_partWidgets.indexOf(w);
      if (idx > 0) {
        m_partWidgets.swapItemsAt(idx, idx - 1);
        m_dynamicListLayout->removeWidget(w);
        m_dynamicListLayout->insertWidget(idx - 1, w);
      }
    });
    connect(pw, &ProtocolPartWidget::moveDownRequested, this, [this](ProtocolPartWidget *w){
      int idx = m_partWidgets.indexOf(w);
      if (idx >= 0 && idx < m_partWidgets.size() - 1) {
        m_partWidgets.swapItemsAt(idx, idx + 1);
        m_dynamicListLayout->removeWidget(w);
        m_dynamicListLayout->insertWidget(idx + 1, w);
      }
    });
  }

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

  config.name = m_editNameCommon->text();
  config.configMode = m_comboMode->currentIndex();
  for (auto *pw : m_partWidgets) {
    config.dynamicSegments.append(pw->getSegment());
  }

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
