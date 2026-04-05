#include "verticaltabwidget.h"
#include "TOOLS/CIconFont.h"
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>

// ─────────────────────────────────────────────────────────────
//  Internal chrome-style tab button
// ─────────────────────────────────────────────────────────────
ChromeTabButton::ChromeTabButton(const QString &label, const QString &iconCode,
                                 int tabIndex, QWidget *parent)
    : QPushButton(parent), m_label(label), m_iconCode(iconCode),
      m_tabIndex(tabIndex) {
  setCheckable(true);
  setCursor(Qt::PointingHandCursor);
  setMouseTracking(true);
  setFixedHeight(72);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  // Animated hover opacity
  m_hoverAnim = new QPropertyAnimation(this, "hoverOpacity", this);
  m_hoverAnim->setDuration(150);
  m_hoverAnim->setEasingCurve(QEasingCurve::InOutQuad);

  // Close button (floats over the tab)
  m_btnClose = new QPushButton(this);
  m_btnClose->setFixedSize(18, 18);
  m_btnClose->setCursor(Qt::PointingHandCursor);
  m_btnClose->setText("✕");
  m_btnClose->setStyleSheet("QPushButton {"
                            "  background: transparent;"
                            "  color: #7F8C8D;"
                            "  border: none;"
                            "  border-radius: 9px;"
                            "  font-size: 9px;"
                            "  font-weight: bold;"
                            "}"
                            "QPushButton:hover {"
                            "  background: #E74C3C;"
                            "  color: white;"
                            "}");
  m_btnClose->hide();
  connect(m_btnClose, &QPushButton::clicked, this,
          [this]() { emit closeRequested(m_tabIndex); });
}

void ChromeTabButton::setTabInfo(const QString &iconCode) {
  m_iconCode = iconCode;
  update();
}

void ChromeTabButton::setHoverOpacity(qreal v) {
  m_hoverOpacity = v;
  update();
}

void ChromeTabButton::resizeEvent(QResizeEvent *e) {
  QPushButton::resizeEvent(e);
  // Position close button top-right
  m_btnClose->move(width() - m_btnClose->width() - 4, 4);
}

void ChromeTabButton::enterEvent(QEvent *event) {
  QPushButton::enterEvent(event);

  if (m_hoverAnim) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverOpacity);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
  }

  if (m_btnClose && !m_btnClose->isVisible()) {
    m_btnClose->show();
  }
}

void ChromeTabButton::leaveEvent(QEvent *e) {
  QPushButton::leaveEvent(e);
  m_hoverAnim->stop();
  m_hoverAnim->setStartValue(m_hoverOpacity);
  m_hoverAnim->setEndValue(0.0);
  m_hoverAnim->start();
  if (!isChecked()) {
    m_btnClose->hide();
  }
}

void ChromeTabButton::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const bool active = isChecked();
  const int w = width();
  const int h = height();

  // ── Background ──────────────────────────────────────────
  if (active) {
    // White pill blending into right content pane
    QPainterPath bg;
    bg.addRoundedRect(QRectF(4, 3, w - 4, h - 6), 10, 10);
    // Clip right edge flush (no rounded corner on right)
    QPainterPath clip;
    clip.addRect(QRectF(0, 0, w + 2, h));
    p.setClipPath(clip);
    p.fillPath(bg, QColor("#FFFFFF"));
    p.setClipping(false);
  } else if (m_hoverOpacity > 0.0) {
    QPainterPath bg;
    bg.addRoundedRect(QRectF(4, 3, w - 4, h - 6), 10, 10);
    QColor hover(200, 212, 226, (int)(m_hoverOpacity * 90));
    p.fillPath(bg, hover);
  }

  // ── Blue active indicator bar ────────────────────────────
  if (active) {
    QPainterPath bar;
    bar.addRoundedRect(QRectF(1, 14, 3, h - 28), 2, 2);
    p.fillPath(bar, QColor("#1A73E8"));
  }

  // ── Icon ────────────────────────────────────────────────
  QRect iconRect(0, 10, w, h - 10);
  QColor iconColor =
      active ? QColor("#1A73E8")
             : (m_hoverOpacity > 0.5 ? QColor("#3C4043") : QColor("#5F6368"));

  if (!m_iconCode.isEmpty()) {
    int iconPx = qRound(qMin(iconRect.width(), iconRect.height()) * 0.60);
    iconPx = qMax(iconPx, 22);
    try {
      QFont iconFont = CIconFont::instance()->getIconFont(iconPx);
      iconFont.setPixelSize(iconPx);
      p.setFont(iconFont);
    } catch (...) {
      QFont f = p.font();
      f.setPixelSize(iconPx);
      p.setFont(f);
    }
    p.setPen(iconColor);
    p.drawText(iconRect, Qt::AlignCenter, m_iconCode);
  }
}

void ChromeTabButton::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    m_dragStartPos = e->pos();
  }
  QPushButton::mousePressEvent(e);
}

// ─────────────────────────────────────────────────────────────
//  VerticalTabWidget
// ─────────────────────────────────────────────────────────────
VerticalTabWidget::VerticalTabWidget(QWidget *parent) : QWidget(parent) {
  setupUi();
}

void VerticalTabWidget::setupUi() {
  QHBoxLayout *root = new QHBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // ── Left sidebar container ──────────────────────────────────
  QWidget *sidebar = new QWidget();
  sidebar->setObjectName("vtSidebar");
  sidebar->setFixedWidth(80);
  sidebar->setStyleSheet("QWidget#vtSidebar {"
                         "  background: #F1F3F4;"
                         "  border-right: 1px solid #DADCE0;"
                         "}");

  QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
  sidebarLayout->setContentsMargins(0, 0, 0, 0);
  sidebarLayout->setSpacing(0);

  // ── "New Tab" button at top ─────────────────────────────────
  m_btnNewTab = new QPushButton("+");
  m_btnNewTab->setFixedHeight(40);
  m_btnNewTab->setCursor(Qt::PointingHandCursor);
  m_btnNewTab->setToolTip("新建标签页");
  m_btnNewTab->setStyleSheet("QPushButton {"
                             "  background: transparent;"
                             "  color: #5F6368;"
                             "  font-size: 20px;"
                             "  font-weight: 300;"
                             "  border: none;"
                             "}"
                             "QPushButton:hover {"
                             "  background: #E8EAED;"
                             "  color: #1A73E8;"
                             "}");
  sidebarLayout->addWidget(m_btnNewTab);
  connect(m_btnNewTab, &QPushButton::clicked, this,
          &VerticalTabWidget::newTabRequested);

  // ── Scrollable tab list ─────────────────────────────────────
  m_scrollArea = new QScrollArea();
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // 4px ultra-thin scrollbar
  m_scrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:vertical {"
      "  width: 4px;"
      "  background: transparent;"
      "  margin: 0;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #BDC1C6;"
      "  border-radius: 2px;"
      "  min-height: 20px;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
      "height:0; }"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { "
      "background: none; }");

  QWidget *tabContainer = new QWidget();
  tabContainer->setObjectName("vtTabContainer");
  tabContainer->setStyleSheet(
      "QWidget#vtTabContainer { background: transparent; }");
  m_tabLayout = new QVBoxLayout(tabContainer);
  m_tabLayout->setContentsMargins(4, 4, 4, 4);
  m_tabLayout->setSpacing(2);
  m_tabLayout->addStretch();

  m_scrollArea->setWidget(tabContainer);
  m_scrollArea->setWidgetResizable(true);
  sidebarLayout->addWidget(m_scrollArea, 1);

  root->addWidget(sidebar);

  // ── Right content area ──────────────────────────────────────
  m_stackedWidget = new QStackedWidget();
  m_stackedWidget->setStyleSheet(
      "QStackedWidget { background: #FFFFFF; border: none; }");
  root->addWidget(m_stackedWidget, 1);
}

int VerticalTabWidget::addTab(QWidget *widget, const QString &label,
                              const QString &iconCode) {
  int index = m_tabButtons.size();

  auto *btn = new ChromeTabButton(label, iconCode, index, nullptr);
  connect(btn, &ChromeTabButton::clicked, this, [this, btn]() {
    int idx = m_tabButtons.indexOf(btn);
    if (idx >= 0)
      setCurrentIndex(idx);
  });
  connect(btn, &ChromeTabButton::closeRequested, this,
          [this](int idx) { emit tabCloseRequested(idx); });

  m_tabButtons.append(btn);

  // Insert before the trailing stretch
  int stretchPos = m_tabLayout->count() - 1;
  m_tabLayout->insertWidget(stretchPos, btn);

  m_stackedWidget->addWidget(widget);

  if (index == 0) {
    btn->setChecked(true);
    m_currentIndex = 0;
  }

  return index;
}

void VerticalTabWidget::removeTab(int index) {
  if (index < 0 || index >= m_tabButtons.size())
    return;

  auto *btn = static_cast<ChromeTabButton *>(m_tabButtons.takeAt(index));
  m_tabLayout->removeWidget(btn);
  btn->deleteLater();

  auto *page = m_stackedWidget->widget(index);
  m_stackedWidget->removeWidget(page);

  // Rebuild indices on remaining buttons
  for (int i = 0; i < m_tabButtons.size(); ++i) {
    static_cast<ChromeTabButton *>(m_tabButtons[i])->setTabIndex(i);
  }

  if (m_currentIndex >= m_tabButtons.size()) {
    m_currentIndex = m_tabButtons.size() - 1;
  }
  if (m_currentIndex >= 0) {
    setCurrentIndex(m_currentIndex);
  }
}

void VerticalTabWidget::setCurrentIndex(int index) {
  if (index < 0 || index >= m_tabButtons.size())
    return;
  m_currentIndex = index;
  m_stackedWidget->setCurrentIndex(index);
  for (int i = 0; i < m_tabButtons.size(); ++i) {
    m_tabButtons[i]->setChecked(i == index);
  }
  emit currentChanged(index);
}

void VerticalTabWidget::updateTabIcon(int index, const QString &iconCode) {
  if (index < 0 || index >= m_tabButtons.size())
    return;
  auto *btn = static_cast<ChromeTabButton *>(m_tabButtons[index]);
  btn->setTabInfo(iconCode);
}

void VerticalTabWidget::updateTabStyles() {
  for (int i = 0; i < m_tabButtons.size(); ++i) {
    m_tabButtons[i]->setChecked(i == m_currentIndex);
  }
}

void VerticalTabWidget::rebuildConnections() {}

QWidget *VerticalTabWidget::widget(int index) const {
  return m_stackedWidget->widget(index);
}
