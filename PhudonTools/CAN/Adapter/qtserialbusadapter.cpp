#include "qtserialbusadapter.h"
#include <QDateTime>
#include <QDebug>

QtSerialBusAdapter::QtSerialBusAdapter(const QString &pluginName, QObject *parent)
    : ICanAdapter(parent), m_pluginName(pluginName) {}

QtSerialBusAdapter::~QtSerialBusAdapter() {
  closeDevice();
}

QString QtSerialBusAdapter::displayName() const {
  if (m_pluginName == "peakcan") return "PEAK PCAN 硬件适配器 (Qt)";
  if (m_pluginName == "vectorcan") return "Vector VN 系列适配器 (Qt)";
  if (m_pluginName == "passthrucan") return "SAE J2534 PassThru 适配器 (Qt)";
  if (m_pluginName == "systeccan") return "SYS TEC USB-CANmodul (Qt)";
  if (m_pluginName == "tinycan") return "MHS TinyCAN USB 适配器 (Qt)";
  if (m_pluginName == "virtualcan") return "Qt VirtualCAN 虚拟通道 (Qt)";
  if (m_pluginName == "socketcan") return "Linux SocketCAN 适配器 (Qt)";
  return QString("Qt %1 适配器").arg(m_pluginName);
}

QStringList QtSerialBusAdapter::availableInterfaces(const QString &pluginName) {
  QStringList list;
  if (!QCanBus::instance()->plugins().contains(pluginName)) {
    return list;
  }
  QString errorString;
  const QList<QCanBusDeviceInfo> devices = QCanBus::instance()->availableDevices(pluginName, &errorString);
  for (const QCanBusDeviceInfo &info : devices) {
    list << info.name();
  }
  return list;
}

bool QtSerialBusAdapter::openDevice(const CanDeviceParams &params) {
  if (m_isOpen) closeDevice();
  m_deviceParams = params;
  m_isOpen = true;
  emit deviceOpened();
  return true;
}

void QtSerialBusAdapter::closeDevice() {
  if (!m_isOpen) return;

  for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
    QCanBusDevice *dev = it.value();
    if (dev) {
      dev->disconnectDevice();
      delete dev;
    }
    emit channelStopped(it.key());
  }
  m_devices.clear();
  m_isOpen = false;
  emit deviceClosed();
}

bool QtSerialBusAdapter::isDeviceOpen() const {
  return m_isOpen;
}

bool QtSerialBusAdapter::startChannel(int channel, const CanChannelParams &cfg) {
  if (!m_isOpen) return false;

  stopChannel(channel);

  QString ifaceName = m_deviceParams.deviceName;
  if (ifaceName.isEmpty()) {
    // 默认生成接口名
    if (m_pluginName == "peakcan") {
      ifaceName = QString("usb%1").arg(channel);
    } else if (m_pluginName == "vectorcan") {
      ifaceName = QString("can%1").arg(channel);
    } else if (m_pluginName == "virtualcan") {
      ifaceName = QString("can%1").arg(channel);
    } else {
      ifaceName = QString("can%1").arg(channel);
    }
  } else if (channel > 0 && !ifaceName.contains(QString::number(channel))) {
    ifaceName = QString("%1_%2").arg(ifaceName).arg(channel);
  }

  QString err;
  QCanBusDevice *device = QCanBus::instance()->createDevice(m_pluginName, ifaceName, &err);
  if (!device) {
    emit errorOccurred(QString("创建 %1 设备 [%2] 失败: %3").arg(m_pluginName, ifaceName, err));
    return false;
  }

  // 配置波特率与工作模式
  device->setConfigurationParameter(QCanBusDevice::BitRateKey, cfg.abitBaud);
  if (cfg.isFd) {
    device->setConfigurationParameter(QCanBusDevice::CanFdKey, true);
    device->setConfigurationParameter(QCanBusDevice::DataBitRateKey, cfg.dbitBaud);
  }
  if (cfg.mode == CanWorkMode::ListenOnly) {
    // 某些插件支持监听模式
    device->setConfigurationParameter(QCanBusDevice::ReceiveOwnKey, false);
  } else if (cfg.mode == CanWorkMode::Loopback) {
    device->setConfigurationParameter(QCanBusDevice::LoopbackKey, true);
  }

  // 连接信号
  device->setProperty("channelIndex", channel);
  connect(device, &QCanBusDevice::framesReceived, this, &QtSerialBusAdapter::onFramesReceived);
  connect(device, &QCanBusDevice::framesWritten, this, &QtSerialBusAdapter::onFramesWritten);
  connect(device, &QCanBusDevice::errorOccurred, this, &QtSerialBusAdapter::onErrorOccurred);
  connect(device, &QCanBusDevice::stateChanged, this, &QtSerialBusAdapter::onStateChanged);

  if (!device->connectDevice()) {
    QString connErr = device->errorString();
    emit errorOccurred(QString("连接 %1 通道 %2 失败: %3").arg(m_pluginName).arg(channel).arg(connErr));
    delete device;
    return false;
  }

  m_devices[channel] = device;
  m_configs[channel] = cfg;
  m_stats[channel] = CanBusStatistics();

  emit channelStarted(channel);
  return true;
}

bool QtSerialBusAdapter::stopChannel(int channel) {
  if (m_devices.contains(channel)) {
    QCanBusDevice *dev = m_devices.take(channel);
    if (dev) {
      dev->disconnectDevice();
      delete dev;
    }
    emit channelStopped(channel);
    return true;
  }
  return false;
}

bool QtSerialBusAdapter::isChannelRunning(int channel) const {
  if (!m_isOpen || !m_devices.contains(channel)) return false;
  QCanBusDevice *dev = m_devices.value(channel);
  return dev && dev->state() == QCanBusDevice::ConnectedState;
}

CanChannelParams QtSerialBusAdapter::channelConfig(int channel) const {
  return m_configs.value(channel, CanChannelParams());
}

bool QtSerialBusAdapter::sendFrame(int channel, const CanRawFrame &frame) {
  if (!isChannelRunning(channel)) return false;

  QCanBusDevice *dev = m_devices.value(channel);
  if (!dev) return false;

  QCanBusFrame qframe;
  qframe.setFrameId(frame.id);
  qframe.setExtendedFrameFormat(frame.extended);
  qframe.setPayload(frame.data);

  if (frame.remote) {
    qframe.setFrameType(QCanBusFrame::RemoteRequestFrame);
  } else if (frame.errorFrame) {
    qframe.setFrameType(QCanBusFrame::ErrorFrame);
  } else {
    qframe.setFrameType(QCanBusFrame::DataFrame);
  }

  if (frame.fd) {
    qframe.setFlexibleDataRateFormat(true);
    qframe.setBitrateSwitch(frame.brs);
  }

  bool ok = dev->writeFrame(qframe);
  if (ok) {
    CanRawFrame sent = frame;
    sent.channel = channel;
    if (sent.timestamp == 0) sent.timestamp = QDateTime::currentMSecsSinceEpoch();

    auto &stat = m_stats[channel];
    stat.txFrames++;
    stat.txBytes += sent.data.size();

    emit frameSent(sent);
  } else {
    emit errorOccurred(QString("发送失败: %1").arg(dev->errorString()));
  }
  return ok;
}

CanBusStatistics QtSerialBusAdapter::statistics(int channel) const {
  return m_stats.value(channel, CanBusStatistics());
}

void QtSerialBusAdapter::onFramesReceived() {
  QCanBusDevice *dev = qobject_cast<QCanBusDevice*>(sender());
  if (!dev) return;

  int ch = dev->property("channelIndex").toInt();
  auto &stat = m_stats[ch];

  while (dev->framesAvailable()) {
    QCanBusFrame qf = dev->readFrame();
    CanRawFrame rf;
    rf.id = qf.frameId();
    rf.extended = qf.hasExtendedFrameFormat();
    rf.remote = (qf.frameType() == QCanBusFrame::RemoteRequestFrame);
    rf.errorFrame = (qf.frameType() == QCanBusFrame::ErrorFrame);
    rf.fd = qf.hasFlexibleDataRateFormat();
    rf.brs = qf.hasBitrateSwitch();
    rf.data = qf.payload();
    rf.channel = ch;

    // 时间戳
    if (qf.timeStamp().seconds() > 0) {
      rf.timestamp = qf.timeStamp().seconds() * 1000 + qf.timeStamp().microSeconds() / 1000;
    } else {
      rf.timestamp = QDateTime::currentMSecsSinceEpoch();
    }

    stat.rxFrames++;
    stat.rxBytes += rf.data.size();

    emit frameReceived(rf);
  }
}

void QtSerialBusAdapter::onFramesWritten(qint64 count) {
  Q_UNUSED(count);
}

void QtSerialBusAdapter::onErrorOccurred(QCanBusDevice::CanBusError error) {
  QCanBusDevice *dev = qobject_cast<QCanBusDevice*>(sender());
  QString msg = dev ? dev->errorString() : QString("Error %1").arg(error);
  emit errorOccurred(msg);
}

void QtSerialBusAdapter::onStateChanged(QCanBusDevice::CanBusDeviceState state) {
  QCanBusDevice *dev = qobject_cast<QCanBusDevice*>(sender());
  if (!dev) return;
  int ch = dev->property("channelIndex").toInt();
  if (state == QCanBusDevice::UnconnectedState) {
    emit busStatusChanged(ch, true, 0, 0);
  }
}
