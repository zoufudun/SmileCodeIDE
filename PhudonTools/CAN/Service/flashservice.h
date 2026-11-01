#ifndef FLASHSERVICE_H
#define FLASHSERVICE_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QTimer>
#include "../Engine/udsengine.h"

// 刷写段结构
struct FlashSegment {
  quint32 address = 0;
  QByteArray data;
};

// 刷写工程配置
struct FlashConfig {
  quint32 targetAddress = 0x08008000; // 默认 Flash 起始地址
  int blockSize = 512;               // 传输块大小 (字节)
  quint8 securityLevel = 0x01;        // 安全访问级别
  QByteArray securityKey;            // 解锁密钥 (或者固定密钥)
  bool needErase = true;             // 是否需要擦除
  bool needReset = true;             // 刷写完成是否重启
  quint16 eraseRoutineId = 0xFF00;   // 擦除例程 ID
  quint16 checkRoutineId = 0x0202;   // 校验例程 ID
};

// 刷写状态
enum class FlashStep {
  Idle,
  EnterExtendedSession,
  SecurityAccessSeed,
  SecurityAccessKey,
  EnterProgrammingSession,
  EraseFlash,
  RequestDownload,
  TransferData,
  RequestTransferExit,
  VerifyChecksum,
  EcuReset,
  Completed,
  Failed
};

class FlashService : public QObject {
  Q_OBJECT
public:
  explicit FlashService(UdsEngine *uds, QObject *parent = nullptr);
  ~FlashService() override;

  void setUdsEngine(UdsEngine *uds);
  void setConfig(const FlashConfig &cfg);
  FlashConfig config() const { return m_cfg; }

  // 加载固件文件 (支持 Bin / Hex / S19)
  bool loadFirmwareFile(const QString &filePath);
  bool setFirmwareData(quint32 baseAddr, const QByteArray &data);
  QList<FlashSegment> segments() const { return m_segments; }
  quint32 totalFirmwareBytes() const;

  // 刷写任务控制
  bool startFlashing();
  void abortFlashing();
  bool isFlashing() const;

signals:
  void stepChanged(FlashStep step, const QString &stepDesc);
  void progressChanged(int percent, double speedKBs, int bytesTransferred, int totalBytes);
  void flashFinished(bool success, const QString &message);
  void logMessage(const QString &msg);

private slots:
  void onUdsResponseReceived(const UdsResponse &resp);
  void onUdsTimeout(quint8 sid);
  void onUdsFailed(quint8 sid, const QString &err);

private:
  void proceedNextStep();
  void sendTransferDataChunk();
  void finishFlashing(bool ok, const QString &msg);

  // 文件解析辅助
  bool parseIntelHex(const QString &content);
  bool parseMotorolaS19(const QString &content);

  UdsEngine *m_uds = nullptr;
  FlashConfig m_cfg;
  QList<FlashSegment> m_segments;

  FlashStep m_currentStep = FlashStep::Idle;
  int m_segmentIndex = 0;
  int m_segmentOffset = 0;
  quint8 m_blockSeq = 1;
  int m_totalBytes = 0;
  int m_transferredBytes = 0;
  qint64 m_startTime = 0;
};

#endif // FLASHSERVICE_H
