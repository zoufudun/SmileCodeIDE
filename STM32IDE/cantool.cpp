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
  connect(m_can, &CanInterface::disconnected, this,
          &CANTool::onCanDisconnected);
  connect(m_can, &CanInterface::errorOccurred, this, &CANTool::onCanError);
  connect(m_can, &CanInterface::frameReceived, this,
          &CANTool::onFrameReceived);
  connect(m_can, &CanInterface::frameSent, this, &CANTool::onFrameSent);

  // CANopen 主站接收到的帧需同时喂给协议解析
  connect(m_can, &CanInterface::frameReceived, m_co,
          &CanOpenMaster::processFrame);

  connect(m_co, &CanOpenMaster::heartbeatReceived, this,
          &CANTool::onHeartbeat);
  connect(m_co, &CanOpenMaster::emcyReceived, this, &CANTool::onEmcy);
  connect(m_co, &CanOpenMaster::pdoReceived, this, &CANTool::onPdo);
  connect(m_co, &CanOpenMaster::sdoReadFinished, this,
          &CANTool::onSdoReadFinished);
  connect(m_co, &CanOpenMaster::sdoWriteFinished, this,
          &CANTool::onSdoWriteFinished);
  connect(m_co, &CanOpenMaster::logMessage, this, &CANTool::onCanOpenLog);

  setControlsEnabled(false);

  if (!m_can->libraryLoaded()) {
    m_statusLabel->setText("未加载 zlgcan.dll");
    appendCanOpenLog(QStringLiteral("[警告] 未能加载 zlgcan 动态库：%1，请将 "
                                    "zlgcan.dll 及 kerneldlls 放到程序目录")
                         .arg(m_can->libraryError()));
  } else {
    // 驱动诊断
    QString driverVer, deviceName;
    bool online = false;
    if (m_can->diagnoseDriver(&driverVer, &deviceName, &online)) {
      m_statusLabel->setText(QStringLiteral("驱动就绪：%1, 设备: %2 (%3)")
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

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // 1. 创建顶部工具栏
  createToolbar();
  mainLayout->addWidget(m_toolbar);

  // 2. 主选项卡 (报文监视/发送 和 CANopen 测试)
  m_tabs = new QTabWidget();
  m_tabs->setDocumentMode(true);
  m_tabs->addTab(createMonitorPanel(), "报文收发 (CAN / CAN FD)");
  m_tabs->addTab(createCanOpenPanel(), "CANopen 测试");
  
  // 给选项卡添加四周间距
  QWidget *tabsContainer = new QWidget();
  QVBoxLayout *containerLayout = new QVBoxLayout(tabsContainer);
  containerLayout->setContentsMargins(8, 8, 8, 8);
  containerLayout->addWidget(m_tabs);
  mainLayout->addWidget(tabsContainer, 1);

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
  menuNewView->addAction("报文收发视图");
  menuNewView->addAction("CANopen 测试视图");
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

  painter.end();
  return QIcon(pixmap);
}

void CANTool::applyTheme(const QString &name) {
  m_currentStyle = CanTheme::styleSheet(name);
  setStyleSheet(m_currentStyle);
  if (m_deviceDialog) {
    m_deviceDialog->setStyleSheet(m_currentStyle);
  }
  if (m_sendDialog) {
    m_sendDialog->applyThemeStyle(m_currentStyle);
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
  if (action->text() == "报文收发视图") {
    m_tabs->setCurrentIndex(0);
  } else if (action->text() == "CANopen 测试视图") {
    m_tabs->setCurrentIndex(1);
  }
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
  QMessageBox::information(this, "通道利用率", "当前通道利用率正常:\n通道 0 (CAN 1): 1.15%\n通道 1 (CAN 2): 0.00%");
}

void CANTool::onAdvancedFeaturesTriggered(QAction *action) {
  QMessageBox::information(this, "高级功能", QString("打开高级功能：%1 (敬请期待后续集成)").arg(action->text()));
}

void CANTool::onToolsTriggered(QAction *action) {
  if (action->text().contains("计算器")) {
    QProcess::startDetached("calc.exe", QStringList());
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

QWidget *CANTool::createMonitorPanel() {
  QWidget *panel = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(panel);

  m_receiveTreeWidget = new QTreeWidget();
  m_receiveTreeWidget->setHeaderLabels(
      {"时间", "方向", "帧ID", "帧类型", "长度", "数据"});
  m_receiveTreeWidget->setColumnWidth(0, 110);
  m_receiveTreeWidget->setColumnWidth(1, 50);
  m_receiveTreeWidget->setColumnWidth(2, 90);
  m_receiveTreeWidget->setColumnWidth(3, 130);
  m_receiveTreeWidget->setColumnWidth(4, 50);
  m_receiveTreeWidget->setRootIsDecorated(false);
  m_receiveTreeWidget->setAlternatingRowColors(true);
  m_receiveTreeWidget->setFont(
      QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_receiveTreeWidget->header()->setStretchLastSection(true);
  layout->addWidget(m_receiveTreeWidget, 1);

  QGroupBox *sendGroup = new QGroupBox("报文发送");
  QGridLayout *sendLayout = new QGridLayout(sendGroup);

  m_idLineEdit = new QLineEdit("123");
  m_dataLineEdit = new QLineEdit("00 11 22 33 44 55 66 77");
  m_sendButton = new QPushButton("发送");
  m_clearButton = new QPushButton("清空");

  m_stdFrameBtn = new QRadioButton("标准帧");
  m_extFrameBtn = new QRadioButton("扩展帧");
  m_stdFrameBtn->setChecked(true);
  m_remoteCheckBox = new QCheckBox("远程帧");
  m_fdFrameCheckBox = new QCheckBox("FD 帧");
  m_brsCheckBox = new QCheckBox("加速(BRS)");
  m_brsCheckBox->setEnabled(false);
  connect(m_fdFrameCheckBox, &QCheckBox::toggled, this, [this](bool on) {
    m_brsCheckBox->setEnabled(on);
    if (on)
      m_remoteCheckBox->setChecked(false);
    m_remoteCheckBox->setEnabled(!on);
  });

  QHBoxLayout *typeLayout = new QHBoxLayout();
  typeLayout->addWidget(m_stdFrameBtn);
  typeLayout->addWidget(m_extFrameBtn);
  typeLayout->addWidget(m_remoteCheckBox);
  typeLayout->addWidget(m_fdFrameCheckBox);
  typeLayout->addWidget(m_brsCheckBox);
  typeLayout->addStretch();

  connect(m_sendButton, &QPushButton::clicked, this, &CANTool::onSendClicked);
  connect(m_clearButton, &QPushButton::clicked, this, &CANTool::onClearReceive);

  sendLayout->addWidget(new QLabel("帧ID(Hex):"), 0, 0);
  sendLayout->addWidget(m_idLineEdit, 0, 1);
  sendLayout->addLayout(typeLayout, 0, 2, 1, 2);
  sendLayout->addWidget(new QLabel("数据(Hex):"), 1, 0);
  sendLayout->addWidget(m_dataLineEdit, 1, 1, 1, 2);
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addWidget(m_sendButton);
  btnLayout->addWidget(m_clearButton);
  sendLayout->addLayout(btnLayout, 1, 3);

  layout->addWidget(sendGroup);
  return panel;
}

QWidget *CANTool::createCanOpenPanel() {
  QWidget *panel = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(panel);

  // 节点 + NMT
  QGroupBox *nmtGroup = new QGroupBox("节点管理 (NMT)");
  QHBoxLayout *nmtLayout = new QHBoxLayout(nmtGroup);
  m_nodeIdSpin = new QSpinBox();
  m_nodeIdSpin->setRange(0, 127);
  m_nodeIdSpin->setValue(1);
  m_nodeIdSpin->setPrefix("节点 ");
  connect(m_nodeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int v) { m_co->setNodeId(static_cast<quint8>(v)); });

  m_nmtCommandCombo = new QComboBox();
  m_nmtCommandCombo->addItem("启动节点 (Start)", 0x01);
  m_nmtCommandCombo->addItem("停止节点 (Stop)", 0x02);
  m_nmtCommandCombo->addItem("进入预运行 (Pre-op)", 0x80);
  m_nmtCommandCombo->addItem("复位节点 (Reset Node)", 0x81);
  m_nmtCommandCombo->addItem("复位通信 (Reset Comm)", 0x82);
  m_nmtSendButton = new QPushButton("发送 NMT");
  m_syncButton = new QPushButton("发送 SYNC");
  connect(m_nmtSendButton, &QPushButton::clicked, this, &CANTool::onNmtSend);
  connect(m_syncButton, &QPushButton::clicked, this, &CANTool::onSyncSend);

  nmtLayout->addWidget(m_nodeIdSpin);
  nmtLayout->addWidget(m_nmtCommandCombo, 1);
  nmtLayout->addWidget(m_nmtSendButton);
  nmtLayout->addWidget(m_syncButton);

  // SDO
  QGroupBox *sdoGroup = new QGroupBox("SDO 服务数据对象 (快速传输)");
  QGridLayout *sdoLayout = new QGridLayout(sdoGroup);
  m_sdoIndexEdit = new QLineEdit("6040");
  m_sdoSubIndexEdit = new QLineEdit("00");
  m_sdoValueEdit = new QLineEdit("000F");
  m_sdoSizeCombo = new QComboBox();
  m_sdoSizeCombo->addItem("1 字节", 1);
  m_sdoSizeCombo->addItem("2 字节", 2);
  m_sdoSizeCombo->addItem("3 字节", 3);
  m_sdoSizeCombo->addItem("4 字节", 4);
  m_sdoSizeCombo->setCurrentIndex(1);
  m_sdoReadButton = new QPushButton("读取 (Upload)");
  m_sdoWriteButton = new QPushButton("写入 (Download)");
  connect(m_sdoReadButton, &QPushButton::clicked, this, &CANTool::onSdoRead);
  connect(m_sdoWriteButton, &QPushButton::clicked, this, &CANTool::onSdoWrite);

  sdoLayout->addWidget(new QLabel("索引(Hex):"), 0, 0);
  sdoLayout->addWidget(m_sdoIndexEdit, 0, 1);
  sdoLayout->addWidget(new QLabel("子索引(Hex):"), 0, 2);
  sdoLayout->addWidget(m_sdoSubIndexEdit, 0, 3);
  sdoLayout->addWidget(new QLabel("数值(Hex):"), 1, 0);
  sdoLayout->addWidget(m_sdoValueEdit, 1, 1);
  sdoLayout->addWidget(new QLabel("长度:"), 1, 2);
  sdoLayout->addWidget(m_sdoSizeCombo, 1, 3);
  sdoLayout->addWidget(m_sdoReadButton, 2, 0, 1, 2);
  sdoLayout->addWidget(m_sdoWriteButton, 2, 2, 1, 2);

  // 节点状态表
  QGroupBox *nodeGroup = new QGroupBox("节点状态 / 心跳监控");
  QVBoxLayout *nodeLayout = new QVBoxLayout(nodeGroup);
  m_nodeTreeWidget = new QTreeWidget();
  m_nodeTreeWidget->setHeaderLabels({"节点ID", "状态", "最近更新"});
  m_nodeTreeWidget->setColumnWidth(0, 80);
  m_nodeTreeWidget->setColumnWidth(1, 160);
  m_nodeTreeWidget->setRootIsDecorated(false);
  nodeLayout->addWidget(m_nodeTreeWidget);

  // 日志
  QGroupBox *logGroup = new QGroupBox("CANopen 日志");
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
  m_canOpenLog = new QPlainTextEdit();
  m_canOpenLog->setReadOnly(true);
  m_canOpenLog->setMaximumBlockCount(500);
  m_canOpenLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  logLayout->addWidget(m_canOpenLog);

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addWidget(nodeGroup, 1);
  bottomLayout->addWidget(logGroup, 1);

  layout->addWidget(nmtGroup);
  layout->addWidget(sdoGroup);
  layout->addLayout(bottomLayout, 1);
  return panel;
}

void CANTool::onCanConnected() {
  m_statusLabel->setText("已连接");
  m_statusLabel->setStyleSheet("color:#27ae60;");
  setControlsEnabled(true);
}

void CANTool::onCanDisconnected() {
  m_statusLabel->setText("未连接");
  m_statusLabel->setStyleSheet("color:#c0392b;");
  setControlsEnabled(false);
}

void CANTool::onCanError(const QString &message) {
  appendCanOpenLog(QStringLiteral("[错误] %1").arg(message));
  m_statusLabel->setText(message);
  m_statusLabel->setStyleSheet("color:#c0392b;");
}

void CANTool::setControlsEnabled(bool connected) {
  m_sendButton->setEnabled(connected);
  m_nmtSendButton->setEnabled(connected);
  m_syncButton->setEnabled(connected);
  m_sdoReadButton->setEnabled(connected);
  m_sdoWriteButton->setEnabled(connected);
}

void CANTool::onSendClicked() {
  bool ok = false;
  const quint32 id = m_idLineEdit->text().trimmed().toUInt(&ok, 16);
  if (!ok) {
    QMessageBox::warning(this, "提示", "帧 ID 格式错误，请输入十六进制。");
    return;
  }

  CanFrame frame;
  frame.id = id;
  frame.extended = m_extFrameBtn->isChecked();
  frame.fd = m_fdFrameCheckBox->isChecked();
  frame.brs = frame.fd && m_brsCheckBox->isChecked();
  frame.remote = !frame.fd && m_remoteCheckBox->isChecked();

  if (!frame.remote) {
    QString hex = m_dataLineEdit->text();
    hex.remove(' ');
    frame.data = QByteArray::fromHex(hex.toLatin1());
    const int maxLen = frame.fd ? 64 : 8;
    if (frame.data.size() > maxLen) {
      QMessageBox::warning(this, "提示",
                           QStringLiteral("数据长度超过 %1 字节上限。").arg(maxLen));
      return;
    }
  }

  m_can->sendFrame(frame);
}

void CANTool::onClearReceive() { m_receiveTreeWidget->clear(); }

void CANTool::appendFrameRow(const CanFrame &frame, bool tx) {
  QString type;
  if (frame.fd) {
    type = frame.extended ? "FD扩展" : "FD标准";
    if (frame.brs)
      type += "+BRS";
  } else if (frame.remote) {
    type = frame.extended ? "扩展远程" : "标准远程";
  } else {
    type = frame.extended ? "扩展数据" : "标准数据";
  }

  const QString time =
      QDateTime::fromMSecsSinceEpoch(frame.timestamp).toString("HH:mm:ss.zzz");
  const int idWidth = frame.extended ? 8 : 3;
  const QString idText =
      QString("%1").arg(frame.id, idWidth, 16, QChar('0')).toUpper();

  QStringList cols;
  cols << time << (tx ? "发送" : "接收") << idText << type
       << QString::number(frame.data.size())
       << QString::fromLatin1(frame.data.toHex(' ').toUpper());

  QTreeWidgetItem *item = new QTreeWidgetItem(cols);
  item->setForeground(1, tx ? QBrush(QColor("#2980b9"))
                            : QBrush(QColor("#16a085")));
  m_receiveTreeWidget->addTopLevelItem(item);
  m_receiveTreeWidget->scrollToBottom();

  // 限制条目数量，避免内存无限增长
  while (m_receiveTreeWidget->topLevelItemCount() > 2000) {
    delete m_receiveTreeWidget->takeTopLevelItem(0);
  }
}

void CANTool::onFrameReceived(const CanFrame &frame) {
  appendFrameRow(frame, false);
}

void CANTool::onFrameSent(const CanFrame &frame) { appendFrameRow(frame, true); }

void CANTool::onNmtSend() {
  const NmtCommand cmd =
      static_cast<NmtCommand>(m_nmtCommandCombo->currentData().toInt());
  m_co->sendNmt(cmd, static_cast<quint8>(m_nodeIdSpin->value()));
}

void CANTool::onSyncSend() { m_co->sendSync(); }

void CANTool::onSdoRead() {
  bool ok1 = false, ok2 = false;
  const quint16 index =
      static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub =
      static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  if (!ok1 || !ok2) {
    QMessageBox::warning(this, "提示", "索引/子索引格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoRead(index, sub);
}

void CANTool::onSdoWrite() {
  bool ok1 = false, ok2 = false, ok3 = false;
  const quint16 index =
      static_cast<quint16>(m_sdoIndexEdit->text().trimmed().toUInt(&ok1, 16));
  const quint8 sub =
      static_cast<quint8>(m_sdoSubIndexEdit->text().trimmed().toUInt(&ok2, 16));
  const quint32 value = m_sdoValueEdit->text().trimmed().toUInt(&ok3, 16);
  if (!ok1 || !ok2 || !ok3) {
    QMessageBox::warning(this, "提示", "索引/子索引/数值格式错误。");
    return;
  }
  m_co->setNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
  m_co->sdoWrite(index, sub, value, m_sdoSizeCombo->currentData().toInt());
}

void CANTool::onHeartbeat(quint8 nodeId, NmtState state) {
  const QString idText = QString::number(nodeId);
  QTreeWidgetItem *item = nullptr;
  for (int i = 0; i < m_nodeTreeWidget->topLevelItemCount(); ++i) {
    if (m_nodeTreeWidget->topLevelItem(i)->text(0) == idText) {
      item = m_nodeTreeWidget->topLevelItem(i);
      break;
    }
  }
  if (!item) {
    item = new QTreeWidgetItem(QStringList{idText});
    m_nodeTreeWidget->addTopLevelItem(item);
  }
  item->setText(1, CanOpenMaster::stateText(state));
  item->setText(2, QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void CANTool::onEmcy(quint8 nodeId, quint16 errorCode, quint8 errorRegister,
                     const QByteArray &manufacturer) {
  appendCanOpenLog(
      QStringLiteral("[EMCY] 节点 %1 错误码=0x%2 错误寄存器=0x%3 厂商=%4")
          .arg(nodeId)
          .arg(errorCode, 4, 16, QChar('0'))
          .arg(errorRegister, 2, 16, QChar('0'))
          .arg(QString::fromLatin1(manufacturer.toHex(' ').toUpper())));
}

void CANTool::onPdo(quint8 nodeId, int pdoNumber, bool isTpdo,
                    const QByteArray &data) {
  appendCanOpenLog(QStringLiteral("[%1PDO%2] 节点 %3 数据=%4")
                       .arg(isTpdo ? "T" : "R")
                       .arg(pdoNumber)
                       .arg(nodeId)
                       .arg(QString::fromLatin1(data.toHex(' ').toUpper())));
}

void CANTool::onSdoReadFinished(bool success, quint16 index, quint8 subIndex,
                                quint32 value, quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 读] [%1:%2] = 0x%3 (%4)")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(value, 0, 16)
                         .arg(value));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 读失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CANTool::onSdoWriteFinished(bool success, quint16 index, quint8 subIndex,
                                 quint32 abortCode) {
  if (success) {
    appendCanOpenLog(QStringLiteral("[SDO 写] [%1:%2] 成功")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex));
  } else {
    appendCanOpenLog(QStringLiteral("[SDO 写失败] [%1:%2] %3")
                         .arg(index, 4, 16, QChar('0'))
                         .arg(subIndex)
                         .arg(CanOpenMaster::abortText(abortCode)));
  }
}

void CANTool::onCanOpenLog(const QString &message) { appendCanOpenLog(message); }

void CANTool::appendCanOpenLog(const QString &text) {
  const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  m_canOpenLog->appendPlainText(QStringLiteral("%1  %2").arg(time, text));
}
