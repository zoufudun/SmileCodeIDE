#ifndef CANPROTOCOLMONITOR_H
#define CANPROTOCOLMONITOR_H

#include <QDialog>
#include <QHash>
#include <QList>

#include "canprotocolconfigdialog.h"
#include "caninterface.h"

class QGridLayout;
class QLabel;
class QPlainTextEdit;
class QScrollArea;
class QPushButton;
class CanInterface;
class CanDeviceDialog;
class DeviceStatusWidget;

// CAN 2.0B 自定义协议设备状态监控面板。
// 内置 CanInterface 实例，接收 CAN 帧后根据配置的位映射实时更新设备状态图标。
class CanProtocolMonitor : public QDialog {
  Q_OBJECT
public:
  explicit CanProtocolMonitor(QWidget *parent = nullptr);
  ~CanProtocolMonitor() override;

signals:
  void closed();

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void onDeviceManage();
  void onFrameReceived(const CanFrame &frame);
  void onCanConnected();
  void onCanDisconnected();
  void onConfigClicked();
  void onImportClicked();
  void onExportClicked();
  void onClearClicked();
  void processBatch();

private:
  void rebuildGrid();
  void appendLog(const QString &text, bool isAlarm = false);
  void loadConfig();
  void saveConfig();

  CanInterface *m_can;
  CanDeviceDialog *m_devDialog = nullptr;
  QList<DeviceBitMapping> m_mappings;

  // UI 组件
  QLabel *m_connStatus;
  QPushButton *m_btnDev;
  QPushButton *m_btnConfig;
  QPushButton *m_btnImport;
  QPushButton *m_btnExport;
  QPushButton *m_btnClear;
  QScrollArea *m_scrollArea;
  QWidget *m_gridContainer;
  QGridLayout *m_gridLayout;
  QPlainTextEdit *m_log;

  // deviceId → widget 映射
  QHash<int, DeviceStatusWidget *> m_deviceWidgets;

  int m_gridCols = 5;

  QVector<CanFrame> m_pendingFrames;
  QTimer *m_batchTimer = nullptr;
};

#endif // CANPROTOCOLMONITOR_H
