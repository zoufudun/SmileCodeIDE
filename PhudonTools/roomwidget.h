#ifndef ROOMWIDGET_H
#define ROOMWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>

// 可拖拽/缩放/重命名的房间区域控件
class QPushButton;
class RoomWidget : public QWidget {
  Q_OBJECT
public:
  explicit RoomWidget(const QString &id, const QString &name,
                      const QRect &geom, int shape = 0, QWidget *parent = nullptr);

  QString roomId() const { return m_id; }
  QString roomName() const { return m_titleLabel->text(); }
  void setRoomName(const QString &name) { m_titleLabel->setText(name); }
  QRect roomGeom() const { return geometry(); }
  int roomShape() const { return m_shape; }

  void setDeviceCount(int n);
  void updateTitleFromLabel();
  void setEditingEnabled(bool enable);
  bool isEditingEnabled() const { return m_editingEnabled; }

  bool isLocked() const { return m_locked; }
  void setLocked(bool locked);
  QColor roomColor() const { return m_color; }
  void setRoomColor(const QColor &color);
  void setRoomShape(int shape);
  void setMinimumContentSize(int minW, int minH) {
    m_minW = qMax(160, minW);
    m_minH = qMax(140, minH);
  }

signals:
  void roomDragging(const QString &id, const QRect &currentGeom);
  void roomDragFinished(const QString &id);
  void roomMoved(const QString &id, const QRect &newGeom);
  void roomResized(const QString &id, const QRect &newGeom);
  void roomClicked(const QString &id);
  void roomRenameRequested(const QString &id);
  void roomDeleteRequested(const QString &id);
  void roomLockToggled(const QString &id, bool locked);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

private:
  enum Edge { None = 0, Top, Bottom, Left, Right, TopLeft, TopRight, BottomLeft, BottomRight };
  Edge hitTest(const QPoint &pos) const;
  void updateCursor(Edge e);

  QString m_id;
  QLabel *m_titleLabel;
  QLabel *m_countLabel;
  QLabel *m_lockIconLabel = nullptr;
  QPushButton *m_btnLock = nullptr;
  QPushButton *m_btnRename = nullptr;
  QPushButton *m_btnDelete = nullptr;
  int m_shape = 0;
  int m_minW = 160;
  int m_minH = 140;
  bool m_editingEnabled = true;
  bool m_locked = false;
  QColor m_color = QColor(0, 212, 255);
  bool m_dragging = false;
  bool m_resizing = false;
  Edge m_resizeEdge = None;
  QPoint m_dragStart;
  QRect m_dragStartGeom;
  static constexpr int HANDLE_SIZE = 8;
};

#endif // ROOMWIDGET_H
