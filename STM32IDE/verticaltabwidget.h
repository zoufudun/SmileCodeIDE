#ifndef VERTICALTABWIDGET_H
#define VERTICALTABWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>

class QVBoxLayout;

// 垂直标签页控件
class VerticalTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit VerticalTabWidget(QWidget *parent = nullptr);

  // 添加页面
  int addTab(QWidget *widget, const QString &label, const QString &iconCode = QString());

  // 设置当前页面
  void setCurrentIndex(int index);
  int currentIndex() const { return m_currentIndex; }

  // 动态更新标签图标
  void updateTabIcon(int index, const QString &iconCode);

  // 获取页面
  QWidget *widget(int index) const;

signals:
  void currentChanged(int index);

private:
  void setupUi();
  void updateTabStyles();

  QVector<QPushButton *> m_tabButtons;
  QStackedWidget *m_stackedWidget;
  QVBoxLayout *m_tabLayout = nullptr;
  int m_currentIndex = 0;
};

#endif // VERTICALTABWIDGET_H
