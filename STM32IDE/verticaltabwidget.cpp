#include "verticaltabwidget.h"
#include "TOOLS/CIconFont.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStackedWidget>

VerticalTabWidget::VerticalTabWidget(QWidget *parent) : QWidget(parent) {
  setupUi();
}

void VerticalTabWidget::setupUi() {
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // 左侧标签栏 - 深色渐变背景
  QWidget *tabBar = new QWidget();
  tabBar->setFixedWidth(100);
  tabBar->setStyleSheet(
      "QWidget { "
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
      "    stop:0 #263238, stop:0.5 #37474F, stop:1 #455A64);"
      "}");

  m_tabLayout = new QVBoxLayout(tabBar);
  m_tabLayout->setContentsMargins(0, 30, 0, 30);
  m_tabLayout->setSpacing(15);
  m_tabLayout->addStretch();

  mainLayout->addWidget(tabBar);

  // 右侧内容区 - 浅色背景
  m_stackedWidget = new QStackedWidget();
  m_stackedWidget->setStyleSheet(
      "QStackedWidget { "
      "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
      "    stop:0 #FAFAFA, stop:1 #F5F5F5);"
      "}");
  mainLayout->addWidget(m_stackedWidget, 1);
}

int VerticalTabWidget::addTab(QWidget *widget, const QString &label,
                                const QString &iconCode) {
  // 创建标签按钮 - 纯图标
  QPushButton *tabBtn = new QPushButton();
  tabBtn->setCheckable(true);
  tabBtn->setFixedSize(100, 100);
  tabBtn->setCursor(Qt::PointingHandCursor);

  // 只显示图标，不显示文字
  if (!iconCode.isEmpty()) {
    try {
      QFont iconFont = CIconFont::instance()->getIconFont(48);
      tabBtn->setFont(iconFont);
      tabBtn->setText(iconCode);
      tabBtn->setToolTip(label);  // 文字作为提示
    } catch (...) {
      tabBtn->setText(label.left(2));  // 如果图标加载失败，显示前两个字
    }
  } else {
    tabBtn->setText(label.left(2));
  }

  // 沉浸式一体样式 - 选中后与内容区完美融合
  tabBtn->setStyleSheet(
      "QPushButton {"
      "  background: transparent;"
      "  color: rgba(176, 190, 197, 0.7);"
      "  border: none;"
      "  border-radius: 0px;"
      "  padding: 0px;"
      "  margin: 0px;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
      "    stop:0 rgba(66, 165, 245, 0.2), stop:1 transparent);"
      "  color: rgba(236, 239, 241, 1);"
      "}"
      "QPushButton:checked {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
      "    stop:0 #FAFAFA, stop:0.2 #F8F8F8, stop:1 #F5F5F5);"
      "  color: #2196F3;"
      "  font-weight: bold;"
      "  border-top-left-radius: 0px;"
      "  border-bottom-left-radius: 0px;"
      "  border-top-right-radius: 25px;"
      "  border-bottom-right-radius: 25px;"
      "  margin-right: -5px;"
      "}"
      "QPushButton:checked:hover {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
      "    stop:0 #FFFFFF, stop:0.2 #FAFAFA, stop:1 #F5F5F5);"
      "}");

  int index = m_tabButtons.size();
  m_tabButtons.append(tabBtn);

  // 添加到布局
  if (m_tabLayout) {
    m_tabLayout->insertWidget(m_tabLayout->count() - 1, tabBtn);
  }

  // 添加内容页面
  m_stackedWidget->addWidget(widget);

  // 连接信号
  connect(tabBtn, &QPushButton::clicked, this, [this, index]() {
    setCurrentIndex(index);
  });

  // 如果是第一个标签，设置为选中
  if (index == 0) {
    tabBtn->setChecked(true);
  }

  return index;
}

void VerticalTabWidget::setCurrentIndex(int index) {
  if (index < 0 || index >= m_tabButtons.size()) {
    return;
  }

  m_currentIndex = index;
  m_stackedWidget->setCurrentIndex(index);

  // 更新标签样式
  for (int i = 0; i < m_tabButtons.size(); ++i) {
    m_tabButtons[i]->setChecked(i == index);
  }

  emit currentChanged(index);
}

void VerticalTabWidget::updateTabIcon(int index, const QString &iconCode) {
  if (index < 0 || index >= m_tabButtons.size()) {
    return;
  }

  QPushButton *tabBtn = m_tabButtons[index];
  if (tabBtn && !iconCode.isEmpty()) {
    try {
      QFont iconFont = CIconFont::instance()->getIconFont(48);
      tabBtn->setFont(iconFont);
      tabBtn->setText(iconCode);
    } catch (...) {
      // 图标加载失败，保持原样
    }
  }
}

void VerticalTabWidget::updateTabStyles() {
  for (int i = 0; i < m_tabButtons.size(); ++i) {
    m_tabButtons[i]->setChecked(i == m_currentIndex);
  }
}

QWidget *VerticalTabWidget::widget(int index) const {
  return m_stackedWidget->widget(index);
}
