#include "apphubwindow.h"
#include "pluginmanagerdialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QFile>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QDebug>

AppHubWindow::AppHubWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("SmileCode Studio - 嵌入式开发与调试工具应用工作台"));
    setWindowIcon(QIcon(":/resources/logo.png"));
    resize(1200, 780);
    setMinimumSize(960, 620);

    setupUi();
    setupTrayIcon();

    // 监听应用状态变更
    connect(AppManager::instance(), &AppManager::appStatusChanged, this, &AppHubWindow::onAppStatusChanged);
    connect(AppManager::instance(), &AppManager::pluginListChanged, this, &AppHubWindow::refreshAppGrid);
    connect(AppManager::instance(), &AppManager::appFavoriteChanged, this, [this](const QString &, bool) {
        if (m_currentCategory == AppCategory::Favorites) {
            refreshAppGrid();
        }
    });

    // 初始载入
    refreshAppGrid();
}

AppHubWindow::~AppHubWindow() {
}

void AppHubWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    setupHeader();
    rootLayout->addWidget(m_searchEdit->parentWidget()); // 顶栏

    // 中部主要分割区（左侧导航 + 右侧卡片网格）
    QWidget *middleWidget = new QWidget(this);
    QHBoxLayout *middleLayout = new QHBoxLayout(middleWidget);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    setupSidebar();
    setupCentralArea();

    middleLayout->addWidget(m_sidebarList);
    middleLayout->addWidget(m_scrollArea, 1);

    rootLayout->addWidget(middleWidget, 1);

    setupFooter();

    // 快捷键 Ctrl+F 聚焦搜索框
    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });
}

void AppHubWindow::setupHeader() {
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(64);
    headerWidget->setStyleSheet("background-color: rgba(30, 39, 46, 0.95); border-bottom: 1px solid rgba(255,255,255,0.08);");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(16);

    // 1. 品牌 Logo 与标题
    QLabel *logoLabel = new QLabel(headerWidget);
    QPixmap logoPix(":/resources/logo.png");
    if (!logoPix.isNull()) {
        logoLabel->setPixmap(logoPix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(36, 36);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    QLabel *brandLabel = new QLabel(QStringLiteral("SmileCode Studio"), headerWidget);
    brandLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    QLabel *sloganLabel = new QLabel(QStringLiteral("嵌入式开发与调试工具应用市场容器"), headerWidget);
    sloganLabel->setStyleSheet("font-size: 11px; color: #a4b0be;");
    titleLayout->addWidget(brandLabel);
    titleLayout->addWidget(sloganLabel);

    // 2. 全局搜索栏
    m_searchEdit = new QLineEdit(headerWidget);
    m_searchEdit->setPlaceholderText(QStringLiteral("🔍 搜索应用、工具、通信协议关键字 (Ctrl+F)..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(380);
    m_searchEdit->setFixedHeight(34);
    m_searchEdit->setStyleSheet(
        "QLineEdit { "
        "  background-color: rgba(255, 255, 255, 0.08); "
        "  color: #ffffff; "
        "  border: 1px solid rgba(255, 255, 255, 0.15); "
        "  border-radius: 17px; "
        "  padding: 0 16px; "
        "  font-size: 13px; "
        "} "
        "QLineEdit:focus { border: 1px solid #74b9ff; background-color: rgba(255, 255, 255, 0.12); }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AppHubWindow::onSearchTextChanged);

    // 3. 插件中心按钮
    QPushButton *pluginBtn = new QPushButton(QStringLiteral("🧩 扩展与插件中心"), headerWidget);
    pluginBtn->setCursor(Qt::PointingHandCursor);
    pluginBtn->setFixedHeight(32);
    pluginBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #6c5ce7; "
        "  color: white; "
        "  font-weight: bold; "
        "  border: none; "
        "  border-radius: 6px; "
        "  padding: 0 14px; "
        "  font-size: 12px; "
        "} "
        "QPushButton:hover { background-color: #a29bfe; }");
    connect(pluginBtn, &QPushButton::clicked, this, &AppHubWindow::onOpenPluginManagerClicked);

    // 4. 主题切换器
    m_themeCombo = new QComboBox(headerWidget);
    m_themeCombo->setFixedHeight(32);
    m_themeCombo->setStyleSheet("QComboBox { border-radius: 6px; padding: 0 10px; font-size: 12px; }");
    m_themeCombo->addItem(QStringLiteral("🌙 深色模式 (Dark)"), "dark");
    m_themeCombo->addItem(QStringLiteral("☀️ 浅色模式 (Light)"), "light");
    m_themeCombo->addItem(QStringLiteral("🟣 OneDark Pro"), "onedarkpro");
    m_themeCombo->addItem(QStringLiteral("🧛 Dracula"), "dracula");
    m_themeCombo->addItem(QStringLiteral("🐙 GitHub Dark"), "githubdark");
    m_themeCombo->addItem(QStringLiteral("🌲 Nord"), "nord");
    m_themeCombo->addItem(QStringLiteral("🌿 Vue Theme"), "vue");
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppHubWindow::onThemeChanged);

    // 5. 关于按钮
    QPushButton *aboutBtn = new QPushButton(QStringLiteral("ℹ️ 关于"), headerWidget);
    aboutBtn->setCursor(Qt::PointingHandCursor);
    aboutBtn->setFixedHeight(32);
    aboutBtn->setStyleSheet("QPushButton { border-radius: 6px; padding: 0 10px; font-size: 12px; }");
    connect(aboutBtn, &QPushButton::clicked, this, &AppHubWindow::onShowAboutClicked);

    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(m_searchEdit);
    headerLayout->addStretch();
    headerLayout->addWidget(pluginBtn);
    headerLayout->addWidget(m_themeCombo);
    headerLayout->addWidget(aboutBtn);
}

void AppHubWindow::setupSidebar() {
    m_sidebarList = new QListWidget(this);
    m_sidebarList->setFixedWidth(210);
    m_sidebarList->setFrameShape(QFrame::NoFrame);
    m_sidebarList->setStyleSheet(
        "QListWidget { "
        "  background-color: rgba(24, 28, 34, 0.95); "
        "  border-right: 1px solid rgba(255,255,255,0.06); "
        "  outline: none; "
        "  padding-top: 12px; "
        "} "
        "QListWidget::item { "
        "  height: 42px; "
        "  color: #a4b0be; "
        "  padding-left: 16px; "
        "  margin: 2px 8px; "
        "  border-radius: 8px; "
        "  font-size: 13px; "
        "} "
        "QListWidget::item:hover { "
        "  background-color: rgba(255, 255, 255, 0.05); "
        "  color: #ffffff; "
        "} "
        "QListWidget::item:selected { "
        "  background-color: #3867d6; "
        "  color: #ffffff; "
        "  font-weight: bold; "
        "}");

    m_sidebarList->addItem(QStringLiteral("🏠 全部应用"));
    m_sidebarList->addItem(QStringLiteral("⭐ 常用推荐"));
    m_sidebarList->addItem(QStringLiteral("💻 嵌入式开发"));
    m_sidebarList->addItem(QStringLiteral("📡 总线与通信"));
    m_sidebarList->addItem(QStringLiteral("📊 测量与分析"));
    m_sidebarList->addItem(QStringLiteral("🚀 固件与烧录"));
    m_sidebarList->addItem(QStringLiteral("🧩 插件与扩展"));

    m_sidebarList->setCurrentRow(0);
    connect(m_sidebarList, &QListWidget::currentRowChanged, this, &AppHubWindow::onCategorySelected);
}

void AppHubWindow::setupCentralArea() {
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: rgba(18, 22, 28, 0.98); }");

    m_gridContainer = new QWidget(m_scrollArea);
    m_gridContainer->setStyleSheet("background-color: transparent;");

    QVBoxLayout *containerLayout = new QVBoxLayout(m_gridContainer);
    containerLayout->setContentsMargins(24, 20, 24, 20);
    containerLayout->setSpacing(20);

    // ==========================================
    // 顶部 Hero 快捷推荐卡片
    // ==========================================
    QFrame *heroFrame = new QFrame(m_gridContainer);
    heroFrame->setObjectName("HeroBanner");
    heroFrame->setStyleSheet(
        "#HeroBanner { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2b5876, stop:1 #4e4376); "
        "  border-radius: 14px; "
        "  padding: 16px; "
        "}");
    heroFrame->setFixedHeight(110);

    QHBoxLayout *heroLayout = new QHBoxLayout(heroFrame);
    heroLayout->setContentsMargins(20, 10, 20, 10);
    heroLayout->setSpacing(16);

    QVBoxLayout *heroTextLayout = new QVBoxLayout();
    QLabel *heroTitle = new QLabel(QStringLiteral("✨ SmileCode 一站式嵌入式开发工作台"), heroFrame);
    heroTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: white;");
    QLabel *heroSub = new QLabel(QStringLiteral("集合代码编辑、总线调试、实时波形测量与固件烧录，支持模块化插件扩展。"), heroFrame);
    heroSub->setStyleSheet("font-size: 12px; color: #dcdde1;");
    heroTextLayout->addWidget(heroTitle);
    heroTextLayout->addWidget(heroSub);

    QPushButton *quickIdeBtn = new QPushButton(QStringLiteral("⚡ 启动代码编辑器"), heroFrame);
    quickIdeBtn->setCursor(Qt::PointingHandCursor);
    quickIdeBtn->setStyleSheet("background-color: #ffffff; color: #2f3542; font-weight: bold; padding: 8px 16px; border-radius: 6px;");
    connect(quickIdeBtn, &QPushButton::clicked, this, [this]() {
        AppManager::instance()->launchApp("smilecode_ide");
    });

    heroLayout->addLayout(heroTextLayout, 1);
    heroLayout->addWidget(quickIdeBtn);

    containerLayout->addWidget(heroFrame);

    // ==========================================
    // 卡片网格布局
    // ==========================================
    m_gridLayout = new QGridLayout();
    m_gridLayout->setSpacing(16);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addLayout(m_gridLayout);

    // 空状态提示
    m_emptyStateLabel = new QLabel(QStringLiteral("未找到匹配的应用，请检查搜索关键字或尝试在“扩展中心”安装插件"), m_gridContainer);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setStyleSheet("font-size: 14px; color: #7f8c8d; padding: 40px;");
    m_emptyStateLabel->setVisible(false);
    containerLayout->addWidget(m_emptyStateLabel);

    containerLayout->addStretch();

    m_scrollArea->setWidget(m_gridContainer);
}

void AppHubWindow::setupFooter() {
    QWidget *footerWidget = new QWidget(this);
    footerWidget->setFixedHeight(32);
    footerWidget->setStyleSheet("background-color: rgba(24, 28, 34, 0.95); border-top: 1px solid rgba(255,255,255,0.06);");

    QHBoxLayout *footerLayout = new QHBoxLayout(footerWidget);
    footerLayout->setContentsMargins(16, 0, 16, 0);

    m_statusRunningLabel = new QLabel(QStringLiteral("🟢 运行中应用: 0 个"), footerWidget);
    m_statusRunningLabel->setStyleSheet("color: #2ed573; font-size: 11px;");

    m_statusTotalLabel = new QLabel(QStringLiteral("📦 已集成应用: 0 个"), footerWidget);
    m_statusTotalLabel->setStyleSheet("color: #747d8c; font-size: 11px;");

    QLabel *verLabel = new QLabel(QStringLiteral("SmileCode Studio v2.1.0"), footerWidget);
    verLabel->setStyleSheet("color: #747d8c; font-size: 11px;");

    footerLayout->addWidget(m_statusRunningLabel);
    footerLayout->addSpacing(16);
    footerLayout->addWidget(m_statusTotalLabel);
    footerLayout->addStretch();
    footerLayout->addWidget(verLabel);

    centralWidget()->layout()->addWidget(footerWidget);
}

void AppHubWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/resources/logo.png"));
    m_trayIcon->setToolTip(QStringLiteral("SmileCode Studio 应用工作台"));

    m_trayMenu = new QMenu(this);
    QAction *showHubAction = m_trayMenu->addAction(QStringLiteral("🏠 显示应用工作台"));
    connect(showHubAction, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });

    m_trayMenu->addSeparator();

    QAction *ideAction = m_trayMenu->addAction(QStringLiteral("💻 SmileCode 代码编辑器"));
    connect(ideAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("smilecode_ide"); });

    QAction *scopeAction = m_trayMenu->addAction(QStringLiteral("📊 多通信接口示波器"));
    connect(scopeAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("oscilloscope"); });

    QAction *serialAction = m_trayMenu->addAction(QStringLiteral("📟 串口调试助手"));
    connect(serialAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("serial_plot"); });

    QAction *canAction = m_trayMenu->addAction(QStringLiteral("🚗 CAN 总线调试助手"));
    connect(canAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("can_tool"); });

    QAction *netAction = m_trayMenu->addAction(QStringLiteral("🌐 网络通信调试助手"));
    connect(netAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("network_tool"); });

    QAction *iapAction = m_trayMenu->addAction(QStringLiteral("🚀 STM32 IAP 固件升级"));
    connect(iapAction, &QAction::triggered, this, []() { AppManager::instance()->launchApp("iap_tool"); });

    m_trayMenu->addSeparator();

    QAction *exitAction = m_trayMenu->addAction(QStringLiteral("❌ 退出程序"));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &AppHubWindow::onTrayIconActivated);
    m_trayIcon->show();
}

void AppHubWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        showNormal();
        raise();
        activateWindow();
    }
}

void AppHubWindow::onCategorySelected(int row) {
    switch (row) {
    case 0: m_currentCategory = AppCategory::All; break;
    case 1: m_currentCategory = AppCategory::Favorites; break;
    case 2: m_currentCategory = AppCategory::EmbeddedDev; break;
    case 3: m_currentCategory = AppCategory::BusProtocol; break;
    case 4: m_currentCategory = AppCategory::Measurement; break;
    case 5: m_currentCategory = AppCategory::Flashing; break;
    case 6: m_currentCategory = AppCategory::PluginExtensions; break;
    default: m_currentCategory = AppCategory::All; break;
    }
    refreshAppGrid();
}

void AppHubWindow::onSearchTextChanged(const QString &text) {
    m_searchKeyword = text;
    refreshAppGrid();
}

void AppHubWindow::onThemeChanged(int index) {
    QString themeKey = m_themeCombo->itemData(index).toString();
    QString sheetPath;

    if (themeKey == "dark") sheetPath = ":/resources/styles/dark.qss";
    else if (themeKey == "light") sheetPath = ":/resources/styles/light.qss";
    else if (themeKey == "onedarkpro") sheetPath = ":/resources/styles/onedarkpro.qss";
    else if (themeKey == "dracula") sheetPath = ":/resources/styles/dracula.qss";
    else if (themeKey == "githubdark") sheetPath = ":/resources/styles/githubdark.qss";
    else if (themeKey == "nord") sheetPath = ":/resources/styles/nord.qss";
    else if (themeKey == "vue") sheetPath = ":/resources/styles/vue.qss";

    if (!sheetPath.isEmpty()) {
        applyTheme(sheetPath);
    }
}

void AppHubWindow::applyTheme(const QString &sheetPath) {
    QFile f(sheetPath);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QString qss = QString::fromUtf8(f.readAll());
        qApp->setStyleSheet(qss);
    }
}

void AppHubWindow::onOpenPluginManagerClicked() {
    PluginManagerDialog dlg(this);
    dlg.exec();
}

void AppHubWindow::onShowAboutClicked() {
    QMessageBox::about(this, QStringLiteral("关于 SmileCode Studio"),
        QStringLiteral("<h3>SmileCode Studio v2.1.0</h3>"
                       "<p>专业嵌入式开发与多协议总线调试应用平台</p>"
                       "<p><b>集成组件：</b></p>"
                       "<ul>"
                       "<li>SmileCode IDE（嵌入式 C/C++ 代码编辑器与 GDB 调试）</li>"
                       "<li>多通信接口数字示波器（串口/TCP/UDP/WebSocket 数据波形测量）</li>"
                       "<li>串口调试助手（高速多标签页收发与分屏监视）</li>"
                       "<li>CAN 调试助手（支持 ZLG USBCANFD 与 CXCAN 及 CANopen Master）</li>"
                       "<li>网络调试助手（TCP 客户端/服务端与 UDP 调试）</li>"
                       "<li>STM32 IAP 固件升级工具（串口与网络 Bootloader 升级）</li>"
                       "<li>第三方插件与外部工具扩展中心</li>"
                       "</ul>"
                       "<p>Copyright © 2026 PhudonZou / SmileCode Team</p>"));
}

void AppHubWindow::onAppLaunched(const QString &appId, QWidget *widget) {
    Q_UNUSED(widget);
    if (m_cardMap.contains(appId)) {
        m_cardMap[appId]->setRunningState(true);
    }
    m_statusRunningLabel->setText(QStringLiteral("🟢 运行中应用: %1 个").arg(AppManager::instance()->getRunningAppCount()));
}

void AppHubWindow::onAppStatusChanged(const QString &appId, bool isRunning) {
    if (m_cardMap.contains(appId)) {
        m_cardMap[appId]->setRunningState(isRunning);
    }
    m_statusRunningLabel->setText(QStringLiteral("🟢 运行中应用: %1 个").arg(AppManager::instance()->getRunningAppCount()));
}

void AppHubWindow::refreshAppGrid() {
    // 清空现有网格
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_cardMap.clear();

    QVector<AppInfo> apps = AppManager::instance()->searchApps(m_searchKeyword, m_currentCategory);

    m_emptyStateLabel->setVisible(apps.isEmpty());

    int viewportWidth = m_scrollArea->viewport()->width();
    int cols = qMax(1, (viewportWidth - 40) / 320);
    m_lastColumnCount = cols;

    int row = 0;
    int col = 0;

    for (const AppInfo &info : apps) {
        AppCardWidget *card = new AppCardWidget(info, m_gridContainer);
        connect(card, &AppCardWidget::launchRequested, this, [](const QString &appId) {
            AppManager::instance()->launchApp(appId);
        });
        connect(card, &AppCardWidget::favoriteToggled, this, [](const QString &appId, bool isFav) {
            AppManager::instance()->setAppFavorite(appId, isFav);
        });

        m_cardMap[info.id] = card;
        m_gridLayout->addWidget(card, row, col);

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }

    m_statusTotalLabel->setText(QStringLiteral("📦 已集成应用: %1 个").arg(AppManager::instance()->getAllApps().size()));
    m_statusRunningLabel->setText(QStringLiteral("🟢 运行中应用: %1 个").arg(AppManager::instance()->getRunningAppCount()));
}

void AppHubWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    int viewportWidth = m_scrollArea->viewport()->width();
    int cols = qMax(1, (viewportWidth - 40) / 320);
    if (cols != m_lastColumnCount) {
        refreshAppGrid();
    }
}

void AppHubWindow::closeEvent(QCloseEvent *event) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(QStringLiteral("SmileCode Studio"),
            QStringLiteral("程序已最小化到系统托盘，双击托盘图标即可唤出。"),
            QSystemTrayIcon::Information, 2000);
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}
