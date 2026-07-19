#ifndef ROOMWIDGET_H
#define ROOMWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>

// 可拖拽/缩放/重命名的房间区域控件
class RoomWidget : public QWidget {
  Q_OBJECT
public:
  explicit RoomWidget(const QString &id, const QString &name,
                      const QRect &geom, QWidget *parent = nullptr);

  QString roomId() const { return m_id; }
  QString roomName() const { return m_titleLabel->text(); }
  void setRoomName(const QString &name) { m_titleLabel->setText(name); }
  QRect roomGeom() const { return geometry(); }

  void setDeviceCount(int n);
  void updateTitleFromLabel();

signals:
  void roomMoved(const QString &id, const QRect &newGeom);
  void roomResized(const QString &id, const QRect &newGeom);
  void roomClicked(const QString &id);

protected:
  void paintEvent(QPaintEvent *) override;
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
  bool m_dragging = false;
  bool m_resizing = false;
  Edge m_resizeEdge = None;
  QPoint m_dragStart;
  QRect m_dragStartGeom;
  static constexpr int HANDLE_SIZE = 8;
};

#endif // ROOMWIDGET_H
