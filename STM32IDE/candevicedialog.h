#ifndef CANDEVICEDIALOG_H
#define CANDEVICEDIALOG_H

#include <QDialog>
#include <QMap>

class QComboBox;
class QSpinBox;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class CanInterface;

// 重新设计的设备管理窗口，支持树状图设备管理及独立的通道参数启动配置。
class CanDeviceDialog : public QDialog {
  Q_OBJECT
public:
  explicit CanDeviceDialog(CanInterface *can, QWidget *parent = nullptr);

private slots:
  void onOpenDeviceClicked();
  void onCloseDeviceClicked();
  void onStartChannelClicked(int channel);
  void onStopChannelClicked(int channel);
  void onStartAllChannels();
  void onStopAllChannels();
  void onShowDeviceInfoClicked();
  void onCloudDeviceClicked();

private:
  void setupUi();
  void refreshDeviceTree();
  void updateButtonStates();

  CanInterface *m_can;

  QComboBox *m_deviceTypeCombo;
  QSpinBox *m_deviceIndexSpin;
  QPushButton *m_openButton;
  QPushButton *m_cloudButton;
  QPushButton *m_closeButton;

  QTreeWidget *m_deviceTree;

  // 跟踪各通道的控制按钮
  QMap<int, QPushButton*> m_startButtons;
  QMap<int, QPushButton*> m_stopButtons;

  // 跟踪设备的控制按钮
  QPushButton *m_deviceStartButton;
  QPushButton *m_deviceStopButton;
  QPushButton *m_deviceCloseButton;
  QPushButton *m_deviceInfoButton;
};

#endif // CANDEVICEDIALOG_H
