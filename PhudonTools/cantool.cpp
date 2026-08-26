#include "cantool.h"
#include "appmanager.h"
#include "normalsenddialog.h"

#include <QAction>
#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QToolButton>
#include <QVBoxLayout>


#include "TOOLS/CIconFont.h"
#include "USBCANFD/zlgcan.h"
#include "canbusutilizationdialog.h"
#include "candevicedialog.h"
#include "canopenviewpanel.h"
#include "cantheme.h"
#include "canviewpanel.h"
#include "devicemonitordialog.h"
#include "devicemonitorpanel.h"
#include <cmath>


// 扁平紧凑风格的自定义工具栏按钮，仅显示图标，悬停时显示完整文本提示
class CANToolButton : public QToolButton {
public:
  CANToolButton(const QString &text, int iconType, bool hasMenu,
                QWidget *parent = nullptr)
      : QToolButton(parent), m_hasMenu(hasMenu), m_iconType(iconType) {
    setObjectName("canNavButton");
    setToolTip(text);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setFixedSize(64, 48);
    setCursor(Qt::PointingHandCursor);

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
      painter.setBrush(QColor("#FF7043")); // 橙红色精致小倒三角

      int x = width() - 9;
      int y = height() - 8;
      QPolygon triangle;
      triangle << QPoint(x, y) << QPoint(x + 5, y) << QPoint(x + 2, y + 3);
      painter.drawPolygon(triangle);
    }
  }

private:
  bool m_hasMenu;
  int m_iconType;
};

CANTool::CANTool(QWidget *parent) : QDialog(parent) {
  setObjectName("canRoot");
  m_can = new CanInterface(this);
  m_co = new CanOpenMaster(m_can, this);
  setupUi();

  // 后端信号
  connect(m_can, &CanInterface::connected, this, &CANTool::onCanConnected);
  connect(m_can, &CanInterface::disconnected, this,
          &CANTool::onCanDisconnected);
  connect(m_can, &CanInterface::errorOccurred, this, &CANTool::onCanError);

  // CANopen 主站接收到的帧需同时喂给协议解析
  connect(m_can, &CanInterface::frameReceived, m_co,
          &CanOpenMaster::processFrame);

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
  setObjectName("canRoot");
  setWindowTitle("CAN / CAN FD / CANopen 测试工具");
  setWindowIcon(QIcon(":/icons/xptools2.png"));
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
  QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
  statusLayout->setContentsMargins(12, 0, 12, 0);
  statusLayout->setSpacing(8);

  m_statusLabel = new QLabel("未连接");
  m_statusLabel->setObjectName("canStatusText");

  // 右下角主题切换按钮 (图标 0xe622)
  m_btnTheme = new QToolButton(this);
  m_btnTheme->setObjectName("btnThemeIcon");
  try {
    QFont iconFont = CIconFont::instance()->getIconFont(18);
    iconFont.setPixelSize(18);
    m_btnTheme->setFont(iconFont);
  } catch (...) {
  }
  m_btnTheme->setText(QString(QChar(0xe622)));
  m_btnTheme->setToolTip("切换主题");
  m_btnTheme->setPopupMode(QToolButton::InstantPopup);
  m_btnTheme->setStyleSheet("QToolButton::menu-indicator { image: none; }");

  QMenu *menuTheme = new QMenu(m_btnTheme);
  for (const QString &themeName : CanTheme::names()) {
    menuTheme->addAction(themeName, this, [this, themeName]() {
      applyTheme(themeName);
      AppManager::instance()->setCurrentTheme(themeName);
    });
  }
  m_btnTheme->setMenu(menuTheme);

  statusLayout->addWidget(new QLabel("设备状态:"));
  statusLayout->addWidget(m_statusLabel, 1);
  statusLayout->addWidget(m_btnTheme);
  mainLayout->addWidget(statusBar);

  setAttribute(Qt::WA_DeleteOnClose);

  // 默认应用当前全局主题
  applyTheme(AppManager::instance()->getCurrentTheme());

  // 默认添加第一个 CAN 监视视图
  QAction *defaultAction = new QAction("新建CAN视图", this);
  onNewViewTriggered(defaultAction);
  delete defaultAction;
}

static QFrame *createToolBarSeparator(QWidget *parent) {
  QFrame *sep = new QFrame(parent);
  sep->setFrameShape(QFrame::VLine);
  sep->setFrameShadow(QFrame::Plain);
  sep->setFixedWidth(1);
  sep->setFixedHeight(22);
  sep->setStyleSheet("background-color: rgba(148, 163, 184, 0.25); border: "
                     "none; margin: 0 4px;");
  return sep;
}

void CANTool::createToolbar() {
  m_toolbar = new QWidget(this);
  m_toolbar->setObjectName("canToolbar");
  m_toolbar->setFixedHeight(48);

  QHBoxLayout *layout = new QHBoxLayout(m_toolbar);
  layout->setContentsMargins(10, 4, 10, 4);
  layout->setSpacing(6);

  // [分组 1: 设备与视图]
  // 1. 设备管理
  CANToolButton *btnDevice = new CANToolButton("设备管理", 1, false, this);
  btnDevice->setIcon(createToolbarIcon(1));
  btnDevice->setIconSize(QSize(28, 28));
  connect(btnDevice, &QToolButton::clicked, this, &CANTool::onDeviceManage);
  layout->addWidget(btnDevice);

  // 2. 新建视图
  CANToolButton *btnNewView = new CANToolButton("新建视图", 2, true, this);
  btnNewView->setIcon(createToolbarIcon(2));
  btnNewView->setIconSize(QSize(28, 28));
  QMenu *menuNewView = new QMenu(this);
  menuNewView->addAction("新建CAN视图");
  menuNewView->addAction("新建CANopen视图");
  connect(menuNewView, &QMenu::triggered, this, &CANTool::onNewViewTriggered);
  btnNewView->setMenu(menuNewView);
  layout->addWidget(btnNewView);

  layout->addWidget(createToolBarSeparator(m_toolbar));

  // [分组 2: 报文与监控]
  // 3. 发送数据
  CANToolButton *btnSendData = new CANToolButton("发送数据", 3, true, this);
  btnSendData->setIcon(createToolbarIcon(3));
  btnSendData->setIconSize(QSize(28, 28));
  QMenu *menuSendData = new QMenu(this);
  menuSendData->addAction("普通发送");
  menuSendData->addAction("列表发送");
  connect(menuSendData, &QMenu::triggered, this, &CANTool::onSendDataTriggered);
  btnSendData->setMenu(menuSendData);
  layout->addWidget(btnSendData);

  // 4. 通道利用率
  CANToolButton *btnChannel = new CANToolButton("通道利用率", 4, false, this);
  btnChannel->setIcon(createToolbarIcon(4));
  btnChannel->setIconSize(QSize(28, 28));
  connect(btnChannel, &QToolButton::clicked, this,
          &CANTool::onChannelUtilization);
  layout->addWidget(btnChannel);

  layout->addWidget(createToolBarSeparator(m_toolbar));

  // [分组 3: 进阶功能与工具箱]
  // 5. 高级功能
  CANToolButton *btnAdvanced = new CANToolButton("高级功能", 5, true, this);
  btnAdvanced->setIcon(createToolbarIcon(5));
  btnAdvanced->setIconSize(QSize(28, 28));
  QMenu *menuAdvanced = new QMenu(this);
  menuAdvanced->addAction("报文过滤配置");
  menuAdvanced->addAction("错误诊断");
  connect(menuAdvanced, &QMenu::triggered, this,
          &CANTool::onAdvancedFeaturesTriggered);
  btnAdvanced->setMenu(menuAdvanced);
  layout->addWidget(btnAdvanced);

  // 6. 工具
  CANToolButton *btnTools = new CANToolButton("工具", 6, true, this);
  btnTools->setIcon(createToolbarIcon(6));
  btnTools->setIconSize(QSize(28, 28));
  QMenu *menuTools = new QMenu(this);
  menuTools->addAction("DBC 解析器");
  menuTools->addAction("内置计算器");
  menuTools->addAction("Status Monitor");
  connect(menuTools, &QMenu::triggered, this, &CANTool::onToolsTriggered);
  btnTools->setMenu(menuTools);
  layout->addWidget(btnTools);

  layout->addWidget(createToolBarSeparator(m_toolbar));

  // [分组 4: 帮助与系统]
  // 7. 设置&帮助
  CANToolButton *btnSettings = new CANToolButton("设置&帮助", 7, true, this);
  btnSettings->setIcon(createToolbarIcon(7));
  btnSettings->setIconSize(QSize(28, 28));
  QMenu *menuSettings = new QMenu(this);
  menuSettings->addAction("查看帮助");
  menuSettings->addAction("关于");
  connect(menuSettings, &QMenu::triggered, this,
          &CANTool::onSettingsHelpTriggered);
  btnSettings->setMenu(menuSettings);
  layout->addWidget(btnSettings);

  layout->addStretch();
}

QIcon CANTool::createToolbarIcon(int type) {
  QPixmap pixmap(64, 64);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);

  QColor white("#E2E8F0");
  QColor orange("#FF6D00");

  if (type == 1) { // 设备管理: 芯片/接口盒 + 精度齿轮
    // 芯片外壳
    painter.setPen(QPen(white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(8, 14, 48, 36), 6, 6);
    // 左侧信号线
    painter.drawLine(14, 24, 26, 24);
    painter.drawLine(14, 32, 22, 32);
    painter.drawLine(14, 40, 26, 40);
    // 右侧设置齿轮
    painter.setPen(Qt::NoPen);
    painter.setBrush(orange);
    painter.drawEllipse(QRectF(34, 22, 20, 20));
    painter.setPen(QPen(orange, 3, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 8; ++i) {
      double angle = i * 3.14159265 / 4.0;
      int cx = 44 + qRound(12.0 * std::cos(angle));
      int cy = 32 + qRound(12.0 * std::sin(angle));
      painter.drawLine(44, 32, cx, cy);
    }
    // 齿轮中心轴心
    painter.setBrush(QColor("#0F172A"));
    painter.drawEllipse(QRectF(40, 28, 8, 8));
  } else if (type == 2) { // 新建视图: 双叠层窗口 + 醒目 "+" 号
    painter.setPen(QPen(white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    // 底层窗口
    painter.drawRoundedRect(QRectF(8, 8, 36, 30), 4, 4);
    painter.drawLine(8, 17, 44, 17);
    // 顶层窗口
    painter.drawRoundedRect(QRectF(20, 20, 36, 32), 4, 4);
    painter.drawLine(20, 29, 56, 29);
    // 顶层窗口右下角的 "+" 图标
    painter.setPen(QPen(orange, 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(38, 42, 48, 42);
    painter.drawLine(43, 37, 43, 47);
  } else if (type == 3) { // 发送数据: 双向高速流箭头 + "1010" 报文码
    // 上侧向右发送箭头 (Tx)
    painter.setPen(
        QPen(white, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(8, 16, 52, 16);
    painter.drawLine(52, 16, 42, 8);
    painter.drawLine(52, 16, 42, 24);
    // 下侧向左接收箭头 (Rx)
    painter.drawLine(12, 48, 56, 48);
    painter.drawLine(12, 48, 22, 40);
    painter.drawLine(12, 48, 22, 56);
    // 中间数据流脉冲码
    painter.setPen(orange);
    QFont font = painter.font();
    font.setFamily("Consolas");
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(6, 22, 52, 20), Qt::AlignCenter, "1 0 1 0");
  } else if (type == 4) { // 通道利用率: 精密仪表盘 + 动态利用率刻度
    painter.setPen(QPen(white, 3, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    // 仪表盘外圈圆弧
    painter.drawArc(QRectF(8, 8, 48, 48), -30 * 16, 240 * 16);
    // 橙色高利用率扇区
    painter.setPen(Qt::NoPen);
    painter.setBrush(orange);
    painter.drawPie(QRectF(12, 12, 40, 40), 30 * 16, 120 * 16);
    // 仪表盘中心轴和指针
    painter.setPen(QPen(white, 3.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(32, 34, 46, 18);
    painter.setBrush(orange);
    painter.drawEllipse(QRectF(27, 29, 10, 10));
    // 底座线
    painter.setPen(QPen(white, 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(16, 54, 48, 54);
  } else if (type == 5) { // 高级功能: 报文漏斗过滤器 + 诊断脉冲
    painter.setPen(QPen(white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    // 漏斗轮廓
    QPolygonF funnel;
    funnel << QPointF(8, 10) << QPointF(56, 10) << QPointF(38, 34)
           << QPointF(38, 52) << QPointF(26, 56) << QPointF(26, 34);
    painter.drawPolygon(funnel);
    // 内部诊断波形
    painter.setPen(QPen(orange, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(16, 22, 24, 22);
    painter.drawLine(24, 22, 28, 14);
    painter.drawLine(28, 14, 34, 30);
    painter.drawLine(34, 30, 40, 22);
    painter.drawLine(40, 22, 48, 22);
  } else if (type == 6) { // 工具: 扳手 & 螺丝刀精密交叉
    // 螺丝刀 (左上到右下)
    painter.setPen(QPen(white, 3.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(12, 12, 48, 48);
    painter.setPen(Qt::NoPen);
    painter.setBrush(white);
    painter.drawRoundedRect(QRectF(8, 8, 14, 14), 3, 3);
    // 扳手 (右上到左下)
    painter.setPen(QPen(orange, 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(50, 14, 14, 50);
    painter.setPen(QPen(orange, 3.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(QRectF(40, 6, 18, 18), 45 * 16, 270 * 16);
  } else if (type == 7) { // 设置&帮助: 精密齿轮 + 醒目问号
    // 齿轮外圈
    painter.setPen(Qt::NoPen);
    painter.setBrush(white);
    painter.drawEllipse(QRectF(12, 12, 40, 40));
    painter.setPen(QPen(white, 4, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 8; ++i) {
      double angle = i * 3.14159265 / 4.0;
      int cx = 32 + qRound(22.0 * std::cos(angle));
      int cy = 32 + qRound(22.0 * std::sin(angle));
      painter.drawLine(32, 32, cx, cy);
    }
    // 齿轮内部深色底
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#0F172A"));
    painter.drawEllipse(QRectF(20, 20, 24, 24));
    // 中心橙色问号
    painter.setPen(orange);
    QFont font = painter.font();
    font.setFamily("Segoe UI");
    font.setPixelSize(18);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(20, 20, 24, 24), Qt::AlignCenter, "?");
  } else if (type == 8) { // Status Monitor (屏幕 + 心跳)
    painter.setPen(QPen(white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(8, 10, 48, 34), 4, 4); // Screen
    painter.drawLine(18, 48, 46, 48);                     // Base
    painter.drawLine(32, 44, 32, 48);                     // Stem
    // 脉冲波形
    painter.setPen(QPen(orange, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(12, 27, 22, 27);
    painter.drawLine(22, 27, 26, 17);
    painter.drawLine(26, 17, 32, 37);
    painter.drawLine(32, 37, 36, 23);
    painter.drawLine(36, 23, 40, 27);
    painter.drawLine(40, 27, 52, 27);
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
      CanViewPanel *cv = qobject_cast<CanViewPanel *>(w);
      if (cv)
        cv->applyThemeStyle(m_currentStyle);
      CanOpenViewPanel *cov = qobject_cast<CanOpenViewPanel *>(w);
      if (cov)
        cov->applyThemeStyle(m_currentStyle);
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
    connect(
        panel, &CanViewPanel::closeRequested, this, [this](CanViewPanel *p) {
          if (m_splitter->count() <= 1) {
            QMessageBox::information(this, "提示", "请至少保留一个视图窗口！");
            return;
          }
          p->close();
          p->deleteLater();
        });
  } else if (action->text() == "新建CANopen视图") {
    CanOpenViewPanel *panel =
        new CanOpenViewPanel(m_co, m_can, viewCounter++, this);
    panel->applyThemeStyle(m_currentStyle);
    m_splitter->addWidget(panel);
    connect(panel, &CanOpenViewPanel::closeRequested, this,
            [this](CanOpenViewPanel *p) {
              if (m_splitter->count() <= 1) {
                QMessageBox::information(this, "提示",
                                         "请至少保留一个视图窗口！");
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
  QMessageBox::information(
      this, "高级功能",
      QString("打开高级功能：%1 (敬请期待后续集成)").arg(action->text()));
}

void CANTool::onToolsTriggered(QAction *action) {
  if (action->text().contains("计算器")) {
    QProcess::startDetached("calc.exe", QStringList());
  } else if (action->text() == "Status Monitor") {
    onStatusMonitorClicked();
  } else {
    QMessageBox::information(
        this, "工具",
        QString("启动调试工具：%1 (敬请期待后续集成)").arg(action->text()));
  }
}

void CANTool::onSettingsHelpTriggered(QAction *action) {
  if (action->text().contains("帮助")) {
    QMessageBox::information(
        this, "帮助说明",
        "1. 使用 [设备管理] 进行 CAN 设备型号和波特率的设定与启闭。\n"
        "2. [新建视图] 用于在 报文收发 与 CANopen 调试页面之间进行快速切换。\n"
        "3. [发送数据] 会启动 [普通发送] 定时与增量发送控制器，进行多 Tab "
        "发送管理与列表发送控制。\n"
        "4. 支持经典 CAN 及 CAN FD，内置 CANopen 主站测试环境（NMT、PDO、SDO "
        "诊断）。");
  } else if (action->text().contains("关于")) {
    QMessageBox::about(this, "关于 CAN 测试调试工具",
                       "CAN / CAN FD / CANopen Tool v2.0\n基于周立功 ZLG "
                       "SDK\nSmileCode IDE 专属调试套件");
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
