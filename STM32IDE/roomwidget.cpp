#include "roomwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

RoomWidget::RoomWidget(const QString &id, const QString &name,
                       const QRect &geom, QWidget *parent)
    : QWidget(parent), m_id(id) {
  setGeometry(geom);
  setMouseTracking(true);

  // 标题栏
  m_titleLabel = new QLabel(name, this);
  m_titleLabel->setStyleSheet(
      "QLabel { color: #00D4FF; font-size: 11px; font-weight: bold; "
      "font-family: 'Microsoft YaHei'; background: rgba(13,17,23,180); "
      "padding: 3px 8px; border-radius: 4px; }");
  m_titleLabel->move(8, 6);
  m_titleLabel->setFixedHeight(22);

  m_countLabel = new QLabel("0", this);
  m_countLabel->setStyleSheet(
      "QLabel { color: #64748B; font-size: 9px; font-weight: bold; "
      "font-family: 'Consolas'; background: rgba(0,0,0,60); "
      "padding: 1px 6px; border-radius: 3px; }");
  m_countLabel->move(8, 30);
  m_countLabel->setFixedHeight(16);

  show();
}

void RoomWidget::setDeviceCount(int n) {
  m_countLabel->setText(QStringLiteral("%1 个设备").arg(n));
}

void RoomWidget::updateTitleFromLabel() {
  // 无需额外操作，title 由外部设置
}

void RoomWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRect r = rect().adjusted(1, 1, -1, -1);
  p.setPen(QPen(QColor(0x00, 0xD4, 0xFF, 60), 1.5, Qt::DashLine));
  p.setBrush(QColor(0x00, 0xD4, 0xFF, 10));
  p.drawRoundedRect(r, 6, 6);

  // 缩放手柄指示点
  p.setBrush(QColor(0x00, 0xD4, 0xFF, 100));
  p.setPen(Qt::NoPen);
  int s = 5;
  p.drawEllipse(QPoint(r.left() + s, r.top() + s), 2, 2);
  p.drawEllipse(QPoint(r.right() - s, r.top() + s), 2, 2);
  p.drawEllipse(QPoint(r.left() + s, r.bottom() - s), 2, 2);
  p.drawEllipse(QPoint(r.right() - s, r.bottom() - s), 2, 2);
}

RoomWidget::Edge RoomWidget::hitTest(const QPoint &pos) const {
  int x = pos.x(), y = pos.y(), w = width(), h = height();
  int s = HANDLE_SIZE;
  bool l = x < s, r = x > w - s, t = y < s, b = y > h - s;
  if (l && t) return TopLeft;
  if (r && t) return TopRight;
  if (l && b) return BottomLeft;
  if (r && b) return BottomRight;
  if (l) return Left;
  if (r) return Right;
  if (t) return Top;
  if (b) return Bottom;
  return None;
}

void RoomWidget::updateCursor(Edge e) {
  switch (e) {
    case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
    case Left: case Right: setCursor(Qt::SizeHorCursor); break;
    case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
    default: setCursor(Qt::ArrowCursor); break;
  }
}

void RoomWidget::mousePressEvent(QMouseEvent *e) {
  m_resizeEdge = hitTest(e->pos());
  if (m_resizeEdge != None) {
    m_resizing = true;
    m_dragStart = e->globalPos();
    m_dragStartGeom = geometry();
  } else if (e->pos().y() < 32) {
    // 标题栏拖拽
    m_dragging = true;
    m_dragStart = e->globalPos();
    m_dragStartGeom = geometry();
    setCursor(Qt::ClosedHandCursor);
  } else {
    emit roomClicked(m_id);
  }
}

void RoomWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!m_dragging && !m_resizing) {
    updateCursor(hitTest(e->pos()));
    return;
  }

  QPoint delta = e->globalPos() - m_dragStart;
  QRect g = m_dragStartGeom;

  if (m_dragging) {
    g.translate(delta);
  } else {
    switch (m_resizeEdge) {
      case Right: g.setRight(g.right() + delta.x()); break;
      case Left: g.setLeft(g.left() + delta.x()); break;
      case Bottom: g.setBottom(g.bottom() + delta.y()); break;
      case Top: g.setTop(g.top() + delta.y()); break;
      case TopLeft: g.setTopLeft(g.topLeft() + delta); break;
      case TopRight: g.setTopRight(g.topRight() + delta); break;
      case BottomLeft: g.setBottomLeft(g.bottomLeft() + delta); break;
      case BottomRight: g.setBottomRight(g.bottomRight() + delta); break;
      default: break;
    }
    if (g.width() < 100) g.setWidth(100);
    if (g.height() < 80) g.setHeight(80);
  }

  setGeometry(g);
}

void RoomWidget::mouseReleaseEvent(QMouseEvent *e) {
  if (m_dragging) {
    m_dragging = false;
    setCursor(Qt::ArrowCursor);
    emit roomMoved(m_id, geometry());
  }
  if (m_resizing) {
    m_resizing = false;
    setCursor(Qt::ArrowCursor);
    emit roomResized(m_id, geometry());
  }
}
