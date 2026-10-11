#include "flashservice.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

FlashService::FlashService(UdsEngine *uds, QObject *parent)
    : QObject(parent), m_uds(nullptr) {
  setUdsEngine(uds);
}

FlashService::~FlashService() {
  abortFlashing();
}

void FlashService::setUdsEngine(UdsEngine *uds) {
  if (m_uds) {
    disconnect(m_uds, &UdsEngine::responseReceived, this, &FlashService::onUdsResponseReceived);
    disconnect(m_uds, &UdsEngine::requestTimeout, this, &FlashService::onUdsTimeout);
    disconnect(m_uds, &UdsEngine::requestFailed, this, &FlashService::onUdsFailed);
  }
  m_uds = uds;
  if (m_uds) {
    connect(m_uds, &UdsEngine::responseReceived, this, &FlashService::onUdsResponseReceived);
    connect(m_uds, &UdsEngine::requestTimeout, this, &FlashService::onUdsTimeout);
    connect(m_uds, &UdsEngine::requestFailed, this, &FlashService::onUdsFailed);
  }
}

void FlashService::setConfig(const FlashConfig &cfg) {
  m_cfg = cfg;
}

quint32 FlashService::totalFirmwareBytes() const {
  quint32 sum = 0;
  for (const auto &seg : m_segments) {
    sum += seg.data.size();
  }
  return sum;
}

bool FlashService::setFirmwareData(quint32 baseAddr, const QByteArray &data) {
  m_segments.clear();
  FlashSegment seg;
  seg.address = baseAddr;
  seg.data = data;
  m_segments.append(seg);
  return true;
}

bool FlashService::loadFirmwareFile(const QString &filePath) {
  m_segments.clear();
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    emit logMessage(QString("无法打开固件文件: %1").arg(filePath));
    return false;
  }

  QFileInfo fi(filePath);
  QString suffix = fi.suffix().toLower();

  if (suffix == "hex") {
    QTextStream in(&file);
    bool ok = parseIntelHex(in.readAll());
    file.close();
    return ok;
  } else if (suffix == "s19" || suffix == "srec" || suffix == "s28" || suffix == "s37") {
    QTextStream in(&file);
    bool ok = parseMotorolaS19(in.readAll());
    file.close();
    return ok;
  } else {
    // 默认作为纯二进制 (Bin)
    QByteArray binData = file.readAll();
    file.close();
    return setFirmwareData(m_cfg.targetAddress, binData);
  }
}

bool FlashService::parseIntelHex(const QString &content) {
  QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
  quint32 upperAddr = 0;
  FlashSegment curSeg;

  for (const QString &l : lines) {
    QString line = l.trimmed();
    if (!line.startsWith(':') || line.size() < 11) continue;

    int len = line.mid(1, 2).toInt(nullptr, 16);
    quint16 offset = line.mid(3, 4).toUShort(nullptr, 16);
    int type = line.mid(7, 2).toInt(nullptr, 16);
    QByteArray data = QByteArray::fromHex(line.mid(9, len * 2).toLatin1());

    if (type == 0x00) { // Data record
      quint32 fullAddr = upperAddr | offset;
      if (curSeg.data.isEmpty()) {
        curSeg.address = fullAddr;
        curSeg.data.append(data);
      } else if (curSeg.address + curSeg.data.size() == fullAddr) {
        curSeg.data.append(data);
      } else {
        m_segments.append(curSeg);
        curSeg.address = fullAddr;
        curSeg.data = data;
      }
    } else if (type == 0x04) { // Extended linear address
      upperAddr = line.mid(9, 4).toUInt(nullptr, 16) << 16;
    } else if (type == 0x01) { // End of file
      break;
    }
  }

  if (!curSeg.data.isEmpty()) {
    m_segments.append(curSeg);
  }
  emit logMessage(QString("解析 Intel HEX 成功，共 %1 个段，总计 %2 字节").arg(m_segments.size()).arg(totalFirmwareBytes()));
  return !m_segments.isEmpty();
}

bool FlashService::parseMotorolaS19(const QString &content) {
  QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
  FlashSegment curSeg;

  for (const QString &l : lines) {
    QString line = l.trimmed();
    if (!line.startsWith('S') || line.size() < 8) continue;
    char type = line.at(1).toLatin1();

    int addrBytes = 0;
    if (type == '1') addrBytes = 2;
    else if (type == '2') addrBytes = 3;
    else if (type == '3') addrBytes = 4;
    else continue;

    int byteCount = line.mid(2, 2).toInt(nullptr, 16);
    quint32 addr = line.mid(4, addrBytes * 2).toUInt(nullptr, 16);
    int dataBytes = byteCount - addrBytes - 1;
    QByteArray data = QByteArray::fromHex(line.mid(4 + addrBytes * 2, dataBytes * 2).toLatin1());

    if (curSeg.data.isEmpty()) {
      curSeg.address = addr;
      curSeg.data = data;
    } else if (curSeg.address + curSeg.data.size() == addr) {
      curSeg.data.append(data);
    } else {
      m_segments.append(curSeg);
      curSeg.address = addr;
      curSeg.data = data;
    }
  }

  if (!curSeg.data.isEmpty()) {
    m_segments.append(curSeg);
  }
  emit logMessage(QString("解析 Motorola S19 成功，共 %1 个段，总计 %2 字节").arg(m_segments.size()).arg(totalFirmwareBytes()));
  return !m_segments.isEmpty();
}

bool FlashService::startFlashing() {
  if (!m_uds) {
    emit logMessage("错误: UdsEngine 未配置");
    return false;
  }
  if (m_segments.isEmpty()) {
    emit logMessage("错误: 未加载固件文件");
    return false;
  }

  m_totalBytes = totalFirmwareBytes();
  m_transferredBytes = 0;
  m_segmentIndex = 0;
  m_segmentOffset = 0;
  m_blockSeq = 1;
  m_startTime = QDateTime::currentMSecsSinceEpoch();

  emit logMessage(QString("开始执行刷写流程，总大小: %1 字节...").arg(m_totalBytes));
  m_currentStep = FlashStep::EnterExtendedSession;
  proceedNextStep();
  return true;
}

void FlashService::abortFlashing() {
  if (isFlashing()) {
    finishFlashing(false, "用户终止刷写");
  }
}

bool FlashService::isFlashing() const {
  return m_currentStep != FlashStep::Idle &&
         m_currentStep != FlashStep::Completed &&
         m_currentStep != FlashStep::Failed;
}

void FlashService::proceedNextStep() {
  switch (m_currentStep) {
  case FlashStep::EnterExtendedSession:
    emit stepChanged(m_currentStep, "进入扩展诊断会话 (0x10 03)...");
    emit logMessage("[1/8] 发送 0x10 03 (ExtendedSession)");
    m_uds->diagnosticSessionControl(0x03);
    break;

  case FlashStep::SecurityAccessSeed:
    emit stepChanged(m_currentStep, "请求安全访问 Seed (0x27 01)...");
    emit logMessage("[2/8] 发送 0x27 01 (RequestSeed)");
    m_uds->securityAccessRequestSeed(m_cfg.securityLevel);
    break;

  case FlashStep::SecurityAccessKey:
    emit stepChanged(m_currentStep, "发送安全访问 Key (0x27 02)...");
    emit logMessage("[3/8] 发送 0x27 02 (SendKey)");
    m_uds->securityAccessSendKey(m_cfg.securityLevel + 1, m_cfg.securityKey.isEmpty() ? QByteArray::fromHex("00000000") : m_cfg.securityKey);
    break;

  case FlashStep::EnterProgrammingSession:
    emit stepChanged(m_currentStep, "进入编程会话 (0x10 02)...");
    emit logMessage("[4/8] 发送 0x10 02 (ProgrammingSession)");
    m_uds->diagnosticSessionControl(0x02);
    break;

  case FlashStep::EraseFlash:
    if (m_cfg.needErase) {
      emit stepChanged(m_currentStep, "执行 Flash 擦除例程 (0x31 01)...");
      emit logMessage("[5/8] 发送 0x31 01 (EraseFlashRoutine)");
      m_uds->routineControl(0x01, m_cfg.eraseRoutineId);
    } else {
      m_currentStep = FlashStep::RequestDownload;
      proceedNextStep();
    }
    break;

  case FlashStep::RequestDownload: {
    emit stepChanged(m_currentStep, "请求固件下载 (0x34)...");
    emit logMessage(QString("[6/8] 发送 0x34 (RequestDownload) 段 %1/%2").arg(m_segmentIndex + 1).arg(m_segments.size()));

    const FlashSegment &seg = m_segments[m_segmentIndex];
    quint32 addr = seg.address;
    quint32 size = seg.data.size();

    QByteArray param;
    param.append(static_cast<char>(0x00)); // 无加密无压缩
    param.append(static_cast<char>(0x44)); // 4字节地址，4字节大小
    param.append(static_cast<char>((addr >> 24) & 0xFF));
    param.append(static_cast<char>((addr >> 16) & 0xFF));
    param.append(static_cast<char>((addr >> 8) & 0xFF));
    param.append(static_cast<char>(addr & 0xFF));
    param.append(static_cast<char>((size >> 24) & 0xFF));
    param.append(static_cast<char>((size >> 16) & 0xFF));
    param.append(static_cast<char>((size >> 8) & 0xFF));
    param.append(static_cast<char>(size & 0xFF));

    m_uds->sendRequest(0x34, param);
    break;
  }

  case FlashStep::TransferData:
    sendTransferDataChunk();
    break;

  case FlashStep::RequestTransferExit:
    emit stepChanged(m_currentStep, "请求退出传输 (0x37)...");
    emit logMessage("[7/8] 发送 0x37 (RequestTransferExit)");
    m_uds->sendRequest(0x37);
    break;

  case FlashStep::VerifyChecksum:
    emit stepChanged(m_currentStep, "校验固件完整性 CRC (0x31 01)...");
    emit logMessage("[8/8] 发送 0x31 01 (CheckMemoryRoutine)");
    m_uds->routineControl(0x01, m_cfg.checkRoutineId);
    break;

  case FlashStep::EcuReset:
    if (m_cfg.needReset) {
      emit stepChanged(m_currentStep, "复位控制器引导程序 (0x11 01)...");
      emit logMessage("发送 0x11 01 (EcuReset)");
      m_uds->ecuReset(0x01);
    } else {
      finishFlashing(true, "刷写圆满成功！");
    }
    break;

  default:
    break;
  }
}

void FlashService::sendTransferDataChunk() {
  if (m_segmentIndex >= m_segments.size()) {
    m_currentStep = FlashStep::RequestTransferExit;
    proceedNextStep();
    return;
  }

  const FlashSegment &seg = m_segments[m_segmentIndex];
  int remaining = seg.data.size() - m_segmentOffset;

  if (remaining <= 0) {
    // 当前段传输完毕
    m_segmentIndex++;
    m_segmentOffset = 0;
    m_blockSeq = 1;
    if (m_segmentIndex < m_segments.size()) {
      m_currentStep = FlashStep::RequestDownload;
      proceedNextStep();
    } else {
      m_currentStep = FlashStep::RequestTransferExit;
      proceedNextStep();
    }
    return;
  }

  int chunk = qMin(m_cfg.blockSize, remaining);
  QByteArray chunkData = seg.data.mid(m_segmentOffset, chunk);

  QByteArray req;
  req.append(static_cast<char>(m_blockSeq));
  req.append(chunkData);

  m_uds->sendRequest(0x36, req);
}

void FlashService::onUdsResponseReceived(const UdsResponse &resp) {
  if (!resp.isPositive) {
    finishFlashing(false, QString("UDS 负响应: SID 0x%1, %2").arg(resp.sid, 2, 16, QLatin1Char('0')).arg(resp.nrcDescription));
    return;
  }

  switch (m_currentStep) {
  case FlashStep::EnterExtendedSession:
    m_currentStep = FlashStep::SecurityAccessSeed;
    proceedNextStep();
    break;

  case FlashStep::SecurityAccessSeed: {
    // 收到 Seed，执行密钥计算 (默认简单变换或固定 key)
    if (resp.payload.size() >= 2) {
      quint8 sub = static_cast<quint8>(resp.payload.at(0));
      if (sub == 0x00) { // 已解锁
        m_currentStep = FlashStep::EnterProgrammingSession;
        proceedNextStep();
        return;
      }
    }
    m_currentStep = FlashStep::SecurityAccessKey;
    proceedNextStep();
    break;
  }

  case FlashStep::SecurityAccessKey:
    m_currentStep = FlashStep::EnterProgrammingSession;
    proceedNextStep();
    break;

  case FlashStep::EnterProgrammingSession:
    m_currentStep = FlashStep::EraseFlash;
    proceedNextStep();
    break;

  case FlashStep::EraseFlash:
    m_currentStep = FlashStep::RequestDownload;
    proceedNextStep();
    break;

  case FlashStep::RequestDownload:
    m_currentStep = FlashStep::TransferData;
    sendTransferDataChunk();
    break;

  case FlashStep::TransferData: {
    const FlashSegment &seg = m_segments[m_segmentIndex];
    int chunk = qMin(m_cfg.blockSize, seg.data.size() - m_segmentOffset);
    m_segmentOffset += chunk;
    m_transferredBytes += chunk;
    m_blockSeq = (m_blockSeq + 1) & 0xFF;
    if (m_blockSeq == 0) m_blockSeq = 1;

    int percent = (m_totalBytes > 0) ? (m_transferredBytes * 100 / m_totalBytes) : 100;
    qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - m_startTime;
    double speedKBs = (elapsedMs > 0) ? (static_cast<double>(m_transferredBytes) / elapsedMs) : 0.0;

    emit progressChanged(percent, speedKBs, m_transferredBytes, m_totalBytes);

    // 继续下一块
    sendTransferDataChunk();
    break;
  }

  case FlashStep::RequestTransferExit:
    m_currentStep = FlashStep::VerifyChecksum;
    proceedNextStep();
    break;

  case FlashStep::VerifyChecksum:
    m_currentStep = FlashStep::EcuReset;
    proceedNextStep();
    break;

  case FlashStep::EcuReset:
    finishFlashing(true, "固件刷写与引导校验圆满成功！");
    break;

  default:
    break;
  }
}

void FlashService::onUdsTimeout(quint8 sid) {
  finishFlashing(false, QString("UDS 请求超时: SID 0x%1").arg(sid, 2, 16, QLatin1Char('0')));
}

void FlashService::onUdsFailed(quint8 sid, const QString &err) {
  finishFlashing(false, QString("UDS 请求失败: SID 0x%1, %2").arg(sid, 2, 16, QLatin1Char('0')).arg(err));
}

void FlashService::finishFlashing(bool ok, const QString &msg) {
  m_currentStep = ok ? FlashStep::Completed : FlashStep::Failed;
  emit logMessage(ok ? QString("✅ %1").arg(msg) : QString("❌ %1").arg(msg));
  emit flashFinished(ok, msg);
}
