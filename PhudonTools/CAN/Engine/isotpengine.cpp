#include "isotpengine.h"
#include <QDebug>

IsoTpEngine::IsoTpEngine(ICanAdapter *adapter, QObject *parent)
    : QObject(parent), m_adapter(nullptr) {
  m_txTimer = new QTimer(this);
  m_txTimer->setSingleShot(true);
  connect(m_txTimer, &QTimer::timeout, this, &IsoTpEngine::onSendNextConsecutiveFrame);

  m_rxTimer = new QTimer(this);
  m_rxTimer->setSingleShot(true);
  connect(m_rxTimer, &QTimer::timeout, this, &IsoTpEngine::onTimeout);

  setAdapter(adapter);
}

IsoTpEngine::~IsoTpEngine() {
  reset();
}

void IsoTpEngine::setAdapter(ICanAdapter *adapter) {
  if (m_adapter) {
    disconnect(m_adapter, &ICanAdapter::frameReceived, this, &IsoTpEngine::onFrameReceived);
  }
  m_adapter = adapter;
  if (m_adapter) {
    connect(m_adapter, &ICanAdapter::frameReceived, this, &IsoTpEngine::onFrameReceived);
  }
}

void IsoTpEngine::setConfig(const IsoTpConfig &cfg) {
  m_cfg = cfg;
}

void IsoTpEngine::reset() {
  m_txTimer->stop();
  m_rxTimer->stop();
  m_txState = TxState::Idle;
  m_rxState = RxState::Idle;
  m_txBuffer.clear();
  m_rxBuffer.clear();
  m_txOffset = 0;
  m_rxExpectedLength = 0;
}

bool IsoTpEngine::sendData(const QByteArray &data) {
  if (!m_adapter || !m_adapter->isChannelRunning(m_cfg.channel)) {
    emit errorOccurred("CAN 通道未运行");
    return false;
  }
  if (data.isEmpty()) return false;
  if (m_txState != TxState::Idle) {
    emit errorOccurred("传输正在进行中，请等待完成");
    return false;
  }

  int maxSfLen = m_cfg.isFd ? 63 : 7;
  if (data.size() <= maxSfLen) {
    // 1. 发送单帧 (Single Frame)
    CanRawFrame frame;
    frame.id = m_cfg.txId;
    frame.channel = m_cfg.channel;
    frame.fd = m_cfg.isFd;

    QByteArray payload;
    payload.append(static_cast<char>(data.size() & 0x0F)); // SF PCI: 0x0L
    payload.append(data);

    // 填充到 8 字节
    if (!m_cfg.isFd) {
      while (payload.size() < 8) payload.append(static_cast<char>(0xAA));
    }
    frame.data = payload;

    bool ok = m_adapter->sendFrame(m_cfg.channel, frame);
    if (ok) {
      emit transmitProgress(data.size(), data.size());
      emit transmitCompleted(true);
    } else {
      emit transmitCompleted(false);
    }
    return ok;
  }

  // 2. 发送首帧 (First Frame)
  m_txBuffer = data;
  m_txOffset = 0;
  m_txSeqNumber = 1;

  CanRawFrame ff;
  ff.id = m_cfg.txId;
  ff.channel = m_cfg.channel;
  ff.fd = m_cfg.isFd;

  QByteArray ffPayload;
  int totalLen = data.size();
  ffPayload.append(static_cast<char>(0x10 | ((totalLen >> 8) & 0x0F)));
  ffPayload.append(static_cast<char>(totalLen & 0xFF));

  int ffDataLen = m_cfg.isFd ? 62 : 6;
  ffPayload.append(data.left(ffDataLen));
  m_txOffset = ffDataLen;
  ff.data = ffPayload;

  m_txState = TxState::WaitFlowControl;
  m_txTimer->start(m_cfg.timeoutBs); // 等待流控超时

  bool ok = m_adapter->sendFrame(m_cfg.channel, ff);
  if (!ok) {
    reset();
    emit transmitCompleted(false);
    return false;
  }

  emit transmitProgress(m_txOffset, totalLen);
  return true;
}

void IsoTpEngine::onFrameReceived(const CanRawFrame &frame) {
  // 只接收匹配目标 RxID 的报文
  if (frame.id != m_cfg.rxId || frame.channel != m_cfg.channel) return;
  if (frame.data.isEmpty()) return;

  quint8 pci = static_cast<quint8>(frame.data.at(0)) >> 4;

  switch (pci) {
  case 0x0: // Single Frame
    handleSingleFrame(frame);
    break;
  case 0x1: // First Frame
    handleFirstFrame(frame);
    break;
  case 0x2: // Consecutive Frame
    handleConsecutiveFrame(frame);
    break;
  case 0x3: // Flow Control Frame
    handleFlowControl(frame);
    break;
  default:
    break;
  }
}

void IsoTpEngine::handleSingleFrame(const CanRawFrame &frame) {
  quint8 len = static_cast<quint8>(frame.data.at(0)) & 0x0F;
  if (len == 0 && frame.data.size() > 1 && m_cfg.isFd) {
    len = static_cast<quint8>(frame.data.at(1));
    if (frame.data.size() >= len + 2) {
      emit dataReceived(frame.data.mid(2, len));
    }
  } else if (frame.data.size() >= len + 1) {
    emit dataReceived(frame.data.mid(1, len));
  }
}

void IsoTpEngine::handleFirstFrame(const CanRawFrame &frame) {
  if (frame.data.size() < 2) return;
  quint16 totalLen = ((static_cast<quint8>(frame.data.at(0)) & 0x0F) << 8) | static_cast<quint8>(frame.data.at(1));

  m_rxBuffer = frame.data.mid(2);
  m_rxExpectedLength = totalLen;
  m_rxExpectedSeq = 1;
  m_rxBlockRemaining = m_cfg.blockSize;
  m_rxState = RxState::ReceivingConsecutive;

  // 发送流控帧 (Flow Control - CTS 继续发送)
  sendFlowControl(0x00);
  m_rxTimer->start(m_cfg.timeoutCr);
}

void IsoTpEngine::handleConsecutiveFrame(const CanRawFrame &frame) {
  if (m_rxState != RxState::ReceivingConsecutive) return;
  if (frame.data.size() < 2) return;

  quint8 seq = static_cast<quint8>(frame.data.at(0)) & 0x0F;
  if (seq != m_rxExpectedSeq) {
    emit errorOccurred(QString("ISO-TP 序号错误: 预期 %1, 实际 %2").arg(m_rxExpectedSeq).arg(seq));
    reset();
    return;
  }

  m_rxExpectedSeq = (m_rxExpectedSeq + 1) & 0x0F;
  int needed = m_rxExpectedLength - m_rxBuffer.size();
  int chunk = qMin(needed, frame.data.size() - 1);
  m_rxBuffer.append(frame.data.mid(1, chunk));

  m_rxTimer->start(m_cfg.timeoutCr);

  if (m_rxBuffer.size() >= m_rxExpectedLength) {
    m_rxState = RxState::Idle;
    m_rxTimer->stop();
    emit dataReceived(m_rxBuffer);
    m_rxBuffer.clear();
  } else if (m_cfg.blockSize > 0) {
    m_rxBlockRemaining--;
    if (m_rxBlockRemaining <= 0) {
      m_rxBlockRemaining = m_cfg.blockSize;
      sendFlowControl(0x00); // 下一包流控
    }
  }
}

void IsoTpEngine::handleFlowControl(const CanRawFrame &frame) {
  if (m_txState != TxState::WaitFlowControl) return;
  if (frame.data.size() < 3) return;

  quint8 fs = static_cast<quint8>(frame.data.at(0)) & 0x0F;
  quint8 bs = static_cast<quint8>(frame.data.at(1));
  quint8 stMin = static_cast<quint8>(frame.data.at(2));

  m_txTimer->stop();

  if (fs == 0x00) { // CTS
    m_txState = TxState::SendingConsecutive;
    m_txBlockRemaining = bs;
    int interval = (stMin <= 127) ? stMin : 10;
    if (interval < 2) interval = 2; // 最小 2ms
    m_txTimer->setInterval(interval);
    onSendNextConsecutiveFrame();
  } else if (fs == 0x01) { // WT 等待
    m_txTimer->start(m_cfg.timeoutBs);
  } else { // 溢出或错误
    emit errorOccurred("ISO-TP 流控帧返回溢出/错误");
    reset();
    emit transmitCompleted(false);
  }
}

void IsoTpEngine::sendFlowControl(quint8 flowStatus) {
  if (!m_adapter) return;
  CanRawFrame fc;
  fc.id = m_cfg.txId;
  fc.channel = m_cfg.channel;
  fc.fd = m_cfg.isFd;

  QByteArray data;
  data.append(static_cast<char>(0x30 | (flowStatus & 0x0F))); // FC PCI
  data.append(static_cast<char>(m_cfg.blockSize));
  data.append(static_cast<char>(m_cfg.stMin));

  if (!m_cfg.isFd) {
    while (data.size() < 8) data.append(static_cast<char>(0xAA));
  }
  fc.data = data;
  m_adapter->sendFrame(m_cfg.channel, fc);
}

void IsoTpEngine::onSendNextConsecutiveFrame() {
  if (m_txState != TxState::SendingConsecutive || !m_adapter) return;

  if (m_txOffset >= m_txBuffer.size()) {
    // 发送完成
    int total = m_txBuffer.size();
    reset();
    emit transmitProgress(total, total);
    emit transmitCompleted(true);
    return;
  }

  CanRawFrame cf;
  cf.id = m_cfg.txId;
  cf.channel = m_cfg.channel;
  cf.fd = m_cfg.isFd;

  QByteArray payload;
  payload.append(static_cast<char>(0x20 | (m_txSeqNumber & 0x0F)));
  m_txSeqNumber = (m_txSeqNumber + 1) & 0x0F;

  int maxChunk = m_cfg.isFd ? 63 : 7;
  int remaining = m_txBuffer.size() - m_txOffset;
  int chunk = qMin(maxChunk, remaining);
  payload.append(m_txBuffer.mid(m_txOffset, chunk));
  m_txOffset += chunk;

  if (!m_cfg.isFd) {
    while (payload.size() < 8) payload.append(static_cast<char>(0xAA));
  }
  cf.data = payload;

  m_adapter->sendFrame(m_cfg.channel, cf);
  emit transmitProgress(m_txOffset, m_txBuffer.size());

  if (m_txBlockRemaining > 0) {
    m_txBlockRemaining--;
    if (m_txBlockRemaining == 0) {
      m_txState = TxState::WaitFlowControl;
      m_txTimer->start(m_cfg.timeoutBs);
      return;
    }
  }

  // 下一帧
  m_txTimer->start();
}

void IsoTpEngine::onTimeout() {
  emit errorOccurred("ISO-TP 传输超时");
  reset();
  emit transmitCompleted(false);
}
