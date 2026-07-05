#ifndef CANDEVICEDIALOG_H
#define CANDEVICEDIALOG_H

#include <QDialog>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class CanInterface;

// 设备管理窗口：配置 ZLG USBCANFD 设备型号、通道、波特率等参数，
// 并完成设备的打开/启动与关闭。操作共享的 CanInterface 实例，
// 设备状态变化通过 CanInterface 的信号反馈到主界面。
class CanDeviceDialog : public QDialog {
  Q_OBJECT
public:
  explicit CanDeviceDialog(CanInterface *can, QWidget *parent = nullptr);

private slots:
  void onOpenCloseClicked();
  void onDiagnoseClicked();
  void onDeviceConnected();
  void onDeviceDisconnected();
  void onDeviceError(const QString &message);

private:
  void setupUi();
  void refreshState();

  CanInterface *m_can;

  QComboBox *m_deviceTypeCombo;
  QSpinBox *m_deviceIndexSpin;
  QComboBox *m_channelCombo;
  QComboBox *m_baudCombo;
  QCheckBox *m_fdCheck;
  QComboBox *m_dataBaudCombo;
  QComboBox *m_modeCombo;
  QCheckBox *m_termResCheck;
  QPushButton *m_openButton;
  QPushButton *m_diagButton;
  QLabel *m_statusLabel;
};

#endif // CANDEVICEDIALOG_H
