#ifndef QTSERIALBUSADAPTER_H
#define QTSERIALBUSADAPTER_H

#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QMap>
#include "icanadapter.h"

class QtSerialBusAdapter : public ICanAdapter {
  Q_OBJECT
public:
  explicit QtSerialBusAdapter(const QString &pluginName, QObject *parent = nullptr);
  ~QtSerialBusAdapter() override;

  QString adapterType() const override { return m_pluginName; }
  QString displayName() const override;

  bool openDevice(const CanDeviceParams &params) override;
  void closeDevice() override;
  bool isDeviceOpen() const override;

  bool startChannel(int channel, const CanChannelParams &cfg) override;
  bool stopChannel(int channel) override;
  bool isChannelRunning(int channel) const override;
  CanChannelParams channelConfig(int channel) const override;

  bool sendFrame(int channel, const CanRawFrame &frame) override;
  CanBusStatistics statistics(int channel) const override;

  // 获取插件支持的可用接口列表
  static QStringList availableInterfaces(const QString &pluginName);

private slots:
  void onFramesReceived();
  void onFramesWritten(qint64 count);
  void onErrorOccurred(QCanBusDevice::CanBusError error);
  void onStateChanged(QCanBusDevice::CanBusDeviceState state);

private:
  QString m_pluginName;
  CanDeviceParams m_deviceParams;
  bool m_isOpen = false;

  // QtCanBus 是一个通道对应一个 QCanBusDevice 实例
  QMap<int, QCanBusDevice*> m_devices;
  QMap<int, CanChannelParams> m_configs;
  QMap<int, CanBusStatistics> m_stats;
};

#endif // QTSERIALBUSADAPTER_H
