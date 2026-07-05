#include "caninterface.h"

#include <cstring>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
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
  CHANNEL_HANDLE chan = INVALID_CHANNEL_HANDLE;
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

bool CanInterface::setDeviceBaud(int channel, int abitBaud, int dbitBaud,
                                 bool terminalRes) {
  // USBCANFD 系列通过属性接口设置波特率，须在 InitCAN 之前调用。
  // 路径形如 "<channel>/canfd_abit_baud_rate"
  const QByteArray abit = QByteArray::number(abitBaud);
  const QByteArray dbit = QByteArray::number(dbitBaud);

  QByteArray path = QByteArray::number(channel) + "/canfd_abit_baud_rate";
  if (m_d->setValue(m_d->device, path.constData(), abit.constData()) !=
      STATUS_OK) {
    emit errorOccurred(QStringLiteral("设置仲裁域波特率失败"));
    return false;
  }

  path = QByteArray::number(channel) + "/canfd_dbit_baud_rate";
  if (m_d->setValue(m_d->device, path.constData(), dbit.constData()) !=
      STATUS_OK) {
    emit errorOccurred(QStringLiteral("设置数据域波特率失败"));
    return false;
  }

  // 内部终端电阻
  const QByteArray res = terminalRes ? "1" : "0";
  path = QByteArray::number(channel) + "/initenal_resistance";
  m_d->setValue(m_d->device, path.constData(), res.constData());
  return true;
}

bool CanInterface::open(quint32 deviceType, int deviceIndex, int channel,
                        int abitBaud, bool enableFd, int dbitBaud, CanMode mode,
                        bool terminalRes) {
  if (m_open) {
    close();
  }
  if (!m_d->resolved()) {
    emit errorOccurred(
        QStringLiteral("zlgcan 动态库未就绪：%1").arg(m_libError));
    return false;
  }

  m_d->deviceType = deviceType;
  m_d->deviceIndex = deviceIndex;
  m_channel = channel;
  m_fdEnabled = enableFd;

  // zlgcan.dll 在 OpenDevice 时会从 "kerneldlls" 子目录加载设备内核库，
  // 该目录是相对于进程工作目录查找的。这里临时把工作目录切到程序目录，
  // 保证能找到与程序一起分发的 kerneldlls，函数返回时自动还原。
  const QString appDir = QCoreApplication::applicationDirPath();
  const QString prevCwd = QDir::currentPath();
  QDir::setCurrent(appDir);
  auto cwdGuard = qScopeGuard([prevCwd]() { QDir::setCurrent(prevCwd); });

  m_d->device = m_d->openDevice(deviceType, static_cast<UINT>(deviceIndex), 0);
  if (m_d->device == INVALID_DEVICE_HANDLE) {
    const bool hasKernel = QDir(appDir + "/kerneldlls").exists();
    const QString kernelInfo =
        hasKernel ? QStringLiteral("已找到 kerneldlls: %1/kerneldlls").arg(appDir)
                  : QStringLiteral("未找到 kerneldlls，缺失目录: %1/kerneldlls")
                        .arg(appDir);
    QString driverVer, devName;
    bool online = false;
    const bool diagOk = diagnoseDriver(&driverVer, &devName, &online);
    if (diagOk) {
      emit errorOccurred(QStringLiteral("打开设备失败（索引 %1，设备类型 %2）\n"
                                        "驱动版本: %3\n"
                                        "设备: %4 %5\n"
                                        "%6")
                             .arg(deviceIndex)
                             .arg(deviceType)
                             .arg(driverVer)
                             .arg(devName)
                             .arg(online ? "在线" : "离线")
                             .arg(kernelInfo));
    } else {
      emit errorOccurred(
          QStringLiteral("打开设备失败（索引 %1，类型 %2）。请逐项排查：\n"
                         "1) 设备已通过 USB 连接，并已安装 ZLG USBCANFD 驱动"
                         "（设备管理器可见）；\n"
                         "2) 设备未被 ZCANPRO 等其他程序占用；\n"
                         "3) %3；\n"
                         "4) 设备索引正确（仅接 1 台时应为 0）。")
              .arg(deviceIndex)
              .arg(deviceType)
              .arg(kernelInfo));
    }
    return false;
  }

  if (!setDeviceBaud(channel, abitBaud, enableFd ? dbitBaud : abitBaud,
                     terminalRes)) {
    m_d->closeDevice(m_d->device);
    m_d->device = INVALID_DEVICE_HANDLE;
    return false;
  }

  ZCAN_CHANNEL_INIT_CONFIG cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.can_type = TYPE_CANFD; // 200U 为 CANFD 设备，固定为 1
  cfg.canfd.mode = (mode == CanMode::ListenOnly) ? 1 : 0;
  cfg.canfd.acc_code = 0;
  cfg.canfd.acc_mask = 0xFFFFFFFF; // 接收全部 ID
  cfg.canfd.filter = 0;

  m_d->chan = m_d->initCan(m_d->device, static_cast<UINT>(channel), &cfg);
  if (m_d->chan == INVALID_CHANNEL_HANDLE) {
    emit errorOccurred(QStringLiteral("初始化 CAN 通道 %1 失败").arg(channel));
    m_d->closeDevice(m_d->device);
    m_d->device = INVALID_DEVICE_HANDLE;
    return false;
  }

  if (m_d->startCan(m_d->chan) != STATUS_OK) {
    emit errorOccurred(QStringLiteral("启动 CAN 通道失败"));
    m_d->closeDevice(m_d->device);
    m_d->device = INVALID_DEVICE_HANDLE;
    m_d->chan = INVALID_CHANNEL_HANDLE;
    return false;
  }

  if (m_d->clearBuffer) {
    m_d->clearBuffer(m_d->chan);
  }

  m_open = true;
  m_timer->start();
  emit connected();
  return true;
}

void CanInterface::close() {
  if (!m_open) {
    return;
  }
  m_timer->stop();
  if (m_d->resetCan && m_d->chan != INVALID_CHANNEL_HANDLE) {
    m_d->resetCan(m_d->chan);
  }
  if (m_d->closeDevice && m_d->device != INVALID_DEVICE_HANDLE) {
    m_d->closeDevice(m_d->device);
  }
  m_d->device = INVALID_DEVICE_HANDLE;
  m_d->chan = INVALID_CHANNEL_HANDLE;
  m_open = false;
  emit disconnected();
}

bool CanInterface::sendFrame(const CanFrame &frame) {
  if (!m_open) {
    emit errorOccurred(QStringLiteral("CAN 通道未打开"));
    return false;
  }
  if (frame.fd && !m_fdEnabled) {
    emit errorOccurred(QStringLiteral("当前未启用 CAN FD，无法发送 FD 帧"));
    return false;
  }

  const canid_t canId =
      MAKE_CAN_ID(frame.id, frame.extended ? 1 : 0, frame.remote ? 1 : 0, 0);

  UINT sent = 0;
  if (frame.fd) {
    ZCAN_TransmitFD_Data tx;
    memset(&tx, 0, sizeof(tx));
    tx.transmit_type = 0; // 正常发送
    tx.frame.can_id = canId;
    int len = frame.data.size();
    if (len > CANFD_MAX_DLEN)
      len = CANFD_MAX_DLEN;
    tx.frame.len = static_cast<BYTE>(len);
    tx.frame.flags = frame.brs ? CANFD_BRS : 0;
    memcpy(tx.frame.data, frame.data.constData(), len);
    sent = m_d->transmitFd(m_d->chan, &tx, 1);
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
    sent = m_d->transmit(m_d->chan, &tx, 1);
  }

  if (sent != 1) {
    emit errorOccurred(QStringLiteral("发送报文失败"));
    return false;
  }

  CanFrame echo = frame;
  echo.timestamp = QDateTime::currentMSecsSinceEpoch();
  emit frameSent(echo);
  return true;
}

void CanInterface::pollReceive() {
  if (!m_open) {
    return;
  }

  // 经典 CAN 帧
  UINT classicNum = m_d->getReceiveNum(m_d->chan, TYPE_CAN);
  int handled = 0;
  while (classicNum > 0 && handled < kMaxFramesPerPoll) {
    ZCAN_Receive_Data buf[64];
    UINT want = qMin<UINT>(classicNum, 64);
    UINT got = m_d->receive(m_d->chan, buf, want, 0);
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

  // CAN FD 帧
  UINT fdNum = m_d->getReceiveNum(m_d->chan, TYPE_CANFD);
  handled = 0;
  while (fdNum > 0 && handled < kMaxFramesPerPoll) {
    ZCAN_ReceiveFD_Data buf[32];
    UINT want = qMin<UINT>(fdNum, 32);
    UINT got = m_d->receiveFd(m_d->chan, buf, want, 0);
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
      int len = qMin<int>(f.len, CANFD_MAX_DLEN);
      frame.data = QByteArray(reinterpret_cast<const char *>(f.data), len);
      frame.timestamp = QDateTime::currentMSecsSinceEpoch();
      emit frameReceived(frame);
    }
    handled += got;
    fdNum -= got;
  }
}

bool CanInterface::diagnoseDriver(QString *driverVer, QString *deviceName,
                                  bool *online) const {
  if (!m_d->lib.isLoaded() || !m_d->diagnosticsAvailable()) {
    return false;
  }

  // 打开一个临时句柄（设备索引 0）获取版本和设备信息
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