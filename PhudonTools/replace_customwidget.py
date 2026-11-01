import os

filepath = r"m:\TOPFIRE\SmileCodeIDE\STM32IDE\customwidget.cpp"

with open(filepath, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Add headers and ProtocolPartWidget impl
impl_text = """#include <QScrollArea>
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
  
  m_comboSize = new QComboBox();
  m_comboSize->addItems({"1 字节", "2 字节", "4 字节"});
  m_stackedParams->addWidget(m_comboSize); // page 1
  
  m_comboChecksumAlgo = new QComboBox();
  m_comboChecksumAlgo->addItem("无校验", int(ChecksumType::None));
  m_comboChecksumAlgo->addItem("异或校验 (XOR)", int(ChecksumType::XOR));
  m_comboChecksumAlgo->addItem("CRC-8", int(ChecksumType::CRC8));
  m_comboChecksumAlgo->addItem("CRC-16", int(ChecksumType::CRC16));
  m_comboChecksumAlgo->addItem("CRC-32", int(ChecksumType::CRC32));
  m_comboChecksumAlgo->addItem("8位累加和", int(ChecksumType::Sum8));
  m_comboChecksumAlgo->addItem("16位累加和", int(ChecksumType::Sum16));
  m_stackedParams->addWidget(m_comboChecksumAlgo); // page 2

  QPushButton *btnRemove = new QPushButton("-");
  btnRemove->setFixedWidth(30);
  
  layout->addWidget(m_comboType);
  layout->addWidget(m_stackedParams, 1);
  layout->addWidget(btnRemove);
  
  connect(m_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProtocolPartWidget::onTypeChanged);
  connect(btnRemove, &QPushButton::clicked, this, [this](){ emit removeRequested(this); });
  
  // Init values
  m_editValue->setText(seg.value.toHex(' '));
  if (seg.size == 1) m_comboSize->setCurrentIndex(0);
  else if (seg.size == 2) m_comboSize->setCurrentIndex(1);
  else if (seg.size == 4) m_comboSize->setCurrentIndex(2);
  
  for (int i = 0; i < m_comboChecksumAlgo->count(); ++i) {
    if (m_comboChecksumAlgo->itemData(i).toInt() == seg.config) {
      m_comboChecksumAlgo->setCurrentIndex(i);
      break;
    }
  }
  
  onTypeChanged((int)seg.type);
}

void ProtocolPartWidget::onTypeChanged(int index) {
  SegmentType t = (SegmentType)index;
  if (t == SegmentType::DataLength || t == SegmentType::Sequence) {
    m_stackedParams->setCurrentIndex(1);
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
  seg.config = m_comboChecksumAlgo->currentData().toInt();
  return seg;
}

// ============ CustomProtocolButton 实现 ============"""
content = content.replace("// ============ CustomProtocolButton 实现 ============", impl_text)

# 2. Modify buildFrame()
build_frame_old = "QByteArray CustomProtocolButton::buildFrame() {\n  QByteArray frame;\n"
build_frame_new = """QByteArray CustomProtocolButton::buildFrame() {
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
        frame.append(QByteArray(seg.size, '\\0'));
        break;
      }
      case SegmentType::Checksum: {
        checksumPositions.append(frame.size());
        int checksumSize = 1;
        if (seg.config == int(ChecksumType::CRC16) || seg.config == int(ChecksumType::Sum16)) checksumSize = 2;
        else if (seg.config == int(ChecksumType::CRC32)) checksumSize = 4;
        frame.append(QByteArray(checksumSize, '\\0'));
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
"""
content = content.replace(build_frame_old, build_frame_new)

# 3. ProtocolConfigDialog constructor replacement
dialog_start = "ProtocolConfigDialog::ProtocolConfigDialog(QWidget *parent)\n    : QDialog(parent) {\n  setWindowTitle(\"协议配置\");\n  setMinimumWidth(500);\n\n  QVBoxLayout *mainLayout = new QVBoxLayout(this);\n\n  // 基本信息\n  QGroupBox *basicGroup = new QGroupBox(\"基本信息\");\n  QFormLayout *basicLayout = new QFormLayout(basicGroup);\n\n  m_editName = new QLineEdit();\n  basicLayout->addRow(\"协议名称:\", m_editName);\n\n  mainLayout->addWidget(basicGroup);"
dialog_end = "  loopLayout->addRow(\"发送间隔:\", m_spinLoopInterval);\n\n  mainLayout->addWidget(loopGroup);\n\n  // 按钮\n  QHBoxLayout *buttonLayout = new QHBoxLayout();\n  buttonLayout->addStretch();\n  QPushButton *btnOk = new QPushButton(\"确定\");\n  QPushButton *btnCancel = new QPushButton(\"取消\");\n  buttonLayout->addWidget(btnOk);\n  buttonLayout->addWidget(btnCancel);\n  mainLayout->addLayout(buttonLayout);\n\n  connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);\n  connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);\n}"

new_dialog_code = """ProtocolConfigDialog::ProtocolConfigDialog(QWidget *parent)
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
}
"""

start_idx = content.find("ProtocolConfigDialog::ProtocolConfigDialog(QWidget *parent)")
end_idx = content.find("void ProtocolConfigDialog::setConfig(const ProtocolConfig &config)")
if start_idx != -1 and end_idx != -1:
    content = content[:start_idx] + new_dialog_code + "\n" + content[end_idx:]


# 4. Modify setConfig and getConfig to support Mode 2 and Common Name
content = content.replace("m_editName->setText(config.name);", "m_editNameCommon->setText(config.name);\n  m_comboMode->setCurrentIndex(config.configMode);\n  \n  // Handle Mode 2 load\n  for (const auto &seg : config.dynamicSegments) {\n    ProtocolPartWidget *pw = new ProtocolPartWidget(seg);\n    m_dynamicListLayout->insertWidget(m_partWidgets.size(), pw);\n    m_partWidgets.append(pw);\n    connect(pw, &ProtocolPartWidget::removeRequested, this, [this](ProtocolPartWidget *w){\n      m_partWidgets.removeOne(w);\n      m_dynamicListLayout->removeWidget(w);\n      delete w;\n    });\n  }")

get_config_old = "ProtocolConfig config;\n\n  config.name = m_editName->text();\n"
get_config_new = """ProtocolConfig config;

  config.name = m_editNameCommon->text();
  config.configMode = m_comboMode->currentIndex();
  for (auto *pw : m_partWidgets) {
    config.dynamicSegments.append(pw->getSegment());
  }
"""
content = content.replace(get_config_old, get_config_new)

with open(filepath, "w", encoding="utf-8") as f:
    f.write(content)
print("Replacement script completed successfully.")
