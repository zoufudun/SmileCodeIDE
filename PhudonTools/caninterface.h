#ifndef CANINTERFACE_H
#define CANINTERFACE_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>

class QTimer;

// 单帧 CAN/CAN FD 报文数据结构
struct CanFrame {
  quint32 id = 0;          // 帧 ID（标准 11 位或扩展 29 位）
  bool extended = false;   // 扩展帧
  bool remote = false;     // 远程帧（仅经典 CAN）
  bool fd = false;         // CAN FD 帧
  bool brs = false;        // CAN FD 加速（Bit Rate Switch）
  bool errorFrame = false; // 错误帧
  QByteArray data;         // 数据负载（经典 CAN 最多 8 字节，FD 最多 64 字节）
  qint64 timestamp = 0;    // 接收/发送时间戳（ms，主机时钟）
  int channel = 0;         // 通道索引 (0 或 1)
  int transmitType = 0;    // 发送方式：0-正常发送，1-单次发送，2-自发自收，3-单次自发自收

  int dlc() const { return data.size(); }
};

// 滤波规则结构
struct CanFilterRule {
  int mode = 0; // 0: 标准帧明确ID, 1: 扩展帧明确ID, 2: 标准帧段ID, 3: 扩展帧段ID
  quint32 startId = 0;
  quint32 endId = 0;
};

// CAN 工作模式
enum class CanMode {
  Normal,    // 正常收发
  ListenOnly // 静默监听
};

// CAN 通道参数结构
struct CanChannelConfig {
  bool isFd = false;
  int abitBaud = 500000;
  int dbitBaud = 2000000;
  bool isIso = true;
  bool enableBrs = true;
  CanMode mode = CanMode::Normal;
  bool terminalRes = true;
  bool reportBusUsage = false;
  int busUsagePeriod = 100;
  int retrySend = 1; // 1: 发送到总线关闭, 3: 重试3次, 0: 不重试
  bool enableFilter = false;
};

// 创芯科技 ControlCAN 系列设备类型常量
#define CX_USBCAN1           103 // 创芯 USBCAN-1C
#define CX_USBCAN2           104 // 创芯 USBCAN-2C
#define CX_USBCAN_E_U        120 // 创芯 USBCAN-E-U
#define CX_USBCAN_2E_U       121 // 创芯 USBCAN-2E-U

// CAN/CAN FD 多驱动通用收发后端（支持周立功 ZLG 与 创芯科技 USBCAN 系列）。
class CanInterface : public QObject {
  Q_OBJECT
public:
  explicit CanInterface(QObject *parent = nullptr);
  ~CanInterface() override;

  // zlgcan 动态库是否加载成功
  bool libraryLoaded() const;
  QString libraryError() const { return m_libError; }

  // 获取当前打开的设备信息
  quint32 deviceType() const;
  int deviceIndex() const;

  // 兼容旧版，检查是否有任何通道正在运行
  bool isOpen() const;
  bool fdEnabled() const { return m_fdEnabled; }
  int channel() const { return m_channel; }

  // 设备层面控制
  bool openDevice(quint32 deviceType, int deviceIndex);
  void closeDevice();
  bool isDeviceOpen() const;

  // 通道层面控制
  bool startChannel(int channel, const CanChannelConfig &cfg);
  bool stopChannel(int channel);
  bool isChannelRunning(int channel) const;
  CanChannelConfig channelConfig(int channel) const;
  void setChannelConfig(int channel, const CanChannelConfig &cfg);

  // 通道滤波设置
  void setChannelFilters(int channel, const QList<CanFilterRule> &rules);
  QList<CanFilterRule> channelFilters(int channel) const;
  bool matchesFilter(int channel, const CanFrame &frame) const;

  // 获取连接中的设备信息 (Image 4 需求)
  bool getDeviceInformation(QString *hwVer, QString *fwVer, QString *drVer,
                            QString *libVer, int *canNum, QString *serial,
                            QString *typeStr) const;

  // 兼容旧接口的打开方式（打开设备并启动指定通道）
  bool open(quint32 deviceType, int deviceIndex, int channel, int abitBaud,
            bool enableFd, int dbitBaud, CanMode mode, bool terminalRes);
  void close();

  // 发送一帧报文 (支持指定通道与兼容老接口的默认通道)
  bool sendFrame(int channel, const CanFrame &frame);
  bool sendFrame(const CanFrame &frame);

  static quint32 defaultDeviceType(); // ZCAN_USBCANFD_200U

  // 判断设备类型是否支持 CAN FD（41+ 的 USBCANFD 系列及虚拟设备）
  static bool isDeviceFdCapable(quint32 deviceType);

  // 判断是否为创芯科技 ControlCAN 系列设备 (USBCAN-2C 等)
  static bool isControlCanDevice(quint32 deviceType);

  // 诊断：获取驱动版本和设备状态（需在 open 前调用）
  bool diagnoseDriver(QString *driverVer, QString *deviceName, bool *online) const;

signals:
  void connected();
  void disconnected();
  void frameReceived(const CanFrame &frame);
  void frameSent(const CanFrame &frame);
  void errorOccurred(const QString &message);

private slots:
  void pollReceive();

private:
  bool loadLibrary();
  bool setDeviceBaud(int channel, const CanChannelConfig &cfg);

  struct Impl;     // 隐藏 zlgcan 相关类型与函数指针
  Impl *m_d;       // PIMPL
  QTimer *m_timer; // 接收轮询定时器

  bool m_open = false; // 用于兼容旧的开启状态
  bool m_fdEnabled = false;
  int m_channel = 0;
  QString m_libError;
};

#endif // CANINTERFACE_H
