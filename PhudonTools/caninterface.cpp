#include "caninterface.h"

#include <cstring>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QMap>
#include <QSet>
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

// 创芯科技 ControlCAN 驱动数据结构定义（Qt 干净类型，防宏冲突）
struct VCI_BOARD_INFO_CX {
  quint16 hw_Version;
  quint16 fw_Version;
  quint16 dr_Version;
  quint16 in_Version;
  quint16 irq_Num;
  quint8 can_Num;
  char str_Serial_Num[20];
  char str_hw_Type[40];
  quint16 Reserved[4];
};

struct VCI_CAN_OBJ_CX {
  quint32 ID;
  quint32 TimeStamp;
  quint8 TimeFlag;
  quint8 SendType;
  quint8 RemoteFlag;
  quint8 ExternFlag;
  quint8 DataLen;
  quint8 Data[8];
  quint8 Reserved[3];
};

struct VCI_INIT_CONFIG_CX {
  quint32 AccCode;
  quint32 AccMask;
  quint32 Reserved;
  quint8 Filter;
  quint8 Timing0;
  quint8 Timing1;
  quint8 Mode;
};

typedef quint32 (ZLG_CALL *Fn_VCI_OpenDevice)(quint32, quint32, quint32);
typedef quint32 (ZLG_CALL *Fn_VCI_CloseDevice)(quint32, quint32);
typedef quint32 (ZLG_CALL *Fn_VCI_InitCAN)(quint32, quint32, quint32, VCI_INIT_CONFIG_CX *);
typedef quint32 (ZLG_CALL *Fn_VCI_StartCAN)(quint32, quint32, quint32);
typedef quint32 (ZLG_CALL *Fn_VCI_ResetCAN)(quint32, quint32, quint32);
typedef quint32 (ZLG_CALL *Fn_VCI_ClearBuffer)(quint32, quint32, quint32);
typedef ulong (ZLG_CALL *Fn_VCI_GetReceiveNum)(quint32, quint32, quint32);
typedef ulong (ZLG_CALL *Fn_VCI_Transmit)(quint32, quint32, quint32, VCI_CAN_OBJ_CX *, ulong);
typedef ulong (ZLG_CALL *Fn_VCI_Receive)(quint32, quint32, quint32, VCI_CAN_OBJ_CX *, ulong, int);
typedef quint32 (ZLG_CALL *Fn_VCI_ReadBoardInfo)(quint32, quint32, VCI_BOARD_INFO_CX *);

quint32 mapCxDeviceType(quint32 deviceType) {
  switch (deviceType) {
    case CX_USBCAN1:     return 3;
    case CX_USBCAN2:     return 4;
    case CX_USBCAN_E_U:  return 20;
    case CX_USBCAN_2E_U: return 21;
    default:             return deviceType;
  }
}

// 将设备类型码映射为可读的设备名称（用于 UI 显示或兜底）
QString deviceTypeName(quint32 deviceType) {
  switch (deviceType) {
    case CX_USBCAN1:       return QStringLiteral("创芯 USBCAN-1C");
    case CX_USBCAN2:       return QStringLiteral("创芯 USBCAN-2C");
    case CX_USBCAN_E_U:    return QStringLiteral("创芯 USBCAN-E-U");
    case CX_USBCAN_2E_U:   return QStringLiteral("创芯 USBCAN-2E-U");
    case ZCAN_USBCAN1:       return QStringLiteral("USBCAN-1");
    case ZCAN_USBCAN2:       return QStringLiteral("USBCAN-2");
    case ZCAN_USBCAN_E_U:    return QStringLiteral("USBCAN-E-U");
    case ZCAN_USBCAN_2E_U:   return QStringLiteral("USBCAN-2E-U");
    case ZCAN_USBCAN_4E_U:   return QStringLiteral("USBCAN-4E-U");
    case ZCAN_USBCAN_8E_U:   return QStringLiteral("USBCAN-8E-U");
    case ZCAN_USBCANFD_200U: return QStringLiteral("USBCANFD-200U");
    case ZCAN_USBCANFD_100U: return QStringLiteral("USBCANFD-100U");
    case ZCAN_USBCANFD_MINI: return QStringLiteral("USBCANFD-MINI");
    case ZCAN_USBCANFD_800U: return QStringLiteral("USBCANFD-800U");
    case ZCAN_USBCANFD_400U: return QStringLiteral("USBCANFD-400U");
    case ZCAN_USBCANFD_800H: return QStringLiteral("USBCANFD-800H");
    case ZCAN_VIRTUAL_DEVICE:return QStringLiteral("VirtualUSBCAN");
    default:                 return QStringLiteral("CAN 设备 (%1)").arg(deviceType);
  }
}

// 根据设备类型返回预期的通道数（当硬件查询失败时的回退值）
int deviceTypeChannelCount(quint32 deviceType) {
  switch (deviceType) {
    case CX_USBCAN1:       return 1;
    case CX_USBCAN2:       return 2;
    case CX_USBCAN_E_U:    return 1;
    case CX_USBCAN_2E_U:   return 2;
    case ZCAN_USBCAN1:       return 1;
    case ZCAN_USBCAN2:       return 2;
    case ZCAN_USBCAN_E_U:    return 1;
    case ZCAN_USBCAN_2E_U:   return 2;
    case ZCAN_USBCAN_4E_U:   return 4;
    case ZCAN_USBCAN_8E_U:   return 8;
    case ZCAN_USBCANFD_100U: return 1;
    case ZCAN_USBCANFD_MINI: return 1;
    case ZCAN_USBCANFD_200U: return 2;
    case ZCAN_USBCANFD_400U: return 4;
    case ZCAN_USBCANFD_800U: return 8;
    case ZCAN_USBCANFD_800H: return 8;
    default:                 return 2;  // 大多数 CAN 设备默认 2 通道
  }
}

// zlgcan 导出函数指针类型
typedef DEVICE_HANDLE(ZLG_CALL *Fn_OpenDevice)(UINT, UINT, UINT);
typedef UINT(ZLG_CALL *Fn_CloseDevice)(DEVICE_HANDLE);
typedef UINT(ZLG_CALL *Fn_IsDeviceOnLine)(DEVICE_HANDLE);
typedef UINT(ZLG_CALL *Fn_GetDeviceInfoEx)(DEVICE_HANDLE, ZCAN_DEVICE_INFO_EX *);
typedef UINT(ZLG_CALL *Fn_GetDeviceInf)(DEVICE_HANDLE, ZCAN_DEVICE_INFO *);
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

bool CanInterface::isControlCanDevice(quint32 deviceType) {
  return (deviceType == CX_USBCAN1 || deviceType == CX_USBCAN2 ||
          deviceType == CX_USBCAN_E_U || deviceType == CX_USBCAN_2E_U);
}

struct CanInterface::Impl {
  QLibrary lib;
  QLibrary libCx;

  Fn_OpenDevice openDevice = nullptr;
  Fn_CloseDevice closeDevice = nullptr;
  Fn_IsDeviceOnLine isDeviceOnLine = nullptr;
  Fn_GetDeviceInfoEx getDeviceInfoEx = nullptr;
  Fn_GetDeviceInf getDeviceInf = nullptr;
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

  // ControlCAN (创芯科技 / VCI) 函数指针
  Fn_VCI_OpenDevice vciOpenDevice = nullptr;
  Fn_VCI_CloseDevice vciCloseDevice = nullptr;
  Fn_VCI_InitCAN vciInitCan = nullptr;
  Fn_VCI_StartCAN vciStartCan = nullptr;
  Fn_VCI_ResetCAN vciResetCan = nullptr;
  Fn_VCI_ClearBuffer vciClearBuffer = nullptr;
  Fn_VCI_GetReceiveNum vciGetReceiveNum = nullptr;
  Fn_VCI_Transmit vciTransmit = nullptr;
  Fn_VCI_Receive vciReceive = nullptr;
  Fn_VCI_ReadBoardInfo vciReadBoardInfo = nullptr;

  DEVICE_HANDLE device = INVALID_DEVICE_HANDLE;
  bool isControlCan = false;
  quint32 cxDevType = 4;
  quint32 cxDevIndex = 0;
  QSet<int> activeCxChans;

  QMap<int, CHANNEL_HANDLE> activeChans;
  quint32 deviceType = kUsbCanFd200U;
  int deviceIndex = 0;
  QMap<int, CanChannelConfig> chanConfigs;
  QMap<int, QList<CanFilterRule>> chanFilters;

  bool resolved() const {
    return openDevice && closeDevice && initCan && startCan && getReceiveNum &&
           transmit && receive && transmitFd && receiveFd && setValue;
  }
  bool vciResolved() const {
    return vciOpenDevice && vciCloseDevice && vciInitCan && vciStartCan &&
           vciGetReceiveNum && vciTransmit && vciReceive;
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

bool CanInterface::isDeviceFdCapable(quint32 deviceType) {
  // USBCANFD 系列 (41+) 及虚拟设备 (99) 支持 CAN FD，
  // 经典 VCI CAN 设备 (3, 4, 20, 21, 31, 34 等) 仅支持经典 CAN。
  if (deviceType == ZCAN_VIRTUAL_DEVICE) {
    return true;
  }
  return deviceType >= 41;
}

bool CanInterface::libraryLoaded() const { return m_d->resolved() || m_d->vciResolved(); }

bool CanInterface::loadLibrary() {
  // 1. 加载 zlgcan.dll
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
  if (loaded) {
    m_d->openDevice = reinterpret_cast<Fn_OpenDevice>(m_d->lib.resolve("ZCAN_OpenDevice"));
    m_d->closeDevice = reinterpret_cast<Fn_CloseDevice>(m_d->lib.resolve("ZCAN_CloseDevice"));
    m_d->isDeviceOnLine = reinterpret_cast<Fn_IsDeviceOnLine>(m_d->lib.resolve("ZCAN_IsDeviceOnLine"));
    m_d->getDeviceInfoEx = reinterpret_cast<Fn_GetDeviceInfoEx>(m_d->lib.resolve("ZCAN_GetDeviceInfoEx"));
    m_d->getDeviceInf = reinterpret_cast<Fn_GetDeviceInf>(m_d->lib.resolve("ZCAN_GetDeviceInf"));
    m_d->initCan = reinterpret_cast<Fn_InitCAN>(m_d->lib.resolve("ZCAN_InitCAN"));
    m_d->startCan = reinterpret_cast<Fn_StartCAN>(m_d->lib.resolve("ZCAN_StartCAN"));
    m_d->resetCan = reinterpret_cast<Fn_ResetCAN>(m_d->lib.resolve("ZCAN_ResetCAN"));
    m_d->clearBuffer = reinterpret_cast<Fn_ClearBuffer>(m_d->lib.resolve("ZCAN_ClearBuffer"));
    m_d->getReceiveNum = reinterpret_cast<Fn_GetReceiveNum>(m_d->lib.resolve("ZCAN_GetReceiveNum"));
    m_d->transmit = reinterpret_cast<Fn_Transmit>(m_d->lib.resolve("ZCAN_Transmit"));
    m_d->receive = reinterpret_cast<Fn_Receive>(m_d->lib.resolve("ZCAN_Receive"));
    m_d->transmitFd = reinterpret_cast<Fn_TransmitFD>(m_d->lib.resolve("ZCAN_TransmitFD"));
    m_d->receiveFd = reinterpret_cast<Fn_ReceiveFD>(m_d->lib.resolve("ZCAN_ReceiveFD"));
    m_d->setValue = reinterpret_cast<Fn_SetValue>(m_d->lib.resolve("ZCAN_SetValue"));
  }

  // 2. 加载 创芯科技 ControlCAN.dll
  const QStringList cxCandidates = {appDir + "/ControlCAN.dll",
                                    appDir + "/CXCAN/ControlCAN.dll",
                                    QStringLiteral("ControlCAN")};
  bool cxLoaded = false;
  for (const QString &name : cxCandidates) {
    m_d->libCx.setFileName(name);
    if (m_d->libCx.load()) {
      cxLoaded = true;
      break;
    }
  }
  if (cxLoaded) {
    m_d->vciOpenDevice = reinterpret_cast<Fn_VCI_OpenDevice>(m_d->libCx.resolve("VCI_OpenDevice"));
    m_d->vciCloseDevice = reinterpret_cast<Fn_VCI_CloseDevice>(m_d->libCx.resolve("VCI_CloseDevice"));
    m_d->vciInitCan = reinterpret_cast<Fn_VCI_InitCAN>(m_d->libCx.resolve("VCI_InitCAN"));
    m_d->vciStartCan = reinterpret_cast<Fn_VCI_StartCAN>(m_d->libCx.resolve("VCI_StartCAN"));
    m_d->vciResetCan = reinterpret_cast<Fn_VCI_ResetCAN>(m_d->libCx.resolve("VCI_ResetCAN"));
    m_d->vciClearBuffer = reinterpret_cast<Fn_VCI_ClearBuffer>(m_d->libCx.resolve("VCI_ClearBuffer"));
    m_d->vciGetReceiveNum = reinterpret_cast<Fn_VCI_GetReceiveNum>(m_d->libCx.resolve("VCI_GetReceiveNum"));
    m_d->vciTransmit = reinterpret_cast<Fn_VCI_Transmit>(m_d->libCx.resolve("VCI_Transmit"));
    m_d->vciReceive = reinterpret_cast<Fn_VCI_Receive>(m_d->libCx.resolve("VCI_Receive"));
    m_d->vciReadBoardInfo = reinterpret_cast<Fn_VCI_ReadBoardInfo>(m_d->libCx.resolve("VCI_ReadBoardInfo"));
  }

  if (!m_d->resolved() && !m_d->vciResolved()) {
    m_libError = QStringLiteral("未找到有效的 CAN 驱动库 (zlgcan.dll 或 ControlCAN.dll)");
    return false;
  }
  return true;
}

bool CanInterface::setDeviceBaud(int channel, const CanChannelConfig &cfg) {
  // 通过属性接口设置通道参数，须在 InitCAN 之前调用。
  QByteArray chStr = QByteArray::number(channel);
  QByteArray path;

  if (cfg.isFd) {
    // ---- CAN FD 设备属性 ----
    path = chStr + "/canfd_abit_baud_rate";
    m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.abitBaud).constData());

    path = chStr + "/canfd_dbit_baud_rate";
    m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.dbitBaud).constData());

    path = chStr + "/canfd_standard";
    m_d->setValue(m_d->device, path.constData(), cfg.isIso ? "0" : "1");

    path = chStr + "/canfd_brs";
    m_d->setValue(m_d->device, path.constData(), cfg.enableBrs ? "1" : "0");

    // 内部终端电阻（仅 CAN FD 设备支持）
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
  } else {
    // ---- 经典 CAN 设备属性（如 USBCAN-4E-U） ----
    // 经典 CAN 设备使用 "baud_rate" 属性路径设置波特率
    path = chStr + "/baud_rate";
    m_d->setValue(m_d->device, path.constData(), QByteArray::number(cfg.abitBaud).constData());
    // 注：经典 CAN 设备通常不支持终端电阻、总线利用率上报和发送重试等属性，
    // 仅设置波特率即可。
  }

  return true;
}

bool CanInterface::isOpen() const {
  return !m_d->activeChans.isEmpty();
}

quint32 CanInterface::deviceType() const {
  return m_d->deviceType;
}

int CanInterface::deviceIndex() const {
  return m_d->deviceIndex;
}

bool CanInterface::openDevice(quint32 deviceType, int deviceIndex) {
  if (isDeviceOpen()) {
    return true;
  }

  m_d->deviceType = deviceType;
  m_d->deviceIndex = deviceIndex;

  if (isControlCanDevice(deviceType)) {
    if (!m_d->vciResolved()) {
      emit errorOccurred(QStringLiteral("ControlCAN.dll 驱动未就绪"));
      return false;
    }
    m_d->isControlCan = true;
    m_d->cxDevType = mapCxDeviceType(deviceType);
    m_d->cxDevIndex = static_cast<quint32>(deviceIndex);

    quint32 ret = m_d->vciOpenDevice(m_d->cxDevType, m_d->cxDevIndex, 0);
    if (ret != 1) {
      m_d->isControlCan = false;
      emit errorOccurred(QStringLiteral("打开创芯 USBCAN 设备失败 (类型 %1, 索引 %2)").arg(deviceType).arg(deviceIndex));
      return false;
    }
    return true;
  }

  if (!m_d->resolved()) {
    emit errorOccurred(QStringLiteral("zlgcan 动态库未就绪：%1").arg(m_libError));
    return false;
  }

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

  if (m_d->isControlCan) {
    QList<int> activeChansList = m_d->activeCxChans.toList();
    for (int channelIdx : activeChansList) {
      stopChannel(channelIdx);
    }
    m_d->vciCloseDevice(m_d->cxDevType, m_d->cxDevIndex);
    m_d->isControlCan = false;
    m_d->activeCxChans.clear();
    m_open = false;
    emit disconnected();
    return;
  }

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
  return m_d->isControlCan || (m_d->device != INVALID_DEVICE_HANDLE);
}

bool CanInterface::startChannel(int channel, const CanChannelConfig &cfg) {
  if (!isDeviceOpen()) {
    emit errorOccurred(QStringLiteral("设备未打开，无法启动通道"));
    return false;
  }

  if (isChannelRunning(channel)) {
    stopChannel(channel);
  }

  if (m_d->isControlCan) {
    VCI_INIT_CONFIG_CX initConfig;
    memset(&initConfig, 0, sizeof(initConfig));
    initConfig.AccCode = 0x00000000;
    initConfig.AccMask = 0xFFFFFFFF;
    initConfig.Filter = 1;
    initConfig.Mode = (cfg.mode == CanMode::ListenOnly ? 1 : 0);

    switch (cfg.abitBaud) {
      case 1000000: initConfig.Timing0 = 0x00; initConfig.Timing1 = 0x14; break;
      case 800000:  initConfig.Timing0 = 0x00; initConfig.Timing1 = 0x16; break;
      case 500000:  initConfig.Timing0 = 0x00; initConfig.Timing1 = 0x1C; break;
      case 250000:  initConfig.Timing0 = 0x01; initConfig.Timing1 = 0x1C; break;
      case 125000:  initConfig.Timing0 = 0x03; initConfig.Timing1 = 0x1C; break;
      case 100000:  initConfig.Timing0 = 0x04; initConfig.Timing1 = 0x1C; break;
      case 50000:   initConfig.Timing0 = 0x09; initConfig.Timing1 = 0x1C; break;
      case 20000:   initConfig.Timing0 = 0x18; initConfig.Timing1 = 0x1C; break;
      case 10000:   initConfig.Timing0 = 0x31; initConfig.Timing1 = 0x1C; break;
      case 5000:    initConfig.Timing0 = 0xBF; initConfig.Timing1 = 0xFF; break;
      default:      initConfig.Timing0 = 0x00; initConfig.Timing1 = 0x1C; break;
    }

    if (m_d->vciInitCan(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(channel), &initConfig) != 1) {
      emit errorOccurred(QStringLiteral("创芯 USBCAN 通道 %1 初始化失败").arg(channel));
      return false;
    }

    if (m_d->vciStartCan(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(channel)) != 1) {
      emit errorOccurred(QStringLiteral("创芯 USBCAN 通道 %1 启动失败").arg(channel));
      return false;
    }

    if (m_d->vciClearBuffer) {
      m_d->vciClearBuffer(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(channel));
    }

    m_d->activeCxChans.insert(channel);
    m_d->chanConfigs[channel] = cfg;
    m_open = true;
    m_channel = channel;
    m_fdEnabled = false;

    if (!m_timer->isActive()) {
      m_timer->start();
    }

    emit connected();
    return true;
  }

  if (!setDeviceBaud(channel, cfg)) {
    return false;
  }

  // 根据协议类型选择 CAN 类型和对应的初始化配置 union 成员
  ZCAN_CHANNEL_INIT_CONFIG zCfg;
  memset(&zCfg, 0, sizeof(zCfg));
  if (cfg.isFd) {
    zCfg.can_type = TYPE_CANFD;
    zCfg.canfd.mode = (cfg.mode == CanMode::ListenOnly) ? 1 : 0;
    zCfg.canfd.acc_code = 0;
    zCfg.canfd.acc_mask = 0xFFFFFFFF;
    zCfg.canfd.filter = 0;
  } else {
    zCfg.can_type = TYPE_CAN;
    zCfg.can.mode = (cfg.mode == CanMode::ListenOnly) ? 1 : 0;
    zCfg.can.acc_code = 0;
    zCfg.can.acc_mask = 0xFFFFFFFF;
    zCfg.can.filter = 0;
  }

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
  m_d->chanConfigs[channel] = cfg;
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
  if (m_d->isControlCan) {
    if (!m_d->activeCxChans.contains(channel)) {
      return true;
    }
    if (m_d->vciResetCan) {
      m_d->vciResetCan(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(channel));
    }
    m_d->activeCxChans.remove(channel);
    m_d->chanConfigs.remove(channel);
    m_d->chanFilters.remove(channel);
    if (m_d->activeCxChans.isEmpty()) {
      m_timer->stop();
      m_open = false;
    }
    return true;
  }

  if (!m_d->activeChans.contains(channel)) {
    return true;
  }

  CHANNEL_HANDLE chanHandle = m_d->activeChans[channel];
  if (m_d->resetCan) {
    m_d->resetCan(chanHandle);
  }

  m_d->activeChans.remove(channel);
  m_d->chanConfigs.remove(channel);
  m_d->chanFilters.remove(channel);
  if (m_d->activeChans.isEmpty()) {
    m_timer->stop();
    m_open = false;
  }
  return true;
}

bool CanInterface::isChannelRunning(int channel) const {
  return m_d->activeCxChans.contains(channel) || m_d->activeChans.contains(channel);
}

CanChannelConfig CanInterface::channelConfig(int channel) const {
  if (m_d->chanConfigs.contains(channel)) {
    return m_d->chanConfigs[channel];
  }
  return CanChannelConfig();
}

void CanInterface::setChannelConfig(int channel, const CanChannelConfig &cfg) {
  m_d->chanConfigs[channel] = cfg;
}

void CanInterface::setChannelFilters(int channel, const QList<CanFilterRule> &rules) {
  m_d->chanFilters[channel] = rules;
}

QList<CanFilterRule> CanInterface::channelFilters(int channel) const {
  if (m_d->chanFilters.contains(channel)) {
    return m_d->chanFilters[channel];
  }
  return QList<CanFilterRule>();
}

bool CanInterface::matchesFilter(int channel, const CanFrame &frame) const {
  if (!m_d->chanConfigs.contains(channel)) {
    return true;
  }
  const auto &cfg = m_d->chanConfigs[channel];
  if (!cfg.enableFilter) {
    return true;
  }
  if (!m_d->chanFilters.contains(channel)) {
    return false; // Enabled but no rules => drop everything
  }
  const auto &rules = m_d->chanFilters[channel];
  if (rules.isEmpty()) {
    return false; // Whitelist is empty => drop everything
  }

  for (const auto &rule : rules) {
    bool isExtendedRule = (rule.mode == 1 || rule.mode == 3);
    bool isSegmentRule = (rule.mode == 2 || rule.mode == 3);

    // Frame type must match rule frame type (standard vs extended)
    if (frame.extended != isExtendedRule) {
      continue;
    }

    if (isSegmentRule) {
      if (frame.id >= rule.startId && frame.id <= rule.endId) {
        return true;
      }
    } else {
      if (frame.id == rule.startId) {
        return true;
      }
    }
  }
  return false;
}

bool CanInterface::getDeviceInformation(QString *hwVer, QString *fwVer, QString *drVer,
                                         QString *libVer, int *canNum, QString *serial,
                                         QString *typeStr) const {
  if (!isDeviceOpen()) {
    return false;
  }

  if (m_d->isControlCan) {
    VCI_BOARD_INFO_CX info;
    memset(&info, 0, sizeof(info));
    if (m_d->vciReadBoardInfo && m_d->vciReadBoardInfo(m_d->cxDevType, m_d->cxDevIndex, &info) == 1) {
      if (hwVer)  *hwVer  = QString("V%1.%2").arg(info.hw_Version >> 8).arg(info.hw_Version & 0xFF, 2, 16, QChar('0'));
      if (fwVer)  *fwVer  = QString("V%1.%2").arg(info.fw_Version >> 8).arg(info.fw_Version & 0xFF, 2, 16, QChar('0'));
      if (drVer)  *drVer  = QString("V%1.%2").arg(info.dr_Version >> 8).arg(info.dr_Version & 0xFF, 2, 16, QChar('0'));
      if (libVer) *libVer = QString("V%1.%2").arg(info.in_Version >> 8).arg(info.in_Version & 0xFF, 2, 16, QChar('0'));
      if (canNum) *canNum = info.can_Num;
      if (serial) *serial = QString::fromLatin1(reinterpret_cast<const char*>(info.str_Serial_Num)).trimmed();
      if (typeStr) *typeStr = QString::fromLatin1(reinterpret_cast<const char*>(info.str_hw_Type)).trimmed();
      if (typeStr && typeStr->isEmpty()) {
        *typeStr = deviceTypeName(m_d->deviceType);
      }
      return true;
    }
    if (hwVer)   *hwVer   = QStringLiteral("—");
    if (fwVer)   *fwVer   = QStringLiteral("—");
    if (drVer)   *drVer   = QStringLiteral("—");
    if (libVer)  *libVer  = QStringLiteral("—");
    if (canNum)  *canNum  = deviceTypeChannelCount(m_d->deviceType);
    if (serial)  *serial  = QStringLiteral("—");
    if (typeStr) *typeStr = deviceTypeName(m_d->deviceType);
    return true;
  }

  // 1. 尝试使用扩展接口获取设备信息（主要用于 USBCANFD 系列及支持该接口的设备）
  ZCAN_DEVICE_INFO_EX infoEx;
  memset(&infoEx, 0, sizeof(infoEx));
  bool queryOk = false;

  if (m_d->getDeviceInfoEx) {
    queryOk = (m_d->getDeviceInfoEx(m_d->device, &infoEx) == STATUS_OK);
  }

  if (queryOk) {
    if (hwVer) {
      *hwVer = QString("V%1.%2%3")
                   .arg(infoEx.hardware_version.major_version)
                   .arg(infoEx.hardware_version.minor_version, 2, 10, QChar('0'))
                   .arg(infoEx.hardware_version.patch_version, 2, 10, QChar('0'));
    }
    if (fwVer) {
      *fwVer = QString("V%1.%2%3")
                   .arg(infoEx.firmware_version.major_version)
                   .arg(infoEx.firmware_version.minor_version, 2, 10, QChar('0'))
                   .arg(infoEx.firmware_version.patch_version, 2, 10, QChar('0'));
    }
    if (drVer) {
      *drVer = QString("V%1.%2%3")
                   .arg(infoEx.driver_version.major_version)
                   .arg(infoEx.driver_version.minor_version, 2, 10, QChar('0'))
                   .arg(infoEx.driver_version.patch_version, 2, 10, QChar('0'));
    }
    if (libVer) {
      *libVer = QString("V%1.%2%3")
                    .arg(infoEx.library_version.major_version)
                    .arg(infoEx.library_version.minor_version, 2, 10, QChar('0'))
                    .arg(infoEx.library_version.patch_version, 2, 10, QChar('0'));
    }
    if (canNum) {
      *canNum = infoEx.can_channel_number;
    }
    if (serial) {
      *serial = QString::fromLatin1(reinterpret_cast<const char*>(infoEx.serial_number)).trimmed();
    }
    if (typeStr) {
      *typeStr = QString::fromLatin1(reinterpret_cast<const char*>(infoEx.device_name)).trimmed();
      if (typeStr->isEmpty()) {
        *typeStr = deviceTypeName(m_d->deviceType);
      }
    }
  } else {
    // 2. 如果扩展接口失败或不支持，尝试使用经典接口获取设备信息（主要用于经典 VCI 系列设备如 USBCAN-4E-U 等）
    ZCAN_DEVICE_INFO info;
    memset(&info, 0, sizeof(info));
    if (m_d->getDeviceInf && m_d->getDeviceInf(m_d->device, &info) == STATUS_OK) {
      queryOk = true;
      if (hwVer) {
        *hwVer = QString("V%1.%2")
                     .arg(info.hw_Version >> 8)
                     .arg(info.hw_Version & 0xFF, 2, 16, QChar('0'));
      }
      if (fwVer) {
        *fwVer = QString("V%1.%2")
                     .arg(info.fw_Version >> 8)
                     .arg(info.fw_Version & 0xFF, 2, 16, QChar('0'));
      }
      if (drVer) {
        *drVer = QString("V%1.%2")
                     .arg(info.dr_Version >> 8)
                     .arg(info.dr_Version & 0xFF, 2, 16, QChar('0'));
      }
      if (libVer) {
        *libVer = QString("V%1.%2")
                      .arg(info.in_Version >> 8)
                      .arg(info.in_Version & 0xFF, 2, 16, QChar('0'));
      }
      if (canNum) {
        *canNum = info.can_Num;
      }
      if (serial) {
        *serial = QString::fromLatin1(reinterpret_cast<const char*>(info.str_Serial_Num)).trimmed();
      }
      if (typeStr) {
        *typeStr = QString::fromLatin1(reinterpret_cast<const char*>(info.str_hw_Type)).trimmed();
        if (typeStr->isEmpty()) {
          *typeStr = deviceTypeName(m_d->deviceType);
        }
      }
    } else {
      // 3. 两者都失败时，进行兜底
      if (hwVer)    *hwVer    = QStringLiteral("—");
      if (fwVer)    *fwVer    = QStringLiteral("—");
      if (drVer)    *drVer    = QStringLiteral("—");
      if (libVer)   *libVer   = QStringLiteral("—");
      if (canNum)   *canNum   = deviceTypeChannelCount(m_d->deviceType);
      if (serial)   *serial   = QStringLiteral("—");
      if (typeStr)  *typeStr  = deviceTypeName(m_d->deviceType);
    }
  }

  return queryOk;
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
  if (m_d->isControlCan) {
    if (!m_d->activeCxChans.contains(channel)) {
      emit errorOccurred(QStringLiteral("通道 %1 未启动，无法发送").arg(channel));
      return false;
    }

    VCI_CAN_OBJ_CX obj;
    memset(&obj, 0, sizeof(obj));
    obj.ID = frame.id;
    obj.SendType = static_cast<quint8>(frame.transmitType);
    obj.RemoteFlag = frame.remote ? 1 : 0;
    obj.ExternFlag = frame.extended ? 1 : 0;
    obj.DataLen = static_cast<quint8>(qMin(frame.data.size(), 8));
    memcpy(obj.Data, frame.data.constData(), obj.DataLen);

    ulong ret = m_d->vciTransmit(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(channel), &obj, 1);
    if (ret != 1) {
      emit errorOccurred(QStringLiteral("创芯 USBCAN 发送报文失败"));
      return false;
    }

    CanFrame echo = frame;
    echo.channel = channel;
    echo.timestamp = QDateTime::currentMSecsSinceEpoch();
    emit frameSent(echo);
    return true;
  }

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
    tx.transmit_type = static_cast<UINT>(frame.transmitType);
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
    tx.transmit_type = static_cast<UINT>(frame.transmitType);
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
  if (m_d->isControlCan) {
    if (m_d->activeCxChans.isEmpty()) return;

    for (int ch : m_d->activeCxChans) {
      ulong num = m_d->vciGetReceiveNum(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(ch));
      if (num > 0) {
        VCI_CAN_OBJ_CX buf[128];
        ulong got = m_d->vciReceive(m_d->cxDevType, m_d->cxDevIndex, static_cast<quint32>(ch), buf, qMin(num, (ulong)128), 0);
        for (ulong i = 0; i < got; ++i) {
          CanFrame f;
          f.id = buf[i].ID;
          f.extended = (buf[i].ExternFlag != 0);
          f.remote = (buf[i].RemoteFlag != 0);
          f.fd = false;
          f.channel = ch;
          int len = qMin<int>(buf[i].DataLen, 8);
          if (!f.remote) {
            f.data = QByteArray(reinterpret_cast<const char*>(buf[i].Data), len);
          }
          f.timestamp = QDateTime::currentMSecsSinceEpoch();
          if (matchesFilter(ch, f)) {
            emit frameReceived(f);
          }
        }
      }
    }
    return;
  }
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
        if (matchesFilter(channelIdx, frame)) {
          emit frameReceived(frame);
        }
      }
      handled += got;
      classicNum -= got;
    }

    // CAN FD 接收（仅 FD 模式下才查询，经典 CAN 设备无 FD 缓冲区）
    if (m_fdEnabled) {
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
        if (matchesFilter(channelIdx, frame)) {
          emit frameReceived(frame);
        }
      }
      handled += got;
      fdNum -= got;
    }
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

