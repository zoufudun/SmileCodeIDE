#ifndef DEVICEMONITORPANEL_H
#define DEVICEMONITORPANEL_H

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QRect>
#include <QString>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QWidget>

#include "caninterface.h"
#include "canprotocolconfigdialog.h"

class QGridLayout;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class CanInterface;
class DeviceStatusWidget;
class RoomWidget;

// ===== 房间区域数据结构 =====
struct RoomRegion {
  QString id;
  QString name;
  QRect geom; // x, y, w, h (画布坐标)
};

// 科技风设备状态监控面板 —— 嵌入 CANTool 标签页。
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

  // Room 模式
  void onToggleRoomMode();
  void onAddRoom();
  void onRenameRoom();
  void onDeleteRoom();
  void onDeviceDragged(int deviceId, const QPoint &newPos);
  void onRoomMoved(const QString &id, const QRect &newGeom);
  void onRoomResized(const QString &id, const QRect &newGeom);

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void rebuildGrid();
  void rebuildRoomCanvas();
  void appendLog(const QString &text, bool isAlarm = false);
  void loadConfig();
  void saveConfig();
  void setupUi();
  void sendConfigToClient(QWebSocket *client);
  void broadcastMessage(const QJsonObject &json);

  // Room 辅助
  void saveRoomLayout();
  void loadRoomLayout();
  RoomRegion *roomAtPos(const QPoint &pos);

  CanInterface *m_can;
  QList<DeviceBitMapping> m_mappings;

  // UI
  QPushButton *m_btnConfig;
  QPushButton *m_btnImport;
  QPushButton *m_btnExport;
  QPushButton *m_btnReset;
  QPushButton *m_btnToggleRoom;
  QPushButton *m_btnAddRoom;
  QPushButton *m_btnRenameRoom;
  QPushButton *m_btnDeleteRoom;
  QScrollArea *m_scrollArea;
  QWidget *m_gridContainer;
  QGridLayout *m_gridLayout;
  QPlainTextEdit *m_log;
  QLabel *m_lblCount = nullptr;

  QHash<int, DeviceStatusWidget *> m_deviceWidgets;
  int m_gridCols = 5;
  int m_frameCount = 0;

  // Room 模式
  bool m_roomMode = false;
  QList<RoomRegion> m_rooms;
  QList<RoomWidget *> m_roomWidgets;
  QHash<int, QPoint> m_deviceRoomPos; // deviceId → 画布坐标

  // WebSocket
  QWebSocketServer *m_wsServer = nullptr;
  QList<QWebSocket *> m_clients;
};

#endif // DEVICEMONITORPANEL_H
