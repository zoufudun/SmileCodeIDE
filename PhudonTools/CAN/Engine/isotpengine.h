#ifndef ISOTPENGINE_H
#define ISOTPENGINE_H

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QTimer>
#include "../Adapter/icanadapter.h"

// ISO 15765-2 传输层参数
struct IsoTpConfig {
  quint32 txId = 0x7E0;
  quint32 rxId = 0x7E8;
  int channel = 0;
  bool isFd = false;
  quint8 blockSize = 8;        // 流控块大小 (0 表示不限制)
  quint8 stMin = 10;           // 连续帧最小间隔 (ms)
  int timeoutAr = 1000;        // 发送超时 (ms)
  int timeoutBs = 1000;        // 等待流控超时 (ms)
  int timeoutCr = 1000;        // 等待连续帧超时 (ms)
};

class IsoTpEngine : public QObject {
  Q_OBJECT
public:
  explicit IsoTpEngine(ICanAdapter *adapter, QObject *parent = nullptr);
  ~IsoTpEngine() override;

  void setAdapter(ICanAdapter *adapter);
  void setConfig(const IsoTpConfig &cfg);
  IsoTpConfig config() const { return m_cfg; }

  // 异步发送完整的多字节数据 (如 UDS 请求)
  bool sendData(const QByteArray &data);

  // 重置传输状态
  void reset();

signals:
  void dataReceived(const QByteArray &data);
  void transmitProgress(int bytesSent, int totalBytes);
  void transmitCompleted(bool success);
  void errorOccurred(const QString &msg);

private slots:
  void onFrameReceived(const CanRawFrame &frame);
  void onSendNextConsecutiveFrame();
  void onTimeout();

private:
  void handleSingleFrame(const CanRawFrame &frame);
  void handleFirstFrame(const CanRawFrame &frame);
  void handleConsecutiveFrame(const CanRawFrame &frame);
  void handleFlowControl(const CanRawFrame &frame);

  void sendFlowControl(quint8 flowStatus);

  ICanAdapter *m_adapter = nullptr;
  IsoTpConfig m_cfg;

  // 发送上下文
  enum class TxState { Idle, WaitFlowControl, SendingConsecutive };
  TxState m_txState = TxState::Idle;
  QByteArray m_txBuffer;
  int m_txOffset = 0;
  quint8 m_txSeqNumber = 1;
  int m_txBlockRemaining = 0;
  QTimer *m_txTimer = nullptr;

  // 接收上下文
  enum class RxState { Idle, ReceivingConsecutive };
  RxState m_rxState = RxState::Idle;
  QByteArray m_rxBuffer;
  int m_rxExpectedLength = 0;
  quint8 m_rxExpectedSeq = 1;
  int m_rxBlockRemaining = 0;
  QTimer *m_rxTimer = nullptr;
};

#endif // ISOTPENGINE_H
