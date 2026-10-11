#ifndef SIMULATIONCANADAPTER_H
#define SIMULATIONCANADAPTER_H

#include <QMap>
#include <QObject>
#include <QTimer>
#include "icanadapter.h"

class SimulationCanAdapter : public ICanAdapter {
  Q_OBJECT
public:
  explicit SimulationCanAdapter(QObject *parent = nullptr);
  ~SimulationCanAdapter() override;

  QString adapterType() const override { return "simulation"; }
  QString displayName() const override { return "纯软件仿真适配器 (离线全真模拟)"; }

  bool openDevice(const CanDeviceParams &params) override;
  void closeDevice() override;
  bool isDeviceOpen() const override;

  bool startChannel(int channel, const CanChannelParams &cfg) override;
  bool stopChannel(int channel) override;
  bool isChannelRunning(int channel) const override;
  CanChannelParams channelConfig(int channel) const override;

  bool sendFrame(int channel, const CanRawFrame &frame) override;
  CanBusStatistics statistics(int channel) const override;

  bool getDeviceInfo(QString *hwVer, QString *fwVer, QString *drVer,
                     QString *libVer, int *canNum, QString *serial,
                     QString *typeStr) const override;

  // 仿真特有控制
  void setEcuSimulationEnabled(bool enabled);
  bool isEcuSimulationEnabled() const { return m_ecuSimulation; }

private slots:
  void onSimulationTick();

private:
  void processUdsSimulation(int channel, const CanRawFrame &frame);

  bool m_isOpen = false;
  bool m_ecuSimulation = true;
  QMap<int, bool> m_channelRunning;
  QMap<int, CanChannelParams> m_configs;
  QMap<int, CanBusStatistics> m_stats;

  QTimer *m_simTimer = nullptr;
  double m_simAngle = 0.0;
  int m_simStep = 0;
  quint32 m_expectedSeed = 0x12345678;
  bool m_securityUnlocked = false;
};

#endif // SIMULATIONCANADAPTER_H
