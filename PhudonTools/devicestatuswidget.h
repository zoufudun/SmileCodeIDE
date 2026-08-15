#ifndef DEVICESTATUSWIDGET_H
#define DEVICESTATUSWIDGET_H

#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QWidget>

// 科技风设备状态指示卡片 —— 使用 QPainter 绘制探测器/阀门图标，
// 深色背景 + 霓虹色彩 + 发光边框，报警时脉冲闪烁。
class DeviceStatusWidget : public QWidget {
  Q_OBJECT
public:
  enum DeviceKind {
    Detector,            // 烟温探测器
    Valve,               // 控制分配阀 (保留兼容)
    ValveDistributor,    // 分配阀
    ValveZone,           // 区域阀
    ValveMainIsolation,  // 总管隔离阀
    ManualAlarm,         // 手动报警按钮
    GasCylinder,         // 1301气体钢瓶
    WaterPump,           // 水泵
    PressureSwitch,      // 压力开关
    MobileSprayGun       // 移动喷枪
  };

  explicit DeviceStatusWidget(int deviceId, DeviceKind kind,
                              const QString &label, quint32 canId = 0, QWidget *parent = nullptr);

  int deviceId() const { return m_deviceId; }
  DeviceKind deviceKind() const { return m_kind; }
  QString label() const { return m_label; }
  bool status() const { return m_status; }
  quint32 canId() const { return m_canId; }
  int defaultVal() const { return m_defaultVal; }

  void setDraggable(bool enable) { m_draggable = enable; }
  void startDragging(const QPoint &globalPos);
  void setLabel(const QString &label);
  void setCanId(quint32 canId);
  void setDeviceKind(DeviceKind kind);
  void setDefaultVal(int val);

  // 图标风格选择（全局静态，所有 widget 共享）
  static int iconStyle() { return s_iconStyle; }
  static void setIconStyle(int style);
  void invalidateCache();

signals:
  void deviceDragged(int deviceId, const QPoint &newPos);
  void deviceDragging(int deviceId, const QPoint &newPos);
  void deviceDragFinished(int deviceId);
  void dragStartedFromDock(int deviceId, const QPoint &globalPos);
  void editRequested(int deviceId);

public slots:
  void setStatus(bool value);

protected:
  void paintEvent(QPaintEvent *event) override;
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

  QSize sizeHint() const override { return QSize(96, 106); }
  QSize minimumSizeHint() const override { return QSize(85, 95); }

private:
  void renderCache();
  void drawDetector(QPainter &p, const QRect &area);
  void drawValve(QPainter &p, const QRect &area);
  void drawValveDistributor(QPainter &p, const QRect &area);
  void drawValveZone(QPainter &p, const QRect &area);
  void drawValveMainIsolation(QPainter &p, const QRect &area);
  void drawManualAlarm(QPainter &p, const QRect &area);
  void drawGasCylinder(QPainter &p, const QRect &area);
  void drawWaterPump(QPainter &p, const QRect &area);
  void drawPressureSwitch(QPainter &p, const QRect &area);
  void drawMobileSprayGun(QPainter &p, const QRect &area);
  // 简约风格备选图标
  void drawDetectorSimple(QPainter &p, const QRect &area);
  void drawValveSimple(QPainter &p, const QRect &area);
  void drawManualAlarmSimple(QPainter &p, const QRect &area);
  void drawGasCylinderSimple(QPainter &p, const QRect &area);
  void drawWaterPumpSimple(QPainter &p, const QRect &area);
  void drawPressureSwitchSimple(QPainter &p, const QRect &area);
  void drawMobileSprayGunSimple(QPainter &p, const QRect &area);
  QColor accentColor() const;
  QColor glowColor() const;

  int m_deviceId;
  DeviceKind m_kind;
  QString m_label;
  quint32 m_canId;
  int m_defaultVal = 0;
  bool m_status = false;
  static int s_iconStyle;
  bool m_alarmPhase = false;
  bool m_hovered = false;
  QTimer *m_flashTimer;

  // 离屏渲染缓存
  QPixmap m_cachedCard;
  bool m_cacheDirty = true;

  // 拖拽
  bool m_draggable = false;
  bool m_dragActive = false;
  QPoint m_dragStartPos;
  QPoint m_dragWidgetStart;
};

#endif // DEVICESTATUSWIDGET_H
