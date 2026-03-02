#include "serialportplot.h"
#include "curvesettings.h"
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
#include <QScrollBar>
#include <QSize>
#include <QSplitter>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>


SerialSession::SerialSession(QWidget *parent)
    : QWidget(parent), m_rxCount(0), m_txCount(0), m_xValue(0),
      m_lastPortCount(0), m_scrollPos(0) {
  m_serial = new QSerialPort(this);
  m_autoSendTimer = new QTimer(this);
  m_portCheckTimer = new QTimer(this);

  // Multi-send: init with 1 page of 20 empty items
  m_multiPage = 0;
  m_multiLoopIndex = 0;
  m_multiLoopTimer = new QTimer(this);
  m_multiPages.append(QVector<MultiSendItem>(MULTI_PER_PAGE));

  m_welcomeText = "   欢迎使用uSmilePro串口示波器V1.0   ";

  setupUi();
  setupChart();
  setupConnections();
  refreshMultiPage(); // init multi-send page display
  refreshPorts();

  // Start timers
  m_portCheckTimer->start(1000);
}

SerialSession::~SerialSession() {
  if (m_serial->isOpen())
    m_serial->close();
}

void SerialSession::setupUi() {
  QVBoxLayout *mainLayout =
      new QVBoxLayout(this); // Changed to Vertical for Status Bar

  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

  // Connect splitter to mainLayout later, but first let's keep the structure
  // The original code did mainLayout->addWidget(splitter). We will do that too.
  // splitter->setStyleSheet("QSplitter::handle { background-color: #cccccc;
  // }");
  // --- Left Panel: Settings ---
  QWidget *leftPanel =
      new QWidget(); // 创建左面板，当前没有指定其父，后面通过
                     // splitter->addWidget(leftPanel);确定QSplitter为其父
  QVBoxLayout *leftLayout = new QVBoxLayout(
      leftPanel); // 垂直布局绑定leftPanel，确定垂直布局的父是leftPanel；

  // 给左侧布局设内边距（面板边缘到控件的距离）
  leftLayout->setContentsMargins(10, 10, 10, 10); // 上下左右各10像素
  // 设置控件之间的间距
  leftLayout->setSpacing(8); // 控件之间隔8像素

  // 给左侧面板加浅灰色背景，和右侧绘图区域区分开
  // Global Stylesheet for GroupBoxes and Background
  this->setStyleSheet("SerialPortPlot { background-color: #f5f5f5; }"
                      "QGroupBox { "
                      "    border: 1px solid #BDBDBD; "
                      "    border-radius: 10px; "
                      "    margin-top: 10px; "
                      "    font-weight: bold; "
                      "}"
                      "QGroupBox::title { "
                      "    subcontrol-origin: margin; "
                      "    subcontrol-position: top left; "
                      "    padding: 0 5px; "
                      "    left: 10px; "
                      "}"
                      "QTextEdit { "
                      "    border: 1px solid #4CAF50; "
                      "    border-radius: 10px; "
                      "    padding: 5px; "
                      "    background-color: #FFFFFF; "
                      "}");

  // 1. Port Settings
  QGroupBox *grpPort = new QGroupBox(
      "串口设置"); // 垂直布局的子对象grpPort（串口设置分组框）
                   // 当前没有指定其父，后面通过leftLayout->addWidget(grpPort);确定其父为leftLayout
  QGridLayout *portLayout = new QGridLayout(
      grpPort); // 串口网格布局，布局绑定grpPort，确定网格布局的父是grpPort

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

  m_btnRefresh = new QPushButton("刷新");
  m_btnRefresh->setMinimumWidth(80);
  m_btnRefresh->setMinimumHeight(40);
  m_btnRefresh->setStyleSheet(getButtonStyle(ButtonType::Refresh));
  portActionLayout->addWidget(m_btnRefresh);

  m_btnOpenClose = new QPushButton("打开串口");
  m_btnOpenClose->setCheckable(true);

  // Make button smaller (compact)
  m_btnOpenClose->setMinimumWidth(80);
  m_btnOpenClose->setMinimumHeight(40);
  m_btnOpenClose->setStyleSheet(getButtonStyle(ButtonType::Open));

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

  leftLayout->addWidget(grpPort);

  // 2. Receive Settings
  QGroupBox *grpRx = new QGroupBox("接收设置");
  QVBoxLayout *rxLayout = new QVBoxLayout(grpRx);

  QHBoxLayout *rxModeLayout = new QHBoxLayout();
  m_rbRxAscii = new QRadioButton("ASCII");
  m_rbRxHex = new QRadioButton("HEX");
  m_rbRxAscii->setChecked(true);
  rxModeLayout->addWidget(m_rbRxAscii);
  rxModeLayout->addWidget(m_rbRxHex);
  rxLayout->addLayout(rxModeLayout);

  m_chkRxLog = new QCheckBox("日志模式");
  m_chkRxTime = new QCheckBox("显示时间");
  m_chkRxNewLine = new QCheckBox(
      "自动换行"); // Actually TextEdit does this, maybe "Add Newline"? Let's
                   // assume user means data interpretation or auto-scrolling

  rxLayout->addWidget(m_chkRxLog);
  rxLayout->addWidget(m_chkRxTime);
  // rxLayout->addWidget(m_chkRxNewLine);

  QHBoxLayout *rxBtnLayout = new QHBoxLayout();
  m_btnClearRx = new QPushButton("清空");
  m_btnClearRx->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnStopRx = new QPushButton("暂停显示");
  m_btnStopRx->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnStopRx->setCheckable(true);
  rxBtnLayout->addWidget(m_btnClearRx);
  rxBtnLayout->addWidget(m_btnStopRx);
  rxLayout->addLayout(rxBtnLayout);

  rxLayout->addLayout(rxBtnLayout);

  // Waveform Toggle (Removed, moved to Toolbar)
  // m_chkEnableWaveform = new QCheckBox("启用波形显示");
  // rxLayout->addWidget(m_chkEnableWaveform);

  // 4. Waveform Settings (Moved to Side Panel in Waveform Page)
  // Just initialize controls here if needed, or better, do it when creating
  // m_waveformPage Let's defer creation to the m_waveformPage section to keep
  // layout logic together

  // m_grpWaveformSettings was removed from UI layout

  leftLayout->addWidget(grpRx);

  // 3. Send Settings
  QGroupBox *grpTx = new QGroupBox("发送设置");
  QVBoxLayout *txLayout = new QVBoxLayout(grpTx);

  QHBoxLayout *txModeLayout = new QHBoxLayout();
  m_rbTxAscii = new QRadioButton("ASCII");
  m_rbTxHex = new QRadioButton("HEX");
  m_rbTxAscii->setChecked(true);
  txModeLayout->addWidget(m_rbTxAscii);
  txModeLayout->addWidget(m_rbTxHex);
  txLayout->addLayout(txModeLayout);

  m_chkTxNewLine = new QCheckBox("发送新行");
  m_chkTxTime = new QCheckBox("显示时间"); // Timestamp in local log
  txLayout->addWidget(m_chkTxNewLine);
  txLayout->addWidget(m_chkTxTime);

  m_chkAutoSend = new QCheckBox("自动发送(ms):");
  m_chkAutoSend->setEnabled(false); // Disabled until port is open
  m_spinAutoSendInterval = new QSpinBox();
  m_spinAutoSendInterval->setRange(10, 10000);
  m_spinAutoSendInterval->setValue(1000);
  m_spinAutoSendInterval->setEnabled(false); // Disabled until port is open

  QHBoxLayout *autoSendLayout = new QHBoxLayout();
  autoSendLayout->addWidget(m_chkAutoSend);
  autoSendLayout->addWidget(m_spinAutoSendInterval);
  txLayout->addLayout(autoSendLayout);

  leftLayout->addWidget(grpTx);

  // === 新增：示波器设置 GroupBox ===
  QGroupBox *grpScope = new QGroupBox("示波器设置");
  QVBoxLayout *grpScopeLayout = new QVBoxLayout(grpScope);
  grpScopeLayout->setContentsMargins(6, 6, 6, 6);
  grpScopeLayout->setSpacing(4);

  m_chkEnableWaveform = new QCheckBox("使能波形显示");
  m_chkScopeSettings = new QCheckBox("参数设置");
  m_chkHideRxTx = new QCheckBox("隐藏收发区");
  m_chkHideRxData = new QCheckBox("不显示接收");
  m_chkShowRawData = new QCheckBox("显示原始数据");

  // 让复选框错落有致，美化布局
  grpScopeLayout->addWidget(m_chkEnableWaveform);
  grpScopeLayout->addWidget(m_chkScopeSettings);
  grpScopeLayout->addWidget(m_chkHideRxTx);
  grpScopeLayout->addWidget(m_chkHideRxData);
  grpScopeLayout->addWidget(m_chkShowRawData);
  leftLayout->addWidget(grpScope);

  // Status (Counts)
  QGroupBox *grpStatus = new QGroupBox("统计");
  QGridLayout *statusLayout = new QGridLayout(grpStatus);
  statusLayout->addWidget(new QLabel("RX:"), 0, 0);
  m_lblRxCount = new QLabel("0");
  statusLayout->addWidget(m_lblRxCount, 0, 1);
  statusLayout->addWidget(new QLabel("TX:"), 1, 0);
  m_lblTxCount = new QLabel("0");
  statusLayout->addWidget(m_lblTxCount, 1, 1);
  leftLayout->addWidget(grpStatus);

  leftLayout->addStretch();

  splitter->addWidget(leftPanel);

  // --- Right Panel: Data and Waveform ---
  QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
  m_dataSplitter = rightSplitter;

  // Receive Area
  QGroupBox *grpData = new QGroupBox("数据接收");
  QVBoxLayout *dataLayout = new QVBoxLayout(grpData);
  m_textReceive = new QTextEdit();
  m_textReceive->setReadOnly(true);
  dataLayout->addWidget(m_textReceive);
  rightSplitter->addWidget(grpData);

  // --- Send Area: 4-tab QTabWidget ---
  m_sendTabWidget = new QTabWidget();
  m_sendTabWidget->setDocumentMode(false);

  // ========== Tab 0: 单条发送 ==========
  QWidget *tabSingle = new QWidget();
  QVBoxLayout *singleLayout = new QVBoxLayout(tabSingle);
  singleLayout->setContentsMargins(6, 6, 6, 6);

  m_textSend = new QTextEdit();
  m_textSend->setMaximumHeight(60);
  m_textSend->setPlaceholderText("输入要发送的数据...");
  singleLayout->addWidget(m_textSend);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  m_btnSend = new QPushButton("发送");
  m_btnSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnSend->setMinimumHeight(36);

  m_btnClearSend = new QPushButton("清除");
  m_btnClearSend->setStyleSheet(getButtonStyle(ButtonType::Normal));
  m_btnClearSend->setMinimumHeight(36);

  btnLayout->addWidget(m_btnClearSend);
  btnLayout->addWidget(m_btnSend);
  singleLayout->addLayout(btnLayout);

  QHBoxLayout *histLayout = new QHBoxLayout();
  histLayout->addWidget(new QLabel("历史:"));
  m_comboHistory = new QComboBox();
  m_comboHistory->setEditable(false);
  histLayout->addWidget(m_comboHistory);
  singleLayout->addLayout(histLayout);

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

  // 外包 GroupBox "数据发送"
  QGroupBox *grpSend = new QGroupBox("数据发送");
  QVBoxLayout *grpSendLayout = new QVBoxLayout(grpSend);
  grpSendLayout->setContentsMargins(4, 8, 4, 4);
  grpSendLayout->addWidget(m_sendTabWidget);
  rightSplitter->addWidget(grpSend);

  splitter->addWidget(rightSplitter); // Middle Pane

  // --- Extended Waveform Page ---
  // We use a QMainWindow to easily support Dock Widgets
  m_waveformPage = new QMainWindow();
  // m_waveformPage->setWindowFlags(Qt::Widget); // Embeddable

  // -- Chart (Central Widget) --
  m_customPlot = new QCustomPlot();
  m_waveformPage->setCentralWidget(m_customPlot);

  // -- Side Settings Panel (Dock Widget) --
  m_dockSettings = new QDockWidget("绘图设置", m_waveformPage);
  m_dockSettings->setAllowedAreas(Qt::AllDockWidgetAreas);
  connect(m_dockSettings, &QDockWidget::dockLocationChanged, this,
          &SerialSession::onDockLocationChanged);

  QWidget *dockContents = new QWidget();
  m_settingsLayout = new QBoxLayout(QBoxLayout::TopToBottom, dockContents);
  m_settingsLayout->setContentsMargins(5, 10, 5, 10);
  m_settingsLayout->setSpacing(8);

  // Points
  m_settingsLayout->addWidget(new QLabel("显示点数:"));
  m_spinPoints = new QSpinBox();
  m_spinPoints->setRange(10, 10000);
  m_spinPoints->setValue(100);
  m_spinPoints->setSingleStep(10);
  m_settingsLayout->addWidget(m_spinPoints);

  // Y Axis Min/Max
  m_settingsLayout->addWidget(new QLabel("Y轴最小值:"));
  m_spinYMin = new QDoubleSpinBox();
  m_spinYMin->setRange(-99999, 99999);
  m_spinYMin->setValue(0);
  m_spinYMin->setEnabled(false); // Default Auto is ON
  m_settingsLayout->addWidget(m_spinYMin);

  m_settingsLayout->addWidget(new QLabel("Y轴最大值:"));
  m_spinYMax = new QDoubleSpinBox();
  m_spinYMax->setRange(-99999, 99999);
  m_spinYMax->setValue(255);
  m_spinYMax->setEnabled(false); // Default Auto is ON
  m_settingsLayout->addWidget(m_spinYMax);

  // Y-axis ticks
  m_settingsLayout->addWidget(new QLabel("Y轴刻度:"));
  m_spinYTick = new QDoubleSpinBox();
  m_spinYTick->setRange(0, 99999);
  m_spinYTick->setValue(0);       // 0 corresponds to Auto
  m_spinYTick->setEnabled(false); // Default Auto is ON
  m_settingsLayout->addWidget(m_spinYTick);

  // Grid
  m_chkShowGrid = new QCheckBox("显示网格");
  m_chkShowGrid->setChecked(true);
  m_settingsLayout->addWidget(m_chkShowGrid);

  // Auto Scale Button
  m_btnAutoScale = new QPushButton("自动缩放");
  m_btnAutoScale->setCheckable(true);
  m_btnAutoScale->setChecked(true);
  m_settingsLayout->addWidget(m_btnAutoScale);

  m_btnClearWaveform = new QPushButton("清空波形");
  m_settingsLayout->addWidget(m_btnClearWaveform);

  m_btnResetChart = new QPushButton("重置参数");
  m_settingsLayout->addWidget(m_btnResetChart);

  m_btnCurveSettings = new QPushButton("曲线设置");
  m_settingsLayout->addWidget(m_btnCurveSettings);

  m_btnStopWaveform = new QPushButton("暂停波形");
  m_btnStopWaveform->setCheckable(true);
  m_settingsLayout->addWidget(m_btnStopWaveform);

  m_settingsLayout->addStretch();

  m_dockSettings->setWidget(dockContents);
  m_waveformPage->addDockWidget(Qt::LeftDockWidgetArea, m_dockSettings);

  splitter->addWidget(m_waveformPage); // Rightmost Pane

  // Hide initially
  m_waveformPage->setVisible(false);

  // Splitter Layout Factors
  // 0: Settings (Fixed-ish)
  // 1: Center (Data/Send) (Expands)
  // 2: Waveform (Expands when visible)
  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);
  splitter->setCollapsible(2, true);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setStretchFactor(2, 2); // Give waveform plenty of space

  // --- Layout Integration ---
  // Directly add content to Tab Widget without Navigation Bar

  // 移除了内部多余的 m_mainTabWidget，直接将主分割器添加到布局中
  mainLayout->addWidget(splitter);

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

  statusBarLayout->addWidget(m_lblPortInfo);
  statusBarLayout->addStretch();
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

  // Interactions: Scroll and Zoom? Maybe later, keep simple for now
  m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                                QCP::iSelectLegend | QCP::iSelectPlottables);

  // Antialiasing for better-looking curves
  m_customPlot->setAntialiasedElements(QCP::aeAll);

  // Initial Range
  m_customPlot->xAxis->setRange(0, 100);
  m_customPlot->yAxis->setRange(0, 255);

  m_customPlot->setVisible(true);

  // Styling
  // Dashed Grids
  m_customPlot->xAxis->grid()->setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
  m_customPlot->yAxis->grid()->setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
  m_customPlot->xAxis->grid()->setSubGridVisible(true);
  m_customPlot->yAxis->grid()->setSubGridVisible(true);

  // Axes Arrows
  m_customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
  m_customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
}

void SerialSession::setupConnections() {
  connect(m_btnRefresh, &QPushButton::clicked, this,
          &SerialSession::refreshPorts);
  connect(m_btnOpenClose, &QPushButton::clicked, this,
          &SerialSession::openClosePort);
  connect(m_serial, &QSerialPort::readyRead, this, &SerialSession::onReadyRead);
  connect(m_serial, &QSerialPort::errorOccurred, this,
          &SerialSession::onPortError);

  connect(m_btnClearRx, &QPushButton::clicked, this,
          &SerialSession::clearReceiveArea);
  connect(m_btnSend, &QPushButton::clicked, this, &SerialSession::sendData);

  // Send Area Connections
  connect(m_btnClearSend, &QPushButton::clicked,
          [this]() { m_textSend->clear(); });
  connect(m_comboHistory, QOverload<int>::of(&QComboBox::activated),
          [this](int index) {
            if (index >= 0)
              m_textSend->setText(m_comboHistory->itemText(index));
          });

  connect(m_chkAutoSend, &QCheckBox::toggled, this,
          &SerialSession::toggleAutoSend);
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
    m_customPlot->replot();
    m_xValue = 0;
  });

  connect(m_btnResetChart, &QPushButton::clicked, [this]() {
    m_spinPoints->setValue(100);
    m_btnAutoScale->setChecked(true);
    m_chkShowGrid->setChecked(true);
    m_spinYTick->setValue(0);
    updateChartSettings();
  });

  // 波形显示及参数设置按钮控制
  connect(m_chkEnableWaveform, &QCheckBox::toggled, this,
          &SerialSession::onWaveformEnabled);

  // 波形参数设置面板控制
  connect(m_chkScopeSettings, &QCheckBox::toggled, this,
          [this](bool checked) { m_dockSettings->setVisible(checked); });

  // 双向绑定：用户手动关闭 dock 时，复选框也会联动取消勾选状态
  connect(m_dockSettings, &QDockWidget::visibilityChanged, this,
          [this](bool visible) {
            if (m_chkScopeSettings->isChecked() != visible) {
              m_chkScopeSettings->setChecked(visible);
            }
          });

  // 隐藏收发区控制
  connect(m_chkHideRxTx, &QCheckBox::toggled, this,
          [this](bool checked) { m_dataSplitter->setVisible(!checked); });

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
    m_serial->close();
    m_serial->close();
    m_serial->close();
    m_welcomeText =
        "   欢迎使用uSmilePro串口示波器V1.0   "; // Revert to default
    m_scrollPos = 0;
    ToastWidget::showToast("串口 " + m_serial->portName() + " 已关闭", false,
                           this);
    m_btnOpenClose->setText("打开串口");
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
      m_scrollPos = 0;              // Reset scroll position
      ToastWidget::showToast("串口 " + m_serial->portName() + " 已打开", true,
                             this);

      m_btnOpenClose->setText("关闭串口");
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
      QMessageBox::critical(this, "错误",
                            "无法打开串口:\n" + m_serial->errorString());
      m_btnOpenClose->setChecked(false);
    }
  }
}

void SerialSession::onPortError(QSerialPort::SerialPortError error) {
  if (error == QSerialPort::ResourceError) {
    QMessageBox::critical(this, "严重错误", "串口连接中断！");
    openClosePort(); // Force close UI state
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

    // Build HTML line: red timestamp + plain data
    QString htmlLine;
    if (m_chkRxTime->isChecked()) {
      QString timeStr =
          QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
      htmlLine = QString("<span style='color:red;'>%1</span>"
                         "<span>[RX] %2</span>")
                     .arg(timeStr.toHtmlEscaped())
                     .arg(rawStr.toHtmlEscaped());
    } else {
      htmlLine = QString("<span>[RX] %1</span>").arg(rawStr.toHtmlEscaped());
    }

    if (!m_chkHideRxData->isChecked() && !m_chkShowRawData->isChecked()) {
      m_textReceive->append(htmlLine);

      // Auto Scroll
      m_textReceive->verticalScrollBar()->setValue(
          m_textReceive->verticalScrollBar()->maximum());
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
  if (m_btnStopWaveform && m_btnStopWaveform->isChecked()) {
    return;
  }

  QString str = QString::fromLocal8Bit(data);
  static QString buffer;
  buffer.append(str);

  int maxPoints = m_spinPoints->value();
  bool autoScale = m_btnAutoScale->isChecked();
  bool dataAdded = false;

  while (true) {
    int startIdx = buffer.indexOf('$');
    if (startIdx == -1) {
      if (buffer.length() > 4096)
        buffer.clear(); // Safety check
      break;
    }

    int endIdx = buffer.indexOf(';', startIdx);
    if (endIdx == -1) {
      if (buffer.length() > 4096) {
        buffer = buffer.mid(startIdx);
        if (buffer.length() > 4096)
          buffer.clear(); // Safety clear
      }
      break; // Need more data
    }

    // Found complete frame: "$ ... ;"
    QString payload = buffer.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
    buffer.remove(0, endIdx + 1); // Remove processed frame

    if (payload.isEmpty())
      continue;

    QStringList parts = payload.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (parts.isEmpty())
      continue;

    // Ensure we have enough graphs
    int neededGraphs = parts.size();
    while (m_customPlot->graphCount() < neededGraphs) {
      int idx = m_customPlot->graphCount();
      m_customPlot->addGraph();

      // Assign distinct colors using HSV
      int hue = (idx * 137) % 360; // Golden angle approx
      QColor color = QColor::fromHsv(hue, 200, 200);
      QPen pen(color);
      pen.setWidthF(1.5f); // Bolder lines
      m_customPlot->graph(idx)->setPen(pen);
      m_customPlot->graph(idx)->setName(QString("CH%1").arg(idx + 1));
    }

    // Add data to graphs
    for (int i = 0; i < parts.size(); ++i) {
      bool ok;
      double val = parts[i].toDouble(&ok);
      if (ok) {
        m_customPlot->graph(i)->addData(m_xValue, val);
        if (m_customPlot->graph(i)->dataCount() > maxPoints) {
          m_customPlot->graph(i)->data()->removeBefore(m_xValue - maxPoints);
        }
      }
    }

    if (m_chkShowRawData->isChecked()) {
      m_textReceive->append(payload);
      m_textReceive->verticalScrollBar()->setValue(
          m_textReceive->verticalScrollBar()->maximum());
    }

    m_xValue++;
    dataAdded = true;
  }

  if (dataAdded) {
    m_customPlot->xAxis->setRange(m_xValue, maxPoints, Qt::AlignRight);
    if (autoScale) {
      for (int i = 0; i < m_customPlot->graphCount(); ++i) {
        if (i == 0)
          m_customPlot->graph(i)->rescaleValueAxis(false, true);
        else
          m_customPlot->graph(i)->rescaleValueAxis(true, true);
      }
    }
    if (!m_waveformPage->isHidden()) {
      m_customPlot->replot();
    }
  }
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

  // Build HTML line: blue timestamp + plain TX data
  QString txHtmlLine;
  if (m_chkTxTime->isChecked()) {
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    txHtmlLine = QString("<span style='color:blue;'>%1</span>"
                         "<span>[TX] %2</span>")
                     .arg(timeStr.toHtmlEscaped())
                     .arg(txRawStr.toHtmlEscaped());
  } else {
    txHtmlLine = QString("<span>[TX] %1</span>").arg(txRawStr.toHtmlEscaped());
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
  m_rxCount = 0;
  m_lblRxCount->setText("0");
  for (int i = 0; i < m_customPlot->graphCount(); ++i) {
    if (m_customPlot->graph(i) && m_customPlot->graph(i)->data()) {
      m_customPlot->graph(i)->data()->clear();
    }
  }
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

void SerialSession::onAutoSendTimeout() { sendData(); }

void SerialSession::scrollWelcomeMessage() {
  if (m_welcomeText.isEmpty())
    return;

  // Smooth scrolling using stylesheet text-indent
  // Double the text for seamless loop
  QString doubleText = m_welcomeText + "    " + m_welcomeText;
  m_lblWelcome->setText(doubleText);

  // Calculate text width for loop point
  QFontMetrics fm(m_lblWelcome->font());
  int singleTextWidth = fm.horizontalAdvance(m_welcomeText + "    ");

  // Increment scroll position smoothly (1 pixel per frame)
  m_scrollPos += 1;
  if (m_scrollPos >= singleTextWidth) {
    m_scrollPos = 0;
  }

  // Apply text-indent via stylesheet for smooth pixel movement
  m_lblWelcome->setStyleSheet(
      QString("color: blue; font-style: italic; text-indent: -%1px;")
          .arg(m_scrollPos));
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
  QString baseStyle = "QPushButton { "
                      "    border-radius: 10px; "
                      "    border: 1px solid #90A4AE; "
                      "    padding: 5px; "
                      "    font-family: 'Microsoft YaHei UI'; "
                      "    color: black; "
                      "}"
                      "QPushButton:hover { "
                      "    font-weight: 900; " // Flashy text on hover
                      "    font-size: 10pt; "
                      "}";

  QString gradient;
  switch (type) {
  case ButtonType::Normal:
    // Glossy Blue-Gray
    gradient = "QPushButton { "
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
        data.append('\n');
    }

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString txHtml =
        QString("<span style='color:blue;'>%1</span><span>[TX#%2] %3</span>")
            .arg(timeStr.toHtmlEscaped())
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
      data.append('\n');

    m_serial->write(data);
    m_txCount += data.size();
    m_lblTxCount->setText(QString::number(m_txCount));

    QString rawStr =
        isHex ? data.toHex(' ').toUpper() : QString::fromLocal8Bit(data);
    QString timeStr =
        QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    QString html =
        QString(
            "<span style='color:blue;'>%1</span><span>[LOOP P%2#%3] %4</span>")
            .arg(timeStr.toHtmlEscaped())
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
#include <QAction>
#include <QApplication>
#include <QFile>
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

  // --- 全局工具栏 (移自 SerialSession) ---
  QToolBar *toolbar = new QToolBar("Main Toolbar");
  toolbar->setMovable(false);

  // Settings
  toolbar->addAction("设置");

  // Oscilloscope Settings (Moved to individual tabs per user request)

  // --------- Theme (Menu) ---------
  QToolButton *btnTheme = new QToolButton();
  btnTheme->setText("主题");
  btnTheme->setPopupMode(QToolButton::InstantPopup);
  QMenu *menuTheme = new QMenu(btnTheme);

  // 1. Color Theme Submenu
  QMenu *menuColorTheme = menuTheme->addMenu("颜色主题");
  // Github
  QMenu *menuGithub = menuColorTheme->addMenu("Github");
  menuGithub->addAction("Dark", this, [this]() {
    applyGlobalTheme("githubdark.qss");
    emit themeChanged("dark");
  });
  menuGithub->addAction("Light", this, [this]() {
    applyGlobalTheme("githublight.qss");
    emit themeChanged("light");
  });
  // Aura
  QMenu *menuAura = menuColorTheme->addMenu("Aura");
  menuAura->addAction("Dark", this, [this]() {
    applyGlobalTheme("auradark.qss");
    emit themeChanged("dark");
  });
  menuAura->addAction("Light", this, [this]() {
    applyGlobalTheme("auralight.qss");
    emit themeChanged("light");
  });
  // ATOM
  QMenu *menuAtom = menuColorTheme->addMenu("ATOM");
  menuAtom->addAction("Dark", this, [this]() {
    applyGlobalTheme("atomone.qss");
    emit themeChanged("dark");
  });
  menuAtom->addAction("Light", this, [this]() {
    applyGlobalTheme("atomlight.qss");
    emit themeChanged("light");
  });
  // Solarized
  QMenu *menuSolarized = menuColorTheme->addMenu("Solarized");
  menuSolarized->addAction("Dark", this, [this]() {
    applyGlobalTheme("solarizeddark.qss");
    emit themeChanged("dark");
  });
  menuSolarized->addAction("Light", this, [this]() {
    applyGlobalTheme("solarizedlight.qss");
    emit themeChanged("light");
  });

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
  toolbar->addWidget(btnTheme);

  // Help
  toolbar->addAction("帮助");

  // About
  toolbar->addAction("关于");

  mainLayout->addWidget(toolbar);

  // --- 选项卡区域 ---
  m_sessionTabs = new QTabWidget(this);
  m_sessionTabs->setTabsClosable(true);
  m_sessionTabs->setMovable(true);

  // 应用仿浏览器圆角标签页样式
  m_sessionTabs->setStyleSheet(R"(
    QTabWidget::pane {
        border-top: 1px solid #C0C0C0;
        background-color: transparent;
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
        border-bottom-color: #FFFFFF; /* 与页面内容融为一体 */
    }
    QTabBar::tab:selected {
        margin-top: 0px; /* 选中的标签稍微凸起 */
        font-weight: bold;
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

  // 插入到 "  ➕  " 号前面
  int addIndex = m_sessionTabs->count() - 1;
  int index = m_sessionTabs->insertTab(addIndex, session, title);
  m_sessionTabs->setCurrentIndex(index);
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

  // 如果这是最后一个真实的选项卡（即总 count==2，1个真实 +
  // 1个"+"），关闭它直接退出整个应用
  if (m_sessionTabs->count() <= 2) {
    if (window()) {
      window()->close();
    }
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
}

void SerialPortPlot::applyFileIconTheme(const QString &themeName) {
  QIcon::setThemeName(themeName);
}
