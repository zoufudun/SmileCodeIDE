#ifndef CANDEVICEDIALOG_H
#define CANDEVICEDIALOG_H

#include <QDialog>
#include <QMap>

class QComboBox;
class QLineEdit;
class QLabel;
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
  void applyThemeStyle(const QString &qss);

private slots:
  void onOpenDeviceClicked();
  void onCloseDeviceClicked();
  void onStartChannelClicked(int channel);
  void onStopChannelClicked(int channel);
  void onFilterSettingsClicked(int channel);
  void onStartAllChannels();
  void onStopAllChannels();
  void onShowDeviceInfoClicked();
  void onCloudDeviceClicked();
  void onRefreshClicked();
  void onExpandAllClicked();
  void onCollapseAllClicked();
  void onCopyConfigClicked();
  void onTreeDeviceActivated(QTreeWidgetItem *item, int column);
  void onSearchTextChanged(const QString &text);
  void onVendorFilterChanged(int index);
  void onDetectDevicesClicked();

private:
  void setupUi();
  void refreshDeviceTree();
  void updateButtonStates();
  void populateDeviceTypeCombo(int vendorFilterIdx = 0);
  void detectAndPromptDevices(bool autoSelectFirstHardware = true);
  QString vendorForDevice(const QString &deviceName, quint32 deviceType) const;
  bool deviceMatchesFilter(const QString &vendor, const QString &deviceName) const;
  void updateSummary(int totalDevices, int visibleDevices, int runningChannels);

  CanInterface *m_can;
  QString m_currentStyle;

  QComboBox *m_deviceTypeCombo;
  QComboBox *m_vendorFilterCombo;
  QLineEdit *m_searchEdit;
  QLabel *m_summaryLabel;
  QLabel *m_detectBannerLabel;
  QSpinBox *m_deviceIndexSpin;
  QPushButton *m_openButton;
  QPushButton *m_detectButton;
  QPushButton *m_cloudButton;
  QPushButton *m_closeButton;
  QPushButton *m_refreshButton;
  QPushButton *m_expandButton;
  QPushButton *m_collapseButton;
  QPushButton *m_copyConfigButton;

  QTreeWidget *m_deviceTree;

  // 跟踪各通道的控制按钮
  QMap<int, QPushButton*> m_startButtons;
  QMap<int, QPushButton*> m_stopButtons;
  QMap<int, QPushButton*> m_filterButtons;

  // 跟踪设备的控制按钮
  QPushButton *m_deviceStartButton;
  QPushButton *m_deviceStopButton;
  QPushButton *m_deviceCloseButton;
  QPushButton *m_deviceInfoButton;
};


#endif // CANDEVICEDIALOG_H
