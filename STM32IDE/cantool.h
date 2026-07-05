#ifndef CANTOOL_H
#define CANTOOL_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTreeWidget>

#include "normalsenddialog.h"
#include "canopenmaster.h"

class QPlainTextEdit;
class CanDeviceDialog;

class CANTool : public QDialog {
  Q_OBJECT
public:
  explicit CANTool(QWidget *parent = nullptr);
  ~CANTool() override;

private slots:
  void onDeviceManage();
  void onSendClicked();
  void onClearReceive();

  // Toolbar slots
  void onNewViewTriggered(QAction *action);
  void onSendDataTriggered(QAction *action);
  void onChannelUtilization();
  void onAdvancedFeaturesTriggered(QAction *action);
  void onToolsTriggered(QAction *action);
  void onSettingsHelpTriggered(QAction *action);

  void onCanConnected();
  void onCanDisconnected();
  void onCanError(const QString &message);
  void onFrameReceived(const CanFrame &frame);
  void onFrameSent(const CanFrame &frame);

  // CANopen 操作
  void onNmtSend();
  void onSdoRead();
  void onSdoWrite();
  void onSyncSend();
  void onHeartbeat(quint8 nodeId, NmtState state);
  void onEmcy(quint8 nodeId, quint16 errorCode, quint8 errorRegister,
              const QByteArray &manufacturer);
  void onPdo(quint8 nodeId, int pdoNumber, bool isTpdo,
             const QByteArray &data);
  void onSdoReadFinished(bool success, quint16 index, quint8 subIndex,
                         quint32 value, quint32 abortCode);
  void onSdoWriteFinished(bool success, quint16 index, quint8 subIndex,
                          quint32 abortCode);
  void onCanOpenLog(const QString &message);

private:
  void setupUi();
  void createToolbar();
  QIcon createToolbarIcon(int type);
  QWidget *createConnectionPanel();
  QWidget *createMonitorPanel();
  QWidget *createCanOpenPanel();
  void appendFrameRow(const CanFrame &frame, bool tx);
  void setControlsEnabled(bool connected);
  void appendCanOpenLog(const QString &text);
  void applyTheme(const QString &name);

  CanInterface *m_can;
  CanOpenMaster *m_co;

  // 连接 / 设备管理
  QPushButton *m_deviceButton;
  QComboBox *m_themeCombo;
  QLabel *m_statusLabel;
  CanDeviceDialog *m_deviceDialog = nullptr;
  QString m_currentStyle;

  // 报文监视/发送
  QTreeWidget *m_receiveTreeWidget;
  QLineEdit *m_idLineEdit;
  QLineEdit *m_dataLineEdit;
  QPushButton *m_sendButton;
  QPushButton *m_clearButton;
  QRadioButton *m_stdFrameBtn;
  QRadioButton *m_extFrameBtn;
  QCheckBox *m_remoteCheckBox;
  QCheckBox *m_fdFrameCheckBox;
  QCheckBox *m_brsCheckBox;

  // CANopen
  QSpinBox *m_nodeIdSpin;
  QComboBox *m_nmtCommandCombo;
  QPushButton *m_nmtSendButton;
  QPushButton *m_syncButton;
  QLineEdit *m_sdoIndexEdit;
  QLineEdit *m_sdoSubIndexEdit;
  QLineEdit *m_sdoValueEdit;
  QComboBox *m_sdoSizeCombo;
  QPushButton *m_sdoReadButton;
  QPushButton *m_sdoWriteButton;
  QTreeWidget *m_nodeTreeWidget;
  QPlainTextEdit *m_canOpenLog;

  QWidget *m_toolbar = nullptr;
  QPointer<NormalSendDialog> m_sendDialog = nullptr;
  QTabWidget *m_tabs = nullptr;
};

#endif // CANTOOL_H
