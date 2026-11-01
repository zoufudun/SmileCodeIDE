#include "busmonitorservice.h"

BusMonitorService::BusMonitorService(ICanAdapter *adapter, QObject *parent)
    : QObject(parent), m_adapter(nullptr) {
  m_statsTimer = new QTimer(this);
  connect(m_statsTimer, &QTimer::timeout, this, &BusMonitorService::onStatsTimer);
  m_statsTimer->start(1000); // 1秒计算一次帧率与负载率

  setAdapter(adapter);
}

BusMonitorService::~BusMonitorService() {
  m_statsTimer->stop();
}

void BusMonitorService::setAdapter(ICanAdapter *adapter) {
  if (m_adapter) {
    disconnect(m_adapter, &ICanAdapter::frameReceived, this, &BusMonitorService::onFrameReceived);
    disconnect(m_adapter, &ICanAdapter::frameSent, this, &BusMonitorService::onFrameSent);
  }
  m_adapter = adapter;
  if (m_adapter) {
    connect(m_adapter, &ICanAdapter::frameReceived, this, &BusMonitorService::onFrameReceived);
    connect(m_adapter, &ICanAdapter::frameSent, this, &BusMonitorService::onFrameSent);
  }
}

ChannelMonitorStats BusMonitorService::channelStats(int channel) const {
  ChannelMonitorStats res;
  res.channel = channel;
  if (m_counters.contains(channel)) {
    const auto &c = m_counters.value(channel);
    res.totalRx = c.totalRx;
    res.totalTx = c.totalTx;
    res.totalErrors = c.totalErrors;
    res.rxFps = static_cast<double>(c.periodRx);
    res.txFps = static_cast<double>(c.periodTx);
    res.busLoadPercent = c.busLoad;
  }
  return res;
}

void BusMonitorService::onFrameReceived(const CanRawFrame &frame) {
  auto &c = m_counters[frame.channel];
  c.totalRx++;
  c.periodRx++;
  c.periodRxBytes += frame.data.size();
  if (frame.errorFrame) c.totalErrors++;

  emit frameDispatched(frame, false);
}

void BusMonitorService::onFrameSent(const CanRawFrame &frame) {
  auto &c = m_counters[frame.channel];
  c.totalTx++;
  c.periodTx++;
  c.periodTxBytes += frame.data.size();
  if (frame.errorFrame) c.totalErrors++;

  emit frameDispatched(frame, true);
}

void BusMonitorService::onStatsTimer() {
  for (auto it = m_counters.begin(); it != m_counters.end(); ++it) {
    int ch = it.key();
    auto &c = it.value();

    ChannelMonitorStats s;
    s.channel = ch;
    s.totalRx = c.totalRx;
    s.totalTx = c.totalTx;
    s.totalErrors = c.totalErrors;
    s.rxFps = static_cast<double>(c.periodRx);
    s.txFps = static_cast<double>(c.periodTx);

    // 计算总线负载率：以 500kbps 估算，标准帧约 110 bit
    quint64 totalBits = (c.periodRx + c.periodTx) * 110 + (c.periodRxBytes + c.periodTxBytes) * 8;
    double baud = 500000.0;
    if (m_adapter) {
      CanChannelParams p = m_adapter->channelConfig(ch);
      if (p.abitBaud > 0) baud = p.abitBaud;
    }
    double load = (totalBits / baud) * 100.0;
    c.busLoad = qBound(0.0, load, 100.0);
    s.busLoadPercent = c.busLoad;

    // 清空周期计数
    c.periodRx = 0;
    c.periodTx = 0;
    c.periodRxBytes = 0;
    c.periodTxBytes = 0;

    emit statsUpdated(ch, s);
  }
}
