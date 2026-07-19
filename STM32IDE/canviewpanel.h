#ifndef CANVIEWPANEL_H
#define CANVIEWPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include "caninterface.h"

class CanViewPanel : public QWidget {
  Q_OBJECT
public:
  CanViewPanel(CanInterface *can, int viewId, QWidget *parent = nullptr);
  ~CanViewPanel() override;

  void applyThemeStyle(const QString &qss);
  int viewId() const { return m_viewId; }

signals:
  void closeRequested(CanViewPanel *panel);

private slots:
  void onSendClicked();
  void onClearClicked();
  void onFrameReceived(const CanFrame &frame);
  void onFrameSent(const CanFrame &frame);
  void onDeviceConnected();
  void onDeviceDisconnected();
  void refreshChannels();

private:
  void setupUi();
  void appendFrameRow(const CanFrame &frame, bool tx);

  CanInterface *m_can;
  int m_viewId;
  bool m_paused = false;

  // Header UI
  QWidget *m_headerWidget;
  QLabel *m_titleLabel;
  QPushButton *m_closeBtn;

  // Top Bar UI
  QComboBox *m_channelCombo;
  QPushButton *m_btnSaveRealtime;
  QPushButton *m_btnSave;
  QPushButton *m_btnClear;
  QPushButton *m_btnPause;
  QCheckBox *m_chkClassify;
  QPushButton *m_btnSetting;

  // Monitor UI
  QTreeWidget *m_receiveTreeWidget;

  // Sending UI
  QLineEdit *m_idLineEdit;
  QLineEdit *m_dataLineEdit;
  QPushButton *m_sendButton;
  QPushButton *m_clearButton;
  QRadioButton *m_stdFrameBtn;
  QRadioButton *m_extFrameBtn;
  QCheckBox *m_remoteCheckBox;
  QCheckBox *m_fdFrameCheckBox;
  QCheckBox *m_brsCheckBox;

  // Bottom UI
  QCheckBox *m_chkShowError;
  QLabel *m_lblRxCount;
  QLabel *m_lblTxCount;

  int m_rxCount = 0;
  int m_txCount = 0;
};

#endif // CANVIEWPANEL_H
