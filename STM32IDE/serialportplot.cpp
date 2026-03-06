#include "serialportplot.h"
#include "TOOLS/CIconFont.h"
#include "curvesettings.h"
#include "mainwindow.h"
#include "toastwidget.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSize>
#include <QSplitter>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
class ScaledAxisTicker : public QCPAxisTicker {
public:
  explicit ScaledAxisTicker(double divisor, int fixedPrecision = -1)
      : m_divisor(divisor > 0.0 ? divisor : 1.0),
        m_fixedPrecision(fixedPrecision) {}

  QString getTickLabel(double tick, const QLocale &locale, QChar formatChar,
                       int precision) Q_DECL_OVERRIDE {
    const QChar fmt = formatChar.isNull() ? QChar('f') : formatChar;
    const int p = (m_fixedPrecision >= 0) ? m_fixedPrecision : precision;
    return locale.toString(tick / m_divisor, fmt.toLatin1(), p);
  }

private:
  double m_divisor;
  int m_fixedPrecision;
};
} // namespace

SerialSession::SerialSession(QWidget *parent)
    : QWidget(parent), m_lastPortCount(0), m_rxCount(0), m_txCount(0),
      m_xValue(0) {
  m_serial = new QSerialPort(this);
  m_autoSendTimer = new QTimer(this);
  m_portCheckTimer = new QTimer(this);

  // Multi-send: init with 1 page of 20 empty items
  m_multiPage = 0;
  m_multiLoopIndex = 0;
  m_multiLoopTimer = new QTimer(this);
  m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));

  m_welcomeText = "   欢迎使用uSmilePro串口示波器V1.0   ";

  // Render Throttling Setup
  m_needsReplot = false;
  m_replotTimer = new QTimer(this);
  m_replotTimer->setInterval(33); // ~30 FPS limit for QCustomPlot

  setupUi();
  setupChart();
  setupConnections();
  refreshMultiPage(); // init multi-send page display
  refreshPorts();

  // Start timers
  m_portCheckTimer->start(1000);
  m_replotTimer->start();
}

SerialSession::~SerialSession() {
  if (m_serial->isOpen())
    m_serial->close();
}

void SerialSession::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_innerMainWindow = new QMainWindow(this);
  m_innerMainWindow->setWindowFlags(Qt::Widget);
  m_innerMainWindow->setDockOptions(QMainWindow::AnimatedDocks |
                                    QMainWindow::AllowNestedDocks);
  m_innerMainWindow->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);

  // 先创建 central widget，再添加 Dock，避免 Dock 被错误放置到顶部区域
  m_mainHorizSplitter = new QSplitter(Qt::Horizontal);
  m_innerMainWindow->setCentralWidget(m_mainHorizSplitter);

  // 左侧数据区（接收/发送）在一个垂直分割器里
  QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
  m_mainHorizSplitter->addWidget(rightSplitter);
  m_dataSplitter = rightSplitter;

  // Global Stylesheet for Custom Cards
  this->setStyleSheet("SerialPortPlot { background-color: #f5f5f5; }"
                      "QWidget#cardWidget { "
                      "    background-color: #FFFFFF; "
                      "    border: 1px solid #E0E0E0; "
                      "    border-radius: 12px; "
                      "}"
                      "QLabel#cardTitle { "
                      "    font-weight: bold; "
                      "    font-size: 13px; "
                      "    color: #333333; "
                      "    padding-bottom: 5px; "
                      "}"
                      "QTextEdit { "
                      "    border: 1px solid #4CAF50; "
                      "    border-radius: 10px; "
                      "    padding: 5px; "
                      "    background-color: #FAFAFA; "
                      "}");

  // 1. Port Settings
  QWidget *grpPort = new QWidget();
  grpPort->setObjectName("cardWidget");
  QVBoxLayout *portMainLayout = new QVBoxLayout(grpPort);
  portMainLayout->setContentsMargins(10, 10, 10, 10);

  QGridLayout *portLayout = new QGridLayout();
  portLayout->setContentsMargins(0, 0, 0, 0);
  portMainLayout->addLayout(portLayout);

  portLayout->addWidget(new QLabel("端口号:"), 0, 0);
  m_comboPort = new QComboBox();
  portLayout->addWidget(m_comboPort, 0, 1, 1, 2);

  portLayout->addWidget(new QLabel("波特率:"), 1, 0);
  m_comboBaud = new QComboBox();
  m_comboBaud->addItems(
      {"9600", "19200", "38400", "57600", "115200", "921600"});
  m_comboBaud->setCurrentText("115200");
  portLayout->addWidget(m_comboBaud, 1, 1, 1, 2);

  portLayout->addWidget(new QLabel("数据位:"), 2, 0);
  m_comboDataBits = new QComboBox();
  m_comboDataBits->addItems({"5", "6", "7", "8"});
  m_comboDataBits->setCurrentText("8");
  portLayout->addWidget(m_comboDataBits, 2, 1, 1, 2);

  portLayout->addWidget(new QLabel("校验位:"), 3, 0);
  m_comboParity = new QComboBox();
  m_comboParity->addItem("None", QSerialPort::NoParity);
  m_comboParity->addItem("Odd", QSerialPort::OddParity);
  m_comboParity->addItem("Even", QSerialPort::EvenParity);
  portLayout->addWidget(m_comboParity, 3, 1, 1, 2);

  portLayout->addWidget(new QLabel("停止位:"), 4, 0);
  m_comboStopBits = new QComboBox();
  m_comboStopBits->addItem("1", QSerialPort::OneStop);
  m_comboStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
  m_comboStopBits->addItem("2", QSerialPort::TwoStop);
  portLayout->addWidget(m_comboStopBits, 4, 1, 1, 2);

  QHBoxLayout *portActionLayout = new QHBoxLayout();

  // 刷新按钮：纯图标样式，无按钮边框
  m_btnRefresh = new QPushButton(QChar(0xE84D));
  m_btnRefresh->setFont(CIconFont::instance()->getIconFont(50));
  m_btnRefresh->setToolTip("刷新端口");
  m_btnRefresh->setMinimumWidth(52);
  m_btnRefresh->setMinimumHeight(44);
  m_btnRefresh->setFlat(true);
  m_btnRefresh->setStyleSheet(
      "QPushButton { background: transparent; border: none; border-radius: 8px;"
      "  color: #1565C0; padding: 4px; }"
      "QPushButton:hover { background: rgba(21,101,192,40); }"
      "QPushButton:pressed { background: rgba(21,101,192,80); }");

  // m_btnRefresh->setStyleSheet(
  //     "QPushButton { color: #555555; background: #E3F2FD; border: 1px solid "
  //     "#BBDEFB; border-radius: 16px; }"
  //     "QPushButton:hover { background: #BBDEFB; color: #1976D2; }"
  //     "QPushButton:checked { background: #C8E6C9; color: #388E3C; "
  //     "border-color: #A5D6A7; }");
  portActionLayout->addWidget(m_btnRefresh);

  // 打开串口按钮：纯图标样式，无按钮边框
  m_btnOpenClose = new QPushButton(QChar(0xE84E));
  m_btnOpenClose->setFont(CIconFont::instance()->getIconFont(50));
  m_btnOpenClose->setCheckable(true);
  m_btnOpenClose->setToolTip("打开串口");
  m_btnOpenClose->setFlat(true);
  m_btnOpenClose->setMinimumWidth(52);
  m_btnOpenClose->setMinimumHeight(44);
  m_btnOpenClose->setStyleSheet(
      "QPushButton { background: transparent; border: none; border-radius: 8px;"
      "  color: #2E7D32; padding: 4px; }"
      "QPushButton:hover { background: rgba(46,125,50,40); }"
      "QPushButton:checked { color: #C62828; }"
      "QPushButton:checked:hover { background: rgba(198,40,40,40); }"
      "QPushButton:pressed { background: rgba(46,125,50,80); }");

  // Status Icon Label
  m_lblStatusIcon = new QLabel();
  m_lblStatusIcon->setPixmap(
      QPixmap(":/icons/ONOFF/OFF5.png")
          .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_lblStatusIcon->setAlignment(Qt::AlignCenter);

  portActionLayout->addStretch();
  portActionLayout->addWidget(m_lblStatusIcon);
  portActionLayout->addStretch();
  portActionLayout->addWidget(m_btnOpenClose);
  portActionLayout->addStretch();

  // Add sub-layout to main grid at row 5, spanning 3 cols
  portLayout->addLayout(portActionLayout, 5, 0, 1, 3);

  m_dockPort = new QDockWidget("串口设置", m_innerMainWindow);
  m_dockPort->setObjectName("premiumDock");
  m_dockPort->setFeatures(QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable |
                          QDockWidget::DockWidgetClosable);
  m_dockPort->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockPort->setWidget(grpPort);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockPort);

  // Apply premium sci-fi theme to dock frame
  m_dockPort->setStyleSheet(R"(
    QDockWidget#premiumDock {
        border: 2px solid transparent;
        border-image: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 rgba(102, 126, 234, 0.1),
            stop:1 rgba(118, 75, 162, 0.1));
    }
    QDockWidget#premiumDock::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 rgba(102, 126, 234, 0.9),
            stop:0.5 rgba(118, 75, 162, 0.9),
            stop:1 rgba(102, 126, 234, 0.9));
        padding-top: 10px;
        padding-bottom: 6px;
        padding-left: 10px;
        font-weight: bold;
        font-size: 11pt;
        color: #FFFFFF;
        border: none;
        border-bottom: 2px solid rgba(255, 255, 255, 0.3);
        text-shadow: 0 0 10px rgba(102, 126, 234, 0.8);
    }
    QDockWidget#premiumDock::close-button {
        background: transparent;
        border: 1px solid rgba(255, 255, 255, 0.5);
        border-radius: 5px;
        padding: 3px;
        icon-size: 18px;
        subcontrol-position: top right;
        subcontrol-origin: margin;
        position: absolute;
        top: 6px;
        right: 8px;
        width: 26px;
        height: 26px;
    }
    QDockWidget#premiumDock::close-button:hover {
        background: rgba(244, 67, 54, 0.8);
        border: 1px solid #F44336;
    }
    QDockWidget#premiumDock::float-button {
        background: transparent;
        border: 1px solid rgba(255, 255, 255, 0.5);
        border-radius: 5px;
        padding: 3px;
        icon-size: 18px;
        subcontrol-position: top right;
        subcontrol-origin: margin;
        position: absolute;
        top: 6px;
        right: 38px;
        width: 26px;
        height: 26px;
    }
    QDockWidget#premiumDock::float-button:hover {
        background: rgba(33, 150, 243, 0.8);
        border: 1px solid #2196F3;
    }
    QWidget#cardWidget {
        background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 #FAFAFA,
            stop:1 #F0F0F5);
        border: none;
    }
  )");

  // 2. Receive Settings
  QWidget *grpRx = new QWidget();
  grpRx->setObjectName("cardWidget");
  QVBoxLayout *rxMainLayout = new QVBoxLayout(grpRx);
  rxMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *rxLayout = new QVBoxLayout();
  rxLayout->setContentsMargins(0, 0, 0, 0);
  rxMainLayout->addLayout(rxLayout);

  QHBoxLayout *rxModeLayout = new QHBoxLayout();
  m_rbRxAscii = new QRadioButton("ASCII");
  m_rbRxHex = new QRadioButton("HEX");
  m_rbRxAscii->setChecked(true);
  rxModeLayout->addWidget(m_rbRxAscii);
  rxModeLayout->addWidget(m_rbRxHex);
  rxLayout->addLayout(rxModeLayout);

  m_chkRxLog = new QCheckBox("日志模式");
  m_chkRxTime = new QCheckBox("显示时间");

  rxLayout->addWidget(m_chkRxLog);
  rxLayout->addWidget(m_chkRxTime);

  // Instantiate the logical buttons that are used by the Rx panel signals.
  // They are no longer added to the layout directly (replaced by floating
  // controls), but their logical state (isChecked, clicked) is required by
  // onReadyRead and toggles.
  m_btnStopRx = new QPushButton(this);
  m_btnStopRx->setCheckable(true);
  m_btnStopRx->setVisible(false);
  m_btnClearRx = new QPushButton(this);
  m_btnClearRx->setVisible(false);

  // Floating controls will replace the inline buttons later, but keeping for
  // now or remove if redundant

  m_dockRx = new QDockWidget("接收设置", m_innerMainWindow);
  m_dockRx->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);
  m_dockRx->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockRx->setWidget(grpRx);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockRx);
  m_dockRx->hide(); // Requirement 1: Hide by default

  // 3. Send Settings
  QWidget *grpTx = new QWidget();
  grpTx->setObjectName("cardWidget");
  QVBoxLayout *txMainLayout = new QVBoxLayout(grpTx);
  txMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *txLayout = new QVBoxLayout();
  txLayout->setContentsMargins(0, 0, 0, 0);
  txMainLayout->addLayout(txLayout);

  QHBoxLayout *txModeLayout = new QHBoxLayout();
  m_rbTxAscii = new QRadioButton("ASCII");
  m_rbTxHex = new QRadioButton("HEX");
  m_rbTxAscii->setChecked(true);
  txModeLayout->addWidget(m_rbTxAscii);
  txModeLayout->addWidget(m_rbTxHex);
  txLayout->addLayout(txModeLayout);

  // Dock-side controls are kept as mirrors of the floating controls below.
  QCheckBox *dockChkTxNewLine = new QCheckBox("发送新行");
  m_chkTxTime = new QCheckBox("显示时间"); // Timestamp in local log
  txLayout->addWidget(dockChkTxNewLine);
  txLayout->addWidget(m_chkTxTime);

  QCheckBox *dockChkAutoSend = new QCheckBox("自动发送(ms):");
  dockChkAutoSend->setEnabled(false); // Disabled until port is open
  QSpinBox *dockSpinAutoSendInterval = new QSpinBox();
  dockSpinAutoSendInterval->setRange(10, 10000);
  dockSpinAutoSendInterval->setValue(1000);
  dockSpinAutoSendInterval->setEnabled(false); // Disabled until port is open

  QHBoxLayout *autoSendLayout = new QHBoxLayout();
  autoSendLayout->addWidget(dockChkAutoSend);
  autoSendLayout->addWidget(dockSpinAutoSendInterval);
  txLayout->addLayout(autoSendLayout);

  m_dockTx = new QDockWidget("发送设置", m_innerMainWindow);
  m_dockTx->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);
  m_dockTx->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockTx->setWidget(grpTx);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockTx);
  m_dockTx->hide(); // Requirement 1: Hide by default

  // === 新增：示波器设置 Card ===
  QWidget *grpScope = new QWidget();
  grpScope->setObjectName("cardWidget");
  QVBoxLayout *grpScopeMainLayout = new QVBoxLayout(grpScope);
  grpScopeMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *grpScopeLayout = new QVBoxLayout();
  grpScopeLayout->setContentsMargins(0, 0, 0, 0);
  grpScopeMainLayout->addLayout(grpScopeLayout);
  grpScopeLayout->setSpacing(4);
  m_settingsLayout = grpScopeLayout;

  const QFont scopeIconFont = CIconFont::instance()->getIconFont(50);
  auto setupScopeIconCheck = [&scopeIconFont](QCheckBox *check,
                                               const QString &icon,
                                               const QString &tooltip) {
    check->setFont(scopeIconFont);
    check->setText(icon);
    check->setToolTip(tooltip);
    check->setCursor(Qt::PointingHandCursor);
    check->setStyleSheet(
        "QCheckBox {"
        "  background: transparent;"
        "  border: none;"
        "  color: #546E7A;"
        "  font-size: 50px;"
        "  padding: 0px;"
        "  margin: 0px;"
        "}"
        "QCheckBox:hover { color: #42A5F5; }"
        "QCheckBox:checked { color: #1976D2; }"
        "QCheckBox::indicator { width: 0px; height: 0px; }");
    check->setFixedSize(62, 62);
  };

  auto setupScopeIconButton = [&scopeIconFont](QPushButton *button,
                                                const QString &icon,
                                                const QString &tooltip,
                                                bool checkable) {
    button->setFont(scopeIconFont);
    button->setText(icon);
    button->setToolTip(tooltip);
    button->setCheckable(checkable);
    button->setCursor(Qt::PointingHandCursor);
    button->setFlat(true);
    button->setFixedSize(62, 62);
    button->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #546E7A;"
        "  font-size: 50px;"
        "  padding: 0px;"
        "  margin: 0px;"
        "}"
        "QPushButton:hover { color: #42A5F5; }"
        "QPushButton:pressed { color: #1E88E5; }"
        "QPushButton:checked { color: #1976D2; }");
  };

  m_chkEnableWaveform = new QCheckBox();
  setupScopeIconCheck(m_chkEnableWaveform, "\ue86b", "启用/关闭波形显示");

  m_chkHideRxTx = new QCheckBox();
  setupScopeIconCheck(m_chkHideRxTx, "\ue9db", "隐藏/显示收发区");

  m_chkHideRxData = new QCheckBox();
  setupScopeIconCheck(m_chkHideRxData, "\ue883", "不显示接收数据");

  m_chkShowRawData = new QCheckBox();
  setupScopeIconCheck(m_chkShowRawData, "\ue881", "显示原始数据");

  m_chkShowGrid = new QCheckBox();
  setupScopeIconCheck(m_chkShowGrid, "\ue866", "显示/隐藏网格");
  m_chkShowGrid->setChecked(true);

  m_btnAutoScale = new QPushButton();
  setupScopeIconButton(m_btnAutoScale, "\ue879", "自动缩放开关", true);
  m_btnAutoScale->setChecked(true);

  m_btnClearWaveform = new QPushButton();
  setupScopeIconButton(m_btnClearWaveform, "\ue604", "清空波形数据", false);

  m_btnResetChart = new QPushButton();
  setupScopeIconButton(m_btnResetChart, "\ue85d", "重置图表设置", false);

  m_btnStopWaveform = new QPushButton();
  setupScopeIconButton(m_btnStopWaveform, "\ue87c", "暂停/继续波形显示",
                       true);

  m_btnCurveSettings = new QPushButton();
  setupScopeIconButton(m_btnCurveSettings, "\ue872", "查看/修改曲线样式",
                       false);

  auto makeScopeItem = [](QWidget *iconWidget) -> QWidget * {
    QWidget *cell = new QWidget();
    QVBoxLayout *cellLayout = new QVBoxLayout(cell);
    cellLayout->setContentsMargins(0, 0, 0, 0);
    cellLayout->setSpacing(0);
    cellLayout->addStretch();
    cellLayout->addWidget(iconWidget, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    cellLayout->addStretch();
    return cell;
  };

  QGridLayout *scopeIconRow = new QGridLayout();
  scopeIconRow->setContentsMargins(0, 0, 0, 0);
  scopeIconRow->setHorizontalSpacing(8);
  scopeIconRow->setVerticalSpacing(0);
  scopeIconRow->addWidget(makeScopeItem(m_chkEnableWaveform), 0, 0);
  scopeIconRow->addWidget(makeScopeItem(m_chkShowGrid), 0, 1);
  scopeIconRow->addWidget(makeScopeItem(m_btnAutoScale), 0, 2);
  scopeIconRow->addWidget(makeScopeItem(m_btnClearWaveform), 0, 3);
  scopeIconRow->addWidget(makeScopeItem(m_btnResetChart), 0, 4);
  scopeIconRow->addWidget(makeScopeItem(m_btnStopWaveform), 0, 5);
  scopeIconRow->addWidget(makeScopeItem(m_btnCurveSettings), 0, 6);
  for (int c = 0; c < 7; ++c) {
    scopeIconRow->setColumnStretch(c, 1);
  }
  grpScopeLayout->addLayout(scopeIconRow);

  QGridLayout *scopeFlagsLayout = new QGridLayout();
  scopeFlagsLayout->setContentsMargins(0, 6, 0, 0);
  scopeFlagsLayout->setHorizontalSpacing(8);
  scopeFlagsLayout->setVerticalSpacing(0);
  scopeFlagsLayout->addWidget(makeScopeItem(m_chkHideRxTx), 0, 0);
  scopeFlagsLayout->addWidget(makeScopeItem(m_chkHideRxData), 0, 1);
  scopeFlagsLayout->addWidget(makeScopeItem(m_chkShowRawData), 0, 2);
  scopeFlagsLayout->addWidget(new QWidget(), 0, 3); // spacer
  for (int c = 0; c < 4; ++c) {
    scopeFlagsLayout->setColumnStretch(c, 1);
  }
  grpScopeLayout->addLayout(scopeFlagsLayout);

  // 1. Group Box for Plot Parameters (Window width, buffer, theme)
  m_groupPlotParams = new QGroupBox();
  QVBoxLayout *paramsLayout = new QVBoxLayout(m_groupPlotParams);
  paramsLayout->setContentsMargins(4, 8, 4, 4);
  paramsLayout->setSpacing(4);

  paramsLayout->addWidget(new QLabel("视窗宽度(∆t):"));
  m_spinPoints = new QSpinBox();
  m_spinPoints->setRange(10, 100000);
  m_spinPoints->setValue(100);
  paramsLayout->addWidget(m_spinPoints);

  paramsLayout->addWidget(new QLabel("缓冲区上限:"));
  m_spinBufferLimit = new QSpinBox();
  m_spinBufferLimit->setRange(100, 1000000);
  m_spinBufferLimit->setValue(10000);
  paramsLayout->addWidget(m_spinBufferLimit);

  paramsLayout->addWidget(new QLabel("X轴标签单位:"));
  m_comboTimeUnit = new QComboBox();
  m_comboTimeUnit->addItems({"点数 (Points)", "毫秒 (ms)", "秒 (s)"});
  paramsLayout->addWidget(m_comboTimeUnit);

  paramsLayout->addWidget(new QLabel("波形主题:"));
  m_comboChartTheme = new QComboBox();
  m_comboChartTheme->addItems({"亮色主题", "暗黑炫光", "科幻示波器"});
  m_comboChartTheme->setCurrentIndex(1);
  paramsLayout->addWidget(m_comboChartTheme);

  m_groupPlotParams->setFlat(true);
  m_groupPlotParams->setStyleSheet(
      "QGroupBox { border: none; margin-top: 0px; font-weight: 600; color: "
      "#455A64; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 0px; top: 0px; "
      "padding: 0px; }");
  grpScopeLayout->addWidget(m_groupPlotParams);

  // 2. Group Box for Y-Axis control
  m_groupYAxis = new QGroupBox();
  QGridLayout *yLayout = new QGridLayout(m_groupYAxis);
  yLayout->setContentsMargins(4, 8, 4, 4);
  yLayout->setHorizontalSpacing(8);
  yLayout->setVerticalSpacing(6);

  yLayout->addWidget(new QLabel("最小值:"), 0, 0);
  m_spinYMin = new QDoubleSpinBox();
  m_spinYMin->setRange(-99999, 99999);
  m_spinYMin->setEnabled(false);
  yLayout->addWidget(m_spinYMin, 0, 1);

  yLayout->addWidget(new QLabel("最大值:"), 1, 0);
  m_spinYMax = new QDoubleSpinBox();
  m_spinYMax->setRange(-99999, 99999);
  m_spinYMax->setEnabled(false);
  yLayout->addWidget(m_spinYMax, 1, 1);

  yLayout->addWidget(new QLabel("刻度:"), 2, 0);
  m_spinYTick = new QDoubleSpinBox();
  m_spinYTick->setRange(0, 99999);
  m_spinYTick->setEnabled(false);
  yLayout->addWidget(m_spinYTick, 2, 1);

  m_groupYAxis->setFlat(true);
  m_groupYAxis->setStyleSheet(
      "QGroupBox { border: none; margin-top: 0px; font-weight: 600; color: "
      "#455A64; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 0px; top: 0px; "
      "padding: 0px; }");
  grpScopeLayout->addWidget(m_groupYAxis);

  QGroupBox *grpChannels = new QGroupBox("通道管理");
  m_channelsLayout = new QVBoxLayout(grpChannels);
  m_channelsLayout->setContentsMargins(4, 4, 4, 4);
  m_channelsLayout->addStretch();
  grpScopeLayout->addWidget(grpChannels);

  grpScopeLayout->addStretch();

  m_dockScopeSettings = new QDockWidget("示波器设置", m_innerMainWindow);
  m_dockScopeSettings->setObjectName("premiumDock");
  m_dockScopeSettings->setFeatures(QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetFloatable |
                                   QDockWidget::DockWidgetClosable);
  m_dockScopeSettings->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_dockScopeSettings->setWidget(grpScope);
  m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockScopeSettings);

  // Style scope settings dock with sci-fi theme
  m_dockScopeSettings->setStyleSheet(R"(
    QDockWidget#premiumDock {
        border: 2px solid transparent;
        border-image: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 rgba(17, 153, 142, 0.1),
            stop:1 rgba(56, 239, 125, 0.1));
    }
    QDockWidget#premiumDock::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 rgba(17, 153, 142, 0.9),
            stop:0.5 rgba(56, 239, 125, 0.9),
            stop:1 rgba(17, 153, 142, 0.9));
        padding-top: 10px;
        padding-bottom: 6px;
        padding-left: 10px;
        font-weight: bold;
        font-size: 11pt;
        color: #FFFFFF;
        border: none;
        border-bottom: 2px solid rgba(255, 255, 255, 0.3);
        text-shadow: 0 0 10px rgba(56, 239, 125, 0.8);
    }
    QDockWidget#premiumDock::close-button {
        background: transparent;
        border: 1px solid rgba(255, 255, 255, 0.5);
        border-radius: 5px;
        padding: 3px;
        icon-size: 18px;
        subcontrol-position: top right;
        subcontrol-origin: margin;
        position: absolute;
        top: 6px;
        right: 8px;
        width: 26px;
        height: 26px;
    }
    QDockWidget#premiumDock::close-button:hover {
        background: rgba(244, 67, 54, 0.8);
        border: 1px solid #F44336;
    }
    QDockWidget#premiumDock::float-button {
        background: transparent;
        border: 1px solid rgba(255, 255, 255, 0.5);
        border-radius: 5px;
        padding: 3px;
        icon-size: 18px;
        subcontrol-position: top right;
        subcontrol-origin: margin;
        position: absolute;
        top: 6px;
        right: 38px;
        width: 26px;
        height: 26px;
    }
    QDockWidget#premiumDock::float-button:hover {
        background: rgba(33, 150, 243, 0.8);
        border: 1px solid #2196F3;
    }
  )");

  m_lblRxCount = new QLabel("0");
  m_lblTxCount = new QLabel("0");

  // --- View Toolbar removed as requested by user, merged into main window ---

  // 强制将示波器设置停靠在串口设置下方，避免跑到顶部横条遮挡内容
  auto ensureLeftDockArea = [this](QDockWidget *dock) {
    if (!dock)
      return;
    if (m_innerMainWindow->dockWidgetArea(dock) == Qt::LeftDockWidgetArea)
      return;

    const bool wasVisible = dock->isVisible();
    m_innerMainWindow->removeDockWidget(dock);
    m_innerMainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->setVisible(wasVisible);
  };
  ensureLeftDockArea(m_dockPort);
  ensureLeftDockArea(m_dockRx);
  ensureLeftDockArea(m_dockTx);
  ensureLeftDockArea(m_dockScopeSettings);

  m_innerMainWindow->splitDockWidget(m_dockPort, m_dockScopeSettings,
                                     Qt::Vertical);

  // Receive Area
  QWidget *grpData = new QWidget();
  grpData->setObjectName("cardWidget");
  QVBoxLayout *dataMainLayout = new QVBoxLayout(grpData);
  dataMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *dataLayout = new QVBoxLayout();
  dataLayout->setContentsMargins(0, 0, 0, 0);
  dataMainLayout->addLayout(dataLayout);

  // To make the buttons float over the text area securely
  QWidget *rxContainer = new QWidget();
  QVBoxLayout *rxContainerLayout = new QVBoxLayout(rxContainer);
  rxContainerLayout->setContentsMargins(0, 0, 0, 0);

  m_textReceive = new QTextEdit();
  // Prevent unbounded growth that causes permanent UI freeze on rapid
  // auto-sends
  m_textReceive->document()->setMaximumBlockCount(1000);
  m_textReceive->setReadOnly(true);
  m_textReceive->installEventFilter(this);

  // Floating container
  QWidget *rxFloatWidget = new QWidget(m_textReceive);
  rxFloatWidget->setObjectName("rxFloat");
  rxFloatWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  rxFloatWidget->setStyleSheet("background: transparent; border: none;");
  rxFloatWidget->raise(); // 确保浮动控件在最上层

  QHBoxLayout *rxFloatLayout = new QHBoxLayout(rxFloatWidget);
  rxFloatLayout->setContentsMargins(0, 0, 0, 0);
  rxFloatLayout->setSpacing(8);

  auto createFloatBtn = [](QChar iconCode) {
    QToolButton *btn = new QToolButton();
    btn->setFont(CIconFont::instance()->getIconFont(25));
    btn->setText(iconCode);
    btn->setFixedSize(32, 32);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    // Give it a solid background so it masks text underneath
    btn->setStyleSheet(
        "QToolButton { color: #555555; background: #E3F2FD; border: 1px solid "
        "#BBDEFB; border-radius: 16px; }"
        "QToolButton:hover { background: #BBDEFB; color: #1976D2; }"
        "QToolButton:checked { background: #C8E6C9; color: #388E3C; "
        "border-color: #A5D6A7; }");
    return btn;
  };

  m_btnRxHexToggle = createFloatBtn(QChar(0xEBBC));
  m_btnRxTimeToggle = createFloatBtn(QChar(0xE676));
  m_btnRxPauseToggle = createFloatBtn(QChar(0xE617));
  m_btnRxClear = createFloatBtn(QChar(0xE621));
  m_btnRxClear->setCheckable(false);

  rxFloatLayout->addWidget(m_btnRxHexToggle);
  rxFloatLayout->addWidget(m_btnRxTimeToggle);
  rxFloatLayout->addWidget(m_btnRxPauseToggle);
  rxFloatLayout->addWidget(m_btnRxClear);

  // 初始化浮动控件位置（避免显示在左上角）
  rxFloatWidget->adjustSize();
  rxFloatWidget->move(
      qMax(4, m_textReceive->width() - rxFloatWidget->width() - 10),
      qMax(4, m_textReceive->height() - rxFloatWidget->height() - 10));

  rxContainerLayout->addWidget(m_textReceive);

  dataLayout->addWidget(rxContainer);

  rightSplitter->addWidget(grpData);

  // --- Send Area Container ---
  QWidget *grpSend = new QWidget();
  grpSend->setObjectName("cardWidget");
  QVBoxLayout *grpSendMainLayout = new QVBoxLayout(grpSend);
  grpSendMainLayout->setContentsMargins(10, 10, 10, 10);

  QVBoxLayout *grpSendLayout = new QVBoxLayout();
  grpSendLayout->setContentsMargins(0, 0, 0, 0);
  grpSendMainLayout->addLayout(grpSendLayout);

  // --- Send Area: 4-tab QTabWidget ---
  m_sendTabWidget = new QTabWidget();
  m_sendTabWidget->setDocumentMode(false);
  // 消除标签页面板左上角遮挡条框
  m_sendTabWidget->setStyleSheet(R"(
    QTabWidget::pane {
        border: 1px solid #C8C8C8;
        border-top: none;
        background: transparent;
    }
    QTabWidget::left-corner {
        background: transparent;
        border: none;
        width: 0px;
        height: 0px;
    }
    QTabWidget::right-corner {
        background: transparent;
        border: none;
        width: 0px;
        height: 0px;
    }
    QTabWidget::tab-bar { left: 0px; }
    QTabBar::tab {
        background: #F0F0F0;
        border: 1px solid #C8C8C8;
        border-bottom: none;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
        min-width: 70px;
        padding: 4px 12px;
        margin-right: 1px;
    }
    QTabBar::tab:selected {
        background: #FFFFFF;
        font-weight: bold;
        color: #1565C0;
    }
    QTabBar::tab:hover:!selected { background: #E8EDF5; }
    QTabBar::tab:first { margin-left: 0px; }
  )");

  // ========== Tab 0: 单条发送 ==========
  QWidget *tabSingle = new QWidget();
  QVBoxLayout *singleLayout = new QVBoxLayout(tabSingle);
  singleLayout->setContentsMargins(6, 6, 6, 6);

  // We need the Tx icons floating ABOVE the Send text box.
  QWidget *txContainer = new QWidget();
  QVBoxLayout *txContainerLayout = new QVBoxLayout(txContainer);
  txContainerLayout->setContentsMargins(0, 0, 0, 0);

  m_textSend = new QTextEdit();
  m_textSend->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_textSend->setPlaceholderText("输入要发送的数据...");
  m_textSend->installEventFilter(this);

  // Floating widget for right corner (icons)
  QWidget *txFloatRightWidget = new QWidget(m_textSend);
  txFloatRightWidget->setObjectName("txFloatRight");
  txFloatRightWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  txFloatRightWidget->setStyleSheet("background: transparent; border: none;");
  txFloatRightWidget->raise(); // 确保浮动控件在最上层
  QHBoxLayout *txFloatRightLayout = new QHBoxLayout(txFloatRightWidget);
  txFloatRightLayout->setContentsMargins(0, 0, 0, 0);
  txFloatRightLayout->setSpacing(10);

  m_btnSend = createFloatBtn(QChar(0xE651));
  m_btnSend->setCheckable(false);
  m_btnTxHexToggle = createFloatBtn(QChar(0xEBBC));
  m_btnTxClear = createFloatBtn(QChar(0xE621));
  m_btnTxClear->setCheckable(false);

  // 悬浮在右下角
  txFloatRightLayout->addWidget(m_btnTxHexToggle);
  txFloatRightLayout->addWidget(m_btnSend);
  txFloatRightLayout->addWidget(m_btnTxClear);

  // Floating widget for left corner (Auto send, new line)
  QWidget *txFloatLeftWidget = new QWidget(m_textSend);
  txFloatLeftWidget->setObjectName("txFloatLeft");
  txFloatLeftWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  txFloatLeftWidget->setStyleSheet("background: transparent; border: none;");
  txFloatLeftWidget->raise(); // 确保浮动控件在最上层
  QHBoxLayout *txFloatLeftLayout = new QHBoxLayout(txFloatLeftWidget);
  txFloatLeftLayout->setContentsMargins(6, 3, 6, 3);
  txFloatLeftLayout->setSpacing(8);

  m_chkAutoSend = new QCheckBox("自动发送");
  m_chkAutoSend->setEnabled(false);
  m_chkAutoSend->setStyleSheet(
      "QCheckBox { color: #263238; font-weight: 600; spacing: 6px; }"
      "QCheckBox:disabled { color: #546E7A; }"
      "QCheckBox::indicator { width: 0px; height: 0px; }");
  m_spinAutoSendInterval = new QSpinBox();
  m_spinAutoSendInterval->setRange(10, 10000);
  m_spinAutoSendInterval->setValue(1000);
  m_spinAutoSendInterval->setSuffix(" ms");
  m_spinAutoSendInterval->setEnabled(false);
  m_spinAutoSendInterval->setStyleSheet(
      "QSpinBox { color: #263238; background: rgba(255,255,255,240);"
      " border: 1px solid #90A4AE; border-radius: 4px; padding: 0 4px; }"
      "QSpinBox:disabled { color: #546E7A; }");
  m_spinAutoSendInterval->hide(); // Only show when checked

  m_chkTxNewLine = new QCheckBox("发送新行");
  m_chkTxNewLine->setStyleSheet(
      "QCheckBox { color: #263238; font-weight: 600; spacing: 6px; }"
      "QCheckBox::indicator { width: 0px; height: 0px; }");

  // Keep dock-side and floating controls in sync without overriding members.
  connect(dockChkTxNewLine, &QCheckBox::toggled, m_chkTxNewLine,
          &QCheckBox::setChecked);
  connect(m_chkTxNewLine, &QCheckBox::toggled, dockChkTxNewLine,
          &QCheckBox::setChecked);
  connect(dockChkAutoSend, &QCheckBox::toggled, m_chkAutoSend,
          &QCheckBox::setChecked);
  connect(m_chkAutoSend, &QCheckBox::toggled, dockChkAutoSend,
          &QCheckBox::setChecked);
  connect(dockSpinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          m_spinAutoSendInterval, &QSpinBox::setValue);
  connect(m_spinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          dockSpinAutoSendInterval, &QSpinBox::setValue);
  connect(m_btnOpenClose, &QPushButton::toggled, dockChkAutoSend,
          &QCheckBox::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, dockSpinAutoSendInterval,
          &QSpinBox::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, m_chkAutoSend,
          &QCheckBox::setEnabled);
  connect(m_btnOpenClose, &QPushButton::toggled, m_spinAutoSendInterval,
          &QSpinBox::setEnabled);

  txFloatLeftLayout->addWidget(m_chkAutoSend);
  txFloatLeftLayout->addWidget(m_spinAutoSendInterval);
  txFloatLeftLayout->addWidget(m_chkTxNewLine);

  // 初始化浮动控件位置（避免显示在左上角）
  txFloatRightWidget->adjustSize();
  txFloatRightWidget->move(
      qMax(4, m_textSend->width() - txFloatRightWidget->width() - 10),
      qMax(4, m_textSend->height() - txFloatRightWidget->height() - 10));
  txFloatLeftWidget->adjustSize();
  txFloatLeftWidget->move(
      10, qMax(4, m_textSend->height() - txFloatLeftWidget->height() - 10));

  connect(m_chkAutoSend, &QCheckBox::toggled, this, [this](bool checked) {
    m_spinAutoSendInterval->setVisible(checked);
    if (!m_textSend)
      return;

    QWidget *txFloatLeft = m_textSend->findChild<QWidget *>("txFloatLeft");
    if (txFloatLeft) {
      txFloatLeft->adjustSize();
      txFloatLeft->move(
          10, qMax(4, m_textSend->height() - txFloatLeft->height() - 10));
    }
  });

  txContainerLayout->addWidget(m_textSend);

  singleLayout->addWidget(txContainer);

  // Bottom Area (under text box) Layout
  QGridLayout *txBottomLayout = new QGridLayout();
  txBottomLayout->setSpacing(8);

  m_comboHistory = new QComboBox();
  m_comboHistory->setEditable(false);
  m_comboHistory->setMinimumWidth(150);

  // Row 1 of Bottom: History (History ComboBox)
  QHBoxLayout *txHistory = new QHBoxLayout();
  txHistory->addWidget(new QLabel("历史记录:"));
  txHistory->addWidget(m_comboHistory);
  txHistory->addStretch();

  txBottomLayout->addLayout(txHistory, 0, 0);

  singleLayout->addLayout(txBottomLayout);

  m_sendTabWidget->addTab(tabSingle, "单条发送");

  // ========== Tab 1: 多条发送 ==========
  QWidget *tabMulti = new QWidget();
  QVBoxLayout *multiLayout = new QVBoxLayout(tabMulti);
  multiLayout->setContentsMargins(4, 4, 4, 4);
  multiLayout->setSpacing(3);

  // 双列 x 10行 网格 (共20条)
  QGridLayout *multiGrid = new QGridLayout();
  multiGrid->setSpacing(2);
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    int col = i / MULTI_ROWS;
    int row = i % MULTI_ROWS;
    m_chkMultiItem[i] = new QCheckBox();
    m_chkMultiItem[i]->setFixedWidth(20);
    m_leMultiItem[i] = new QLineEdit();
    m_leMultiItem[i]->setPlaceholderText(QString("条目 %1").arg(i + 1));
    m_leMultiItem[i]->setMinimumWidth(80);
    QHBoxLayout *cell = new QHBoxLayout();
    cell->setContentsMargins(0, 0, 0, 0);
    cell->setSpacing(2);
    cell->addWidget(m_chkMultiItem[i]);
    cell->addWidget(m_leMultiItem[i]);
    QWidget *cellW = new QWidget();
    cellW->setLayout(cell);
    multiGrid->addWidget(cellW, row, col);
  }
  multiLayout->addLayout(multiGrid);

  // ================= 底部工具栏布局 =================
  QVBoxLayout *bottomLayout = new QVBoxLayout();
  bottomLayout->setContentsMargins(0, 4, 0, 0);
  bottomLayout->setSpacing(6);

  // 第一排：选项 与 导入导出/全选
  QHBoxLayout *row1 = new QHBoxLayout();
  m_chkMultiNewLine = new QCheckBox("发送新行");
  m_chkMultiHex = new QCheckBox("16进制");
  m_chkMultiLoop = new QCheckBox("自动循环(ms):");
  m_spinMultiLoopInterval = new QSpinBox();
  m_spinMultiLoopInterval->setRange(10, 60000);
  m_spinMultiLoopInterval->setValue(1000);
  m_spinMultiLoopInterval->setEnabled(false);

  row1->addWidget(m_chkMultiNewLine);
  row1->addWidget(m_chkMultiHex);
  row1->addWidget(m_chkMultiLoop);
  row1->addWidget(m_spinMultiLoopInterval);
  row1->addStretch();

  QPushButton *btnSelAll = new QPushButton("全选");
  QPushButton *btnDeselAll = new QPushButton("全不选");
  m_btnMultiImport = new QPushButton("导入");
  m_btnMultiExport = new QPushButton("导出");
  for (auto *b : {btnSelAll, btnDeselAll, m_btnMultiImport, m_btnMultiExport}) {
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
    b->setMinimumHeight(28);
    row1->addWidget(b);
  }
  bottomLayout->addLayout(row1);

  // 第二排：分页导航 与 发送按钮
  QHBoxLayout *row2 = new QHBoxLayout();
  m_btnMultiFirst = new QPushButton("|<");
  m_btnMultiPrev = new QPushButton("<");
  m_btnMultiNext = new QPushButton(">");
  m_btnMultiLast = new QPushButton(">|");
  for (auto *b :
       {m_btnMultiFirst, m_btnMultiPrev, m_btnMultiNext, m_btnMultiLast}) {
    b->setFixedWidth(32);
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
  }
  m_lblMultiPage = new QLabel("第 1 / 1 页");
  m_lblMultiPage->setAlignment(Qt::AlignCenter);

  m_btnMultiAddPage = new QPushButton("+页");
  m_btnMultiDelPage = new QPushButton("-页");
  for (auto *b : {m_btnMultiAddPage, m_btnMultiDelPage}) {
    b->setFixedWidth(40);
    b->setStyleSheet(getButtonStyle(ButtonType::Normal));
  }

  m_spinJumpPage = new QSpinBox();
  m_spinJumpPage->setRange(1, 1);
  m_spinJumpPage->setPrefix("页码: ");
  m_spinJumpPage->setFixedWidth(80);
  QPushButton *btnJump = new QPushButton("GO");
  btnJump->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnJump->setFixedWidth(36);

  row2->addWidget(m_btnMultiFirst);
  row2->addWidget(m_btnMultiPrev);
  row2->addWidget(m_lblMultiPage);
  row2->addWidget(m_btnMultiNext);
  row2->addWidget(m_btnMultiLast);
  row2->addSpacing(10);
  row2->addWidget(m_btnMultiAddPage);
  row2->addWidget(m_btnMultiDelPage);
  row2->addSpacing(10);
  row2->addWidget(m_spinJumpPage);
  row2->addWidget(btnJump);
  row2->addStretch();

  // 突出发送选中按钮
  m_btnSendAll = new QPushButton("发送选中");
  QString sendStyle = getButtonStyle(ButtonType::Normal);
  // 可选：在基础样式上加一点微调让发送按钮更显眼
  sendStyle += "font-weight: bold;";
  m_btnSendAll->setStyleSheet(sendStyle);
  m_btnSendAll->setMinimumHeight(32);
  m_btnSendAll->setMinimumWidth(100);
  row2->addWidget(m_btnSendAll);

  bottomLayout->addLayout(row2);
  multiLayout->addLayout(bottomLayout);

  connect(btnSelAll, &QPushButton::clicked, [this]() {
    for (int i = 0; i < MULTI_PER_PAGE; i++)
      m_chkMultiItem[i]->setChecked(true);
  });
  connect(btnDeselAll, &QPushButton::clicked, [this]() {
    for (int i = 0; i < MULTI_PER_PAGE; i++)
      m_chkMultiItem[i]->setChecked(false);
  });
  connect(btnJump, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_spinJumpPage->value() - 1); });

  m_sendTabWidget->addTab(tabMulti, "多条发送");

  // ========== Tab 2: 协议传送 ==========
  QWidget *tabProtocol = new QWidget();
  QVBoxLayout *protoLayout = new QVBoxLayout(tabProtocol);
  protoLayout->setContentsMargins(6, 6, 6, 6);
  QGridLayout *protoGrid = new QGridLayout();
  protoGrid->addWidget(new QLabel("协议名:"), 0, 0);
  protoGrid->addWidget(new QComboBox(), 0, 1);
  protoGrid->addWidget(new QLabel("起始帧:"), 1, 0);
  QLineEdit *leStartFrame = new QLineEdit("AA");
  protoGrid->addWidget(leStartFrame, 1, 1);
  protoGrid->addWidget(new QLabel("数据域:"), 2, 0);
  QLineEdit *leData = new QLineEdit();
  leData->setPlaceholderText("Hex 数据...");
  protoGrid->addWidget(leData, 2, 1);
  protoGrid->addWidget(new QLabel("校验:"), 3, 0);
  QComboBox *cbChecksum = new QComboBox();
  cbChecksum->addItems({"无", "Sum8", "CRC8", "CRC16"});
  protoGrid->addWidget(cbChecksum, 3, 1);
  protoLayout->addLayout(protoGrid);
  QPushButton *btnProtoSend = new QPushButton("发送协议帧");
  btnProtoSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnProtoSend->setMinimumHeight(36);
  protoLayout->addWidget(btnProtoSend);
  protoLayout->addStretch();
  m_sendTabWidget->addTab(tabProtocol, "协议传送");

  // ========== Tab 3: 自定义发送 ==========
  QWidget *tabCustom = new QWidget();
  QVBoxLayout *customLayout = new QVBoxLayout(tabCustom);
  customLayout->setContentsMargins(6, 6, 6, 6);
  QPlainTextEdit *editScript = new QPlainTextEdit();
  editScript->setPlaceholderText(
      "// 自定义发送脚本（预留扩展）\n// 例如：循环发送、条件发送等");
  customLayout->addWidget(editScript);
  QPushButton *btnRunScript = new QPushButton("执行脚本（开发中）");
  btnRunScript->setStyleSheet(getButtonStyle(ButtonType::Normal));
  btnRunScript->setEnabled(false);
  customLayout->addWidget(btnRunScript);
  m_sendTabWidget->addTab(tabCustom, "自定义发送");

  grpSendLayout->addWidget(m_sendTabWidget);

  // Instead of rightSplitter->addWidget(grpSend), we add to rightSplitter
  // directly which is already set as CentralWidget
  rightSplitter->addWidget(grpSend);

  // Set rightSplitter vertical stretch factor so send area is larger
  rightSplitter->setStretchFactor(0, 3); // Receive Area
  rightSplitter->setStretchFactor(1, 2); // Send Area (made taller)

  // Lastly, add our main internal window to the actual top-level layout
  mainLayout->addWidget(m_innerMainWindow);

  // --- Extended Waveform Page ---
  // We use a QMainWindow to easily support Dock Widgets
  m_waveformPage = new QMainWindow();
  m_waveformPage->setWindowFlags(Qt::Widget); // Embeddable
  m_waveformPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // -- Chart and Scrollbar Container (Central Widget) --
  QWidget *chartContainer = new QWidget();
  QVBoxLayout *chartLayout = new QVBoxLayout(chartContainer);
  chartLayout->setContentsMargins(0, 0, 0, 0);
  chartLayout->setSpacing(0);

  m_customPlot = new QCustomPlot();
  m_scrollbarWaveform = new QScrollBar(Qt::Horizontal);
  m_scrollbarWaveform->setMinimum(0);
  m_scrollbarWaveform->setMaximum(0);

  // Modern Glowing Sci-Fi Scrollbar QSS
  QString scrollbarStyle = R"(
    QScrollBar:horizontal {
        background: transparent;
        height: 12px;
        margin: 2px 0 2px 0;
    }
    QScrollBar::handle:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 212, 255, 180), stop:1 rgba(9, 9, 121, 180));
        border-radius: 4px;
        min-width: 40px;
    }
    QScrollBar::handle:horizontal:hover {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 255, 255, 255), stop:1 rgba(0, 150, 255, 255));
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0px;
        background: none;
    }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
        background: rgba(30, 30, 40, 100);
        border-radius: 4px;
    }
  )";
  m_scrollbarWaveform->setStyleSheet(scrollbarStyle);

  chartLayout->addWidget(m_customPlot, 1);
  chartLayout->addWidget(m_scrollbarWaveform, 0);

  m_waveformPage->setCentralWidget(chartContainer);

  // m_dockSettings merged into m_dockScopeSettings above.

  m_mainHorizSplitter->addWidget(
      m_waveformPage); // Append to main layout vertically

  // Hide initially
  m_waveformPage->setVisible(false); // Give waveform plenty of space

  // --- Layout Integration ---
  // Directly add content to Tab Widget without Navigation Bar

  // 移除了内部多余的 m_mainTabWidget，直接将主分割器添加到布局中

  // --- Status Bar ---
  QHBoxLayout *statusBarLayout = new QHBoxLayout();
  statusBarLayout->setContentsMargins(5, 2, 5, 2);

  // Status Icon/Text
  m_lblPortInfo = new QLabel("串口关闭");
  m_lblPortInfo->setStyleSheet("color: red; font-weight: bold;");

  // Scrolling Message - wider label for smooth scrolling
  m_lblWelcome = new ScrollingLabel();
  m_lblWelcome->setStyleSheet("color: blue; font-style: italic;");
  // m_lblWelcome->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // Not
  // needed for custom widget
  m_lblWelcome->setMinimumWidth(400); // Fixed width for scrolling area
  m_lblWelcome->setText(m_welcomeText);

  statusBarLayout->addWidget(m_lblPortInfo);
  statusBarLayout->addStretch();

  // RX/TX Statistics
  QLabel *lblRxText = new QLabel("RX:");
  lblRxText->setStyleSheet("font-weight: bold; color: #555;");
  statusBarLayout->addWidget(lblRxText);
  statusBarLayout->addWidget(m_lblRxCount);
  statusBarLayout->addSpacing(15);

  QLabel *lblTxText = new QLabel("TX:");
  lblTxText->setStyleSheet("font-weight: bold; color: #555;");
  statusBarLayout->addWidget(lblTxText);
  statusBarLayout->addWidget(m_lblTxCount);
  statusBarLayout->addSpacing(20);

  statusBarLayout->addWidget(m_lblWelcome);

  mainLayout->addLayout(statusBarLayout);
}

void SerialSession::setupChart() {
  // QCustomPlot Setup
  m_customPlot->xAxis->setLabel("Time");
  m_customPlot->yAxis->setLabel("Value");

  m_customPlot->legend->setVisible(true);

  // Context Menu Policy
  m_customPlot->setContextMenuPolicy(Qt::CustomContextMenu);

  // Interactions: Scroll and Zoom
  m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                                QCP::iSelectLegend | QCP::iSelectPlottables);

  // Performance Optimizations for Large Data
  m_customPlot->setNoAntialiasingOnDrag(
      true); // Disable AA during heavy interactions
  m_customPlot->setAntialiasedElements(QCP::aePlottables | QCP::aeAxes |
                                       QCP::aeGrid);

  // Initial Range
  m_customPlot->xAxis->setRange(0, 100);
  m_customPlot->yAxis->setRange(0, 255);

  m_customPlot->setVisible(true);

  // High-Tech Neon Styling
  // 1. Dark glowing gradient background
  QLinearGradient plotGradient;
  plotGradient.setStart(0, 0);
  plotGradient.setFinalStop(0, 350);
  plotGradient.setColorAt(0, QColor("#1E1E28"));
  plotGradient.setColorAt(1, QColor("#282836"));
  m_customPlot->setBackground(plotGradient);

  // Set axis rectangle background to slightly darker translucent
  m_customPlot->axisRect()->setBackground(QColor(10, 10, 15, 180));

  // 2. Glowing cyan/blue axes
  QPen axisPen(QColor("#00E5FF"), 2);
  m_customPlot->xAxis->setBasePen(axisPen);
  m_customPlot->yAxis->setBasePen(axisPen);
  m_customPlot->xAxis->setTickPen(axisPen);
  m_customPlot->yAxis->setTickPen(axisPen);
  m_customPlot->xAxis->setSubTickPen(QPen(QColor("#00B0FF"), 1));
  m_customPlot->yAxis->setSubTickPen(QPen(QColor("#00B0FF"), 1));

  // Axis Labels and Tick Labels
  m_customPlot->xAxis->setTickLabelColor(QColor("#E0E0FF"));
  m_customPlot->yAxis->setTickLabelColor(QColor("#E0E0FF"));
  m_customPlot->xAxis->setLabelColor(QColor("#00E5FF"));
  m_customPlot->yAxis->setLabelColor(QColor("#00E5FF"));

  QFont tickFont = font();
  tickFont.setPointSize(9);
  m_customPlot->xAxis->setTickLabelFont(tickFont);
  m_customPlot->yAxis->setTickLabelFont(tickFont);

  QFont labelFont = font();
  labelFont.setPointSize(11);
  labelFont.setBold(true);
  m_customPlot->xAxis->setLabelFont(labelFont);
  m_customPlot->yAxis->setLabelFont(labelFont);

  // 3. Subtle Dashed Grids (Dark Cyan)
  m_customPlot->xAxis->grid()->setPen(
      QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine));
  m_customPlot->yAxis->grid()->setPen(
      QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine));
  m_customPlot->xAxis->grid()->setSubGridVisible(true);
  m_customPlot->yAxis->grid()->setSubGridVisible(true);
  m_customPlot->xAxis->grid()->setSubGridPen(
      QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine));
  m_customPlot->yAxis->grid()->setSubGridPen(
      QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine));

  // Axes Arrows
  m_customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
  m_customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);

  m_customPlot->legend->setTextColor(QColor("#00E5FF"));

  // Floating Play/Pause Button overlay
  m_btnFloatingPlay =
      new QToolButton(m_customPlot); // Parent is customPlot so it draws on top
  m_btnFloatingPlay->setFont(CIconFont::instance()->getIconFont(64));
  m_btnFloatingPlay->setText(QChar(0xE719)); // e719: Play Icon
  m_btnFloatingPlay->setFixedSize(120, 120);
  m_btnFloatingPlay->setCursor(Qt::PointingHandCursor);
  m_btnFloatingPlay->setStyleSheet("QToolButton {"
                                   "   color: rgba(255, 255, 255, 150);"
                                   "   background: transparent;"
                                   "   border: none;"
                                   "}"
                                   "QToolButton:hover {"
                                   "   color: rgba(0, 229, 255, 220);"
                                   "   background: rgba(255, 255, 255, 10);"
                                   "   border-radius: 60px;"
                                   "}");

  // Install event filter to track customPlot resize and hover
  m_customPlot->installEventFilter(this);
}

void SerialSession::applyChartTheme(int index) {
  QColor tickLabelColor, labelColor, gridColor, subGridColor;
  QPen gridPen, subGridPen, axisPen;
  QFont tickFont = font();
  tickFont.setPointSize(9);
  QFont labelFont = font();
  labelFont.setPointSize(11);
  labelFont.setBold(true);

  if (index == 0) {
    // 0: Light Theme
    m_customPlot->setBackground(Qt::white);
    m_customPlot->axisRect()->setBackground(Qt::white);

    axisPen = QPen(Qt::black, 1);
    tickLabelColor = Qt::black;
    labelColor = Qt::black;

    gridPen = QPen(Qt::lightGray, 1, Qt::DashLine);
    subGridPen = QPen(QColor(230, 230, 230), 1, Qt::DotLine);

    m_customPlot->legend->setBrush(QColor(255, 255, 255, 200));
    m_customPlot->legend->setBorderPen(QPen(Qt::lightGray));
    m_customPlot->legend->setTextColor(Qt::black);
  } else if (index == 1) {
    // 1: Neon Theme (Dark)
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor("#1E1E28"));
    plotGradient.setColorAt(1, QColor("#282836"));
    m_customPlot->setBackground(plotGradient);
    m_customPlot->axisRect()->setBackground(QColor(10, 10, 15, 180));

    axisPen = QPen(QColor("#00E5FF"), 2);
    tickLabelColor = QColor("#E0E0FF");
    labelColor = QColor("#00E5FF");

    gridPen = QPen(QColor(0, 150, 200, 50), 1, Qt::DashLine);
    subGridPen = QPen(QColor(0, 150, 200, 20), 1, Qt::DotLine);

    m_customPlot->legend->setBrush(QColor(20, 20, 30, 150));
    m_customPlot->legend->setBorderPen(Qt::NoPen);
    m_customPlot->legend->setTextColor(QColor("#00E5FF"));
  } else if (index == 2) {
    // 2: Sci-Fi Oscilloscope Theme (Intense Green/Cyan on Deep Black)
    m_customPlot->setBackground(Qt::black);
    m_customPlot->axisRect()->setBackground(QColor(0, 5, 0, 255));

    axisPen = QPen(QColor("#00FF41"), 2); // Matrix Green
    tickLabelColor = QColor("#00FF41");
    labelColor = QColor("#00FF41");

    gridPen =
        QPen(QColor(0, 255, 65, 80), 1, Qt::SolidLine); // More visible grid
    subGridPen =
        QPen(QColor(0, 255, 65, 30), 1, Qt::DotLine); // CRT-like secondary grid

    m_customPlot->legend->setBrush(QColor(0, 20, 0, 180));
    m_customPlot->legend->setBorderPen(QPen(QColor("#00FF41")));
    m_customPlot->legend->setTextColor(QColor("#00FF41"));
  }

  // Apply Axes styles
  m_customPlot->xAxis->setBasePen(axisPen);
  m_customPlot->yAxis->setBasePen(axisPen);
  m_customPlot->xAxis->setTickPen(axisPen);
  m_customPlot->yAxis->setTickPen(axisPen);
  m_customPlot->xAxis->setSubTickPen(QPen(axisPen.color(), 1));
  m_customPlot->yAxis->setSubTickPen(QPen(axisPen.color(), 1));

  m_customPlot->xAxis->setTickLabelColor(tickLabelColor);
  m_customPlot->yAxis->setTickLabelColor(tickLabelColor);
  m_customPlot->xAxis->setLabelColor(labelColor);
  m_customPlot->yAxis->setLabelColor(labelColor);

  m_customPlot->xAxis->setTickLabelFont(tickFont);
  m_customPlot->yAxis->setTickLabelFont(tickFont);
  m_customPlot->xAxis->setLabelFont(labelFont);
  m_customPlot->yAxis->setLabelFont(labelFont);

  // Apply Grid styles
  m_customPlot->xAxis->grid()->setPen(gridPen);
  m_customPlot->yAxis->grid()->setPen(gridPen);
  m_customPlot->xAxis->grid()->setSubGridVisible(true);
  m_customPlot->yAxis->grid()->setSubGridVisible(true);
  m_customPlot->xAxis->grid()->setSubGridPen(subGridPen);
  m_customPlot->yAxis->grid()->setSubGridPen(subGridPen);

  m_customPlot->replot();
}

void SerialSession::setupConnections() {
  connect(m_btnRefresh, &QPushButton::clicked, this,
          &SerialSession::refreshPorts);
  connect(m_btnOpenClose, &QPushButton::clicked, this,
          &SerialSession::openClosePort);
  connect(m_serial, &QSerialPort::readyRead, this, &SerialSession::onReadyRead);
  connect(m_serial, &QSerialPort::errorOccurred, this,
          &SerialSession::onPortError);

  connect(m_btnRxClear, &QToolButton::clicked, this,
          &SerialSession::clearReceiveArea);
  connect(m_btnSend, &QToolButton::clicked, this, &SerialSession::sendData);

  // Send Area Connections
  connect(m_btnTxClear, &QToolButton::clicked,
          [this]() { m_textSend->clear(); });
  connect(m_comboHistory, QOverload<int>::of(&QComboBox::activated),
          [this](int index) {
            if (index >= 0)
              m_textSend->setText(m_comboHistory->itemText(index));
          });

  connect(m_chkAutoSend, &QCheckBox::toggled, [this](bool checked) {
    if (checked) {
      m_spinAutoSendInterval->show();
    } else {
      m_spinAutoSendInterval->hide();
    }
    toggleAutoSend(checked);
  });

  // Floating Action Toggles
  connect(m_btnRxHexToggle, &QToolButton::toggled, [this](bool checked) {
    m_rbRxHex->setChecked(checked);
    m_rbRxAscii->setChecked(!checked);
  });
  connect(m_btnTxHexToggle, &QToolButton::toggled, [this](bool checked) {
    m_rbTxHex->setChecked(checked);
    m_rbTxAscii->setChecked(!checked);
  });
  connect(m_btnRxTimeToggle, &QToolButton::toggled,
          [this](bool checked) { m_chkRxTime->setChecked(checked); });
  // Connect the floating play/pause to the logical pause variable
  connect(m_btnRxPauseToggle, &QToolButton::toggled, [this](bool paused) {
    m_btnStopRx->setChecked(paused);
    if (paused) {
      m_btnRxPauseToggle->setText(QChar(0xE7D8)); // Pause icon
    } else {
      m_btnRxPauseToggle->setText(QChar(0xE617)); // Play icon
    }
  });
  connect(m_autoSendTimer, &QTimer::timeout, this,
          &SerialSession::onAutoSendTimeout);

  // Auto-convert TX input when switching ASCII <-> HEX
  connect(m_rbTxHex, &QRadioButton::toggled, this,
          &SerialSession::onTxModeChanged);

  // Multi-send tab button connections
  connect(m_btnMultiFirst, &QPushButton::clicked,
          [this]() { onMultiPageChanged(0); });
  connect(m_btnMultiPrev, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPage - 1); });
  connect(m_btnMultiNext, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPage + 1); });
  connect(m_btnMultiLast, &QPushButton::clicked,
          [this]() { onMultiPageChanged(m_multiPages.count() - 1); });
  connect(m_btnMultiAddPage, &QPushButton::clicked, [this]() {
    m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));
    int newPage = m_multiPages.count() - 1;
    m_spinJumpPage->setRange(1, m_multiPages.count());
    onMultiPageChanged(newPage);
  });
  // Scope Settings Toggles
  connect(m_chkHideRxTx, &QCheckBox::toggled,
          [this](bool checked) { m_dataSplitter->setVisible(!checked); });
  connect(m_chkHideRxData, &QCheckBox::toggled,
          [this](bool checked) { m_textReceive->setVisible(!checked); });

  connect(m_btnMultiDelPage, &QPushButton::clicked, [this]() {
    if (m_multiPages.count() <= 1)
      return;
    m_multiPages.removeAt(m_multiPage);
    m_spinJumpPage->setRange(1, m_multiPages.count());
    onMultiPageChanged(qMin(m_multiPage, m_multiPages.count() - 1));
  });
  connect(m_btnSendAll, &QPushButton::clicked, this,
          &SerialSession::sendSelectedMulti);
  connect(m_btnMultiImport, &QPushButton::clicked, this,
          &SerialSession::importMultiData);
  connect(m_btnMultiExport, &QPushButton::clicked, this,
          &SerialSession::exportMultiData);
  connect(m_chkMultiLoop, &QCheckBox::toggled, [this](bool on) {
    m_spinMultiLoopInterval->setEnabled(on);
    if (on) {
      m_multiLoopIndex = 0;
      m_multiLoopTimer->start(m_spinMultiLoopInterval->value());
    } else {
      m_multiLoopTimer->stop();
    }
  });
  connect(m_multiLoopTimer, &QTimer::timeout, this,
          &SerialSession::onMultiSendLoop);
  connect(m_spinMultiLoopInterval, QOverload<int>::of(&QSpinBox::valueChanged),
          [this](int v) {
            if (m_multiLoopTimer->isActive())
              m_multiLoopTimer->setInterval(v);
          });
  // Save grid edits back to model when focus leaves a cell
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    connect(m_chkMultiItem[i], &QCheckBox::toggled, [this, i](bool v) {
      if (m_multiPage < m_multiPages.count())
        m_multiPages[m_multiPage][i].enabled = v;
    });
    connect(m_leMultiItem[i], &QLineEdit::editingFinished, [this, i]() {
      if (m_multiPage < m_multiPages.count())
        m_multiPages[m_multiPage][i].content = m_leMultiItem[i]->text();
    });
  }

  connect(m_spinPoints, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &SerialSession::updateChartSettings);
  connect(m_chkShowGrid, &QCheckBox::toggled, this,
          &SerialSession::updateChartSettings);
  connect(m_btnAutoScale, &QPushButton::toggled, this,
          &SerialSession::updateChartSettings);
  connect(m_spinYMin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);
  connect(m_portCheckTimer, &QTimer::timeout, this, &SerialSession::checkPorts);

  connect(m_spinYMax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);
  connect(m_spinYTick, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SerialSession::updateChartSettings);

  connect(m_btnClearWaveform, &QPushButton::clicked, [this]() {
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (m_customPlot->graph(i) && m_customPlot->graph(i)->data()) {
        m_customPlot->graph(i)->data()->clear();
      }
    }
    m_xValue = 0;
    m_scrollbarWaveform->setMinimum(0);
    m_scrollbarWaveform->setMaximum(0);
    m_scrollbarWaveform->setValue(0);
    m_customPlot->replot();
  });

  connect(m_btnResetChart, &QPushButton::clicked, [this]() {
    m_spinPoints->setValue(100);
    m_btnAutoScale->setChecked(true);
    m_chkShowGrid->setChecked(true);
    m_spinYTick->setValue(0);
    updateChartSettings();
  });

  connect(m_replotTimer, &QTimer::timeout, this,
          &SerialSession::onReplotTimeout);

  connect(m_scrollbarWaveform, &QScrollBar::valueChanged, this,
          &SerialSession::onWaveformScroll);
  connect(m_comboTimeUnit, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SerialSession::onTimeUnitChanged);
  connect(m_comboChartTheme,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SerialSession::onChartThemeChanged);
  connect(m_spinBufferLimit, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int val) {
            if (m_spinPoints->value() > val) {
              m_spinPoints->setValue(val);
            }
          });

  connect(m_dockScopeSettings, &QDockWidget::dockLocationChanged, this,
          &SerialSession::onDockLocationChanged);

  // 波形显示控制
  connect(m_chkEnableWaveform, &QCheckBox::toggled, this,
          &SerialSession::onWaveformEnabled);

  // 图表右键菜单
  connect(m_customPlot, &QCustomPlot::customContextMenuRequested, this,
          &SerialSession::onChartContextMenu);

  // 曲线设置按钮
  connect(m_btnCurveSettings, &QPushButton::clicked, this,
          &SerialSession::onCurveSettingsClicked);

  // 图例双击显示/隐藏逻辑
  connect(m_customPlot, &QCustomPlot::legendDoubleClick, this,
          [this](QCPLegend *legend, QCPAbstractLegendItem *item,
                 QMouseEvent *event) {
            Q_UNUSED(legend);
            Q_UNUSED(event);
            if (item) {
              QCPPlottableLegendItem *plItem =
                  qobject_cast<QCPPlottableLegendItem *>(item);
              if (plItem) {
                bool visible = plItem->plottable()->visible();
                plItem->plottable()->setVisible(!visible);
                m_customPlot->replot();
              }
            }
          });

  // Floating Play Pause Logic Integration
  connect(m_btnFloatingPlay, &QToolButton::clicked, [this]() {
    m_btnStopWaveform->setChecked(!m_btnStopWaveform->isChecked());
  });

  connect(m_btnStopWaveform, &QPushButton::toggled, [this](bool checked) {
    if (checked) {
      // Checked meaning stopped/paused: Show play icon (e87d)
      m_btnStopWaveform->setText("\ue87d");
      m_btnFloatingPlay->setText(QChar(0xE719));
      m_btnFloatingPlay->show();
    } else {
      // Unchecked meaning playing: Show pause icon (e87c)
      m_btnStopWaveform->setText("\ue87c");
      m_btnFloatingPlay->hide();
    }
  });

  // Set default state to paused (playing starts when explicitly clicked)
  m_btnStopWaveform->setChecked(true);
  onTimeUnitChanged(m_comboTimeUnit->currentIndex());
}

void SerialSession::onCurveSettingsClicked() {
  CurveSettingsDialog dialog(m_customPlot, this);
  dialog.exec();
}

void SerialSession::refreshPorts() {
  QString currentPort = m_comboPort->currentData().toString();
  m_comboPort->clear();
  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
    m_comboPort->addItem(info.portName() + " (" + info.description() + ")",
                         info.portName());
  }

  // Restore selection if possible
  int idx = m_comboPort->findData(currentPort);
  if (idx >= 0) {
    m_comboPort->setCurrentIndex(idx);
  }
}

void SerialSession::checkPorts() {
  const auto infos = QSerialPortInfo::availablePorts();
  if (infos.count() != m_lastPortCount) {
    m_lastPortCount = infos.count();
    refreshPorts();
  } else {
    // Double check simple count mismatch might miss same count different ports
    // But for simple usage count check is often enough, or check list names
    bool changed = false;
    if (m_comboPort->count() != infos.count()) {
      changed = true;
    } else {
      for (int i = 0; i < infos.size(); i++) {
        if (m_comboPort->itemData(i).toString() != infos.at(i).portName()) {
          changed = true;
          break;
        }
      }
    }

    if (changed) {
      refreshPorts();
    }
  }
}

void SerialSession::openClosePort() {
  if (m_serial->isOpen()) {
    m_serial->close(); // close once — duplicate calls caused state corruption
    m_welcomeText =
        "   欢迎使用uSmilePro串口示波器V1.0   "; // Revert to default
    m_lblWelcome->setText(m_welcomeText);
    ToastWidget::showToast("串口 " + m_serial->portName() + " 已关闭", false,
                           this);
    m_btnOpenClose->setText(QChar(0xE84E));
    m_btnOpenClose->setChecked(false);
    m_lblStatusIcon->setPixmap(
        QPixmap(":/icons/ONOFF/OFF5.png")
            .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateStatusInfo();
    // Disable auto-send
    m_chkAutoSend->setChecked(false); // triggers toggleAutoSend -> stops timer
    m_chkAutoSend->setEnabled(false);
    m_spinAutoSendInterval->setEnabled(false);
    // Enable port settings
    m_comboPort->setEnabled(true);
    m_comboBaud->setEnabled(true);
    m_comboDataBits->setEnabled(true);
    m_comboParity->setEnabled(true);
    m_comboStopBits->setEnabled(true);
  } else {
    m_serial->setPortName(m_comboPort->currentData().toString());
    m_serial->setBaudRate(m_comboBaud->currentText().toInt());
    m_serial->setDataBits(static_cast<QSerialPort::DataBits>(
        m_comboDataBits->currentText().toInt()));
    m_serial->setParity(
        static_cast<QSerialPort::Parity>(m_comboParity->currentData().toInt()));
    m_serial->setStopBits(static_cast<QSerialPort::StopBits>(
        m_comboStopBits->currentData().toInt()));

    if (m_serial->open(QIODevice::ReadWrite)) {
      m_welcomeText = "   串口 " + m_serial->portName() +
                      " 已打开   "; // Change text for scroll
      m_lblWelcome->setText(m_welcomeText);
      ToastWidget::showToast("串口 " + m_serial->portName() + " 已打开", true,
                             this);

      m_btnOpenClose->setText(QChar(0xE855));
      m_btnOpenClose->setChecked(true);
      m_lblStatusIcon->setPixmap(
          QPixmap(":/icons/ONOFF/ON2.png")
              .scaled(100, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      updateStatusInfo();
      // Enable auto-send controls
      m_chkAutoSend->setEnabled(true);
      // Disable port settings
      m_comboPort->setEnabled(false);
      m_comboBaud->setEnabled(false);
      m_comboDataBits->setEnabled(false);
      m_comboParity->setEnabled(false);
      m_comboStopBits->setEnabled(false);
    } else {
      QString errorStr = m_serial->errorString();
      QTimer::singleShot(0, this, [this, errorStr]() {
        QMessageBox::critical(this, "错误", "无法打开串口:\n" + errorStr);
      });
      m_btnOpenClose->setChecked(false);
      m_btnOpenClose->setText(QChar(0xE84E));
      m_btnOpenClose->setToolTip("打开串口");
    }
  }
}

void SerialSession::onPortError(QSerialPort::SerialPortError error) {
  if (error == QSerialPort::ResourceError) {
    // Defer dialog + close to next event loop iteration to avoid re-entrant
    // signal handling (calling QMessageBox directly from a serial error signal
    // can cause recursive event-loop processing and crash).
    QTimer::singleShot(0, this, [this]() {
      QMessageBox::critical(this, "严重错误", "串口连接中断！");
      openClosePort();
    });
  }
}

void SerialSession::onReadyRead() {
  QByteArray data = m_serial->readAll();
  m_rxCount += data.size();
  m_lblRxCount->setText(QString::number(m_rxCount));

  if (!m_btnStopRx->isChecked()) {
    QString rawStr;

    // Hex vs ASCII
    if (m_rbRxHex->isChecked()) {
      rawStr = data.toHex(' ').toUpper();
    } else {
      rawStr = QString::fromLocal8Bit(data); // Support Local encoding
    }

    const bool logMode = m_chkRxLog->isChecked();
    const bool showTimestamp = m_chkRxTime->isChecked() || logMode;
    const QString rxTag = logMode ? "[LOG][RX]" : "[RX]";

    // Build HTML line: red timestamp + plain data
    QString htmlLine;
    if (showTimestamp) {
      QString timeStr =
          QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
      htmlLine = QString("<span style='color:red;'>%1</span>"
                         "<span>%2 %3</span>")
                     .arg(timeStr.toHtmlEscaped())
                     .arg(rxTag)
                     .arg(rawStr.toHtmlEscaped());
    } else {
      htmlLine = QString("<span>%1 %2</span>")
                     .arg(rxTag)
                     .arg(rawStr.toHtmlEscaped());
    }

    if (!m_chkHideRxData->isChecked()) {
      bool showRaw = m_chkShowRawData->isChecked();
      if (!showRaw) {
        // 未勾选「显示原始数据」时，显示接收到的完整原始数据
        m_textReceive->append(htmlLine);
        m_textReceive->verticalScrollBar()->setValue(
            m_textReceive->verticalScrollBar()->maximum());
      }
      // 勾选时，updateWaveform 会显示去掉帧头帧尾的 Payload
    }
  }

  updateWaveform(data);
}

void SerialSession::onWaveformEnabled(bool checked) {
  m_waveformPage->setVisible(checked);
  if (checked) {
    m_customPlot->replot();
  }
}

void SerialSession::onDockLocationChanged(Qt::DockWidgetArea area) {
  if (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea) {
    m_settingsLayout->setDirection(QBoxLayout::LeftToRight);
  } else {
    m_settingsLayout->setDirection(QBoxLayout::TopToBottom);
  }
}

void SerialSession::updateChartSettings() {
  if (m_btnAutoScale->isChecked()) {
    m_spinYMin->setEnabled(false);
    m_spinYMax->setEnabled(false);
    m_spinYTick->setEnabled(false);
    // Determine best fit immediately
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (m_customPlot->graph(i)) {
        if (i == 0)
          m_customPlot->graph(i)->rescaleValueAxis(true);
        else
          m_customPlot->graph(i)->rescaleValueAxis(true, true);
      }
    }
    // Auto handled in updateWaveform or by QCP rescale
  } else {
    m_spinYMin->setEnabled(true);
    m_spinYMax->setEnabled(true);
    m_spinYTick->setEnabled(true);
    m_customPlot->yAxis->setRange(m_spinYMin->value(), m_spinYMax->value());

    if (m_spinYTick->value() > 0) {
      QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
      fixedTicker->setTickStep(m_spinYTick->value());
      m_customPlot->yAxis->setTicker(fixedTicker);
    } else {
      QSharedPointer<QCPAxisTicker> autoTicker(new QCPAxisTicker);
      m_customPlot->yAxis->setTicker(autoTicker);
    }
  }

  // Grid
  bool showGrid = m_chkShowGrid->isChecked();
  m_customPlot->xAxis->grid()->setVisible(showGrid);
  m_customPlot->yAxis->grid()->setVisible(showGrid);
  m_customPlot->xAxis->grid()->setSubGridVisible(showGrid);
  m_customPlot->yAxis->grid()->setSubGridVisible(showGrid);

  m_customPlot->replot();
}

void SerialSession::updateWaveform(const QByteArray &data) {
  bool waveformStopped = m_btnStopWaveform && m_btnStopWaveform->isChecked();
  // 若波形已暂停且不需要显示 Payload，则直接跳过
  if (waveformStopped && !m_chkShowRawData->isChecked()) {
    return;
  }

  // Use the per-instance member buffer — NOT a static, which would be shared
  // across all SerialSession instances and causes data corruption + crashes
  // when two serial ports are open simultaneously.
  m_rxBuffer.append(data);

  int maxPoints = m_spinPoints->value();
  bool dataAdded = false;

  QVector<QVector<double>> channelDataBatch;
  QVector<QVector<double>> channelKeysBatch;

  while (true) {
    int startIdx = m_rxBuffer.indexOf('$');
    if (startIdx == -1) {
      if (m_rxBuffer.size() > 4096)
        m_rxBuffer.clear(); // Safety check
      break;
    }

    int endIdx = m_rxBuffer.indexOf(';', startIdx);
    if (endIdx == -1) {
      if (m_rxBuffer.size() > 4096) {
        m_rxBuffer = m_rxBuffer.mid(startIdx);
        if (m_rxBuffer.size() > 4096)
          m_rxBuffer.clear(); // Safety clear
      }
      break; // Need more data
    }

    // Found complete frame: "$ ... ;"
    QByteArray payloadBytes =
        m_rxBuffer.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
    m_rxBuffer.remove(0, endIdx + 1); // Remove processed frame

    if (payloadBytes.isEmpty())
      continue;

    QString payload = QString::fromLatin1(payloadBytes);

    // 勾选「显示原始数据」时，显示去掉帧头帧尾后的 Payload
    if (m_chkShowRawData->isChecked()) {
      m_textReceive->append("<span style='color: #4CAF50;'>[Payload] " +
                            payload.toHtmlEscaped() + "</span>");
      m_textReceive->verticalScrollBar()->setValue(
          m_textReceive->verticalScrollBar()->maximum());
    }

    // 如果波形已暂停，跳过图表数据更新
    if (waveformStopped) {
      continue;
    }

    QStringList parts =
        payload.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.isEmpty())
      continue;

    // Ensure we have enough graphs
    int neededGraphs = parts.size();
    while (m_customPlot->graphCount() < neededGraphs) {
      int idx = m_customPlot->graphCount();
      m_customPlot->addGraph();

      // Assign distinct vibrant colors using HSV for a "neon" effect
      int hue = (idx * 137) % 360; // Golden angle approx
      // Use high saturation and value for neon glow
      QColor color = QColor::fromHsv(hue, 230, 255);
      QPen pen(color);
      pen.setWidthF(2.0f); // Thicker line for glowing appearance
      m_customPlot->graph(idx)->setPen(pen);
      m_customPlot->graph(idx)->setName(QString("CH%1").arg(idx + 1));

      // Create channel eye icon toggle row
      QWidget *chWidget = new QWidget();
      QHBoxLayout *chLayout = new QHBoxLayout(chWidget);
      chLayout->setContentsMargins(0, 0, 0, 0);

      QLabel *colorLabel = new QLabel("■");
      colorLabel->setStyleSheet(
          QString("color: %1; font-size: 16px;").arg(color.name()));

      QLabel *nameLabel = new QLabel(m_customPlot->graph(idx)->name());

      QToolButton *eyeBtn = new QToolButton();
      eyeBtn->setCheckable(true);
      eyeBtn->setChecked(true);
      eyeBtn->setFont(CIconFont::instance()->getIconFont(16));
      eyeBtn->setText(QChar(0xE846)); // visible icon
      eyeBtn->setStyleSheet(
          "QToolButton:checked { font-weight: normal; color: #4CAF50; border: "
          "none; background: transparent; } "
          "QToolButton:!checked { font-weight: normal; color: gray; "
          "border: none; background: transparent; }");

      connect(eyeBtn, &QToolButton::toggled, [this, idx, eyeBtn](bool checked) {
        eyeBtn->setText(checked ? QChar(0xE846) : QChar(0xE847));
        if (idx < m_customPlot->graphCount() && m_customPlot->graph(idx)) {
          m_customPlot->graph(idx)->setVisible(checked);
          m_customPlot->replot();
        }
      });

      chLayout->addWidget(colorLabel);
      chLayout->addWidget(nameLabel);
      chLayout->addStretch();
      chLayout->addWidget(eyeBtn);

      if (m_channelsLayout->count() > 0) {
        m_channelsLayout->insertWidget(m_channelsLayout->count() - 1, chWidget);
      } else {
        m_channelsLayout->addWidget(chWidget);
      }
      m_channelWidgets[idx] = chWidget;
    }

    // Accumulate data to graphs
    for (int i = 0; i < parts.size(); ++i) {
      bool ok;
      double val = parts[i].toDouble(&ok);
      if (ok) {
        if (i >= channelDataBatch.size()) {
          channelDataBatch.resize(i + 1);
          channelKeysBatch.resize(i + 1);
        }
        channelDataBatch[i].append(val);
        channelKeysBatch[i].append(m_xValue);
      }
    }

    m_xValue++;
    dataAdded = true;
  }

  if (dataAdded) {
    int bufferLimit = m_spinBufferLimit->value();
    // Batch removal threshold to prevent continuous array shifting
    int removalThreshold = bufferLimit + maxPoints;

    for (int i = 0; i < channelDataBatch.size(); ++i) {
      if (!channelDataBatch[i].isEmpty()) {
        m_customPlot->graph(i)->addData(channelKeysBatch[i],
                                        channelDataBatch[i]);

        // Only trim when it exceeds limit significantly (e.g. by one screen
        // width)
        if (m_customPlot->graph(i)->dataCount() > removalThreshold) {
          m_customPlot->graph(i)->data()->removeBefore(m_xValue - bufferLimit);
        }
      }
    }

    double dataMinX = qMax(0.0, m_xValue - bufferLimit);
    double dataMaxX = m_xValue;

    bool isTracking =
        (m_scrollbarWaveform->value() == m_scrollbarWaveform->maximum());
    int scrollMax = qMax(0, (int)(dataMaxX - dataMinX - maxPoints));

    // Disable signals briefly to avoid triggering onWaveformScroll during
    // internal setup
    m_scrollbarWaveform->blockSignals(true);
    m_scrollbarWaveform->setMinimum(0);
    m_scrollbarWaveform->setMaximum(scrollMax);
    m_scrollbarWaveform->blockSignals(false);

    if (isTracking) {
      m_scrollbarWaveform->setValue(scrollMax);
      m_customPlot->xAxis->setRange(m_xValue, maxPoints, Qt::AlignRight);
    } else {
      double viewLeft = dataMinX + m_scrollbarWaveform->value();
      m_customPlot->xAxis->setRange(viewLeft, viewLeft + maxPoints);
    }

    if (!m_waveformPage->isHidden()) {
      m_needsReplot = true; // Flag for the 30fps timer to pick up
    }
  }
}

void SerialSession::onReplotTimeout() {
  if (!m_needsReplot || m_waveformPage->isHidden())
    return;

  if (m_btnAutoScale->isChecked()) {
    for (int i = 0; i < m_customPlot->graphCount(); ++i) {
      if (i == 0)
        m_customPlot->graph(i)->rescaleValueAxis(false, true);
      else
        m_customPlot->graph(i)->rescaleValueAxis(true, true);
    }
  }

  m_customPlot->replot();
  m_needsReplot = false;
}

void SerialSession::onWaveformScroll(int value) {
  int maxPoints = m_spinPoints->value();
  int bufferLimit = m_spinBufferLimit->value();
  double dataMinX = qMax(0.0, m_xValue - bufferLimit);

  double viewLeft = dataMinX + value;
  m_customPlot->xAxis->setRange(viewLeft, viewLeft + maxPoints);

  if (!m_waveformPage->isHidden()) {
    m_needsReplot = true;
  }
}

void SerialSession::onTimeUnitChanged(int index) {
  QString label;
  double divisor = 1.0;
  int precision = 0;

  if (index == 0) {
    label = "Time (Points)";
    divisor = 1.0;
    precision = 0;
  } else if (index == 1) {
    label = "Time (ms)";
    divisor = 1.0; // 1 point is treated as 1 ms for display purposes.
    precision = 0;
  } else if (index == 2) {
    label = "Time (s)";
    divisor = 1000.0;
    precision = 3;
  } else {
    label = "Time";
  }

  m_customPlot->xAxis->setTicker(
      QSharedPointer<QCPAxisTicker>(new ScaledAxisTicker(divisor, precision)));
  m_customPlot->xAxis->setLabel(label);
  m_needsReplot = true;
}

void SerialSession::onChartThemeChanged(int index) {
  applyChartTheme(index);

  // Update existing channels' line thickness and color vibrancy based on theme
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    if (m_customPlot->graph(i)) {
      int hue = (i * 137) % 360;
      QColor color;
      QPen pen;

      if (index == 0) {
        // Light Theme: Standard saturation
        color = QColor::fromHsv(hue, 200, 200);
        pen = QPen(color, 1.5f);
      } else if (index == 1) {
        // Neon Theme: High saturation and value
        color = QColor::fromHsv(hue, 230, 255);
        pen = QPen(color, 2.0f);
      } else if (index == 2) {
        // Sci-Fi Oscilloscope: Extremely bright, almost glowing borders
        color = QColor::fromHsv(hue, 250, 255);
        pen = QPen(color, 2.5f);
      }

      m_customPlot->graph(i)->setPen(pen);
    }
  }
  m_needsReplot = true;
}

void SerialSession::onChartContextMenu(const QPoint &pos) {
  QMenu menu(this);
  menu.addAction("复制绘图", this, [this]() {
    QApplication::clipboard()->setPixmap(m_customPlot->toPixmap());
    ToastWidget::showToast("已复制到剪贴板", false, this);
  });
  menu.addSeparator();
  menu.addAction("导出为 PNG", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 PNG", "", "Images (*.png)");
    if (!fileName.isEmpty()) {
      m_customPlot->savePng(fileName);
      ToastWidget::showToast("已成功导出为 PNG", false, this);
    }
  });
  menu.addAction("导出为 JPG", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 JPG", "", "Images (*.jpg)");
    if (!fileName.isEmpty()) {
      m_customPlot->saveJpg(fileName);
      ToastWidget::showToast("已成功导出为 JPG", false, this);
    }
  });
  menu.addAction("导出为 PDF", this, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "保存为 PDF", "", "PDF (*.pdf)");
    if (!fileName.isEmpty()) {
      m_customPlot->savePdf(fileName);
      ToastWidget::showToast("已成功导出为 PDF", false, this);
    }
  });

  menu.exec(m_customPlot->mapToGlobal(pos));
}

bool SerialSession::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_customPlot) {
    if (event->type() == QEvent::Resize) {
      if (m_btnFloatingPlay) {
        // Center the floating button dynamically
        int x = (m_customPlot->width() - m_btnFloatingPlay->width()) / 2;
        int y = (m_customPlot->height() - m_btnFloatingPlay->height()) / 2;
        m_btnFloatingPlay->move(x, y);
      }
    } else if (event->type() == QEvent::Enter) {
      // Hover Enter: if plotting is currently active, show a pause icon faintly
      if (m_btnStopWaveform && !m_btnStopWaveform->isChecked() &&
          m_btnFloatingPlay) {
        m_btnFloatingPlay->setText(QChar(0xE6D3)); // Pause icon
        m_btnFloatingPlay->show();
      }
    } else if (event->type() == QEvent::Leave) {
      // Hover Leave: hide pause button if we were just hovering during play
      if (m_btnStopWaveform && !m_btnStopWaveform->isChecked() &&
          m_btnFloatingPlay) {
        m_btnFloatingPlay->hide();
      }
    }
  } else if (watched == m_textReceive &&
             (event->type() == QEvent::Resize ||
              event->type() == QEvent::Show)) {
    QSize size = m_textReceive->size();
    if (event->type() == QEvent::Resize) {
      QResizeEvent *re = static_cast<QResizeEvent *>(event);
      size = re->size();
    }
    QWidget *rxFloat = m_textReceive->findChild<QWidget *>("rxFloat");
    if (rxFloat) {
      rxFloat->adjustSize();
      rxFloat->move(qMax(4, size.width() - rxFloat->width() - 10),
                    qMax(4, size.height() - rxFloat->height() - 10));
    }
  } else if (watched == m_textSend &&
             (event->type() == QEvent::Resize ||
              event->type() == QEvent::Show)) {
    QSize size = m_textSend->size();
    if (event->type() == QEvent::Resize) {
      QResizeEvent *re = static_cast<QResizeEvent *>(event);
      size = re->size();
    }
    QWidget *txFloatRight = m_textSend->findChild<QWidget *>("txFloatRight");
    QWidget *txFloatLeft = m_textSend->findChild<QWidget *>("txFloatLeft");
    if (txFloatRight) {
      txFloatRight->adjustSize();
      txFloatRight->move(qMax(4, size.width() - txFloatRight->width() - 10),
                         qMax(4, size.height() - txFloatRight->height() - 10));
    }
    if (txFloatLeft) {
      txFloatLeft->adjustSize();
      txFloatLeft->move(10,
                        qMax(4, size.height() - txFloatLeft->height() - 10));
    }
  }

  return QWidget::eventFilter(watched, event);
}

void SerialSession::sendData() {
  if (!m_serial->isOpen())
    return;

  QString text = m_textSend->toPlainText();
  if (text.isEmpty())
    return;

  QByteArray idx;
  if (m_rbTxHex->isChecked()) {
    idx = QByteArray::fromHex(text.toUtf8());
  } else {
    idx = text.toLocal8Bit();
  }

  if (m_chkTxNewLine->isChecked()) {
    idx.append("\r\n");
  }

  m_serial->write(idx);
  m_txCount += idx.size();
  m_lblTxCount->setText(QString::number(m_txCount));

  QString txRawStr;
  if (m_rbTxHex->isChecked()) {
    txRawStr = idx.toHex(' ').toUpper();
  } else {
    txRawStr = QString::fromLocal8Bit(idx);
  }

  const bool logMode = m_chkRxLog->isChecked();
  const bool showTimestamp = m_chkTxTime->isChecked() || logMode;
  const QString txTag = logMode ? "[LOG][TX]" : "[TX]";

  // Build HTML line: blue timestamp + plain TX data
  QString txHtmlLine;
  if (showTimestamp) {
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    txHtmlLine = QString("<span style='color:blue;'>%1</span>"
                         "<span>%2 %3</span>")
                     .arg(timeStr.toHtmlEscaped())
                     .arg(txTag)
                     .arg(txRawStr.toHtmlEscaped());
  } else {
    txHtmlLine = QString("<span>%1 %2</span>")
                     .arg(txTag)
                     .arg(txRawStr.toHtmlEscaped());
  }

  m_textReceive->append(txHtmlLine);
  m_textReceive->verticalScrollBar()->setValue(
      m_textReceive->verticalScrollBar()->maximum());

  // Add to History (avoid duplicates at top, simple Lru-like)
  if (m_comboHistory->findText(text) == -1) {
    m_comboHistory->insertItem(0, text);
    if (m_comboHistory->count() > 10) {
      m_comboHistory->removeItem(m_comboHistory->count() - 1);
    }
  } else {
    // If exists, move to top? Standard behavior varies.
    // Let's just ensure it's at 0 if we want 'recent' behavior
    int index = m_comboHistory->findText(text);
    m_comboHistory->removeItem(index);
    m_comboHistory->insertItem(0, text);
  }
  m_comboHistory->setCurrentIndex(0);
}

void SerialSession::clearReceiveArea() {
  m_textReceive->clear();
  m_rxBuffer.clear();
  m_rxCount = 0;
  m_lblRxCount->setText("0");
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    if (m_customPlot->graph(i) && m_customPlot->graph(i)->data()) {
      m_customPlot->graph(i)->data()->clear();
    }
  }
  m_scrollbarWaveform->setMinimum(0);
  m_scrollbarWaveform->setMaximum(0);
  m_scrollbarWaveform->setValue(0);
  m_customPlot->replot();
  m_xValue = 0;
}

void SerialSession::toggleAutoSend(bool checked) {
  if (checked) {
    m_autoSendTimer->start(m_spinAutoSendInterval->value());
    m_spinAutoSendInterval->setEnabled(false);
  } else {
    m_autoSendTimer->stop();
    m_spinAutoSendInterval->setEnabled(true);
  }
}

void SerialSession::onAutoSendTimeout() {
  // 仅在串口未打开时停止（不检查 isVisible，切换标签页时应继续发送）
  if (!m_serial->isOpen()) {
    m_autoSendTimer->stop();
    m_chkAutoSend->setChecked(false);
    return;
  }
  sendData();
}

void SerialSession::hideEvent(QHideEvent *event) {
  // 仅在串口未打开时停止定时器并取消勾选。
  // 若串口已打开，保持定时器运行：onAutoSendTimeout/onMultiSendLoop
  // 内部均有 isOpen() 检查，隐藏时继续发送也是安全的。
  if (!m_serial->isOpen()) {
    if (m_autoSendTimer->isActive()) {
      m_autoSendTimer->stop();
      m_chkAutoSend->setChecked(false);
    }
    if (m_multiLoopTimer->isActive()) {
      m_multiLoopTimer->stop();
      m_chkMultiLoop->setChecked(false);
    }
  }
  QWidget::hideEvent(event);
}

void SerialSession::updateStatusInfo() {
  if (m_serial->isOpen()) {
    QString info =
        QString("串口[%1] 已打开 %2 %3-%4-%5")
            .arg(m_serial->portName())
            .arg(m_serial->baudRate())
            .arg(m_serial->dataBits())
            .arg(m_serial->parity() == QSerialPort::NoParity
                     ? "N"
                     : (m_serial->parity() == QSerialPort::OddParity ? "O"
                                                                     : "E"))
            .arg(m_serial->stopBits() == QSerialPort::OneStop
                     ? "1"
                     : (m_serial->stopBits() == QSerialPort::OneAndHalfStop
                            ? "1.5"
                            : "2"));
    m_lblPortInfo->setText(info);
    m_lblPortInfo->setStyleSheet("color: green; font-weight: bold;");
  } else {
    QString info =
        QString("串口[%1] 已关闭 %2 %3-%4-%5")
            .arg(m_serial->portName())
            .arg(m_serial->baudRate())
            .arg(m_serial->dataBits())
            .arg(m_serial->parity() == QSerialPort::NoParity
                     ? "N"
                     : (m_serial->parity() == QSerialPort::OddParity ? "O"
                                                                     : "E"))
            .arg(m_serial->stopBits() == QSerialPort::OneStop
                     ? "1"
                     : (m_serial->stopBits() == QSerialPort::OneAndHalfStop
                            ? "1.5"
                            : "2"));
    m_lblPortInfo->setText(info);
    m_lblPortInfo->setStyleSheet("color: red; font-weight: bold;");
  }
}

QString SerialSession::getButtonStyle(ButtonType type) {
  // Base Style with shared settings
  // NOTE: font-family is intentionally NOT in baseStyle so that icon buttons
  // (Refresh/Open) can use the iconfont set via setFont() without QSS override.
  QString baseStyle = "QPushButton { "
                      "    border-radius: 10px; "
                      "    border: 1px solid #90A4AE; "
                      "    padding: 5px; "
                      "    color: black; "
                      "}"
                      "QPushButton:hover { "
                      "    font-weight: 900; "
                      "    font-size: 10pt; "
                      "}";

  QString gradient;
  switch (type) {
  case ButtonType::Normal:
    // Glossy Blue-Gray, include font-family for CJK text buttons
    gradient = "QPushButton { "
               "    font-family: 'Microsoft YaHei UI'; "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #CFD8DC, stop:1 #B0BEC5); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #ECEFF1, stop:1 #CFD8DC); "
               "}"
               "QPushButton:pressed { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #B0BEC5, stop:1 #90A4AE); "
               "}";
    break;
  case ButtonType::Refresh:
    // Glossy Light Gray
    gradient = "QPushButton { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #E0E0E0, stop:1 #BDBDBD); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #F5F5F5, stop:1 #E0E0E0); "
               "}"
               "QPushButton:pressed { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #E0E0E0, stop:1 #9E9E9E); "
               "}";
    break;
  case ButtonType::Open:
    // Glossy Green / Red
    gradient = "QPushButton { " // Normal (Open - Green)
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #C8E6C9, stop:1 #81C784); "
               "}"
               "QPushButton:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #E8F5E9, stop:1 #A5D6A7); "
               "}"
               "QPushButton:checked { " // Checked (Close - Red)
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #FFCDD2, stop:1 #E57373); "
               "}"
               "QPushButton:checked:hover { "
               "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
               "stop:0 #FFFFFF, stop:0.1 #FFEBEE, stop:1 #EF9A9A); "
               "}";
    break;
  default:
    break;
  }

  return baseStyle + gradient;
}

void SerialSession::onTxModeChanged(bool hexChecked) {
  QString current = m_textSend->toPlainText().trimmed();
  if (current.isEmpty())
    return;

  if (hexChecked) {
    // ASCII -> HEX: encode text bytes to space-separated uppercase hex
    QByteArray bytes = current.toLocal8Bit();
    QString hexStr = bytes.toHex(' ').toUpper();
    m_textSend->setPlainText(hexStr);
  } else {
    // HEX -> ASCII: try to decode hex string back to text
    // Remove all whitespace first to handle spaces between bytes
    QString clean = current.remove(' ').remove('\t');
    QByteArray bytes = QByteArray::fromHex(clean.toUtf8());
    if (!bytes.isEmpty()) {
      m_textSend->setPlainText(QString::fromLocal8Bit(bytes));
    }
  }
}

// ──────────────────────────────────────────────────────────
// Multi-send: helper to push current page data into widgets
// ──────────────────────────────────────────────────────────
void SerialSession::refreshMultiPage() {
  if (m_multiPages.isEmpty())
    return;
  const auto &page = m_multiPages.at(m_multiPage);
  for (int i = 0; i < MULTI_PER_PAGE; i++) {
    const MultiSendItem &item =
        (i < page.size()) ? page.at(i) : MultiSendItem{};
    // Block signals to avoid saving back while loading
    m_chkMultiItem[i]->blockSignals(true);
    m_leMultiItem[i]->blockSignals(true);
    m_chkMultiItem[i]->setChecked(item.enabled);
    m_leMultiItem[i]->setText(item.content);
    m_chkMultiItem[i]->blockSignals(false);
    m_leMultiItem[i]->blockSignals(false);
  }
  int total = m_multiPages.count();
  m_lblMultiPage->setText(
      QString("第 %1 / %2 页").arg(m_multiPage + 1).arg(total));
  m_spinJumpPage->setValue(m_multiPage + 1);
  m_btnMultiFirst->setEnabled(m_multiPage > 0);
  m_btnMultiPrev->setEnabled(m_multiPage > 0);
  m_btnMultiNext->setEnabled(m_multiPage < total - 1);
  m_btnMultiLast->setEnabled(m_multiPage < total - 1);
  m_btnMultiDelPage->setEnabled(total > 1);
}

void SerialSession::onMultiPageChanged(int page) {
  // Save current page edits to model first
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }
  // Clamp
  m_multiPage = qBound(0, page, m_multiPages.count() - 1);
  refreshMultiPage();
}

// ──────────────────────────────────────────────────────────
// Send all checked items on current page
// ──────────────────────────────────────────────────────────
void SerialSession::sendSelectedMulti() {
  if (!m_serial->isOpen())
    return;

  // Save current edits first
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }

  bool globalHex = m_chkMultiHex->isChecked();
  bool globalNewLine = m_chkMultiNewLine->isChecked();
  const bool logMode = m_chkRxLog->isChecked();

  const auto &page = m_multiPages.at(m_multiPage);
  int idx = 0;
  for (const MultiSendItem &item : page) {
    idx++;
    if (!item.enabled || item.content.isEmpty())
      continue;

    QString text = item.content;
    QByteArray data;
    bool isHex = globalHex || item.isHex;
    if (isHex) {
      data = QByteArray::fromHex(text.remove(' ').toUtf8());
    } else {
      data = text.toLocal8Bit();
      if (globalNewLine)
        data.append("\r\n");
    }

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString txHtml =
        QString("<span style='color:blue;'>%1</span><span>%2#%3 %4</span>")
            .arg(timeStr.toHtmlEscaped())
            .arg(logMode ? "[LOG][TX]" : "[TX]")
            .arg(idx)
            .arg(rawStr.toHtmlEscaped());
    m_textReceive->append(txHtml);
    m_textReceive->verticalScrollBar()->setValue(
        m_textReceive->verticalScrollBar()->maximum());
  }
}

// ──────────────────────────────────────────────────────────
// Auto-loop: cycle through ALL pages sending checked items
// ──────────────────────────────────────────────────────────
void SerialSession::onMultiSendLoop() {
  if (!m_serial->isOpen()) {
    m_multiLoopTimer->stop();
    return;
  }

  const bool logMode = m_chkRxLog->isChecked();

  // Find next enabled item across all pages
  int totalItems = m_multiPages.count() * MULTI_PER_PAGE;
  for (int tries = 0; tries < totalItems; tries++) {
    int page = m_multiLoopIndex / MULTI_PER_PAGE;
    int idx = m_multiLoopIndex % MULTI_PER_PAGE;
    m_multiLoopIndex = (m_multiLoopIndex + 1) % totalItems;

    if (page >= m_multiPages.count())
      continue;
    const MultiSendItem &item = m_multiPages.at(page).at(idx);
    if (!item.enabled || item.content.isEmpty())
      continue;

    bool isHex = m_chkMultiHex->isChecked() || item.isHex;
    QString text = item.content;
    QByteArray data = isHex ? QByteArray::fromHex(text.remove(' ').toUtf8())
                            : text.toLocal8Bit();
    if (m_chkMultiNewLine->isChecked() && !isHex)
      data.append("\r\n");

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString html =
        QString("<span style='color:blue;'>%1</span><span>%2 P%3#%4 %5</span>")
            .arg(timeStr.toHtmlEscaped())
            .arg(logMode ? "[LOG][LOOP]" : "[LOOP]")
            .arg(page + 1)
            .arg(idx + 1)
            .arg(rawStr.toHtmlEscaped());
    m_textReceive->append(html);
    m_textReceive->verticalScrollBar()->setValue(
        m_textReceive->verticalScrollBar()->maximum());
    break; // send one per tick
  }
}

// ──────────────────────────────────────────────────────────
// Import / Export CSV
// Format: page_index,item_index,enabled,hex,content
// ──────────────────────────────────────────────────────────
void SerialSession::importMultiData() {
  QString path = QFileDialog::getOpenFileName(this, "导入多条发送数据", "",
                                              "CSV (*.csv);;All files (*)");
  if (path.isEmpty())
    return;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "导入失败", f.errorString());
    return;
  }
  QTextStream ts(&f);
  // Clear existing data
  m_multiPages.clear();

  while (!ts.atEnd()) {
    QString line = ts.readLine().trimmed();
    if (line.isEmpty() || line.startsWith('#'))
      continue;
    QStringList cols = line.split(',');
    if (cols.size() < 5)
      continue;
    int pageIdx = cols[0].toInt();
    int itemIdx = cols[1].toInt();
    while (m_multiPages.size() <= pageIdx)
      m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));
    if (itemIdx >= 0 && itemIdx < MULTI_PER_PAGE) {
      MultiSendItem &item = m_multiPages[pageIdx][itemIdx];
      item.enabled = cols[2].trimmed() == "1";
      item.isHex = cols[3].trimmed() == "1";
      item.content = cols.mid(4).join(','); // content may contain commas
    }
  }
  if (m_multiPages.isEmpty())
    m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));

  m_multiPage = 0;
  m_spinJumpPage->setRange(1, m_multiPages.count());
  refreshMultiPage();
}

void SerialSession::exportMultiData() {
  // Save current page edits
  if (m_multiPage < m_multiPages.size()) {
    auto &cur = m_multiPages[m_multiPage];
    for (int i = 0; i < MULTI_PER_PAGE; i++) {
      cur[i].enabled = m_chkMultiItem[i]->isChecked();
      cur[i].content = m_leMultiItem[i]->text();
    }
  }

  QString path = QFileDialog::getSaveFileName(
      this, "导出多条发送数据", "multi_send.csv", "CSV (*.csv);;All files (*)");
  if (path.isEmpty())
    return;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "导出失败", f.errorString());
    return;
  }
  QTextStream ts(&f);
  ts << "# page_index,item_index,enabled,hex,content\n";
  for (int p = 0; p < m_multiPages.count(); p++) {
    const auto &page = m_multiPages.at(p);
    for (int i = 0; i < page.size(); i++) {
      const MultiSendItem &item = page.at(i);
      ts << p << ',' << i << ',' << (item.enabled ? 1 : 0) << ','
         << (item.isHex ? 1 : 0) << ',' << item.content << '\n';
    }
  }
}

// sendAll() kept for MOC compatibility – delegates to sendSelectedMulti()
void SerialSession::sendAll() { sendSelectedMulti(); }

void SerialSession::applyTheme(const QString &themeMode) {
  if (!m_customPlot)
    return;
  bool isDark = themeMode.contains("dark", Qt::CaseInsensitive) ||
                themeMode.contains("one", Qt::CaseInsensitive);

  QColor bgColor = isDark ? QColor("#1E1E1E") : QColor("#FFFFFF");
  QColor textColor = isDark ? QColor("#DCDCDC") : QColor("#000000");
  QColor gridColor = isDark ? QColor("#3E3E42") : QColor("#E0E0E0");

  m_customPlot->setBackground(bgColor);
  m_customPlot->axisRect()->setBackground(bgColor);

  // X Axis
  m_customPlot->xAxis->setBasePen(QPen(textColor));
  m_customPlot->xAxis->setTickPen(QPen(textColor));
  m_customPlot->xAxis->setSubTickPen(QPen(textColor));
  m_customPlot->xAxis->setTickLabelColor(textColor);
  m_customPlot->xAxis->setLabelColor(textColor);
  m_customPlot->xAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));
  m_customPlot->xAxis->grid()->setZeroLinePen(QPen(gridColor));

  // Y Axis
  m_customPlot->yAxis->setBasePen(QPen(textColor));
  m_customPlot->yAxis->setTickPen(QPen(textColor));
  m_customPlot->yAxis->setSubTickPen(QPen(textColor));
  m_customPlot->yAxis->setTickLabelColor(textColor);
  m_customPlot->yAxis->setLabelColor(textColor);
  m_customPlot->yAxis->grid()->setPen(QPen(gridColor, 1, Qt::DotLine));
  m_customPlot->yAxis->grid()->setZeroLinePen(QPen(gridColor));

  m_customPlot->replot();
}

// =========================================================================
// SerialPortPlot: 顶层多标签页容器（会话管理器）
// =========================================================================
#include "TOOLS/CIconFont.h"
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>

SerialPortPlot::SerialPortPlot(QWidget *parent)
    : QWidget(parent), m_sessionCounter(1) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // --- 选项卡区域 ---
  m_sessionTabs = new QTabWidget(this);
  m_sessionTabs->setTabsClosable(true);

  // Split actions corner widget
  QWidget *cornerWidget = new QWidget(this);
  QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
  cornerLayout->setContentsMargins(0, 0, 0, 0);
  cornerLayout->setSpacing(2);

  QFont iconFont = CIconFont::instance()->getIconFont(24);

  QToolButton *btnSplitH = new QToolButton(this);
  btnSplitH->setFont(iconFont);
  btnSplitH->setText(QString(QChar(0xe7f7)));
  btnSplitH->setToolTip("水平分屏");

  QToolButton *btnSplitV = new QToolButton(this);
  btnSplitV->setFont(iconFont);
  btnSplitV->setText(QString(QChar(0xe8cc)));
  btnSplitV->setToolTip("垂直分屏");

  QToolButton *btnCloseSplit = new QToolButton(this);
  btnCloseSplit->setFont(iconFont);
  btnCloseSplit->setText(QString(QChar(0xe7ac)));
  btnCloseSplit->setToolTip("关闭分屏");

  cornerLayout->addWidget(btnSplitH);
  cornerLayout->addWidget(btnSplitV);
  cornerLayout->addWidget(btnCloseSplit);

  m_sessionTabs->setCornerWidget(cornerWidget, Qt::TopRightCorner);

  connect(btnSplitH, &QToolButton::clicked, this,
          &SerialPortPlot::onSplitHorizontal);
  connect(btnSplitV, &QToolButton::clicked, this,
          &SerialPortPlot::onSplitVertical);
  connect(btnCloseSplit, &QToolButton::clicked, this,
          &SerialPortPlot::onCloseSplit);
  m_sessionTabs->setMovable(true);

  // 应用仿浏览器圆角标签页样式
  m_sessionTabs->setStyleSheet(R"(
    QTabWidget::pane {
        border-top: 1px solid #C0C0C0;
        background-color: transparent;
        margin-top: -1px;
    }
    QTabWidget::left-corner {
        background: transparent;
        border: none;
        width: 0px;
    }
    QTabWidget::right-corner {
        background: transparent;
        border: none;
    }
    QTabBar::tab {
        background: #E8E8E8;
        border: 1px solid #C0C0C0;
        border-bottom-color: #C0C0C0;
        border-top-left-radius: 8px;
        border-top-right-radius: 8px;
        min-width: 100px;
        padding: 6px 16px;
        margin-right: 2px;
        margin-top: 4px;
    }
    QTabBar::tab:selected, QTabBar::tab:hover {
        background: #FFFFFF;
        border-bottom-color: #FFFFFF;
    }
    QTabBar::tab:selected {
        margin-top: 0px;
        font-weight: bold;
    }
    QTabBar::tab:first {
        margin-left: 0px;
    }
    /* 针对我们特殊的加号标签稍作样式调整 */
    QTabBar::tab:last {
        min-width: 30px;
        padding: 6px 8px;
        background: transparent;
        border: none;
        margin-top: 4px;
        font-weight: bold;
        color: #555555;
    }
    QTabBar::tab:last:hover {
        background: #D0D0D0;
        border-radius: 8px;
        color: #000000;
    }
  )");

  // 安装事件过滤器用于监听双击重命名和“+”号假选项卡的点击
  m_sessionTabs->tabBar()->installEventFilter(this);

  connect(m_sessionTabs, &QTabWidget::tabCloseRequested, this,
          &SerialPortPlot::onTabCloseRequested);

  mainLayout->addWidget(m_sessionTabs);

  // 先添加 ➕ 号假标签（使用更宽更显眼的全角加号或者emoji包裹空格）
  m_sessionTabs->addTab(new QWidget(), "  ➕  ");
  // 隐藏假标签的关闭按钮
  m_sessionTabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);

  // 默认启动一个会话
  addNewSession();
}

SerialPortPlot::~SerialPortPlot() {}

bool SerialPortPlot::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_sessionTabs->tabBar()) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      int index = m_sessionTabs->tabBar()->tabAt(mouseEvent->pos());
      // 如果点击的是最后一个 "+" 号标签页
      if (index == m_sessionTabs->count() - 1 &&
          mouseEvent->button() == Qt::LeftButton) {
        addNewSession();
        return true;
      }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      int index = m_sessionTabs->tabBar()->tabAt(mouseEvent->pos());
      // 排除 "+" 标签页的重命名
      if (index >= 0 && index < m_sessionTabs->count() - 1) {
        onTabDoubleClicked(index);
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void SerialPortPlot::addNewSession() {
  SerialSession *session = new SerialSession(this);
  QString title;
  if (m_sessionCounter == 1) {
    title = "SerialPortPlotPro";
  } else {
    title = QString("SerialPortPlotPro%1").arg(m_sessionCounter);
  }
  m_sessionCounter++;

  // The user requested View toolbar inside the inner session instead of main
  // window. We will configure the local toolbar inside SerialSession.

  // 插入到 "  ➕  " 号前面
  int addIndex = m_sessionTabs->count() - 1;
  int index = m_sessionTabs->insertTab(addIndex, session, title);
  m_sessionTabs->setCurrentIndex(index);
}

SerialSession *SerialPortPlot::getActiveSession() const {
  if (!m_sessionTabs)
    return nullptr;
  int currentIndex = m_sessionTabs->currentIndex();
  if (currentIndex < 0 || currentIndex >= m_sessionTabs->count() - 1)
    return nullptr;

  return qobject_cast<SerialSession *>(m_sessionTabs->widget(currentIndex));
}

void SerialPortPlot::toggleDock(int dockType, bool checked) {
  // Get current session
  int currentIndex = m_sessionTabs->currentIndex();
  if (currentIndex < 0 || currentIndex >= m_sessionTabs->count() - 1)
    return;

  SerialSession *session =
      qobject_cast<SerialSession *>(m_sessionTabs->widget(currentIndex));
  if (!session)
    return;

  switch (dockType) {
  case 0:
    session->m_dockPort->setVisible(checked);
    break;
  case 1:
    session->m_dockRx->setVisible(checked);
    break;
  case 2:
    session->m_dockTx->setVisible(checked);
    break;
  case 3:
    session->m_dockScopeSettings->setVisible(checked);
    break;
  }
}

void SerialPortPlot::onTabDoubleClicked(int index) {
  if (index < 0)
    return;
  bool ok;
  QString currentTitle = m_sessionTabs->tabText(index);
  QString newTitle = QInputDialog::getText(
      this, "重命名会话", "新名称:", QLineEdit::Normal, currentTitle, &ok);
  if (ok && !newTitle.isEmpty()) {
    m_sessionTabs->setTabText(index, newTitle);
  }
}

void SerialPortPlot::onTabCloseRequested(int index) {
  // 禁止关闭 "+" 号标签页
  if (index == m_sessionTabs->count() - 1)
    return;

  // 保持至少一个真实会话，避免误关闭整个串口工具窗口
  if (m_sessionTabs->count() <= 2) {
    return;
  }

  // 记录即将关闭前要跳转到的索引，防止跳到 "+" 标签
  int nextIndex = -1;
  if (m_sessionTabs->currentIndex() == index) {
    if (index > 0) {
      nextIndex = index - 1; // 优先跳到左边
    } else if (index < m_sessionTabs->count() - 2) {
      nextIndex = index + 1; // 不可能的话跳到右边的真实标签
    }
  }

  QWidget *widget = m_sessionTabs->widget(index);
  if (widget) {
    widget->deleteLater();
  }
  m_sessionTabs->removeTab(index);

  // 切换到合适的索引，防止激活 "+" 标签
  if (nextIndex >= 0) {
    m_sessionTabs->setCurrentIndex(nextIndex);
  } else {
    // 默认情况如果跑到了 "+" 标签，则强制跳回最后一个真实标签
    if (m_sessionTabs->currentIndex() == m_sessionTabs->count() - 1) {
      m_sessionTabs->setCurrentIndex(m_sessionTabs->count() - 2);
    }
  }
}

void SerialPortPlot::applyGlobalTheme(const QString &themeFile) {
  QFile file(QString(":/resources/styles/") + themeFile);
  if (file.open(QFile::ReadOnly)) {
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
    file.close();
  }

  // Also pass to internal sessions
  for (int i = 0; i < m_sessionTabs->count() - 1; i++) {
    SerialSession *session =
        qobject_cast<SerialSession *>(m_sessionTabs->widget(i));
    if (session) {
      session->applyTheme(themeFile);
    }
  }

  emit themeChanged(themeFile);
}

void SerialPortPlot::applyFileIconTheme(const QString &themeName) {
  QIcon::setThemeName(themeName);
}

void SerialPortPlot::onSplitHorizontal() { emit requestSplitHorizontal(this); }

void SerialPortPlot::onSplitVertical() { emit requestSplitVertical(this); }

void SerialPortPlot::onCloseSplit() { emit requestCloseSplit(this); }

// =========================================================================
// SerialPortContainer Implementation
// =========================================================================

SerialPortContainer::SerialPortContainer(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  // --- 全局工具栏 ---
  m_toolbar = new QToolBar("Main Toolbar", this);
  m_toolbar->setMovable(false);

  // Settings
  m_toolbar->addAction("设置");

  // --------- Theme (Menu) ---------
  QToolButton *btnTheme = new QToolButton(this);
  btnTheme->setText("主题");
  btnTheme->setPopupMode(QToolButton::InstantPopup);
  QMenu *menuTheme = new QMenu(btnTheme);

  // 1. Color Theme Submenu
  QMenu *menuColorTheme = menuTheme->addMenu("颜色主题");
  // Github
  QMenu *menuGithub = menuColorTheme->addMenu("Github");
  menuGithub->addAction("Dark", this,
                        [this]() { applyGlobalTheme("githubdark.qss"); });
  menuGithub->addAction("Light", this,
                        [this]() { applyGlobalTheme("githublight.qss"); });
  // Aura
  QMenu *menuAura = menuColorTheme->addMenu("Aura");
  menuAura->addAction("Dark", this,
                      [this]() { applyGlobalTheme("auradark.qss"); });
  menuAura->addAction("Light", this,
                      [this]() { applyGlobalTheme("auralight.qss"); });
  // ATOM
  QMenu *menuAtom = menuColorTheme->addMenu("ATOM");
  menuAtom->addAction("Dark", this,
                      [this]() { applyGlobalTheme("atomone.qss"); });
  menuAtom->addAction("Light", this,
                      [this]() { applyGlobalTheme("atomlight.qss"); });
  // Solarized
  QMenu *menuSolarized = menuColorTheme->addMenu("Solarized");
  menuSolarized->addAction("Dark", this,
                           [this]() { applyGlobalTheme("solarizeddark.qss"); });
  menuSolarized->addAction(
      "Light", this, [this]() { applyGlobalTheme("solarizedlight.qss"); });

  // 2. File Icon Theme Submenu
  QMenu *menuFileIcon = menuTheme->addMenu("文件图标主题");
  menuFileIcon->addAction("Material Icon", this,
                          [this]() { applyFileIconTheme("material"); });
  menuFileIcon->addAction("VSCode Icon", this,
                          [this]() { applyFileIconTheme("vscode"); });

  // 3. Product Icon Theme Submenu
  QMenu *menuProductIcon = menuTheme->addMenu("产品图标主题");
  menuProductIcon->addAction("Default", this,
                             [this]() { applyFileIconTheme("default"); });

  btnTheme->setMenu(menuTheme);
  m_toolbar->addWidget(btnTheme);

  // Help
  m_toolbar->addAction("帮助");

  // About
  m_toolbar->addAction("关于");

  // 视图 (View)
  QToolButton *btnView = new QToolButton(this);
  btnView->setText("视图");
  btnView->setPopupMode(QToolButton::InstantPopup);
  QMenu *menuView = new QMenu(btnView);

  connect(menuView, &QMenu::aboutToShow, this, [this, menuView]() {
    menuView->clear();
    if (m_plotHistory.isEmpty())
      return;

    SerialPortPlot *activePlot = nullptr;
    for (SerialPortPlot *plot : m_plotHistory) {
      if (plot->isAncestorOf(QApplication::focusWidget()) || plot->hasFocus()) {
        activePlot = plot;
        break;
      }
    }
    if (!activePlot)
      activePlot = m_plotHistory.last();

    SerialSession *session = activePlot->getActiveSession();
    if (session) {
      menuView->addAction(session->m_dockPort->toggleViewAction());
      menuView->addAction(session->m_dockRx->toggleViewAction());
      menuView->addAction(session->m_dockTx->toggleViewAction());
      menuView->addAction(session->m_dockScopeSettings->toggleViewAction());
    } else {
      menuView->addAction("当前无活动会话")->setEnabled(false);
    }
  });

  btnView->setMenu(menuView);
  m_toolbar->addWidget(btnView);

  layout->addWidget(m_toolbar);

  m_mainSplitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(m_mainSplitter);

  SerialPortPlot *initialPlot = createNewPlot();
  m_mainSplitter->addWidget(initialPlot);
}

SerialPortContainer::~SerialPortContainer() {}

SerialPortPlot *SerialPortContainer::createNewPlot() {
  SerialPortPlot *plot = new SerialPortPlot(this);
  connect(plot, &SerialPortPlot::requestSplitHorizontal, this,
          &SerialPortContainer::handleSplitHorizontal);
  connect(plot, &SerialPortPlot::requestSplitVertical, this,
          &SerialPortContainer::handleSplitVertical);
  connect(plot, &SerialPortPlot::requestCloseSplit, this,
          &SerialPortContainer::handleCloseSplit);
  connect(plot, &SerialPortPlot::themeChanged, this,
          &SerialPortContainer::handleThemeChanged);

  if (!m_currentTheme.isEmpty()) {
    plot->applyGlobalTheme(m_currentTheme);
  }

  m_plotHistory.append(plot);
  return plot;
}

void SerialPortContainer::handleThemeChanged(const QString &themeName) {
  m_currentTheme = themeName;
}

void SerialPortContainer::applyGlobalTheme(const QString &themeFile) {
  m_currentTheme = themeFile;
  for (SerialPortPlot *plot : qAsConst(m_plotHistory)) {
    plot->applyGlobalTheme(themeFile);
  }
}

void SerialPortContainer::applyFileIconTheme(const QString &themeName) {
  for (SerialPortPlot *plot : qAsConst(m_plotHistory)) {
    plot->applyFileIconTheme(themeName);
  }
}

void SerialPortContainer::handleSplitHorizontal() {
  SerialPortPlot *senderPlot = qobject_cast<SerialPortPlot *>(sender());
  if (!senderPlot)
    return;

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(senderPlot->parentWidget());
  if (!parentSplitter)
    return;

  SerialPortPlot *newPlot = createNewPlot();

  if (parentSplitter->orientation() == Qt::Horizontal) {
    int index = parentSplitter->indexOf(senderPlot);
    parentSplitter->insertWidget(index + 1, newPlot);
  } else {
    int index = parentSplitter->indexOf(senderPlot);
    QSplitter *newSplitter = new QSplitter(Qt::Horizontal, parentSplitter);

    QList<int> sizes = parentSplitter->sizes();

    parentSplitter->insertWidget(index, newSplitter);
    newSplitter->addWidget(senderPlot);
    newSplitter->addWidget(newPlot);

    parentSplitter->setSizes(sizes);
  }
}

void SerialPortContainer::handleSplitVertical() {
  SerialPortPlot *senderPlot = qobject_cast<SerialPortPlot *>(sender());
  if (!senderPlot)
    return;

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(senderPlot->parentWidget());
  if (!parentSplitter)
    return;

  SerialPortPlot *newPlot = createNewPlot();

  if (parentSplitter->orientation() == Qt::Vertical) {
    int index = parentSplitter->indexOf(senderPlot);
    parentSplitter->insertWidget(index + 1, newPlot);
  } else {
    int index = parentSplitter->indexOf(senderPlot);
    QSplitter *newSplitter = new QSplitter(Qt::Vertical, parentSplitter);

    QList<int> sizes = parentSplitter->sizes();

    parentSplitter->insertWidget(index, newSplitter);
    newSplitter->addWidget(senderPlot);
    newSplitter->addWidget(newPlot);

    parentSplitter->setSizes(sizes);
  }
}

void SerialPortContainer::handleCloseSplit(SerialPortPlot *plotToClose) {
  if (m_plotHistory.size() <= 1) {
    return; // Don't close the last one
  }
  if (!plotToClose || !m_plotHistory.contains(plotToClose)) {
    return;
  }

  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(plotToClose->parentWidget());
  if (parentSplitter) {
    plotToClose->hide();
    plotToClose->deleteLater();
    m_plotHistory.removeAll(plotToClose);
  }
}
