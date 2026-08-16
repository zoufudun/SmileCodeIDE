#include "oscilloscopewindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Modbus CRC16 计算辅助函数
static quint16 calculateModbusCRC(const QByteArray &data) {
  quint16 crc = 0xFFFF;
  for (int pos = 0; pos < data.size(); pos++) {
    crc ^= static_cast<quint8>(data.at(pos));
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

OscilloscopeWindow::OscilloscopeWindow(QWidget *parent)
    : QMainWindow(parent), m_timeTracker() {
  setWindowTitle(QStringLiteral("独立多通信接口数字示波器 (Multi-Interface Digital Oscilloscope)"));
  resize(1360, 860);
  setMinimumSize(960, 600);

  // 设置示波器窗口图标 (ICON 图标 0xe86e)
  QPixmap winPix(32, 32);
  winPix.fill(Qt::transparent);
  {
    QPainter painter(&winPix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(CIconFont::instance()->getIconFont(26));
    painter.setPen(QColor("#0ea5e9"));
    painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, QString(QChar(0xe86e)));
  }
  setWindowIcon(QIcon(winPix));

  // 读取已保存的主题设置
  QSettings settings("PhudonTools", "Oscilloscope");
  int savedTheme = settings.value("theme", 0).toInt();
  if (savedTheme < 0 || savedTheme >= ScopeTheme::themeNames().size()) {
    savedTheme = 0;
  }

  initChannels();
  initUi();
  initPlots();
  initToolBar();
  initDocks();

  m_timeTracker.start();
  m_fpsTimer.start();

  // 定时器设置
  m_plotTimer = new QTimer(this);
  connect(m_plotTimer, &QTimer::timeout, this, &OscilloscopeWindow::onPlotRefreshTimeout);
  m_plotTimer->start(25); // ~40 FPS 顺滑刷新

  m_virtualTimer = new QTimer(this);
  connect(m_virtualTimer, &QTimer::timeout, this, &OscilloscopeWindow::onVirtualSignalTimeout);
  m_virtualTimer->start(10); // 100 Hz 模拟采样

  m_modbusTimer = new QTimer(this);
  connect(m_modbusTimer, &QTimer::timeout, this, &OscilloscopeWindow::onModbusPollTimeout);

  applyScopeTheme(savedTheme); // 应用主题
  appendLog(QStringLiteral("✓ 示波器系统启动就绪，支持 RS232/485、Modbus、CAN、以太网、USB及虚拟信号源"));
}

OscilloscopeWindow::~OscilloscopeWindow() {
  if (m_serialPort && m_serialPort->isOpen()) m_serialPort->close();
  if (m_tcpClient && m_tcpClient->isOpen()) m_tcpClient->close();
  if (m_tcpServer && m_tcpServer->isListening()) m_tcpServer->close();
  if (m_udpSocket && m_udpSocket->isOpen()) m_udpSocket->close();
  if (m_canInterface && m_canInterface->isOpen()) m_canInterface->closeDevice();
}

void OscilloscopeWindow::initChannels() {
  m_channels.clear();
  const QColor defaultColors[16] = {
      QColor("#00E5FF"), QColor("#FFD700"), QColor("#00E676"), QColor("#FF5252"),
      QColor("#E040FB"), QColor("#FF9100"), QColor("#40C4FF"), QColor("#EEFF41"),
      QColor("#FF4081"), QColor("#7C4DFF"), QColor("#1DE9B6"), QColor("#FF6E40"),
      QColor("#B388FF"), QColor("#69F0AE"), QColor("#FFAB40"), QColor("#EA80FC")};

  for (int i = 0; i < MAX_CHANNELS; ++i) {
    ScopeChannelConfig ch;
    ch.id = i;
    ch.name = QStringLiteral("CH%1").arg(i + 1);
    ch.enabled = (i < 4); // 默认启用前 4 通道
    ch.visible = (i < 4);
    ch.color = defaultColors[i % 16];
    ch.scale = 1.0;
    ch.offset = 0.0;
    ch.lineWidth = 2;
    m_channels.append(ch);
  }
}

void OscilloscopeWindow::initUi() {
  // 状态栏初始化
  m_lblStatus = new QLabel(QStringLiteral("● 运行中 (虚拟信号源)"), this);
  m_lblStatus->setStyleSheet("font-weight: bold; padding-left: 8px;");
  m_lblSampleRate = new QLabel(QStringLiteral("采样率: 100 Sa/s"), this);
  m_lblFps = new QLabel(QStringLiteral("帧率: 40 FPS"), this);

  statusBar()->addWidget(m_lblStatus);
  statusBar()->addPermanentWidget(m_lblSampleRate);
  statusBar()->addPermanentWidget(m_lblFps);
}

void OscilloscopeWindow::initPlots() {
  auto *centralSplitter = new QSplitter(Qt::Vertical, this);
  centralSplitter->setHandleWidth(3);

  // 1. 主时域波形图表 (Time Domain Plot)
  m_plot = new QCustomPlot(this);
  m_plot->setNoAntialiasingOnDrag(true);
  m_plot->setInteraction(QCP::iRangeDrag, true);
  m_plot->setInteraction(QCP::iRangeZoom, true);

  // 添加所有通道曲线
  for (int i = 0; i < MAX_CHANNELS; ++i) {
    QCPGraph *graph = m_plot->addGraph();
    graph->setName(m_channels[i].name);
    graph->setPen(QPen(m_channels[i].color, m_channels[i].lineWidth));
    graph->setVisible(m_channels[i].visible);
  }

  // 初始坐标系
  m_plot->xAxis->setRange(0, 5.0);
  m_plot->yAxis->setRange(-10.0, 10.0);
  m_plot->xAxis->setLabel(QStringLiteral("时间 (秒 / Time)"));
  m_plot->yAxis->setLabel(QStringLiteral("幅值 (伏特 / Amplitude)"));

  // 游标指示线
  m_cursorX1 = new QCPItemStraightLine(m_plot);
  m_cursorX1->setPen(QPen(QColor("#FFD700"), 1, Qt::DashLine));
  m_cursorX1->point1->setCoords(1.0, 0);
  m_cursorX1->point2->setCoords(1.0, 1);
  m_cursorX1->setVisible(false);

  m_cursorX2 = new QCPItemStraightLine(m_plot);
  m_cursorX2->setPen(QPen(QColor("#FF9100"), 1, Qt::DashLine));
  m_cursorX2->point1->setCoords(3.0, 0);
  m_cursorX2->point2->setCoords(3.0, 1);
  m_cursorX2->setVisible(false);

  m_cursorY1 = new QCPItemStraightLine(m_plot);
  m_cursorY1->setPen(QPen(QColor("#00E5FF"), 1, Qt::DashDotLine));
  m_cursorY1->point1->setCoords(0, 3.0);
  m_cursorY1->point2->setCoords(1, 3.0);
  m_cursorY1->setVisible(false);

  m_cursorY2 = new QCPItemStraightLine(m_plot);
  m_cursorY2->setPen(QPen(QColor("#00E676"), 1, Qt::DashDotLine));
  m_cursorY2->point1->setCoords(0, -3.0);
  m_cursorY2->point2->setCoords(1, -3.0);
  m_cursorY2->setVisible(false);

  // 触发电平虚线
  m_triggerLevelLine = new QCPItemStraightLine(m_plot);
  m_triggerLevelLine->setPen(QPen(QColor("#FF1744"), 1, Qt::DotLine));
  m_triggerLevelLine->point1->setCoords(0, 0.0);
  m_triggerLevelLine->point2->setCoords(1, 0.0);
  m_triggerLevelLine->setVisible(false);

  centralSplitter->addWidget(m_plot);

  // 2. 频域 FFT 频谱图表 (Frequency Domain Spectrum Plot)
  m_fftPlot = new QCustomPlot(this);
  m_fftPlot->plotLayout()->insertRow(0);
  m_fftPlot->xAxis->setLabel(QStringLiteral("频率 (Hz / Frequency)"));
  m_fftPlot->yAxis->setLabel(QStringLiteral("幅度 (dB / Amplitude)"));
  m_fftPlot->xAxis->setRange(0, 50.0);
  m_fftPlot->yAxis->setRange(0, 100.0);

  QCPGraph *fftGraph = m_fftPlot->addGraph();
  fftGraph->setName(QStringLiteral("FFT 频谱 (CH1)"));
  fftGraph->setPen(QPen(QColor("#00E5FF"), 1.5));
  fftGraph->setBrush(QBrush(QColor(0, 229, 255, 30)));

  centralSplitter->addWidget(m_fftPlot);
  centralSplitter->setSizes({600, 220});
  m_fftPlot->hide(); // 默认收起 FFT，可点击工具栏一键开启

  setCentralWidget(centralSplitter);
}

void OscilloscopeWindow::initToolBar() {
  auto *tb = addToolBar(QStringLiteral("示波器主工具栏"));
  tb->setIconSize(QSize(18, 18));
  tb->setMovable(false);

  // 1. 运行/暂停与单次触发
  m_btnRunStop = new QPushButton(QStringLiteral("⏸ 暂停"), this);
  m_btnRunStop->setCheckable(true);
  connect(m_btnRunStop, &QPushButton::clicked, this, [this](bool checked) {
    if (checked) {
      stopAcquisition();
      m_btnRunStop->setText(QStringLiteral("▶ 运行"));
      m_btnRunStop->setStyleSheet("background-color: #00E676; color: #0B0F19;");
    } else {
      startAcquisition();
      m_btnRunStop->setText(QStringLiteral("⏸ 暂停"));
      m_btnRunStop->setStyleSheet("");
    }
  });
  tb->addWidget(m_btnRunStop);

  m_btnSingle = new QPushButton(QStringLiteral("🎯 单次触发"), this);
  connect(m_btnSingle, &QPushButton::clicked, this, &OscilloscopeWindow::singleTrigger);
  tb->addWidget(m_btnSingle);

  m_btnClear = new QPushButton(QStringLiteral("🧹 清空波形"), this);
  connect(m_btnClear, &QPushButton::clicked, this, &OscilloscopeWindow::clearWaveforms);
  tb->addWidget(m_btnClear);

  tb->addSeparator();

  // 2. 接口选择与连接
  tb->addWidget(new QLabel(QStringLiteral(" 通信接口: "), this));
  m_comboInterface = new QComboBox(this);
  m_comboInterface->addItem(QStringLiteral("⚡ 虚拟信号发生器 (Virtual Wave)"), 0);
  m_comboInterface->addItem(QStringLiteral("🔌 RS232 / RS485 串口"), 1);
  m_comboInterface->addItem(QStringLiteral("🏭 Modbus (RTU / TCP)"), 2);
  m_comboInterface->addItem(QStringLiteral("🚗 CAN / CAN-FD 总线"), 3);
  m_comboInterface->addItem(QStringLiteral("🌐 以太网 (TCP / UDP)"), 4);
  connect(m_comboInterface, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &OscilloscopeWindow::onInterfaceTypeChanged);
  tb->addWidget(m_comboInterface);

  m_btnConnect = new QPushButton(QStringLiteral("🔌 打开连接"), this);
  connect(m_btnConnect, &QPushButton::clicked, this, &OscilloscopeWindow::toggleConnection);
  tb->addWidget(m_btnConnect);

  tb->addSeparator();

  // 3. 协议解析模式
  tb->addWidget(new QLabel(QStringLiteral(" 解析格式: "), this));
  m_comboProtocol = new QComboBox(this);
  m_comboProtocol->addItem(QStringLiteral("ASCII 逗号/文本流"), 0);
  m_comboProtocol->addItem(QStringLiteral("二进制 Float/Int 流"), 1);
  m_comboProtocol->addItem(QStringLiteral("标准帧格式 (0xAA 0x55)"), 2);
  m_comboProtocol->addItem(QStringLiteral("VOFA+ FireWater 协议"), 3);
  m_comboProtocol->addItem(QStringLiteral("JustFloat 协议"), 4);
  connect(m_comboProtocol, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    m_currentProtocol = static_cast<ScopeProtocolType>(idx);
    appendLog(QStringLiteral("已切换数据协议解析模式: %1").arg(m_comboProtocol->currentText()));
  });
  tb->addWidget(m_comboProtocol);

  tb->addSeparator();

  // 4. 时基调整与自适应
  tb->addWidget(new QLabel(QStringLiteral(" 时基 (Time/div): "), this));
  m_comboTimebase = new QComboBox(this);
  m_comboTimebase->addItem(QStringLiteral("1.0 秒/格 (全屏 5s)"), 5.0);
  m_comboTimebase->addItem(QStringLiteral("0.5 秒/格 (全屏 2.5s)"), 2.5);
  m_comboTimebase->addItem(QStringLiteral("0.2 秒/格 (全屏 1s)"), 1.0);
  m_comboTimebase->addItem(QStringLiteral("2.0 秒/格 (全屏 10s)"), 10.0);
  m_comboTimebase->addItem(QStringLiteral("5.0 秒/格 (全屏 25s)"), 25.0);
  connect(m_comboTimebase, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &OscilloscopeWindow::onTimebaseChanged);
  tb->addWidget(m_comboTimebase);

  m_btnAutoFit = new QPushButton(QStringLiteral("📐 垂直自适应"), this);
  connect(m_btnAutoFit, &QPushButton::clicked, this, &OscilloscopeWindow::onAutoFitYAxis);
  tb->addWidget(m_btnAutoFit);

  tb->addSeparator();

  // 5. 游标、FFT 与数据导出
  auto *btnCursor = new QPushButton(QStringLiteral("📏 双游标测量"), this);
  btnCursor->setCheckable(true);
  connect(btnCursor, &QPushButton::toggled, this, &OscilloscopeWindow::onCursorToggled);
  tb->addWidget(btnCursor);

  auto *btnFft = new QPushButton(QStringLiteral("📊 FFT 频谱"), this);
  btnFft->setCheckable(true);
  connect(btnFft, &QPushButton::toggled, this, &OscilloscopeWindow::onFftToggled);
  tb->addWidget(btnFft);

  tb->addSeparator();

  auto *btnExportCsv = new QPushButton(QStringLiteral("💾 导出 CSV"), this);
  connect(btnExportCsv, &QPushButton::clicked, this, &OscilloscopeWindow::exportDataCsv);
  tb->addWidget(btnExportCsv);

  auto *btnExportPng = new QPushButton(QStringLiteral("📷 截图"), this);
  connect(btnExportPng, &QPushButton::clicked, this, &OscilloscopeWindow::exportWaveformPng);
  tb->addWidget(btnExportPng);

  tb->addSeparator();

  // 6. 🎨 皮肤主题一键切换
  tb->addWidget(new QLabel(QStringLiteral(" 🎨 皮肤主题: "), this));
  m_comboTheme = new QComboBox(this);
  const auto themes = ScopeTheme::themeNames();
  for (int i = 0; i < themes.size(); ++i) {
    m_comboTheme->addItem(themes[i], i);
  }
  connect(m_comboTheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &OscilloscopeWindow::onThemeChanged);
  tb->addWidget(m_comboTheme);
}

void OscilloscopeWindow::initDocks() {
  // ===== 1. 接口配置 Dock =====
  m_dockInterface = new QDockWidget(QStringLiteral("⚙ 通信接口参数配置"), this);
  m_dockInterface->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  m_interfaceTabWidget = new QTabWidget(m_dockInterface);

  // Tab 0: 虚拟信号发生器
  auto *tabVirt = new QWidget(this);
  auto *layVirt = new QFormLayout(tabVirt);
  layVirt->setContentsMargins(12, 12, 12, 12);
  m_comboVirtWaveType = new QComboBox(tabVirt);
  m_comboVirtWaveType->addItem(QStringLiteral("正弦波 (Sine)"), 0);
  m_comboVirtWaveType->addItem(QStringLiteral("方波 (Square)"), 1);
  m_comboVirtWaveType->addItem(QStringLiteral("三角波 (Triangle)"), 2);
  m_comboVirtWaveType->addItem(QStringLiteral("锯齿波 (Sawtooth)"), 3);
  m_comboVirtWaveType->addItem(QStringLiteral("高斯白噪声 (Noise)"), 4);
  m_comboVirtWaveType->addItem(QStringLiteral("扫频信号 (Chirp)"), 5);
  layVirt->addRow(QStringLiteral("波形种类:"), m_comboVirtWaveType);

  m_spinVirtFreq = new QDoubleSpinBox(tabVirt);
  m_spinVirtFreq->setRange(0.1, 1000.0);
  m_spinVirtFreq->setValue(1.0);
  m_spinVirtFreq->setSuffix(QStringLiteral(" Hz"));
  layVirt->addRow(QStringLiteral("信号频率:"), m_spinVirtFreq);

  m_spinVirtAmp = new QDoubleSpinBox(tabVirt);
  m_spinVirtAmp->setRange(0.1, 100.0);
  m_spinVirtAmp->setValue(5.0);
  m_spinVirtAmp->setSuffix(QStringLiteral(" V"));
  layVirt->addRow(QStringLiteral("峰值幅值:"), m_spinVirtAmp);

  m_spinVirtNoise = new QDoubleSpinBox(tabVirt);
  m_spinVirtNoise->setRange(0.0, 10.0);
  m_spinVirtNoise->setValue(0.2);
  m_spinVirtNoise->setSuffix(QStringLiteral(" V"));
  layVirt->addRow(QStringLiteral("叠加噪声:"), m_spinVirtNoise);

  m_interfaceTabWidget->addTab(tabVirt, QStringLiteral("虚拟信号源"));

  // Tab 1: 串口配置 (RS232/485)
  auto *tabSerial = new QWidget(this);
  auto *laySerial = new QFormLayout(tabSerial);
  laySerial->setContentsMargins(12, 12, 12, 12);

  auto *portBox = new QHBoxLayout();
  m_comboPortName = new QComboBox(tabSerial);
  auto *btnRefreshPort = new QPushButton(QStringLiteral("🔄"), tabSerial);
  btnRefreshPort->setFixedWidth(32);
  connect(btnRefreshPort, &QPushButton::clicked, this, &OscilloscopeWindow::refreshSerialPorts);
  portBox->addWidget(m_comboPortName, 1);
  portBox->addWidget(btnRefreshPort);
  laySerial->addRow(QStringLiteral("串口号:"), portBox);

  m_comboBaudRate = new QComboBox(tabSerial);
  const QStringList bauds = {"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600", "2000000"};
  m_comboBaudRate->addItems(bauds);
  m_comboBaudRate->setCurrentText("115200");
  laySerial->addRow(QStringLiteral("波特率:"), m_comboBaudRate);

  m_comboDataBits = new QComboBox(tabSerial);
  m_comboDataBits->addItems({"8", "7", "6", "5"});
  laySerial->addRow(QStringLiteral("数据位:"), m_comboDataBits);

  m_comboParity = new QComboBox(tabSerial);
  m_comboParity->addItem(QStringLiteral("None (无校验)"), QSerialPort::NoParity);
  m_comboParity->addItem(QStringLiteral("Even (偶校验)"), QSerialPort::EvenParity);
  m_comboParity->addItem(QStringLiteral("Odd (奇校验)"), QSerialPort::OddParity);
  laySerial->addRow(QStringLiteral("校验位:"), m_comboParity);

  m_comboStopBits = new QComboBox(tabSerial);
  m_comboStopBits->addItem("1", QSerialPort::OneStop);
  m_comboStopBits->addItem("2", QSerialPort::TwoStop);
  laySerial->addRow(QStringLiteral("停止位:"), m_comboStopBits);

  m_interfaceTabWidget->addTab(tabSerial, QStringLiteral("RS232/485串口"));

  // Tab 2: Modbus 配置
  auto *tabModbus = new QWidget(this);
  auto *layModbus = new QFormLayout(tabModbus);
  layModbus->setContentsMargins(12, 12, 12, 12);

  m_comboModbusMode = new QComboBox(tabModbus);
  m_comboModbusMode->addItem(QStringLiteral("Modbus RTU (基于串口)"), 0);
  m_comboModbusMode->addItem(QStringLiteral("Modbus TCP (基于以太网)"), 1);
  layModbus->addRow(QStringLiteral("通信模式:"), m_comboModbusMode);

  m_spinSlaveId = new QSpinBox(tabModbus);
  m_spinSlaveId->setRange(1, 247);
  m_spinSlaveId->setValue(1);
  layModbus->addRow(QStringLiteral("从机站号:"), m_spinSlaveId);

  m_comboFunctionCode = new QComboBox(tabModbus);
  m_comboFunctionCode->addItem(QStringLiteral("03 读保持寄存器 (Holding)"), 3);
  m_comboFunctionCode->addItem(QStringLiteral("04 读输入寄存器 (Input)"), 4);
  m_comboFunctionCode->addItem(QStringLiteral("01 读线圈状态 (Coils)"), 1);
  m_comboFunctionCode->addItem(QStringLiteral("02 读离散输入 (Discrete)"), 2);
  layModbus->addRow(QStringLiteral("功能码:"), m_comboFunctionCode);

  m_spinModbusStartAddr = new QSpinBox(tabModbus);
  m_spinModbusStartAddr->setRange(0, 65535);
  m_spinModbusStartAddr->setValue(0);
  layModbus->addRow(QStringLiteral("起始地址:"), m_spinModbusStartAddr);

  m_spinModbusRegCount = new QSpinBox(tabModbus);
  m_spinModbusRegCount->setRange(1, 32);
  m_spinModbusRegCount->setValue(4);
  layModbus->addRow(QStringLiteral("寄存器数量:"), m_spinModbusRegCount);

  m_spinModbusInterval = new QSpinBox(tabModbus);
  m_spinModbusInterval->setRange(10, 5000);
  m_spinModbusInterval->setValue(50);
  m_spinModbusInterval->setSuffix(QStringLiteral(" ms"));
  layModbus->addRow(QStringLiteral("轮询周期:"), m_spinModbusInterval);

  m_comboModbusDataType = new QComboBox(tabModbus);
  m_comboModbusDataType->addItem(QStringLiteral("16位有符号整数 (Int16)"), 0);
  m_comboModbusDataType->addItem(QStringLiteral("16位无符号整数 (UInt16)"), 1);
  m_comboModbusDataType->addItem(QStringLiteral("32位浮点数 (Float32 ABCD)"), 2);
  m_comboModbusDataType->addItem(QStringLiteral("32位浮点数 (Float32 CDAB)"), 3);
  layModbus->addRow(QStringLiteral("数据格式:"), m_comboModbusDataType);

  m_interfaceTabWidget->addTab(tabModbus, QStringLiteral("Modbus"));

  // Tab 3: CAN 总线配置
  auto *tabCan = new QWidget(this);
  auto *layCan = new QFormLayout(tabCan);
  layCan->setContentsMargins(12, 12, 12, 12);

  m_comboCanDeviceType = new QComboBox(tabCan);
  m_comboCanDeviceType->addItem(QStringLiteral("创芯科技 ControlCAN (USBCAN)"), 1);
  m_comboCanDeviceType->addItem(QStringLiteral("致远电子 ZLG USBCAN-FD"), 2);
  m_comboCanDeviceType->addItem(QStringLiteral("虚拟测试 CAN 节点 (Virtual)"), 0);
  layCan->addRow(QStringLiteral("设备类型:"), m_comboCanDeviceType);

  m_spinCanDeviceIndex = new QSpinBox(tabCan);
  m_spinCanDeviceIndex->setRange(0, 10);
  m_spinCanDeviceIndex->setValue(0);
  layCan->addRow(QStringLiteral("设备索引:"), m_spinCanDeviceIndex);

  m_spinCanChannelIndex = new QSpinBox(tabCan);
  m_spinCanChannelIndex->setRange(0, 3);
  m_spinCanChannelIndex->setValue(0);
  layCan->addRow(QStringLiteral("通道索引:"), m_spinCanChannelIndex);

  m_comboCanBaud = new QComboBox(tabCan);
  m_comboCanBaud->addItems({"500Kbps", "250Kbps", "1Mbps", "125Kbps", "100Kbps"});
  layCan->addRow(QStringLiteral("波特率:"), m_comboCanBaud);

  m_editCanFilterId = new QLineEdit(tabCan);
  m_editCanFilterId->setPlaceholderText(QStringLiteral("留空接收全部, 或输入如 0x181"));
  layCan->addRow(QStringLiteral("CAN ID 过滤:"), m_editCanFilterId);

  m_interfaceTabWidget->addTab(tabCan, QStringLiteral("CAN总线"));

  // Tab 4: 以太网配置
  auto *tabEth = new QWidget(this);
  auto *layEth = new QFormLayout(tabEth);
  layEth->setContentsMargins(12, 12, 12, 12);

  m_comboEthMode = new QComboBox(tabEth);
  m_comboEthMode->addItem(QStringLiteral("TCP 客户端 (Client)"), 0);
  m_comboEthMode->addItem(QStringLiteral("TCP 服务端 (Server)"), 1);
  m_comboEthMode->addItem(QStringLiteral("UDP 数据报模式"), 2);
  layEth->addRow(QStringLiteral("以太网模式:"), m_comboEthMode);

  m_editEthIp = new QLineEdit(QStringLiteral("127.0.0.1"), tabEth);
  layEth->addRow(QStringLiteral("目标/本地 IP:"), m_editEthIp);

  m_spinEthPort = new QSpinBox(tabEth);
  m_spinEthPort->setRange(1, 65535);
  m_spinEthPort->setValue(8080);
  layEth->addRow(QStringLiteral("端口号:"), m_spinEthPort);

  m_interfaceTabWidget->addTab(tabEth, QStringLiteral("以太网"));

  m_dockInterface->setWidget(m_interfaceTabWidget);
  addDockWidget(Qt::LeftDockWidgetArea, m_dockInterface);

  // ===== 2. 通道管理 Dock =====
  m_dockChannels = new QDockWidget(QStringLiteral("🎛 通道参数配置 (1~16通道)"), this);
  m_tableChannels = new QTableWidget(MAX_CHANNELS, 6, this);
  m_tableChannels->setAlternatingRowColors(true);
  m_tableChannels->setShowGrid(true);
  m_tableChannels->setHorizontalHeaderLabels({
      QStringLiteral("开"), QStringLiteral("名称"), QStringLiteral("颜色"),
      QStringLiteral("比例 (V/div)"), QStringLiteral("偏置 (V)"), QStringLiteral("当前值")});
  m_tableChannels->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_tableChannels->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_tableChannels->verticalHeader()->setDefaultSectionSize(28);

  for (int i = 0; i < MAX_CHANNELS; ++i) {
    auto *chk = new QCheckBox();
    chk->setChecked(m_channels[i].enabled);
    chk->setStyleSheet("margin-left: 6px;");
    connect(chk, &QCheckBox::toggled, this, [this, i](bool checked) {
      m_channels[i].enabled = checked;
      m_channels[i].visible = checked;
      if (m_plot && i < m_plot->graphCount()) {
        m_plot->graph(i)->setVisible(checked);
        m_plot->replot(QCustomPlot::rpQueuedReplot);
      }
    });
    m_tableChannels->setCellWidget(i, 0, chk);

    auto *editName = new QLineEdit(m_channels[i].name);
    connect(editName, &QLineEdit::textChanged, this, [this, i](const QString &text) {
      m_channels[i].name = text;
      if (m_plot && i < m_plot->graphCount()) {
        m_plot->graph(i)->setName(text);
      }
    });
    m_tableChannels->setCellWidget(i, 1, editName);

    auto *btnColor = new QPushButton();
    btnColor->setFixedWidth(28);
    connect(btnColor, &QPushButton::clicked, this, [this, i, btnColor]() {
      QColor c = QColorDialog::getColor(m_channels[i].color, this, QStringLiteral("选择通道颜色"));
      if (c.isValid()) {
        m_channels[i].color = c;
        btnColor->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(c.name()));
        if (m_plot && i < m_plot->graphCount()) {
          m_plot->graph(i)->setPen(QPen(c, m_channels[i].lineWidth));
          m_plot->replot(QCustomPlot::rpQueuedReplot);
        }
      }
    });
    m_tableChannels->setCellWidget(i, 2, btnColor);

    auto *spinScale = new QDoubleSpinBox();
    spinScale->setRange(0.001, 1000.0);
    spinScale->setValue(m_channels[i].scale);
    connect(spinScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i](double val) {
      m_channels[i].scale = val;
    });
    m_tableChannels->setCellWidget(i, 3, spinScale);

    auto *spinOffset = new QDoubleSpinBox();
    spinOffset->setRange(-1000.0, 1000.0);
    spinOffset->setValue(m_channels[i].offset);
    connect(spinOffset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i](double val) {
      m_channels[i].offset = val;
    });
    m_tableChannels->setCellWidget(i, 4, spinOffset);

    auto *lblCur = new QLabel(QStringLiteral("0.000"));
    lblCur->setAlignment(Qt::AlignCenter);
    m_tableChannels->setCellWidget(i, 5, lblCur);
  }

  m_dockChannels->setWidget(m_tableChannels);
  addDockWidget(Qt::LeftDockWidgetArea, m_dockChannels);

  // ===== 3. 测量统计 Dock =====
  m_dockMeasure = new QDockWidget(QStringLiteral("📊 实时特征测量统计"), this);
  m_tableMeasure = new QTableWidget(MAX_CHANNELS, 7, this);
  m_tableMeasure->setAlternatingRowColors(true);
  m_tableMeasure->setShowGrid(true);
  m_tableMeasure->setHorizontalHeaderLabels({
      QStringLiteral("通道"), QStringLiteral("峰峰值 Vpp"), QStringLiteral("最大值 Max"),
      QStringLiteral("最小值 Min"), QStringLiteral("平均值 Avg"), QStringLiteral("有效值 Rms"),
      QStringLiteral("频率 Freq")});
  m_tableMeasure->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_tableMeasure->verticalHeader()->setDefaultSectionSize(26);
  for (int i = 0; i < MAX_CHANNELS; ++i) {
    m_tableMeasure->setItem(i, 0, new QTableWidgetItem(m_channels[i].name));
    for (int col = 1; col < 7; ++col) {
      m_tableMeasure->setItem(i, col, new QTableWidgetItem(QStringLiteral("--")));
    }
  }
  m_dockMeasure->setWidget(m_tableMeasure);
  addDockWidget(Qt::BottomDockWidgetArea, m_dockMeasure);

  // ===== 4. 日志 Dock =====
  m_dockLog = new QDockWidget(QStringLiteral("📝 示波器通信日志"), this);
  m_textLog = new QTextEdit(this);
  m_textLog->setReadOnly(true);
  m_dockLog->setWidget(m_textLog);
  addDockWidget(Qt::BottomDockWidgetArea, m_dockLog);

  refreshSerialPorts();
}

void OscilloscopeWindow::refreshSerialPorts() {
  if (!m_comboPortName) return;
  m_comboPortName->clear();
  const auto ports = QSerialPortInfo::availablePorts();
  for (const auto &p : ports) {
    m_comboPortName->addItem(QString("%1 (%2)").arg(p.portName(), p.description()), p.portName());
  }
  if (m_comboPortName->count() == 0) {
    m_comboPortName->addItem(QStringLiteral("未检测到串口"), "");
  }
}

void OscilloscopeWindow::onInterfaceTypeChanged(int index) {
  m_currentInterface = static_cast<ScopeInterfaceType>(index);
  if (m_interfaceTabWidget) {
    m_interfaceTabWidget->setCurrentIndex(index);
  }
  appendLog(QStringLiteral("已切换通信接口: %1").arg(m_comboInterface->currentText()));
}

void OscilloscopeWindow::toggleConnection() {
  if (m_isConnected) {
    // 断开连接
    if (m_serialPort && m_serialPort->isOpen()) m_serialPort->close();
    if (m_tcpClient && m_tcpClient->isOpen()) m_tcpClient->close();
    if (m_tcpServer && m_tcpServer->isListening()) m_tcpServer->close();
    if (m_udpSocket && m_udpSocket->isOpen()) m_udpSocket->close();
    if (m_canInterface && m_canInterface->isOpen()) m_canInterface->closeDevice();
    if (m_modbusTimer) m_modbusTimer->stop();

    m_isConnected = false;
    m_btnConnect->setText(QStringLiteral("🔌 打开连接"));
    m_btnConnect->setStyleSheet("");
    m_lblStatus->setText(QStringLiteral("● 已断开连接"));
    m_lblStatus->setStyleSheet("color: #EF4444; font-weight: bold;");
    appendLog(QStringLiteral("已断开当前通信接口"));
    return;
  }

  // 建立连接
  switch (m_currentInterface) {
  case ScopeInterfaceType::VirtualSignal:
    m_isConnected = true;
    break;

  case ScopeInterfaceType::SerialPort: {
    QString port = m_comboPortName->currentData().toString();
    if (port.isEmpty()) port = m_comboPortName->currentText().split(" ").first();
    if (port.isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("串口错误"), QStringLiteral("请先选择有效串口！"));
      return;
    }
    if (!m_serialPort) {
      m_serialPort = new QSerialPort(this);
      connect(m_serialPort, &QSerialPort::readyRead, this, &OscilloscopeWindow::onSerialReadyRead);
      connect(m_serialPort, &QSerialPort::errorOccurred, this, &OscilloscopeWindow::onSerialPortError);
    }
    m_serialPort->setPortName(port);
    m_serialPort->setBaudRate(m_comboBaudRate->currentText().toInt());
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(m_comboDataBits->currentText().toInt()));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(m_comboParity->currentData().toInt()));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(m_comboStopBits->currentData().toInt()));

    if (m_serialPort->open(QIODevice::ReadWrite)) {
      m_isConnected = true;
      appendLog(QStringLiteral("✓ 串口 %1 打开成功 (波特率: %2)").arg(port, m_comboBaudRate->currentText()));
    } else {
      QMessageBox::critical(this, QStringLiteral("打开串口失败"), m_serialPort->errorString());
      return;
    }
    break;
  }

  case ScopeInterfaceType::Modbus: {
    int mode = m_comboModbusMode->currentIndex();
    if (mode == 0) { // RTU 串口模式
      QString port = m_comboPortName->currentData().toString();
      if (!m_serialPort) {
        m_serialPort = new QSerialPort(this);
        connect(m_serialPort, &QSerialPort::readyRead, this, &OscilloscopeWindow::onSerialReadyRead);
      }
      m_serialPort->setPortName(port);
      m_serialPort->setBaudRate(m_comboBaudRate->currentText().toInt());
      if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
      } else {
        QMessageBox::critical(this, QStringLiteral("Modbus RTU 串口错误"), m_serialPort->errorString());
        return;
      }
    } else { // TCP 模式
      if (!m_tcpClient) {
        m_tcpClient = new QTcpSocket(this);
        connect(m_tcpClient, &QTcpSocket::readyRead, this, &OscilloscopeWindow::onTcpReadyRead);
      }
      m_tcpClient->connectToHost(m_editEthIp->text().trimmed(), m_spinEthPort->value());
      if (m_tcpClient->waitForConnected(2000)) {
        m_isConnected = true;
      } else {
        QMessageBox::critical(this, QStringLiteral("Modbus TCP 连接失败"), m_tcpClient->errorString());
        return;
      }
    }
    m_modbusTimer->start(m_spinModbusInterval->value());
    appendLog(QStringLiteral("✓ Modbus 引擎启动，轮询间隔: %1 ms").arg(m_spinModbusInterval->value()));
    break;
  }

  case ScopeInterfaceType::CanBus: {
    if (!m_canInterface) {
      m_canInterface = new CanInterface(this);
      connect(m_canInterface, &CanInterface::frameReceived, this, &OscilloscopeWindow::onCanFrameReceived);
    }
    quint32 devType = CX_USBCAN2;
    int typeIdx = m_comboCanDeviceType->currentIndex();
    if (typeIdx == 0) devType = CX_USBCAN2;
    else if (typeIdx == 1) devType = 41; // ZCAN_USBCANFD_200U
    else devType = 0; // Virtual

    int devIdx = m_spinCanDeviceIndex->value();
    int chIdx = m_spinCanChannelIndex->value();
    int baud = 500000;
    QString baudStr = m_comboCanBaud->currentText();
    if (baudStr.contains("1M")) baud = 1000000;
    else if (baudStr.contains("250K")) baud = 250000;
    else if (baudStr.contains("125K")) baud = 125000;
    else if (baudStr.contains("100K")) baud = 100000;

    if (m_canInterface->open(devType, devIdx, chIdx, baud, false, 2000000, CanMode::Normal, true)) {
      m_isConnected = true;
      appendLog(QStringLiteral("✓ CAN 总线设备打开成功 (设备:%1, 通道:%2, 波特率:%3)").arg(devIdx).arg(chIdx).arg(baudStr));
    } else {
      QMessageBox::critical(this, QStringLiteral("CAN 设备打开失败"),
                            m_canInterface->lastError().isEmpty() ? QStringLiteral("无法打开所选 CAN 接口，请确认驱动与硬件连接正常！") : m_canInterface->lastError());
      return;
    }
    break;
  }

  case ScopeInterfaceType::Ethernet: {
    int ethMode = m_comboEthMode->currentIndex();
    if (ethMode == 0) { // TCP Client
      if (!m_tcpClient) {
        m_tcpClient = new QTcpSocket(this);
        connect(m_tcpClient, &QTcpSocket::connected, this, &OscilloscopeWindow::onTcpSocketConnected);
        connect(m_tcpClient, &QTcpSocket::disconnected, this, &OscilloscopeWindow::onTcpSocketDisconnected);
        connect(m_tcpClient, &QTcpSocket::readyRead, this, &OscilloscopeWindow::onTcpReadyRead);
      }
      m_tcpClient->connectToHost(m_editEthIp->text().trimmed(), m_spinEthPort->value());
      if (m_tcpClient->waitForConnected(2000)) {
        m_isConnected = true;
      } else {
        QMessageBox::critical(this, QStringLiteral("TCP 连接失败"), m_tcpClient->errorString());
        return;
      }
    } else if (ethMode == 1) { // TCP Server
      if (!m_tcpServer) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &OscilloscopeWindow::onTcpNewConnection);
      }
      if (m_tcpServer->listen(QHostAddress::Any, m_spinEthPort->value())) {
        m_isConnected = true;
        appendLog(QStringLiteral("✓ TCP 服务端已在端口 %1 开启监听").arg(m_spinEthPort->value()));
      } else {
        QMessageBox::critical(this, QStringLiteral("TCP 服务端启动失败"), m_tcpServer->errorString());
        return;
      }
    } else { // UDP
      if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &OscilloscopeWindow::onUdpReadyRead);
      }
      if (m_udpSocket->bind(m_spinEthPort->value(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_isConnected = true;
        appendLog(QStringLiteral("✓ UDP 端口 %1 绑定监听成功").arg(m_spinEthPort->value()));
      } else {
        QMessageBox::critical(this, QStringLiteral("UDP 绑定失败"), m_udpSocket->errorString());
        return;
      }
    }
    break;
  }
  default:
    m_isConnected = true;
    break;
  }

  m_btnConnect->setText(QStringLiteral("❌ 断开连接"));
  m_btnConnect->setStyleSheet("background-color: #EF4444; color: white; border-color: #EF4444;");
  m_lblStatus->setText(QStringLiteral("● 已连接 (%1)").arg(m_comboInterface->currentText()));
  m_lblStatus->setStyleSheet("color: #00E5FF; font-weight: bold;");
}

void OscilloscopeWindow::startAcquisition() {
  m_isRunning = true;
  m_lblStatus->setText(QStringLiteral("● 采集运行中"));
  m_lblStatus->setStyleSheet("color: #00E676; font-weight: bold;");
}

void OscilloscopeWindow::stopAcquisition() {
  m_isRunning = false;
  m_lblStatus->setText(QStringLiteral("⏸ 采集已暂停"));
  m_lblStatus->setStyleSheet("color: #FFB300; font-weight: bold;");
}

void OscilloscopeWindow::singleTrigger() {
  m_waitingSingleTrigger = true;
  m_isRunning = true;
  appendLog(QStringLiteral("⚡ 示波器已进入单次触发等待状态..."));
}

void OscilloscopeWindow::clearWaveforms() {
  for (int i = 0; i < MAX_CHANNELS; ++i) {
    m_channels[i].xData.clear();
    m_channels[i].yData.clear();
    if (m_plot && i < m_plot->graphCount()) {
      m_plot->graph(i)->data()->clear();
    }
  }
  if (m_fftPlot && m_fftPlot->graphCount() > 0) {
    m_fftPlot->graph(0)->data()->clear();
    m_fftPlot->replot();
  }
  m_currentSimTime = 0.0;
  m_timeTracker.restart();
  m_plot->xAxis->setRange(0, m_comboTimebase->currentData().toDouble());
  m_plot->replot(QCustomPlot::rpQueuedReplot);
  appendLog(QStringLiteral("已清空所有通道波形缓存"));
}

void OscilloscopeWindow::onTimebaseChanged(int index) {
  Q_UNUSED(index);
  double timeRange = m_comboTimebase->currentData().toDouble();
  if (timeRange <= 0.1) timeRange = 5.0;
  m_plot->xAxis->setRange(m_currentSimTime > timeRange ? (m_currentSimTime - timeRange) : 0.0,
                          qMax(timeRange, m_currentSimTime));
  m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void OscilloscopeWindow::onAutoFitYAxis() {
  double minY = 1e9, maxY = -1e9;
  bool hasData = false;

  for (int i = 0; i < MAX_CHANNELS; ++i) {
    if (!m_channels[i].enabled || m_channels[i].yData.isEmpty()) continue;
    for (double v : m_channels[i].yData) {
      minY = qMin(minY, v);
      maxY = qMax(maxY, v);
      hasData = true;
    }
  }

  if (hasData) {
    double span = maxY - minY;
    if (span < 0.1) span = 1.0;
    m_plot->yAxis->setRange(minY - span * 0.15, maxY + span * 0.15);
    m_plot->replot(QCustomPlot::rpQueuedReplot);
    appendLog(QStringLiteral("✓ 垂直量程自适应完毕: [%1 V, %2 V]").arg(minY, 0, 'f', 2).arg(maxY, 0, 'f', 2));
  }
}

void OscilloscopeWindow::resetDefaultView() {
  m_plot->xAxis->setRange(0, 5.0);
  m_plot->yAxis->setRange(-10.0, 10.0);
  m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void OscilloscopeWindow::onVirtualSignalTimeout() {
  if (!m_isRunning || m_currentInterface != ScopeInterfaceType::VirtualSignal) return;

  m_currentSimTime += 0.01;
  double t = m_currentSimTime;
  double freq = m_spinVirtFreq->value();
  double amp = m_spinVirtAmp->value();
  double noiseLevel = m_spinVirtNoise->value();
  int waveType = m_comboVirtWaveType->currentIndex();

  for (int i = 0; i < 8; ++i) {
    if (!m_channels[i].enabled) continue;
    double phase = i * (M_PI / 4.0);
    double y = 0.0;
    double w = 2.0 * M_PI * freq * t + phase;

    switch (waveType) {
    case 0: // 正弦波
      y = amp * std::sin(w);
      break;
    case 1: // 方波
      y = (std::sin(w) >= 0) ? amp : -amp;
      break;
    case 2: // 三角波
      y = amp * (2.0 / M_PI) * std::asin(std::sin(w));
      break;
    case 3: // 锯齿波
      y = amp * (2.0 * (t * freq - std::floor(t * freq + 0.5)));
      break;
    case 4: // 噪声
      y = (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * amp;
      break;
    case 5: // 扫频
      y = amp * std::sin(2.0 * M_PI * (freq + t * 0.5) * t + phase);
      break;
    }

    if (noiseLevel > 0.001) {
      y += (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * noiseLevel;
    }

    appendSample(i, t, y);
  }
}

void OscilloscopeWindow::appendSample(int channelIdx, double xVal, double yVal) {
  if (channelIdx < 0 || channelIdx >= MAX_CHANNELS) return;

  // 经过增益与偏置转换
  double finalY = yVal * m_channels[channelIdx].scale + m_channels[channelIdx].offset;

  double prevY = m_channels[channelIdx].yData.isEmpty() ? finalY : m_channels[channelIdx].yData.last();
  m_channels[channelIdx].xData.append(xVal);
  m_channels[channelIdx].yData.append(finalY);

  // 内存环形缓冲区限制（保留最多 50000 点，防止超长运行爆内存）
  const int MAX_POINTS = 50000;
  if (m_channels[channelIdx].xData.size() > MAX_POINTS) {
    m_channels[channelIdx].xData.remove(0, 1000);
    m_channels[channelIdx].yData.remove(0, 1000);
  }

  // 触发条件判断
  checkTriggerCondition(channelIdx, prevY, finalY);
}

void OscilloscopeWindow::checkTriggerCondition(int channelIdx, double prevY, double curY) {
  if (!m_waitingSingleTrigger) return;
  if (channelIdx != m_triggerChannel) return;

  bool triggered = false;
  if (m_triggerEdge == 0) { // 上升沿
    if (prevY < m_triggerLevel && curY >= m_triggerLevel) triggered = true;
  } else { // 下降沿
    if (prevY > m_triggerLevel && curY <= m_triggerLevel) triggered = true;
  }

  if (triggered) {
    m_waitingSingleTrigger = false;
    stopAcquisition();
    appendLog(QStringLiteral("🎯 单次触发捕获成功 (CH%1, 电平: %2 V)").arg(channelIdx + 1).arg(curY, 0, 'f', 2));
  }
}

void OscilloscopeWindow::onPlotRefreshTimeout() {
  if (!m_isRunning) return;

  double timeRange = m_comboTimebase->currentData().toDouble();
  if (timeRange <= 0.1) timeRange = 5.0;

  // 1. 同步数据至 QCustomPlot Graph
  for (int i = 0; i < MAX_CHANNELS; ++i) {
    if (!m_channels[i].enabled) continue;
    m_plot->graph(i)->setData(m_channels[i].xData, m_channels[i].yData);

    // 更新通道表格实时读数
    if (!m_channels[i].yData.isEmpty()) {
      double cur = m_channels[i].yData.last();
      auto *lbl = qobject_cast<QLabel *>(m_tableChannels->cellWidget(i, 5));
      if (lbl) lbl->setText(QString::number(cur, 'f', 3));
    }
  }

  // 2. X 轴滚屏跟随
  if (m_currentSimTime > timeRange) {
    m_plot->xAxis->setRange(m_currentSimTime - timeRange, m_currentSimTime);
  } else {
    m_plot->xAxis->setRange(0, timeRange);
  }

  m_plot->replot(QCustomPlot::rpQueuedReplot);

  // 3. 计算并刷新特征值与 FFT
  for (int i = 0; i < 4; ++i) {
    if (m_channels[i].enabled) {
      calculateChannelStatistics(i);
    }
  }
  if (m_fftPlot->isVisible() && m_channels[0].enabled) {
    calculateFft(0);
  }

  // 4. 统计帧率
  m_frameCount++;
  if (m_fpsTimer.elapsed() >= 1000) {
    m_lblFps->setText(QStringLiteral("帧率: %1 FPS").arg(m_frameCount));
    m_frameCount = 0;
    m_fpsTimer.restart();
  }
}

void OscilloscopeWindow::calculateChannelStatistics(int channelIdx) {
  const auto &yData = m_channels[channelIdx].yData;
  if (yData.isEmpty()) return;

  double minVal = 1e9, maxVal = -1e9, sum = 0.0, sumSq = 0.0;
  int count = qMin(500, yData.size());
  int startIdx = yData.size() - count;

  for (int i = startIdx; i < yData.size(); ++i) {
    double v = yData[i];
    minVal = qMin(minVal, v);
    maxVal = qMax(maxVal, v);
    sum += v;
    sumSq += v * v;
  }

  double avg = sum / count;
  double rms = std::sqrt(sumSq / count);
  double vpp = maxVal - minVal;

  m_tableMeasure->item(channelIdx, 1)->setText(QString::number(vpp, 'f', 2) + " V");
  m_tableMeasure->item(channelIdx, 2)->setText(QString::number(maxVal, 'f', 2) + " V");
  m_tableMeasure->item(channelIdx, 3)->setText(QString::number(minVal, 'f', 2) + " V");
  m_tableMeasure->item(channelIdx, 4)->setText(QString::number(avg, 'f', 2) + " V");
  m_tableMeasure->item(channelIdx, 5)->setText(QString::number(rms, 'f', 2) + " V");
  m_tableMeasure->item(channelIdx, 6)->setText(QString::number(m_spinVirtFreq->value(), 'f', 1) + " Hz");
}

void OscilloscopeWindow::calculateFft(int channelIdx) {
  const auto &yData = m_channels[channelIdx].yData;
  if (yData.size() < 128) return;

  const int N = 256;
  int start = qMax(0, yData.size() - N);
  QVector<double> freqs(N / 2);
  QVector<double> mags(N / 2);

  double samplingRate = 100.0; // 假设 100Hz
  for (int k = 0; k < N / 2; ++k) {
    double real = 0.0, imag = 0.0;
    for (int n = 0; n < N; ++n) {
      double val = (start + n < yData.size()) ? yData[start + n] : 0.0;
      double angle = 2.0 * M_PI * k * n / N;
      real += val * std::cos(angle);
      imag -= val * std::sin(angle);
    }
    freqs[k] = k * (samplingRate / N);
    mags[k] = std::sqrt(real * real + imag * imag) * (2.0 / N);
  }

  m_fftPlot->graph(0)->setData(freqs, mags);
  m_fftPlot->xAxis->setRange(0, samplingRate / 2);
  m_fftPlot->yAxis->rescale();
  m_fftPlot->replot(QCustomPlot::rpQueuedReplot);
}

// 通信数据接收与协议解析
void OscilloscopeWindow::onSerialReadyRead() {
  if (!m_serialPort) return;
  QByteArray raw = m_serialPort->readAll();
  processRawIncomingBytes(raw);
}

void OscilloscopeWindow::onTcpReadyRead() {
  if (!m_tcpClient) return;
  QByteArray raw = m_tcpClient->readAll();
  processRawIncomingBytes(raw);
}

void OscilloscopeWindow::onUdpReadyRead() {
  if (!m_udpSocket) return;
  while (m_udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(m_udpSocket->pendingDatagramSize());
    m_udpSocket->readDatagram(datagram.data(), datagram.size());
    processRawIncomingBytes(datagram);
  }
}

void OscilloscopeWindow::onTcpNewConnection() {
  if (!m_tcpServer) return;
  QTcpSocket *client = m_tcpServer->nextPendingConnection();
  if (client) {
    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
      QByteArray raw = client->readAll();
      processRawIncomingBytes(raw);
    });
    appendLog(QStringLiteral("✓ 新客户端已连接: %1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
  }
}

void OscilloscopeWindow::onCanFrameReceived(const CanFrame &frame) {
  if (!m_isRunning) return;

  m_currentSimTime += 0.01;
  if (frame.data.size() < 2) return;

  // 解析 CAN 报文数据字段（通道 0/1/2/3 分别映射前 4 个 16 位整数）
  for (int ch = 0; ch < 4 && (ch * 2 + 1) < frame.data.size(); ++ch) {
    if (!m_channels[ch].enabled) continue;
    qint16 val = static_cast<qint16>((static_cast<quint8>(frame.data[ch * 2 + 1]) << 8) |
                                     static_cast<quint8>(frame.data[ch * 2]));
    appendSample(ch, m_currentSimTime, val * 0.01);
  }
}

void OscilloscopeWindow::processRawIncomingBytes(const QByteArray &data) {
  if (!m_isRunning) return;

  if (m_currentProtocol == ScopeProtocolType::AsciiCsv) {
    m_rxTextBuffer.append(QString::fromUtf8(data));
    int newlineIdx = -1;
    while ((newlineIdx = m_rxTextBuffer.indexOf('\n')) >= 0) {
      QString line = m_rxTextBuffer.left(newlineIdx).trimmed();
      m_rxTextBuffer.remove(0, newlineIdx + 1);
      if (!line.isEmpty()) {
        parseAsciiTextStream(line);
      }
    }
  } else if (m_currentProtocol == ScopeProtocolType::BinaryStream) {
    parseBinaryStream(data);
  } else if (m_currentProtocol == ScopeProtocolType::VofaFireWater) {
    parseVofaStream(data);
  }
}

void OscilloscopeWindow::parseAsciiTextStream(const QString &text) {
  m_currentSimTime += 0.01;
  QStringList tokens = text.split(QRegExp("[,;\\s]+"), Qt::SkipEmptyParts);
  for (int i = 0; i < tokens.size() && i < MAX_CHANNELS; ++i) {
    bool ok = false;
    double val = tokens[i].toDouble(&ok);
    if (ok && m_channels[i].enabled) {
      appendSample(i, m_currentSimTime, val);
    }
  }
}

void OscilloscopeWindow::parseBinaryStream(const QByteArray &data) {
  m_currentSimTime += 0.01;
  int floatCount = data.size() / sizeof(float);
  const float *floats = reinterpret_cast<const float *>(data.constData());
  for (int i = 0; i < floatCount && i < MAX_CHANNELS; ++i) {
    if (m_channels[i].enabled) {
      appendSample(i, m_currentSimTime, static_cast<double>(floats[i]));
    }
  }
}

void OscilloscopeWindow::parseVofaStream(const QByteArray &data) {
  m_rxRawBuffer.append(data);
  const QByteArray tail = QByteArray::fromHex("0000807f"); // VOFA+ Tail
  int tailIdx = -1;
  while ((tailIdx = m_rxRawBuffer.indexOf(tail)) >= 0) {
    QByteArray frame = m_rxRawBuffer.left(tailIdx);
    m_rxRawBuffer.remove(0, tailIdx + tail.size());
    parseBinaryStream(frame);
  }
}

void OscilloscopeWindow::onModbusPollTimeout() {
  if (!m_isConnected) return;
  sendModbusPollRequest();
}

void OscilloscopeWindow::sendModbusPollRequest() {
  quint8 slaveId = m_spinSlaveId->value();
  quint8 func = m_comboFunctionCode->currentData().toInt();
  quint16 startAddr = m_spinModbusStartAddr->value();
  quint16 regCount = m_spinModbusRegCount->value();

  QByteArray req;
  req.append(static_cast<char>(slaveId));
  req.append(static_cast<char>(func));
  req.append(static_cast<char>((startAddr >> 8) & 0xFF));
  req.append(static_cast<char>(startAddr & 0xFF));
  req.append(static_cast<char>((regCount >> 8) & 0xFF));
  req.append(static_cast<char>(regCount & 0xFF));

  quint16 crc = calculateModbusCRC(req);
  req.append(static_cast<char>(crc & 0xFF));
  req.append(static_cast<char>((crc >> 8) & 0xFF));

  if (m_serialPort && m_serialPort->isOpen()) {
    m_serialPort->write(req);
  } else if (m_tcpClient && m_tcpClient->isOpen()) {
    // Modbus TCP Header (MBAP: Transaction ID, Protocol 0, Length, UnitID)
    QByteArray tcpReq;
    tcpReq.append(QByteArray::fromHex("000100000006"));
    tcpReq.append(req.left(6)); // 去掉末尾 CRC
    m_tcpClient->write(tcpReq);
  }
}

void OscilloscopeWindow::onCursorToggled(bool checked) {
  m_showCursors = checked;
  m_cursorX1->setVisible(checked);
  m_cursorX2->setVisible(checked);
  m_cursorY1->setVisible(checked);
  m_cursorY2->setVisible(checked);
  m_plot->replot(QCustomPlot::rpQueuedReplot);
  appendLog(checked ? QStringLiteral("开启双游标测量模式") : QStringLiteral("关闭游标测量"));
}

void OscilloscopeWindow::onFftToggled(bool checked) {
  m_fftPlot->setVisible(checked);
}

void OscilloscopeWindow::onTriggerSettingsChanged() {}
void OscilloscopeWindow::onThemeChanged(int themeIndex) { applyScopeTheme(themeIndex); }

void OscilloscopeWindow::applyScopeTheme(int theme) {
  if (theme < 0 || theme >= ScopeTheme::themeNames().size()) {
    theme = 0;
  }

  ScopeTheme::ThemeType themeType = static_cast<ScopeTheme::ThemeType>(theme);
  const ScopeTheme::ScopePalette p = ScopeTheme::paletteFor(themeType);

  // 1. 注入全套零死角一体化 QSS 样式表 (消除所有白块、白角标、白滚动条)
  setStyleSheet(ScopeTheme::generateStyleSheet(themeType));

  // 2. 主波形图表 QCustomPlot 配色深度融合
  if (m_plot) {
    m_plot->setBackground(QBrush(p.plotBgColor));
    if (m_plot->axisRect()) {
      m_plot->axisRect()->setBackground(QBrush(p.plotBgColor));
    }
    m_plot->xAxis->setBasePen(QPen(p.axisPenColor, 1));
    m_plot->yAxis->setBasePen(QPen(p.axisPenColor, 1));
    m_plot->xAxis->setTickPen(QPen(p.tickPenColor, 1));
    m_plot->yAxis->setTickPen(QPen(p.tickPenColor, 1));
    m_plot->xAxis->setSubTickPen(QPen(p.subTickPenColor, 1));
    m_plot->yAxis->setSubTickPen(QPen(p.subTickPenColor, 1));
    m_plot->xAxis->setTickLabelColor(p.tickLabelColor);
    m_plot->yAxis->setTickLabelColor(p.tickLabelColor);
    m_plot->xAxis->setLabelColor(p.axisLabelColor);
    m_plot->yAxis->setLabelColor(p.axisLabelColor);
    m_plot->xAxis->grid()->setPen(QPen(p.gridPenColor, 1, Qt::DashLine));
    m_plot->yAxis->grid()->setPen(QPen(p.gridPenColor, 1, Qt::DashLine));
    m_plot->xAxis->grid()->setSubGridPen(QPen(p.subGridPenColor, 1, Qt::DotLine));
    m_plot->yAxis->grid()->setSubGridPen(QPen(p.subGridPenColor, 1, Qt::DotLine));
    m_plot->xAxis->grid()->setSubGridVisible(true);
    m_plot->yAxis->grid()->setSubGridVisible(true);
  }

  // 3. FFT 频谱图表配色融合
  if (m_fftPlot) {
    m_fftPlot->setBackground(QBrush(p.plotBgColor));
    if (m_fftPlot->axisRect()) {
      m_fftPlot->axisRect()->setBackground(QBrush(p.plotBgColor));
    }
    m_fftPlot->xAxis->setBasePen(QPen(p.axisPenColor, 1));
    m_fftPlot->yAxis->setBasePen(QPen(p.axisPenColor, 1));
    m_fftPlot->xAxis->setTickPen(QPen(p.tickPenColor, 1));
    m_fftPlot->yAxis->setTickPen(QPen(p.tickPenColor, 1));
    m_fftPlot->xAxis->setSubTickPen(QPen(p.subTickPenColor, 1));
    m_fftPlot->yAxis->setSubTickPen(QPen(p.subTickPenColor, 1));
    m_fftPlot->xAxis->setTickLabelColor(p.tickLabelColor);
    m_fftPlot->yAxis->setTickLabelColor(p.tickLabelColor);
    m_fftPlot->xAxis->setLabelColor(p.axisLabelColor);
    m_fftPlot->yAxis->setLabelColor(p.axisLabelColor);
    m_fftPlot->xAxis->grid()->setPen(QPen(p.gridPenColor, 1, Qt::DashLine));
    m_fftPlot->yAxis->grid()->setPen(QPen(p.gridPenColor, 1, Qt::DashLine));
    m_fftPlot->xAxis->grid()->setSubGridPen(QPen(p.subGridPenColor, 1, Qt::DotLine));
    m_fftPlot->yAxis->grid()->setSubGridPen(QPen(p.subGridPenColor, 1, Qt::DotLine));
    m_fftPlot->xAxis->grid()->setSubGridVisible(true);
    m_fftPlot->yAxis->grid()->setSubGridVisible(true);
    if (m_fftPlot->graphCount() > 0) {
      m_fftPlot->graph(0)->setPen(QPen(QColor(p.accent), 1.5));
      QColor brushColor = QColor(p.accent);
      brushColor.setAlpha(35);
      m_fftPlot->graph(0)->setBrush(QBrush(brushColor));
    }
  }

  // 4. 游标与触发电平线配色更新
  if (m_cursorX1) m_cursorX1->setPen(QPen(p.cursorXColor, 1, Qt::DashLine));
  if (m_cursorX2) m_cursorX2->setPen(QPen(p.cursorXColor, 1, Qt::DashLine));
  if (m_cursorY1) m_cursorY1->setPen(QPen(p.cursorYColor, 1, Qt::DashDotLine));
  if (m_cursorY2) m_cursorY2->setPen(QPen(p.cursorYColor, 1, Qt::DashDotLine));
  if (m_triggerLevelLine) m_triggerLevelLine->setPen(QPen(p.triggerLineColor, 1, Qt::DotLine));

  // 5. 适配 16 通道预设波形配色
  for (int i = 0; i < MAX_CHANNELS && i < p.channelColors.size() && i < m_channels.size(); ++i) {
    m_channels[i].color = p.channelColors[i];
    if (m_plot && i < m_plot->graphCount()) {
      m_plot->graph(i)->setPen(QPen(m_channels[i].color, m_channels[i].lineWidth));
    }
    if (m_tableChannels) {
      auto *btnColor = qobject_cast<QPushButton *>(m_tableChannels->cellWidget(i, 2));
      if (btnColor) {
        btnColor->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 3px;").arg(m_channels[i].color.name(), p.borderLight));
      }
      auto *lblCur = qobject_cast<QLabel *>(m_tableChannels->cellWidget(i, 5));
      if (lblCur) {
        lblCur->setStyleSheet(QString("color: %1; font-weight: bold;").arg(p.accent));
      }
    }
  }

  // 6. 状态栏与日志文本框配色更新
  if (m_lblStatus) {
    m_lblStatus->setStyleSheet(QString("color: %1; font-weight: bold; padding-left: 8px;").arg(m_isRunning ? "#00E676" : "#FFB300"));
  }
  if (m_textLog) {
    m_textLog->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Consolas', monospace; font-size: 11px; border: 1px solid %3;").arg(p.baseBg, p.textMain, p.border));
  }

  // 7. 刷新图表重绘
  if (m_plot) m_plot->replot(QCustomPlot::rpQueuedReplot);
  if (m_fftPlot) m_fftPlot->replot(QCustomPlot::rpQueuedReplot);

  // 8. 保持下拉框与持久化设置一致
  if (m_comboTheme && m_comboTheme->currentIndex() != theme) {
    m_comboTheme->blockSignals(true);
    m_comboTheme->setCurrentIndex(theme);
    m_comboTheme->blockSignals(false);
  }

  QSettings settings("PhudonTools", "Oscilloscope");
  settings.setValue("theme", theme);
  appendLog(QStringLiteral("🎨 已切换示波器皮肤主题: %1 %2").arg(p.icon, p.name));
}

void OscilloscopeWindow::exportDataCsv() {
  QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("导出示波器波形数据"),
                                                  "scope_data_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv",
                                                  QStringLiteral("CSV 文件 (*.csv)"));
  if (filename.isEmpty()) return;

  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << "Time(s)";
    for (int i = 0; i < MAX_CHANNELS; ++i) {
      if (m_channels[i].enabled) out << "," << m_channels[i].name;
    }
    out << "\n";

    int minLen = 1e9;
    for (int i = 0; i < MAX_CHANNELS; ++i) {
      if (m_channels[i].enabled) minLen = qMin(minLen, m_channels[i].xData.size());
    }

    for (int pt = 0; pt < minLen; ++pt) {
      out << m_channels[0].xData[pt];
      for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (m_channels[i].enabled) {
          out << "," << m_channels[i].yData[pt];
        }
      }
      out << "\n";
    }
    file.close();
    appendLog(QStringLiteral("✓ 数据已成功导出至: %1").arg(filename));
  }
}

void OscilloscopeWindow::exportWaveformPng() {
  QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("保存示波器波形截图"),
                                                  "scope_screenshot_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png",
                                                  QStringLiteral("PNG 图片 (*.png)"));
  if (filename.isEmpty()) return;
  m_plot->savePng(filename, 1920, 1080);
  appendLog(QStringLiteral("✓ 高清截图已保存: %1").arg(filename));
}

void OscilloscopeWindow::exportReportPdf() {
  QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("导出 PDF 报告"),
                                                  "scope_report.pdf", QStringLiteral("PDF 文件 (*.pdf)"));
  if (filename.isEmpty()) return;
  m_plot->savePdf(filename, 800, 600);
  appendLog(QStringLiteral("✓ PDF 报告导出成功: %1").arg(filename));
}

void OscilloscopeWindow::onChannelSettingsChanged() {}
void OscilloscopeWindow::onSerialPortError(QSerialPort::SerialPortError error) {
  if (error != QSerialPort::NoError) {
    appendLog(QStringLiteral("串口通信异常: %1").arg(m_serialPort ? m_serialPort->errorString() : "Unknown"), true);
  }
}
void OscilloscopeWindow::onTcpSocketConnected() { appendLog(QStringLiteral("✓ TCP 套接字已建立连接")); }
void OscilloscopeWindow::onTcpSocketDisconnected() { appendLog(QStringLiteral("TCP 连接已断开"), true); }
void OscilloscopeWindow::handleModbusResponse(const QByteArray &response) { Q_UNUSED(response); }

void OscilloscopeWindow::appendLog(const QString &msg, bool isError) {
  if (!m_textLog) return;
  QString timeStr = QTime::currentTime().toString("hh:mm:ss.zzz");
  QString color = isError ? "#EF4444" : "#38BDF8";
  m_textLog->append(QString("<span style='color:#64748B;'>[%1]</span> <span style='color:%2;'>%3</span>")
                        .arg(timeStr, color, msg));
  m_textLog->verticalScrollBar()->setValue(m_textLog->verticalScrollBar()->maximum());
}

void OscilloscopeWindow::closeEvent(QCloseEvent *event) {
  QMainWindow::closeEvent(event);
}
