#include "devicestatuswidget.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QDrag>
#include <QMimeData>

// 全局图标风格
int DeviceStatusWidget::s_iconStyle = 0;

namespace TechColors {
  const QColor bg(0x0F, 0x17, 0x2A);
  const QColor border(0x1E, 0x29, 0x3B);
  const QColor borderHover(0x3B, 0x4F, 0x72);
  const QColor text(0xE2, 0xE8, 0xF0);
  const QColor textDim(0x64, 0x74, 0x8B);
  const QColor accentCyan(0x00, 0xD4, 0xFF);
  const QColor green(0x10, 0xB9, 0x81);
  const QColor greenBright(0x00, 0xE6, 0x76);
  const QColor red(0xEF, 0x44, 0x44);
  const QColor amber(0xF5, 0x9E, 0x0B);
  const QColor gray(0x47, 0x55, 0x69);
}

DeviceStatusWidget::DeviceStatusWidget(int deviceId, DeviceKind kind,
                                       const QString &label, quint32 canId, QWidget *parent)
    : QWidget(parent),
      m_deviceId(deviceId), m_kind(kind), m_label(label.left(32)), m_canId(canId),
      m_flashTimer(new QTimer(this))
{
  // 紧凑尺寸策略 — 缩小占用面积
  setMinimumSize(85, 95);
  setMaximumSize(150, 160);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  setToolTip(QStringLiteral("[%1]\nID: #%2 | CAN: 0x%3")
                 .arg(m_label)
                 .arg(m_deviceId)
                 .arg(m_canId, 3, 16, QChar('0'))
                 .toUpper());
  setStyleSheet(QStringLiteral("QToolTip { color: #00D4FF; background-color: #0F172A; border: 1px solid #00D4FF; border-radius: 4px; padding: 5px 10px; font-size: 12px; font-family: 'Microsoft YaHei'; }"));

  m_flashTimer->setInterval(500);
  connect(m_flashTimer, &QTimer::timeout, this, [this]() {
    m_alarmPhase = !m_alarmPhase;
    invalidateCache();
  });
}

void DeviceStatusWidget::enterEvent(QEvent *event) {
  m_hovered = true;
  update();
  QWidget::enterEvent(event);
}

void DeviceStatusWidget::leaveEvent(QEvent *event) {
  m_hovered = false;
  update();
  QWidget::leaveEvent(event);
}

QColor DeviceStatusWidget::accentColor() const {
  if (!m_status) return TechColors::accentCyan;
  return TechColors::red;
}

QColor DeviceStatusWidget::glowColor() const {
  QColor c = accentColor();
  c.setAlpha(m_status ? (m_alarmPhase ? 100 : 30) : 40);
  return c;
}

void DeviceStatusWidget::setIconStyle(int style) {
  if (s_iconStyle != style) {
    s_iconStyle = style;
  }
}

void DeviceStatusWidget::invalidateCache() {
  m_cacheDirty = true;
  update();
}

void DeviceStatusWidget::setDefaultVal(int val) {
  if (m_defaultVal != val) {
    m_defaultVal = val;
    setStatus(val == 1);
    invalidateCache();
  }
}

void DeviceStatusWidget::setStatus(bool value) {
  if (m_status == value && m_flashTimer->isActive() == value) return;
  m_status = value;
  if (value) { m_alarmPhase = true; m_flashTimer->start(); }
  else { m_flashTimer->stop(); m_alarmPhase = false; }
  invalidateCache();
}

void DeviceStatusWidget::setLabel(const QString &label) {
  QString newLbl = label.left(32);
  if (m_label != newLbl) {
    m_label = newLbl;
    setToolTip(QStringLiteral("[%1]\nID: #%2 | CAN: 0x%3")
                   .arg(m_label)
                   .arg(m_deviceId)
                   .arg(m_canId, 3, 16, QChar('0'))
                   .toUpper());
    invalidateCache();
  }
}

void DeviceStatusWidget::setCanId(quint32 canId) {
  if (m_canId != canId) {
    m_canId = canId;
    setToolTip(QStringLiteral("[%1]\nID: #%2 | CAN: 0x%3")
                   .arg(m_label)
                   .arg(m_deviceId)
                   .arg(m_canId, 3, 16, QChar('0'))
                   .toUpper());
    invalidateCache();
  }
}

void DeviceStatusWidget::setDeviceKind(DeviceKind kind) {
  if (m_kind != kind) {
    m_kind = kind;
    invalidateCache();
  }
}

void DeviceStatusWidget::resizeEvent(QResizeEvent *event) {
  Q_UNUSED(event);
  invalidateCache();
}

void DeviceStatusWidget::contextMenuEvent(QContextMenuEvent *event) {
  Q_UNUSED(event);
  emit editRequested(m_deviceId);
}

// ===== 离屏渲染快照缓存 =====
void DeviceStatusWidget::renderCache() {
  if (width() <= 0 || height() <= 0) return;
  m_cachedCard = QPixmap(size());
  m_cachedCard.fill(Qt::transparent);

  QPainter p(&m_cachedCard);
  p.setRenderHint(QPainter::Antialiasing);

  const int w = width(), h = height();
  const QRect card(2, 2, w - 4, h - 4);
  qreal s = qMin(w / 96.0, h / 106.0);

  // ---- 卡片背景 ----
  {
    QPainterPath bgPath;
    bgPath.addRoundedRect(card, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(TechColors::bg);
    p.drawPath(bgPath);

    // 顶部微弱渐变
    QLinearGradient g(card.left(), card.top(), card.left(), card.top() + 24 * s);
    g.setColorAt(0, QColor(0x1E, 0x2A, 0x3E, 100));
    g.setColorAt(1, QColor(0x0F, 0x17, 0x2A, 0));
    p.setBrush(g);
    p.drawPath(bgPath);

    // 边框 (报警时红边，正常时常规边框)
    QColor bc = m_status ? TechColors::red : TechColors::border;
    p.setPen(QPen(bc, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(bgPath);
  }

  // ---- 图标区域 (居中在卡片中上部，无上方ID和CAN ID号) ----
  const int iconCX = card.center().x();
  const int iconCY = card.top() + 36 * s;
  QRect iconArea(iconCX - 28 * s, iconCY - 28 * s, 56 * s, 56 * s);

  // 设备图标
  if (s_iconStyle == 0) {
    switch (m_kind) {
      case Detector:            drawDetector(p, iconArea); break;
      case ValveDistributor:    drawValveDistributor(p, iconArea); break;
      case ValveZone:           drawValveZone(p, iconArea); break;
      case ValveMainIsolation:  drawValveMainIsolation(p, iconArea); break;
      case Valve:               drawValve(p, iconArea); break;
      case ManualAlarm:         drawManualAlarm(p, iconArea); break;
      case GasCylinder:         drawGasCylinder(p, iconArea); break;
      case WaterPump:           drawWaterPump(p, iconArea); break;
      case PressureSwitch:      drawPressureSwitch(p, iconArea); break;
      case MobileSprayGun:      drawMobileSprayGun(p, iconArea); break;
    }
  } else {
    switch (m_kind) {
      case Detector:            drawDetectorSimple(p, iconArea); break;
      case ValveDistributor:
      case ValveZone:
      case ValveMainIsolation:
      case Valve:               drawValveSimple(p, iconArea); break;
      case ManualAlarm:         drawManualAlarmSimple(p, iconArea); break;
      case GasCylinder:         drawGasCylinderSimple(p, iconArea); break;
      case WaterPump:           drawWaterPumpSimple(p, iconArea); break;
      case PressureSwitch:      drawPressureSwitchSimple(p, iconArea); break;
      case MobileSprayGun:      drawMobileSprayGunSimple(p, iconArea); break;
    }
  }

  // ---- LED 指示灯 (红/绿指示) ----
  const int ledY = iconCY + 30 * s;
  QColor ledCol = m_status ? TechColors::red : TechColors::green;
  {
    QPointF lc(card.center().x(), ledY);
    QColor og = ledCol; og.setAlpha(50);
    p.setPen(Qt::NoPen); p.setBrush(og);
    p.drawEllipse(lc, 5 * s, 5 * s);
    QRadialGradient lg(lc + QPointF(-0.4, -0.4), 2.4 * s);
    lg.setColorAt(0, ledCol.lighter(200));
    lg.setColorAt(0.4, ledCol);
    lg.setColorAt(1, ledCol.darker(180));
    p.setBrush(lg);
    p.drawEllipse(lc, 2.4 * s, 2.4 * s);
  }

  // ---- 设备名称 (去掉下方报警、开字样文本块) ----
  const int nameY = ledY + 6 * s;
  p.setPen(TechColors::text);
  p.setFont(QFont("Microsoft YaHei", qMax(7, qRound(8.0 * s))));
  p.drawText(QRect(card.left() + 2, nameY, card.width() - 4, qMax(16, card.bottom() - nameY - 2)),
             Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_label);

  m_cacheDirty = false;
}

// ===== 主绘制 (高速离屏贴图) =====
void DeviceStatusWidget::paintEvent(QPaintEvent *) {
  if (m_cacheDirty || m_cachedCard.size() != size()) {
    renderCache();
  }

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.drawPixmap(0, 0, m_cachedCard);

  // 悬停高亮轮廓
  if (m_hovered) {
    const QRect card(2, 2, width() - 4, height() - 4);
    QColor glow = accentColor();
    glow.setAlpha(35);
    p.setPen(QPen(glow, 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(card, 8, 8);
  }
}

// ===== 细节图标绘制函数 =====

// ===== 探测器 — 同心雷达环 + 中心 LED =====
void DeviceStatusWidget::drawDetector(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::red : TechColors::green;

  // 报警脉冲波纹
  if (m_status && m_alarmPhase) {
    p.setPen(QPen(QColor(0xEF, 0x44, 0x44, 120), 2.0 * s));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(cx, cy), 25 * s, 25 * s);
  }

  // 三同心环
  p.setPen(QPen(QColor(0x3B, 0x82, 0xF6, 60), 2.0 * s)); p.setBrush(Qt::NoBrush);
  p.drawEllipse(QPointF(cx, cy), 26 * s, 26 * s);
  p.setPen(QPen(QColor(0x3B, 0x82, 0xF6, 120), 2.0 * s));
  p.drawEllipse(QPointF(cx, cy), 18 * s, 18 * s);
  p.setPen(QPen(QColor(0x3B, 0x82, 0xF6, 180), 2.0 * s));
  p.drawEllipse(QPointF(cx, cy), 10 * s, 10 * s);

  // 中心 LED
  p.setPen(Qt::NoPen);
  QRadialGradient g(cx - 0.5, cy - 0.5, 4 * s);
  g.setColorAt(0, c.lighter(200));
  g.setColorAt(0.5, c);
  g.setColorAt(1, c.darker(150));
  p.setBrush(g);
  p.drawEllipse(QPointF(cx, cy), 3.5 * s, 3.5 * s);
}

// ===== 阀门 — 水平管道 + 垂直阀芯 + 水流粒 =====
void DeviceStatusWidget::drawValve(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  // 左管道
  QLinearGradient pg(cx - 28 * s, 0, cx + 28 * s, 0);
  pg.setColorAt(0, QColor(0x33, 0x41, 0x55));
  pg.setColorAt(0.5, QColor(0x64, 0x74, 0x8B));
  pg.setColorAt(1, QColor(0x33, 0x41, 0x55));
  p.setPen(Qt::NoPen); p.setBrush(pg);
  p.drawRoundedRect(cx - 28 * s, cy - 3 * s, 56 * s, 6 * s, 2, 2);

  // 法兰
  p.setBrush(QColor(0x47, 0x55, 0x69));
  p.drawRect(cx - 28 * s, cy - 5 * s, 3 * s, 10 * s);
  p.drawRect(cx + 25 * s, cy - 5 * s, 3 * s, 10 * s);

  // 阀体圆
  QRadialGradient vg(cx - 1, cy - 2, 9 * s);
  vg.setColorAt(0, QColor(0x64, 0x74, 0x8B));
  vg.setColorAt(0.6, QColor(0x33, 0x41, 0x55));
  vg.setColorAt(1, QColor(0x1E, 0x29, 0x3B));
  p.setBrush(vg);
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.0));
  p.drawEllipse(QPointF(cx, cy), 9 * s, 9 * s);

  // 阀芯 — 关闭垂直(琥珀) 开启水平(绿)
  p.setPen(Qt::NoPen);
  if (m_status) {
    // 水平 = 开启
    p.setBrush(QColor(0x10, 0xB9, 0x81));
    p.drawRoundedRect(cx - 7 * s, cy - 1.5 * s, 14 * s, 3 * s, 1, 1);
    // 水流指示
    for (int i = 0; i < 3; ++i) {
      qreal dx = -15 * s + i * 15 * s;
      p.setBrush(QColor(0x10, 0xB9, 0x81, 180));
      p.drawEllipse(QPointF(cx + dx, cy - 5 * s), 2.5 * s, 2.5 * s);
    }
  } else {
    // 垂直 = 关闭
    p.setBrush(TechColors::amber);
    p.drawRoundedRect(cx - 1.5 * s, cy - 7 * s, 3 * s, 14 * s, 1, 1);
  }
}

// ===== 分配阀 (蝶阀) — 薄型蝶板 + 大法兰 =====
void DeviceStatusWidget::drawValveDistributor(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;

  // 管道
  QLinearGradient pg(cx - 26 * s, 0, cx + 26 * s, 0);
  pg.setColorAt(0, QColor(0x33, 0x41, 0x55));
  pg.setColorAt(0.5, QColor(0x64, 0x74, 0x8B));
  pg.setColorAt(1, QColor(0x33, 0x41, 0x55));
  p.setPen(Qt::NoPen); p.setBrush(pg);
  p.drawRoundedRect(cx - 24 * s, cy - 3 * s, 48 * s, 6 * s, 2, 2);

  // 大法兰盘
  p.setBrush(QColor(0x47, 0x55, 0x69));
  p.setPen(QPen(QColor(0x64, 0x74, 0x8B), 1.0));
  p.drawRoundedRect(cx - 26 * s, cy - 6 * s, 4 * s, 12 * s, 1, 1);
  p.drawRoundedRect(cx + 22 * s, cy - 6 * s, 4 * s, 12 * s, 1, 1);
  // 法兰螺栓
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0x94, 0xA3, 0xB8));
  p.drawEllipse(QPointF(cx - 24 * s, cy - 3 * s), 1.2 * s, 1.2 * s);
  p.drawEllipse(QPointF(cx - 24 * s, cy + 3 * s), 1.2 * s, 1.2 * s);
  p.drawEllipse(QPointF(cx + 24 * s, cy - 3 * s), 1.2 * s, 1.2 * s);
  p.drawEllipse(QPointF(cx + 24 * s, cy + 3 * s), 1.2 * s, 1.2 * s);

  // 阀体 (圆形)
  QRadialGradient vg(cx - 1, cy - 1, 8 * s);
  vg.setColorAt(0, QColor(0x64, 0x74, 0x8B));
  vg.setColorAt(0.6, QColor(0x47, 0x55, 0x69));
  vg.setColorAt(1, QColor(0x1E, 0x29, 0x3B));
  p.setBrush(vg);
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 0.8));
  p.drawEllipse(QPointF(cx, cy), 7 * s, 7 * s);

  // 阀芯 — 蝶板
  p.setPen(Qt::NoPen);
  if (m_status) {
    // 开启 → 水平薄蝶板 + 水流
    p.setBrush(QColor(0x10, 0xB9, 0x81));
    p.drawRoundedRect(cx - 6 * s, cy - 1 * s, 12 * s, 2 * s, 0.5, 0.5);
    for (int i = 0; i < 3; ++i) {
      qreal dx = -14 * s + i * 14 * s;
      p.setBrush(QColor(0x10, 0xB9, 0x81, 180));
      p.drawEllipse(QPointF(cx + dx, cy - 4.5 * s), 1.8 * s, 1.8 * s);
    }
  } else {
    // 关闭 → 垂直蝶板
    p.setBrush(TechColors::amber);
    p.drawRoundedRect(cx - 1 * s, cy - 6 * s, 2 * s, 12 * s, 0.5, 0.5);
  }
}

// ===== 区域阀 (闸阀) — 大型闸板 + 手轮 =====
void DeviceStatusWidget::drawValveZone(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;

  // 管道
  QLinearGradient pg(cx - 26 * s, 0, cx + 26 * s, 0);
  pg.setColorAt(0, QColor(0x2D, 0x3B, 0x4F));
  pg.setColorAt(0.3, QColor(0x5B, 0x6A, 0x7E));
  pg.setColorAt(0.7, QColor(0x5B, 0x6A, 0x7E));
  pg.setColorAt(1, QColor(0x2D, 0x3B, 0x4F));
  p.setPen(Qt::NoPen); p.setBrush(pg);
  p.drawRoundedRect(cx - 26 * s, cy - 3.5 * s, 52 * s, 7 * s, 2, 2);

  // 法兰
  p.setBrush(QColor(0x47, 0x55, 0x69));
  p.drawRect(cx - 26 * s, cy - 6 * s, 3 * s, 12 * s);
  p.drawRect(cx + 23 * s, cy - 6 * s, 3 * s, 12 * s);

  // 阀体 (方形闸阀壳体)
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.2));
  QLinearGradient bodyG(0, cy - 8 * s, 0, cy + 8 * s);
  bodyG.setColorAt(0, QColor(0x47, 0x55, 0x69));
  bodyG.setColorAt(0.5, QColor(0x37, 0x41, 0x51));
  bodyG.setColorAt(1, QColor(0x2D, 0x3B, 0x4F));
  p.setBrush(bodyG);
  p.drawRoundedRect(cx - 9 * s, cy - 9 * s, 18 * s, 18 * s, 3, 3);

  // 手轮 (顶部)
  p.setPen(QPen(QColor(0x64, 0x74, 0x8B), 1.5 * s));
  p.setBrush(Qt::NoBrush);
  p.drawEllipse(QPointF(cx, cy - 12 * s), 5 * s, 2.5 * s);
  p.setPen(QPen(QColor(0x94, 0xA3, 0xB8), 1.0));
  p.drawLine(cx, cy - 14.5 * s, cx, cy - 9 * s);
  // 手轮辐条
  p.drawLine(cx - 5 * s, cy - 12 * s, cx + 5 * s, cy - 12 * s);

  // 闸板
  p.setPen(Qt::NoPen);
  if (m_status) {
    // 开启 → 闸板提升 + 水流
    p.setBrush(QColor(0x10, 0xB9, 0x81));
    p.drawRoundedRect(cx - 0.8 * s, cy - 5 * s, 1.6 * s, 3 * s, 0.5, 0.5);
    for (int i = 0; i < 3; ++i) {
      qreal dx = -14 * s + i * 14 * s;
      p.setBrush(QColor(0x10, 0xB9, 0x81, 180));
      p.drawEllipse(QPointF(cx + dx, cy - 5.5 * s), 2 * s, 2 * s);
    }
  } else {
    // 关闭 → 闸板落下
    p.setBrush(TechColors::amber);
    p.drawRoundedRect(cx - 0.8 * s, cy - 7 * s, 1.6 * s, 14 * s, 0.5, 0.5);
  }
}

// ===== 总管隔离阀 (截止阀) — 大型阀体 + 大法兰 + 大水流 =====
void DeviceStatusWidget::drawValveMainIsolation(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;

  // 主管道（更粗）
  QLinearGradient pg(cx - 30 * s, 0, cx + 30 * s, 0);
  pg.setColorAt(0, QColor(0x25, 0x33, 0x45));
  pg.setColorAt(0.3, QColor(0x55, 0x65, 0x78));
  pg.setColorAt(0.5, QColor(0x6B, 0x7B, 0x8E));
  pg.setColorAt(0.7, QColor(0x55, 0x65, 0x78));
  pg.setColorAt(1, QColor(0x25, 0x33, 0x45));
  p.setPen(Qt::NoPen); p.setBrush(pg);
  p.drawRoundedRect(cx - 28 * s, cy - 4.5 * s, 56 * s, 9 * s, 3, 3);

  // 大法兰盘 (六螺栓)
  p.setBrush(QColor(0x47, 0x55, 0x69));
  p.setPen(QPen(QColor(0x5B, 0x6A, 0x7E), 1.0));
  p.drawRoundedRect(cx - 30 * s, cy - 7 * s, 5 * s, 14 * s, 2, 2);
  p.drawRoundedRect(cx + 25 * s, cy - 7 * s, 5 * s, 14 * s, 2, 2);
  // 螺栓
  p.setPen(Qt::NoPen);
  for (int y = -4; y <= 4; y += 4) {
    p.setBrush(QColor(0x94, 0xA3, 0xB8));
    p.drawEllipse(QPointF(cx - 27.5 * s, cy + y * s), 1.3 * s, 1.3 * s);
  }
  for (int y = -4; y <= 4; y += 4) {
    p.setBrush(QColor(0x94, 0xA3, 0xB8));
    p.drawEllipse(QPointF(cx + 27.5 * s, cy + y * s), 1.3 * s, 1.3 * s);
  }

  // 大型阀体
  QRadialGradient vg(cx - 1, cy - 2, 10 * s);
  vg.setColorAt(0, QColor(0x5B, 0x6A, 0x7E));
  vg.setColorAt(0.4, QColor(0x47, 0x55, 0x69));
  vg.setColorAt(0.7, QColor(0x2D, 0x3B, 0x4F));
  vg.setColorAt(1, QColor(0x1A, 0x24, 0x35));
  p.setBrush(vg);
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.5));
  p.drawEllipse(QPointF(cx, cy), 10 * s, 10 * s);

  // 手轮
  p.setPen(QPen(QColor(0x94, 0xA3, 0xB8), 1.8 * s));
  p.setBrush(Qt::NoBrush);
  p.drawEllipse(QPointF(cx, cy - 13 * s), 7 * s, 3 * s);
  p.drawLine(cx, cy - 16 * s, cx, cy - 10 * s);
  p.drawLine(cx - 7 * s, cy - 13 * s, cx + 7 * s, cy - 13 * s);
  p.drawLine(cx - 4 * s, cy - 14.5 * s, cx + 4 * s, cy - 11.5 * s);

  // 阀芯
  p.setPen(Qt::NoPen);
  if (m_status) {
    // 开启 → 水平 + 大水流
    p.setBrush(QColor(0x00, 0xE6, 0x76));
    p.drawRoundedRect(cx - 7 * s, cy - 1.5 * s, 14 * s, 3 * s, 1, 1);
    for (int i = 0; i < 4; ++i) {
      qreal dx = -18 * s + i * 12 * s;
      p.setBrush(QColor(0x00, 0xE6, 0x76, 200));
      p.drawEllipse(QPointF(cx + dx, cy - 6.5 * s), 2.5 * s, 2.5 * s);
    }
  } else {
    // 关闭 → 垂直阀芯
    p.setBrush(TechColors::amber);
    p.drawRoundedRect(cx - 2 * s, cy - 9 * s, 4 * s, 18 * s, 1.5, 1.5);
  }
}

// ===== 手动报警按钮 — 方形面板 + 红色按钮 =====
void DeviceStatusWidget::drawManualAlarm(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::red : TechColors::amber;

  // 面板
  p.setBrush(QColor(0x1E, 0x29, 0x3B));
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.5 * s));
  p.drawRoundedRect(cx - 18 * s, cy - 18 * s, 36 * s, 36 * s, 5, 5);

  // 内部透明玻璃掩体
  p.setBrush(QColor(0x0F, 0x17, 0x2A, 200));
  p.setPen(QPen(QColor(0x33, 0x41, 0x55), 1.0));
  p.drawRoundedRect(cx - 13 * s, cy - 13 * s, 26 * s, 26 * s, 3, 3);

  // 中心红色报警按钮
  QRadialGradient bg(cx, cy, 10 * s);
  bg.setColorAt(0, c.lighter(130));
  bg.setColorAt(0.7, c);
  bg.setColorAt(1, c.darker(150));
  p.setBrush(bg);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPointF(cx, cy), 9 * s, 9 * s);

  // 高光
  p.setBrush(QColor(255, 255, 255, m_status ? 30 : 50));
  p.drawEllipse(QPointF(cx - 2 * s, cy - 3 * s), 3 * s, 2.5 * s);
}

// ===== 1301气体钢瓶 — 红色瓶体 + 表针 =====
void DeviceStatusWidget::drawGasCylinder(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor bodyCol = TechColors::red;

  // 气体喷放静态效果 (只在 status 激活即释放状态显示)
  if (m_status) {
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 4; ++i) {
      qreal dist = 8 * s + i * 4 * s;
      qreal sprayAngle = -M_PI_2 + (i - 1.5) * 0.22;
      qreal px = cx + dist * qCos(sprayAngle);
      qreal py = cy - 26 * s + dist * qSin(sprayAngle);
      p.setBrush(QColor(0xE2, 0xE8, 0xF0, 160));
      p.drawEllipse(QPointF(px, py), 2.5 * s, 2.5 * s);
    }
  }

  // 瓶体
  QLinearGradient bg(cx - 10 * s, 0, cx + 10 * s, 0);
  bg.setColorAt(0, bodyCol.darker(160));
  bg.setColorAt(0.3, bodyCol);
  bg.setColorAt(0.6, bodyCol.lighter(110));
  bg.setColorAt(1, bodyCol.darker(160));
  p.setPen(Qt::NoPen); p.setBrush(bg);
  p.drawRoundedRect(cx - 9 * s, cy - 20 * s, 18 * s, 40 * s, 4, 4);

  // 高光
  p.setBrush(QColor(255, 255, 255, 25));
  p.drawRoundedRect(cx - 6 * s, cy - 17 * s, 3 * s, 30 * s, 2, 2);

  // 瓶颈/阀
  p.setBrush(QColor(0x94, 0xA3, 0xB8));
  p.drawRect(cx - 3 * s, cy - 23 * s, 6 * s, 3 * s);
  p.drawRoundedRect(cx - 6 * s, cy - 26 * s, 12 * s, 3 * s, 1, 1);

  // 表盘
  p.setBrush(TechColors::bg.darker(150));
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.0));
  p.drawEllipse(QPointF(cx, cy + 2 * s), 8 * s, 8 * s);
  // 表针 (正常为-M_PI_4，喷放为-M_PI * 3.0/4.0 零位)
  p.setPen(QPen(m_status ? TechColors::red : TechColors::green, 1.5 * s));
  qreal angle = m_status ? (-M_PI * 3.0 / 4.0) : -M_PI_4;
  p.drawLine(QPointF(cx, cy + 2 * s),
             QPointF(cx + 5 * s * qCos(angle), cy + 2 * s + 5 * s * qSin(angle)));
  p.setPen(Qt::NoPen);
}

// ===== 水泵 — 电机圆形 + 旋转叶轮 + 水流 =====
void DeviceStatusWidget::drawWaterPump(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::green : TechColors::amber;

  // 底座
  p.setBrush(QColor(0x1E, 0x29, 0x3B));
  p.setPen(Qt::NoPen);
  p.drawRoundedRect(cx - 18 * s, cy + 20 * s, 36 * s, 5 * s, 2, 2);

  // 管道
  p.setPen(QPen(QColor(0x33, 0x41, 0x55), 4 * s));
  p.drawLine(cx - 22 * s, cy, cx - 10 * s, cy);
  p.drawLine(cx + 10 * s, cy, cx + 22 * s, cy);
  p.setPen(Qt::NoPen);

  // 泵体圆
  QRadialGradient pg(cx - 1, cy - 1, 12 * s);
  pg.setColorAt(0, QColor(0x64, 0x74, 0x8B));
  pg.setColorAt(0.5, QColor(0x47, 0x55, 0x69));
  pg.setColorAt(1, QColor(0x1E, 0x29, 0x3B));
  p.setBrush(pg);
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.0));
  p.drawEllipse(QPointF(cx, cy), 12 * s, 12 * s);

  // 叶轮 - 运转状态
  p.setPen(Qt::NoPen);
  qreal angle = m_status ? 45.0 : 0;
  p.save();
  p.translate(cx, cy);
  p.rotate(angle);
  p.setBrush(c.lighter(m_status ? 140 : 100));
  QPainterPath impeller;
  impeller.moveTo(0, -5 * s);
  impeller.lineTo(2.5 * s, 0);
  impeller.lineTo(6 * s, 1.5 * s);
  impeller.lineTo(2.5 * s, 2.5 * s);
  impeller.lineTo(0, 5 * s);
  impeller.lineTo(-2.5 * s, 2.5 * s);
  impeller.lineTo(-6 * s, 1.5 * s);
  impeller.lineTo(-2.5 * s, 0);
  impeller.closeSubpath();
  p.drawPath(impeller);
  p.restore();

  // 中心轴
  p.setBrush(QColor(0x94, 0xA3, 0xB8));
  p.drawEllipse(QPointF(cx, cy), 2.5 * s, 2.5 * s);

  // 水流指示 (运转时)
  if (m_status) {
    for (int i = 0; i < 2; ++i) {
      qreal dx = -12 * s + i * 24 * s;
      p.setBrush(QColor(0x10, 0xB9, 0x81, 180));
      p.drawEllipse(QPointF(cx + dx, cy - 4 * s), 2.0 * s, 2.0 * s);
    }
  }
}

// ===== 压力开关 — 膜片盒 + 电气触点 + 表针 =====
void DeviceStatusWidget::drawPressureSwitch(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::accentCyan : TechColors::gray;

  // 壳体
  p.setBrush(QColor(0x1E, 0x29, 0x3B, 180));
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 1.5 * s));
  p.drawRoundedRect(cx - 14 * s, cy - 18 * s, 28 * s, 28 * s, 4, 4);

  // 膜片
  p.setPen(Qt::NoPen);
  qreal diaphY = m_status ? cy - 2 * s : cy + 4 * s;
  p.setBrush(QColor(0x64, 0x74, 0x8B));
  p.drawEllipse(QPointF(cx, diaphY), 10 * s, 6 * s);

  // 触点
  p.setBrush(QColor(0x33, 0x41, 0x55));
  p.drawEllipse(QPointF(cx - 6 * s, cy + 8 * s), 2.5 * s, 2.5 * s);
  p.drawEllipse(QPointF(cx + 6 * s, cy + 8 * s), 2.5 * s, 2.5 * s);

  // 压力管
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 3 * s));
  p.drawLine(cx, cy + 12 * s, cx, cy + 22 * s);

  // 表针
  p.setPen(QPen(c, 1.8 * s));
  qreal ang = m_status ? -M_PI_4 : -2.8;
  p.drawLine(QPointF(cx, cy - 10 * s),
             QPointF(cx + 6 * s * qCos(ang), cy - 10 * s + 6 * s * qSin(ang)));
  p.setPen(Qt::NoPen);
}

// ===== 移动喷枪 — 枪身 + 喷嘴 + 喷雾粒子 =====
void DeviceStatusWidget::drawMobileSprayGun(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  // 握把
  p.setPen(QPen(QColor(0x47, 0x55, 0x69), 4 * s, Qt::SolidLine, Qt::RoundCap));
  p.drawLine(cx - 12 * s, cy + 10 * s, cx - 4 * s, cy + 2 * s);
  p.setPen(Qt::NoPen);

  // 枪身管
  p.setPen(QPen(QColor(0x64, 0x74, 0x8B), 5 * s, Qt::SolidLine, Qt::FlatCap));
  p.drawLine(cx - 6 * s, cy + 4 * s, cx + 12 * s, cy - 8 * s);
  p.setPen(Qt::NoPen);

  // 喷嘴
  p.setBrush(QColor(0x94, 0xA3, 0xB8));
  // 喷雾指示
  if (m_status) {
    for (int i = 0; i < 4; ++i) {
      qreal dx = 13 * s + i * 4 * s;
      qreal dy = -9 * s - i * 2 * s;
      p.setBrush(QColor(0x10, 0xB9, 0x81, 180));
      p.drawEllipse(QPointF(cx + dx, cy + dy), 2.0 * s, 2.0 * s);
    }
  }
}

// ===== 简约风格备选图标 (风格1) =====

void DeviceStatusWidget::drawDetectorSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::red:TechColors::green;
  p.setPen(QPen(c,2.5*s)); p.setBrush(Qt::NoBrush);
  p.drawEllipse(QPointF(cx,cy),20*s,20*s);
  p.setPen(Qt::NoPen); p.setBrush(c);
  p.drawEllipse(QPointF(cx,cy),6*s,6*s);
}
void DeviceStatusWidget::drawValveSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::green:TechColors::amber;
  p.setPen(QPen(QColor(0x64,0x74,0x8B),4*s)); p.setBrush(Qt::NoBrush);
  p.drawLine(cx-22*s,cy,cx+22*s,cy);
  p.setPen(Qt::NoPen);
  p.setBrush(c);
  if(m_status){p.drawRoundedRect(cx-8*s,cy-2*s,16*s,4*s,2,2);}
  else{p.drawRoundedRect(cx-2*s,cy-8*s,4*s,16*s,2,2);}
}
void DeviceStatusWidget::drawManualAlarmSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::red:TechColors::green;
  p.setBrush(c.darker(140));p.setPen(QPen(c,2*s));
  p.drawRoundedRect(cx-16*s,cy-16*s,32*s,32*s,4,4);
  p.setPen(Qt::NoPen);p.setBrush(c);
  p.drawEllipse(QPointF(cx,cy),8*s,8*s);
}
void DeviceStatusWidget::drawGasCylinderSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::red:TechColors::accentCyan;
  p.setBrush(c);p.setPen(Qt::NoPen);
  p.drawRoundedRect(cx-8*s,cy-18*s,16*s,36*s,4,4);
  p.setBrush(QColor(0x94,0xA3,0xB8));
  p.drawRect(cx-3*s,cy-22*s,6*s,4*s);
}
void DeviceStatusWidget::drawWaterPumpSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::green:TechColors::amber;
  p.setBrush(c);p.setPen(QPen(QColor(0x47,0x55,0x69),1.5*s));
  p.drawEllipse(QPointF(cx,cy),16*s,16*s);
  p.setPen(Qt::NoPen);p.setBrush(TechColors::bg);
  QPainterPath tri;tri.moveTo(cx-5*s,cy-8*s);tri.lineTo(cx+10*s,cy);tri.lineTo(cx-5*s,cy+8*s);tri.closeSubpath();
  p.drawPath(tri);
}
void DeviceStatusWidget::drawPressureSwitchSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::accentCyan:TechColors::gray;
  p.setPen(QPen(c,2*s));p.setBrush(QColor(0x1E,0x29,0x3B,120));
  p.drawRoundedRect(cx-12*s,cy-16*s,24*s,24*s,3,3);
  p.setPen(QPen(c,1.8*s));p.drawLine(cx,cy-12*s,cx+6*s,cy-12*s);
}
void DeviceStatusWidget::drawMobileSprayGunSimple(QPainter &p, const QRect &area) {
  int cx=area.center().x(), cy=area.center().y();
  qreal s=area.height()/85.0;
  QColor c=m_status?TechColors::green:TechColors::gray;
  p.setPen(QPen(c,4*s));p.setBrush(Qt::NoBrush);
  p.drawLine(cx-12*s,cy+6*s,cx+10*s,cy-8*s);
  p.setPen(Qt::NoPen);p.setBrush(c);
  p.drawEllipse(QPointF(cx+12*s,cy-9*s),3*s,3*s);
}

// ===== 拖拽功能 =====
void DeviceStatusWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() != Qt::LeftButton) {
    QWidget::mousePressEvent(e);
    return;
  }
  m_dragStartPos = e->globalPos();
  m_dragWidgetStart = pos();

  if (m_draggable) {
    m_dragActive = true;
    raise();
    setCursor(Qt::ClosedHandCursor);
  } else {
    // 未摆放的卡片，在 mousePressEvent 中只记录起点，在 mouseMoveEvent 中触发拖动
    QWidget::mousePressEvent(e);
  }
}

void DeviceStatusWidget::mouseMoveEvent(QMouseEvent *e) {
  if (m_dragActive) {
    QPoint delta = e->globalPos() - m_dragStartPos;
    QPoint newPos = m_dragWidgetStart + delta;
    // 限制在父控件内
    if (parentWidget()) {
      newPos.setX(qMax(0, qMin(newPos.x(), parentWidget()->width() - width())));
      newPos.setY(qMax(0, qMin(newPos.y(), parentWidget()->height() - height())));
    }
    move(newPos);
    emit deviceDragging(m_deviceId, pos());
  } else if (!m_draggable && (e->buttons() & Qt::LeftButton)) {
    // 只能从未放置侧边栏发起 QDrag 拖拽入舱
    if (parentWidget() && parentWidget()->objectName() == QStringLiteral("unplacedContainer")) {
      if ((e->globalPos() - m_dragStartPos).manhattanLength() > 10) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString("device:%1").arg(m_deviceId));
        drag->setMimeData(mimeData);
        
        QPixmap pixmap = grab();
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(48, 53)); // 96x106 卡片中心点
        
        drag->exec(Qt::MoveAction);
        setCursor(Qt::ArrowCursor);
      }
    } else {
      QWidget::mouseMoveEvent(e);
    }
  } else {
    QWidget::mouseMoveEvent(e);
  }
}

void DeviceStatusWidget::mouseReleaseEvent(QMouseEvent *e) {
  if (m_dragActive) {
    m_dragActive = false;
    if (mouseGrabber() == this) {
      releaseMouse();
    }
    setCursor(Qt::ArrowCursor);
    emit deviceDragFinished(m_deviceId);
    emit deviceDragged(m_deviceId, pos());
  } else if (!m_draggable && e->button() == Qt::LeftButton) {
    // 如果没有拖拽，只是单纯点击释放，则执行点击摆放
    if ((e->globalPos() - m_dragStartPos).manhattanLength() < 5) {
      emit dragStartedFromDock(m_deviceId, e->globalPos());
    }
  } else {
    QWidget::mouseReleaseEvent(e);
  }
}

void DeviceStatusWidget::startDragging(const QPoint &globalPos) {
  m_draggable = true;
  m_dragActive = true;
  m_dragStartPos = globalPos;
  m_dragWidgetStart = pos();
  raise();
  setCursor(Qt::ClosedHandCursor);
  grabMouse(); // 抓取鼠标事件，实现无缝连续拖拽
}
