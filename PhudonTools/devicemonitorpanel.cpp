#include "devicemonitorpanel.h"
#include "roommanagerdialog.h"

#include <QAction>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QScopeGuard>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTabBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWindow>

#include "caninterface.h"
#include "canprotocolconfigdialog.h"
#include "devicestatuswidget.h"
#include "roomwidget.h"

// ========== 布局模板背景轮廓图 ==========

class TemplateBackground : public QWidget {
public:
  QString tpl;
  explicit TemplateBackground(QWidget *parent = nullptr) : QWidget(parent) {
    setObjectName(QStringLiteral("TemplateBackground"));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    if (tpl.isEmpty())
      return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c(0x00, 0xD4, 0xFF, 15);
    QPen pen(c, 1.5, Qt::DotLine);
    p.setPen(pen);
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 8));

    if (tpl == QStringLiteral("submarine")) {
      // 核潜艇外形
      QPainterPath hull;
      hull.moveTo(120, 350);
      hull.cubicTo(50, 200, 300, 180, 500, 180);
      hull.lineTo(1400, 180);
      hull.cubicTo(1550, 180, 1580, 300, 1580, 350);
      hull.cubicTo(1580, 420, 1550, 540, 1400, 540);
      hull.lineTo(500, 540);
      hull.cubicTo(300, 540, 50, 500, 120, 350);
      p.drawPath(hull);
      // 指挥塔
      p.drawRect(400, 100, 150, 80);
      // 螺旋桨
      p.setPen(QPen(c, 2));
      p.drawLine(1560, 350, 1600, 310);
      p.drawLine(1560, 350, 1600, 350);
      p.drawLine(1560, 350, 1600, 390);
      // 隔舱
      p.setPen(pen);
      p.drawLine(350, 200, 350, 520);
      p.drawLine(650, 200, 650, 520);
      p.drawLine(900, 200, 900, 520);
      p.drawLine(1150, 200, 1150, 520);
    } else if (tpl == QStringLiteral("building")) {
      p.drawRoundedRect(80, 80, 640, 400, 8, 8);
      p.drawLine(80, 215, 720, 215);
      p.drawLine(80, 350, 720, 350);
      for (int x = 160; x < 720; x += 120)
        p.drawLine(x, 80, x, 480);
      p.drawLine(400, 80, 400, 40);
      p.drawEllipse(QPoint(400, 40), 6, 6);
    } else if (tpl == QStringLiteral("warship")) {
      QPainterPath ship;
      ship.moveTo(80, 250);
      ship.lineTo(150, 180);
      ship.lineTo(350, 180);
      ship.lineTo(400, 130);
      ship.lineTo(500, 130);
      ship.lineTo(550, 180);
      ship.lineTo(700, 180);
      ship.lineTo(750, 160);
      ship.lineTo(800, 160);
      ship.lineTo(850, 200);
      ship.lineTo(980, 200);
      ship.lineTo(1020, 250);
      ship.closeSubpath();
      p.drawPath(ship);
      p.drawLine(80, 250, 1020, 250);
      p.drawRect(200, 170, 50, 30);
      p.drawLine(450, 130, 450, 60);
      p.drawEllipse(QPoint(450, 55), 8, 5);
    } else if (tpl == QStringLiteral("carrier")) {
      QPainterPath deck;
      deck.moveTo(30, 220);
      deck.lineTo(30, 170);
      deck.lineTo(280, 100);
      deck.lineTo(550, 100);
      deck.lineTo(1010, 170);
      deck.lineTo(1010, 220);
      deck.closeSubpath();
      p.drawPath(deck);
      p.drawRect(590, 60, 160, 130);
      p.drawLine(30, 220, 1010, 220);
      for (int x = 100; x < 500; x += 80) {
        p.drawLine(x, 120, x - 30, 180);
        p.drawLine(x + 40, 120, x + 10, 180);
      }
    }
  }
};

// ========== 多屏联动独立监控窗口 (显示区域实际 2D 布局图) ==========

class SubMonitorWindow : public QWidget {
public:
  QString viewName;
  QLabel *lblTitle = nullptr;
  QScrollArea *scrollArea = nullptr;
  QWidget *canvasContainer = nullptr;
  QList<RoomWidget *> roomWidgets;
  QHash<int, DeviceStatusWidget *> deviceWidgets;

  explicit SubMonitorWindow(const QString &title, QWidget *parent = nullptr)
      : QWidget(parent), viewName(title) {
    // Use Qt::Window so it appears as independent OS window (no dialog chrome).
    // Do NOT use QDialog - its accept/reject/done machinery and modal event
    // loop can interact badly with our custom window lifecycle management.
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false); // We manage lifetime via m_subWindows
    setWindowTitle(QStringLiteral("多屏联动监控 - %1").arg(title));
    setMinimumSize(850, 600);
    setStyleSheet("QWidget { background-color: #0D1117; color: #E2E8F0; }");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 头部信息
    auto *header = new QWidget(this);
    header->setFixedHeight(36);
    header->setStyleSheet("background-color: #111827; border: 1px solid "
                          "#1E293E; border-radius: 4px;");
    auto *hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(10, 2, 10, 2);

    lblTitle = new QLabel(
        QStringLiteral("🖥️ 多屏联动拓展窗口  |  区域实际 2D 布局图: %1").arg(title), header);
    lblTitle->setStyleSheet("color: #00D4FF; font-size: 12px; font-weight: "
                            "bold; font-family: 'Microsoft YaHei';");
    hLayout->addWidget(lblTitle);
    hLayout->addStretch();
    mainLayout->addWidget(header);

    // 画布视图区
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background-color: #0D1117; border: 1px solid #1E293E; }"
        "QScrollBar:vertical { background: #0D1117; width: 8px; }"
        "QScrollBar::handle:vertical { background: #1E293E; border-radius: "
        "4px; min-height: 30px; }");

    canvasContainer = new QWidget();
    canvasContainer->setStyleSheet("background-color: #0D1117;");
    scrollArea->setWidget(canvasContainer);

    mainLayout->addWidget(scrollArea, 1);
  }

  void setViewName(const QString &name) {
    if (viewName != name) {
      viewName = name;
      setWindowTitle(QStringLiteral("多屏联动监控 - %1").arg(name));
      if (lblTitle) {
        lblTitle->setText(
            QStringLiteral("🖥️ 多屏联动拓展窗口  |  区域实际 2D 布局图: %1").arg(name));
      }
    }
  }

  void rebuildDevicesAndRooms(const QList<DeviceBitMapping> &mappings,
                             const QHash<int, DeviceStatusWidget *> &mainWidgets,
                             const QList<RoomRegion> &rooms,
                             const QHash<int, QPoint> &deviceRoomPos,
                             const QString &activeTemplate) {
    // 1. 清理旧控件 —— 使用同步 delete 而非 deleteLater，避免延迟删除
    //    导致下次 rebuildDevicesAndRooms 时发现未删除的旧控件而产生
    //    重复 deleteLater / 状态不一致 / use-after-free 崩溃。
    //    由于 setParent(nullptr) 已将控件移出父级链，delete 不会触发双重释放。

    // 收集本轮需要保留/创建的设备 ID（仅属于此 viewName 的）
    QSet<int> neededDeviceIds;
    for (const auto &m : mappings) {
      QString targetView = m.targetView.trimmed();
      if (targetView.isEmpty()) targetView = QStringLiteral("界面1");
      if (targetView == viewName) {
        neededDeviceIds.insert(m.deviceId);
      }
    }

    // 1a. 删除不再需要的旧设备控件（使用 deleteLater 避免在绘制/事件处理中野指针）
    for (auto it = deviceWidgets.begin(); it != deviceWidgets.end(); ) {
      if (!neededDeviceIds.contains(it.key())) {
        if (it.value()) {
          it.value()->hide();
          delete it.value();
        }
        it = deviceWidgets.erase(it);
      } else {
        ++it;
      }
    }

    // 1b. 清理旧模板背景
    QList<TemplateBackground *> oldBgs = canvasContainer->findChildren<TemplateBackground *>();
    for (auto *child : oldBgs) {
      if (child) {
        child->hide();
        delete child;
      }
    }

    // 1c. 清理旧房间区域
    for (auto *rw : roomWidgets) {
      if (rw) {
        rw->hide();
        delete rw;
      }
    }
    roomWidgets.clear();

    // 1d. 隐藏仍保留的设备控件（将在步骤 3 中重新显示并定位）
    for (auto *w : deviceWidgets) {
      w->setVisible(false);
    }

    // 2. 创建属于此界面的房间区域 (RoomWidgets)
    for (const auto &r : rooms) {
      QString rView = r.targetView.trimmed();
      if (rView.isEmpty()) rView = QStringLiteral("界面1");
      bool matchesView =
          (rView == viewName) ||
          (viewName == QStringLiteral("界面1") &&
           (rView == QStringLiteral("界面1") || rView.isEmpty()));
      if (!matchesView) continue;

      auto *rw = new RoomWidget(r.id, r.name, r.geom, r.shape, canvasContainer);
      rw->setEditingEnabled(false);
      rw->setAttribute(Qt::WA_TransparentForMouseEvents); // 副屏上仅展示
      roomWidgets.append(rw);
      rw->show();
    }

    // 计算画布的适应尺寸
    int maxX = 850, maxY = 600;
    for (const auto &r : rooms) {
      maxX = qMax(maxX, r.geom.right() + 50);
      maxY = qMax(maxY, r.geom.bottom() + 50);
    }

    // 3. 放置/复用属于 viewName 的设备图标到 2D 布局坐标 (deviceRoomPos)
    int countInView = 0;
    for (const auto &m : mappings) {
      QString targetView = m.targetView.trimmed();
      if (targetView.isEmpty()) targetView = QStringLiteral("界面1");
      if (targetView != viewName) continue;

      countInView++;

      DeviceStatusWidget::DeviceKind kind = DeviceStatusWidget::Detector;
      if (m.deviceType == QStringLiteral("valve")) kind = DeviceStatusWidget::Valve;
      else if (m.deviceType == QStringLiteral("valve_distributor")) kind = DeviceStatusWidget::ValveDistributor;
      else if (m.deviceType == QStringLiteral("valve_zone")) kind = DeviceStatusWidget::ValveZone;
      else if (m.deviceType == QStringLiteral("valve_main_isolation")) kind = DeviceStatusWidget::ValveMainIsolation;
      else if (m.deviceType == QStringLiteral("manual_alarm")) kind = DeviceStatusWidget::ManualAlarm;
      else if (m.deviceType == QStringLiteral("gas_cylinder")) kind = DeviceStatusWidget::GasCylinder;
      else if (m.deviceType == QStringLiteral("water_pump")) kind = DeviceStatusWidget::WaterPump;
      else if (m.deviceType == QStringLiteral("pressure_switch")) kind = DeviceStatusWidget::PressureSwitch;
      else if (m.deviceType == QStringLiteral("mobile_spray_gun")) kind = DeviceStatusWidget::MobileSprayGun;

      DeviceStatusWidget *widget = deviceWidgets.value(m.deviceId, nullptr);
      if (!widget) {
        widget = new DeviceStatusWidget(m.deviceId, kind, m.label, m.canId, canvasContainer);
        widget->setDefaultVal(m.defaultVal);
        widget->setStatus(m.defaultVal == 1);
        widget->setDraggable(false); // 副屏上不可拖拽
        deviceWidgets.insert(m.deviceId, widget);
      } else {
        widget->setLabel(m.label);
        widget->setCanId(m.canId);
        widget->setDeviceKind(kind);
      }

      if (mainWidgets.contains(m.deviceId)) {
        widget->setStatus(mainWidgets[m.deviceId]->status());
      }

      // 计算位置：如果在 deviceRoomPos 中，按坐标定位在 2D 布局图上；否则按网格排列
      if (deviceRoomPos.contains(m.deviceId)) {
        QPoint pos = deviceRoomPos.value(m.deviceId);
        widget->setGeometry(pos.x(), pos.y(), 128, 155);
        maxX = qMax(maxX, pos.x() + 180);
        maxY = qMax(maxY, pos.y() + 200);
      } else {
        int r = (countInView - 1) / 5;
        int c = (countInView - 1) % 5;
        widget->setGeometry(40 + c * 140, maxY + 20 + r * 160, 128, 155);
      }
      widget->show();
      widget->raise();
    }

    // 适配画布尺寸
    int vpW = scrollArea->viewport()->width();
    int vpH = scrollArea->viewport()->height();
    canvasContainer->setFixedSize(qMax(vpW, maxX), qMax(vpH, maxY));

    // 模板背景轮廓图
    if (!activeTemplate.isEmpty()) {
      auto *bg = new TemplateBackground(canvasContainer);
      bg->tpl = activeTemplate;
      bg->setGeometry(0, 0, maxX, maxY);
      bg->lower();
      bg->show();
    }

    // 更新房间卡片内的设备数统计
    for (auto *rw : roomWidgets) {
      int cnt = 0;
      for (const auto &m : mappings) {
        if (!deviceRoomPos.contains(m.deviceId)) continue;
        auto *w = deviceWidgets.value(m.deviceId, nullptr);
        if (w && w->isVisible()) {
          QRect devGeom(w->pos(), w->size());
          if (rw->geometry().contains(devGeom.center())) ++cnt;
        }
      }
      rw->setDeviceCount(cnt);
    }
  }

  void closeEvent(QCloseEvent *event) override {
    // Just hide instead of closing/destroying - lifecycle managed by DeviceMonitorPanel
    event->ignore();
    hide();
  }

  ~SubMonitorWindow() override {
    for (auto *w : deviceWidgets) {
      if (w) {
        delete w;
      }
    }
    deviceWidgets.clear();

    for (auto *rw : roomWidgets) {
      if (rw) {
        delete rw;
      }
    }
    roomWidgets.clear();
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

    lblTitle = new QLabel(
        QStringLiteral("CAN2.0B Status Monitor  |  v2.0  |  Phudon"));
    lblTitle->setStyleSheet("color:#00D4FF;font-size:10px;font-weight:bold;"
                            "font-family:'Consolas';");
    lay->addWidget(lblTitle);
    lay->addStretch();
    lblClock = new QLabel();
    lblClock->setStyleSheet(
        "color:#64748B;font-size:10px;font-family:'Consolas';");
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
  connect(m_batchTimer, &QTimer::timeout, this,
          &DeviceMonitorPanel::processBatch);
  m_batchTimer->start();

  // Debounce timer for updateSubWindows: coalesces rapid repeated requests
  // into a single deferred execution, preventing race conditions.
  m_subWinUpdateTimer = new QTimer(this);
  m_subWinUpdateTimer->setSingleShot(true);
  m_subWinUpdateTimer->setInterval(0); // Fire on next event loop iteration
  connect(m_subWinUpdateTimer, &QTimer::timeout,
          this, &DeviceMonitorPanel::updateSubWindows);

  // 连接 CAN 帧接收信号
  connect(m_can, &CanInterface::frameReceived, this,
          &DeviceMonitorPanel::onFrameReceived);

  // 加载持久化配置
  loadConfig();

  // 启动 WebSocket 服务器
  m_wsServer = new QWebSocketServer(QStringLiteral("StatusMonitorServer"),
                                    QWebSocketServer::NonSecureMode, this);
  if (m_wsServer->listen(QHostAddress::Any, 12345)) {
    connect(m_wsServer, &QWebSocketServer::newConnection, this,
            &DeviceMonitorPanel::onNewConnection);
    appendLog(QStringLiteral("[WebSocket] 服务器已启动，监听端口 12345"),
              false);
  } else {
    appendLog(QStringLiteral("[WebSocket] 服务器启动失败，端口 12345 被占用"),
              true);
  }
}

DeviceMonitorPanel::~DeviceMonitorPanel() {
  saveConfig();
  // Stop debounce timer first to prevent any pending updateSubWindows from firing
  if (m_subWinUpdateTimer) m_subWinUpdateTimer->stop();
  // SubMonitorWindows have nullptr parent, Qt won't auto-delete them
  for (auto subWin : m_subWindows) {
    if (subWin && !subWin.isNull()) {
      subWin->hide();
      delete subWin.data();
    }
  }
  m_subWindows.clear();
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
  m_toolbar->setStyleSheet("QWidget { background-color: #111827; "
                           "border-bottom: 1px solid #1E293E; }");

  auto *tbLayout = new QHBoxLayout(m_toolbar);
  tbLayout->setContentsMargins(10, 5, 10, 5);
  tbLayout->setSpacing(8);

  const QString techBtn =
      "QPushButton { padding: 5px 14px; border-radius: 3px; font-size: 11px; "
      "font-weight: bold; font-family: 'Microsoft YaHei'; }";

  // ---- 1. ⚙ 设置菜单按钮 ----
  m_btnSettingsMenu = new QToolButton(m_toolbar);
  m_btnSettingsMenu->setText(QStringLiteral("⚙ 设置"));
  m_btnSettingsMenu->setPopupMode(QToolButton::InstantPopup);
  m_btnSettingsMenu->setStyleSheet(
      "QToolButton { color: #00D4FF; background: #1A2740; border: 1px solid #1E3A5F; "
      "padding: 5px 14px; border-radius: 3px; font-size: 11px; font-weight: bold; font-family: 'Microsoft YaHei'; }"
      "QToolButton:hover { background: #1E3A5F; border-color: #00D4FF; }"
      "QToolButton::menu-indicator { image: none; }");

  auto *settingsMenu = new QMenu(m_btnSettingsMenu);
  settingsMenu->setStyleSheet(
      "QMenu { background-color: #0F172A; border: 1px solid #1E293B; color: #E2E8F0; padding: 4px; font-family: 'Microsoft YaHei'; font-size: 12px; }"
      "QMenu::item { padding: 6px 22px; border-radius: 4px; }"
      "QMenu::item:selected { background: #0284C7; color: #FFFFFF; }");

  QAction *actConfig = settingsMenu->addAction(QStringLiteral("⚙ 设备映射配置"));
  connect(actConfig, &QAction::triggered, this, &DeviceMonitorPanel::onConfigClicked);

  QMenu *iconSubMenu = settingsMenu->addMenu(QStringLiteral("🎨 图标风格设置"));
  iconSubMenu->setStyleSheet(settingsMenu->styleSheet());
  QAction *actIconDefault = iconSubMenu->addAction(QStringLiteral("🎨 默认风格"));
  QAction *actIconSimple = iconSubMenu->addAction(QStringLiteral("🎨 简约风格"));
  QAction *actIconRetro = iconSubMenu->addAction(QStringLiteral("🎨 复古风格"));
  connect(actIconDefault, &QAction::triggered, this, [this]() { DeviceStatusWidget::setIconStyle(0); update(); for(auto *w: m_deviceWidgets) w->update(); });
  connect(actIconSimple, &QAction::triggered, this, [this]() { DeviceStatusWidget::setIconStyle(1); update(); for(auto *w: m_deviceWidgets) w->update(); });
  connect(actIconRetro, &QAction::triggered, this, [this]() { DeviceStatusWidget::setIconStyle(2); update(); for(auto *w: m_deviceWidgets) w->update(); });

  QMenu *zoomSubMenu = settingsMenu->addMenu(QStringLiteral("🔍 显示缩放设置"));
  zoomSubMenu->setStyleSheet(settingsMenu->styleSheet());
  QAction *actZoomIn = zoomSubMenu->addAction(QStringLiteral("🔍+ 放大视角"));
  QAction *actZoomOut = zoomSubMenu->addAction(QStringLiteral("🔍− 缩小视角"));
  QAction *actZoomFit = zoomSubMenu->addAction(QStringLiteral("⊡ 视口自适应"));
  connect(actZoomIn, &QAction::triggered, this, &DeviceMonitorPanel::zoomIn);
  connect(actZoomOut, &QAction::triggered, this, &DeviceMonitorPanel::zoomOut);
  connect(actZoomFit, &QAction::triggered, this, &DeviceMonitorPanel::zoomFit);

  settingsMenu->addSeparator();

  QAction *actReset = settingsMenu->addAction(QStringLiteral("🗑 重置所有设置"));
  connect(actReset, &QAction::triggered, this, &DeviceMonitorPanel::onResetClicked);

  m_btnSettingsMenu->setMenu(settingsMenu);
  tbLayout->addWidget(m_btnSettingsMenu);

  // ---- 2. 📐 布局/视图 切换按钮 ----
  m_btnLayoutToggle = new QPushButton(QStringLiteral("📐 布局/视图"), m_toolbar);
  m_btnLayoutToggle->setStyleSheet(
      techBtn + "QPushButton { color: #10B981; background: #1A2720; border: 1px solid #1E3E2E; }"
                "QPushButton:hover { background: #1E3E2E; border-color: #10B981; }");
  connect(m_btnLayoutToggle, &QPushButton::clicked, this, [this]() {
    toggleLayoutMode(!m_layoutEditingEnabled);
  });
  tbLayout->addWidget(m_btnLayoutToggle);

  // ---- 2.5 🏠 房间管理 按钮 ----
  auto *btnManageRooms = new QPushButton(QStringLiteral("🏠 房间管理"), m_toolbar);
  btnManageRooms->setStyleSheet(
      techBtn + "QPushButton { color: #00D4FF; background: #132438; border: 1px solid #00D4FF; }"
                "QPushButton:hover { background: #00D4FF; color: #0F172A; }");
  tbLayout->addWidget(btnManageRooms);
  connect(btnManageRooms, &QPushButton::clicked, this, &DeviceMonitorPanel::onManageRoomsRequested);

  auto *btnAutoArrangeToolbar = new QPushButton(QStringLiteral("🧹 一键整理"), m_toolbar);
  btnAutoArrangeToolbar->setStyleSheet(
      techBtn + "QPushButton { color: #10B981; background: #064E3B; border: 1px solid #059669; }"
                "QPushButton:hover { background: #059669; color: #FFFFFF; }");
  tbLayout->addWidget(btnAutoArrangeToolbar);
  connect(btnAutoArrangeToolbar, &QPushButton::clicked, this, &DeviceMonitorPanel::autoArrangeRoomsAndDevices);

  // ---- 3. 📐 界面分割 ----
  m_btnSplitConfig = new QPushButton(QStringLiteral("📐 界面分割"), m_toolbar);
  m_btnSplitConfig->setStyleSheet(
      techBtn + "QPushButton { color: #38BDF8; background: #1E293B; border: 1px solid #0284C7; }"
                "QPushButton:hover { background: #0284C7; color: #FFFFFF; }");
  tbLayout->addWidget(m_btnSplitConfig);
  connect(m_btnSplitConfig, &QPushButton::clicked, this, &DeviceMonitorPanel::onSplitConfigClicked);

  // ---- 4. 🖥️ 多屏联动 ----
  m_btnMultiScreen = new QPushButton(QStringLiteral("🖥️ 多屏联动"), m_toolbar);
  m_btnMultiScreen->setCheckable(true);
  m_btnMultiScreen->setChecked(m_multiScreenActive);
  m_btnMultiScreen->setStyleSheet(
      techBtn + "QPushButton { color: #A855F7; background: #2E1065; border: 1px solid #7E22CE; }"
                "QPushButton:hover { background: #7E22CE; color: #FFFFFF; }"
                "QPushButton:checked { color: #00E676; background: #14532D; border-color: #22C55E; }");
  tbLayout->addWidget(m_btnMultiScreen);
  connect(m_btnMultiScreen, &QPushButton::toggled, this, &DeviceMonitorPanel::onMultiScreenToggled);

  // ---- 5. 📺 全屏显示 ----
  m_btnFullScreen = new QPushButton(QStringLiteral("📺 全屏显示"), m_toolbar);
  m_btnFullScreen->setStyleSheet(
      techBtn + "QPushButton { color: #F59E0B; background: #27201A; border: 1px solid #3E2E1E; }"
                "QPushButton:hover { background: #3E2E1E; border-color: #F59E0B; }");
  tbLayout->addWidget(m_btnFullScreen);
  connect(m_btnFullScreen, &QPushButton::clicked, this, &DeviceMonitorPanel::toggleFullScreen);

  // ---- 3. 信息日志按钮 ----
  m_btnInfoLog = new QPushButton(QStringLiteral("📟 信息日志"));
  m_btnInfoLog->setCheckable(true);
  m_btnInfoLog->setChecked(true);
  m_btnInfoLog->setStyleSheet(techBtn +
      "QPushButton{color:#7C879A;background:#1A2235;border:1px solid #1E293E;}"
      "QPushButton:hover{color:#00D4FF;border-color:#00D4FF;}"
      "QPushButton:checked{color:#00D4FF;border-color:#00D4FF;}");
  tbLayout->addWidget(m_btnInfoLog);

  tbLayout->addStretch();

  m_lblCount = new QLabel(QStringLiteral("CAN 2.0B Protocol Monitor"));
  m_lblCount->setStyleSheet(
      "color: #475569; font-size: 10px; "
      "font-family: 'Consolas', monospace; padding-right: 4px;");
  tbLayout->addWidget(m_lblCount);

  mainLayout->addWidget(m_toolbar);

  // 监听屏幕分布硬件变化
  connect(qApp, &QGuiApplication::screenAdded, this,
          &DeviceMonitorPanel::onScreenLayoutChanged);
  connect(qApp, &QGuiApplication::screenRemoved, this,
          &DeviceMonitorPanel::onScreenLayoutChanged);

  // ---- 未摆放设备停靠区 (仅布局模式可见) ----
  m_unplacedDock = new QWidget();
  m_unplacedDock->setVisible(false);
  m_unplacedDock->setStyleSheet("background: rgba(15, 23, 42, 0.85);"
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
      "QScrollBar::handle:horizontal { background: #1E293E; border-radius: "
      "3px; min-width: 20px; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
      "width: 0px; }");

  m_unplacedContainer = new QWidget();
  m_unplacedContainer->setStyleSheet("background: transparent; border: none;");
  m_unplacedLayout = new QHBoxLayout(m_unplacedContainer);
  m_unplacedLayout->setContentsMargins(4, 4, 4, 4);
  m_unplacedLayout->setSpacing(14);
  m_unplacedLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  dockScroll->setWidget(m_unplacedContainer);
  dockVLayout->addWidget(dockScroll);
  mainLayout->addWidget(m_unplacedDock);

  // ---- 设备网格区域 (可滚动，独占全屏区域) ----
  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidgetResizable(true); // 默认网格模式：自动扩展填充视口
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setStyleSheet(
      "QScrollArea { background-color: #0D1117; border: none; }"
      "QScrollBar:vertical { background: #0D1117; width: 8px; }"
      "QScrollBar::handle:vertical { background: #1E293E; border-radius: 4px; "
      "min-height: 30px; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: "
      "0px; }"
      "QScrollBar:horizontal { background: #0D1117; height: 8px; }"
      "QScrollBar::handle:horizontal { background: #1E293E; border-radius: "
      "4px; min-width: 30px; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
      "width: 0px; }");

  m_gridContainer = new QWidget();
  m_gridContainer->setAcceptDrops(true);
  m_gridContainer->installEventFilter(this);
  m_gridContainer->setStyleSheet("background-color: #0D1117;");
  m_gridLayout = new QGridLayout(m_gridContainer);
  m_gridLayout->setContentsMargins(20, 20, 20, 20);
  m_gridLayout->setSpacing(18);
  m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  m_scrollArea->setWidget(m_gridContainer);
  mainLayout->addWidget(m_scrollArea, 1);

  // ---- 底部状态栏 ----
  m_bottomBar = new BottomStatusBar(this);
  mainLayout->addWidget(m_bottomBar);

  // ---- 悬浮事件日志框 (可拖拽、可关闭、科技感 HUD 风格) ----
  m_logWrapper = new QWidget(this);
  m_logWrapper->setObjectName("FloatingLog");
  m_logWrapper->setFixedSize(360, 220);
  m_logWrapper->setStyleSheet(
      "QWidget#FloatingLog { background: rgba(13, 18, 30, 0.93); "
      "border: 1px solid #00D4FF; border-radius: 6px; }");

  // 阴影效果
  auto *shadow = new QGraphicsDropShadowEffect(m_logWrapper);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 212, 255, 60));
  shadow->setOffset(0, 4);
  m_logWrapper->setGraphicsEffect(shadow);

  auto *logWLayout = new QVBoxLayout(m_logWrapper);
  logWLayout->setContentsMargins(1, 1, 1, 1);
  logWLayout->setSpacing(0);

  // 可拖拽日志标题栏
  m_logTitleBar = new QWidget(m_logWrapper);
  m_logTitleBar->setFixedHeight(28);
  m_logTitleBar->setCursor(Qt::SizeAllCursor);
  m_logTitleBar->setStyleSheet(
      "background: #111B2E; border-bottom: 1px solid #1E293E; "
      "border-top-left-radius: 5px; border-top-right-radius: 5px;");
  m_logTitleBar->installEventFilter(this);

  auto *ltLayout = new QHBoxLayout(m_logTitleBar);
  ltLayout->setContentsMargins(10, 0, 4, 0);
  auto *logTitleLbl =
      new QLabel(QStringLiteral("📋 事件日志 (按住标题栏拖拽)"));
  logTitleLbl->setStyleSheet("color: #00D4FF; font-size: 10px; font-weight: "
                             "bold; font-family: 'Microsoft YaHei';");
  ltLayout->addWidget(logTitleLbl);
  ltLayout->addStretch();

  auto *btnClearLog = new QPushButton(QStringLiteral("🗑"));
  btnClearLog->setFixedSize(22, 22);
  btnClearLog->setToolTip(QStringLiteral("清除日志"));
  btnClearLog->setStyleSheet(
      "QPushButton{color:#64748B;background:transparent;border:none;font-size:"
      "12px;}QPushButton:hover{color:#F87171;}");
  ltLayout->addWidget(btnClearLog);

  m_btnToggleLogSize = new QPushButton(QStringLiteral("🗖"));
  m_btnToggleLogSize->setFixedSize(22, 22);
  m_btnToggleLogSize->setToolTip(QStringLiteral("放大日志框"));
  m_btnToggleLogSize->setStyleSheet(
      "QPushButton{color:#00D4FF;background:transparent;border:none;font-size:"
      "12px;}QPushButton:hover{color:#38BDF8;}");
  ltLayout->addWidget(m_btnToggleLogSize);

  auto *btnCloseLog = new QPushButton(QStringLiteral("✕"));
  btnCloseLog->setFixedSize(22, 22);
  btnCloseLog->setToolTip(QStringLiteral("关闭日志框"));
  btnCloseLog->setStyleSheet(
      "QPushButton{color:#64748B;background:transparent;border:none;font-size:"
      "11px;}QPushButton:hover{color:#EF4444;}");
  ltLayout->addWidget(btnCloseLog);

  logWLayout->addWidget(m_logTitleBar);

  m_log = new QPlainTextEdit(m_logWrapper);
  m_log->setReadOnly(true);
  m_log->setFrameShape(QFrame::NoFrame);
  m_log->setStyleSheet(
      "QPlainTextEdit { background: transparent; color: #94A3B8; "
      "font-size: 11px; font-family: 'Microsoft YaHei', 'Consolas', monospace; "
      "padding: 6px; border: none; }");
  m_log->setPlaceholderText(QStringLiteral("事件日志 — 设备状态变更记录"));
  logWLayout->addWidget(m_log);

  // 清除/放大/关闭/显示连接
  connect(btnClearLog, &QPushButton::clicked, this,
          [this]() { m_log->clear(); });
  connect(m_btnToggleLogSize, &QPushButton::clicked, this, [this]() {
    m_logExpanded = !m_logExpanded;
    if (m_logExpanded) {
      int expW = qMin(640, width() - 40);
      int expH = qMin(420, height() - 80);
      m_logWrapper->setFixedSize(expW, expH);
      m_btnToggleLogSize->setText(QStringLiteral("🗗"));
      m_btnToggleLogSize->setToolTip(QStringLiteral("还原日志框"));
    } else {
      m_logWrapper->setFixedSize(360, 220);
      m_btnToggleLogSize->setText(QStringLiteral("🗖"));
      m_btnToggleLogSize->setToolTip(QStringLiteral("放大日志框"));
    }
    repositionFloatingWidgets();
  });
  connect(btnCloseLog, &QPushButton::clicked, this, [this]() {
    if (m_btnInfoLog)
      m_btnInfoLog->setChecked(false);
  });
  connect(m_btnInfoLog, &QPushButton::toggled, this, [this](bool checked) {
    if (m_logWrapper) {
      m_logWrapper->setVisible(checked);
      if (checked) {
        repositionFloatingWidgets();
        m_logWrapper->raise();
      }
    }
  });

  // 系统时钟定时器
  auto *clockTimer = new QTimer(this);
  connect(clockTimer, &QTimer::timeout, this, [this]() {
    if (m_bottomBar && m_bottomBar->lblClock) {
      m_bottomBar->lblClock->setText(
          QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss"));
    }
  });
  clockTimer->start(1000);

  // ---- 界面切换开关：悬浮放置于界面底端居中 ----
  m_floatingTabWrapper = new QWidget(this);
  m_floatingTabWrapper->setObjectName("floatingTabWrapper");
  m_floatingTabWrapper->setStyleSheet(
      "QWidget#floatingTabWrapper { background: rgba(15, 23, 42, 0.95); "
      "border: 1px solid rgba(0, 212, 255, 0.5); border-radius: 18px; }");

  auto *tabShadow = new QGraphicsDropShadowEffect(m_floatingTabWrapper);
  tabShadow->setBlurRadius(16);
  tabShadow->setColor(QColor(0, 212, 255, 90));
  tabShadow->setOffset(0, 2);
  m_floatingTabWrapper->setGraphicsEffect(tabShadow);

  auto *fwLayout = new QHBoxLayout(m_floatingTabWrapper);
  fwLayout->setContentsMargins(8, 4, 8, 4);
  fwLayout->setSpacing(0);

  m_viewTabBar = new QTabBar(m_floatingTabWrapper);
  m_viewTabBar->setDrawBase(false);
  m_viewTabBar->setExpanding(false);
  m_viewTabBar->setUsesScrollButtons(false); // 禁用左右箭头按钮，长度根据界面数量自动扩展档位
  m_viewTabBar->setElideMode(Qt::ElideNone);
  m_viewTabBar->setStyleSheet(
      "QTabBar { background: transparent; border: none; margin: 0px; padding: "
      "0px; }"
      "QTabBar::tab { background: transparent; color: #94A3B8; border: none; "
      "border-radius: 14px; padding: 5px 16px; margin: 0px 2px; font-size: "
      "11px; font-weight: bold; font-family: 'Microsoft YaHei'; }"
      "QTabBar::tab:selected { background: qlineargradient(x1:0, y1:0, x2:1, "
      "y2:0, stop:0 #0284C7, stop:1 #00D4FF); color: #FFFFFF; font-weight: "
      "bold; }"
      "QTabBar::tab:hover:!selected { color: #F1F5F9; background: rgba(30, 41, "
      "59, 0.6); }");
  fwLayout->addWidget(m_viewTabBar);
  connect(m_viewTabBar, &QTabBar::currentChanged, this,
          &DeviceMonitorPanel::onTabChanged);
  if (m_bottomBar && m_bottomBar->lblClock) {
    m_bottomBar->lblClock->setText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss"));
  }

  loadRoomLayout();

  // 初始占位与全量画布渲染
  rebuildRoomCanvas();
}

// ========== CAN 帧处理 ==========

void DeviceMonitorPanel::onFrameReceived(const CanFrame &frame) {
  if (m_mappings.isEmpty())
    return;
  m_frameCount++;
  m_ringBuffer.push(frame);
}

void DeviceMonitorPanel::buildMappingHash() {
  m_canIdToMappingIndices.clear();
  for (int i = 0; i < m_mappings.size(); ++i) {
    m_canIdToMappingIndices[m_mappings[i].canId].append(i);
  }
}

void DeviceMonitorPanel::processBatch() {
  if (m_ringBuffer.isEmpty() || m_updatingSubWindows || m_isRebuildingCanvas)
    return;

  std::vector<CanFrame> batch;
  m_ringBuffer.pop_batch(batch, 4096);

  // 拷贝当前子窗口列表快照，防止遍历过程中 UI 重建破坏指针
  QList<QPointer<SubMonitorWindow>> subWinSnapshot = m_subWindows;

  for (const auto &frame : batch) {
    const auto &indices = m_canIdToMappingIndices.value(frame.id);
    if (indices.isEmpty())
      continue;

    for (int idx : indices) {
      if (idx < 0 || idx >= m_mappings.size()) continue;
      const auto &mapping = m_mappings[idx];
      if (mapping.byteIndex >= frame.data.size())
        continue;

      const quint8 byteVal = static_cast<quint8>(frame.data[mapping.byteIndex]);
      const bool bitVal = (byteVal >> mapping.bitIndex) & 0x01;

      auto *widget = m_deviceWidgets.value(mapping.deviceId, nullptr);
      if (!widget)
        continue;

      const bool prev = widget->status();
      widget->setStatus(bitVal);

      // 安全地同步更新多屏联动独立窗口中的设备状态
      for (auto subWin : subWinSnapshot) {
        if (subWin && !subWin.isNull()) {
          auto *subWidget = subWin->deviceWidgets.value(mapping.deviceId, nullptr);
          if (subWidget) {
            subWidget->setStatus(bitVal);
          }
        }
      }

      if (bitVal != prev) {
        // 设备类型中文名
        QString typeName;
        if (mapping.deviceType == QStringLiteral("detector"))
          typeName = QStringLiteral("烟温探测器");
        else if (mapping.deviceType == QStringLiteral("valve"))
          typeName = QStringLiteral("控制分配阀");
        else if (mapping.deviceType == QStringLiteral("valve_distributor"))
          typeName = QStringLiteral("分配阀");
        else if (mapping.deviceType == QStringLiteral("valve_zone"))
          typeName = QStringLiteral("区域阀");
        else if (mapping.deviceType == QStringLiteral("valve_main_isolation"))
          typeName = QStringLiteral("总管隔离阀");
        else if (mapping.deviceType == QStringLiteral("manual_alarm"))
          typeName = QStringLiteral("手动报警按钮");
        else if (mapping.deviceType == QStringLiteral("gas_cylinder"))
          typeName = QStringLiteral("1301气体钢瓶");
        else if (mapping.deviceType == QStringLiteral("water_pump"))
          typeName = QStringLiteral("水泵");
        else if (mapping.deviceType == QStringLiteral("pressure_switch"))
          typeName = QStringLiteral("压力开关");
        else if (mapping.deviceType == QStringLiteral("mobile_spray_gun"))
          typeName = QStringLiteral("移动喷枪");
        else
          typeName = mapping.deviceType;

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
        updateObj[QStringLiteral("timestamp")] =
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        broadcastMessage(updateObj);
      }
    }
  }
}

void DeviceMonitorPanel::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  if (m_logWrapper && m_logWrapper->isVisible()) {
    int bHeight = m_bottomBar ? m_bottomBar->height() : 30;
    int tHeight = m_toolbar ? m_toolbar->height() : 42;
    int maxX = qMax(0, width() - m_logWrapper->width() - 20);
    int maxY = qMax(tHeight, height() - m_logWrapper->height() - bHeight - 15);
    QPoint curPos = m_logWrapper->pos();
    if (curPos.x() == 0 && curPos.y() == 0) {
      m_logWrapper->move(maxX, maxY);
    } else {
      m_logWrapper->move(qBound(0, curPos.x(), maxX),
                         qBound(tHeight, curPos.y(), maxY));
    }
    m_logWrapper->raise();
  }

  // 居中悬浮底端 Tab 切换开关栏
  if (m_floatingTabWrapper && m_floatingTabWrapper->isVisible()) {
    m_floatingTabWrapper->adjustSize();
    int bHeight =
        (m_bottomBar && m_bottomBar->isVisible()) ? m_bottomBar->height() : 0;
    int tabW = m_floatingTabWrapper->width();
    int tabH = m_floatingTabWrapper->height();
    int posX = qMax(10, (width() - tabW) / 2);
    int posY = height() - tabH - bHeight - 12;
    m_floatingTabWrapper->move(posX, posY);
    m_floatingTabWrapper->raise();
  }

  if (!m_scrollArea || m_mappings.isEmpty())
    return;

  updateZoom();
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
  client->sendTextMessage(
      QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
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
  connect(client, &QWebSocket::disconnected, this,
          &DeviceMonitorPanel::onClientDisconnected);
  m_clients.append(client);

  appendLog(QStringLiteral("[WebSocket] 客户端已连接：%1:%2")
                .arg(client->peerAddress().toString())
                .arg(client->peerPort()),
            false);

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

  dlg.setAvailableViews(m_viewNames);
  QStringList roomNames;
  for (const auto &r : m_rooms) {
    if (!r.name.trimmed().isEmpty() && !roomNames.contains(r.name.trimmed())) {
      roomNames.append(r.name.trimmed());
    }
  }
  dlg.setAvailableRooms(roomNames);
  dlg.setMappings(m_mappings);
  if (dlg.exec() == QDialog::Accepted) {
    m_mappings = dlg.mappings();
    autoArrangeRoomDevices();
    rebuildRoomCanvas();
    updateZoom();
    updateUnplacedDock();
    saveRoomLayout();
    saveConfig();
    appendLog(
        QStringLiteral("✓ 配置已更新 — %1 个设备").arg(m_mappings.size()));
  }
}

void DeviceMonitorPanel::onImportClicked() {
  QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("导入配置或布局文件"), QString(),
                                   QStringLiteral("JSON Files (*.json)"));
  if (path.isEmpty())
    return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, QStringLiteral("读取失败"), QStringLiteral("无法打开选中的 JSON 文件！"));
    return;
  }
  QByteArray data = f.readAll();
  f.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull()) {
    QMessageBox::warning(this, QStringLiteral("格式错误"), QStringLiteral("文件不是有效的 JSON 格式！"));
    return;
  }

  if (doc.isObject()) {
    // 导入房间与设备 2D 布局配置
    QJsonObject root = doc.object();
    m_activeTemplate = root["activeTemplate"].toString();
    m_rooms.clear();
    for (const auto &rv : root["rooms"].toArray()) {
      QJsonObject ro = rv.toObject();
      RoomRegion r;
      r.id = ro["id"].toString();
      r.name = ro["name"].toString();
      r.shape = ro["shape"].toInt(0);
      r.geom = QRect(ro["x"].toInt(), ro["y"].toInt(), ro["w"].toInt(), ro["h"].toInt());
      r.targetView = ro["targetView"].toString(QStringLiteral("界面1"));
      m_rooms.append(r);
    }

    m_deviceRoomPos.clear();
    QJsonObject devPos = root["devicePositions"].toObject();
    for (auto it = devPos.begin(); it != devPos.end(); ++it) {
      QJsonObject dp = it.value().toObject();
      m_deviceRoomPos[it.key().toInt()] = QPoint(dp["x"].toInt(), dp["y"].toInt());
    }

    // 若包含嵌入的设备映射，一并载入
    if (root.contains("mappings") && root["mappings"].isArray()) {
      m_mappings.clear();
      for (const auto &v : root["mappings"].toArray()) {
        auto o = v.toObject();
        DeviceBitMapping m;
        m.deviceId = o["deviceId"].toInt(1);
        m.label = o["label"].toString(QStringLiteral("设备"));
        m.deviceType = o["deviceType"].toString(QStringLiteral("detector"));
        m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
        m.byteIndex = o["byteIndex"].toInt(0);
        m.bitIndex = o["bitIndex"].toInt(0);
        m.defaultVal = o["defaultVal"].toInt(0);
        m.targetView = o["targetView"].toString(QStringLiteral("界面1"));
        m.targetRoom = o["targetRoom"].toString();
        m_mappings.append(m);
      }
    }

    autoArrangeRoomDevices();
    saveRoomLayout();
    rebuildRoomCanvas();
    updateZoom();
    scheduleSubWindowUpdate();
    appendLog(QStringLiteral("✓ 成功导入 2D 布局配置文件 (%1 个房间区域)").arg(m_rooms.size()), false);
  } else if (doc.isArray()) {
    // 导入设备 CAN 位映射列表
    m_mappings.clear();
    for (const auto &v : doc.array()) {
      auto o = v.toObject();
      DeviceBitMapping m;
      m.deviceId = o["deviceId"].toInt(1);
      m.label = o["label"].toString(QStringLiteral("设备"));
      m.deviceType = o["deviceType"].toString(QStringLiteral("detector"));
      m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
      m.byteIndex = o["byteIndex"].toInt(0);
      m.bitIndex = o["bitIndex"].toInt(0);
      m.defaultVal = o["defaultVal"].toInt(0);
      m.targetView = o["targetView"].toString(QStringLiteral("界面1"));
      m.targetRoom = o["targetRoom"].toString();
      m_mappings.append(m);
    }
    autoArrangeRoomDevices();
    rebuildRoomCanvas();
    saveConfig();
    scheduleSubWindowUpdate();
    appendLog(QStringLiteral("✓ 成功导入设备位映射配置 (%1 个设备)").arg(m_mappings.size()), false);
  }
}

void DeviceMonitorPanel::onExportClicked() {
  QJsonObject root;
  root["activeTemplate"] = m_activeTemplate;

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
    ro["targetView"] = r.targetView;
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

  QJsonArray mappingsArr;
  for (const auto &m : m_mappings) {
    QJsonObject o;
    o["deviceId"] = m.deviceId;
    o["label"] = m.label;
    o["deviceType"] = m.deviceType;
    o["canId"] = static_cast<int>(m.canId);
    o["byteIndex"] = m.byteIndex;
    o["bitIndex"] = m.bitIndex;
    o["defaultVal"] = m.defaultVal;
    o["targetView"] = m.targetView;
    o["targetRoom"] = m.targetRoom;
    mappingsArr.append(o);
  }
  root["mappings"] = mappingsArr;

  QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出完整系统配置与布局"), QStringLiteral("full_device_layout_config.json"),
      QStringLiteral("JSON Files (*.json)"));
  if (path.isEmpty())
    return;
  QFile f(path);
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    appendLog(QStringLiteral("✓ 已成功导出完整配置与房间 2D 布局文件"), false);
  }
}

void DeviceMonitorPanel::onResetClicked() {
  for (const auto &m : m_mappings) {
    auto *w = m_deviceWidgets.value(m.deviceId, nullptr);
    if (w) {
      w->setDefaultVal(m.defaultVal);
      w->setStatus(m.defaultVal == 1);
    }
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
    o["targetView"] = m.targetView;
    o["targetRoom"] = m.targetRoom;
    arr.append(o);
  }
  QFile f(QStringLiteral("device_config.json"));
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
  }

  QSettings settings("Phudon", "DeviceMonitorPanel");
  settings.setValue("viewNames", m_viewNames);
}

void DeviceMonitorPanel::loadConfig() {
  QSettings settings("Phudon", "DeviceMonitorPanel");
  m_viewNames = settings
                    .value("viewNames", QStringList{QStringLiteral("界面1"),
                                                    QStringLiteral("界面2"),
                                                    QStringLiteral("界面3")})
                    .toStringList();
  if (m_viewNames.isEmpty()) {
    m_viewNames = QStringList{QStringLiteral("界面1"), QStringLiteral("界面2"),
                              QStringLiteral("界面3")};
  }
  updateTabBar();

  QFile f(QStringLiteral("device_config.json"));
  if (!f.open(QIODevice::ReadOnly))
    return;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();
  if (!doc.isArray())
    return;
  m_mappings.clear();
  for (const auto &v : doc.array()) {
    auto o = v.toObject();
    DeviceBitMapping m;
    m.deviceId = o["deviceId"].toInt(1);
    m.label = o["label"].toString(QStringLiteral("设备"));
    m.deviceType = o["deviceType"].toString(QStringLiteral("detector"));
    m.canId = static_cast<quint32>(o["canId"].toInt(0x100));
    m.byteIndex = o["byteIndex"].toInt(0);
    m.bitIndex = o["bitIndex"].toInt(0);
    m.defaultVal = o["defaultVal"].toInt(0);
    m.targetView = o["targetView"].toString(QStringLiteral("界面1"));
    m.targetRoom = o["targetRoom"].toString();
    m_mappings.append(m);
  }
  autoArrangeRoomDevices();
  rebuildRoomCanvas();
}

// ========== 日志 ==========

void DeviceMonitorPanel::appendLog(const QString &text, bool isAlarm) {
  const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_log->setUpdatesEnabled(false);
  if (isAlarm) {
    m_log->appendHtml(
        QStringLiteral("<span style='color:#EF4444;'>[%1] %2</span>")
            .arg(ts, text.toHtmlEscaped()));
  } else {
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(ts, text));
  }
  // 高效批量裁剪旧日志
  auto *doc = m_log->document();
  int extraBlocks = doc->blockCount() - 300;
  if (extraBlocks > 0) {
    QTextCursor c(doc);
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor,
                   extraBlocks);
    c.removeSelectedText();
  }
  m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
  m_log->setUpdatesEnabled(true);
}

void DeviceMonitorPanel::onDeviceDragged(int deviceId, const QPoint &newPos) {
  // 1. 计算未缩放的基础坐标与设备中心点
  QPoint basePos(newPos.x() / m_zoomLevel, newPos.y() / m_zoomLevel);
  QSize devSize(128, 155);
  auto *w = m_deviceWidgets.value(deviceId, nullptr);
  if (w) devSize = w->size();
  QPoint devCenter = basePos + QPoint(devSize.width() / 2, devSize.height() / 2);

  // 2. 判断设备中心点落入哪个房间
  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  RoomRegion *foundRoom = nullptr;
  for (auto &r : m_rooms) {
    QString rView = r.targetView.trimmed();
    if (rView.isEmpty()) rView = QStringLiteral("界面1");
    bool matchesView =
        (rView == activeViewName) ||
        (activeViewName == QStringLiteral("界面1") &&
         (rView == QStringLiteral("界面1") || rView.isEmpty()));
    if (matchesView && r.geom.contains(devCenter)) {
      foundRoom = &r;
      break;
    }
  }

  // 3. 更新设备的 targetRoom 与坐标关联
  bool roomChanged = false;
  for (auto &m : m_mappings) {
    if (m.deviceId == deviceId) {
      QString newRoomName = foundRoom ? foundRoom->name : QString();
      if (m.targetRoom.trimmed() != newRoomName.trimmed()) {
        m.targetRoom = newRoomName;
        roomChanged = true;
      }
      break;
    }
  }

  if (foundRoom) {
    m_deviceRoomPos[deviceId] = basePos;
  } else {
    // 若移出所有房间，清除属于房间的坐标，使其退回未摆放设备区域
    m_deviceRoomPos.remove(deviceId);
  }

  // 4. 更新房间设备计数
  for (auto *rw : m_roomWidgets) {
    int cnt = 0;
    for (auto it = m_deviceWidgets.begin(); it != m_deviceWidgets.end(); ++it) {
      QRect devGeom(it.value()->pos(), it.value()->size());
      if (rw->geometry().contains(devGeom.center()))
        ++cnt;
    }
    rw->setDeviceCount(cnt);
  }

  saveRoomLayout();
  saveConfig();

  if (roomChanged || !foundRoom) {
    rebuildRoomCanvas();
    updateZoom();
  }

  if (m_multiScreenActive) {
    scheduleSubWindowUpdate();
  }
}

void DeviceMonitorPanel::onRoomMoved(const QString &id, const QRect &newGeom) {
  for (auto &r : m_rooms) {
    if (r.id == id) {
      QRect oldGeom = r.geom;
      QRect unzoomedNewGeom(
          newGeom.x() / m_zoomLevel, newGeom.y() / m_zoomLevel,
          newGeom.width() / m_zoomLevel, newGeom.height() / m_zoomLevel);

      int dx = unzoomedNewGeom.x() - oldGeom.x();
      int dy = unzoomedNewGeom.y() - oldGeom.y();

      r.geom = unzoomedNewGeom;

      if (dx != 0 || dy != 0) {
        // 移动房间时，同步移动该房间内的所有设备图标
        for (const auto &m : m_mappings) {
          bool isRoomDevice = (m.targetRoom.trimmed() == r.name.trimmed());
          bool wasInsideOldGeom = m_deviceRoomPos.contains(m.deviceId) &&
                                  oldGeom.contains(m_deviceRoomPos[m.deviceId]);
          if (isRoomDevice || wasInsideOldGeom) {
            if (m_deviceRoomPos.contains(m.deviceId)) {
              m_deviceRoomPos[m.deviceId] += QPoint(dx, dy);
              auto *devW = m_deviceWidgets.value(m.deviceId, nullptr);
              if (devW) {
                devW->move(devW->x() + dx * m_zoomLevel,
                           devW->y() + dy * m_zoomLevel);
              }
            }
          }
        }
      }
      break;
    }
  }
  saveRoomLayout();
  if (m_multiScreenActive) {
    scheduleSubWindowUpdate();
  }
}

void DeviceMonitorPanel::onRoomResized(const QString &id,
                                       const QRect &newGeom) {
  for (auto &r : m_rooms) {
    if (r.id == id) {
      r.geom =
          QRect(newGeom.x() / m_zoomLevel, newGeom.y() / m_zoomLevel,
                newGeom.width() / m_zoomLevel, newGeom.height() / m_zoomLevel);
      break;
    }
  }
  saveRoomLayout();
  if (m_multiScreenActive) {
    scheduleSubWindowUpdate();
  }
}

void DeviceMonitorPanel::onRoomLockToggled(const QString &id, bool locked) {
  for (auto &r : m_rooms) {
    if (r.id == id) {
      r.isLocked = locked;
      appendLog(QStringLiteral("%1 房间【%2】已%3")
                    .arg(locked ? QStringLiteral("🔒") : QStringLiteral("🔓"))
                    .arg(r.name)
                    .arg(locked ? QStringLiteral("锁定固定") : QStringLiteral("解除锁定")),
                false);
      break;
    }
  }
  saveRoomLayout();
  if (m_multiScreenActive) {
    scheduleSubWindowUpdate();
  }
}



// ===== 缩放 =====

void DeviceMonitorPanel::zoomIn() {
  m_zoomLevel = qMin(2.0, m_zoomLevel + 0.25);
  updateZoom();
}
void DeviceMonitorPanel::zoomOut() {
  m_zoomLevel = qMax(0.25, m_zoomLevel - 0.25);
  updateZoom();
}

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
  for (auto *rw : m_roomWidgets) {
    if (!rw) continue;
    for (const auto &r : m_rooms) {
      if (r.id == rw->roomId()) {
        rw->setGeometry(
            r.geom.x() * m_zoomLevel, r.geom.y() * m_zoomLevel,
            r.geom.width() * m_zoomLevel, r.geom.height() * m_zoomLevel);
        break;
      }
    }
  }
  if (m_lblCount)
    m_lblCount->setText(QStringLiteral("缩放: %1% | %2 房间 | %3 设备")
                            .arg((int)(m_zoomLevel * 100))
                            .arg(m_rooms.size())
                            .arg(m_mappings.size()));
}

void DeviceMonitorPanel::repositionFloatingWidgets() {
  if (m_logWrapper && m_logWrapper->isVisible()) {
    int bHeight = (m_bottomBar && m_bottomBar->isVisible()) ? m_bottomBar->height() : 30;
    int posX = qMax(10, width() - m_logWrapper->width() - 20);
    int posY = qMax(10, height() - m_logWrapper->height() - bHeight - 12);
    m_logWrapper->move(posX, posY);
  }
}

// Ctrl+滚轮缩放
void DeviceMonitorPanel::wheelEvent(QWheelEvent *e) {
  if (e->modifiers() & Qt::ControlModifier) {
    if (e->angleDelta().y() > 0)
      zoomIn();
    else
      zoomOut();
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
  if (watched == m_layoutFloatingTitleBar && m_layoutFloatingDialog) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = static_cast<QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        m_layoutFloatingDragging = true;
        m_layoutFloatingDragStartPos = me->globalPos() - m_layoutFloatingDialog->pos();
        m_layoutFloatingDialog->raise();
        return true;
      }
    } else if (event->type() == QEvent::MouseMove) {
      QMouseEvent *me = static_cast<QMouseEvent *>(event);
      if (m_layoutFloatingDragging && (me->buttons() & Qt::LeftButton)) {
        m_layoutFloatingDialog->move(me->globalPos() - m_layoutFloatingDragStartPos);
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonRelease) {
      if (m_layoutFloatingDragging) {
        m_layoutFloatingDragging = false;
        return true;
      }
    }
  }
  if (watched == m_logTitleBar && m_logWrapper) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = static_cast<QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        m_logDragging = true;
        m_logDragStartPos = me->globalPos() - m_logWrapper->pos();
        m_logWrapper->raise();
        return true;
      }
    } else if (event->type() == QEvent::MouseMove) {
      QMouseEvent *me = static_cast<QMouseEvent *>(event);
      if (m_logDragging && (me->buttons() & Qt::LeftButton)) {
        QPoint newPos = me->globalPos() - m_logDragStartPos;
        int bHeight = m_bottomBar ? m_bottomBar->height() : 30;
        int tHeight = m_toolbar ? m_toolbar->height() : 42;
        int maxX = qMax(0, width() - m_logWrapper->width());
        int maxY = qMax(tHeight, height() - m_logWrapper->height() - bHeight);
        newPos.setX(qBound(0, newPos.x(), maxX));
        newPos.setY(qBound(tHeight, newPos.y(), maxY));
        m_logWrapper->move(newPos);
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonRelease) {
      if (m_logDragging) {
        m_logDragging = false;
        return true;
      }
    }
  }
  if (watched == m_logWrapper) {
    if (event->type() == QEvent::Hide || event->type() == QEvent::Show) {
      bool isVisible = (event->type() == QEvent::Show);
      if (m_btnInfoLog && m_btnInfoLog->isChecked() != isVisible) {
        m_btnInfoLog->blockSignals(true);
        m_btnInfoLog->setChecked(isVisible);
        m_btnInfoLog->blockSignals(false);
      }
      QTimer::singleShot(20, this, [this]() {
        updateZoom();
      });
    }
  }
  if (watched == m_gridContainer) {
    if (event->type() == QEvent::Paint) {
      if (!m_activeTemplate.isEmpty()) {
        QPainter p(m_gridContainer);
        p.setRenderHint(QPainter::Antialiasing);
        drawTemplateBackground(p, m_gridContainer->rect(), m_activeTemplate);
      }
    } else if (event->type() == QEvent::DragEnter) {
      QDragEnterEvent *dee = static_cast<QDragEnterEvent *>(event);
      if (dee->mimeData()->hasText() &&
          dee->mimeData()->text().startsWith(QStringLiteral("device:"))) {
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

void DeviceMonitorPanel::drawTemplateBackground(QPainter &p, const QRect &rect,
                                                const QString &tpl) {
  if (tpl.isEmpty()) return;
  p.save();

  int w = rect.width();
  int h = rect.height();

  // 暗色高科技深邃背景
  p.fillRect(rect, QColor(0x0A, 0x0F, 0x1D));

  // 精细 HUD 居中网格线
  QPen gridPen(QColor(0x1E, 0x2A, 0x3E, 60), 1, Qt::DotLine);
  p.setPen(gridPen);
  for (int x = 0; x < w; x += 50) {
    p.drawLine(x, 0, x, h);
  }
  for (int y = 0; y < h; y += 50) {
    p.drawLine(0, y, w, y);
  }

  QColor neonCyan(0x00, 0xD4, 0xFF, 180);
  QColor neonAmber(0xF5, 0x9E, 0x0B, 180);
  QColor neonGreen(0x10, 0xB9, 0x81, 180);

  if (tpl == QStringLiteral("submarine")) {
    // 🚢 核潜艇全屏流线型外廓与舱室剖面线
    int cy = h / 2;
    int bowX = 80;
    int sternX = w - 120;
    int hullH = qMin(380, h - 200);

    QPainterPath subOutline;
    subOutline.moveTo(bowX + 150, cy - hullH / 2);
    // 艇艏圆弧水滴
    subOutline.cubicTo(bowX + 20, cy - hullH / 2, bowX, cy, bowX + 20, cy + hullH / 2);
    subOutline.lineTo(bowX + 150, cy + hullH / 2);
    // 艇身
    subOutline.lineTo(sternX - 100, cy + hullH / 2 - 20);
    // 艇艉渐窄
    subOutline.lineTo(sternX, cy + 30);
    subOutline.lineTo(sternX + 40, cy); // 螺旋桨轴
    subOutline.lineTo(sternX, cy - 30);
    subOutline.lineTo(sternX - 100, cy - hullH / 2 + 20);
    subOutline.closeSubpath();

    // 艇体阴影与亮青发光线条
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 15));
    p.setPen(QPen(neonCyan, 2.5));
    p.drawPath(subOutline);

    // 指挥塔/水翼 (Sail)
    QPainterPath sail;
    sail.moveTo(bowX + 450, cy - hullH / 2);
    sail.lineTo(bowX + 470, cy - hullH / 2 - 70);
    sail.lineTo(bowX + 680, cy - hullH / 2 - 70);
    sail.lineTo(bowX + 720, cy - hullH / 2);
    sail.closeSubpath();
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 25));
    p.drawPath(sail);

    // 潜望镜与雷达桅杆
    p.setPen(QPen(neonCyan, 1.8));
    p.drawLine(bowX + 520, cy - hullH / 2 - 70, bowX + 520, cy - hullH / 2 - 105);
    p.drawLine(bowX + 560, cy - hullH / 2 - 70, bowX + 560, cy - hullH / 2 - 120);

    // 尾部 7 叶大侧斜螺旋桨轮廓
    p.setPen(QPen(neonAmber, 2.0));
    p.drawEllipse(QPoint(sternX + 45, cy), 15, 45);

    // 舱壁分隔虚线与标识
    p.setPen(QPen(QColor(0x64, 0x74, 0x8B, 160), 1.5, Qt::DashLine));
    p.drawLine(bowX + 330, cy - hullH / 2 + 10, bowX + 330, cy + hullH / 2 - 10);
    p.drawLine(bowX + 650, cy - hullH / 2 + 10, bowX + 650, cy + hullH / 2 - 10);
    p.drawLine(bowX + 920, cy - hullH / 2 + 10, bowX + 920, cy + hullH / 2 - 10);
    p.drawLine(bowX + 1180, cy - hullH / 2 + 10, bowX + 1180, cy + hullH / 2 - 10);

    // 水波雷达扫描圈
    p.setPen(QPen(QColor(0x00, 0xD4, 0xFF, 60), 1, Qt::DotLine));
    p.drawEllipse(QPoint(bowX + 30, cy), 80, 80);
    p.drawEllipse(QPoint(bowX + 30, cy), 140, 140);
  }
  else if (tpl == QStringLiteral("building")) {
    // 🏢 写字楼三层立体结构与建筑蓝图轮廓
    int marginX = 80;
    int bldW = w - marginX * 2;
    int floorH = qMin(220, (h - 150) / 3);

    for (int i = 0; i < 3; ++i) {
      int fy = h - 100 - (i + 1) * (floorH + 15);
      QRect fRect(marginX, fy, bldW, floorH);

      QPainterPath floorPath;
      floorPath.addRoundedRect(fRect, 8, 8);
      p.setBrush(QColor(0x10, 0xB9, 0x81, 12));
      p.setPen(QPen(neonGreen, 2.0));
      p.drawPath(floorPath);

      // 电梯井与管道核心筒
      p.setPen(QPen(QColor(0x00, 0xD4, 0xFF, 120), 1.5, Qt::DashLine));
      p.drawRect(marginX + 20, fy + 10, 60, floorH - 20);

      // 玻璃幕墙网格
      p.setPen(QPen(QColor(0x10, 0xB9, 0x81, 40), 1, Qt::DotLine));
      for (int fx = marginX + 120; fx < marginX + bldW; fx += 100) {
        p.drawLine(fx, fy, fx, fy + floorH);
      }
    }
  }
  else if (tpl == QStringLiteral("warship")) {
    // 🛥️ 水面隐身驱逐舰/巡洋舰外廓轮廓
    int bowX = 60;
    int sternX = w - 80;
    int cy = h / 2 + 30;
    int hullH = qMin(320, h - 220);

    QPainterPath shipPath;
    shipPath.moveTo(bowX, cy); // 舰艏水线尖角
    shipPath.lineTo(bowX + 220, cy - hullH / 2);
    shipPath.lineTo(sternX, cy - hullH / 2 + 10);
    shipPath.lineTo(sternX + 20, cy + hullH / 2 - 10);
    shipPath.lineTo(bowX + 180, cy + hullH / 2);
    shipPath.closeSubpath();

    p.setBrush(QColor(0x00, 0xD4, 0xFF, 15));
    p.setPen(QPen(neonCyan, 2.5));
    p.drawPath(shipPath);

    // 隐身舰桥与双烟囱结构
    QPainterPath bridge;
    bridge.moveTo(bowX + 320, cy - hullH / 2);
    bridge.lineTo(bowX + 350, cy - hullH / 2 - 80);
    bridge.lineTo(bowX + 580, cy - hullH / 2 - 80);
    bridge.lineTo(bowX + 620, cy - hullH / 2);
    bridge.closeSubpath();
    p.setBrush(QColor(0x00, 0xD4, 0xFF, 25));
    p.drawPath(bridge);

    // 舰艏 130mm 主炮轮廓
    p.setPen(QPen(neonAmber, 2.0));
    p.drawEllipse(QPoint(bowX + 160, cy - 10), 18, 18);
    p.drawLine(bowX + 160, cy - 10, bowX + 110, cy - 25);

    // 舰艉直升机甲板 H 标志
    p.setPen(QPen(neonGreen, 2.0));
    int heloX = sternX - 140;
    p.drawRect(heloX - 40, cy - 40, 80, 80);
    p.drawLine(heloX - 20, cy - 25, heloX - 20, cy + 25);
    p.drawLine(heloX + 20, cy - 25, heloX + 20, cy + 25);
    p.drawLine(heloX - 20, cy, heloX + 20, cy);
  }
  else if (tpl == QStringLiteral("carrier")) {
    // 🛫 航母全景飞行甲板与斜角降落跑道外廓
    int marginX = 50;
    int marginY = 50;
    int deckW = w - marginX * 2;
    int deckH = h - marginY * 2;

    QPainterPath carrierDeck;
    carrierDeck.moveTo(marginX + 200, marginY);
    carrierDeck.lineTo(marginX + deckW - 100, marginY);
    carrierDeck.lineTo(marginX + deckW, marginY + 80);
    carrierDeck.lineTo(marginX + deckW, marginY + deckH - 60);
    carrierDeck.lineTo(marginX + deckW - 150, marginY + deckH);
    carrierDeck.lineTo(marginX + 80, marginY + deckH);
    carrierDeck.lineTo(marginX, marginY + deckH - 120);
    carrierDeck.closeSubpath();

    p.setBrush(QColor(0x00, 0xD4, 0xFF, 12));
    p.setPen(QPen(neonCyan, 2.5));
    p.drawPath(carrierDeck);

    // 斜角降落甲板跑道白线与阻拦索
    p.setPen(QPen(neonAmber, 2.0, Qt::DashLine));
    p.drawLine(marginX + deckW - 180, marginY + 30, marginX + 100, marginY + deckH - 30);

    // 右舷舰岛 (Island) 轮廓
    QRect island(marginX + deckW - 320, marginY + 70, 160, 220);
    p.setBrush(QColor(0xF5, 0x9E, 0x0B, 30));
    p.setPen(QPen(neonAmber, 2.0));
    p.drawRoundedRect(island, 8, 8);
  }

  p.restore();
}

void DeviceMonitorPanel::autoArrangeRoomsAndDevices() {
  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  QList<int> roomIndices;
  for (int i = 0; i < m_rooms.size(); ++i) {
    auto &r = m_rooms[i];
    QString rView = r.targetView.trimmed();
    if (rView.isEmpty()) rView = QStringLiteral("界面1");
    bool matchesView =
        r.visible &&
        ((rView == activeViewName) ||
         (activeViewName == QStringLiteral("界面1") &&
          (rView == QStringLiteral("界面1") || rView.isEmpty())));
    if (matchesView) {
      roomIndices.append(i);
    }
  }

  if (roomIndices.isEmpty()) {
    appendLog(QStringLiteral("ℹ️ 当前界面没有可见房间可供规整排列"), false);
    return;
  }

  int cols = 2;
  int roomWidth = 420;
  int baseRoomHeight = 280;
  int startX = 50, startY = 50;
  int gapX = 50, gapY = 50;

  for (int idx = 0; idx < roomIndices.size(); ++idx) {
    int rIdx = roomIndices[idx];
    auto &r = m_rooms[rIdx];
    int row = idx / cols;
    int col = idx % cols;
    r.geom = QRect(startX + col * (roomWidth + gapX),
                   startY + row * (baseRoomHeight + gapY),
                   roomWidth, baseRoomHeight);
  }

  // 内部设备对齐与房间高度拓展
  autoArrangeRoomDevices();

  rebuildRoomCanvas();
  updateZoom();
  updateUnplacedDock();
  saveRoomLayout();
  saveConfig();
  appendLog(QStringLiteral("🧹 已为您一键规整排列当前界面的 %1 个房间及对应设备")
                .arg(roomIndices.size()),
            false);
}

void DeviceMonitorPanel::applyLayoutTemplate(const QString &tpl) {
  m_activeTemplate = tpl;
  m_rooms.clear();
  for (auto *rw : m_roomWidgets) {
    if (rw) {
      rw->hide();
      delete rw;
    }
  }
  m_roomWidgets.clear();

  int vpW = m_scrollArea ? m_scrollArea->viewport()->width() : 1600;
  int vpH = m_scrollArea ? m_scrollArea->viewport()->height() : 900;
  int canvasW = qMax(1600, vpW);
  int canvasH = qMax(900, vpH);
  m_gridContainer->setFixedSize(canvasW, canvasH);
  m_baseCanvasW = canvasW;
  m_baseCanvasH = canvasH;

  if (tpl.isEmpty()) {
    m_deviceRoomPos.clear();
    rebuildRoomCanvas();
    saveRoomLayout();
    zoomFit();
    updateZoom();
    scheduleSubWindowUpdate();
    appendLog(QStringLiteral("✓ 已清空模板背景，恢复默认无模板画布局"), false);
    return;
  }
  m_deviceRoomPos.clear();

  // 核潜艇布局
  if (tpl == QStringLiteral("submarine")) {
    m_rooms.append({"sub_1", QStringLiteral("舱首/鱼雷舱"), QRect(180, 320, 240, 260)});
    m_rooms.append({"sub_2", QStringLiteral("指挥与战术中心"), QRect(480, 240, 280, 340)});
    m_rooms.append({"sub_3", QStringLiteral("生活休息舱"), QRect(800, 320, 240, 260)});
    m_rooms.append({"sub_4", QStringLiteral("反应堆舱区"), QRect(1080, 290, 250, 300)});
    m_rooms.append({"sub_5", QStringLiteral("动力/推进舱"), QRect(1370, 320, 240, 260)});
  }
  // 写字楼布局
  else if (tpl == QStringLiteral("building")) {
    m_rooms.append({"bld_3", QStringLiteral("3F - 云数据机房"), QRect(120, 110, canvasW - 240, 180)});
    m_rooms.append({"bld_2", QStringLiteral("2F - 行政与会议中心"), QRect(120, 320, canvasW - 240, 180)});
    m_rooms.append({"bld_1", QStringLiteral("1F - 研发测试中心"), QRect(120, 530, canvasW - 240, 180)});
  }
  // 水面舰船布局
  else if (tpl == QStringLiteral("warship")) {
    m_rooms.append({"ship_1", QStringLiteral("舰艏武器库区"), QRect(100, 240, 260, 240)});
    m_rooms.append({"ship_2", QStringLiteral("舰桥驾驶控制舱"), QRect(400, 140, 300, 340)});
    m_rooms.append({"ship_3", QStringLiteral("舰舯机电舱室"), QRect(740, 240, 280, 240)});
    m_rooms.append({"ship_4", QStringLiteral("舰艉直升机库"), QRect(1060, 220, 280, 260)});
  }
  // 航母布局
  else if (tpl == QStringLiteral("carrier")) {
    m_rooms.append({"car_1", QStringLiteral("飞行甲板/舰载机区"), QRect(80, 80, 680, 260)});
    m_rooms.append({"car_2", QStringLiteral("舰岛/指挥塔"), QRect(800, 80, 280, 240)});
    m_rooms.append({"car_3", QStringLiteral("机库/维修区"), QRect(80, 380, 680, 260)});
    m_rooms.append({"car_4", QStringLiteral("动力/推进舱"), QRect(800, 360, 280, 280)});
    m_rooms.append({"car_5", QStringLiteral("武器/防御区"), QRect(1110, 80, 260, 560)});
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

  autoArrangeRoomDevices();
  rebuildRoomCanvas();
  saveRoomLayout();
  zoomFit();
  updateZoom();
  appendLog(QStringLiteral("✓ 已成功应用场景矢量渲染模板: %1 (画布已全屏最大化)").arg(tpl), false);
}

void DeviceMonitorPanel::onAddRoom() {
  // 形状选择
  QStringList shapes = {QStringLiteral("矩形"), QStringLiteral("圆形"),
                        QStringLiteral("菱形")};
  bool ok;
  QString shapeStr =
      QInputDialog::getItem(this, QStringLiteral("新建房间 — 选择形状"),
                            QStringLiteral("房间形状:"), shapes, 0, false, &ok);
  if (!ok)
    return;

  int shape = shapes.indexOf(shapeStr);
  QString name = QInputDialog::getText(
      this, QStringLiteral("新建房间"), QStringLiteral("房间名称:"),
      QLineEdit::Normal, QStringLiteral("房间 %1").arg(m_rooms.size() + 1),
      &ok);
  if (!ok || name.trimmed().isEmpty())
    return;

  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  RoomRegion r;
  r.id = QStringLiteral("room_%1").arg(QDateTime::currentMSecsSinceEpoch());
  r.name = name.trimmed();
  r.targetView = activeViewName;
  r.shape = shape;
  r.geom = QRect(50 + m_rooms.size() * 40, 50 + m_rooms.size() * 40, 320, 240);
  m_rooms.append(r);

  rebuildRoomCanvas();
  updateZoom();

  appendLog(QStringLiteral("✓ 已创建房间: %1").arg(r.name), false);
  saveRoomLayout();

  if (m_lblCount) {
    m_lblCount->setText(QStringLiteral("分区模式 | %1 个房间 | %2 个设备")
                            .arg(m_rooms.size())
                            .arg(m_mappings.size()));
  }
}

void DeviceMonitorPanel::onManageRoomsRequested() {
  RoomManagerDialog dlg(m_rooms, m_viewNames, this);
  if (dlg.exec() == QDialog::Accepted) {
    m_rooms = dlg.rooms();

    // 根据更新后的房间结构重新排布与整齐网格对齐
    autoArrangeRoomDevices();

    saveRoomLayout();
    saveConfig();

    rebuildRoomCanvas();
    updateZoom();
    updateUnplacedDock();

    appendLog(QStringLiteral("✓ 已完成房间管理全量配置与同步处理"), false);
  }
}

// ===== Room 布局持久化 =====
void DeviceMonitorPanel::saveRoomLayout() {
  QJsonObject root;
  root["activeTemplate"] = m_activeTemplate;
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
    ro["targetView"] = r.targetView;
    ro["visible"] = r.visible;
    ro["isLocked"] = r.isLocked;
    ro["color"] = r.color.name();
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
  s.setValue("monitor/roomLayout", QString::fromUtf8(QJsonDocument(root).toJson(
                                       QJsonDocument::Compact)));
}

void DeviceMonitorPanel::loadRoomLayout() {
  QSettings s("PhudonTools", "PhudonTools");
  QString json = s.value("monitor/roomLayout").toString();
  if (json.isEmpty())
    return;

  QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
  if (!doc.isObject())
    return;

  QJsonObject root = doc.object();
  m_activeTemplate = root["activeTemplate"].toString();
  m_rooms.clear();
  for (const auto &rv : root["rooms"].toArray()) {
    QJsonObject ro = rv.toObject();
    RoomRegion r;
    r.id = ro["id"].toString();
    r.name = ro["name"].toString();
    r.shape = ro["shape"].toInt(0);
    r.geom = QRect(ro["x"].toInt(), ro["y"].toInt(), ro["w"].toInt(),
                   ro["h"].toInt());
    r.targetView = ro["targetView"].toString(QStringLiteral("界面1"));
    r.visible = ro.contains("visible") ? ro["visible"].toBool(true) : true;
    r.isLocked = ro["isLocked"].toBool(false);
    if (ro.contains("color")) {
      r.color = QColor(ro["color"].toString("#00D4FF"));
    } else {
      r.color = QColor(0, 212, 255);
    }
    m_rooms.append(r);
  }

  m_deviceRoomPos.clear();
  QJsonObject devPos = root["devicePositions"].toObject();
  for (auto it = devPos.begin(); it != devPos.end(); ++it) {
    QJsonObject dp = it.value().toObject();
    m_deviceRoomPos[it.key().toInt()] =
        QPoint(dp["x"].toInt(), dp["y"].toInt());
  }

  // 根据设备的 targetRoom 自动建房与对齐
  autoArrangeRoomDevices();
}

void DeviceMonitorPanel::autoArrangeRoomDevices() {
  for (const auto &m : m_mappings) {
    QString tRoom = m.targetRoom.trimmed();
    if (tRoom.isEmpty()) {
      continue;
    }
    QString tView = m.targetView.trimmed();
    if (tView.isEmpty()) tView = QStringLiteral("界面1");

    // 1. 查找是否存在对应的房间
    RoomRegion *targetRoomPtr = nullptr;
    for (auto &r : m_rooms) {
      if (r.name == tRoom && (r.targetView == tView || r.targetView.isEmpty())) {
        targetRoomPtr = &r;
        break;
      }
    }

    // 2. 若不存在，自动创建该房间
    if (!targetRoomPtr) {
      int countInView = 0;
      for (const auto &r : m_rooms) {
        if (r.targetView == tView) countInView++;
      }
      RoomRegion newRoom;
      newRoom.id = QStringLiteral("room_%1").arg(QDateTime::currentMSecsSinceEpoch() + countInView);
      newRoom.name = tRoom;
      newRoom.targetView = tView;
      newRoom.shape = 0; // 矩形
      int rx = 50 + (countInView % 3) * 380;
      int ry = 50 + (countInView / 3) * 280;
      newRoom.geom = QRect(rx, ry, 350, 250);
      m_rooms.append(newRoom);
      targetRoomPtr = &m_rooms.last();
    }

    // 3. 收集指定到该房间的所有设备列表，并在房间内整齐网格排列
    QList<int> roomDeviceIds;
    for (const auto &item : m_mappings) {
      QString itemRoom = item.targetRoom.trimmed();
      QString itemView = item.targetView.trimmed();
      if (itemView.isEmpty()) itemView = QStringLiteral("界面1");
      if (itemRoom == tRoom && itemView == tView) {
        roomDeviceIds.append(item.deviceId);
      }
    }

    int cols = 2;
    int rows = (roomDeviceIds.size() + cols - 1) / cols;
    int minHeight = 45 + rows * 160 + 20;
    if (targetRoomPtr->geom.height() < minHeight) {
      targetRoomPtr->geom.setHeight(minHeight);
    }

    for (int idx = 0; idx < roomDeviceIds.size(); ++idx) {
      int dId = roomDeviceIds[idx];
      int r = idx / cols;
      int c = idx % cols;
      int posX = targetRoomPtr->geom.x() + 20 + c * 140;
      int posY = targetRoomPtr->geom.y() + 45 + r * 160;
      m_deviceRoomPos[dId] = QPoint(posX, posY);
    }
  }
}

void DeviceMonitorPanel::rebuildRoomCanvas() {
  if (m_isRebuildingCanvas) return;
  m_isRebuildingCanvas = true;
  auto reentrancyGuard = qScopeGuard([this]() { m_isRebuildingCanvas = false; });

  // 最优先安全清理未放置区布局项，断开旧 QLayoutItem 指针
  clearUnplacedDock();

  // 获得当前活动的界面名称
  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  // 清理 grid layout
  while (m_gridLayout->count() > 0) {
    QLayoutItem *item = m_gridLayout->takeAt(0);
    delete item;
  }

  // 隐藏并完全重置所有已有设备控件的 parent 指针，防止停靠区残留悬挂
  for (auto *w : m_deviceWidgets) {
    if (w) {
      w->setVisible(false);
      w->setParent(m_gridContainer);
    }
  }

  // 确保所有设备 widget 已创建
  for (int i = 0; i < m_mappings.size(); ++i) {
    const auto &m = m_mappings[i];
    if (!m_deviceWidgets.contains(m.deviceId)) {
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

      auto *w = new DeviceStatusWidget(m.deviceId, kind, m.label, m.canId, m_gridContainer);
      w->setStatus(m.defaultVal);
      w->setDraggable(true);
      connect(w, &DeviceStatusWidget::deviceDragged, this,
              &DeviceMonitorPanel::onDeviceDragged);
      connect(w, &DeviceStatusWidget::dragStartedFromDock, this,
              [this](int deviceId, const QPoint &) {
                placeDeviceOnCanvas(deviceId);
              });
      connect(w, &DeviceStatusWidget::editRequested, this,
              &DeviceMonitorPanel::onEditDeviceRequested);
      m_deviceWidgets[m.deviceId] = w;
    }
  }

  // 画布样式
  m_gridContainer->setStyleSheet("background-color: #0D1117;");

  // 安全清理已在 m_rooms 中被彻底移除的房间控件
  QSet<QString> validRoomIds;
  for (const auto &r : m_rooms) {
    validRoomIds.insert(r.id);
  }
  for (auto it = m_roomWidgets.begin(); it != m_roomWidgets.end(); ) {
    if (!validRoomIds.contains(it.key())) {
      if (it.value()) {
        it.value()->hide();
        delete it.value();
      }
      it = m_roomWidgets.erase(it);
    } else {
      ++it;
    }
  }

  // 管理并复用 RoomWidget (界面切换仅 show/hide，绝不销毁控件)
  for (const auto &r : m_rooms) {
    QString rView = r.targetView.trimmed();
    if (rView.isEmpty()) rView = QStringLiteral("界面1");
    bool matchesView =
        r.visible &&
        ((rView == activeViewName) ||
         (activeViewName == QStringLiteral("界面1") &&
          (rView == QStringLiteral("界面1") || rView.isEmpty())));

    RoomWidget *rw = m_roomWidgets.value(r.id, nullptr);
    if (matchesView) {
      if (!rw) {
        rw = new RoomWidget(r.id, r.name, r.geom, r.shape, m_gridContainer);
        connect(rw, &RoomWidget::roomMoved, this, &DeviceMonitorPanel::onRoomMoved);
        connect(rw, &RoomWidget::roomResized, this,
                &DeviceMonitorPanel::onRoomResized);
        connect(rw, &RoomWidget::roomLockToggled, this,
                &DeviceMonitorPanel::onRoomLockToggled);
        connect(
            rw, &RoomWidget::roomRenameRequested, this, [this](const QString &rid) {
              RoomWidget *w = m_roomWidgets.value(rid, nullptr);
              if (w) {
                bool ok;
                QString name = QInputDialog::getText(
                    this, QStringLiteral("重命名房间"), QStringLiteral("新名称:"),
                    QLineEdit::Normal, w->roomName(), &ok);
                if (ok && !name.trimmed().isEmpty()) {
                  QString oldName = w->roomName().trimmed();
                  QString newName = name.trimmed();
                  w->setRoomName(newName);
                  for (auto &rItem : m_rooms) {
                    if (rItem.id == rid) {
                      rItem.name = newName;
                      break;
                    }
                  }
                  // 同步更新所有原属该房间设备的 targetRoom 属性
                  for (auto &m : m_mappings) {
                    if (m.targetRoom.trimmed() == oldName) {
                      m.targetRoom = newName;
                    }
                  }
                  saveRoomLayout();
                  saveConfig();
                  appendLog(
                      QStringLiteral("✓ 房间已重命名为: %1").arg(newName),
                      false);
                }
              }
            });
        connect(rw, &RoomWidget::roomDeleteRequested, this,
                [this](const QString &rid) {
                  auto btn =
                      QMessageBox::question(this, QStringLiteral("删除房间"),
                                            QStringLiteral("确定要删除此房间吗？"));
                  if (btn == QMessageBox::Yes) {
                    QString deletedRoomName;
                    for (int j = 0; j < m_rooms.size(); ++j) {
                      if (m_rooms[j].id == rid) {
                        deletedRoomName = m_rooms[j].name.trimmed();
                        appendLog(QStringLiteral("✕ 已删除房间: %1").arg(m_rooms[j].name), true);
                        m_rooms.removeAt(j);
                        break;
                      }
                    }
                    if (!deletedRoomName.isEmpty()) {
                      // 清除该房间内所有设备的 targetRoom 及其坐标，退回未摆放设备区域
                      for (auto &m : m_mappings) {
                        if (m.targetRoom.trimmed() == deletedRoomName) {
                          m.targetRoom.clear();
                          m_deviceRoomPos.remove(m.deviceId);
                        }
                      }
                    }
                    if (m_roomWidgets.contains(rid)) {
                      auto *oldRw = m_roomWidgets.take(rid);
                      if (oldRw) {
                        oldRw->hide();
                        delete oldRw;
                      }
                    }
                    saveRoomLayout();
                    saveConfig();
                    rebuildRoomCanvas();
                    updateZoom();
                    updateUnplacedDock();
                  }
                });
        m_roomWidgets.insert(r.id, rw);
      } else {
        rw->setRoomName(r.name);
        rw->setGeometry(
            r.geom.x() * m_zoomLevel, r.geom.y() * m_zoomLevel,
            r.geom.width() * m_zoomLevel, r.geom.height() * m_zoomLevel);
      }
      rw->setRoomColor(r.color);
      rw->setLocked(r.isLocked);
      rw->setRoomShape(r.shape);
      rw->setEditingEnabled(m_layoutEditingEnabled);
      rw->setVisible(true);
      rw->raise();
    } else {
      if (rw) {
        rw->setVisible(false);
      }
    }
  }

  // 自适应画布基准尺寸（不含缩放，以最远设备/房间边界为准）
  int maxX = 800, maxY = 600;
  for (const auto &r : m_rooms) {
    maxX = qMax(maxX, r.geom.right() + 50);
    maxY = qMax(maxY, r.geom.bottom() + 50);
  }
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

  // 模板背景轮廓单例复用
  if (!m_activeTemplate.isEmpty()) {
    if (!m_templateBg) {
      m_templateBg = new TemplateBackground(m_gridContainer);
    }
    m_templateBg->tpl = m_activeTemplate;
    m_templateBg->setGeometry(0, 0, maxX, maxY);
    m_templateBg->lower();
    m_templateBg->show();
  } else {
    if (m_templateBg) {
      m_templateBg->hide();
    }
  }

  // ===== 分流属于 activeViewName 的设备：已摆放的放画布，未摆放的放停靠区
  for (const auto &m : m_mappings) {
    QString targetView = m.targetView.trimmed();
    if (targetView.isEmpty())
      targetView = QStringLiteral("界面1");
    bool matchesView =
        (targetView == activeViewName) ||
        (activeViewName == QStringLiteral("界面1") &&
         (targetView == QStringLiteral("界面1") || targetView.isEmpty()));
    if (!matchesView)
      continue;

    int devId = m.deviceId;
    auto *w = m_deviceWidgets.value(devId, nullptr);
    if (!w)
      continue;

    if (m_deviceRoomPos.contains(devId)) {
      // 已摆放 → 放到画布上
      w->setParent(m_gridContainer);
      w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      QPoint pos = m_deviceRoomPos.value(devId);
      w->setGeometry(pos.x() * m_zoomLevel, pos.y() * m_zoomLevel,
                     128 * m_zoomLevel, 155 * m_zoomLevel);
      w->setDraggable(m_layoutEditingEnabled);
      w->setVisible(true);
      w->raise();
    }
  }

  // 更新未摆放停靠区
  updateUnplacedDock();

  // 更新房间设备计数（仅计算已摆放的设备）
  for (auto *rw : m_roomWidgets) {
    int cnt = 0;
    for (const auto &m : m_mappings) {
      int devId = m.deviceId;
      if (!m_deviceRoomPos.contains(devId))
        continue;
      auto *w = m_deviceWidgets.value(devId, nullptr);
      if (w && w->isVisible()) {
        QRect devGeom(w->pos(), w->size());
        if (rw->geometry().contains(devGeom.center()))
          ++cnt;
      }
    }
    rw->setDeviceCount(cnt);
  }

  if (m_lblCount) {
    int placed = 0;
    int totalInView = 0;
    for (const auto &mapping : m_mappings) {
      QString targetView = mapping.targetView.trimmed();
      if (targetView.isEmpty())
        targetView = QStringLiteral("界面1");
      bool matchesView =
          (targetView == activeViewName) ||
          (activeViewName == QStringLiteral("界面1") &&
           (targetView == QStringLiteral("界面1") || targetView.isEmpty()));
      if (matchesView) {
        totalInView++;
        if (m_deviceRoomPos.contains(mapping.deviceId))
          ++placed;
      }
    }
    m_lblCount->setText(
        QStringLiteral("[%1] 视图 | %2 个房间 | %3/%4 设备已摆放 %5")
            .arg(activeViewName)
            .arg(m_rooms.size())
            .arg(placed)
            .arg(totalInView)
            .arg(m_layoutEditingEnabled ? QStringLiteral("(布局使能中)") : QString()));
  }

  // 同步多屏联动子窗口
  scheduleSubWindowUpdate();
}

// ===== 未摆放设备停靠区管理 (安全清理与更新) =====

void DeviceMonitorPanel::clearUnplacedDock() {
  if (!m_unplacedLayout)
    return;

  while (m_unplacedLayout->count() > 0) {
    QLayoutItem *item = m_unplacedLayout->takeAt(0);
    if (item) {
      if (item->widget()) {
        item->widget()->setParent(m_gridContainer);
        item->widget()->setVisible(false);
      }
      delete item;
    }
  }
}

void DeviceMonitorPanel::updateUnplacedDock() {
  if (!m_unplacedContainer || !m_unplacedLayout)
    return;

  clearUnplacedDock();

  // 视图模式下隐藏未摆放停靠区（恢复原始简洁界面）
  if (!m_layoutEditingEnabled) {
    if (m_unplacedDock) m_unplacedDock->setVisible(false);
    return;
  }

  // 获得当前活动的界面名称
  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  // 仅收集属于当前活动界面的未摆放设备（没有在 m_deviceRoomPos 中的）
  QList<int> unplacedIds;
  for (const auto &mapping : m_mappings) {
    QString targetView = mapping.targetView.trimmed();
    if (targetView.isEmpty())
      targetView = QStringLiteral("界面1");
    bool matchesView =
        (targetView == activeViewName) ||
        (activeViewName == QStringLiteral("界面1") &&
         (targetView == QStringLiteral("界面1") || targetView.isEmpty()));
    if (!matchesView)
      continue;

    if (!m_deviceRoomPos.contains(mapping.deviceId)) {
      unplacedIds.append(mapping.deviceId);
    }
  }

  // 关键！若当前界面所有设备已放置，则隐蔽未放置停靠区域
  if (unplacedIds.isEmpty()) {
    if (m_unplacedDock) m_unplacedDock->setVisible(false);
    return;
  }

  if (m_unplacedDock) m_unplacedDock->setVisible(true);

  for (int devId : unplacedIds) {
    auto *w = m_deviceWidgets.value(devId, nullptr);
    if (!w) continue;

    w->setParent(m_unplacedContainer);
    w->setFixedSize(128, 155);
    w->setDraggable(false);
    w->setCursor(Qt::PointingHandCursor);
    w->show();

    m_unplacedLayout->addWidget(w);
  }
}

void DeviceMonitorPanel::placeDeviceOnCanvas(int deviceId) {
  auto *w = m_deviceWidgets.value(deviceId, nullptr);
  if (!w)
    return;

  QString activeViewName =
      (m_activeViewIndex >= 0 && m_activeViewIndex < m_viewNames.size())
          ? m_viewNames[m_activeViewIndex]
          : QStringLiteral("界面1");

  // 1. 优先寻找当前界面中的已有房间
  RoomRegion *targetRoomPtr = nullptr;
  for (auto &r : m_rooms) {
    QString rView = r.targetView.trimmed();
    if (rView.isEmpty()) rView = QStringLiteral("界面1");
    if (rView == activeViewName ||
        (activeViewName == QStringLiteral("界面1") && (rView == QStringLiteral("界面1") || rView.isEmpty()))) {
      targetRoomPtr = &r;
      break;
    }
  }

  // 2. 若不存在房间，自动创建新房间
  if (!targetRoomPtr) {
    RoomRegion newRoom;
    newRoom.id = QStringLiteral("room_%1").arg(QDateTime::currentMSecsSinceEpoch());
    newRoom.name = QStringLiteral("1号机房");
    newRoom.targetView = activeViewName;
    newRoom.shape = 0;
    newRoom.geom = QRect(50, 50, 350, 250);
    m_rooms.append(newRoom);
    targetRoomPtr = &m_rooms.last();
  }

  // 3. 更新设备的 targetRoom 关联
  for (auto &m : m_mappings) {
    if (m.deviceId == deviceId) {
      m.targetRoom = targetRoomPtr->name;
      break;
    }
  }

  autoArrangeRoomDevices();

  saveRoomLayout();
  saveConfig();

  // 重建画布
  rebuildRoomCanvas();
  updateZoom();

  appendLog(QStringLiteral("✓ 设备 %1 已摆放至房间 [%2]")
                .arg(w->label())
                .arg(targetRoomPtr->name),
            false);
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
  if (targetIdx < 0)
    return;

  DeviceBitMapping &m = m_mappings[targetIdx];

  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("编辑设备信息 (ID: #%1)").arg(deviceId));
  dlg.setMinimumWidth(380);
  dlg.setStyleSheet(
      "QDialog { background: #111827; color: #E2E8F0; font-family: 'Microsoft "
      "YaHei'; }"
      "QLabel { color: #94A3B8; font-size: 12px; }"
      "QLineEdit, QComboBox, QSpinBox { background: #1E293B; color: #E2E8F0; "
      "border: 1px solid #334155; padding: 5px; border-radius: 4px; font-size: "
      "12px; }"
      "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: "
      "#00D4FF; }"
      "QPushButton { background: #1E3A5F; color: #00D4FF; border: 1px solid "
      "#00D4FF; padding: 6px 16px; border-radius: 4px; font-weight: bold; }"
      "QPushButton:hover { background: #00D4FF; color: #111827; }");

  auto *layout = new QFormLayout(&dlg);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(12);

  auto *labelEdit = new QLineEdit(m.label, &dlg);
  labelEdit->setMaxLength(32);
  labelEdit->setPlaceholderText(QStringLiteral("最多可支持 32 个中文字符"));
  layout->addRow(QStringLiteral("设备名称 (上限32字):"), labelEdit);

  auto *typeCombo = new QComboBox(&dlg);
  typeCombo->addItem(QStringLiteral("烟温探测器"), QStringLiteral("detector"));
  typeCombo->addItem(QStringLiteral("分配阀"),
                     QStringLiteral("valve_distributor"));
  typeCombo->addItem(QStringLiteral("区域阀"), QStringLiteral("valve_zone"));
  typeCombo->addItem(QStringLiteral("总管隔离阀"),
                     QStringLiteral("valve_main_isolation"));
  typeCombo->addItem(QStringLiteral("控制分配阀"), QStringLiteral("valve"));
  typeCombo->addItem(QStringLiteral("手动报警按钮"),
                     QStringLiteral("manual_alarm"));
  typeCombo->addItem(QStringLiteral("1301气体钢瓶"),
                     QStringLiteral("gas_cylinder"));
  typeCombo->addItem(QStringLiteral("水泵"), QStringLiteral("water_pump"));
  typeCombo->addItem(QStringLiteral("压力开关"),
                     QStringLiteral("pressure_switch"));
  typeCombo->addItem(QStringLiteral("移动喷枪"),
                     QStringLiteral("mobile_spray_gun"));
  int tIdx = typeCombo->findData(m.deviceType);
  if (tIdx >= 0)
    typeCombo->setCurrentIndex(tIdx);
  layout->addRow(QStringLiteral("设备类型:"), typeCombo);

  auto *canIdEdit =
      new QLineEdit(QString("0x%1").arg(m.canId, 0, 16).toUpper(), &dlg);
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

  auto *targetViewCombo = new QComboBox(&dlg);
  targetViewCombo->setEditable(true);
  for (const QString &vName : m_viewNames) {
    targetViewCombo->addItem(vName);
  }
  QString tView =
      m.targetView.isEmpty() ? QStringLiteral("界面1") : m.targetView;
  int vIdx = targetViewCombo->findText(tView);
  if (vIdx >= 0)
    targetViewCombo->setCurrentIndex(vIdx);
  else
    targetViewCombo->setCurrentText(tView);
  layout->addRow(QStringLiteral("所属界面:"), targetViewCombo);

  auto *targetRoomCombo = new QComboBox(&dlg);
  targetRoomCombo->setEditable(true);
  targetRoomCombo->addItem(QStringLiteral("(未指定/未放置区)"), QString());
  QSet<QString> addedRoomNames;
  for (const auto &r : m_rooms) {
    QString rName = r.name.trimmed();
    if (!rName.isEmpty() && !addedRoomNames.contains(rName)) {
      targetRoomCombo->addItem(rName, rName);
      addedRoomNames.insert(rName);
    }
  }
  int rIdx = targetRoomCombo->findText(m.targetRoom);
  if (rIdx >= 0) targetRoomCombo->setCurrentIndex(rIdx);
  else if (!m.targetRoom.isEmpty()) targetRoomCombo->setCurrentText(m.targetRoom);
  layout->addRow(QStringLiteral("所属房间 (下拉选择或手动输入):"), targetRoomCombo);

  auto *btnBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  layout->addRow(btnBox);

  if (dlg.exec() == QDialog::Accepted) {
    m.label = labelEdit->text().trimmed();
    m.deviceType = typeCombo->currentData().toString();
    bool ok = false;
    quint32 cid = canIdEdit->text().trimmed().toUInt(&ok, 0);
    if (ok)
      m.canId = cid;
    m.byteIndex = byteSpin->value();
    m.bitIndex = bitSpin->value();
    m.defaultVal = defaultCombo->currentData().toInt();
    m.targetView = targetViewCombo->currentText().trimmed();
    if (m.targetView.isEmpty())
      m.targetView = QStringLiteral("界面1");

    QString newRoom = targetRoomCombo->currentText().trimmed();
    if (newRoom == QStringLiteral("(未指定/未放置区)")) newRoom.clear();
    m.targetRoom = newRoom;

    if (!m.targetRoom.isEmpty()) {
      autoArrangeRoomDevices();
    } else {
      m_deviceRoomPos.remove(m.deviceId);
    }

    saveRoomLayout();
    rebuildRoomCanvas();
    saveConfig();

    for (auto *client : m_clients) {
      sendConfigToClient(client);
    }

    appendLog(QStringLiteral("✓ 设备 #%1 [%2] 信息已修改")
                  .arg(m.deviceId)
                  .arg(m.label));
  }
}

void DeviceMonitorPanel::toggleFullScreen() {
  QWidget *topWin = window();
  if (!topWin)
    topWin = topLevelWidget();

  m_isFullScreen = !m_isFullScreen;
  if (m_isFullScreen) {
    if (m_toolbar)
      m_toolbar->setVisible(false);
    if (m_bottomBar)
      m_bottomBar->setVisible(false);
    if (topWin)
      topWin->showFullScreen();
    if (m_btnFullScreen)
      m_btnFullScreen->setText(QStringLiteral("🔙 退出全屏"));
    appendLog(QStringLiteral("🖥 已进入全屏显示模式 (按 ESC 退出)"), false);
  } else {
    if (m_toolbar)
      m_toolbar->setVisible(true);
    if (m_bottomBar)
      m_bottomBar->setVisible(true);
    if (topWin)
      topWin->showNormal();
    if (m_btnFullScreen)
      m_btnFullScreen->setText(QStringLiteral("📺 全屏"));
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

// ========== 界面分割与多屏联动 Slots 与辅助函数 ==========

void DeviceMonitorPanel::onTabChanged(int index) {
  if (index < 0 || index >= m_viewNames.size())
    return;
  if (m_isRebuildingCanvas)
    return;
  if (m_activeViewIndex == index)
    return;
  m_activeViewIndex = index;
  rebuildRoomCanvas();
}

void DeviceMonitorPanel::updateTabBar() {
  if (!m_viewTabBar)
    return;
  m_viewTabBar->blockSignals(true);
  while (m_viewTabBar->count() > 0) {
    m_viewTabBar->removeTab(0);
  }
  for (const QString &vName : m_viewNames) {
    m_viewTabBar->addTab(vName);
  }
  if (m_activeViewIndex >= m_viewTabBar->count()) {
    m_activeViewIndex = 0;
  }
  m_viewTabBar->setCurrentIndex(m_activeViewIndex);
  m_viewTabBar->blockSignals(false);

  if (m_floatingTabWrapper) {
    m_floatingTabWrapper->adjustSize();
    int bHeight = (m_bottomBar && m_bottomBar->isVisible()) ? m_bottomBar->height() : 0;
    int tabW = m_floatingTabWrapper->width();
    int tabH = m_floatingTabWrapper->height();
    int posX = qMax(10, (width() - tabW) / 2);
    int posY = height() - tabH - bHeight - 12;
    m_floatingTabWrapper->move(posX, posY);
    m_floatingTabWrapper->raise();
  }
}

void DeviceMonitorPanel::onSplitConfigClicked() {
  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("界面分割与区域名称设置"));
  dlg.setMinimumSize(420, 320);
  dlg.setStyleSheet(
      "QDialog { background-color: #111827; color: #E2E8F0; font-family: "
      "'Microsoft YaHei'; }"
      "QLabel { color: #94A3B8; font-size: 12px; }"
      "QLineEdit, QSpinBox { background: #1E293B; color: #00D4FF; border: 1px "
      "solid #334155; padding: 5px; border-radius: 4px; font-size: 12px; }"
      "QPushButton { padding: 6px 14px; border-radius: 4px; font-weight: bold; "
      "background: #0284C7; color: white; border: none; }"
      "QPushButton:hover { background: #38BDF8; }");

  auto *layout = new QVBoxLayout(&dlg);
  layout->setContentsMargins(15, 15, 15, 15);
  layout->setSpacing(12);

  auto *spinLayout = new QHBoxLayout();
  spinLayout->addWidget(
      new QLabel(QStringLiteral("分割界面总数 (1-8):"), &dlg));
  auto *spin = new QSpinBox(&dlg);
  spin->setRange(1, 8);
  spin->setValue(m_viewNames.size());
  spinLayout->addWidget(spin);
  layout->addLayout(spinLayout);

  auto *nameScroll = new QScrollArea(&dlg);
  nameScroll->setWidgetResizable(true);
  nameScroll->setStyleSheet(
      "QScrollArea { border: 1px solid #1E293E; background: #0F172A; }");
  auto *nameWidget = new QWidget();
  auto *nameLayout = new QVBoxLayout(nameWidget);
  nameLayout->setSpacing(6);

  QList<QLineEdit *> nameEdits;
  auto updateEdits = [this, nameWidget, nameLayout, &nameEdits](int count) {
    qDeleteAll(nameEdits);
    nameEdits.clear();
    while (nameLayout->count() > 0) {
      QLayoutItem *item = nameLayout->takeAt(0);
      delete item;
    }
    for (int i = 0; i < count; ++i) {
      auto *row = new QHBoxLayout();
      row->addWidget(
          new QLabel(QStringLiteral("界面 %1 名称:").arg(i + 1), nameWidget));
      auto *edit = new QLineEdit(nameWidget);
      QString defaultName = (i < m_viewNames.size())
                                ? m_viewNames[i]
                                : QStringLiteral("界面%1").arg(i + 1);
      edit->setText(defaultName);
      row->addWidget(edit);
      nameLayout->addLayout(row);
      nameEdits.append(edit);
    }
  };

  updateEdits(spin->value());
  connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updateEdits);

  nameScroll->setWidget(nameWidget);
  layout->addWidget(nameScroll, 1);

  auto *btnBox = new QHBoxLayout();
  auto *btnOk = new QPushButton(QStringLiteral("保存设置"), &dlg);
  auto *btnCancel = new QPushButton(QStringLiteral("取消"), &dlg);
  btnCancel->setStyleSheet("background: #334155; color: #94A3B8;");
  btnBox->addStretch();
  btnBox->addWidget(btnOk);
  btnBox->addWidget(btnCancel);
  layout->addLayout(btnBox);

  connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
  connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

  if (dlg.exec() == QDialog::Accepted) {
    m_viewNames.clear();
    for (auto *edit : nameEdits) {
      QString text = edit->text().trimmed();
      if (text.isEmpty())
        text = QStringLiteral("界面");
      m_viewNames.append(text);
    }
    updateTabBar();
    rebuildRoomCanvas();
    saveConfig();
    appendLog(QStringLiteral("✓ 已更新界面分割数量为 %1 个界面")
                  .arg(m_viewNames.size()));
  }
}

void DeviceMonitorPanel::onMultiScreenToggled(bool checked) {
  m_multiScreenActive = checked;
  int screenCount = QGuiApplication::screens().size();
  if (checked) {
    appendLog(
        QStringLiteral("🖥️ 多屏联动模式已开启！系统检测到实际物理屏幕数: %1")
            .arg(screenCount),
        false);
  } else {
    appendLog(QStringLiteral("🖥️ 多屏联动模式已关闭"), false);
  }
  // 根据当前显示模式选择正确的重建函数
  // rebuildGrid/rebuildRoomCanvas 内部已调用 scheduleSubWindowUpdate()
  rebuildRoomCanvas();
}

void DeviceMonitorPanel::onScreenLayoutChanged() {
  int screenCount = QGuiApplication::screens().size();
  appendLog(QStringLiteral("🌐 检测到外接显示设备硬件变更！当前连接屏幕数: %1")
                .arg(screenCount),
            false);
  if (m_multiScreenActive) {
    scheduleSubWindowUpdate();
  }
}

void DeviceMonitorPanel::scheduleSubWindowUpdate() {
  // Coalescing debounce: if timer is already started, just let it run.
  // This prevents multiple rapid calls (from rebuildGrid + onTabChanged
  // both firing) from creating multiple simultaneous updateSubWindows executions.
  if (m_subWinUpdateTimer && !m_subWinUpdateTimer->isActive()) {
    m_subWinUpdateTimer->start();
  }
}

void DeviceMonitorPanel::updateSubWindows() {
  // 重入保护：防止 show()/showMaximized() 内部触发事件处理导致递归调用
  if (m_updatingSubWindows)
    return;
  m_updatingSubWindows = true;

  if (!m_multiScreenActive) {
    for (auto subWin : m_subWindows) {
      if (subWin && !subWin.isNull()) {
        subWin->hide();
        subWin->deleteLater();
      }
    }
    m_subWindows.clear();
    m_updatingSubWindows = false;
    return;
  }

  QList<QScreen *> screens = QGuiApplication::screens();
  int numScreens = screens.size();

  // 收集非主屏当前选中的所有子界面名称列表
  QStringList subViewNames;
  for (int v = 0; v < m_viewNames.size(); ++v) {
    if (v != m_activeViewIndex) {
      subViewNames.append(m_viewNames[v]);
    }
  }

  // 缩减超出的子窗口 - 使用 deleteLater 避免顶层 QDialog 在 OS 消息队列未清空时被析构
  while (m_subWindows.size() > subViewNames.size()) {
    auto oldWin = m_subWindows.takeLast();
    if (oldWin && !oldWin.isNull()) {
      oldWin->hide();
      oldWin->deleteLater();
    }
  }

  // 填充或重定子窗口
  for (int sIdx = 0; sIdx < subViewNames.size(); ++sIdx) {
    QString targetViewName = subViewNames[sIdx];
    SubMonitorWindow *subWin = nullptr;

    if (sIdx < m_subWindows.size()) {
      subWin = m_subWindows[sIdx].data();
      if (subWin) {
        subWin->setViewName(targetViewName);
      }
    }
    if (!subWin) {
      subWin = new SubMonitorWindow(targetViewName, nullptr);
      if (sIdx < m_subWindows.size()) {
        m_subWindows[sIdx] = subWin;
      } else {
        m_subWindows.append(subWin);
      }
    }

    subWin->rebuildDevicesAndRooms(m_mappings, m_deviceWidgets, m_rooms, m_deviceRoomPos, m_activeTemplate);

    // 计算对应的拓展物理屏幕
    int targetScreenIdx =
        (sIdx + 1 < numScreens) ? (sIdx + 1) : (numScreens - 1);
    QScreen *targetScreen = screens[targetScreenIdx];
    QRect targetGeom = targetScreen->geometry();

    subWin->move(targetGeom.x() + 40 * sIdx, targetGeom.y() + 40 * sIdx);
    subWin->show();
    if (numScreens > 1 && (sIdx + 1) < numScreens) {
      if (subWin->windowHandle()) {
        subWin->windowHandle()->setScreen(targetScreen);
      }
      subWin->showMaximized();
    }
  }

  m_updatingSubWindows = false;
}

// ========== 布局/视图 模式与悬浮面板逻辑 ==========

void DeviceMonitorPanel::showLayoutFloatingBox() {
  if (!m_layoutFloatingDialog) {
    m_layoutFloatingDialog = new QDialog(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_layoutFloatingDialog->setObjectName("LayoutFloatingBox");
    m_layoutFloatingDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    m_layoutFloatingDialog->setStyleSheet(
        "QDialog#LayoutFloatingBox { background: rgba(15, 23, 42, 0.95); "
        "border: 1px solid #10B981; border-radius: 8px; }"
        "QLabel { color: #E2E8F0; font-family: 'Microsoft YaHei'; font-size: 12px; }"
        "QPushButton { background: #1E293B; color: #10B981; border: 1px solid #10B981; "
        "padding: 6px 12px; border-radius: 4px; font-size: 11px; font-weight: bold; font-family: 'Microsoft YaHei'; }"
        "QPushButton:hover { background: #10B981; color: #0F172A; }");

    auto *shadow = new QGraphicsDropShadowEffect(m_layoutFloatingDialog);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(16, 185, 129, 80));
    shadow->setOffset(0, 4);
    m_layoutFloatingDialog->setGraphicsEffect(shadow);

    auto *mainLay = new QVBoxLayout(m_layoutFloatingDialog);
    mainLay->setContentsMargins(12, 10, 12, 10);
    mainLay->setSpacing(10);

    // 标题栏 (可拖拽)
    m_layoutFloatingTitleBar = new QWidget(m_layoutFloatingDialog);
    m_layoutFloatingTitleBar->setCursor(Qt::SizeAllCursor);
    m_layoutFloatingTitleBar->installEventFilter(this);
    auto *titleLay = new QHBoxLayout(m_layoutFloatingTitleBar);
    titleLay->setContentsMargins(0, 0, 0, 0);
    auto *titleLbl = new QLabel(QStringLiteral("📐 布局控制 (按住拖拽)"), m_layoutFloatingTitleBar);
    titleLbl->setStyleSheet("color: #10B981; font-weight: bold; font-size: 12px;");
    auto *btnClose = new QPushButton(QStringLiteral("✕"), m_layoutFloatingTitleBar);
    btnClose->setFixedSize(22, 22);
    btnClose->setStyleSheet("QPushButton { border: none; background: transparent; color: #94A3B8; font-size: 12px; }"
                            "QPushButton:hover { color: #EF4444; }");
    connect(btnClose, &QPushButton::clicked, this, [this]() { toggleLayoutMode(false); });
    titleLay->addWidget(titleLbl);
    titleLay->addStretch();
    titleLay->addWidget(btnClose);
    mainLay->addWidget(m_layoutFloatingTitleBar);

    // 按钮网格
    auto *grid = new QGridLayout();
    grid->setSpacing(8);

    auto *btnAddRoom = new QPushButton(QStringLiteral("➕ 新建房间"), m_layoutFloatingDialog);
    connect(btnAddRoom, &QPushButton::clicked, this, &DeviceMonitorPanel::onAddRoom);

    auto *btnTpl = new QPushButton(QStringLiteral("📐 导入布局模板"), m_layoutFloatingDialog);
    auto *tplMenu = new QMenu(btnTpl);
    tplMenu->setStyleSheet(
        "QMenu { background-color: #0F172A; border: 1px solid #1E293B; color: #E2E8F0; padding: 4px; font-family: 'Microsoft YaHei'; font-size: 12px; }"
        "QMenu::item { padding: 6px 22px; border-radius: 4px; }"
        "QMenu::item:selected { background: #10B981; color: #0F172A; }");
    tplMenu->addAction(QStringLiteral("🚢 核潜艇布局"), this, [this]() { applyLayoutTemplate(QStringLiteral("submarine")); });
    tplMenu->addAction(QStringLiteral("🏢 写字楼布局"), this, [this]() { applyLayoutTemplate(QStringLiteral("building")); });
    tplMenu->addAction(QStringLiteral("🛥️ 水面舰船布局"), this, [this]() { applyLayoutTemplate(QStringLiteral("warship")); });
    tplMenu->addAction(QStringLiteral("🛫 航母布局"), this, [this]() { applyLayoutTemplate(QStringLiteral("carrier")); });
    tplMenu->addSeparator();
    tplMenu->addAction(QStringLiteral("🚫 恢复无模板布局"), this, [this]() { applyLayoutTemplate(QString()); });
    btnTpl->setMenu(tplMenu);

    auto *btnDelRoom = new QPushButton(QStringLiteral("🗑️ 删除房间"), m_layoutFloatingDialog);
    connect(btnDelRoom, &QPushButton::clicked, this, &DeviceMonitorPanel::onDeleteRoom);

    auto *btnImport = new QPushButton(QStringLiteral("📥 导入布局文件"), m_layoutFloatingDialog);
    connect(btnImport, &QPushButton::clicked, this, &DeviceMonitorPanel::onImportClicked);

    auto *btnAutoArrange = new QPushButton(QStringLiteral("🧹 一键整理"), m_layoutFloatingDialog);
    connect(btnAutoArrange, &QPushButton::clicked, this, &DeviceMonitorPanel::autoArrangeRoomsAndDevices);

    auto *btnExport = new QPushButton(QStringLiteral("📤 导出布局文件"), m_layoutFloatingDialog);
    connect(btnExport, &QPushButton::clicked, this, &DeviceMonitorPanel::onExportClicked);

    auto *btnSave = new QPushButton(QStringLiteral("💾 保存布局"), m_layoutFloatingDialog);
    connect(btnSave, &QPushButton::clicked, this, [this]() {
      saveRoomLayout();
      appendLog(QStringLiteral("✓ 当前设备房间布局已成功保存！"), false);
    });

    grid->addWidget(btnAddRoom, 0, 0);
    grid->addWidget(btnAutoArrange, 0, 1);
    grid->addWidget(btnTpl, 1, 0);
    grid->addWidget(btnDelRoom, 1, 1);
    grid->addWidget(btnImport, 2, 0);
    grid->addWidget(btnExport, 2, 1);
    grid->addWidget(btnSave, 3, 0, 1, 2);

    mainLay->addLayout(grid);
  }

  m_layoutFloatingDialog->setFixedSize(310, 210);
  int posX = qMax(20, width() - 330);
  int posY = m_toolbar ? m_toolbar->height() + 10 : 50;
  m_layoutFloatingDialog->move(mapToGlobal(QPoint(posX, posY)));
  m_layoutFloatingDialog->show();
  m_layoutFloatingDialog->raise();
}

void DeviceMonitorPanel::closeLayoutFloatingBox() {
  if (m_layoutFloatingDialog) {
    m_layoutFloatingDialog->hide();
  }
}

void DeviceMonitorPanel::toggleLayoutMode(bool enable) {
  m_layoutEditingEnabled = enable;
  if (m_btnLayoutToggle) {
    if (enable) {
      m_btnLayoutToggle->setText(QStringLiteral("📐 布局使能中"));
      m_btnLayoutToggle->setStyleSheet(
          "QPushButton { color: #FFFFFF; background: #10B981; border: 1px solid #059669; "
          "padding: 5px 14px; border-radius: 3px; font-size: 11px; font-weight: bold; font-family: 'Microsoft YaHei'; }");
    } else {
      m_btnLayoutToggle->setText(QStringLiteral("📐 布局/视图"));
      m_btnLayoutToggle->setStyleSheet(
          "QPushButton { color: #10B981; background: #1A2720; border: 1px solid #1E3E2E; "
          "padding: 5px 14px; border-radius: 3px; font-size: 11px; font-weight: bold; font-family: 'Microsoft YaHei'; }"
          "QPushButton:hover { background: #1E3E2E; border-color: #10B981; }");
    }
  }

  if (enable) {
    showLayoutFloatingBox();
  } else {
    closeLayoutFloatingBox();
  }

  for (auto *rw : m_roomWidgets) {
    if (rw) {
      rw->setEditingEnabled(enable);
    }
  }

  rebuildRoomCanvas();
}

void DeviceMonitorPanel::onDeleteRoom() {
  if (m_rooms.isEmpty()) {
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前没有任何房间！"));
    return;
  }

  QStringList roomNames;
  for (const auto &r : m_rooms) {
    roomNames.append(r.name);
  }

  bool ok;
  QString roomToDelete = QInputDialog::getItem(
      this, QStringLiteral("删除房间"), QStringLiteral("请选择要删除的房间:"),
      roomNames, 0, false, &ok);

  if (ok && !roomToDelete.isEmpty()) {
    for (int i = 0; i < m_rooms.size(); ++i) {
      if (m_rooms[i].name == roomToDelete) {
        appendLog(QStringLiteral("✕ 已删除房间: %1").arg(m_rooms[i].name), true);
        m_rooms.removeAt(i);
        break;
      }
    }
    saveRoomLayout();
    rebuildRoomCanvas();
    updateZoom();
  }
}
