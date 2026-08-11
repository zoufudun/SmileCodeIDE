#ifndef DEVICEMONITORPANEL_H
#define DEVICEMONITORPANEL_H

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QWidget>

#include "caninterface.h"
#include "canprotocolconfigdialog.h"
#include "ringbuffer.h"

class QGridLayout;
class QHBoxLayout;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QComboBox;
class QScrollArea;
class QTabBar;
class CanInterface;
class DeviceStatusWidget;
class RoomWidget;
class BottomStatusBar;
class SubMonitorWindow;

// ===== 房间区域数据结构 =====
enum RoomShape { ShapeRectangle = 0, ShapeCircle, ShapeDiamond, ShapeIrregular };
struct RoomRegion {
  QString id;
  QString name;
  QRect geom;
  int shape = ShapeRectangle; // 0=矩形, 1=圆形, 2=菱形, 3=不规则
  QString targetView = QStringLiteral("界面1"); // 所属界面/标签页名称
  bool visible = true;                          // 是否显示
  bool isLocked = false;                        // 是否固定不动 (锁定防拖拽)
  QColor color = QColor(0, 212, 255);           // 主题/边框颜色

  RoomRegion() = default;
  RoomRegion(const QString &i, const QString &n, const QRect &g, int s = ShapeRectangle, const QString &tv = QStringLiteral("界面1"), bool v = true, bool l = false, const QColor &c = QColor(0, 212, 255))
      : id(i), name(n), geom(g), shape(s), targetView(tv), visible(v), isLocked(l), color(c) {}
};

class QToolButton;
class QMenu;
class QAction;

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

  // 界面分割与多屏联动
  void onSplitConfigClicked();
  void onMultiScreenToggled(bool checked);
  void onTabChanged(int index);
  void onScreenLayoutChanged();

  // Room 界面与布局管理
  void toggleLayoutMode(bool enable);
  void showLayoutFloatingBox();
  void closeLayoutFloatingBox();
  void onDeleteRoom();
  void onAddRoom();
  void autoArrangeRoomsAndDevices();
  void onManageRoomsRequested();
  void onDeviceDragged(int deviceId, const QPoint &newPos);
  void onRoomMoved(const QString &id, const QRect &newGeom);
  void onRoomResized(const QString &id, const QRect &newGeom);
  void onRoomLockToggled(const QString &id, bool locked);
  void processBatch();
  void onEditDeviceRequested(int deviceId);
  void toggleFullScreen();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  void rebuildRoomCanvas();
  void buildMappingHash();
  void appendLog(const QString &text, bool isAlarm = false);
  void loadConfig();
  void saveConfig();
  void setupUi();
  void updateTabBar();
  void drawTemplateBackground(QPainter &p, const QRect &rect, const QString &tpl);
  void updateSubWindows();
  void scheduleSubWindowUpdate(); // Debounced: always defers and coalesces multiple calls
  void sendConfigToClient(QWebSocket *client);
  void broadcastMessage(const QJsonObject &json);
  void repositionFloatingWidgets();

  // Room 辅助
  void saveRoomLayout();
  void loadRoomLayout();
  void applyLayoutTemplate(const QString &tpl);
  void autoArrangeRoomDevices();
  void zoomIn();
  void zoomOut();
  void zoomFit();
  void updateZoom();
  void updateUnplacedDock();
  void clearUnplacedDock();
  void placeDeviceOnCanvas(int deviceId);
  RoomRegion *roomAtPos(const QPoint &pos);

  CanInterface *m_can;
  QList<DeviceBitMapping> m_mappings;
  QHash<quint32, QList<int>> m_canIdToMappingIndices;

  // UI
  QWidget *m_toolbar = nullptr;
  BottomStatusBar *m_bottomBar = nullptr;
  QToolButton *m_btnSettingsMenu = nullptr; // ⚙ 设置菜单按钮
  QPushButton *m_btnLayoutToggle = nullptr; // 📐 布局/视图 切换按钮
  QDialog *m_layoutFloatingDialog = nullptr; // 布局模式悬浮控制框
  QWidget *m_layoutFloatingTitleBar = nullptr; // 悬浮控制框标题栏
  bool m_layoutFloatingDragging = false;
  QPoint m_layoutFloatingDragStartPos;
  bool m_layoutEditingEnabled = false;       // 使能布局状态 (默认 false / 视图模式)
  QAction *m_actMultiScreenToggle = nullptr;
  QPushButton *m_btnConfig;
  QPushButton *m_btnImport;
  QPushButton *m_btnExport;
  QPushButton *m_btnReset;
  QPushButton *m_btnAddRoom;
  QPushButton *m_btnFullScreen = nullptr;
  bool m_isFullScreen = false;
  QComboBox *m_templateCombo;
  QScrollArea *m_scrollArea;
  QWidget *m_gridContainer;
  QGridLayout *m_gridLayout;
  QPlainTextEdit *m_log;
  QWidget *m_logWrapper = nullptr;
  QWidget *m_logTitleBar = nullptr;
  QPushButton *m_btnInfoLog = nullptr;
  QPushButton *m_btnToggleLogSize = nullptr;
  bool m_logExpanded = false;
  QLabel *m_lblCount = nullptr;

  // 界面分割与多屏联动 UI
  QWidget *m_floatingTabWrapper = nullptr;
  QTabBar *m_viewTabBar = nullptr;
  QPushButton *m_btnSplitConfig = nullptr;
  QPushButton *m_btnMultiScreen = nullptr;
  QStringList m_viewNames;
  int m_activeViewIndex = 0;
  bool m_multiScreenActive = false;
  bool m_updatingSubWindows = false;   // 重入保护守卫
  bool m_isRebuildingCanvas = false;   // 画布重建重入保护守卫
  QList<QPointer<SubMonitorWindow>> m_subWindows;
  QTimer *m_subWinUpdateTimer = nullptr; // Debounce timer - coalesces multiple updateSubWindows requests

  bool m_logDragging = false;
  QPoint m_logDragStartPos;

  // 未摆放设备停靠区
  QWidget *m_unplacedDock = nullptr;
  QWidget *m_unplacedContainer = nullptr;
  QHBoxLayout *m_unplacedLayout = nullptr;

  QHash<int, DeviceStatusWidget *> m_deviceWidgets;
  int m_gridCols = 5;
  int m_frameCount = 0;

  LockFreeRingBuffer<CanFrame, 16384> m_ringBuffer;
  QTimer *m_batchTimer = nullptr;

  // Room 布局模式数据
  QList<RoomRegion> m_rooms;
  QHash<QString, RoomWidget *> m_roomWidgets;
  QHash<int, QPoint> m_deviceRoomPos;
  QString m_activeTemplate;
  qreal m_zoomLevel = 1.0;
  int m_baseCanvasW = 1600, m_baseCanvasH = 1200; // deviceId → 画布坐标

  // WebSocket
  QWebSocketServer *m_wsServer = nullptr;
  QList<QWebSocket *> m_clients;
};

#endif // DEVICEMONITORPANEL_H
