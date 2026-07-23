#include "canopenmaster.h"

// CANopen 预定义连接集 COB-ID 基地址
namespace {
constexpr quint32 kNmtId = 0x000;
constexpr quint32 kSyncId = 0x080;
constexpr quint32 kEmcyBase = 0x080;
constexpr quint32 kTpdoBase[4] = {0x180, 0x280, 0x380, 0x480};
constexpr quint32 kRpdoBase[4] = {0x200, 0x300, 0x400, 0x500};
constexpr quint32 kSdoTxBase = 0x580; // 服务器(节点)->客户端(主站)
constexpr quint32 kSdoRxBase = 0x600; // 客户端(主站)->服务器(节点)
constexpr quint32 kHeartbeatBase = 0x700;
constexpr int kSdoTimeoutMs = 1000;
} // namespace

CanOpenMaster::CanOpenMaster(CanInterface *iface, QObject *parent)
    : QObject(parent), m_iface(iface) {
  m_sdoTimer.setSingleShot(true);
  m_sdoTimer.setInterval(kSdoTimeoutMs);
  connect(&m_sdoTimer, &QTimer::timeout, this, &CanOpenMaster::onSdoTimeout);
}

bool CanOpenMaster::sendNmt(NmtCommand command, quint8 targetNode) {
  CanFrame frame;
  frame.id = kNmtId;
  frame.extended = false;
  frame.data.append(static_cast<char>(static_cast<quint8>(command)));
  frame.data.append(static_cast<char>(targetNode));
  emit logMessage(QStringLiteral("NMT 命令 0x%1 -> 节点 %2")
                      .arg(static_cast<int>(command), 2, 16, QChar('0'))
                      .arg(targetNode == 0 ? QStringLiteral("ALL")
                                           : QString::number(targetNode)));
  return m_iface->sendFrame(frame);
}

bool CanOpenMaster::sendSync() {
  CanFrame frame;
  frame.id = kSyncId;
  frame.extended = false;
  emit logMessage(QStringLiteral("发送 SYNC"));
  return m_iface->sendFrame(frame);
}

bool CanOpenMaster::sendSdoRequest(const QByteArray &payload) {
  if (m_sdoBusy) {
    emit logMessage(QStringLiteral("SDO 忙，已有未完成的传输"));
    return false;
  }
  CanFrame frame;
  frame.id = kSdoRxBase + m_nodeId;
  frame.extended = false;
  frame.data = payload;
  if (!m_iface->sendFrame(frame)) {
    return false;
  }
  m_sdoBusy = true;
  m_sdoTimer.start();
  return true;
}

bool CanOpenMaster::sdoWrite(quint16 index, quint8 subIndex, quint32 value,
                             int size) {
  if (size < 1 || size > 4) {
    return false;
  }
  // 快速下载命令字: n = 4-size 的未用字节数
  // 0x23(4B) 0x27(3B) 0x2B(2B) 0x2F(1B)
  const quint8 ccs = static_cast<quint8>(0x23 | ((4 - size) << 2));

  QByteArray payload(8, '\0');
  payload[0] = static_cast<char>(ccs);
  payload[1] = static_cast<char>(index & 0xFF);
  payload[2] = static_cast<char>((index >> 8) & 0xFF);
  payload[3] = static_cast<char>(subIndex);
  for (int i = 0; i < size; ++i) {
    payload[4 + i] = static_cast<char>((value >> (8 * i)) & 0xFF);
  }

  m_sdoIsRead = false;
  m_sdoIndex = index;
  m_sdoSubIndex = subIndex;
  emit logMessage(QStringLiteral("SDO 写 [%1:%2] = 0x%3 (%4 字节)")
                      .arg(index, 4, 16, QChar('0'))
                      .arg(subIndex)
                      .arg(value, 0, 16)
                      .arg(size));
  return sendSdoRequest(payload);
}

bool CanOpenMaster::sdoRead(quint16 index, quint8 subIndex) {
  QByteArray payload(8, '\0');
  payload[0] = static_cast<char>(0x40); // 上传请求
  payload[1] = static_cast<char>(index & 0xFF);
  payload[2] = static_cast<char>((index >> 8) & 0xFF);
  payload[3] = static_cast<char>(subIndex);

  m_sdoIsRead = true;
  m_sdoIndex = index;
  m_sdoSubIndex = subIndex;
  emit logMessage(QStringLiteral("SDO 读 [%1:%2]")
                      .arg(index, 4, 16, QChar('0'))
                      .arg(subIndex));
  return sendSdoRequest(payload);
}

void CanOpenMaster::processFrame(const CanFrame &frame) {
  if (frame.extended) {
    return; // CANopen 使用标准 11 位 ID
  }

  const quint32 fn = frame.id & 0x780; // 功能码
  const quint8 node = static_cast<quint8>(frame.id & 0x7F);

  // 心跳 / 启动报文 (0x700 + nodeId)
  if (fn == kHeartbeatBase && node != 0) {
    NmtState state = NmtState::Unknown;
    if (!frame.data.isEmpty()) {
      const quint8 s = static_cast<quint8>(frame.data.at(0)) & 0x7F;
      switch (s) {
      case 0x00:
        state = NmtState::BootUp;
        break;
      case 0x04:
        state = NmtState::Stopped;
        break;
      case 0x05:
        state = NmtState::Operational;
        break;
      case 0x7F:
        state = NmtState::PreOperational;
        break;
      default:
        state = NmtState::Unknown;
        break;
      }
    }
    emit heartbeatReceived(node, state);
    return;
  }

  // 紧急报文 EMCY (0x080 + nodeId), node != 0
  if (fn == kEmcyBase && node != 0 && frame.id != kSyncId) {
    if (frame.data.size() >= 3) {
      const quint16 errorCode =
          static_cast<quint8>(frame.data.at(0)) |
          (static_cast<quint8>(frame.data.at(1)) << 8);
      const quint8 errorReg = static_cast<quint8>(frame.data.at(2));
      emit emcyReceived(node, errorCode, errorReg, frame.data.mid(3));
    }
    return;
  }

  // SDO 应答 (0x580 + nodeId)
  if (fn == kSdoTxBase && node == m_nodeId) {
    handleSdoResponse(frame);
    return;
  }

  // TPDO (节点发送)
  for (int i = 0; i < 4; ++i) {
    if (fn == kTpdoBase[i] && node != 0) {
      emit pdoReceived(node, i + 1, true, frame.data);
      return;
    }
  }
  // RPDO (主站/其他节点发送)
  for (int i = 0; i < 4; ++i) {
    if (fn == kRpdoBase[i] && node != 0) {
      emit pdoReceived(node, i + 1, false, frame.data);
      return;
    }
  }
}

void CanOpenMaster::handleSdoResponse(const CanFrame &frame) {
  if (!m_sdoBusy || frame.data.isEmpty()) {
    return;
  }
  m_sdoTimer.stop();
  m_sdoBusy = false;

  const quint8 scs = static_cast<quint8>(frame.data.at(0));

  // 中止传输
  if (scs == 0x80) {
    quint32 abortCode = 0;
    if (frame.data.size() >= 8) {
      abortCode = static_cast<quint8>(frame.data.at(4)) |
                  (static_cast<quint8>(frame.data.at(5)) << 8) |
                  (static_cast<quint8>(frame.data.at(6)) << 16) |
                  (static_cast<quint32>(static_cast<quint8>(frame.data.at(7)))
                   << 24);
    }
    if (m_sdoIsRead) {
      emit sdoReadFinished(false, m_sdoIndex, m_sdoSubIndex, 0, abortCode);
    } else {
      emit sdoWriteFinished(false, m_sdoIndex, m_sdoSubIndex, abortCode);
    }
    emit logMessage(QStringLiteral("SDO 中止: %1").arg(abortText(abortCode)));
    return;
  }

  if (m_sdoIsRead) {
    // 上传应答 0x4n：n 的 bit1(s)=1 表示长度有效，bit0..(快速)
    quint32 value = 0;
    int size = 4;
    if ((scs & 0xE0) == 0x40) {
      const bool sizeIndicated = scs & 0x01;
      const bool expedited = scs & 0x02;
      if (expedited && sizeIndicated) {
        size = 4 - ((scs >> 2) & 0x03);
      }
      for (int i = 0; i < size && (4 + i) < frame.data.size(); ++i) {
        value |= static_cast<quint32>(
                     static_cast<quint8>(frame.data.at(4 + i)))
                 << (8 * i);
      }
      emit sdoReadFinished(true, m_sdoIndex, m_sdoSubIndex, value, 0);
      emit logMessage(QStringLiteral("SDO 读完成 [%1:%2] = 0x%3")
                          .arg(m_sdoIndex, 4, 16, QChar('0'))
                          .arg(m_sdoSubIndex)
                          .arg(value, 0, 16));
    } else {
      emit sdoReadFinished(false, m_sdoIndex, m_sdoSubIndex, 0, 0);
    }
  } else {
    // 下载应答 0x60 表示成功
    const bool ok = (scs == 0x60);
    emit sdoWriteFinished(ok, m_sdoIndex, m_sdoSubIndex, 0);
    emit logMessage(ok ? QStringLiteral("SDO 写完成 [%1:%2]")
                             .arg(m_sdoIndex, 4, 16, QChar('0'))
                             .arg(m_sdoSubIndex)
                       : QStringLiteral("SDO 写应答异常"));
  }
}

void CanOpenMaster::onSdoTimeout() {
  if (!m_sdoBusy) {
    return;
  }
  m_sdoBusy = false;
  if (m_sdoIsRead) {
    emit sdoReadFinished(false, m_sdoIndex, m_sdoSubIndex, 0, 0x05040000);
  } else {
    emit sdoWriteFinished(false, m_sdoIndex, m_sdoSubIndex, 0x05040000);
  }
  emit logMessage(QStringLiteral("SDO 超时（节点 %1 无应答）").arg(m_nodeId));
}

QString CanOpenMaster::stateText(NmtState state) {
  switch (state) {
  case NmtState::BootUp:
    return QStringLiteral("启动(Boot-up)");
  case NmtState::Stopped:
    return QStringLiteral("停止(Stopped)");
  case NmtState::Operational:
    return QStringLiteral("运行(Operational)");
  case NmtState::PreOperational:
    return QStringLiteral("预运行(Pre-op)");
  default:
    return QStringLiteral("未知");
  }
}

QString CanOpenMaster::abortText(quint32 abortCode) {
  switch (abortCode) {
  case 0x05040000:
    return QStringLiteral("0x05040000 SDO 协议超时");
  case 0x05040001:
    return QStringLiteral("0x05040001 命令字无效/未知");
  case 0x06010000:
    return QStringLiteral("0x06010000 对象访问不支持");
  case 0x06010001:
    return QStringLiteral("0x06010001 试图读只写对象");
  case 0x06010002:
    return QStringLiteral("0x06010002 试图写只读对象");
  case 0x06020000:
    return QStringLiteral("0x06020000 对象不存在于对象字典");
  case 0x06090011:
    return QStringLiteral("0x06090011 子索引不存在");
  case 0x06090030:
    return QStringLiteral("0x06090030 参数值超出范围");
  case 0x08000000:
    return QStringLiteral("0x08000000 一般错误");
  case 0x08000020:
    return QStringLiteral("0x08000020 数据无法传输或存储");
  default:
    return QStringLiteral("0x%1 中止")
        .arg(abortCode, 8, 16, QChar('0'))
        .toUpper();
  }
}
