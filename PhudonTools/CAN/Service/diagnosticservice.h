#ifndef DIAGNOSTICSERVICE_H
#define DIAGNOSTICSERVICE_H

#include <QObject>
#include <QTimer>
#include "../Engine/udsengine.h"
#include "../Engine/knowledgeengine.h"

struct DiagnosticDtcItem {
  QString code;
  quint8 statusMask = 0;
  QString category;
  QString title;
  QString description;
};

class DiagnosticService : public QObject {
  Q_OBJECT
public:
  explicit DiagnosticService(UdsEngine *uds, KnowledgeEngine *knowledge, QObject *parent = nullptr);
  ~DiagnosticService() override;

  void setUdsEngine(UdsEngine *uds);

  // 会话控制
  bool setSession(quint8 sessionType);

  // 会话保持 (TesterPresent 0x3E)
  void setTesterPresentEnabled(bool enabled, int intervalMs = 2000);
  bool isTesterPresentEnabled() const { return m_testerPresentTimer->isActive(); }

  // 故障码操作
  bool queryDtcList(quint8 mask = 0x09);
  bool clearAllDtc();

  // DID 操作
  bool readDid(quint16 did);
  bool writeDid(quint16 did, const QByteArray &data);

signals:
  void dtcListUpdated(const QList<DiagnosticDtcItem> &dtcs);
  void didDataReceived(quint16 did, const QByteArray &data, const QString &hexStr);
  void diagnosticMessage(const QString &msg);
  void errorOccurred(const QString &err);

private slots:
  void onTesterPresentTimeout();
  void onUdsResponse(const UdsResponse &resp);

private:
  UdsEngine *m_uds = nullptr;
  KnowledgeEngine *m_knowledge = nullptr;
  QTimer *m_testerPresentTimer = nullptr;
  quint16 m_lastRequestedDid = 0;
};

#endif // DIAGNOSTICSERVICE_H
