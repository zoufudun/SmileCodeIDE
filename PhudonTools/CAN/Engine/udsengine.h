#ifndef UDSENGINE_H
#define UDSENGINE_H

#include <QByteArray>
#include <QObject>
#include <QTimer>
#include "isotpengine.h"

// UDS 响应状态与结果
struct UdsResponse {
  bool isPositive = false;
  quint8 sid = 0;              // 原始请求 SID
  quint8 responseSid = 0;      // 响应 SID (正响应为 SID + 0x40, 负响应为 0x7F)
  quint8 nrc = 0;              // 负响应码 (NRC)
  QString nrcDescription;      // 负响应码含义描述
  QByteArray payload;          // 响应数据负载 (去除 SID/NRC 头部)
};

class UdsEngine : public QObject {
  Q_OBJECT
public:
  explicit UdsEngine(IsoTpEngine *isoTp, QObject *parent = nullptr);
  ~UdsEngine() override;

  void setIsoTpEngine(IsoTpEngine *isoTp);

  // 基础请求发送 (带超时等待响应)
  bool sendRequest(quint8 sid, const QByteArray &data = QByteArray());

  // 便捷诊断指令封装
  bool diagnosticSessionControl(quint8 sessionType); // 0x10
  bool ecuReset(quint8 resetType);                   // 0x11
  bool securityAccessRequestSeed(quint8 level);      // 0x27 01/03/05...
  bool securityAccessSendKey(quint8 level, const QByteArray &key); // 0x27 02/04/06...
  bool readDataByIdentifier(quint16 did);            // 0x22
  bool writeDataByIdentifier(quint16 did, const QByteArray &data); // 0x2E
  bool readDtcInformation(quint8 subFunction, quint8 mask = 0xFF); // 0x19
  bool clearDiagnosticInformation(quint32 dtcGroup = 0xFFFFFF);    // 0x14
  bool testerPresent(bool suppressPosResponse = false); // 0x3E
  bool routineControl(quint8 subFunction, quint16 routineId, const QByteArray &option = QByteArray()); // 0x31

  // 负响应码文本解释
  static QString nrcToString(quint8 nrc);

signals:
  void responseReceived(const UdsResponse &resp);
  void requestTimeout(quint8 sid);
  void requestFailed(quint8 sid, const QString &err);

private slots:
  void onIsoTpDataReceived(const QByteArray &data);
  void onIsoTpTransmitCompleted(bool ok);
  void onResponseTimeout();

private:
  IsoTpEngine *m_isoTp = nullptr;
  QTimer *m_responseTimer = nullptr;
  quint8 m_currentSid = 0;
  bool m_waitingResponse = false;
  int m_timeoutMs = 2000;
};

#endif // UDSENGINE_H
