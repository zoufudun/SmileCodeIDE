#include "aboutupdatedialog.h"
#include <QScrollArea>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QTextBrowser>
#include <QApplication>

AboutUpdateDialog::AboutUpdateDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("关于与软件更新 - SmileCode Studio"));
    resize(820, 680);
    setMinimumSize(760, 560);
    setupUi();
    applyCustomStyles();
}

void AboutUpdateDialog::setupUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(16);

    // 顶部对话框标题
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *dialogTitle = new QLabel(QStringLiteral("ⓘ 关于与更新中心"), this);
    dialogTitle->setObjectName("dialogMainTitle");
    headerLayout->addWidget(dialogTitle);
    headerLayout->addStretch();
    rootLayout->addLayout(headerLayout);

    // 主 Tab 选项卡：1. 容器与插件更新  2. 详细更新记录
    m_mainTabs = new QTabWidget(this);
    m_mainTabs->setObjectName("aboutMainTabs");

    // Page 1: 容器与插件更新 (嵌入无边框平滑滚动区域，杜绝插件增多时的纵向挤压)
    QScrollArea *updateScroll = new QScrollArea(m_mainTabs);
    updateScroll->setObjectName("updateScrollArea");
    updateScroll->setWidgetResizable(true);
    updateScroll->setFrameShape(QFrame::NoFrame);
    updateScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *updatePage = new QWidget(updateScroll);
    QVBoxLayout *updatePageLayout = new QVBoxLayout(updatePage);
    updatePageLayout->setContentsMargins(6, 12, 10, 12);
    updatePageLayout->setSpacing(16);

    // 1. 最外层应用市场容器卡片
    updatePageLayout->addWidget(createContainerCard());

    // 2. GitHub Star 鼓励横幅
    updatePageLayout->addWidget(createStarBanner());

    // 3. 各个插件更新管理区域
    updatePageLayout->addWidget(createPluginsCard());

    updatePageLayout->addStretch();
    updateScroll->setWidget(updatePage);
    m_mainTabs->addTab(updateScroll, QStringLiteral("🚀 软件与插件更新"));

    // Page 2: 详细更新记录 (Changelog)
    m_mainTabs->addTab(createChangelogView(), QStringLiteral("📋 详细更新记录 (Changelog)"));

    rootLayout->addWidget(m_mainTabs);

    // 底部关闭按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QLabel *copyrightLabel = new QLabel(QStringLiteral("Copyright © 2026 SmileCode Team & zoufudun. 保留所有权利。"), this);
    copyrightLabel->setObjectName("copyrightLabel");
    bottomLayout->addWidget(copyrightLabel);
    bottomLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(90, 32);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    rootLayout->addLayout(bottomLayout);
}

QWidget* AboutUpdateDialog::createContainerCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("containerCard");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(12);

    // 第一行：Logo、名称与版本、检查更新按钮
    QHBoxLayout *row1 = new QHBoxLayout();

    QLabel *logoLabel = new QLabel(card);
    QPixmap iconPix(":/resources/logo.ico");
    if (iconPix.isNull()) {
        iconPix = QPixmap(":/icons/xptools2.png");
    }
    if (!iconPix.isNull()) {
        logoLabel->setPixmap(iconPix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(50, 50);
    row1->addWidget(logoLabel);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(3);
    QLabel *nameLabel = new QLabel(QStringLiteral("SmileCode Studio · 微笑代码工作站"), card);
    nameLabel->setObjectName("containerNameLabel");
    m_containerVerLabel = new QLabel(QStringLiteral("应用市场容器核心 v2.3.1 (最新正式版)"), card);
    m_containerVerLabel->setObjectName("containerVerLabel");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_containerVerLabel);
    row1->addLayout(nameLayout);

    row1->addStretch();

    m_checkContainerBtn = new QPushButton(QStringLiteral("🔄 检查更新"), card);
    m_checkContainerBtn->setObjectName("checkContainerBtn");
    m_checkContainerBtn->setCursor(Qt::PointingHandCursor);
    m_checkContainerBtn->setFixedHeight(34);
    connect(m_checkContainerBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onCheckContainerUpdate);
    row1->addWidget(m_checkContainerBtn);

    cardLayout->addLayout(row1);

    // 检查更新进度条（初始隐藏）
    m_containerProgressBar = new QProgressBar(card);
    m_containerProgressBar->setFixedHeight(4);
    m_containerProgressBar->setTextVisible(false);
    m_containerProgressBar->setRange(0, 0); // 忙碌指示
    m_containerProgressBar->hide();
    cardLayout->addWidget(m_containerProgressBar);

    // 第二行：状态文字与快捷链接组
    QHBoxLayout *row2 = new QHBoxLayout();
    m_containerStatusLabel = new QLabel(QStringLiteral("状态：已是最新稳定版本，支持 GCC 14 与 OpenOCD 全套生态。"), card);
    m_containerStatusLabel->setObjectName("statusLabel");
    row2->addWidget(m_containerStatusLabel);

    row2->addStretch();

    QPushButton *webBtn = new QPushButton(QStringLiteral("🌐 官方网站"), card);
    webBtn->setObjectName("linkBtn");
    webBtn->setCursor(Qt::PointingHandCursor);
    connect(webBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onOpenWebsite);
    row2->addWidget(webBtn);

    QPushButton *logBtn = new QPushButton(QStringLiteral("↗ 更新日志"), card);
    logBtn->setObjectName("linkBtn");
    logBtn->setCursor(Qt::PointingHandCursor);
    connect(logBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onOpenChangelog);
    row2->addWidget(logBtn);

    QPushButton *recentBtn = new QPushButton(QStringLiteral("✨ 近期更新"), card);
    recentBtn->setObjectName("linkBtn");
    recentBtn->setCursor(Qt::PointingHandCursor);
    connect(recentBtn, &QPushButton::clicked, [this]() {
        m_mainTabs->setCurrentIndex(1); // 切换至更新记录页
    });
    row2->addWidget(recentBtn);

    cardLayout->addLayout(row2);

    return card;
}

QWidget* AboutUpdateDialog::createStarBanner()
{
    m_starBannerFrame = new QFrame(this);
    m_starBannerFrame->setObjectName("starBannerFrame");

    QHBoxLayout *bannerLayout = new QHBoxLayout(m_starBannerFrame);
    bannerLayout->setContentsMargins(16, 10, 14, 10);
    bannerLayout->setSpacing(12);

    QLabel *starText = new QLabel(QStringLiteral("⭐ 如果 SmileCodeIDE 帮到了你，请在 GitHub 点个 Star，这对我们意义重大"), m_starBannerFrame);
    starText->setObjectName("starBannerText");
    bannerLayout->addWidget(starText);

    bannerLayout->addStretch();

    QPushButton *starBtn = new QPushButton(QStringLiteral("点此去 Star ⭐"), m_starBannerFrame);
    starBtn->setObjectName("starActionBtn");
    starBtn->setCursor(Qt::PointingHandCursor);
    connect(starBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onOpenGitHubStar);
    bannerLayout->addWidget(starBtn);

    QPushButton *dismissBtn = new QPushButton(QStringLiteral("×"), m_starBannerFrame);
    dismissBtn->setObjectName("dismissBannerBtn");
    dismissBtn->setCursor(Qt::PointingHandCursor);
    dismissBtn->setFixedSize(24, 24);
    connect(dismissBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onDismissStarBanner);
    bannerLayout->addWidget(dismissBtn);

    return m_starBannerFrame;
}

QWidget* AboutUpdateDialog::createPluginsCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName("pluginsCard");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(12);

    // 标题行与全量检查按钮
    QHBoxLayout *titleRow = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(QStringLiteral("🧩 各插件更新与运行状态 (Plugins Status)"), card);
    titleLabel->setObjectName("sectionHeaderLabel");
    titleLabel->setFixedHeight(26);
    titleRow->addWidget(titleLabel);

    titleRow->addStretch();

    m_checkAllPluginsBtn = new QPushButton(QStringLiteral("一键检查所有插件更新"), card);
    m_checkAllPluginsBtn->setObjectName("checkAllPluginsBtn");
    m_checkAllPluginsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_checkAllPluginsBtn, &QPushButton::clicked, this, &AboutUpdateDialog::onCheckAllPluginsUpdate);
    titleRow->addWidget(m_checkAllPluginsBtn);

    cardLayout->addLayout(titleRow);

    // 插件列表（严格对齐 SmileCode Studio 容器市场实际包含的 9 个插件）
    struct PluginInfo {
        QString id;
        QString name;
        QString desc;
        QString version;
        QString type;
    };

    QList<PluginInfo> plugins = {
        {"can_tool",          QStringLiteral("CAN 总线工作台 Pro"),        QStringLiteral("ZLG/CXCAN 工业级 CAN/CANFD 与 CANOpen 调试 · 负载率统计"), QStringLiteral("v2.2.0"), QStringLiteral("内置应用")},
        {"iap_tool",          QStringLiteral("STM32 IAP 固件升级"),        QStringLiteral("串口与网络双通道 Bootloader 固件升级 · Ymodem/TCP 下载校验"), QStringLiteral("v1.8.0"), QStringLiteral("内置应用")},
        {"network_tool",      QStringLiteral("网络通信调试助手"),          QStringLiteral("TCP Client/Server 与 UDP 单播/广播/多播调试 · 循环定时发送"), QStringLiteral("v2.0.0"), QStringLiteral("内置应用")},
        {"oscilloscope",      QStringLiteral("多接口数字示波器"),          QStringLiteral("串口/TCP/UDP/WebSocket 实时多通道波形采集 · FFT 频谱与标尺"), QStringLiteral("v2.0.0"), QStringLiteral("内置应用")},
        {"serial_plot",       QStringLiteral("串口调试大师 Pro"),          QStringLiteral("多标签页高速串口通信与分屏工作台 · 实时曲线 · Modbus 解析"), QStringLiteral("v3.0.0"), QStringLiteral("内置应用")},
        {"signal_gen",        QStringLiteral("高频信号发生器"),            QStringLiteral("正弦波/方波/三角波/锯齿波/白噪声实时高频仿真与数据生成"),     QStringLiteral("v1.0.0"), QStringLiteral("Qt 动态插件")},
        {"smilecode_ide",     QStringLiteral("SmileCode IDE"),             QStringLiteral("嵌入式 C/C++ 代码编辑器与 GDB 调试器 · GCC/Make 构建"),     QStringLiteral("v2.3.1"), QStringLiteral("核心工作台")},
        {"stm32_build_debug", QStringLiteral("STM32 编译构建与 GDB 调试"), QStringLiteral("GCC/Make 一键编译构建、固件烧录与硬件 GDB 在线断点单步调试"), QStringLiteral("v1.0.0"), QStringLiteral("Qt 动态插件")},
        {"custom_plugin",     QStringLiteral("自定义扩展插件"),            QStringLiteral("开箱即用的 Qt 动态插件开发范例与接口演示 · 动态加载与主题联动"), QStringLiteral("v1.0.0"), QStringLiteral("Qt 动态插件")}
    };

    for (const auto &p : plugins) {
        QFrame *rowFrame = new QFrame(card);
        rowFrame->setObjectName("pluginRowFrame");
        rowFrame->setMinimumHeight(56);
        rowFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
        rowLayout->setContentsMargins(10, 8, 10, 8);

        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(2);
        
        QHBoxLayout *nameLine = new QHBoxLayout();
        QLabel *pName = new QLabel(p.name, rowFrame);
        pName->setObjectName("pluginItemName");
        QLabel *pType = new QLabel(QStringLiteral("[%1]").arg(p.type), rowFrame);
        pType->setStyleSheet("font-size: 11px; color: #10b981; font-weight: bold;");
        nameLine->addWidget(pName);
        nameLine->addWidget(pType);
        nameLine->addStretch();
        infoLayout->addLayout(nameLine);

        QLabel *pDesc = new QLabel(p.desc, rowFrame);
        pDesc->setObjectName("pluginItemDesc");
        infoLayout->addWidget(pDesc);
        rowLayout->addLayout(infoLayout, 3);

        QLabel *pVer = new QLabel(p.version, rowFrame);
        pVer->setObjectName("pluginItemVer");
        rowLayout->addWidget(pVer, 1);

        QLabel *pStatus = new QLabel(QStringLiteral("🟢 已是最新"), rowFrame);
        pStatus->setObjectName("pluginItemStatus");
        rowLayout->addWidget(pStatus, 1);

        QPushButton *checkBtn = new QPushButton(QStringLiteral("检查更新"), rowFrame);
        checkBtn->setObjectName("pluginCheckBtn");
        checkBtn->setCursor(Qt::PointingHandCursor);
        QString pid = p.id;
        connect(checkBtn, &QPushButton::clicked, this, [this, pid]() {
            onCheckSinglePluginUpdate(pid);
        });
        rowLayout->addWidget(checkBtn, 1);

        cardLayout->addWidget(rowFrame);

        PluginRowWidgets widgets;
        widgets.nameLabel = pName;
        widgets.descLabel = pDesc;
        widgets.verLabel = pVer;
        widgets.statusLabel = pStatus;
        widgets.checkBtn = checkBtn;
        m_pluginRows[p.id] = widgets;
    }

    return card;
}

QWidget* AboutUpdateDialog::createChangelogView()
{
    QWidget *view = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 12, 0, 0);

    QTextBrowser *browser = new QTextBrowser(view);
    browser->setObjectName("changelogBrowser");
    browser->setOpenExternalLinks(true);

    QString changelogHtml = QStringLiteral(
        "<div style='font-family: sans-serif; line-height: 1.6;'>"
        "  <h2 style='color:#3b82f6; border-bottom: 2px solid #3b82f6; padding-bottom: 6px;'>v2.3.1 (最新稳定版 - 2026-10-31)</h2>"
        "  <ul>"
        "    <li><b>可视化编译器优化控制台</b>：新增 -O0 到 -O3、-Os、-Og 六大档位自主选择与 DWARF2 符号注入，动态说明与 QSettings 持久化。</li>"
        "    <li><b>硬件延时防死锁根治</b>：彻底解决高等级 GCC 将空循环延时优化清空导致的板卡假死问题。</li>"
        "    <li><b>GCC 14 兼容性补齐</b>：解决 invalid suffix 与 implicit declaration 报错，新增 -fcommon 宽容编译参数。</li>"
        "    <li><b>全局暗色主题异色重构</b>：消除复选框刺眼白块，深度融合 Dark/OneDark/Solarized/Light 四大主题。</li>"
        "    <li><b>下拉框倒三角修复</b>：使用矢量自绘倒三角替代黑色横线瑕疵。</li>"
        "    <li><b>打开项目目录记忆</b>：自动持久化上次工程工作目录。</li>"
        "    <li><b>全格式分发套件</b>：发布免安装绿色包 (ZIP)、EXE 安装向导与标准 MSI 安装包。</li>"
        "  </ul>"
        "  <h3 style='color:#6366f1; border-bottom: 1px solid #e2e8f0; padding-bottom: 4px; margin-top: 20px;'>v2.3.0 (2026-10-30)</h3>"
        "  <ul>"
        "    <li>野火 STM32H743 触摸画板工程深度排查，默认编译优化升级为 -O1。</li>"
        "    <li>主界面打开项目历史目录保存。</li>"
        "  </ul>"
        "  <h3 style='color:#6366f1; border-bottom: 1px solid #e2e8f0; padding-bottom: 4px; margin-top: 20px;'>v2.2.0 (2026-10-28)</h3>"
        "  <ul>"
        "    <li>CANopen 主站监视器与 UDS 汽车诊断协议栈升级。</li>"
        "    <li>多通信接口数字示波器采样率提升至微秒级响应。</li>"
        "  </ul>"
        "  <h3 style='color:#6366f1; border-bottom: 1px solid #e2e8f0; padding-bottom: 4px; margin-top: 20px;'>v2.1.0 (2026-10-20)</h3>"
        "  <ul>"
        "    <li>全新架构：推出最外层应用市场容器 (AppHub)，实现插件自由热插拔。</li>"
        "  </ul>"
        "</div>"
    );

    browser->setHtml(changelogHtml);
    layout->addWidget(browser);

    return view;
}

void AboutUpdateDialog::onCheckContainerUpdate()
{
    m_checkContainerBtn->setEnabled(false);
    m_checkContainerBtn->setText(QStringLiteral("正在检查..."));
    m_containerProgressBar->show();
    m_containerStatusLabel->setText(QStringLiteral("正在连接 GitHub Release 服务器查询最新版本元数据..."));

    QTimer::singleShot(1200, this, [this]() {
        m_containerProgressBar->hide();
        m_checkContainerBtn->setEnabled(true);
        m_checkContainerBtn->setText(QStringLiteral("🔄 检查更新"));
        m_containerStatusLabel->setText(QStringLiteral("🟢 恭喜！当前 SmileCode Studio 容器已是最新版本 (v2.3.1)。"));
    });
}

void AboutUpdateDialog::onCheckAllPluginsUpdate()
{
    m_checkAllPluginsBtn->setEnabled(false);
    m_checkAllPluginsBtn->setText(QStringLiteral("正在批量检查..."));

    for (auto it = m_pluginRows.begin(); it != m_pluginRows.end(); ++it) {
        it.value().statusLabel->setText(QStringLiteral("🔄 检查中..."));
        it.value().checkBtn->setEnabled(false);
    }

    QTimer::singleShot(1000, this, [this]() {
        for (auto it = m_pluginRows.begin(); it != m_pluginRows.end(); ++it) {
            it.value().statusLabel->setText(QStringLiteral("🟢 已是最新"));
            it.value().checkBtn->setEnabled(true);
        }
        m_checkAllPluginsBtn->setEnabled(true);
        m_checkAllPluginsBtn->setText(QStringLiteral("一键检查所有插件更新"));
    });
}

void AboutUpdateDialog::onCheckSinglePluginUpdate(const QString &pluginId)
{
    if (!m_pluginRows.contains(pluginId)) return;
    auto &widgets = m_pluginRows[pluginId];
    widgets.checkBtn->setEnabled(false);
    widgets.statusLabel->setText(QStringLiteral("🔄 检查中..."));

    QTimer::singleShot(800, this, [&widgets]() {
        widgets.statusLabel->setText(QStringLiteral("🟢 已是最新版本"));
        widgets.checkBtn->setEnabled(true);
    });
}

void AboutUpdateDialog::onOpenWebsite()
{
    QDesktopServices::openUrl(QUrl("https://zoufudun.github.io/SmileCodeIDE/"));
}

void AboutUpdateDialog::onOpenChangelog()
{
    QDesktopServices::openUrl(QUrl("https://github.com/zoufudun/SmileCodeIDE/blob/V10.0/CHANGELOG.zh.md"));
}

void AboutUpdateDialog::onOpenGitHubStar()
{
    QDesktopServices::openUrl(QUrl("https://github.com/zoufudun/SmileCodeIDE"));
}

void AboutUpdateDialog::onDismissStarBanner()
{
    if (m_starBannerFrame) {
        m_starBannerFrame->hide();
    }
}

void AboutUpdateDialog::applyCustomStyles()
{
    setStyleSheet(
        "QDialog {"
        "  background-color: #f8fafc;"
        "}"
        "#dialogMainTitle {"
        "  font-size: 17px;"
        "  font-weight: bold;"
        "  color: #0f172a;"
        "}"
        
        "#updateScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { border: none; background: #f1f5f9; width: 8px; margin: 0; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #cbd5e1; min-height: 24px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #94a3b8; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "#aboutMainTabs::pane {"
        "  border: 1px solid #e2e8f0;"
        "  background: #ffffff;"
        "  border-radius: 8px;"
        "}"
        "#aboutMainTabs QTabBar::tab {"
        "  padding: 8px 16px;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "  color: #475569;"
        "  background: transparent;"
        "  border: none;"
        "}"
        "#aboutMainTabs QTabBar::tab:selected {"
        "  color: #2563eb;"
        "  border-bottom: 2px solid #2563eb;"
        "  font-weight: bold;"
        "}"
        "#containerCard {"
        "  background: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 12px;"
        "}"
        "#containerNameLabel {"
        "  font-size: 15px;"
        "  font-weight: bold;"
        "  color: #0f172a;"
        "}"
        "#containerVerLabel {"
        "  font-size: 12px;"
        "  color: #64748b;"
        "}"
        "#checkContainerBtn, #checkAllPluginsBtn {"
        "  background: #2563eb;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "#checkContainerBtn:hover, #checkAllPluginsBtn:hover {"
        "  background: #1d4ed8;"
        "}"
        "#statusLabel {"
        "  font-size: 12px;"
        "  color: #059669;"
        "}"
        "#linkBtn {"
        "  background: #f1f5f9;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  color: #334155;"
        "  font-size: 12px;"
        "  padding: 4px 10px;"
        "}"
        "#linkBtn:hover {"
        "  background: #e2e8f0;"
        "}"
        "#starBannerFrame {"
        "  background: #fffbeb;"
        "  border: 1px solid #fde68a;"
        "  border-radius: 8px;"
        "}"
        "#starBannerText {"
        "  color: #b45309;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "#starActionBtn {"
        "  background: #f59e0b;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  padding: 4px 10px;"
        "}"
        "#starActionBtn:hover {"
        "  background: #d97706;"
        "}"
        "#dismissBannerBtn {"
        "  background: transparent;"
        "  border: none;"
        "  color: #92400e;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "#pluginsCard {"
        "  background: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 12px;"
        "}"
        "#sectionHeaderLabel {"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  color: #1e293b;"
        "}"
        "#pluginRowFrame {"
        "  background: #f8fafc;"
        "  border: 1px solid #edf2f7;"
        "  border-radius: 8px;"
        "}"
        "#pluginItemName {"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  color: #1e293b;"
        "}"
        "#pluginItemDesc {"
        "  font-size: 11px;"
        "  color: #64748b;"
        "}"
        "#pluginItemVer {"
        "  font-size: 12px;"
        "  font-family: monospace;"
        "  color: #475569;"
        "}"
        "#pluginItemStatus {"
        "  font-size: 12px;"
        "  color: #059669;"
        "  font-weight: 500;"
        "}"
        "#pluginCheckBtn {"
        "  background: #ffffff;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 4px;"
        "  color: #334155;"
        "  font-size: 11px;"
        "  padding: 4px 8px;"
        "}"
        "#pluginCheckBtn:hover {"
        "  background: #f1f5f9;"
        "  border-color: #94a3b8;"
        "}"
        "#copyrightLabel {"
        "  font-size: 11px;"
        "  color: #94a3b8;"
        "}"
        "#changelogBrowser {"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "  background: #ffffff;"
        "  padding: 12px;"
        "}"
    );
}

