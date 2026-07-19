#include "devicestatuswidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

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
      m_deviceId(deviceId), m_kind(kind), m_label(label), m_canId(canId),
      m_flashTimer(new QTimer(this)), m_animTimer(new QTimer(this))
{
  // 固定尺寸策略 — 不随容器拉伸
  setMinimumSize(128, 155);
  setMaximumSize(200, 240);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_flashTimer->setInterval(500);
  connect(m_flashTimer, &QTimer::timeout, this, [this]() {
    m_alarmPhase = !m_alarmPhase;
    update();
  });

  m_animTimer->setInterval(33);
  connect(m_animTimer, &QTimer::timeout, this, [this]() {
    m_animProgress += 0.025;
    if (m_animProgress > 1.0) m_animProgress -= 1.0;
    update();
  });
  m_animTimer->start();
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

void DeviceStatusWidget::setStatus(bool value) {
  if (m_status == value && m_flashTimer->isActive() == value) return;
  m_status = value;
  if (value) { m_alarmPhase = true; m_flashTimer->start(); }
  else { m_flashTimer->stop(); m_alarmPhase = false; }
  update();
}

// ===== 主绘制 =====
void DeviceStatusWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const int w = width(), h = height();
  const QRect card(2, 2, w - 4, h - 4);
  qreal s = qMin(w / 135.0, h / 160.0); // 统一缩放因子

  // ---- 卡片背景 ----
  {
    QPainterPath bgPath;
    bgPath.addRoundedRect(card, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(TechColors::bg);
    p.drawPath(bgPath);

    // 顶部渐变
    QLinearGradient g(card.left(), card.top(), card.left(), card.top() + 30);
    g.setColorAt(0, QColor(0x1E, 0x2A, 0x3E, 100));
    g.setColorAt(1, QColor(0x0F, 0x17, 0x2A, 0));
    p.setBrush(g);
    p.drawPath(bgPath);

    // 悬停外发光
    if (m_hovered) {
      QColor glow = accentColor();
      glow.setAlpha(25);
      p.setPen(QPen(glow, 3));
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(card.adjusted(-1, -1, 1, 1), 9, 9);
    }

    // 边框
    QColor bc = TechColors::border;
    if (m_hovered) bc = accentColor();
    else if (m_status) bc = m_alarmPhase ? TechColors::red : TechColors::border;
    p.setPen(QPen(bc, m_hovered ? 1.5 : 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(bgPath);
  }

  // ---- 顶部扫描线 ----
  {
    int sy = card.top() + 2 + static_cast<int>(m_animProgress * (card.height() - 4));
    QLinearGradient sg(card.left(), sy - 6, card.left(), sy + 6);
    sg.setColorAt(0, QColor(0, 0, 0, 0));
    sg.setColorAt(0.5, QColor(0x00, 0xD4, 0xFF, 10));
    sg.setColorAt(1, QColor(0, 0, 0, 0));
    p.setPen(Qt::NoPen); p.setBrush(sg);
    p.drawRect(card);
  }

  // ---- CAN ID (右上角) ----
  {
    p.setPen(TechColors::accentCyan);
    p.setFont(QFont("Consolas", qMax(6, qRound(7.0 * s)), QFont::Bold));
    QString canStr = QStringLiteral("CAN 0x%1").arg(m_canId, 3, 16, QChar('0')).toUpper();
    p.drawText(QRect(card.left(), card.top() + 3, card.width() - 5, 14), Qt::AlignRight, canStr);
  }

  // ---- 设备 ID (左上角) ----
  {
    p.setPen(TechColors::textDim);
    p.setFont(QFont("Consolas", qMax(5, qRound(6.5 * s)), QFont::Bold));
    p.drawText(QRect(card.left() + 5, card.top() + 3, 40, 14), Qt::AlignLeft,
               QStringLiteral("#%1").arg(m_deviceId));
  }

  // ---- 图标区域（使用统一中心点，确保HUD环和图标对中） ----
  const int iconCX = card.center().x();
  const int iconCY = card.top() + 48 * s;
  QRect iconArea(iconCX - 30 * s, iconCY - 30 * s, 60 * s, 60 * s);

  // HUD 装饰环
  {
    p.setPen(QPen(TechColors::gray, 0.8));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(iconCX, iconCY), 28 * s, 28 * s);
    // 旋转弧
    qreal sa = m_animProgress * 360.0;
    QRectF ar(iconCX - 26 * s, iconCY - 26 * s, 52 * s, 52 * s);
    QPainterPath arc;
    arc.arcMoveTo(ar, sa);
    arc.arcTo(ar, sa, 100);
    p.setPen(QPen(accentColor(), 1.2));
    p.drawPath(arc);
  }

  // 设备图标
  switch (m_kind) {
    case Detector:       drawDetector(p, iconArea); break;
    case Valve:          drawValve(p, iconArea); break;
    case ManualAlarm:    drawManualAlarm(p, iconArea); break;
    case GasCylinder:    drawGasCylinder(p, iconArea); break;
    case WaterPump:      drawWaterPump(p, iconArea); break;
    case PressureSwitch: drawPressureSwitch(p, iconArea); break;
    case MobileSprayGun: drawMobileSprayGun(p, iconArea); break;
  }

  // ---- LED 指示灯 ----
  const int ledY = iconCY + 38 * s;
  QColor ledCol = m_status ? TechColors::red : TechColors::green;
  if (m_status && !m_alarmPhase) ledCol = QColor(0x7F, 0x22, 0x22);
  {
    QPointF lc(card.center().x(), ledY);
    // 外发光
    QColor og = ledCol; og.setAlpha(50);
    p.setPen(Qt::NoPen); p.setBrush(og);
    p.drawEllipse(lc, 6 * s, 6 * s);
    // LED 实体
    QRadialGradient lg(lc + QPointF(-0.5, -0.5), 2.8 * s);
    lg.setColorAt(0, ledCol.lighter(200));
    lg.setColorAt(0.4, ledCol);
    lg.setColorAt(1, ledCol.darker(180));
    p.setBrush(lg);
    p.drawEllipse(lc, 2.8 * s, 2.8 * s);
  }

  // ---- 设备名称 ----
  const int nameY = ledY + 9 * s;
  p.setPen(TechColors::text);
  p.setFont(QFont("Microsoft YaHei", qMax(8, qRound(9.0 * s))));
  p.drawText(QRect(card.left() + 4, nameY, card.width() - 8, 22 * s),
             Qt::AlignHCenter | Qt::TextWordWrap, m_label);

  // ---- 状态便利贴 ----
  QString stText;
  QColor stColor = m_status ? TechColors::red : TechColors::green;
  switch (m_kind) {
    case Detector:       stText = m_status ? QStringLiteral("报警") : QStringLiteral("正常"); break;
    case Valve:          stText = m_status ? QStringLiteral("开")   : QStringLiteral("关");   stColor = m_status ? TechColors::green : TechColors::amber; break;
    case ManualAlarm:    stText = m_status ? QStringLiteral("按下") : QStringLiteral("正常"); break;
    case GasCylinder:    stText = m_status ? QStringLiteral("泄漏") : QStringLiteral("正常"); break;
    case WaterPump:      stText = m_status ? QStringLiteral("运转") : QStringLiteral("停止"); stColor = m_status ? TechColors::greenBright : TechColors::amber; break;
    case PressureSwitch: stText = m_status ? QStringLiteral("开启") : QStringLiteral("关闭"); stColor = m_status ? TechColors::accentCyan : TechColors::gray; break;
    case MobileSprayGun: stText = m_status ? QStringLiteral("喷射") : QStringLiteral("停止"); stColor = m_status ? TechColors::green : TechColors::gray; break;
  }

  // 便利贴背景
  QRect stRect(card.center().x() - 22 * s, card.bottom() - 32 * s, 44 * s, 18 * s);
  {
    QPainterPath stPath;
    stPath.addRoundedRect(stRect, 4, 4);
    QColor stBg = stColor; stBg.setAlpha(30);
    p.setPen(QPen(stColor, 0.8));
    p.setBrush(stBg);
    p.drawPath(stPath);
    // 文字
    p.setPen(stColor);
    p.setFont(QFont("Microsoft YaHei", qMax(8, qRound(9.0 * s)), QFont::Bold));
    p.drawText(stRect, Qt::AlignCenter, stText);
  }

  // ---- 底部状态条 ----
  {
    int barY = card.bottom() - 5;
    p.setPen(Qt::NoPen);
    p.setBrush(TechColors::border);
    p.drawRoundedRect(card.left() + 10, barY, card.width() - 20, 2.5, 1, 1);
    p.setBrush(stColor);
    p.drawRoundedRect(card.left() + 10, barY, card.width() - 20, 2.5, 1, 1);
  }
}

// ===== 探测器 — 同心雷达环 + 中心 LED =====
void DeviceStatusWidget::drawDetector(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::red : TechColors::green;

  // 报警脉冲波纹
  if (m_status) {
    qreal pulse = 0.6 + 0.4 * m_animProgress;
    p.setPen(QPen(QColor(0xEF, 0x44, 0x44, 80), 1.5 * s));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(cx, cy), 24 * s * pulse, 24 * s * pulse);
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
    // 水流粒子
    for (int i = 0; i < 3; ++i) {
      qreal offset = fmod(m_animProgress + i * 0.33, 1.0);
      qreal dx = -20 * s + offset * 40 * s;
      qreal alpha = (1.0 - qAbs(offset - 0.5) * 2.0);
      p.setBrush(QColor(0x10, 0xB9, 0x81, qRound(alpha * 200)));
      p.drawEllipse(QPointF(cx + dx, cy - 5 * s), 2 * s * alpha, 2 * s * alpha);
    }
  } else {
    // 垂直 = 关闭
    p.setBrush(TechColors::amber);
    p.drawRoundedRect(cx - 1.5 * s, cy - 7 * s, 3 * s, 14 * s, 1, 1);
  }
}

// ===== 手动报警按钮 — 方形面板 + 红色按钮 =====
void DeviceStatusWidget::drawManualAlarm(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor c = m_status ? TechColors::red : TechColors::amber;

  // 面板
  p.setBrush(QColor(0x0F, 0x17, 0x2A));
  p.setPen(QPen(TechColors::border, 2.0 * s));
  p.drawRoundedRect(cx - 20 * s, cy - 20 * s, 40 * s, 40 * s, 5, 5);

  // 按钮圆
  p.setPen(Qt::NoPen);
  QRadialGradient bg(cx - 1, cy - 1, 8 * s);
  bg.setColorAt(0, c.lighter(150));
  bg.setColorAt(0.5, c);
  bg.setColorAt(1, c.darker(150));
  p.setBrush(bg);
  p.drawEllipse(QPointF(cx, cy), 8 * s, 8 * s);

  // 高光
  p.setBrush(QColor(255, 255, 255, m_status ? 30 : 50));
  p.drawEllipse(QPointF(cx - 2 * s, cy - 3 * s), 3 * s, 2.5 * s);

  // 螺丝
  p.setBrush(QColor(0x47, 0x55, 0x69));
  p.drawEllipse(QPointF(cx - 16 * s, cy - 16 * s), 1.5 * s, 1.5 * s);
  p.drawEllipse(QPointF(cx + 16 * s, cy - 16 * s), 1.5 * s, 1.5 * s);
  p.drawEllipse(QPointF(cx - 16 * s, cy + 16 * s), 1.5 * s, 1.5 * s);
  p.drawEllipse(QPointF(cx + 16 * s, cy + 16 * s), 1.5 * s, 1.5 * s);
}

// ===== 1301气体钢瓶 — 红色瓶体 + 表针 =====
void DeviceStatusWidget::drawGasCylinder(QPainter &p, const QRect &area) {
  int cx = area.center().x(), cy = area.center().y();
  qreal s = area.height() / 85.0;
  QColor bodyCol = m_status ? TechColors::red : TechColors::accentCyan;

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
  // 表针
  p.setPen(QPen(bodyCol, 1.5 * s));
  qreal angle = m_status ? (-M_PI_4 + qSin(m_animProgress * M_PI * 4) * 0.4) : -1.2;
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

  // 叶轮 - 运转时旋转
  p.setPen(Qt::NoPen);
  qreal angle = m_status ? m_animProgress * 360.0 : 0;
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

  // 水流粒子 (运转时)
  if (m_status) {
    for (int i = 0; i < 2; ++i) {
      qreal offset = fmod(m_animProgress + i * 0.5, 1.0);
      qreal dx = -18 * s + offset * 36 * s;
      qreal alpha = (1.0 - qAbs(offset - 0.5) * 2.0);
      p.setBrush(QColor(0x10, 0xB9, 0x81, qRound(alpha * 200)));
      p.drawEllipse(QPointF(cx + dx, cy - 4 * s), 1.8 * s * alpha, 1.8 * s * alpha);
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
  p.drawEllipse(QPointF(cx + 13 * s, cy - 9 * s), 3 * s, 3 * s);

  // 喷雾粒子
  if (m_status) {
    for (int i = 0; i < 4; ++i) {
      qreal t = fmod(m_animProgress + i * 0.25, 1.0);
      qreal dx = 13 * s + t * 16 * s;
      qreal dy = -9 * s - t * 8 * s + qSin(t * 8 + i) * 3 * s;
      qreal alpha = 1.0 - t;
      p.setBrush(QColor(0x10, 0xB9, 0x81, qRound(alpha * 220)));
      p.drawEllipse(QPointF(cx + dx, cy + dy), (1.2 + alpha * 1.5) * s, (1.2 + alpha * 1.5) * s);
    }
  }
}

void DeviceStatusWidget::enterEvent(QEvent *) { m_hovered = true; update(); }
void DeviceStatusWidget::leaveEvent(QEvent *) { m_hovered = false; update(); }
