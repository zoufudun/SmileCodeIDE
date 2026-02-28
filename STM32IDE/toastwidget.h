#ifndef TOASTWIDGET_H
#define TOASTWIDGET_H

#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QList>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScreen>
#include <QString>
#include <QTimer>
#include <QWidget>

class ToastWidget : public QWidget {
  Q_OBJECT

public:
  explicit ToastWidget(QWidget *parent = nullptr, const QString &message = "",
                       bool isSuccess = true);
  ~ToastWidget();

  static void showToast(const QString &message, bool isSuccess,
                        QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private slots:
  void closeToast();
  void onAnimationFinished();

private:
  QLabel *m_label;
  QTimer *m_lifeTimer;
  QTimer *m_rippleTimer;
  QPropertyAnimation *m_opacityAnim;
  double m_rippleRadius;
  double m_rippleOpacity;

  // Static list to manage stacking
  static QList<ToastWidget *> s_activeToasts;
  static void repositionToasts(QWidget *parent);
};

#endif // TOASTWIDGET_H
