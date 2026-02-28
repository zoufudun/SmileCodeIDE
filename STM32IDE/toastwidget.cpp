#include "toastwidget.h"
#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QStyleOption>

QList<ToastWidget *> ToastWidget::s_activeToasts;

ToastWidget::ToastWidget(QWidget *parent, const QString &message,
                         bool isSuccess)
    : QWidget(parent) {
  // Child widget overlay, frameless, on top
  // Qt::Dialog or Qt::ToolTip might be useful but Qt::SubWindow works well for
  // MDI-like or just overlay Actually, just default flags + Frameless is often
  // enough if parent is set, but we want it to float above siblings. However,
  // as a direct child, it's clipped to parent.
  setWindowFlags(Qt::FramelessWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose);

  // Styling
  // Let's switch to Layout approach for better control
  // m_label is no longer a direct member, but we need to ensure it's not used
  // elsewhere.
  // The original m_label was deleted in the provided diff, but it's not
  // created in the new code, so no need to delete.

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(15, 10, 20, 10);
  layout->setSpacing(10);

  QLabel *iconLabel = new QLabel(this);
  QPixmap iconPixmap(24, 24);
  iconPixmap.fill(Qt::transparent);
  QPainter painter(&iconPixmap);
  painter.setRenderHint(QPainter::Antialiasing);

  if (isSuccess) {
    // Green checkmark circle
    painter.setBrush(QColor(76, 175, 80)); // Green
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 24, 24);
    // Draw checkmark
    painter.setPen(
        QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(6, 12, 10, 16);
    painter.drawLine(10, 16, 18, 7);
  } else {
    // Yellow exclamation circle
    painter.setBrush(QColor(255, 193, 7)); // Yellow/Amber
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 24, 24);
    // Draw exclamation mark
    painter.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(12, 6, 12, 14);
    painter.drawPoint(12, 18);
    painter.drawEllipse(10, 16, 4, 4);
  }
  painter.end();
  iconLabel->setPixmap(iconPixmap);
  iconLabel->setFixedSize(24, 24);

  QLabel *textLabel = new QLabel(message, this);
  textLabel->setStyleSheet("color: #333333; font-family: 'Microsoft YaHei'; "
                           "font-size: 14px; font-weight: bold; background: "
                           "transparent;");

  layout->addWidget(iconLabel);
  layout->addWidget(textLabel);

  // Background Styling on Main Widget
  // Solid white background with rounded corners
  this->setObjectName("ToastWidget");
  this->setStyleSheet("#ToastWidget {"
                      "   background-color: #FFFFFF;" // Solid White
                      "   border: 1px solid #D0D0D0;"
                      "   border-radius: 10px;" // Rounded Corners
                      "}");

  // Add shadow
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 0, 0, 60));
  shadow->setOffset(0, 4);
  this->setGraphicsEffect(shadow);

  // Layout Size
  // Adjust resize based on content - add extra padding for complete text
  QFontMetrics fm(textLabel->font());
  int textWidth = fm.horizontalAdvance(message);
  int totalWidth = 15 + 24 + 10 + textWidth + 40; // Extra padding
  resize(totalWidth + 20, 54 + 10); // +Shadow margin + extra width

  // Timers
  m_lifeTimer = new QTimer(this);
  m_lifeTimer->setSingleShot(true);
  connect(m_lifeTimer, &QTimer::timeout, this, &ToastWidget::closeToast);
  m_lifeTimer->start(5000); // 5 Seconds

  // Ripple Animation Setup - continuous loop while visible
  m_rippleRadius = 0;
  m_rippleOpacity = 0.4;
  m_rippleTimer = new QTimer(this);
  connect(m_rippleTimer, &QTimer::timeout, this, [this]() {
    m_rippleRadius += 6;
    m_rippleOpacity -= 0.015;
    if (m_rippleOpacity <= 0 || m_rippleRadius > width()) {
      // Restart ripple
      m_rippleRadius = 0;
      m_rippleOpacity = 0.4;
    }
    update(); // Trigger repaint
  });
  m_rippleTimer->start(40); // ~25fps

  // raise to top of parent stack
  raise();
}

ToastWidget::~ToastWidget() {
  s_activeToasts.removeAll(this);
  // Trigger reposition for others?
  // Usually removing one might shift others up, but requirement says "stack
  // down". If one disappears, we could shift up or just leave gap. Shifting up
  // is nicer.
  if (parentWidget()) {
    repositionToasts(parentWidget());
  }
}

void ToastWidget::showToast(const QString &message, bool isSuccess,
                            QWidget *parent) {
  if (!parent)
    return;

  ToastWidget *toast = new ToastWidget(parent, message, isSuccess);

  s_activeToasts.append(toast);

  repositionToasts(parent);

  toast->show();

  // Animation: Slide from right side of PARENT
  QPropertyAnimation *anim = new QPropertyAnimation(toast, "pos");
  anim->setDuration(500);
  anim->setEasingCurve(QEasingCurve::OutBack);

  QPoint endPos = toast->pos(); // Calculated by repositionToasts
  QPoint startPos =
      endPos +
      QPoint(parent->width(), 0); // Start off-screen/right relative to parent

  anim->setStartValue(startPos);
  anim->setEndValue(endPos);
  anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastWidget::repositionToasts(QWidget *parent) {
  if (!parent)
    return;

  QRect parentRect = parent->rect();
  // Position Top Right of PARENT
  int startX = parentRect.width();
  int startY = 20; // Top padding

  int spacing = 10;
  int currentY = startY;

  for (ToastWidget *toast : s_activeToasts) {
    // Filter: Only update toasts belonging to this parent?
    // Or if we have a mix, we must check.
    if (toast->parentWidget() != parent)
      continue;

    // Position: Right aligned to parent edge minus padding
    // We use toast->width() which should be set by resize() in constructor or
    // layout We resized manually in constructor.

    int x = parentRect.width() - toast->width() - 20;

    // Stack downwards

    if (toast->isVisible()) {
      QPropertyAnimation *anim = new QPropertyAnimation(toast, "pos");
      anim->setDuration(300);
      anim->setEndValue(QPoint(x, currentY));
      anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
      toast->move(x, currentY);
    }

    currentY += toast->height() + spacing;
  }
}

void ToastWidget::closeToast() {
  // Fade out
  QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
  anim->setDuration(500);
  anim->setStartValue(1.0);
  anim->setEndValue(0.0);
  connect(anim, &QPropertyAnimation::finished, this, &ToastWidget::close);
  anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastWidget::onAnimationFinished() {
  // nothing specific
}

void ToastWidget::paintEvent(QPaintEvent *event) {
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

  // Draw water ripple effect
  if (m_rippleRadius > 0 && m_rippleOpacity > 0) {
    p.setRenderHint(QPainter::Antialiasing);
    QColor rippleColor(100, 180, 255,
                       static_cast<int>(m_rippleOpacity * 255)); // Light blue
    p.setPen(Qt::NoPen);
    p.setBrush(rippleColor);
    // Draw from center
    QPoint center(width() / 2, height() / 2);
    p.drawEllipse(center, static_cast<int>(m_rippleRadius),
                  static_cast<int>(m_rippleRadius / 2));
  }
}

void ToastWidget::enterEvent(QEvent *) {
  // Pause timer on hover?
  m_lifeTimer->stop();
}

void ToastWidget::leaveEvent(QEvent *) {
  // Resume timer
  m_lifeTimer->start(60000);
}

void ToastWidget::mousePressEvent(QMouseEvent *) { closeToast(); }
