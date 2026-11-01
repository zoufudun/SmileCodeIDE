#ifndef SCROLLINGLABEL_H
#define SCROLLINGLABEL_H

#include <QColor>
#include <QFont>
#include <QTimer>
#include <QWidget>


class ScrollingLabel : public QWidget {
  Q_OBJECT
public:
  explicit ScrollingLabel(QWidget *parent = nullptr);
  void setText(const QString &text);
  void setSpeed(int pxPerFrame); // Speed in pixels per frame

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void updatePosition();

private:
  QString m_text;
  int m_offset;
  int m_pxPerFrame;
  QTimer *m_timer;
  int m_textWidth;
  int m_separatorWidth;
};

#endif // SCROLLINGLABEL_H
