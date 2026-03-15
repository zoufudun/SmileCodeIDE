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

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  ProtocolConfig m_config;
  quint16 m_sequenceNumber = 0;
  class QTimer *m_autoSendTimer = nullptr;
  QPoint m_dragStartPos;

  // 校验算法实现
  quint8 calculateXOR(const QByteArray &data);
  quint8 calculateCRC8(const QByteArray &data);
  quint16 calculateCRC16(const QByteArray &data);
  quint32 calculateCRC32(const QByteArray &data);
  quint8 calculateSum8(const QByteArray &data);
  quint16 calculateSum16(const QByteArray &data);
};

// 协议配置对话框
class ProtocolConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit ProtocolConfigDialog(QWidget *parent = nullptr);

  void setConfig(const ProtocolConfig &config);
  ProtocolConfig getConfig() const;

private:
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
};

#endif // CUSTOMWIDGET_H
