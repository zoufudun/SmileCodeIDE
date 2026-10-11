#ifndef ICANADAPTER_H
#define ICANADAPTER_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>

// 原始 CAN / CAN FD 报文结构
struct CanRawFrame {
  quint32 id = 0;          // 帧 ID (标准11位或扩展29位)
  bool extended = false;   // 是否为扩展帧
  bool remote = false;     // 是否为远程帧 (RTR)
  bool fd = false;         // 是否为 CAN FD 帧
  bool brs = false;        // CAN FD 位速率切换 (Bit Rate Switch)
  bool esi = false;        // CAN FD 错误状态指示 (Error State Indicator)
  bool errorFrame = false; // 是否为错误帧
  QByteArray data;         // 负载数据 (标准 CAN 最大 8 字节, CAN FD 最大 64 字节)
  qint64 timestamp = 0;    // 接收/发送时间戳 (毫秒或微秒)
  int channel = 0;         // 通道索引 (0, 1, 2...)
  int transmitType = 0;    // 发送类型: 0-正常, 1-单次发送, 2-自发自收, 3-单次自发自收

  int dlc() const { return data.size(); }

  // 格式化为用于显示的十六进制字符串
  QString toHexString(const QString &sep = " ") const {
    QString res;
    for (int i = 0; i < data.size(); ++i) {
      if (i > 0) res += sep;
      res += QString("%1").arg(static_cast<quint8>(data.at(i)), 2, 16, QLatin1Char('0')).toUpper();
    }
    return res;
  }
};

// CAN 滤波规则
struct CanFilterConfig {
  int mode = 0;            // 0: 标准帧精确ID, 1: 扩展帧精确ID, 2: 标准帧段ID, 3: 扩展帧段ID
  quint32 startId = 0;
  quint32 endId = 0;
};

// CAN 工作模式
enum class CanWorkMode {
  Normal,     // 正常模式 (收发)
  ListenOnly, // 静默监听模式 (只收不发，不发送ACK)
  Loopback    // 自测试回环模式
};

// 单个通道的配置参数
struct CanChannelParams {
  int channel = 0;
  bool isFd = false;
  int abitBaud = 500000;    // 仲裁域/经典波特率 (bps)
  int dbitBaud = 2000000;   // CAN FD 数据域波特率 (bps)
  bool isIso = true;        // ISO / Non-ISO CAN FD
  bool enableBrs = true;    // 启用位速率切换
  CanWorkMode mode = CanWorkMode::Normal;
  bool terminalRes = true;  // 120欧终端电阻
  bool reportBusUsage = false;
  int busUsagePeriod = 100; // 毫秒
  int retrySend = 1;        // 重试次数
  bool enableFilter = false;
  QList<CanFilterConfig> filters;
};

// 设备打开参数
struct CanDeviceParams {
  QString pluginType;       // 如 "simulation", "peakcan", "vectorcan", "passthrucan", "systeccan", "tinycan", "virtualcan", "socketcan", "zlg", "chuangxin", "tosun"
  QString deviceName;       // 设备名称 / 接口名 (如 "can0", "usb0", "USBCANFD-200U")
  quint32 deviceType = 0;   // 硬件类型代码 (兼容 ZLG / 创芯等代码)
  int deviceIndex = 0;      // 设备索引 (0, 1...)
  int channelCount = 2;     // 支持的通道数
  QString serialNumber;     // 硬件序列号
  QString extraJson;        // 扩展参数 JSON
};

// 总线状态与统计
struct CanBusStatistics {
  quint64 rxFrames = 0;
  quint64 txFrames = 0;
  quint64 rxBytes = 0;
  quint64 txBytes = 0;
  quint64 errorFrames = 0;
  double busLoad = 0.0;     // 总线利用率百分比 (0.0% ~ 100.0%)
  int tec = 0;              // 发送错误计数器
  int rec = 0;              // 接收错误计数器
  bool busOff = false;      // 总线关闭
};

// 统一 CAN 适配层抽象基类
class ICanAdapter : public QObject {
  Q_OBJECT
public:
  explicit ICanAdapter(QObject *parent = nullptr) : QObject(parent) {}
  ~ICanAdapter() override = default;

  // 适配器唯一类型标识 (如 "simulation", "peakcan", "vectorcan", "zlg" 等)
  virtual QString adapterType() const = 0;

  // 适配器友好名称
  virtual QString displayName() const = 0;

  // 设备层面控制
  virtual bool openDevice(const CanDeviceParams &params) = 0;
  virtual void closeDevice() = 0;
  virtual bool isDeviceOpen() const = 0;

  // 通道层面控制
  virtual bool startChannel(int channel, const CanChannelParams &cfg) = 0;
  virtual bool stopChannel(int channel) = 0;
  virtual bool isChannelRunning(int channel) const = 0;
  virtual CanChannelParams channelConfig(int channel) const = 0;

  // 报文发送接口
  virtual bool sendFrame(int channel, const CanRawFrame &frame) = 0;
  virtual bool sendFrames(int channel, const QList<CanRawFrame> &frames, int *sentCount = nullptr) {
    int count = 0;
    for (const auto &f : frames) {
      if (sendFrame(channel, f)) {
        count++;
      } else {
        break;
      }
    }
    if (sentCount) *sentCount = count;
    return count == frames.size();
  }

  // 状态与统计查询
  virtual CanBusStatistics statistics(int channel) const {
    Q_UNUSED(channel);
    return CanBusStatistics();
  }

  virtual bool getDeviceInfo(QString *hwVer, QString *fwVer, QString *drVer,
                             QString *libVer, int *canNum, QString *serial,
                             QString *typeStr) const {
    Q_UNUSED(hwVer); Q_UNUSED(fwVer); Q_UNUSED(drVer); Q_UNUSED(libVer);
    Q_UNUSED(canNum); Q_UNUSED(serial); Q_UNUSED(typeStr);
    return false;
  }

signals:
  void deviceOpened();
  void deviceClosed();
  void channelStarted(int channel);
  void channelStopped(int channel);
  void frameReceived(const CanRawFrame &frame);
  void frameSent(const CanRawFrame &frame);
  void busStatusChanged(int channel, bool busOff, int tec, int rec);
  void errorOccurred(const QString &message);
};

#endif // ICANADAPTER_H
