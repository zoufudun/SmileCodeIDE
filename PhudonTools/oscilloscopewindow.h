#ifndef OSCILLOSCOPEWINDOW_H
#define OSCILLOSCOPEWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QVector>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>
#include <QDateTime>
#include <QColor>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>

#include "../qcustomplot/qcustomplot.h"
#include "caninterface.h"
#include "TOOLS/CIconFont.h"
#include "scopetheme.h"

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QLineEdit;
class QLabel;
class QTableWidget;
class QTabWidget;
class QDockWidget;
class QToolBar;
class QAction;
class QGroupBox;
class QSplitter;
class QTextEdit;

// 通道配置结构
struct ScopeChannelConfig {
  int id = 0;
  QString name;
  bool enabled = true;
  QColor color;
  double scale = 1.0;     // 垂直增益 (V/div 或 比例乘数)
  double offset = 0.0;    // 垂直偏置 (V 或 偏移量)
  int lineWidth = 2;
  bool visible = true;
  QVector<double> xData;
  QVector<double> yData;
  
  // 实时统计缓存
  double curVal = 0.0;
  double minVal = 0.0;
  double maxVal = 0.0;
  double avgVal = 0.0;
  double vpp = 0.0;
  double rms = 0.0;
  double freq = 0.0;
};

// 通信接口类型
enum class ScopeInterfaceType {
  VirtualSignal = 0, // 虚拟多波形信号发生器
  SerialPort,        // RS232 / RS485 / 虚拟串口
  Modbus,            // Modbus RTU / Modbus TCP
  CanBus,            // CAN / CAN-FD 总线
  Ethernet,          // 以太网 TCP/UDP
  UsbRaw             // USB 数据流
};

// 数据协议解析模式
enum class ScopeProtocolType {
  AsciiCsv = 0,      // ASCII 纯文本 / CSV (逗号、空格、换行分隔数字)
  BinaryStream,      // 纯二进制数据流 (Float32 / Int16 / Int32 / Double)
  CustomFrame,       // 标准帧格式 (0xAA 0x55 帧头 + 载荷 + 校验和)
  VofaFireWater,     // VOFA+ / FireWater 协议 (tail: 0x00 0x00 0x80 0x7F)
  JustFloat,         // JustFloat 协议
  ModbusRegisters,   // Modbus 寄存器直接映射
  CanFieldMapping    // CAN 报文按位/按字节映射
};

class OscilloscopeWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit OscilloscopeWindow(QWidget *parent = nullptr);
  ~OscilloscopeWindow() override;
  void applyTheme(const QString &themeName);

  static const int MAX_CHANNELS = 16;

public slots:
  // 示波器主运行控制
  void startAcquisition();
  void stopAcquisition();
  void singleTrigger();
  void clearWaveforms();
  void exportDataCsv();
  void exportWaveformPng();
  void exportReportPdf();

  // 接口连接与断开
  void toggleConnection();
  void onInterfaceTypeChanged(int index);

  // 通道与量程调整
  void onTimebaseChanged(int index);
  void onChannelSettingsChanged();
  void onAutoFitYAxis();
  void resetDefaultView();

private slots:
  // 定时采样/刷新与数据处理
  void onPlotRefreshTimeout();
  void onVirtualSignalTimeout();
  void onModbusPollTimeout();

  // 通信底层回调
  void onSerialReadyRead();
  void onSerialPortError(QSerialPort::SerialPortError error);
  void onTcpSocketConnected();
  void onTcpSocketDisconnected();
  void onTcpReadyRead();
  void onTcpNewConnection();
  void onUdpReadyRead();
  void onCanFrameReceived(const CanFrame &frame);

  // 测量游标与 FFT
  void onCursorToggled(bool checked);
  void onFftToggled(bool checked);
  void onTriggerSettingsChanged();
  void onThemeChanged(int themeIndex);

  // 串口热插拔与刷新
  void refreshSerialPorts();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  void initUi();
  void initPlots();
  void initToolBar();
  void initDocks();
  void initChannels();
  void applyScopeTheme(int theme);

  // 协议数据解析入口
  void processRawIncomingBytes(const QByteArray &data);
  void parseAsciiTextStream(const QString &text);
  void parseBinaryStream(const QByteArray &data);
  void parseCustomFrameStream(const QByteArray &data);
  void parseVofaStream(const QByteArray &data);

  // Modbus 构造与解析
  void sendModbusPollRequest();
  void handleModbusResponse(const QByteArray &response);

  // 波形数据推入通道
  void appendSample(int channelIdx, double xVal, double yVal);
  void calculateChannelStatistics(int channelIdx);
  void calculateFft(int channelIdx);
  void checkTriggerCondition(int channelIdx, double prevY, double curY);

  // 通信对象
  ScopeInterfaceType m_currentInterface = ScopeInterfaceType::VirtualSignal;
  ScopeProtocolType m_currentProtocol = ScopeProtocolType::AsciiCsv;
  bool m_isConnected = false;
  bool m_isRunning = true;

  // 硬件接口
  QSerialPort *m_serialPort = nullptr;
  QTcpSocket *m_tcpClient = nullptr;
  QTcpServer *m_tcpServer = nullptr;
  QUdpSocket *m_udpSocket = nullptr;
  CanInterface *m_canInterface = nullptr;

  // 定时器
  QTimer *m_plotTimer = nullptr;
  QTimer *m_virtualTimer = nullptr;
  QTimer *m_modbusTimer = nullptr;
  QElapsedTimer m_timeTracker;
  double m_currentSimTime = 0.0;

  // 接收缓冲区
  QByteArray m_rxRawBuffer;
  QString m_rxTextBuffer;

  // 波形图表与控件
  QCustomPlot *m_plot = nullptr;
  QCustomPlot *m_fftPlot = nullptr;
  QVector<ScopeChannelConfig> m_channels;

  // 测量游标项
  bool m_showCursors = false;
  QCPItemStraightLine *m_cursorX1 = nullptr;
  QCPItemStraightLine *m_cursorX2 = nullptr;
  QCPItemStraightLine *m_cursorY1 = nullptr;
  QCPItemStraightLine *m_cursorY2 = nullptr;
  QCPItemText *m_cursorLabel = nullptr;

  // 触发系统
  bool m_triggerActive = false;
  bool m_waitingSingleTrigger = false;
  int m_triggerChannel = 0;
  int m_triggerMode = 0;   // 0: Auto, 1: Normal, 2: Single
  int m_triggerEdge = 0;   // 0: Rising, 1: Falling
  double m_triggerLevel = 0.0;
  QCPItemStraightLine *m_triggerLevelLine = nullptr;

  // UI 控件指针
  QComboBox *m_comboInterface = nullptr;
  QComboBox *m_comboProtocol = nullptr;
  QPushButton *m_btnConnect = nullptr;
  QPushButton *m_btnRunStop = nullptr;
  QPushButton *m_btnSingle = nullptr;
  QPushButton *m_btnClear = nullptr;
  QPushButton *m_btnAutoFit = nullptr;
  QComboBox *m_comboTimebase = nullptr;
  QComboBox *m_comboTheme = nullptr;

  // 接口配置 Dock 控件
  QTabWidget *m_interfaceTabWidget = nullptr;
  
  // 串口设置
  QComboBox *m_comboPortName = nullptr;
  QComboBox *m_comboBaudRate = nullptr;
  QComboBox *m_comboDataBits = nullptr;
  QComboBox *m_comboParity = nullptr;
  QComboBox *m_comboStopBits = nullptr;

  // Modbus 设置
  QComboBox *m_comboModbusMode = nullptr; // RTU or TCP
  QSpinBox *m_spinSlaveId = nullptr;
  QComboBox *m_comboFunctionCode = nullptr;
  QSpinBox *m_spinModbusStartAddr = nullptr;
  QSpinBox *m_spinModbusRegCount = nullptr;
  QSpinBox *m_spinModbusInterval = nullptr;
  QComboBox *m_comboModbusDataType = nullptr;

  // CAN 设置
  QComboBox *m_comboCanDeviceType = nullptr;
  QSpinBox *m_spinCanDeviceIndex = nullptr;
  QSpinBox *m_spinCanChannelIndex = nullptr;
  QComboBox *m_comboCanBaud = nullptr;
  QLineEdit *m_editCanFilterId = nullptr;

  // 以太网设置
  QComboBox *m_comboEthMode = nullptr; // TCP Client / TCP Server / UDP
  QLineEdit *m_editEthIp = nullptr;
  QSpinBox *m_spinEthPort = nullptr;

  // 虚拟信号源设置
  QComboBox *m_comboVirtWaveType = nullptr;
  QDoubleSpinBox *m_spinVirtFreq = nullptr;
  QDoubleSpinBox *m_spinVirtAmp = nullptr;
  QDoubleSpinBox *m_spinVirtNoise = nullptr;

  // 通道配置与测量 Dock 控件
  QTableWidget *m_tableChannels = nullptr;
  QTableWidget *m_tableMeasure = nullptr;
  QDockWidget *m_dockInterface = nullptr;
  QDockWidget *m_dockChannels = nullptr;
  QDockWidget *m_dockMeasure = nullptr;
  QDockWidget *m_dockFft = nullptr;
  QDockWidget *m_dockLog = nullptr;
  QTextEdit *m_textLog = nullptr;

  // 状态栏
  QLabel *m_lblStatus = nullptr;
  QLabel *m_lblSampleRate = nullptr;
  QLabel *m_lblFps = nullptr;
  int m_frameCount = 0;
  QElapsedTimer m_fpsTimer;

  void appendLog(const QString &msg, bool isError = false);
};

#endif // OSCILLOSCOPEWINDOW_H
