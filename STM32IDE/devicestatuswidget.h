#ifndef DEVICESTATUSWIDGET_H
#define DEVICESTATUSWIDGET_H

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

  void setDraggable(bool enable) { m_draggable = enable; }

signals:
  void deviceDragged(int deviceId, const QPoint &newPos);

public slots:
  void setStatus(bool value);

protected:
  void paintEvent(QPaintEvent *event) override;
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  QSize sizeHint() const override { return QSize(135, 160); }
  QSize minimumSizeHint() const override { return QSize(128, 155); }

private:
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
  QColor accentColor() const;
  QColor glowColor() const;

  int m_deviceId;
  DeviceKind m_kind;
  QString m_label;
  quint32 m_canId;              // CAN ID
  bool m_status = false;
  bool m_alarmPhase = false;
  bool m_hovered = false;
  qreal m_animProgress = 0.0;
  QTimer *m_flashTimer;
  QTimer *m_animTimer;

  // 拖拽
  bool m_draggable = false;
  bool m_dragActive = false;
  QPoint m_dragStartPos;
  QPoint m_dragWidgetStart;
};

#endif // DEVICESTATUSWIDGET_H
