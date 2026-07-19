#ifndef DEVICEMONITORPANEL_H
#define DEVICEMONITORPANEL_H

#include <QHash>
#include <QList>
#include <QWidget>

#include "canprotocolconfigdialog.h"
#include "caninterface.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class QGridLayout;
class QLabel;
class QPlainTextEdit;
class QScrollArea;
class QPushButton;
class CanInterface;
class DeviceStatusWidget;

// 科技风设备状态监控面板 —— 嵌入 CANTool 标签页。
// 共享 CANTool 的 CanInterface 实例，接收 CAN 帧后实时更新设备状态。
class DeviceMonitorPanel : public QWidget {
  Q_OBJECT
public:
  explicit DeviceMonitorPanel(CanInterface *can, QWidget *parent = nullptr);
  ~DeviceMonitorPanel() override;

signals:
  void logMessage(const QString &msg);

public slots:
  void onFrameReceived(const CanFrame &frame);

private slots:
  void onConfigClicked();
  void onImportClicked();
  void onExportClicked();
  void onResetClicked();
  void onNewConnection();
  void onClientDisconnected();
protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void rebuildGrid();
  void appendLog(const QString &text, bool isAlarm = false);
  void loadConfig();
  void saveConfig();
  void setupUi();
  void sendConfigToClient(QWebSocket *client);
  void broadcastMessage(const QJsonObject &json);

  CanInterface *m_can;
  QList<DeviceBitMapping> m_mappings;

  // UI
  QPushButton *m_btnConfig;
  QPushButton *m_btnImport;
  QPushButton *m_btnExport;
  QPushButton *m_btnReset;
  QScrollArea *m_scrollArea;
  QWidget *m_gridContainer;
  QGridLayout *m_gridLayout;
  QPlainTextEdit *m_log;

  QHash<int, DeviceStatusWidget *> m_deviceWidgets;
  int m_gridCols = 5;
  int m_frameCount = 0;

  // WebSocket Server
  QWebSocketServer *m_wsServer = nullptr;
  QList<QWebSocket *> m_clients;
};

#endif // DEVICEMONITORPANEL_H
