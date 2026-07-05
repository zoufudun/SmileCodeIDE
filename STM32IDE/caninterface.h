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

  int dlc() const { return data.size(); }
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

// 基于周立功(ZLG) zlgcan 动态库的 CAN/CAN FD 收发后端。
class CanInterface : public QObject {
  Q_OBJECT
public:
  explicit CanInterface(QObject *parent = nullptr);
  ~CanInterface() override;

  // zlgcan 动态库是否加载成功
  bool libraryLoaded() const;
  QString libraryError() const { return m_libError; }

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
