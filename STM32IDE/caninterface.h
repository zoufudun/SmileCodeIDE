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

  int dlc() const { return data.size(); }
};

// CAN 工作模式
enum class CanMode {
  Normal,    // 正常收发
  ListenOnly // 静默监听
};

// 基于周立功(ZLG) zlgcan 动态库的 CAN/CAN FD 收发后端。
// 默认面向 USBCANFD-200U，运行时通过 QLibrary 动态加载 zlgcan.dll，
// 避免 MinGW 与 MSVC 导入库不兼容的问题，并在缺少驱动时优雅降级。
class CanInterface : public QObject {
  Q_OBJECT
public:
  explicit CanInterface(QObject *parent = nullptr);
  ~CanInterface() override;

  // zlgcan 动态库是否加载成功
  bool libraryLoaded() const;
  QString libraryError() const { return m_libError; }

  bool isOpen() const { return m_open; }
  bool fdEnabled() const { return m_fdEnabled; }

  // 打开并启动指定设备通道。
  //   deviceType : ZLG 设备类型号，USBCANFD-200U 为 41
  //   deviceIndex: 同型号设备索引，从 0 开始
  //   channel    : 通道号（200U 为 0/1）
  //   abitBaud   : 仲裁域(标称)波特率 bps
  //   enableFd   : 是否启用 CAN FD
  //   dbitBaud   : CAN FD 数据域波特率 bps
  //   mode       : 工作模式
  //   terminalRes: 是否启用内部终端电阻
  bool open(quint32 deviceType, int deviceIndex, int channel, int abitBaud,
            bool enableFd, int dbitBaud, CanMode mode, bool terminalRes);
  void close();

  // 发送一帧报文
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
  bool setDeviceBaud(int channel, int abitBaud, int dbitBaud, bool terminalRes);

  struct Impl;     // 隐藏 zlgcan 相关类型与函数指针
  Impl *m_d;       // PIMPL
  QTimer *m_timer; // 接收轮询定时器

  bool m_open = false;
  bool m_fdEnabled = false;
  int m_channel = 0;
  QString m_libError;
};

#endif // CANINTERFACE_H
