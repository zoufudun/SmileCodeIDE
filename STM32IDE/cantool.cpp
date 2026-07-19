#include "cantool.h"
#include "normalsenddialog.h"

#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QProcess>

#include "candevicedialog.h"
#include "cantheme.h"
#include "devicemonitorpanel.h"
#include "devicemonitordialog.h"
#include "canviewpanel.h"
#include "canopenviewpanel.h"
#include "canbusutilizationdialog.h"
#include "USBCANFD/zlgcan.h"
#include <cmath>

// 扁平紧凑风格的自定义工具栏按钮，隐藏默认下拉箭头并自绘红色/橙色三角指示器
class CANToolButton : public QToolButton {
public:
  CANToolButton(const QString &text, int iconType, bool hasMenu, QWidget *parent = nullptr)
      : QToolButton(parent), m_hasMenu(hasMenu), m_iconType(iconType) {
    setText(text);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setFixedWidth(86);
    setFixedHeight(66);
    
    setStyleSheet(
        "QToolButton { background: transparent; color: #ABB2BF; border: none; font-size: 11px; font-weight: bold; padding: 2px; }"
        "QToolButton:hover { background-color: #2E3A4E; border-radius: 6px; color: #FFFFFF; }"
        "QToolButton:pressed { background-color: #1A2536; }"
        "QToolButton::menu-indicator { image: none; }"
    );
    
    if (hasMenu) {
      setPopupMode(QToolButton::InstantPopup);
    }
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QToolButton::paintEvent(event);
    if (m_hasMenu) {
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor("#FF5722")); // 橙红色倒三角
      
      int x = width() - 14;
      int y = 20;
      QPolygon triangle;
      triangle << QPoint(x, y) << QPoint(x + 6, y) << QPoint(x + 3, y + 4);
      painter.drawPolygon(triangle);
    }
  }

private:
  bool m_hasMenu;
  int m_iconType;
};

CANTool::CANTool(QWidget *parent) : QDialog(parent) {
  m_can = new CanInterface(this);
  m_co = new CanOpenMaster(m_can, this);
  setupUi();

  // 后端信号
  connect(m_can, &CanInterface::connected, this, &CANTool::onCanConnected);
  connect(m_can, &CanInterface::disconnected, this, &CANTool::onCanDisconnected);
  connect(m_can, &CanInterface::errorOccurred, this, &CANTool::onCanError);

  // CANopen 主站接收到的帧需同时喂给协议解析
  connect(m_can, &CanInterface::frameReceived, m_co, &CanOpenMaster::processFrame);

  if (!m_can->libraryLoaded()) {
    m_statusLabel->setText("警告: 未加载 zlgcan.dll (无法收发报文)");
  } else {
    // 驱动诊断
    QString driverVer, deviceName;
    bool online = false;
    if (m_can->diagnoseDriver(&driverVer, &deviceName, &online)) {
      m_statusLabel->setText(QStringLiteral("驱动就绪: %1, 设备: %2 (%3)")
                                 .arg(driverVer)
                                 .arg(deviceName)
                                 .arg(online ? "在线" : "离线"));
    } else {
      m_statusLabel->setText("驱动已加载，诊断失败");
    }
  }
}

CANTool::~CANTool() = default;

void CANTool::setupUi() {
  setWindowTitle("CAN / CAN FD / CANopen 测试工具");
  setMinimumSize(960, 720);
  setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // 1. 创建顶部工具栏
  createToolbar();
  mainLayout->addWidget(m_toolbar);

  // 2. 主区域分割器 (QSplitter)
  m_splitter = new QSplitter(Qt::Horizontal, this);
  m_splitter->setChildrenCollapsible(false);
  m_splitter->setStyleSheet("QSplitter::handle { background-color: #3b4048; width: 4px; }");
  
  // 给分割器添加四周间距
  QWidget *splitterContainer = new QWidget();
  QVBoxLayout *containerLayout = new QVBoxLayout(splitterContainer);
  containerLayout->setContentsMargins(8, 8, 8, 8);
  containerLayout->addWidget(m_splitter);
  mainLayout->addWidget(splitterContainer, 1);

  // 3. 底部状态栏
  QWidget *statusBar = new QWidget();
  statusBar->setFixedHeight(32);
  statusBar->setObjectName("canStatusBar");
  statusBar->setStyleSheet("QWidget#canStatusBar { border-top: 1px solid #3b4048; background-color: #21252b; } QLabel { color: #abb2bf; }");
  QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
  statusLayout->setContentsMargins(12, 0, 12, 0);

  m_statusLabel = new QLabel("未连接");
  m_statusLabel->setStyleSheet("color:#c0392b; font-weight: bold;");

  m_themeCombo = new QComboBox();
  m_themeCombo->addItems(CanTheme::names());
  m_themeCombo->setCurrentText(QStringLiteral("深色 (Dark)"));
  connect(m_themeCombo, &QComboBox::currentTextChanged, this,
          [this](const QString &name) { applyTheme(name); });

  statusLayout->addWidget(new QLabel("设备状态:"));
  statusLayout->addWidget(m_statusLabel, 1);
  statusLayout->addWidget(new QLabel("主题:"));
  statusLayout->addWidget(m_themeCombo);
  mainLayout->addWidget(statusBar);

  setAttribute(Qt::WA_DeleteOnClose);

  // 默认应用深色主题
  applyTheme(QStringLiteral("深色 (Dark)"));

  // 默认添加第一个 CAN 监视视图
  QAction *defaultAction = new QAction("新建CAN视图", this);
  onNewViewTriggered(defaultAction);
  delete defaultAction;
}

void CANTool::createToolbar() {
  m_toolbar = new QWidget(this);
  m_toolbar->setObjectName("canToolbar");
  m_toolbar->setStyleSheet("QWidget#canToolbar { background-color: #1e2835; }");
  m_toolbar->setFixedHeight(72);

  QHBoxLayout *layout = new QHBoxLayout(m_toolbar);
  layout->setContentsMargins(6, 4, 6, 4);
  layout->setSpacing(4);

  // 1. 设备管理
  CANToolButton *btnDevice = new CANToolButton("设备管理", 1, false, this);
  btnDevice->setIcon(createToolbarIcon(1));
  btnDevice->setIconSize(QSize(32, 32));
  connect(btnDevice, &QToolButton::clicked, this, &CANTool::onDeviceManage);
  layout->addWidget(btnDevice);

  // 2. 新建视图
  CANToolButton *btnNewView = new CANToolButton("新建视图", 2, true, this);
  btnNewView->setIcon(createToolbarIcon(2));
  btnNewView->setIconSize(QSize(32, 32));
  QMenu *menuNewView = new QMenu(this);
  menuNewView->addAction("新建CAN视图");
  menuNewView->addAction("新建CANopen视图");
  connect(menuNewView, &QMenu::triggered, this, &CANTool::onNewViewTriggered);
  btnNewView->setMenu(menuNewView);
  layout->addWidget(btnNewView);

  // 3. 发送数据
  CANToolButton *btnSendData = new CANToolButton("发送数据", 3, true, this);
  btnSendData->setIcon(createToolbarIcon(3));
  btnSendData->setIconSize(QSize(32, 32));
  QMenu *menuSendData = new QMenu(this);
  menuSendData->addAction("普通发送");
  menuSendData->addAction("列表发送");
  connect(menuSendData, &QMenu::triggered, this, &CANTool::onSendDataTriggered);
  btnSendData->setMenu(menuSendData);
  layout->addWidget(btnSendData);

  // 4. 通道利用率
  CANToolButton *btnChannel = new CANToolButton("通道利用率", 4, false, this);
  btnChannel->setIcon(createToolbarIcon(4));
  btnChannel->setIconSize(QSize(32, 32));
  connect(btnChannel, &QToolButton::clicked, this, &CANTool::onChannelUtilization);
  layout->addWidget(btnChannel);

  // 5. 高级功能
  CANToolButton *btnAdvanced = new CANToolButton("高级功能", 5, true, this);
  btnAdvanced->setIcon(createToolbarIcon(5));
  btnAdvanced->setIconSize(QSize(32, 32));
  QMenu *menuAdvanced = new QMenu(this);
  menuAdvanced->addAction("报文过滤配置");
  menuAdvanced->addAction("错误诊断");
  connect(menuAdvanced, &QMenu::triggered, this, &CANTool::onAdvancedFeaturesTriggered);
  btnAdvanced->setMenu(menuAdvanced);
  layout->addWidget(btnAdvanced);

  // 6. 工具
  CANToolButton *btnTools = new CANToolButton("工具", 6, true, this);
  btnTools->setIcon(createToolbarIcon(6));
  btnTools->setIconSize(QSize(32, 32));
  QMenu *menuTools = new QMenu(this);
  menuTools->addAction("DBC 解析器");
  menuTools->addAction("内置计算器");
  menuTools->addAction("Status Monitor");
  connect(menuTools, &QMenu::triggered, this, &CANTool::onToolsTriggered);
  btnTools->setMenu(menuTools);
  layout->addWidget(btnTools);

  // 7. 设置&帮助
  CANToolButton *btnSettings = new CANToolButton("设置&帮助", 7, true, this);
  btnSettings->setIcon(createToolbarIcon(7));
  btnSettings->setIconSize(QSize(32, 32));
  QMenu *menuSettings = new QMenu(this);
  menuSettings->addAction("深色 (Dark) 主题");
  menuSettings->addAction("浅色 (Light) 主题");
  menuSettings->addAction("查看帮助");
  menuSettings->addAction("关于");
  connect(menuSettings, &QMenu::triggered, this, &CANTool::onSettingsHelpTriggered);
  btnSettings->setMenu(menuSettings);
  layout->addWidget(btnSettings);

  layout->addStretch();
}

QIcon CANTool::createToolbarIcon(int type) {
  QPixmap pixmap(32, 32);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);

  QColor white("#ABB2BF");
  QColor orange("#FF5722");

  if (type == 1) { // 设备管理
    painter.setPen(QPen(white, 2));
    painter.drawLine(4, 8, 20, 8);
    painter.drawLine(4, 14, 16, 14);
    painter.drawLine(4, 20, 12, 20);
    painter.setPen(Qt::NoPen);
    painter.setBrush(orange);
    painter.drawEllipse(18, 16, 10, 10);
    painter.setPen(QPen(orange, 1.5));
    for (int i = 0; i < 8; ++i) {
      double angle = i * 3.14159265 / 4.0;
      int cx = 23 + qRound(6.0 * std::cos(angle));
      int cy = 21 + qRound(6.0 * std::sin(angle));
      painter.drawLine(23, 21, cx, cy);
    }
    painter.setBrush(QColor("#1e2835"));
    painter.drawEllipse(21, 19, 4, 4);
  }
  else if (type == 2) { // 新建视图
    painter.setPen(QPen(white, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(4, 4, 16, 14);
    painter.drawRect(10, 10, 16, 14);
    painter.setPen(QPen(orange, 2));
    painter.drawLine(14, 17, 22, 17);
    painter.drawLine(18, 13, 18, 21);
  }
  else if (type == 3) { // 发送数据
    painter.setPen(QPen(white, 2));
    painter.drawLine(4, 10, 24, 10);
    painter.drawLine(24, 10, 20, 6);
    painter.drawLine(24, 10, 20, 14);
    painter.drawLine(8, 22, 28, 22);
    painter.drawLine(8, 22, 12, 18);
    painter.drawLine(8, 22, 12, 26);
    painter.setPen(orange);
    QFont font = painter.font();
    font.setPixelSize(7);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(2, 11, 28, 10), Qt::AlignCenter, "1010");
  }
  else if (type == 4) { // 通道利用率
    painter.setPen(QPen(white, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(4, 4, 24, 24);
    painter.drawLine(4, 10, 28, 10);
    painter.setPen(Qt::NoPen);
    painter.setBrush(white);
    painter.drawEllipse(10, 13, 12, 12);
    painter.setBrush(orange);
    painter.drawPie(10, 13, 12, 12, 0, 120 * 16);
  }
  else if (type == 5) { // 高级功能
    painter.setPen(QPen(white, 1.5));
    painter.setBrush(Qt::NoBrush);
    QPolygon paper;
    paper << QPoint(6, 4) << QPoint(20, 4) << QPoint(26, 10) << QPoint(26, 28) << QPoint(6, 28);
    painter.drawPolygon(paper);
    painter.drawLine(20, 4, 20, 10);
    painter.drawLine(20, 10, 26, 10);
    painter.setPen(QPen(orange, 1.5));
    painter.drawArc(12, 14, 8, 8, 0, 180 * 16);
    painter.drawLine(12, 18, 12, 24);
    painter.drawLine(20, 18, 20, 24);
    painter.drawArc(12, 20, 8, 8, 180 * 16, 180 * 16);
  }
  else if (type == 6) { // 工具
    painter.setPen(QPen(white, 2));
    painter.drawLine(6, 26, 24, 8);
    painter.setPen(Qt::NoPen);
    painter.setBrush(white);
    painter.drawRect(20, 4, 8, 6);
    painter.setPen(QPen(orange, 2));
    painter.drawLine(26, 26, 8, 8);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(6, 6, 6, 6);
  }
  else if (type == 7) { // 设置&帮助
    painter.setPen(Qt::NoPen);
    painter.setBrush(white);
    painter.drawEllipse(8, 8, 16, 16);
    painter.setPen(QPen(white, 2.5));
    for (int i = 0; i < 8; ++i) {
      double angle = i * 3.14159265 / 4.0;
      int cx = 16 + qRound(9.0 * std::cos(angle));
      int cy = 16 + qRound(9.0 * std::sin(angle));
      painter.drawLine(16, 16, cx, cy);
    }
    painter.setBrush(QColor("#1e2835"));
    painter.drawEllipse(13, 13, 6, 6);
    painter.setPen(orange);
    QFont font = painter.font();
    font.setPixelSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(18, 18, 14, 14), Qt::AlignCenter, "?");
  }
  else if (type == 8) { // Status Monitor (Pulse/heartbeat or screen)
    painter.setPen(QPen(white, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(4, 6, 24, 16); // Screen
    painter.drawLine(8, 22, 24, 22); // Stand base
    painter.drawLine(16, 22, 16, 25); // Stand stem
    // Pulse wave inside screen
    painter.setPen(QPen(orange, 1.5));
    painter.drawLine(6, 14, 11, 14);
    painter.drawLine(11, 14, 13, 9);
    painter.drawLine(13, 9, 15, 19);
    painter.drawLine(15, 19, 17, 12);
    painter.drawLine(17, 12, 19, 14);
    painter.drawLine(19, 14, 26, 14);
  }

  painter.end();
  return QIcon(pixmap);
}

void CANTool::applyTheme(const QString &name) {
  m_currentStyle = CanTheme::styleSheet(name);
  setStyleSheet(m_currentStyle);
  if (m_deviceDialog) {
    m_deviceDialog->applyThemeStyle(m_currentStyle);
  }
  if (m_sendDialog) {
    m_sendDialog->applyThemeStyle(m_currentStyle);
  }
  if (m_monitorDialog) {
    m_monitorDialog->setStyleSheet(m_currentStyle);
  }
  if (m_busUtilizationDialog) {
    m_busUtilizationDialog->setStyleSheet(m_currentStyle);
  }
  if (m_splitter) {
    for (int i = 0; i < m_splitter->count(); ++i) {
      QWidget *w = m_splitter->widget(i);
      CanViewPanel *cv = qobject_cast<CanViewPanel*>(w);
      if (cv) cv->applyThemeStyle(m_currentStyle);
      CanOpenViewPanel *cov = qobject_cast<CanOpenViewPanel*>(w);
      if (cov) cov->applyThemeStyle(m_currentStyle);
    }
  }
}

void CANTool::onDeviceManage() {
  if (!m_deviceDialog) {
    m_deviceDialog = new CanDeviceDialog(m_can, this);
    if (!m_currentStyle.isEmpty()) {
      m_deviceDialog->setStyleSheet(m_currentStyle);
    }
  }
  m_deviceDialog->show();
  m_deviceDialog->raise();
  m_deviceDialog->activateWindow();
}

void CANTool::onNewViewTriggered(QAction *action) {
  static int viewCounter = 1;
  if (action->text() == "新建CAN视图") {
    CanViewPanel *panel = new CanViewPanel(m_can, viewCounter++, this);
    panel->applyThemeStyle(m_currentStyle);
    m_splitter->addWidget(panel);
    connect(panel, &CanViewPanel::closeRequested, this, [this](CanViewPanel *p) {
      if (m_splitter->count() <= 1) {
        QMessageBox::information(this, "提示", "请至少保留一个视图窗口！");
        return;
      }
      p->close();
      p->deleteLater();
    });
  } else if (action->text() == "新建CANopen视图") {
    CanOpenViewPanel *panel = new CanOpenViewPanel(m_co, m_can, viewCounter++, this);
    panel->applyThemeStyle(m_currentStyle);
    m_splitter->addWidget(panel);
    connect(panel, &CanOpenViewPanel::closeRequested, this, [this](CanOpenViewPanel *p) {
      if (m_splitter->count() <= 1) {
        QMessageBox::information(this, "提示", "请至少保留一个视图窗口！");
        return;
      }
      p->close();
      p->deleteLater();
    });
  }
}

void CANTool::onStatusMonitorClicked() {
  if (!m_monitorDialog) {
    m_monitorDialog = new DeviceMonitorDialog(m_can, this);
    if (!m_currentStyle.isEmpty()) {
      m_monitorDialog->setStyleSheet(m_currentStyle);
    }
  }
  m_monitorDialog->show();
  m_monitorDialog->raise();
  m_monitorDialog->activateWindow();
}

void CANTool::onSendDataTriggered(QAction *action) {
  if (action->text() == "普通发送" || action->text() == "列表发送") {
    if (!m_sendDialog) {
      m_sendDialog = new NormalSendDialog(m_can, this);
      m_sendDialog->applyThemeStyle(m_currentStyle);
    }
    m_sendDialog->show();
    m_sendDialog->raise();
    m_sendDialog->activateWindow();
  }
}

void CANTool::onChannelUtilization() {
  if (!m_busUtilizationDialog) {
    m_busUtilizationDialog = new CanBusUtilizationDialog(m_can, this);
    if (!m_currentStyle.isEmpty()) {
      m_busUtilizationDialog->setStyleSheet(m_currentStyle);
    }
  }
  m_busUtilizationDialog->show();
  m_busUtilizationDialog->raise();
  m_busUtilizationDialog->activateWindow();
}

void CANTool::onAdvancedFeaturesTriggered(QAction *action) {
  QMessageBox::information(this, "高级功能", QString("打开高级功能：%1 (敬请期待后续集成)").arg(action->text()));
}

void CANTool::onToolsTriggered(QAction *action) {
  if (action->text().contains("计算器")) {
    QProcess::startDetached("calc.exe", QStringList());
  } else if (action->text() == "Status Monitor") {
    onStatusMonitorClicked();
  } else {
    QMessageBox::information(this, "工具", QString("启动调试工具：%1 (敬请期待后续集成)").arg(action->text()));
  }
}

void CANTool::onSettingsHelpTriggered(QAction *action) {
  if (action->text() == "深色 (Dark) 主题") {
    m_themeCombo->setCurrentText("深色 (Dark)");
  } else if (action->text() == "浅色 (Light) 主题") {
    m_themeCombo->setCurrentText("浅色 (Light)");
  } else if (action->text().contains("帮助")) {
    QMessageBox::information(this, "帮助说明", 
      "1. 使用 [设备管理] 进行 CAN 设备型号和波特率的设定与启闭。\n"
      "2. [新建视图] 用于在 报文收发 与 CANopen 调试页面之间进行快速切换。\n"
      "3. [发送数据] 会启动 [普通发送] 定时与增量发送控制器，进行多 Tab 发送管理与列表发送控制。\n"
      "4. 支持经典 CAN 及 CAN FD，内置 CANopen 主站测试环境（NMT、PDO、SDO 诊断）。");
  } else if (action->text().contains("关于")) {
    QMessageBox::about(this, "关于 CAN 测试调试工具", "CAN / CAN FD / CANopen Tool v2.0\n基于周立功 ZLG SDK\nSmileCode IDE 专属调试套件");
  }
}

void CANTool::onCanConnected() {
  m_statusLabel->setText("已连接");
  m_statusLabel->setStyleSheet("color:#27ae60;");
}

void CANTool::onCanDisconnected() {
  m_statusLabel->setText("未连接");
  m_statusLabel->setStyleSheet("color:#c0392b;");
}

void CANTool::onCanError(const QString &message) {
  m_statusLabel->setText(message);
  m_statusLabel->setStyleSheet("color:#c0392b;");
}
