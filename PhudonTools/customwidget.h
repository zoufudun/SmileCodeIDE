#ifndef CUSTOMWIDGET_H
#define CUSTOMWIDGET_H

#include <QByteArray>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QVector>
#include <QTimer>

// 校验算法类型
enum class ChecksumType {
  None,
  XOR,      // 异或校验
  CRC8,     // CRC-8
  CRC16,    // CRC-16
  CRC32,    // CRC-32
  Sum8,     // 8位累加和
  Sum16     // 16位累加和
};

// 动态段类型
enum class SegmentType {
  FrameHeader,    // 帧头
  FrameTail,      // 帧尾
  DataLength,     // 数据段长度
  Sequence,       // 帧序号
  FunctionCode,   // 功能码
  DataPayload,    // 数据区
  Checksum,       // 校验
  FixedValue      // 固定字节/数值
};

// 动态协议段配置
struct ProtocolDynamicSegment {
  SegmentType type = SegmentType::FixedValue;
  QByteArray value; // 对于固定值/头尾/功能码/预设数据
  int size = 1;     // 对于长度、序号的字节跨度
  int config = 0;   // 对于校验，则存储 ChecksumType 值；对于长度，存储是否包含头尾等标志
};

// 协议帧配置
struct ProtocolConfig {
  // 帧头
  QByteArray frameHeader;
  bool useFrameHeader = false;

  // 帧尾
  QByteArray frameTail;
  bool useFrameTail = false;

  // 数据长度字段
  bool useLength = false;
  int lengthPosition = 0;  // 长度字段在帧中的位置
  int lengthSize = 1;      // 长度字段字节数 (1/2/4)
  bool lengthIncludesHeader = false;

  // 帧序号
  bool useSequence = false;
  int sequencePosition = 0;
  int sequenceSize = 1;

  // 功能码
  bool useFunctionCode = false;
  int functionCodePosition = 0;
  quint8 functionCode = 0x01;

  // 数据区
  QByteArray dataPayload;

  // 校验码
  ChecksumType checksumType = ChecksumType::None;
  int checksumPosition = -1;  // -1表示在帧尾之前

  // 循环发送
  bool autoSendLoop = false;
  int loopInterval = 1000;  // 毫秒

  QString name = "自定义协议";

  // 模式选择: 0为常规模式(模式1), 1为动态拼接模式(模式2)
  int configMode = 0;
  QVector<ProtocolDynamicSegment> dynamicSegments;
};

// 自定义协议按钮控件
class CustomProtocolButton : public QPushButton {
  Q_OBJECT

public:
  explicit CustomProtocolButton(QWidget *parent = nullptr);

  void setProtocolConfig(const ProtocolConfig &config);
  ProtocolConfig getProtocolConfig() const { return m_config; }

  // 生成完整的协议帧
  QByteArray buildFrame();

  // 设置按钮样式（键盘类按钮）
  void setKeyboardStyle(int styleIndex = 0);

  // 循环发送控制
  void startAutoSend();
  void stopAutoSend();
  bool isAutoSending() const { return m_autoSendTimer && m_autoSendTimer->isActive(); }

signals:
  void sendData(const QByteArray &data);

private slots:
  void onButtonClicked();
  void showConfigDialog();
  void onAutoSendTimeout();
  void onBorderTimerTimeout();

protected:
  void paintEvent(QPaintEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  ProtocolConfig m_config;
  quint16 m_sequenceNumber = 0;
  class QTimer *m_autoSendTimer = nullptr;
  QPoint m_dragStartPos;
  int m_buttonStyle = 0;
  class QTimer *m_borderTimer = nullptr;
  int m_borderAngle = 0;

  // 校验算法实现
  quint8 calculateXOR(const QByteArray &data);
  quint8 calculateCRC8(const QByteArray &data);
  quint16 calculateCRC16(const QByteArray &data);
  quint32 calculateCRC32(const QByteArray &data);
  quint8 calculateSum8(const QByteArray &data);
  quint16 calculateSum16(const QByteArray &data);
};

// 动态段配置行控件
class ProtocolPartWidget : public QWidget {
  Q_OBJECT
public:
  explicit ProtocolPartWidget(const ProtocolDynamicSegment &seg = ProtocolDynamicSegment(), QWidget *parent = nullptr);
  ProtocolDynamicSegment getSegment() const;

signals:
  void removeRequested(ProtocolPartWidget *widget);
  void moveUpRequested(ProtocolPartWidget *widget);
  void moveDownRequested(ProtocolPartWidget *widget);

private slots:
  void onTypeChanged(int index);

private:
  class QComboBox *m_comboType;
  class QStackedWidget *m_stackedParams;

  // Params widgets
  class QLineEdit *m_editValue;        // HEX输入框：适用于 头、尾、数值、功能码、预设数据
  class QComboBox *m_comboSize;        // 占用字节数：适用于 长度、序号 
  class QComboBox *m_comboLengthRange; // 长度包含范围
  class QComboBox *m_comboChecksumAlgo;// 校验算法
};

// 协议配置对话框
class ProtocolConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit ProtocolConfigDialog(QWidget *parent = nullptr);

  void setConfig(const ProtocolConfig &config);
  ProtocolConfig getConfig() const;

private slots:
  void onModeChanged(int index);
  void onAddSegmentClicked();

private:
  class QComboBox *m_comboMode;
  class QStackedWidget *m_mainStackedWidget;

  // Mode 1 UI
  class QLineEdit *m_editName;
  class QLineEdit *m_editFrameHeader;
  class QCheckBox *m_chkUseHeader;
  class QLineEdit *m_editFrameTail;
  class QCheckBox *m_chkUseTail;
  class QCheckBox *m_chkUseLength;
  class QSpinBox *m_spinLengthPos;
  class QSpinBox *m_spinLengthSize;
  class QCheckBox *m_chkLengthIncludesHeader;
  class QCheckBox *m_chkUseSequence;
  class QSpinBox *m_spinSequencePos;
  class QCheckBox *m_chkUseFunctionCode;
  class QSpinBox *m_spinFunctionCodePos;
  class QLineEdit *m_editFunctionCode;
  class QTextEdit *m_editDataPayload;
  class QComboBox *m_comboChecksumType;
  class QSpinBox *m_spinChecksumPos;

  // 循环发送配置
  class QCheckBox *m_chkAutoSendLoop;
  class QSpinBox *m_spinLoopInterval;
  class QLineEdit *m_editNameCommon; // 独立于模式的名称输入框
  
  // Mode 2 UI
  class QVBoxLayout *m_dynamicListLayout;
  QVector<ProtocolPartWidget *> m_partWidgets;
};

#endif // CUSTOMWIDGET_H
