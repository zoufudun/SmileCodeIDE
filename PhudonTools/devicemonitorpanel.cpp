#include "devicemonitorpanel.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QVBoxLayout>

#include "caninterface.h"
#include "canprotocolconfigdialog.h"
#include "devicestatuswidget.h"
#include "roomwidget.h"

// ========== 布局模板背景轮廓图 ==========

class TemplateBackground : public QWidget {
public:
  QString tpl;
  explicit TemplateBackground(QWidget *parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
  }
protected:
  void paintEvent(QPaintEvent *) override {
    if (tpl.isEmpty()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c(0x00, 0xD4, 0xFF, 15);
    QPen pen(c, 1.5, Qt::DotLine);
    p.setPen(pen);
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 8));

    if (tpl == QStringLiteral("submarine")) {
      // 核潜艇外形
      QPainterPath hull;
      hull.moveTo(120, 350); hull.cubicTo(50, 200, 300, 180, 500, 180);
      hull.lineTo(1400, 180); hull.cubicTo(1550, 180, 1580, 300, 1580, 350);
      hull.cubicTo(1580, 420, 1550, 540, 1400, 540);
      hull.lineTo(500, 540); hull.cubicTo(300, 540, 50, 500, 120, 350);
      p.drawPath(hull);
      // 指挥塔
      p.drawRect(400, 100, 150, 80);
      // 螺旋桨
      p.setPen(QPen(c, 2));
      p.drawLine(1560, 350, 1600, 310); p.drawLine(1560, 350, 1600, 350); p.drawLine(1560, 350, 1600, 390);
      // 隔舱
      p.setPen(pen);
      p.drawLine(350, 200, 350, 520); p.drawLine(650, 200, 650, 520);
      p.drawLine(900, 200, 900, 520); p.drawLine(1150, 200, 1150, 520);
    } else if (tpl == QStringLiteral("building")) {
      p.drawRoundedRect(80, 80, 640, 400, 8, 8);
      p.drawLine(80, 215, 720, 215); p.drawLine(80, 350, 720, 350);
      for (int x = 160; x < 720; x += 120) p.drawLine(x, 80, x, 480);
      p.drawLine(400, 80, 400, 40); p.drawEllipse(QPoint(400, 40), 6, 6);
    } else if (tpl == QStringLiteral("warship")) {
      QPainterPath ship;
      ship.moveTo(80, 250); ship.lineTo(150, 180); ship.lineTo(350, 180);
      ship.lineTo(400, 130); ship.lineTo(500, 130); ship.lineTo(550, 180);
      ship.lineTo(700, 180); ship.lineTo(750, 160); ship.lineTo(800, 160);
      ship.lineTo(850, 200); ship.lineTo(980, 200); ship.lineTo(1020, 250); ship.closeSubpath();
      p.drawPath(ship);
      p.drawLine(80, 250, 1020, 250);
      p.drawRect(200, 170, 50, 30);
      p.drawLine(450, 130, 450, 60); p.drawEllipse(QPoint(450, 55), 8, 5);
    } else if (tpl == QStringLiteral("carrier")) {
      QPainterPath deck;
      deck.moveTo(30, 220); deck.lineTo(30, 170); deck.lineTo(280, 100);
      deck.lineTo(550, 100); deck.lineTo(1010, 170); deck.lineTo(1010, 220); deck.closeSubpath();
      p.drawPath(deck);
      p.drawRect(590, 60, 160, 130);
      p.drawLine(30, 220, 1010, 220);
      for (int x = 100; x < 500; x += 80) { p.drawLine(x, 120, x - 30, 180); p.drawLine(x + 40, 120, x + 10, 180); }
    }
  }
};

// ========== 底部状态栏 (自定义绘制) ==========

class BottomStatusBar : public QWidget {
public:
  QLabel *lblClock, *lblTitle;
  explicit BottomStatusBar(QWidget *parent = nullptr) : QWidget(parent) {
    setFixedHeight(30);
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 2, 12, 2);
    lay->setSpacing(10);

    lblTitle = new QLabel(QStringLiteral("CAN2.0B Status Monitor  |  v2.0  |  Phudon"));
    lblTitle->setStyleSheet("color:#00D4FF;font-size:10px;font-weight:bold;font-family:'Consolas';");
    lay->addWidget(lblTitle);
    lay->addStretch();
    lblClock = new QLabel();
    lblClock->setStyleSheet("color:#64748B;font-size:10px;font-family:'Consolas';");
    lay->addWidget(lblClock);
  }

  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    QLinearGradient line(0, 0, width(), 0);
    line.setColorAt(0, QColor(0x00, 0xD4, 0xFF, 0));
    line.setColorAt(0.3, QColor(0x00, 0xD4, 0xFF, 60));
    line.setColorAt(0.5, QColor(0x00, 0xD4, 0xFF, 100));
    line.setColorAt(0.7, QColor(0x00, 0xD4, 0xFF, 60));
    line.setColorAt(1, QColor(0x00, 0xD4, 0xFF, 0));
    p.setPen(QPen(line, 1));
    p.drawLine(0, 0, width(), 0);
    p.fillRect(rect().adjusted(0, 1, 0, 0), QColor(0x0D, 0x12, 0x1E));
  }
};

// ================================================================

DeviceMonitorPanel::DeviceMonitorPanel(CanInterface *can, QWidget *parent)
    : QWidget(parent), m_can(can) {
  setupUi();

  m_batchTimer = new QTimer(this);
  m_batchTimer->setInterval(33);
  connect(m_batchTimer, &QTimer::timeout, this, &DeviceMonitorPanel::processBatch);
  m_batchTimer->start();

  // 连接 CAN 帧接收信号
  connect(m_can, &CanInterface::frameReceived, this,
          &DeviceMonitorPanel::onFrameReceived);

  // 加载持久化配置
  loadConfig();

  // 启动 WebSocket 服务器
  m_wsServer = new QWebSocketServer(QStringLiteral("StatusMonitorServer"), QWebSocketServer::NonSecureMode, this);
  if (m_wsServer->listen(QHostAddress::Any, 12345)) {
    connect(m_wsServer, &QWebSocketServer::newConnection, this, &DeviceMonitorPanel::onNewConnection);
    appendLog(QStringLiteral("[WebSocket] 服务器已启动，监听端口 12345"), false);
  } else {
    appendLog(QStringLiteral("[WebSocket] 服务器启动失败，端口 12345 被占用"), true);
  }
}

DeviceMonitorPanel::~DeviceMonitorPanel() {
  saveConfig();
  if (m_wsServer) {
    m_wsServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
  }
}

void DeviceMonitorPanel::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);


  // ---- 工具栏 ----
  m_toolbar = new QWidget();
  m_toolbar->setFixedHeight(42);
  m_toolbar->setStyleSheet(
      "QWidget { background-color: #111827; border-bottom: 1px solid #1E293E; }");

  auto *tbLayout = new QHBoxLayout(m_toolbar);
  tbLayout->setContentsMargins(10, 5, 10, 5);
  tbLayout->setSpacing(8);

  const QString techBtn =
      "QPushButton { padding: 5px 14px; border-radius: 3px; font-size: 11px; "
      "font-weight: bold; font-family: 'Microsoft YaHei'; }";

  m_btnConfig = new QPushButton(QStringLiteral("⚙ 设备配置"));
  m_btnConfig->setStyleSheet(techBtn +
      "QPushButton { color: #00D4FF; background: #1A2740; "
      "border: 1px solid #1E3A5F; } "
      "QPushButton:hover { background: #1E3A5F; border-color: #00D4FF; }");
  tbLayout->addWidget(m_btnConfig);

  m_btnImport = new QPushButton(QStringLiteral("导入"));
  m_btnImport->setStyleSheet(techBtn +
      "QPushButton { color: #7C879A; background: #1A2235; "
      "border: 1px solid #1E293E; } "
      "QPushButton:hover { color: #E2E8F0; border-color: #3B82F6; }");
  tbLayout->addWidget(m_btnImport);

  m_btnExport = new QPushButton(QStringLiteral("导出"));
  m_btnExport->setStyleSheet(techBtn +
      "QPushButton { color: #7C879A; background: #1A2235; "
      "border: 1px solid #1E293E; } "
      "QPushButton:hover { color: #E2E8F0; border-color: #3B82F6; }");
  tbLayout->addWidget(m_btnExport);

  m_btnReset = new QPushButton(QStringLiteral("重置"));
  m_btnReset->setStyleSheet(techBtn +
      "QPushButton { color: #F87171; background: #271A1A; "
      "border: 1px solid #3E1E1E; } "
      "QPushButton:hover { background: #3E1E1E; border-color: #EF4444; }");
  tbLayout->addWidget(m_btnReset);

  // Room 模式按钮
  m_btnToggleRoom = new QPushButton(QStringLiteral("⊞ 布局"));
  m_btnToggleRoom->setCheckable(true);
  m_btnToggleRoom->setStyleSheet(techBtn +
      "QPushButton { color: #10B981; background: #1A2720; "
      "border: 1px solid #1E3E2E; } "
      "QPushButton:hover { background: #1E3E2E; border-color: #10B981; } "
      "QPushButton:checked { color: #00E676; background: #1A3025; "
      "border-color: #00E676; }");
  tbLayout->addWidget(m_btnToggleRoom);

  m_btnAddRoom = new QPushButton(QStringLiteral("＋房间"));
  m_btnAddRoom->setVisible(false);
  m_btnAddRoom->setStyleSheet(techBtn +
      "QPushButton { color: #00D4FF; background: #1A2A35; "
      "border: 1px solid #1E3A5F; } "
      "QPushButton:hover { background: #1E3A5F; }");
  tbLayout->addWidget(m_btnAddRoom);

  // 图标风格选择器
  auto *iconStyleCombo = new QComboBox();
  iconStyleCombo->setStyleSheet(
      "QComboBox{color:#C084FC;background:#1A1A2E;border:1px solid #2E1E3E;padding:3px 8px;font-size:10px;font-family:'Microsoft YaHei';border-radius:3px;}"
      "QComboBox:hover{border-color:#C084FC;}"
      "QComboBox QAbstractItemView{background:#111827;color:#E2E8F0;selection-background:#1E3A5F;}"
      "QComboBox::drop-down{border:none;}");
  iconStyleCombo->addItem(QStringLiteral("🎨 图标风格: 默认"), 0);
  iconStyleCombo->addItem(QStringLiteral("🎨 图标风格: 简约"), 1);
  iconStyleCombo->addItem(QStringLiteral("🎨 图标风格: 复古"), 2);
  tbLayout->addWidget(iconStyleCombo);
  connect(iconStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    DeviceStatusWidget::setIconStyle(idx);
    for (auto *w : m_deviceWidgets) w->update();
  });

  // 布局模板下拉
  m_templateCombo = new QComboBox();
  m_templateCombo->setVisible(false);
  m_templateCombo->setStyleSheet(
      "QComboBox{color:#FBBF24;background:#1A251A;border:1px solid #2E351E;padding:3px 8px;font-size:10px;font-family:'Microsoft YaHei';border-radius:3px;}"
      "QComboBox:hover{border-color:#FBBF24;}"
      "QComboBox QAbstractItemView{background:#111827;color:#E2E8F0;selection-background:#1E3A5F;}"
      "QComboBox::drop-down{border:none;}");
  m_templateCombo->addItem(QStringLiteral("📐 选择布局模板..."), QString());
  m_templateCombo->addItem(QStringLiteral("🚢 核潜艇布局"), QStringLiteral("submarine"));
  m_templateCombo->addItem(QStringLiteral("🏢 写字楼布局"), QStringLiteral("building"));
  m_templateCombo->addItem(QStringLiteral("🛥️ 水面舰船布局"), QStringLiteral("warship"));
  m_templateCombo->addItem(QStringLiteral("🛫 航母布局"), QStringLiteral("carrier"));
  tbLayout->addWidget(m_templateCombo);

  // 缩放控制
  auto *btnZoomOut = new QPushButton(QStringLiteral("🔍−"));
  btnZoomOut->setFixedWidth(50);
  btnZoomOut->setStyleSheet(techBtn + "QPushButton{color:#7C879A;background:#1A2235;border:1px solid #1E293E;}QPushButton:hover{color:#00D4FF;}");
  tbLayout->addWidget(btnZoomOut);
  auto *btnZoomIn = new QPushButton(QStringLiteral("🔍+"));
  btnZoomIn->setFixedWidth(50);
  btnZoomIn->setStyleSheet(techBtn + "QPushButton{color:#7C879A;background:#1A2235;border:1px solid #1E293E;}QPushButton:hover{color:#00D4FF;}");
  tbLayout->addWidget(btnZoomIn);
  auto *btnZoomFit = new QPushButton(QStringLiteral("⊡ 适中"));
  btnZoomFit->setFixedWidth(58);
  btnZoomFit->setStyleSheet(techBtn + "QPushButton{color:#7C879A;background:#1A2235;border:1px solid #1E293E;}QPushButton:hover{color:#10B981;}");
  tbLayout->addWidget(btnZoomFit);

  // 保存布局按钮
  auto *btnSaveLayout = new QPushButton(QStringLiteral("💾 保存布局"));
  btnSaveLayout->setStyleSheet(techBtn + "QPushButton{color:#10B981;background:#1A2720;border:1px solid #1E3E2E;}QPushButton:hover{background:#1E3E2E;}");
  tbLayout->addWidget(btnSaveLayout);

  // 信息日志按钮
  auto *btnInfoLog = new QPushButton(QStringLiteral("📟 信息日志"));
  btnInfoLog->setCheckable(true);
  btnInfoLog->setChecked(true);
  btnInfoLog->setStyleSheet(techBtn +
      "QPushButton{color:#7C879A;background:#1A2235;border:1px solid #1E293E;}"
      "QPushButton:hover{color:#00D4FF;border-color:#00D4FF;}"
      "QPushButton:checked{color:#00D4FF;border-color:#00D4FF;}");
  tbLayout->addWidget(btnInfoLog);

  // 全屏按钮
  m_btnFullScreen = new QPushButton(QStringLiteral("📺 全屏"));
  m_btnFullScreen->setStyleSheet(techBtn +
      "QPushButton{color:#F59E0B;background:#27201A;border:1px solid #3E2E1E;}"
      "QPushButton:hover{background:#3E2E1E;border-color:#F59E0B;}");
  tbLayout->addWidget(m_btnFullScreen);
  connect(m_btnFullScreen, &QPushButton::clicked, this, &DeviceMonitorPanel::toggleFullScreen);

  tbLayout->addStretch();

  m_lblCount = new QLabel(QStringLiteral("CAN 2.0B Protocol Monitor"));
  m_lblCount->setStyleSheet(
      "color: #475569; font-size: 10px; "
      "font-family: 'Consolas', monospace; padding-right: 4px;");
  tbLayout->addWidget(m_lblCount);

  mainLayout->addWidget(m_toolbar);

  // ---- 未摆放设备停靠区 (仅布局模式可见) ----
  m_unplacedDock = new QWidget();
  m_unplacedDock->setVisible(false);
  m_unplacedDock->setStyleSheet(
      "background: rgba(15, 23, 42, 0.85);"
      "border: 1px solid #1E293E;"
      "border-radius: 8px;"
      "margin: 2px 6px;");
  auto *dockVLayout = new QVBoxLayout(m_unplacedDock);
  dockVLayout->setContentsMargins(12, 8, 12, 8);
  dockVLayout->setSpacing(6);

  auto *dockLabel = new QLabel(QStringLiteral("未摆放设备："));
  dockLabel->setStyleSheet(
      "color: #64748B; font-size: 11px; font-weight: bold;"
      "font-family: 'Microsoft YaHei'; background: transparent; border: none;");
  dockVLayout->addWidget(dockLabel);

  auto *dockScroll = new QScrollArea();
  dockScroll->setFrameShape(QFrame::NoFrame);
  dockScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  dockScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  dockScroll->setWidgetResizable(true);
  dockScroll->setFixedHeight(175);
  dockScroll->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:horizontal { background: #0D1117; height: 6px; }"
      "QScrollBar::handle:horizontal { background: #1E293E; border-radius: 3px; min-width: 20px; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }");

  m_unplacedContainer = new QWidget();
  m_unplacedContainer->setStyleSheet("background: transparent; border: none;");
  auto *dockHLayout = new QHBoxLayout(m_unplacedContainer);
  dockHLayout->setContentsMargins(4, 4, 4, 4);
  dockHLayout->setSpacing(14);
  dockHLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  dockScroll->setWidget(m_unplacedContainer);
  dockVLayout->addWidget(dockScroll);
  mainLayout->addWidget(m_unplacedDock);

  // ---- 页面主体 (采用分割器以支持日志高度拖动调节) ----
  QSplitter *splitter = new QSplitter(Qt::Vertical, this);
  splitter->setChildrenCollapsible(false);
  splitter->setStyleSheet("QSplitter::handle { background-color: #1E293E; height: 3px; }");

  // ---- 设备网格区域 (可滚动) ----
  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidgetResizable(true); // 默认网格模式：自动扩展填充视口
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setStyleSheet(
      "QScrollArea { background-color: #0D1117; border: none; }"
      "QScrollBar:vertical { background: #0D1117; width: 8px; }"
      "QScrollBar::handle:vertical { background: #1E293E; border-radius: 4px; min-height: 30px; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
      "QScrollBar:horizontal { background: #0D1117; height: 8px; }"
      "QScrollBar::handle:horizontal { background: #1E293E; border-radius: 4px; min-width: 30px; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }");

  m_gridContainer = new QWidget();
  m_gridContainer->setAcceptDrops(true);
  m_gridContainer->installEventFilter(this);
  m_gridContainer->setStyleSheet("background-color: #0D1117;");
  m_gridLayout = new QGridLayout(m_gridContainer);
  m_gridLayout->setContentsMargins(20, 20, 20, 20);
  m_gridLayout->setSpacing(18);
  m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  m_scrollArea->setWidget(m_gridContainer);
  splitter->addWidget(m_scrollArea);

  // ---- 事件日志（带右上角清除/关闭按钮） ----
  auto *logWrapper = new QWidget();
  auto *logWLayout = new QVBoxLayout(logWrapper);
  logWLayout->setContentsMargins(0, 0, 0, 0);
  logWLayout->setSpacing(0);

  // 日志标题栏
  auto *logTitleBar = new QWidget();
  logTitleBar->setFixedHeight(26);
  logTitleBar->setStyleSheet("background:#0F1620;border-bottom:1px solid #1E293E;");
  auto *ltLayout = new QHBoxLayout(logTitleBar);
  ltLayout->setContentsMargins(10, 0, 4, 0);
  auto *logTitleLbl = new QLabel(QStringLiteral("📋 事件日志"));
  logTitleLbl->setStyleSheet("color:#7C879A;font-size:10px;font-weight:bold;font-family:'Microsoft YaHei';");
  ltLayout->addWidget(logTitleLbl);
  ltLayout->addStretch();
  auto *btnClearLog = new QPushButton(QStringLiteral("🗑"));
  btnClearLog->setFixedSize(22, 22);
  btnClearLog->setToolTip(QStringLiteral("清除日志"));
  btnClearLog->setStyleSheet("QPushButton{color:#64748B;background:transparent;border:none;font-size:12px;}QPushButton:hover{color:#F87171;}");
  ltLayout->addWidget(btnClearLog);
  auto *btnCloseLog = new QPushButton(QStringLiteral("✕"));
  btnCloseLog->setFixedSize(22, 22);
  btnCloseLog->setToolTip(QStringLiteral("关闭日志"));
  btnCloseLog->setStyleSheet("QPushButton{color:#64748B;background:transparent;border:none;font-size:11px;}QPushButton:hover{color:#EF4444;}");
  ltLayout->addWidget(btnCloseLog);
  logWLayout->addWidget(logTitleBar);

  m_log = new QPlainTextEdit();
  m_log->setReadOnly(true);
  m_log->setFrameShape(QFrame::NoFrame);
  m_log->setStyleSheet(
      "QPlainTextEdit { background-color: #0A0E14; color: #7C879A; "
      "font-size: 11px; font-family: 'Microsoft YaHei', 'Consolas', monospace; "
      "padding: 6px; border: none; }");
  m_log->setPlaceholderText(QStringLiteral("事件日志 — 设备状态变更记录"));
  logWLayout->addWidget(m_log);
  splitter->addWidget(logWrapper);

  // 日志面板清除/关闭按钮
  connect(btnClearLog, &QPushButton::clicked, this, [this]() {
    m_log->clear();
  });
  connect(btnCloseLog, &QPushButton::clicked, this, [this, logWrapper]() {
    logWrapper->setVisible(false);
  });
  // 工具栏信息日志按钮 — 重新打开日志面板
  connect(btnInfoLog, &QPushButton::toggled, this, [logWrapper](bool checked) {
    logWrapper->setVisible(checked);
  });
  // logWrapper 隐藏时同步按钮状态
  // (close 按钮点击后通过 btnCloseLog 连接处理)

  // 设置分割器初始拉伸比例
  splitter->setStretchFactor(0, 5); // 监控区域占5份
  splitter->setStretchFactor(1, 1); // 日志区域占1份

  mainLayout->addWidget(splitter, 1);

  // ---- 底部状态栏 ----
  m_bottomBar = new BottomStatusBar(this);
  mainLayout->addWidget(m_bottomBar);

  // 系统时钟定时器
  auto *clockTimer = new QTimer(this);
  connect(clockTimer, &QTimer::timeout, this, [this]() {
    if (m_bottomBar && m_bottomBar->lblClock) {
      m_bottomBar->lblClock->setText(
          QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss"));
    }
  });
  clockTimer->start(1000);
  if (m_bottomBar && m_bottomBar->lblClock) {
    m_bottomBar->lblClock->setText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss"));
  }

  // ---- 信号连接 ----
  connect(m_btnConfig, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onConfigClicked);
  connect(m_btnImport, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onImportClicked);
  connect(m_btnExport, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onExportClicked);
  connect(m_btnReset, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onResetClicked);

  // Room 模式按钮
  connect(m_btnToggleRoom, &QPushButton::toggled, this,
          &DeviceMonitorPanel::onToggleRoomMode);
  connect(m_btnAddRoom, &QPushButton::clicked, this,
          &DeviceMonitorPanel::onAddRoom);
  connect(btnZoomIn, &QPushButton::clicked, this, &DeviceMonitorPanel::zoomIn);
  connect(btnZoomOut, &QPushButton::clicked, this, &DeviceMonitorPanel::zoomOut);
  connect(btnZoomFit, &QPushButton::clicked, this, &DeviceMonitorPanel::zoomFit);
  connect(btnSaveLayout, &QPushButton::clicked, this, [this]() {
    saveRoomLayout();
    appendLog(QStringLiteral("💾 布局已保存"), false);
  });
  connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int idx) {
    QString tpl = m_templateCombo->itemData(idx).toString();
    if (!tpl.isEmpty()) {
      applyLayoutTemplate(tpl);
      m_templateCombo->setCurrentIndex(0);
    }
  });

  loadRoomLayout();

  // 初始占位
  rebuildGrid();
}

// ========== CAN 帧处理 ==========

void DeviceMonitorPanel::onFrameReceived(const CanFrame &frame) {
  if (m_mappings.isEmpty()) return;
  m_frameCount++;
  if (m_pendingFrames.size() < 5000) {
    m_pendingFrames.append(frame);
  }
}

void DeviceMonitorPanel::processBatch() {
  if (m_pendingFrames.isEmpty()) return;

  QVector<CanFrame> batch = std::move(m_pendingFrames);
  m_pendingFrames.clear();

  for (const auto &frame : batch) {
    for (const auto &mapping : m_mappings) {
      if (frame.id != mapping.canId) continue;
      if (mapping.byteIndex >= frame.data.size()) continue;

      const quint8 byteVal =
          static_cast<quint8>(frame.data[mapping.byteIndex]);
      const bool bitVal = (byteVal >> mapping.bitIndex) & 0x01;

      auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
      if (!widget) continue;

      const bool prev = widget->status();
      widget->setStatus(bitVal);

      if (bitVal != prev) {
        // 设备类型中文名
        QString typeName;
        if (mapping.deviceType == QStringLiteral("detector")) typeName = QStringLiteral("烟温探测器");
        else if (mapping.deviceType == QStringLiteral("valve")) typeName = QStringLiteral("控制分配阀");
        else if (mapping.deviceType == QStringLiteral("valve_distributor")) typeName = QStringLiteral("分配阀");
        else if (mapping.deviceType == QStringLiteral("valve_zone")) typeName = QStringLiteral("区域阀");
        else if (mapping.deviceType == QStringLiteral("valve_main_isolation")) typeName = QStringLiteral("总管隔离阀");
        else if (mapping.deviceType == QStringLiteral("manual_alarm")) typeName = QStringLiteral("手动报警按钮");
        else if (mapping.deviceType == QStringLiteral("gas_cylinder")) typeName = QStringLiteral("1301气体钢瓶");
        else if (mapping.deviceType == QStringLiteral("water_pump")) typeName = QStringLiteral("水泵");
        else if (mapping.deviceType == QStringLiteral("pressure_switch")) typeName = QStringLiteral("压力开关");
        else if (mapping.deviceType == QStringLiteral("mobile_spray_gun")) typeName = QStringLiteral("移动喷枪");
        else typeName = mapping.deviceType;

        // 状态文字
        QString stText;
        if (mapping.deviceType == QStringLiteral("valve") ||
            mapping.deviceType == QStringLiteral("valve_distributor") ||
            mapping.deviceType == QStringLiteral("valve_zone") ||
            mapping.deviceType == QStringLiteral("valve_main_isolation"))
          stText = bitVal ? QStringLiteral("开启") : QStringLiteral("关闭");
        else if (mapping.deviceType == QStringLiteral("water_pump"))
          stText = bitVal ? QStringLiteral("运转") : QStringLiteral("停止");
        else if (mapping.deviceType == QStringLiteral("pressure_switch"))
          stText = bitVal ? QStringLiteral("开启") : QStringLiteral("关闭");
        else if (mapping.deviceType == QStringLiteral("mobile_spray_gun"))
          stText = bitVal ? QStringLiteral("喷射") : QStringLiteral("停止");
        else
          stText = bitVal ? QStringLiteral("报警") : QStringLiteral("正常");

        appendLog(QStringLiteral("%1 [%2]  状态变为：%3")
                      .arg(mapping.label, typeName, stText),
                  bitVal);

        // 向所有连接的 WebSocket 客户端广播状态更新
        QJsonObject updateObj;
        updateObj[QStringLiteral("type")] = QStringLiteral("update");
        updateObj[QStringLiteral("deviceId")] = mapping.deviceId;
        updateObj[QStringLiteral("status")] = bitVal;
        updateObj[QStringLiteral("prevStatus")] = prev;
        updateObj[QStringLiteral("label")] = mapping.label;
        updateObj[QStringLiteral("deviceType")] = mapping.deviceType;
        updateObj[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        broadcastMessage(updateObj);
      }
    }
  }
}

// ========== 网格重建 ==========

void DeviceMonitorPanel::rebuildGrid() {
  // 网格模式：清除固定尺寸，允许容器自适应填充视口
  m_gridContainer->setMinimumSize(0, 0);
  m_gridContainer->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

  if (m_mappings.isEmpty()) {
    qDeleteAll(m_deviceWidgets);
    m_deviceWidgets.clear();
    while (m_gridLayout->count() > 0) {
      QLayoutItem *item = m_gridLayout->takeAt(0);
      delete item;
    }
  } else {
    // 清理已在 m_mappings 中被删除的控件
    QSet<int> currentMappingIds;
    for (const auto &m : m_mappings) {
      currentMappingIds.insert(m.deviceId);
    }
    for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ) {
      if (!currentMappingIds.contains(it.key())) {
        delete it.value();
        it = m_deviceWidgets.erase(it);
      } else {
        ++it;
      }
    }

    // 针对每个映射，同步更新或创建 DeviceStatusWidget 实例
    for (int i = 0; i < m_mappings.size(); ++i) {
      const auto &m = m_mappings[i];
      DeviceStatusWidget::DeviceKind kind = DeviceStatusWidget::Detector;
      if (m.deviceType == QStringLiteral("valve")) {
        kind = DeviceStatusWidget::Valve;
      } else if (m.deviceType == QStringLiteral("valve_distributor")) {
        kind = DeviceStatusWidget::ValveDistributor;
      } else if (m.deviceType == QStringLiteral("valve_zone")) {
        kind = DeviceStatusWidget::ValveZone;
      } else if (m.deviceType == QStringLiteral("valve_main_isolation")) {
        kind = DeviceStatusWidget::ValveMainIsolation;
      } else if (m.deviceType == QStringLiteral("manual_alarm")) {
        kind = DeviceStatusWidget::ManualAlarm;
      } else if (m.deviceType == QStringLiteral("gas_cylinder")) {
        kind = DeviceStatusWidget::GasCylinder;
      } else if (m.deviceType == QStringLiteral("water_pump")) {
        kind = DeviceStatusWidget::WaterPump;
      } else if (m.deviceType == QStringLiteral("pressure_switch")) {
        kind = DeviceStatusWidget::PressureSwitch;
      } else if (m.deviceType == QStringLiteral("mobile_spray_gun")) {
        kind = DeviceStatusWidget::MobileSprayGun;
      }

      DeviceStatusWidget *w = m_deviceWidgets.value(m.deviceId, nullptr);
      if (w) {
        w->setLabel(m.label);
        w->setCanId(m.canId);
        w->setDeviceKind(kind);
      } else {
        w = new DeviceStatusWidget(m.deviceId, kind, m.label, m.canId);
        w->setStatus(m.defaultVal);
        connect(w, &DeviceStatusWidget::editRequested, this, &DeviceMonitorPanel::onEditDeviceRequested);
        m_deviceWidgets[m.deviceId] = w;
      }
    }

    // 重新排列控件坐标
    while (m_gridLayout->count() > 0) {
      m_gridLayout->takeAt(0);
    }

    for (int i = 0; i < m_mappings.size(); ++i) {
      const auto &m = m_mappings[i];
      auto *w = m_deviceWidgets.value(m.deviceId, nullptr);
      if (w) {
        int row = i / m_gridCols;
        int col = i % m_gridCols;
        m_gridLayout->addWidget(w, row, col);
      }
    }
  }

  // 广播配置更新给所有已连接的 WebSocket 客户端
  for (auto *client : m_clients) {
    sendConfigToClient(client);
  }
}

void DeviceMonitorPanel::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (!m_scrollArea || m_mappings.isEmpty()) return;

  if (m_roomMode) {
    // 布局模式：确保画布始终不小于视口
    updateZoom();
  } else {
    // 根据视口宽度动态计算列数 (卡片宽度约150px)
    int areaW = m_scrollArea->viewport()->width();
    int cols = qMax(1, areaW / 150);
    if (cols != m_gridCols) {
      m_gridCols = cols;
      rebuildGrid();
    }
  }
}

void DeviceMonitorPanel::sendConfigToClient(QWebSocket *client) {
  QJsonObject configObj;
  configObj[QStringLiteral("type")] = QStringLiteral("config");

  QJsonArray mappingsArr;
  for (const auto &mapping : m_mappings) {
    QJsonObject item;
    item[QStringLiteral("deviceId")] = mapping.deviceId;
    item[QStringLiteral("label")] = mapping.label;
    item[QStringLiteral("deviceType")] = mapping.deviceType;
    item[QStringLiteral("canId")] = static_cast<int>(mapping.canId);
    item[QStringLiteral("byteIndex")] = mapping.byteIndex;
    item[QStringLiteral("bitIndex")] = mapping.bitIndex;

    // 获取当前状态
    bool currentStatus = false;
    auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
    if (widget) {
      currentStatus = widget->status();
    }
    item[QStringLiteral("status")] = currentStatus;

    mappingsArr.append(item);
  }
  configObj[QStringLiteral("mappings")] = mappingsArr;

  QJsonDocument doc(configObj);
  client->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void DeviceMonitorPanel::broadcastMessage(const QJsonObject &json) {
  QJsonDocument doc(json);
  QString msg = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
  for (auto *client : m_clients) {
    client->sendTextMessage(msg);
  }
}

void DeviceMonitorPanel::onNewConnection() {
  QWebSocket *client = m_wsServer->nextPendingConnection();
  connect(client, &QWebSocket::disconnected, this, &DeviceMonitorPanel::onClientDisconnected);
  m_clients.append(client);

  appendLog(QStringLiteral("[WebSocket] 客户端已连接：%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()), false);

  // 连接建立后，主动发送当前配置和最新设备状态
  sendConfigToClient(client);
}

void DeviceMonitorPanel::onClientDisconnected() {
  QWebSocket *client = qobject_cast<QWebSocket *>(sender());
  if (client) {
    m_clients.removeAll(client);
    client->deleteLater();
    appendLog(QStringLiteral("[WebSocket] 客户端已断开"), false);
  }
}

// ========== 按钮操作 ==========

void DeviceMonitorPanel::onConfigClicked() {
  CanProtocolConfigDialog dlg(this);

  // 获取 CANTool 窗口的当前样式以应用主题
  QWidget *canTool = this;
  while (canTool && !canTool->inherits("CANTool")) {
    canTool = canTool->parentWidget();
  }
  if (canTool) {
    dlg.applyThemeStyle(canTool->styleSheet());
  }

  dlg.setMappings(m_mappings);
  if (dlg.exec() == QDialog::Accepted) {
    m_mappings = dlg.mappings();
    rebuildGrid();
    saveConfig();
    appendLog(
        QStringLiteral("✓ 配置已更新 — %1 个设备").arg(m_mappings.size()));
  }
}

void DeviceMonitorPanel::onImportClicked() {
  QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("导入配置"), QString(),
      QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();

  if (!doc.isArray()) return;
  m_mappings.clear();
  for (const auto &v : doc.array()) {
    auto o = v.toObject();
    DeviceBitMapping m;
    m.deviceId = o["deviceId"].toInt(1);
    m.label = o["label"].toString(QStringLiteral("设备"));
    m.deviceType =
        o["deviceType"].toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
    m.byteIndex = o["byteIndex"].toInt(0);
    m.bitIndex = o["bitIndex"].toInt(0);
    m.defaultVal = o["defaultVal"].toInt(0);
    m_mappings.append(m);
  }
  rebuildGrid();
  saveConfig();
  appendLog(QStringLiteral("✓ 已导入 %1 个设备").arg(m_mappings.size()));
}

void DeviceMonitorPanel::onExportClicked() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject o;
    o["deviceId"] = m.deviceId;
    o["label"] = m.label;
    o["deviceType"] = m.deviceType;
    o["canId"] = static_cast<int>(m.canId);
    o["byteIndex"] = m.byteIndex;
    o["bitIndex"] = m.bitIndex;
    o["defaultVal"] = m.defaultVal;
    arr.append(o);
  }
  QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出"), QStringLiteral("device_config.json"),
      QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) return;
  QFile f(path);
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
  }
}

void DeviceMonitorPanel::onResetClicked() {
  for (const auto &m : m_mappings) {
    auto *w = m_deviceWidgets.value(m.deviceId, nullptr);
    if (w) w->setStatus(m.defaultVal);
  }
  m_log->clear();
  m_frameCount = 0;
  appendLog(QStringLiteral("↻ 全部设备状态已重置为默认配置值"));
}

// ========== 持久化 ==========

void DeviceMonitorPanel::saveConfig() {
  QJsonArray arr;
  for (const auto &m : m_mappings) {
    QJsonObject o;
    o["deviceId"] = m.deviceId;
    o["label"] = m.label;
    o["deviceType"] = m.deviceType;
    o["canId"] = static_cast<int>(m.canId);
    o["byteIndex"] = m.byteIndex;
    o["bitIndex"] = m.bitIndex;
    o["defaultVal"] = m.defaultVal;
    arr.append(o);
  }
  QFile f(QStringLiteral("device_config.json"));
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
  }
}

void DeviceMonitorPanel::loadConfig() {
  QFile f(QStringLiteral("device_config.json"));
  if (!f.open(QIODevice::ReadOnly)) return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();
  if (!doc.isArray()) return;
  for (const auto &v : doc.array()) {
    auto o = v.toObject();
    DeviceBitMapping m;
    m.deviceId = o["deviceId"].toInt(1);
    m.label = o["label"].toString(QStringLiteral("设备"));
    m.deviceType =
        o["deviceType"].toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
    m.byteIndex = o["byteIndex"].toInt(0);
    m.bitIndex = o["bitIndex"].toInt(0);
    m.defaultVal = o["defaultVal"].toInt(0);
    m_mappings.append(m);
  }
  rebuildGrid();
}

// ========== 日志 ==========

void DeviceMonitorPanel::appendLog(const QString &text, bool isAlarm) {
  const QString ts =
      QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_log->setUpdatesEnabled(false);
  if (isAlarm) {
    m_log->appendHtml(
        QStringLiteral("<span style='color:#EF4444;'>[%1] %2</span>")
            .arg(ts, text.toHtmlEscaped()));
  } else {
    m_log->appendPlainText(
        QStringLiteral("[%1] %2").arg(ts, text));
  }
  // 高效批量裁剪旧日志
  auto *doc = m_log->document();
  int extraBlocks = doc->blockCount() - 300;
  if (extraBlocks > 0) {
    QTextCursor c(doc);
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, extraBlocks);
    c.removeSelectedText();
  }
  m_log->verticalScrollBar()->setValue(
      m_log->verticalScrollBar()->maximum());
  m_log->setUpdatesEnabled(true);
}


void DeviceMonitorPanel::onDeviceDragged(int deviceId, const QPoint &newPos) {
  // 存储基础坐标（除以当前缩放）
  m_deviceRoomPos[deviceId] = QPoint(newPos.x() / m_zoomLevel, newPos.y() / m_zoomLevel);
  saveRoomLayout();

  // 更新房间设备计数
  for (auto *rw : m_roomWidgets) {
    int cnt = 0;
    for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ++it) {
      QRect devGeom(it.value()->pos(), it.value()->size());
      if (rw->geometry().contains(devGeom.center())) ++cnt;
    }
    rw->setDeviceCount(cnt);
  }

  saveRoomLayout();
}

void DeviceMonitorPanel::onRoomMoved(const QString &id, const QRect &newGeom) {
  for (auto &r : m_rooms) {
    if (r.id == id) {
      r.geom = QRect(newGeom.x() / m_zoomLevel, newGeom.y() / m_zoomLevel,
                     newGeom.width() / m_zoomLevel, newGeom.height() / m_zoomLevel);
      break;
    }
  }
  saveRoomLayout();
}

void DeviceMonitorPanel::onRoomResized(const QString &id, const QRect &newGeom) {
  for (auto &r : m_rooms) {
    if (r.id == id) {
      r.geom = QRect(newGeom.x() / m_zoomLevel, newGeom.y() / m_zoomLevel,
                     newGeom.width() / m_zoomLevel, newGeom.height() / m_zoomLevel);
      break;
    }
  }
  saveRoomLayout();
}

void DeviceMonitorPanel::onToggleRoomMode() {
  m_roomMode = m_btnToggleRoom->isChecked();
  m_btnAddRoom->setVisible(m_roomMode);
  m_templateCombo->setVisible(m_roomMode);

  if (m_roomMode) {
    m_scrollArea->setWidgetResizable(false); // 布局模式：画布可大于视口
    if (m_rooms.isEmpty()) {
      // 创建默认房间（不自动放入设备，让设备留在停靠区供用户手动拖入）
      int vpW = m_scrollArea->viewport()->width() - 40;
      int vpH = m_scrollArea->viewport()->height() - 40;
      m_baseCanvasW = qMax(vpW, 1000);
      m_baseCanvasH = qMax(vpH, 800);
      RoomRegion r;
      r.id = QStringLiteral("room_1"); r.name = QStringLiteral("1区");
      r.geom = QRect(20, 20, m_baseCanvasW / 2 - 30, m_baseCanvasH - 40);
      m_rooms.append(r);
      RoomRegion r2;
      r2.id = QStringLiteral("room_2"); r2.name = QStringLiteral("2区");
      r2.geom = QRect(m_baseCanvasW / 2 + 10, 20, m_baseCanvasW / 2 - 30, m_baseCanvasH - 40);
      m_rooms.append(r2);
      // 不再自动分散设备到房间，设备默认在"未摆放"停靠区
    }
    rebuildRoomCanvas();
    zoomFit();
    updateZoom();
  } else {
    m_unplacedDock->setVisible(false);
    m_scrollArea->setWidgetResizable(true); // 网格模式：自动填充视口
    qDeleteAll(m_roomWidgets);
    m_roomWidgets.clear();
    rebuildGrid();
  }
}

// ===== 缩放 =====

void DeviceMonitorPanel::zoomIn() { m_zoomLevel = qMin(2.0, m_zoomLevel + 0.25); updateZoom(); }
void DeviceMonitorPanel::zoomOut() { m_zoomLevel = qMax(0.25, m_zoomLevel - 0.25); updateZoom(); }

void DeviceMonitorPanel::zoomFit() {
  int vpW = m_scrollArea->viewport()->width() - 40;
  int vpH = m_scrollArea->viewport()->height() - 40;
  m_zoomLevel = qMin((qreal)vpW / m_baseCanvasW, (qreal)vpH / m_baseCanvasH);
  m_zoomLevel = qBound(0.25, m_zoomLevel, 2.0);
  updateZoom();
}

void DeviceMonitorPanel::updateZoom() {
  // 画布始终不小于视口，放大时可超出视口（出现滚动条）
  int vpW = m_scrollArea->viewport()->width();
  int vpH = m_scrollArea->viewport()->height();
  int canvasW = qMax(vpW, (int)(m_baseCanvasW * m_zoomLevel));
  int canvasH = qMax(vpH, (int)(m_baseCanvasH * m_zoomLevel));
  m_gridContainer->setFixedSize(canvasW, canvasH);

  // 重新缩放所有已摆放的设备和房间
  for (auto *w : m_deviceWidgets) {
    int devId = w->deviceId();
    if (m_deviceRoomPos.contains(devId)) {
      QPoint pos = m_deviceRoomPos.value(devId);
      w->setGeometry(pos.x() * m_zoomLevel, pos.y() * m_zoomLevel,
                     128 * m_zoomLevel, 155 * m_zoomLevel);
    }
    w->update();
  }
  for (int i = 0; i < m_roomWidgets.size() && i < m_rooms.size(); ++i) {
    const auto &r = m_rooms[i];
    m_roomWidgets[i]->setGeometry(r.geom.x() * m_zoomLevel, r.geom.y() * m_zoomLevel,
                                   r.geom.width() * m_zoomLevel, r.geom.height() * m_zoomLevel);
  }
  if (m_lblCount)
    m_lblCount->setText(QStringLiteral("缩放: %1% | %2 房间 | %3 设备")
        .arg((int)(m_zoomLevel * 100)).arg(m_rooms.size()).arg(m_mappings.size()));
}

// Ctrl+滚轮缩放
void DeviceMonitorPanel::wheelEvent(QWheelEvent *e) {
  if (e->modifiers() & Qt::ControlModifier) {
    if (e->angleDelta().y() > 0) zoomIn(); else zoomOut();
    e->accept();
  } else {
    QWidget::wheelEvent(e);
  }
}

bool DeviceMonitorPanel::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Escape && m_isFullScreen) {
      toggleFullScreen();
      return true;
    }
  }
  if (watched == m_gridContainer) {
    if (event->type() == QEvent::DragEnter) {
      QDragEnterEvent *dee = static_cast<QDragEnterEvent *>(event);
      if (dee->mimeData()->hasText() && dee->mimeData()->text().startsWith(QStringLiteral("device:"))) {
        dee->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::DragMove) {
      QDragMoveEvent *dme = static_cast<QDragMoveEvent *>(event);
      dme->acceptProposedAction();
      return true;
    } else if (event->type() == QEvent::Drop) {
      QDropEvent *de = static_cast<QDropEvent *>(event);
      QString text = de->mimeData()->text();
      if (text.startsWith(QStringLiteral("device:"))) {
        int deviceId = text.mid(7).toInt();
        QPoint localPos = de->pos();
        
        // 使卡片中心对齐鼠标光标，并扣除当前的 zoomLevel 缩放
        QPoint canvasPos;
        canvasPos.setX((localPos.x() - 64) / m_zoomLevel);
        canvasPos.setY((localPos.y() - 77) / m_zoomLevel);
        
        m_deviceRoomPos[deviceId] = canvasPos;
        saveRoomLayout();
        
        rebuildRoomCanvas();
        updateZoom();
        
        de->acceptProposedAction();
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void DeviceMonitorPanel::applyLayoutTemplate(const QString &tpl) {
  m_activeTemplate = tpl;
  m_rooms.clear();
  qDeleteAll(m_roomWidgets);
  m_roomWidgets.clear();
  m_deviceRoomPos.clear();

  // 核潜艇布局
  if (tpl == QStringLiteral("submarine")) {
    m_rooms.append({"sub_1", QStringLiteral("舱首/鱼雷舱"), QRect(150, 300, 180, 220)});
    m_rooms.append({"sub_2", QStringLiteral("指挥与战术中心"), QRect(400, 220, 220, 300)});
    m_rooms.append({"sub_3", QStringLiteral("生活休息舱"), QRect(680, 300, 180, 220)});
    m_rooms.append({"sub_4", QStringLiteral("反应堆舱区"), QRect(900, 280, 180, 240)});
    m_rooms.append({"sub_5", QStringLiteral("动力/推进舱"), QRect(1120, 300, 180, 220)});
    m_gridContainer->setFixedSize(2000, 800);
  }
  // 写字楼布局
  else if (tpl == QStringLiteral("building")) {
    m_rooms.append({"bld_3", QStringLiteral("3F - 云数据机房"), QRect(100, 110, 600, 100)});
    m_rooms.append({"bld_2", QStringLiteral("2F - 行政与会议中心"), QRect(100, 225, 600, 100)});
    m_rooms.append({"bld_1", QStringLiteral("1F - 研发测试中心"), QRect(100, 340, 600, 100)});
    m_gridContainer->setFixedSize(800, 550);
  }
  // 水面舰船布局
  else if (tpl == QStringLiteral("warship")) {
    m_rooms.append({"ship_1", QStringLiteral("舰艏武器库区"), QRect(60, 180, 200, 180)});
    m_rooms.append({"ship_2", QStringLiteral("舰桥驾驶控制舱"), QRect(280, 100, 240, 260)});
    m_rooms.append({"ship_3", QStringLiteral("舰舯机电舱室"), QRect(540, 180, 200, 180)});
    m_rooms.append({"ship_4", QStringLiteral("舰艉直升机库"), QRect(760, 160, 200, 200)});
    m_gridContainer->setFixedSize(1100, 550);
  }
  // 航母布局
  else if (tpl == QStringLiteral("carrier")) {
    m_rooms.append({"car_1", QStringLiteral("飞行甲板/舰载机区"), QRect(50, 50, 500, 200)});
    m_rooms.append({"car_2", QStringLiteral("舰岛/指挥塔"), QRect(560, 50, 200, 180)});
    m_rooms.append({"car_3", QStringLiteral("机库/维修区"), QRect(50, 260, 500, 180)});
    m_rooms.append({"car_4", QStringLiteral("动力/推进舱"), QRect(560, 240, 200, 200)});
    m_rooms.append({"car_5", QStringLiteral("武器/防御区"), QRect(770, 50, 200, 390)});
    m_gridContainer->setFixedSize(1050, 550);
  }

  // 自动分配设备到房间
  int idx = 0;
  for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ++it, ++idx) {
    int devId = it.key();
    if (idx < m_rooms.size()) {
      const auto &r = m_rooms[idx];
      m_deviceRoomPos[devId] = QPoint(r.geom.x() + 30, r.geom.y() + 40);
    }
  }

  m_baseCanvasW = m_gridContainer->width();
  m_baseCanvasH = m_gridContainer->height();
  rebuildRoomCanvas();
  saveRoomLayout();
  zoomFit();
  updateZoom();
  appendLog(QStringLiteral("✓ 已应用布局模板: %1").arg(tpl), false);
}

void DeviceMonitorPanel::onAddRoom() {
  // 形状选择
  QStringList shapes = {QStringLiteral("矩形"), QStringLiteral("圆形"), QStringLiteral("菱形")};
  bool ok;
  QString shapeStr = QInputDialog::getItem(this, QStringLiteral("新建房间 — 选择形状"),
      QStringLiteral("房间形状:"), shapes, 0, false, &ok);
  if (!ok) return;

  int shape = shapes.indexOf(shapeStr);
  QString name = QInputDialog::getText(this, QStringLiteral("新建房间"),
      QStringLiteral("房间名称:"), QLineEdit::Normal,
      QStringLiteral("房间 %1").arg(m_rooms.size() + 1), &ok);
  if (!ok || name.trimmed().isEmpty()) return;

  RoomRegion r;
  r.id = QStringLiteral("room_%1").arg(QDateTime::currentMSecsSinceEpoch());
  r.name = name.trimmed();
  r.shape = shape;
  r.geom = QRect(50 + m_rooms.size() * 40, 50 + m_rooms.size() * 40, 320, 240);
  m_rooms.append(r);

  rebuildRoomCanvas();
  updateZoom();

  appendLog(QStringLiteral("✓ 已创建房间: %1").arg(r.name), false);
  saveRoomLayout();

  if (m_lblCount) {
    m_lblCount->setText(QStringLiteral("分区模式 | %1 个房间 | %2 个设备")
        .arg(m_rooms.size()).arg(m_mappings.size()));
  }
}

// (rename/delete 功能已移至 RoomWidget 右上角按钮)

// ===== Room 布局持久化 =====
void DeviceMonitorPanel::saveRoomLayout() {
  QJsonObject root;
  QJsonArray roomsArr;
  for (const auto &r : m_rooms) {
    QJsonObject ro;
    ro["id"] = r.id;
    ro["name"] = r.name;
    ro["shape"] = r.shape;
    ro["x"] = r.geom.x();
    ro["y"] = r.geom.y();
    ro["w"] = r.geom.width();
    ro["h"] = r.geom.height();
    roomsArr.append(ro);
  }
  root["rooms"] = roomsArr;

  QJsonObject devPos;
  for (auto it = m_deviceRoomPos.begin(); it != m_deviceRoomPos.end(); ++it) {
    QJsonObject dp;
    dp["x"] = it.value().x();
    dp["y"] = it.value().y();
    devPos[QString::number(it.key())] = dp;
  }
  root["devicePositions"] = devPos;

  QSettings s("PhudonTools", "PhudonTools");
  s.setValue("monitor/roomLayout",
      QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void DeviceMonitorPanel::loadRoomLayout() {
  QSettings s("PhudonTools", "PhudonTools");
  QString json = s.value("monitor/roomLayout").toString();
  if (json.isEmpty()) return;

  QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject()) return;

  QJsonObject root = doc.object();
  m_rooms.clear();
  for (const auto &rv : root["rooms"].toArray()) {
    QJsonObject ro = rv.toObject();
    RoomRegion r;
    r.id = ro["id"].toString();
    r.name = ro["name"].toString();
    r.shape = ro["shape"].toInt(0);
    r.geom = QRect(ro["x"].toInt(), ro["y"].toInt(),
                   ro["w"].toInt(), ro["h"].toInt());
    m_rooms.append(r);
  }

  m_deviceRoomPos.clear();
  QJsonObject devPos = root["devicePositions"].toObject();
  for (auto it = devPos.begin(); it != devPos.end(); ++it) {
    QJsonObject dp = it.value().toObject();
    m_deviceRoomPos[it.key().toInt()] = QPoint(dp["x"].toInt(), dp["y"].toInt());
  }
}

void DeviceMonitorPanel::rebuildRoomCanvas() {
  // 清理 grid layout
  while (m_gridLayout->count() > 0) {
    QLayoutItem *item = m_gridLayout->takeAt(0);
    delete item;
  }

  // 清理旧 RoomWidget
  qDeleteAll(m_roomWidgets);
  m_roomWidgets.clear();

  // 确保所有设备 widget 已创建
  if (m_deviceWidgets.size() != m_mappings.size()) {
    qDeleteAll(m_deviceWidgets);
    m_deviceWidgets.clear();
    for (int i = 0; i < m_mappings.size(); ++i) {
      const auto &m = m_mappings[i];
      auto kind = DeviceStatusWidget::Detector;
      if (m.deviceType == QStringLiteral("valve"))
        kind = DeviceStatusWidget::Valve;
      else if (m.deviceType == QStringLiteral("valve_distributor"))
        kind = DeviceStatusWidget::ValveDistributor;
      else if (m.deviceType == QStringLiteral("valve_zone"))
        kind = DeviceStatusWidget::ValveZone;
      else if (m.deviceType == QStringLiteral("valve_main_isolation"))
        kind = DeviceStatusWidget::ValveMainIsolation;
      else if (m.deviceType == QStringLiteral("manual_alarm"))
        kind = DeviceStatusWidget::ManualAlarm;
      else if (m.deviceType == QStringLiteral("gas_cylinder"))
        kind = DeviceStatusWidget::GasCylinder;
      else if (m.deviceType == QStringLiteral("water_pump"))
        kind = DeviceStatusWidget::WaterPump;
      else if (m.deviceType == QStringLiteral("pressure_switch"))
        kind = DeviceStatusWidget::PressureSwitch;
      else if (m.deviceType == QStringLiteral("mobile_spray_gun"))
        kind = DeviceStatusWidget::MobileSprayGun;

      auto *w = new DeviceStatusWidget(m.deviceId, kind, m.label, m.canId);
      w->setStatus(m.defaultVal);
      w->setDraggable(true);
      connect(w, &DeviceStatusWidget::deviceDragged, this,
              &DeviceMonitorPanel::onDeviceDragged);
      connect(w, &DeviceStatusWidget::dragStartedFromDock, this,
              [this](int deviceId, const QPoint &) {
                placeDeviceOnCanvas(deviceId);
              });
      m_deviceWidgets[m.deviceId] = w;
    }
  }

  // 画布样式
  m_gridContainer->setStyleSheet("background-color: #0D1117;");

  // 创建 RoomWidget
  for (const auto &r : m_rooms) {
    auto *rw = new RoomWidget(r.id, r.name, r.geom, r.shape, m_gridContainer);
    connect(rw, &RoomWidget::roomMoved, this, &DeviceMonitorPanel::onRoomMoved);
    connect(rw, &RoomWidget::roomResized, this, &DeviceMonitorPanel::onRoomResized);
    connect(rw, &RoomWidget::roomRenameRequested, this, [this](const QString &rid) {
      for (auto *w : m_roomWidgets) {
        if (w->roomId() == rid) {
          bool ok;
          QString name = QInputDialog::getText(this, QStringLiteral("重命名房间"), QStringLiteral("新名称:"),
              QLineEdit::Normal, w->roomName(), &ok);
          if (ok && !name.trimmed().isEmpty()) {
            w->setRoomName(name.trimmed());
            for (auto &r : m_rooms) { if (r.id == rid) { r.name = name.trimmed(); break; } }
            saveRoomLayout();
            appendLog(QStringLiteral("✓ 房间已重命名为: %1").arg(name.trimmed()), false);
          }
          return;
        }
      }
    });
    connect(rw, &RoomWidget::roomDeleteRequested, this, [this](const QString &rid) {
      auto btn = QMessageBox::question(this, QStringLiteral("删除房间"), QStringLiteral("确定要删除此房间吗？"));
      if (btn == QMessageBox::Yes) {
        for (int i = 0; i < m_roomWidgets.size(); ++i) {
          if (m_roomWidgets[i]->roomId() == rid) {
            appendLog(QStringLiteral("✕ 已删除房间: %1").arg(m_roomWidgets[i]->roomName()), true);
            m_rooms.removeAt(i);
            delete m_roomWidgets.takeAt(i);
            saveRoomLayout();
            break;
          }
        }
      }
    });
    m_roomWidgets.append(rw);
    rw->show();
  }

  // 自适应画布基准尺寸（不含缩放，以最远设备/房间边界为准）
  int maxX = 800, maxY = 600;
  for (const auto &r : m_rooms) { maxX = qMax(maxX, r.geom.right() + 50); maxY = qMax(maxY, r.geom.bottom() + 50); }
  for (auto it = m_deviceRoomPos.begin(); it != m_deviceRoomPos.end(); ++it) {
    maxX = qMax(maxX, it.value().x() + 180);
    maxY = qMax(maxY, it.value().y() + 200);
  }
  m_baseCanvasW = maxX;
  m_baseCanvasH = maxY;
  // 画布尺寸 = max(视口, 基准*缩放)，保证始终填满视口
  int vpW = m_scrollArea->viewport()->width();
  int vpH = m_scrollArea->viewport()->height();
  m_gridContainer->setFixedSize(qMax(vpW, (int)(m_baseCanvasW * m_zoomLevel)),
                                qMax(vpH, (int)(m_baseCanvasH * m_zoomLevel)));

  // 模板背景轮廓
  if (!m_activeTemplate.isEmpty()) {
    auto *bg = new TemplateBackground(m_gridContainer);
    bg->tpl = m_activeTemplate;
    bg->setGeometry(0, 0, maxX, maxY);
    bg->lower();
    bg->show();
  }

  // ===== 分流设备：已摆放的放画布，未摆放的放停靠区 =====
  for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ++it) {
    int devId = it.key();
    auto *w = it.value();

    if (m_deviceRoomPos.contains(devId)) {
      // 已摆放 → 放到画布上
      w->setParent(m_gridContainer);
      w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      QPoint pos = m_deviceRoomPos.value(devId);
      w->setGeometry(pos.x() * m_zoomLevel, pos.y() * m_zoomLevel,
                     128 * m_zoomLevel, 155 * m_zoomLevel);
      w->setDraggable(true);
      w->show();
      w->raise();
    }
    // 未摆放的设备会在 updateUnplacedDock() 中处理
  }

  // 更新未摆放停靠区
  updateUnplacedDock();

  // 更新房间设备计数（仅计算已摆放的设备）
  for (auto *rw : m_roomWidgets) {
    int cnt = 0;
    for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ++it) {
      if (!m_deviceRoomPos.contains(it.key())) continue;
      QRect devGeom(it.value()->pos(), it.value()->size());
      if (rw->geometry().contains(devGeom.center())) ++cnt;
    }
    rw->setDeviceCount(cnt);
  }

  if (m_lblCount) {
    int placed = 0;
    for (const auto &mapping : m_mappings) {
      if (m_deviceRoomPos.contains(mapping.deviceId)) ++placed;
    }
    m_lblCount->setText(QStringLiteral("分区模式 | %1 个房间 | %2/%3 设备已摆放")
        .arg(m_rooms.size()).arg(placed).arg(m_mappings.size()));
  }
}

// ===== 未摆放设备停靠区管理 =====

void DeviceMonitorPanel::updateUnplacedDock() {
  if (!m_unplacedContainer) return;

  // 清空停靠区布局（解除控件父级但不删除）
  QLayout *lay = m_unplacedContainer->layout();
  if (lay) {
    while (lay->count() > 0) {
      QLayoutItem *item = lay->takeAt(0);
      if (item->widget()) {
        item->widget()->setParent(nullptr);
      }
      delete item;
    }
  }

  // 收集未摆放的设备（没有在 m_deviceRoomPos 中的）
  QList<int> unplacedIds;
  for (const auto &mapping : m_mappings) {
    if (!m_deviceRoomPos.contains(mapping.deviceId)) {
      unplacedIds.append(mapping.deviceId);
    }
  }

  if (unplacedIds.isEmpty()) {
    // 所有设备已摆放，隐藏停靠区
    m_unplacedDock->setVisible(false);
    return;
  }

  // 显示停靠区并填充设备
  m_unplacedDock->setVisible(true);

  for (int devId : unplacedIds) {
    auto *w = m_deviceWidgets.value(devId, nullptr);
    if (!w) continue;

    w->setParent(m_unplacedContainer);
    w->setFixedSize(128, 155);
    w->setDraggable(false); // 在停靠区不可拖拽
    w->setCursor(Qt::PointingHandCursor);
    w->show();

    // 添加到水平布局
    m_unplacedContainer->layout()->addWidget(w);
  }
}

void DeviceMonitorPanel::placeDeviceOnCanvas(int deviceId) {
  auto *w = m_deviceWidgets.value(deviceId, nullptr);
  if (!w) return;

  // 找到第一个房间的中心作为默认放置位置
  QPoint placePos(60, 60);
  if (!m_rooms.isEmpty()) {
    const auto &firstRoom = m_rooms.first();
    // 计算该房间中已有设备数量，用于排列新设备
    int existing = 0;
    for (auto it = m_deviceRoomPos.begin(); it != m_deviceRoomPos.end(); ++it) {
      QPoint p = it.value();
      if (firstRoom.geom.contains(p)) existing++;
    }
    int col = existing % 4;
    int row = existing / 4;
    placePos = QPoint(firstRoom.geom.x() + 30 + col * 140,
                      firstRoom.geom.y() + 50 + row * 170);
    // 确保在房间边界内
    if (placePos.x() + 128 > firstRoom.geom.right())
      placePos.setX(firstRoom.geom.x() + 30);
    if (placePos.y() + 155 > firstRoom.geom.bottom())
      placePos.setY(firstRoom.geom.y() + 50);
  }

  // 保存设备位置
  m_deviceRoomPos[deviceId] = placePos;
  saveRoomLayout();

  // 重建画布
  rebuildRoomCanvas();
  updateZoom();

  appendLog(QStringLiteral("✓ 设备 %1 已摆放到画布").arg(w->label()), false);
}

// ========== 设备编辑与全屏控制 ==========

void DeviceMonitorPanel::onEditDeviceRequested(int deviceId) {
  int targetIdx = -1;
  for (int i = 0; i < m_mappings.size(); ++i) {
    if (m_mappings[i].deviceId == deviceId) {
      targetIdx = i;
      break;
    }
  }
  if (targetIdx < 0) return;

  DeviceBitMapping &m = m_mappings[targetIdx];

  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("编辑设备信息 (ID: #%1)").arg(deviceId));
  dlg.setMinimumWidth(380);
  dlg.setStyleSheet(
      "QDialog { background: #111827; color: #E2E8F0; font-family: 'Microsoft YaHei'; }"
      "QLabel { color: #94A3B8; font-size: 12px; }"
      "QLineEdit, QComboBox, QSpinBox { background: #1E293B; color: #E2E8F0; border: 1px solid #334155; padding: 5px; border-radius: 4px; font-size: 12px; }"
      "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #00D4FF; }"
      "QPushButton { background: #1E3A5F; color: #00D4FF; border: 1px solid #00D4FF; padding: 6px 16px; border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #00D4FF; color: #111827; }");

  auto *layout = new QFormLayout(&dlg);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(12);

  auto *labelEdit = new QLineEdit(m.label, &dlg);
  layout->addRow(QStringLiteral("设备名称/标签:"), labelEdit);

  auto *typeCombo = new QComboBox(&dlg);
  typeCombo->addItem(QStringLiteral("烟温探测器"), QStringLiteral("detector"));
  typeCombo->addItem(QStringLiteral("分配阀(蝶阀)"), QStringLiteral("valve_distributor"));
  typeCombo->addItem(QStringLiteral("区域阀(闸阀)"), QStringLiteral("valve_zone"));
  typeCombo->addItem(QStringLiteral("总管隔离阀(截止阀)"), QStringLiteral("valve_main_isolation"));
  typeCombo->addItem(QStringLiteral("控制分配阀"), QStringLiteral("valve"));
  typeCombo->addItem(QStringLiteral("手动报警按钮"), QStringLiteral("manual_alarm"));
  typeCombo->addItem(QStringLiteral("1301气体钢瓶"), QStringLiteral("gas_cylinder"));
  typeCombo->addItem(QStringLiteral("水泵"), QStringLiteral("water_pump"));
  typeCombo->addItem(QStringLiteral("压力开关"), QStringLiteral("pressure_switch"));
  typeCombo->addItem(QStringLiteral("移动喷枪"), QStringLiteral("mobile_spray_gun"));
  int tIdx = typeCombo->findData(m.deviceType);
  if (tIdx >= 0) typeCombo->setCurrentIndex(tIdx);
  layout->addRow(QStringLiteral("设备类型:"), typeCombo);

  auto *canIdEdit = new QLineEdit(QString("0x%1").arg(m.canId, 0, 16).toUpper(), &dlg);
  layout->addRow(QStringLiteral("CAN 帧 ID (hex):"), canIdEdit);

  auto *byteSpin = new QSpinBox(&dlg);
  byteSpin->setRange(0, 7);
  byteSpin->setValue(m.byteIndex);
  layout->addRow(QStringLiteral("字节索引 (0-7):"), byteSpin);

  auto *bitSpin = new QSpinBox(&dlg);
  bitSpin->setRange(0, 7);
  bitSpin->setValue(m.bitIndex);
  layout->addRow(QStringLiteral("位索引 (0-7):"), bitSpin);

  auto *defaultCombo = new QComboBox(&dlg);
  defaultCombo->addItem(QStringLiteral("0 (正常/关闭)"), 0);
  defaultCombo->addItem(QStringLiteral("1 (报警/开启)"), 1);
  defaultCombo->setCurrentIndex(m.defaultVal == 1 ? 1 : 0);
  layout->addRow(QStringLiteral("默认状态:"), defaultCombo);

  auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  layout->addRow(btnBox);

  if (dlg.exec() == QDialog::Accepted) {
    m.label = labelEdit->text().trimmed();
    m.deviceType = typeCombo->currentData().toString();
    bool ok = false;
    quint32 cid = canIdEdit->text().trimmed().toUInt(&ok, 0);
    if (ok) m.canId = cid;
    m.byteIndex = byteSpin->value();
    m.bitIndex = bitSpin->value();
    m.defaultVal = defaultCombo->currentData().toInt();

    rebuildGrid();
    saveConfig();

    for (auto *client : m_clients) {
      sendConfigToClient(client);
    }

    appendLog(QStringLiteral("✓ 设备 #%1 [%2] 信息已修改").arg(m.deviceId).arg(m.label));
  }
}

void DeviceMonitorPanel::toggleFullScreen() {
  QWidget *topWin = window();
  if (!topWin) topWin = topLevelWidget();

  m_isFullScreen = !m_isFullScreen;
  if (m_isFullScreen) {
    if (m_toolbar) m_toolbar->setVisible(false);
    if (m_bottomBar) m_bottomBar->setVisible(false);
    if (topWin) topWin->showFullScreen();
    if (m_btnFullScreen) m_btnFullScreen->setText(QStringLiteral("🔙 退出全屏"));
    appendLog(QStringLiteral("🖥 已进入全屏显示模式 (按 ESC 退出)"), false);
  } else {
    if (m_toolbar) m_toolbar->setVisible(true);
    if (m_bottomBar) m_bottomBar->setVisible(true);
    if (topWin) topWin->showNormal();
    if (m_btnFullScreen) m_btnFullScreen->setText(QStringLiteral("📺 全屏"));
    appendLog(QStringLiteral("🖥 已退出全屏显示模式"), false);
  }
}

void DeviceMonitorPanel::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape && m_isFullScreen) {
    toggleFullScreen();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}




