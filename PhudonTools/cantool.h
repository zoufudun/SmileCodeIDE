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
#include <QSplitter>

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
  void onStatusMonitorClicked();

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

private:
  void setupUi();
  void createToolbar();
  QIcon createToolbarIcon(int type);
  void applyTheme(const QString &name);

  CanInterface *m_can;
  CanOpenMaster *m_co;

  // 连接 / 设备管理
  QPushButton *m_deviceButton;
  QToolButton *m_btnTheme = nullptr;
  QLabel *m_statusLabel;
  CanDeviceDialog *m_deviceDialog = nullptr;
  QString m_currentStyle;

  QWidget *m_toolbar = nullptr;
  QPointer<NormalSendDialog> m_sendDialog = nullptr;
  QSplitter *m_splitter = nullptr;
  QPointer<QDialog> m_monitorDialog = nullptr;
  QPointer<QDialog> m_busUtilizationDialog = nullptr;
};

#endif // CANTOOL_H
