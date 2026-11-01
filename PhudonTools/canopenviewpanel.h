#ifndef CANOPENVIEWPANEL_H
#define CANOPENVIEWPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include "canopenmaster.h"
#include "caninterface.h"

class CanOpenViewPanel : public QWidget {
  Q_OBJECT
public:
  CanOpenViewPanel(CanOpenMaster *co, CanInterface *can, int viewId, QWidget *parent = nullptr);
  ~CanOpenViewPanel() override;

  void applyThemeStyle(const QString &qss);
  int viewId() const { return m_viewId; }

signals:
  void closeRequested(CanOpenViewPanel *panel);

private slots:
  void onNmtSend();
  void onSdoRead();
  void onSdoWrite();
  void onSyncSend();
  void onHeartbeat(quint8 nodeId, NmtState state);
  void onEmcy(quint8 nodeId, quint16 errorCode, quint8 errorRegister, const QByteArray &manufacturer);
  void onPdo(quint8 nodeId, int pdoNumber, bool isTpdo, const QByteArray &data);
  void onSdoReadFinished(bool success, quint16 index, quint8 subIndex, quint32 value, quint32 abortCode);
  void onSdoWriteFinished(bool success, quint16 index, quint8 subIndex, quint32 abortCode);
  void onCanOpenLog(const QString &message);

private:
  void setupUi();
  void appendCanOpenLog(const QString &text);

  CanOpenMaster *m_co;
  CanInterface *m_can;
  int m_viewId;

  // Header UI
  QWidget *m_headerWidget;
  QLabel *m_titleLabel;
  QPushButton *m_closeBtn;

  // NMT UI
  QSpinBox *m_nodeIdSpin;
  QComboBox *m_nmtCommandCombo;
  QPushButton *m_nmtSendButton;
  QPushButton *m_syncButton;

  // SDO UI
  QLineEdit *m_sdoIndexEdit;
  QLineEdit *m_sdoSubIndexEdit;
  QLineEdit *m_sdoValueEdit;
  QComboBox *m_sdoSizeCombo;
  QPushButton *m_sdoReadButton;
  QPushButton *m_sdoWriteButton;

  // Node status UI
  QTreeWidget *m_nodeTreeWidget;

  // Log UI
  QPlainTextEdit *m_canOpenLog;
};

#endif // CANOPENVIEWPANEL_H
