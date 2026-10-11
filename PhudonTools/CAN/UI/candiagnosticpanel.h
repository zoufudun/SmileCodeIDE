#ifndef CANDIAGNOSTICPANEL_H
#define CANDIAGNOSTICPANEL_H

#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QComboBox>
#include "caninterface.h"
#include "../CAN/Engine/isotpengine.h"
#include "../CAN/Engine/udsengine.h"
#include "../CAN/Engine/knowledgeengine.h"
#include "../CAN/Service/diagnosticservice.h"

class CanDiagnosticPanel : public QWidget {
  Q_OBJECT
public:
  explicit CanDiagnosticPanel(CanInterface *can, QWidget *parent = nullptr);
  ~CanDiagnosticPanel() override;

  void applyThemeStyle(const QString &qss);

signals:
  void closeRequested(QWidget *panel);
  void requestAiDiagnosticReport(const QStringList &dtcCodes);

private slots:
  void onSessionControlClicked(int type);
  void onTesterPresentToggled(bool checked);
  void onReadDtcClicked();
  void onClearDtcClicked();
  void onReadDidClicked();
  void onSendCustomUdsClicked();
  void onAiReportClicked();

  void onDtcListUpdated(const QList<DiagnosticDtcItem> &dtcs);
  void onDiagnosticMessage(const QString &msg);

private:
  void setupUi();
  void log(const QString &msg);

  CanInterface *m_can = nullptr;
  IsoTpEngine *m_isoTp = nullptr;
  UdsEngine *m_uds = nullptr;
  KnowledgeEngine *m_knowledge = nullptr;
  DiagnosticService *m_diagService = nullptr;

  QWidget *m_headerWidget = nullptr;
  QLabel *m_titleLabel = nullptr;
  QPushButton *m_closeBtn = nullptr;

  // 诊断参数
  QLineEdit *m_editTxId = nullptr;
  QLineEdit *m_editRxId = nullptr;
  QCheckBox *m_chkTesterPresent = nullptr;

  // DTC 树
  QTreeWidget *m_dtcTree = nullptr;
  QPushButton *m_btnReadDtc = nullptr;
  QPushButton *m_btnClearDtc = nullptr;
  QPushButton *m_btnAiReport = nullptr;

  // DID
  QComboBox *m_comboDid = nullptr;
  QPushButton *m_btnReadDid = nullptr;
  QLineEdit *m_editDidResult = nullptr;

  // 自定义
  QLineEdit *m_editCustomHex = nullptr;
  QPushButton *m_btnSendCustom = nullptr;

  // 日志
  QPlainTextEdit *m_logEdit = nullptr;

  QStringList m_currentDtcCodes;
};

#endif // CANDIAGNOSTICPANEL_H
