#include "caninterface.h"

#include <cstring>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QScopeGuard>
#include <QTimer>

#include "USBCANFD/zlgcan.h"

#ifdef Q_OS_WIN
#define ZLG_CALL __stdcall
#else
#define ZLG_CALL
#endif

namespace {
constexpr quint32 kUsbCanFd200U = ZCAN_USBCANFD_200U; // 41
constexpr int kPollIntervalMs = 5;
constexpr int kMaxFramesPerPoll = 256;

// zlgcan 导出函数指针类型
typedef DEVICE_HANDLE(ZLG_CALL *Fn_OpenDevice)(UINT, UINT, UINT);
typedef UINT(ZLG_CALL *Fn_CloseDevice)(DEVICE_HANDLE);
typedef UINT(ZLG_CALL *Fn_IsDeviceOnLine)(DEVICE_HANDLE);
typedef UINT(ZLG_CALL *Fn_GetDeviceInfoEx)(DEVICE_HANDLE, ZCAN_DEVICE_INFO_EX *);
typedef CHANNEL_HANDLE(ZLG_CALL *Fn_InitCAN)(DEVICE_HANDLE, UINT,
                                             ZCAN_CHANNEL_INIT_CONFIG *);
typedef UINT(ZLG_CALL *Fn_StartCAN)(CHANNEL_HANDLE);
typedef UINT(ZLG_CALL *Fn_ResetCAN)(CHANNEL_HANDLE);
typedef UINT(ZLG_CALL *Fn_ClearBuffer)(CHANNEL_HANDLE);
typedef UINT(ZLG_CALL *Fn_GetReceiveNum)(CHANNEL_HANDLE, BYTE);
typedef UINT(ZLG_CALL *Fn_Transmit)(CHANNEL_HANDLE, ZCAN_Transmit_Data *, UINT);
typedef UINT(ZLG_CALL *Fn_Receive)(CHANNEL_HANDLE, ZCAN_Receive_Data *, UINT,
                                   int);
typedef UINT(ZLG_CALL *Fn_TransmitFD)(CHANNEL_HANDLE, ZCAN_TransmitFD_Data *,
                                      UINT);
typedef UINT(ZLG_CALL *Fn_ReceiveFD)(CHANNEL_HANDLE, ZCAN_ReceiveFD_Data *,
                                     UINT, int);
typedef UINT(ZLG_CALL *Fn_SetValue)(DEVICE_HANDLE, const char *, const void *);
} // namespace

struct CanInterface::Impl {
  QLibrary lib;

  Fn_OpenDevice openDevice = nullptr;
  Fn_CloseDevice closeDevice = nullptr;
  Fn_IsDeviceOnLine isDeviceOnLine = nullptr;
  Fn_GetDeviceInfoEx getDeviceInfoEx = nullptr;
  Fn_InitCAN initCan = nullptr;
  Fn_StartCAN startCan = nullptr;
  Fn_ResetCAN resetCan = nullptr;
  Fn_ClearBuffer clearBuffer = nullptr;
  Fn_GetReceiveNum getReceiveNum = nullptr;
  Fn_Transmit transmit = nullptr;
  Fn_Receive receive = nullptr;
  Fn_TransmitFD transmitFd = nullptr;
  Fn_ReceiveFD receiveFd = nullptr;
  Fn_SetValue setValue = nullptr;

  DEVICE_HANDLE device = INVALID_DEVICE_HANDLE;
  QMap<int, CHANNEL_HANDLE> activeChans;
  quint32 deviceType = kUsbCanFd200U;
  int deviceIndex = 0;

  bool resolved() const {
    return openDevice && closeDevice && initCan && startCan && getReceiveNum &&
           transmit && receive && transmitFd && receiveFd && setValue;
  }
  bool diagnosticsAvailable() const {
    return isDeviceOnLine && getDeviceInfoEx;
  }
};

CanInterface::CanInterface(QObject *parent)
    : QObject(parent), m_d(new Impl), m_timer(new QTimer(this)) {
  m_timer->setInterval(kPollIntervalMs);
  connect(m_timer, &QTimer::timeout, this, &CanInterface::pollReceive);
  loadLibrary();
}

CanInterface::~CanInterface() {
  close();
  if (m_d->lib.isLoaded()) {
    m_d->lib.unload();
  }
  delete m_d;
}

quint32 CanInterface::defaultDeviceType() { return kUsbCanFd200U; }

bool CanInterface::libraryLoaded() const { return m_d->resolved(); }

bool CanInterface::loadLibrary() {
  // 优先以绝对路径加载随程序分发的 zlgcan.dll，确保加载到正确的版本；
  // 其次回退到系统搜索路径。
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {appDir + "/zlgcan.dll",
                                  QStringLiteral("zlgcan")};
  bool loaded = false;
  for (const QString &name : candidates) {
    m_d->lib.setFileName(name);
    if (m_d->lib.load()) {
      loaded = true;
      break;
    }
  }
  if (!loaded) {
    m_libError = m_d->lib.errorString();
    return false;
  }

  m_d->openDevice =
      reinterpret_cast<Fn_OpenDevice>(m_d->lib.resolve("ZCAN_OpenDevice"));
  m_d->closeDevice =
      reinterpret_cast<Fn_CloseDevice>(m_d->lib.resolve("ZCAN_CloseDevice"));
  m_d->isDeviceOnLine =
      reinterpret_cast<Fn_IsDeviceOnLine>(m_d->lib.resolve("ZCAN_IsDeviceOnLine"));
  m_d->getDeviceInfoEx = reinterpret_cast<Fn_GetDeviceInfoEx>(
      m_d->lib.resolve("ZCAN_GetDeviceInfoEx"));
  m_d->initCan =
      reinterpret_cast<Fn_InitCAN>(m_d->lib.resolve("ZCAN_InitCAN"));
  m_d->startCan =
      reinterpret_cast<Fn_StartCAN>(m_d->lib.resolve("ZCAN_StartCAN"));
  m_d->resetCan =
      reinterpret_cast<Fn_ResetCAN>(m_d->lib.resolve("ZCAN_ResetCAN"));
  m_d->clearBuffer =
      reinterpret_cast<Fn_ClearBuffer>(m_d->lib.resolve("ZCAN_ClearBuffer"));
  m_d->getReceiveNum = reinterpret_cast<Fn_GetReceiveNum>(
      m_d->lib.resolve("ZCAN_GetReceiveNum"));
  m_d->transmit =
      reinterpret_cast<Fn_Transmit>(m_d->lib.resolve("ZCAN_Transmit"));
  m_d->receive =
      reinterpret_cast<Fn_Receive>(m_d->lib.resolve("ZCAN_Receive"));
  m_d->transmitFd =
      reinterpret_cast<Fn_TransmitFD>(m_d->lib.resolve("ZCAN_TransmitFD"));
  m_d->receiveFd =
      reinterpret_cast<Fn_ReceiveFD>(m_d->lib.resolve("ZCAN_ReceiveFD"));
  m_d->setValue =
      reinterpret_cast<Fn_SetValue>(m_d->lib.resolve("ZCAN_SetValue"));

  if (!m_d->resolved()) {
    m_libError = QStringLiteral("zlgcan.dll 缺少必要的导出函数");
    return false;
  }
  return true;
}

bool CanInterface::setDeviceBaud(int channel, const CanChannelConfig &cfg) {
  // USBCANFD 系列通过属性接口设置波特率，须在 InitCAN 之前调用。
  QByteArray chStr = QByteArray::number(channel);
  QByteArray path;

  // 仲裁域波特率（CAN / CAN FD 都需要设置）
  path = chStr + "/canfd_abit_baud_rate";
  m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.abitBaud).constData());

  // 数据域波特率：非 FD 模式设为与 abit 相同（ZLG SDK 仍要求设置此属性）
  path = chStr + "/canfd_dbit_baud_rate";
  int dbit = cfg.isFd ? cfg.dbitBaud : cfg.abitBaud;
  m_d->setValue(m_d->device, path.constData(), QByteArray::number(dbit).constData());

  if (cfg.isFd) {
    path = chStr + "/canfd_standard";
    m_d->setValue(m_d->device, path.constData(), cfg.isIso ? "0" : "1");

    path = chStr + "/canfd_brs";
    m_d->setValue(m_d->device, path.constData(), cfg.enableBrs ? "1" : "0");
  }

  // 内部终端电阻
  path = chStr + "/initenal_resistance";
  m_d->setValue(m_d->device, path.constData(), cfg.terminalRes ? "1" : "0");

  // 上报总线利用率
  path = chStr + "/bus_usage_report";
  m_d->setValue(m_d->device, path.constData(), cfg.reportBusUsage ? "1" : "0");

  // 总线利用率周期
  path = chStr + "/bus_usage_report_interval";
  m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.busUsagePeriod).constData());

  // 发送重试
  path = chStr + "/tx_retry";
  m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.retrySend).constData());

  return true;
}

bool CanInterface::isOpen() const {
  return !m_d->activeChans.isEmpty();
}

bool CanInterface::openDevice(quint32 deviceType, int deviceIndex) {
  if (isDeviceOpen()) {
    return true;
  }
  if (!m_d->resolved()) {
    emit errorOccurred(QStringLiteral("zlgcan 动态库未就绪：%1").arg(m_libError));
    return false;
  }

  m_d->deviceType = deviceType;
  m_d->deviceIndex = deviceIndex;

  const QString appDir = QCoreApplication::applicationDirPath();
  const QString prevCwd = QDir::currentPath();
  QDir::setCurrent(appDir);
  auto cwdGuard = qScopeGuard([prevCwd]() { QDir::setCurrent(prevCwd); });

  m_d->device = m_d->openDevice(deviceType, static_cast<UINT>(deviceIndex), 0);
  if (m_d->device == INVALID_DEVICE_HANDLE) {
    emit errorOccurred(QStringLiteral("打开设备失败 (类型 %1, 索引 %2)").arg(deviceType).arg(deviceIndex));
    return false;
  }
  return true;
}

void CanInterface::closeDevice() {
  if (!isDeviceOpen()) return;

  // 停止所有通道
  QList<int> activeChansList = m_d->activeChans.keys();
  for (int channelIdx : activeChansList) {
    stopChannel(channelIdx);
  }

  m_d->closeDevice(m_d->device);
  m_d->device = INVALID_DEVICE_HANDLE;
  m_open = false;
  emit disconnected();
}

bool CanInterface::isDeviceOpen() const {
  return m_d->device != INVALID_DEVICE_HANDLE;
}

bool CanInterface::startChannel(int channel, const CanChannelConfig &cfg) {
  if (!isDeviceOpen()) {
    emit errorOccurred(QStringLiteral("设备未打开，无法启动通道"));
    return false;
  }

  if (m_d->activeChans.contains(channel)) {
    stopChannel(channel);
  }

  if (!setDeviceBaud(channel, cfg)) {
    return false;
  }

  // USBCANFD 系列硬件 can_type 始终为 TYPE_CANFD，
  // CAN 与 CAN FD 的区别仅体现在发送时使用 Transmit 还是 TransmitFD。
  ZCAN_CHANNEL_INIT_CONFIG zCfg;
  memset(&zCfg, 0, sizeof(zCfg));
  zCfg.can_type = TYPE_CANFD;
  zCfg.canfd.mode = (cfg.mode == CanMode::ListenOnly) ? 1 : 0;
  zCfg.canfd.acc_code = 0;
  zCfg.canfd.acc_mask = 0xFFFFFFFF;
  zCfg.canfd.filter = 0;

  CHANNEL_HANDLE chanHandle = m_d->initCan(m_d->device, static_cast<UINT>(channel), &zCfg);
  if (chanHandle == INVALID_CHANNEL_HANDLE) {
    emit errorOccurred(QStringLiteral("初始化 CAN 通道 %1 失败").arg(channel));
    return false;
  }

  if (m_d->startCan(chanHandle) != STATUS_OK) {
    // 启动失败，复位已初始化的通道句柄
    if (m_d->resetCan) {
      m_d->resetCan(chanHandle);
    }
    emit errorOccurred(QStringLiteral("启动 CAN 通道 %1 失败").arg(channel));
    return false;
  }

  if (m_d->clearBuffer) {
    m_d->clearBuffer(chanHandle);
  }

  m_d->activeChans[channel] = chanHandle;
  m_open = true;
  m_channel = channel;
  m_fdEnabled = cfg.isFd;

  if (!m_timer->isActive()) {
    m_timer->start();
  }

  emit connected();
  return true;
}

bool CanInterface::stopChannel(int channel) {
  if (!m_d->activeChans.contains(channel)) {
    return true;
  }

  CHANNEL_HANDLE chanHandle = m_d->activeChans[channel];
  if (m_d->resetCan) {
    m_d->resetCan(chanHandle);
  }

  m_d->activeChans.remove(channel);
  if (m_d->activeChans.isEmpty()) {
    m_timer->stop();
    m_open = false;
  }
  return true;
}

bool CanInterface::isChannelRunning(int channel) const {
  return m_d->activeChans.contains(channel);
}

bool CanInterface::getDeviceInformation(QString *hwVer, QString *fwVer, QString *drVer,
                                         QString *libVer, int *canNum, QString *serial,
                                         QString *typeStr) const {
  if (!isDeviceOpen()) {
    return false;
  }

  ZCAN_DEVICE_INFO_EX info;
  memset(&info, 0, sizeof(info));
  if (m_d->getDeviceInfoEx(m_d->device, &info) != STATUS_OK) {
    return false;
  }

  if (hwVer) {
    *hwVer = QString("V%1.%2%3")
                 .arg(info.hardware_version.major_version)
                 .arg(info.hardware_version.minor_version, 2, 10, QChar('0'))
                 .arg(info.hardware_version.patch_version, 2, 10, QChar('0'));
  }
  if (fwVer) {
    *fwVer = QString("V%1.%2%3")
                 .arg(info.firmware_version.major_version)
                 .arg(info.firmware_version.minor_version, 2, 10, QChar('0'))
                 .arg(info.firmware_version.patch_version, 2, 10, QChar('0'));
  }
  if (drVer) {
    *drVer = QString("V%1.%2%3")
                 .arg(info.driver_version.major_version)
                 .arg(info.driver_version.minor_version, 2, 10, QChar('0'))
                 .arg(info.driver_version.patch_version, 2, 10, QChar('0'));
  }
  if (libVer) {
    *libVer = QString("V%1.%2%3")
                 .arg(info.library_version.major_version)
                 .arg(info.library_version.minor_version, 2, 10, QChar('0'))
                 .arg(info.library_version.patch_version, 2, 10, QChar('0'));
  }
  if (canNum) {
    *canNum = info.can_channel_number;
  }
  if (serial) {
    *serial = QString::fromLatin1(reinterpret_cast<const char*>(info.serial_number)).trimmed();
    if (serial->isEmpty()) {
      *serial = "B32070B800BB0784A680"; // 兜底序列号
    }
  }
  if (typeStr) {
    *typeStr = QString::fromLatin1(reinterpret_cast<const char*>(info.device_name)).trimmed();
    if (typeStr->isEmpty()) {
      *typeStr = "USBCANFD-200U";
    }
  }
  return true;
}

bool CanInterface::open(quint32 deviceType, int deviceIndex, int channel, int abitBaud,
                         bool enableFd, int dbitBaud, CanMode mode, bool terminalRes) {
  if (!openDevice(deviceType, deviceIndex)) {
    return false;
  }
  CanChannelConfig cfg;
  cfg.isFd = enableFd;
  cfg.abitBaud = abitBaud;
  cfg.dbitBaud = dbitBaud;
  cfg.mode = mode;
  cfg.terminalRes = terminalRes;
  return startChannel(channel, cfg);
}

void CanInterface::close() {
  closeDevice();
}

bool CanInterface::sendFrame(const CanFrame &frame) {
  return sendFrame(m_channel, frame);
}

bool CanInterface::sendFrame(int channel, const CanFrame &frame) {
  if (!m_d->activeChans.contains(channel)) {
    emit errorOccurred(QStringLiteral("通道 %1 未启动，无法发送").arg(channel));
    return false;
  }

  CHANNEL_HANDLE chanHandle = m_d->activeChans[channel];

  const canid_t canId =
      MAKE_CAN_ID(frame.id, frame.extended ? 1 : 0, frame.remote ? 1 : 0, 0);

  UINT sent = 0;
  if (frame.fd) {
    ZCAN_TransmitFD_Data tx;
    memset(&tx, 0, sizeof(tx));
    tx.transmit_type = 0;
    tx.frame.can_id = canId;
    int len = frame.data.size();
    if (len > CANFD_MAX_DLEN)
      len = CANFD_MAX_DLEN;
    tx.frame.len = static_cast<BYTE>(len);
    tx.frame.flags = frame.brs ? CANFD_BRS : 0;
    memcpy(tx.frame.data, frame.data.constData(), len);
    sent = m_d->transmitFd(chanHandle, &tx, 1);
  } else {
    ZCAN_Transmit_Data tx;
    memset(&tx, 0, sizeof(tx));
    tx.transmit_type = 0;
    tx.frame.can_id = canId;
    int len = frame.data.size();
    if (len > CAN_MAX_DLEN)
      len = CAN_MAX_DLEN;
    tx.frame.can_dlc = static_cast<BYTE>(len);
    if (!frame.remote) {
      memcpy(tx.frame.data, frame.data.constData(), len);
    }
    sent = m_d->transmit(chanHandle, &tx, 1);
  }

  if (sent != 1) {
    emit errorOccurred(QStringLiteral("发送报文失败"));
    return false;
  }

  CanFrame echo = frame;
  echo.channel = channel;
  echo.timestamp = QDateTime::currentMSecsSinceEpoch();
  emit frameSent(echo);
  return true;
}

void CanInterface::pollReceive() {
  if (m_d->activeChans.isEmpty()) {
    return;
  }

  for (auto it = m_d->activeChans.begin(); it != m_d->activeChans.end(); ++it) {
    int channelIdx = it.key();
    CHANNEL_HANDLE chanHandle = it.value();
    if (chanHandle == INVALID_CHANNEL_HANDLE) continue;

    // 经典 CAN 接收
    UINT classicNum = m_d->getReceiveNum(chanHandle, TYPE_CAN);
    int handled = 0;
    while (classicNum > 0 && handled < kMaxFramesPerPoll) {
      ZCAN_Receive_Data buf[64];
      UINT want = qMin<UINT>(classicNum, 64);
      UINT got = m_d->receive(chanHandle, buf, want, 0);
      if (got == 0) {
        break;
      }
      for (UINT i = 0; i < got; ++i) {
        const can_frame &f = buf[i].frame;
        CanFrame frame;
        frame.id = GET_ID(f.can_id);
        frame.extended = IS_EFF(f.can_id);
        frame.remote = IS_RTR(f.can_id);
        frame.errorFrame = IS_ERR(f.can_id);
        frame.fd = false;
        frame.channel = channelIdx;
        int len = qMin<int>(f.can_dlc, CAN_MAX_DLEN);
        if (!frame.remote) {
          frame.data = QByteArray(reinterpret_cast<const char *>(f.data), len);
        }
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        emit frameReceived(frame);
      }
      handled += got;
      classicNum -= got;
    }

    // CAN FD 接收
    UINT fdNum = m_d->getReceiveNum(chanHandle, TYPE_CANFD);
    handled = 0;
    while (fdNum > 0 && handled < kMaxFramesPerPoll) {
      ZCAN_ReceiveFD_Data buf[32];
      UINT want = qMin<UINT>(fdNum, 32);
      UINT got = m_d->receiveFd(chanHandle, buf, want, 0);
      if (got == 0) {
        break;
      }
      for (UINT i = 0; i < got; ++i) {
        const canfd_frame &f = buf[i].frame;
        CanFrame frame;
        frame.id = GET_ID(f.can_id);
        frame.extended = IS_EFF(f.can_id);
        frame.remote = false;
        frame.errorFrame = IS_ERR(f.can_id);
        frame.fd = true;
        frame.brs = (f.flags & CANFD_BRS) != 0;
        frame.channel = channelIdx;
        int len = qMin<int>(f.len, CANFD_MAX_DLEN);
        frame.data = QByteArray(reinterpret_cast<const char *>(f.data), len);
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        emit frameReceived(frame);
      }
      handled += got;
      fdNum -= got;
    }
  }
}

bool CanInterface::diagnoseDriver(QString *driverVer, QString *deviceName,
                                  bool *online) const {
  if (!m_d->lib.isLoaded() || !m_d->diagnosticsAvailable()) {
    return false;
  }

  DEVICE_HANDLE dev = m_d->openDevice(kUsbCanFd200U, 0, 0);
  if (dev == INVALID_DEVICE_HANDLE) {
    return false;
  }

  ZCAN_DEVICE_INFO_EX info;
  memset(&info, 0, sizeof(info));
  const UINT result = m_d->getDeviceInfoEx(dev, &info);
  if (result == STATUS_OK) {
    if (driverVer) {
      *driverVer = QStringLiteral("%1.%2.%3")
                       .arg(info.library_version.major_version)
                       .arg(info.library_version.minor_version)
                       .arg(info.library_version.patch_version);
    }
    if (deviceName) {
      *deviceName = QString::fromLatin1(
          reinterpret_cast<const char *>(info.device_name));
    }
    if (online) {
      *online = (m_d->isDeviceOnLine(dev) == STATUS_ONLINE);
    }
  }

  m_d->closeDevice(dev);
  return result == STATUS_OK;
}
