#include "roomwidget.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>

RoomWidget::RoomWidget(const QString &id, const QString &name,
                       const QRect &geom, int shape, QWidget *parent)
    : QWidget(parent), m_id(id), m_shape(shape) {
  setGeometry(geom);
  setMouseTracking(true);

  // 标题栏 (留出右侧按钮空间)
  m_titleLabel = new QLabel(name, this);
  m_titleLabel->setStyleSheet(
      "QLabel { color: #00D4FF; font-size: 11px; font-weight: bold; "
      "font-family: 'Microsoft YaHei'; background: rgba(13,17,23,180); "
      "padding: 3px 8px; border-radius: 4px; }");
  m_titleLabel->move(8, 6);
  m_titleLabel->setFixedSize(qMax(80, width() - 60), 22);

  // 右上角改名/删除按钮
  m_btnRename = new QPushButton(QStringLiteral("✎"), this);
  m_btnRename->setFixedSize(20, 20);
  m_btnRename->move(width() - 48, 7);
  m_btnRename->setToolTip(QStringLiteral("重命名房间"));
  m_btnRename->setStyleSheet("QPushButton{color:#00D4FF;background:rgba(13,17,23,180);border:1px solid #1E3A5F;border-radius:3px;font-size:10px;}QPushButton:hover{background:#1E3A5F;}");
  m_btnRename->raise();
  connect(m_btnRename, &QPushButton::clicked, this, [this]() { emit roomRenameRequested(m_id); });

  m_btnDelete = new QPushButton(QStringLiteral("✕"), this);
  m_btnDelete->setFixedSize(20, 20);
  m_btnDelete->move(width() - 25, 7);
  m_btnDelete->setToolTip(QStringLiteral("删除房间"));
  m_btnDelete->setStyleSheet("QPushButton{color:#F87171;background:rgba(13,17,23,180);border:1px solid #3E1E1E;border-radius:3px;font-size:10px;}QPushButton:hover{background:#3E1E1E;}");
  m_btnDelete->raise();
  connect(m_btnDelete, &QPushButton::clicked, this, [this]() { emit roomDeleteRequested(m_id); });

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

void RoomWidget::setEditingEnabled(bool enable) {
  m_editingEnabled = enable;
  if (m_btnRename) m_btnRename->setVisible(enable);
  if (m_btnDelete) m_btnDelete->setVisible(enable);
  update();
}

void RoomWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRect r = rect().adjusted(1, 1, -1, -1);
  QPen dashPen(QColor(0x00, 0xD4, 0xFF, m_editingEnabled ? 80 : 40), 1.5, m_editingEnabled ? Qt::DashLine : Qt::SolidLine);
  QBrush fillBrush(QColor(0x00, 0xD4, 0xFF, m_editingEnabled ? 10 : 5));
  p.setPen(dashPen);
  p.setBrush(fillBrush);

  if (m_shape == 1) {
    // 圆形
    QRect sq(r.left(), r.top(), qMin(r.width(), r.height()), qMin(r.width(), r.height()));
    p.drawEllipse(sq);
  } else if (m_shape == 2) {
    // 菱形
    QPainterPath diamond;
    diamond.moveTo(r.center().x(), r.top());
    diamond.lineTo(r.right(), r.center().y());
    diamond.lineTo(r.center().x(), r.bottom());
    diamond.lineTo(r.left(), r.center().y());
    diamond.closeSubpath();
    p.drawPath(diamond);
  } else {
    // 矩形
    p.drawRoundedRect(r, 6, 6);
  }

  // 缩放手柄指示点 (仅在布局使能模式下绘制)
  if (m_editingEnabled) {
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 100));
    p.setPen(Qt::NoPen);
    int s = 5;
    p.drawEllipse(QPoint(r.left() + s, r.top() + s), 2, 2);
    p.drawEllipse(QPoint(r.right() - s, r.top() + s), 2, 2);
    p.drawEllipse(QPoint(r.left() + s, r.bottom() - s), 2, 2);
    p.drawEllipse(QPoint(r.right() - s, r.bottom() - s), 2, 2);
  }
}

RoomWidget::Edge RoomWidget::hitTest(const QPoint &pos) const {
  if (!m_editingEnabled) return None;
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
  if (!m_editingEnabled) {
    setCursor(Qt::ArrowCursor);
    return;
  }
  switch (e) {
    case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
    case Left: case Right: setCursor(Qt::SizeHorCursor); break;
    case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
    default: setCursor(Qt::ArrowCursor); break;
  }
}

void RoomWidget::mousePressEvent(QMouseEvent *e) {
  if (!m_editingEnabled) {
    QWidget::mousePressEvent(e);
    return;
  }
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
