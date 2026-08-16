#ifndef VERTICALTABWIDGET_H
#define VERTICALTABWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QPoint>
#include <QRect>

class QVBoxLayout;
class QResizeEvent;
class QEnterEvent;
class QEvent;
class QPaintEvent;
class QMouseEvent;

// ─────────────────────────────────────────────────────────────
//  Internal chrome-style tab button
// ─────────────────────────────────────────────────────────────
class ChromeTabButton : public QPushButton {
  Q_OBJECT
  Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

public:
  explicit ChromeTabButton(const QString &label,
                           const QString &iconCode,
                           int tabIndex,
                           QWidget *parent = nullptr);

  void setTabInfo(const QString &iconCode);
  void setTabIndex(int idx) { m_tabIndex = idx; }
  void applyTheme(const QString &themeName);

  qreal hoverOpacity() const { return m_hoverOpacity; }
  void setHoverOpacity(qreal v);

signals:
  void closeRequested(int index);

protected:
  void resizeEvent(QResizeEvent *e) override;
  void enterEvent(QEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;

private:
  QString m_label;
  QString m_iconCode;
  int m_tabIndex;
  qreal m_hoverOpacity = 0.0;
  QPushButton *m_btnClose;
  QPropertyAnimation *m_hoverAnim;
  QPoint m_dragStartPos;
};

// Chrome 风格垂直标签页控件
class VerticalTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit VerticalTabWidget(QWidget *parent = nullptr);

  // 添加页面 (返回 index)
  int addTab(QWidget *widget, const QString &label,
             const QString &iconCode = QString());

  // 移除页面
  void removeTab(int index);

  // 设置 / 获取当前页面
  void setCurrentIndex(int index);
  int currentIndex() const { return m_currentIndex; }

  // 动态更新标签图标
  void updateTabIcon(int index, const QString &iconCode);

  // 主题切换
  void applyTheme(const QString &themeName);

  // 获取页面
  QWidget *widget(int index) const;

  // 标签总数
  int count() const { return m_tabButtons.size(); }

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index);
  void newTabRequested();

private:
  void setupUi();
  void updateTabStyles();
  void rebuildConnections();

  QVector<QPushButton *> m_tabButtons;
  QStackedWidget *m_stackedWidget;
  QVBoxLayout *m_tabLayout = nullptr;
  QScrollArea *m_scrollArea = nullptr;
  QPushButton *m_btnNewTab = nullptr;
  int m_currentIndex = 0;

  // Drag-to-sort state
  int m_dragStartIndex = -1;
  QPoint m_dragStartPos;
};

#endif // VERTICALTABWIDGET_H
