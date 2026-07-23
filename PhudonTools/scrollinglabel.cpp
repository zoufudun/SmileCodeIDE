#include "scrollinglabel.h"
#include <QFontMetrics>
#include <QPainter>


ScrollingLabel::ScrollingLabel(QWidget *parent)
    : QWidget(parent), m_offset(0), m_pxPerFrame(1), m_textWidth(0),
      m_separatorWidth(50) {
  // Transparent background
  setAttribute(Qt::WA_TranslucentBackground);

  // Fixed height approx for status bar
  setFixedHeight(30);

  // Font setup
  QFont f = font();
  f.setPointSize(10);
  f.setBold(true);
  setFont(f);

  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, &ScrollingLabel::updatePosition);
  m_timer->start(16); // ~60 FPS
}

void ScrollingLabel::setText(const QString &text) {
  m_text = text;
  QFontMetrics fm(font());
  m_textWidth = fm.horizontalAdvance(m_text);
  update();
}

void ScrollingLabel::setSpeed(int pxPerFrame) { m_pxPerFrame = pxPerFrame; }

void ScrollingLabel::updatePosition() {
  if (m_text.isEmpty())
    return;

  m_offset -= m_pxPerFrame;

  // Cycle logic
  // We draw text at m_offset.
  // If m_offset + m_textWidth + m_separatorWidth < 0, it means the first
  // instance assumes completely offscreen left But we need seamless looping.
  // The Seamless loop pattern is: Text + Space + Text + Space ...
  // The cycle length is (m_textWidth + m_separatorWidth).

  int cycleLen = m_textWidth + m_separatorWidth;
  if (cycleLen <= 0)
    return;

  if (m_offset < -cycleLen) {
    m_offset += cycleLen;
  }

  update();
}

void ScrollingLabel::resizeEvent(QResizeEvent *) {
  // Optional: Recalculate if needed
}

void ScrollingLabel::paintEvent(QPaintEvent *) {
  if (m_text.isEmpty())
    return;

  QPainter p(this);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setPen(QColor(0, 0, 255)); // Blue color as requested
  p.setFont(font());

  // Draw text multiple times to cover the width
  // Start drawing from m_offset

  int cycleLen = m_textWidth + m_separatorWidth;
  if (cycleLen <= 0)
    return;

  int x = m_offset;

  // Ensure we fill the widget width
  while (x < width()) {
    if (x + m_textWidth > 0) {
      p.drawText(x, 0, m_textWidth, height(), Qt::AlignVCenter | Qt::AlignLeft,
                 m_text);
    }
    x += cycleLen;
  }
}
