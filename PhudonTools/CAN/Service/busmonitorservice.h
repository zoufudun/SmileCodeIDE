#ifndef BUSMONITORSERVICE_H
#define BUSMONITORSERVICE_H

#include <QMap>
#include <QObject>
#include <QTimer>
#include "../Adapter/icanadapter.h"

struct ChannelMonitorStats {
  int channel = 0;
  quint64 totalRx = 0;
  quint64 totalTx = 0;
  quint64 totalErrors = 0;
  double rxFps = 0.0;
  double txFps = 0.0;
  double busLoadPercent = 0.0;
};

class BusMonitorService : public QObject {
  Q_OBJECT
public:
  explicit BusMonitorService(ICanAdapter *adapter = nullptr, QObject *parent = nullptr);
  ~BusMonitorService() override;

  void setAdapter(ICanAdapter *adapter);
  ICanAdapter *adapter() const { return m_adapter; }

  ChannelMonitorStats channelStats(int channel) const;

signals:
  void frameDispatched(const CanRawFrame &frame, bool isTx);
  void statsUpdated(int channel, const ChannelMonitorStats &stats);

private slots:
  void onFrameReceived(const CanRawFrame &frame);
  void onFrameSent(const CanRawFrame &frame);
  void onStatsTimer();

private:
  ICanAdapter *m_adapter = nullptr;
  QTimer *m_statsTimer = nullptr;

  struct ChannelCounter {
    quint64 totalRx = 0;
    quint64 totalTx = 0;
    quint64 totalErrors = 0;
    quint64 periodRx = 0;
    quint64 periodTx = 0;
    quint64 periodRxBytes = 0;
    quint64 periodTxBytes = 0;
    double busLoad = 0.0;
  };

  QMap<int, ChannelCounter> m_counters;
};

#endif // BUSMONITORSERVICE_H
