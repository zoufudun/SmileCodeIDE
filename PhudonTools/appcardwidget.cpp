#include "appcardwidget.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

AppCardWidget::AppCardWidget(const AppInfo &info, QWidget *parent)
    : QFrame(parent), m_info(info) {
  setObjectName("AppCardWidget");
  setFrameShape(QFrame::NoFrame);
  setCursor(Qt::PointingHandCursor);
  setMinimumSize(280, 210);
  setMaximumHeight(230);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  setupUi();
  updateAppInfo(m_info);
}

void AppCardWidget::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(10);

  // ==========================================
  // 顶部行：图标 + 标题/版本/分类 + 收藏按钮
  // ==========================================
  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->setSpacing(12);

  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(48, 48);
  m_iconLabel->setAlignment(Qt::AlignCenter);

  QVBoxLayout *titleMetaLayout = new QVBoxLayout();
  titleMetaLayout->setSpacing(3);

  QHBoxLayout *titleRow = new QHBoxLayout();
  titleRow->setSpacing(6);

  m_titleLabel = new QLabel(this);
  m_titleLabel->setStyleSheet(
      "font-size: 15px; font-weight: bold; color: #ecf0f1;");

  m_versionLabel = new QLabel(this);
  m_versionLabel->setStyleSheet(
      "font-size: 11px; color: #95a5a6; background: rgba(255,255,255,0.08); "
      "padding: 2px 5px; border-radius: 4px;");

  titleRow->addWidget(m_titleLabel);
  titleRow->addWidget(m_versionLabel);
  titleRow->addStretch();

  QHBoxLayout *categoryRow = new QHBoxLayout();
  categoryRow->setSpacing(6);
  m_categoryBadge = new QLabel(this);
  m_categoryBadge->setStyleSheet(
      "font-size: 11px; color: #74b9ff; font-weight: 500;");
  categoryRow->addWidget(m_categoryBadge);
  categoryRow->addStretch();

  titleMetaLayout->addLayout(titleRow);
  titleMetaLayout->addLayout(categoryRow);

  m_favBtn = new QToolButton(this);
  m_favBtn->setCursor(Qt::PointingHandCursor);
  m_favBtn->setStyleSheet("QToolButton { border: none; background: "
                          "transparent; font-size: 16px; color: #7f8c8d; } "
                          "QToolButton:hover { color: #f1c40f; }");
  connect(m_favBtn, &QToolButton::clicked, this, [this]() {
    m_info.isFavorite = !m_info.isFavorite;
    m_favBtn->setText(m_info.isFavorite ? QStringLiteral("★")
                                        : QStringLiteral("☆"));
    m_favBtn->setStyleSheet(
        m_info.isFavorite
            ? "QToolButton { border: none; background: transparent; font-size: "
              "16px; color: #f1c40f; }"
            : "QToolButton { border: none; background: transparent; font-size: "
              "16px; color: #7f8c8d; } QToolButton:hover { color: #f1c40f; }");
    emit favoriteToggled(m_info.id, m_info.isFavorite);
  });

  topLayout->addWidget(m_iconLabel);
  topLayout->addLayout(titleMetaLayout, 1);
  topLayout->addWidget(m_favBtn, 0, Qt::AlignTop);

  // ==========================================
  // 中部：副标题与描述
  // ==========================================
  m_subtitleLabel = new QLabel(this);
  m_subtitleLabel->setStyleSheet(
      "font-size: 12px; font-weight: 600; color: #bdc3c7;");
  m_subtitleLabel->setWordWrap(true);

  m_descLabel = new QLabel(this);
  m_descLabel->setStyleSheet(
      "font-size: 11px; color: #95a5a6; line-height: 1.4;");
  m_descLabel->setWordWrap(true);
  m_descLabel->setMaximumHeight(36);

  m_tagsContainer = new QLabel(this);
  m_tagsContainer->setStyleSheet("font-size: 10px; color: #bdc3c7;");

  // ==========================================
  // 底部：运行状态指示 + 启动按钮
  // ==========================================
  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->setSpacing(8);

  QHBoxLayout *statusLayout = new QHBoxLayout();
  statusLayout->setSpacing(4);
  m_statusDot = new QLabel(this);
  m_statusDot->setFixedSize(8, 8);
  m_statusText = new QLabel(this);
  m_statusText->setStyleSheet("font-size: 11px; color: #7f8c8d;");
  statusLayout->addWidget(m_statusDot);
  statusLayout->addWidget(m_statusText);

  m_launchBtn = new QPushButton(QStringLiteral("启动应用"), this);
  m_launchBtn->setCursor(Qt::PointingHandCursor);
  m_launchBtn->setFixedHeight(28);
  m_launchBtn->setMinimumWidth(88);
  connect(m_launchBtn, &QPushButton::clicked, this,
          [this]() { emit launchRequested(m_info.id); });

  bottomLayout->addLayout(statusLayout);
  bottomLayout->addStretch();
  bottomLayout->addWidget(m_launchBtn);

  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(m_subtitleLabel);
  mainLayout->addWidget(m_descLabel);
  mainLayout->addWidget(m_tagsContainer);
  mainLayout->addStretch();
  mainLayout->addLayout(bottomLayout);

  updateStyles();
}

void AppCardWidget::updateAppInfo(const AppInfo &info) {
  m_info = info;

  m_titleLabel->setText(m_info.name);
  m_versionLabel->setText(m_info.version.isEmpty() ? "v1.0" : m_info.version);
  m_categoryBadge->setText(m_info.categoryName);
  m_subtitleLabel->setText(m_info.subtitle);
  m_descLabel->setText(m_info.description);

  // 格式化标签
  QString tagsHtml;
  for (const QString &tag : m_info.tags) {
    tagsHtml +=
        QString(
            "<span "
            "style='background:rgba(255,255,255,0.06);color:#a5b1c2;padding:"
            "2px 6px;border-radius:3px;margin-right:4px;'>%1</span> ")
            .arg(tag);
  }
  m_tagsContainer->setText(tagsHtml);

  // 收藏按钮状态
  m_favBtn->setText(m_info.isFavorite ? QStringLiteral("★")
                                      : QStringLiteral("☆"));
  m_favBtn->setStyleSheet(
      m_info.isFavorite
          ? "QToolButton { border: none; background: transparent; font-size: "
            "16px; color: #f1c40f; }"
          : "QToolButton { border: none; background: transparent; font-size: "
            "16px; color: #7f8c8d; } QToolButton:hover { color: #f1c40f; }");

  // 加载图标
  QPixmap pix;
  if (!m_info.iconPath.isEmpty() && pix.load(m_info.iconPath)) {
    m_iconLabel->setPixmap(
        pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  } else {
    // 如果没有图片图标，生成科技感首字母图标
    QPixmap defaultIcon(48, 48);
    defaultIcon.fill(Qt::transparent);
    QPainter painter(&defaultIcon);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color(m_info.colorHex.isEmpty() ? "#6c5ce7" : m_info.colorHex);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 48, 48, 12, 12);

    painter.setPen(Qt::white);
    QFont f = font();
    f.setPixelSize(20);
    f.setBold(true);
    painter.setFont(f);
    painter.drawText(defaultIcon.rect(), Qt::AlignCenter,
                     m_info.name.left(1).toUpper());
    m_iconLabel->setPixmap(defaultIcon);
  }

  setRunningState(AppManager::instance()->isAppRunning(m_info.id));
}

void AppCardWidget::setRunningState(bool isRunning) {
  m_isRunning = isRunning;
  if (m_isRunning) {
    m_statusDot->setStyleSheet(
        "background-color: #2ecc71; border-radius: 4px;");
    m_statusText->setText(QStringLiteral("运行中"));
    m_statusText->setStyleSheet(
        "font-size: 11px; color: #2ecc71; font-weight: bold;");
    m_launchBtn->setText(QStringLiteral("切换窗口"));
  } else {
    m_statusDot->setStyleSheet(
        "background-color: #7f8c8d; border-radius: 4px;");
    m_statusText->setText(QStringLiteral("就绪"));
    m_statusText->setStyleSheet("font-size: 11px; color: #7f8c8d;");
    m_launchBtn->setText(QStringLiteral("打开应用"));
  }
  updateStyles();
}

void AppCardWidget::updateStyles() {
  QString accentColor = m_info.colorHex.isEmpty() ? "#3498db" : m_info.colorHex;

  if (m_isHovered) {
    setStyleSheet(QString("#AppCardWidget { "
                          "  background-color: rgba(45, 52, 54, 0.95); "
                          "  border: 1px solid %1; "
                          "  border-radius: 12px; "
                          "}")
                      .arg(accentColor));
  } else {
    setStyleSheet("#AppCardWidget { "
                  "  background-color: rgba(36, 41, 46, 0.75); "
                  "  border: 1px solid rgba(255, 255, 255, 0.08); "
                  "  border-radius: 12px; "
                  "}");
  }

  if (m_isRunning) {
    m_launchBtn->setStyleSheet(
        QString("QPushButton { "
                "  background-color: #27ae60; "
                "  color: white; "
                "  border: none; "
                "  border-radius: 6px; "
                "  font-weight: bold; "
                "  font-size: 12px; "
                "  padding: 4px 12px; "
                "} "
                "QPushButton:hover { background-color: #2ecc71; }"));
  } else {
    m_launchBtn->setStyleSheet(QString("QPushButton { "
                                       "  background-color: %1; "
                                       "  color: white; "
                                       "  border: none; "
                                       "  border-radius: 6px; "
                                       "  font-weight: bold; "
                                       "  font-size: 12px; "
                                       "  padding: 4px 12px; "
                                       "} "
                                       "QPushButton:hover { opacity: 0.9; }")
                                   .arg(accentColor));
  }
}

void AppCardWidget::enterEvent(QEvent *event) {
  Q_UNUSED(event);
  m_isHovered = true;
  updateStyles();
}

void AppCardWidget::leaveEvent(QEvent *event) {
  Q_UNUSED(event);
  m_isHovered = false;
  updateStyles();
}

void AppCardWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    emit launchRequested(m_info.id);
  }
  QFrame::mouseDoubleClickEvent(event);
}
