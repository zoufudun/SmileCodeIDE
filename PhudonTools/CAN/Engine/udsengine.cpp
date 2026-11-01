#include "udsengine.h"

UdsEngine::UdsEngine(IsoTpEngine *isoTp, QObject *parent)
    : QObject(parent), m_isoTp(nullptr) {
  m_responseTimer = new QTimer(this);
  m_responseTimer->setSingleShot(true);
  connect(m_responseTimer, &QTimer::timeout, this, &UdsEngine::onResponseTimeout);

  setIsoTpEngine(isoTp);
}

UdsEngine::~UdsEngine() {
  m_responseTimer->stop();
}

void UdsEngine::setIsoTpEngine(IsoTpEngine *isoTp) {
  if (m_isoTp) {
    disconnect(m_isoTp, &IsoTpEngine::dataReceived, this, &UdsEngine::onIsoTpDataReceived);
    disconnect(m_isoTp, &IsoTpEngine::transmitCompleted, this, &UdsEngine::onIsoTpTransmitCompleted);
  }
  m_isoTp = isoTp;
  if (m_isoTp) {
    connect(m_isoTp, &IsoTpEngine::dataReceived, this, &UdsEngine::onIsoTpDataReceived);
    connect(m_isoTp, &IsoTpEngine::transmitCompleted, this, &UdsEngine::onIsoTpTransmitCompleted);
  }
}

bool UdsEngine::sendRequest(quint8 sid, const QByteArray &data) {
  if (!m_isoTp) {
    emit requestFailed(sid, "ISO-TP 引擎未绑定");
    return false;
  }

  QByteArray raw;
  raw.append(static_cast<char>(sid));
  raw.append(data);

  m_currentSid = sid;
  m_waitingResponse = true;

  bool ok = m_isoTp->sendData(raw);
  if (!ok) {
    m_waitingResponse = false;
    emit requestFailed(sid, "ISO-TP 发送请求失败");
    return false;
  }

  m_responseTimer->start(m_timeoutMs);
  return true;
}

bool UdsEngine::diagnosticSessionControl(quint8 sessionType) {
  QByteArray data;
  data.append(static_cast<char>(sessionType));
  return sendRequest(0x10, data);
}

bool UdsEngine::ecuReset(quint8 resetType) {
  QByteArray data;
  data.append(static_cast<char>(resetType));
  return sendRequest(0x11, data);
}

bool UdsEngine::securityAccessRequestSeed(quint8 level) {
  QByteArray data;
  data.append(static_cast<char>(level));
  return sendRequest(0x27, data);
}

bool UdsEngine::securityAccessSendKey(quint8 level, const QByteArray &key) {
  QByteArray data;
  data.append(static_cast<char>(level));
  data.append(key);
  return sendRequest(0x27, data);
}

bool UdsEngine::readDataByIdentifier(quint16 did) {
  QByteArray data;
  data.append(static_cast<char>((did >> 8) & 0xFF));
  data.append(static_cast<char>(did & 0xFF));
  return sendRequest(0x22, data);
}

bool UdsEngine::writeDataByIdentifier(quint16 did, const QByteArray &data) {
  QByteArray req;
  req.append(static_cast<char>((did >> 8) & 0xFF));
  req.append(static_cast<char>(did & 0xFF));
  req.append(data);
  return sendRequest(0x2E, req);
}

bool UdsEngine::readDtcInformation(quint8 subFunction, quint8 mask) {
  QByteArray data;
  data.append(static_cast<char>(subFunction));
  data.append(static_cast<char>(mask));
  return sendRequest(0x19, data);
}

bool UdsEngine::clearDiagnosticInformation(quint32 dtcGroup) {
  QByteArray data;
  data.append(static_cast<char>((dtcGroup >> 16) & 0xFF));
  data.append(static_cast<char>((dtcGroup >> 8) & 0xFF));
  data.append(static_cast<char>(dtcGroup & 0xFF));
  return sendRequest(0x14, data);
}

bool UdsEngine::testerPresent(bool suppressPosResponse) {
  QByteArray data;
  data.append(static_cast<char>(suppressPosResponse ? 0x80 : 0x00));
  return sendRequest(0x3E, data);
}

bool UdsEngine::routineControl(quint8 subFunction, quint16 routineId, const QByteArray &option) {
  QByteArray req;
  req.append(static_cast<char>(subFunction));
  req.append(static_cast<char>((routineId >> 8) & 0xFF));
  req.append(static_cast<char>(routineId & 0xFF));
  req.append(option);
  return sendRequest(0x31, req);
}

void UdsEngine::onIsoTpTransmitCompleted(bool ok) {
  if (!ok && m_waitingResponse) {
    m_responseTimer->stop();
    m_waitingResponse = false;
    emit requestFailed(m_currentSid, "物理发送失败");
  }
}

void UdsEngine::onIsoTpDataReceived(const QByteArray &data) {
  if (data.isEmpty()) return;

  quint8 firstByte = static_cast<quint8>(data.at(0));

  // 1. 处理负响应 (0x7F)
  if (firstByte == 0x7F && data.size() >= 3) {
    quint8 reqSid = static_cast<quint8>(data.at(1));
    quint8 nrc = static_cast<quint8>(data.at(2));

    // 0x78: 请求正确接收但响应挂起 (ResponsePending), 刷新定时器继续等待
    if (nrc == 0x78) {
      m_responseTimer->start(5000);
      return;
    }

    m_responseTimer->stop();
    m_waitingResponse = false;

    UdsResponse resp;
    resp.isPositive = false;
    resp.sid = reqSid;
    resp.responseSid = 0x7F;
    resp.nrc = nrc;
    resp.nrcDescription = nrcToString(nrc);
    resp.payload = data.mid(3);

    emit responseReceived(resp);
    return;
  }

  // 2. 处理正响应 (SID + 0x40)
  m_responseTimer->stop();
  m_waitingResponse = false;

  UdsResponse resp;
  resp.isPositive = true;
  resp.sid = m_currentSid;
  resp.responseSid = firstByte;
  resp.nrc = 0;
  resp.payload = data.mid(1);

  emit responseReceived(resp);
}

void UdsEngine::onResponseTimeout() {
  if (m_waitingResponse) {
    m_waitingResponse = false;
    emit requestTimeout(m_currentSid);
  }
}

QString UdsEngine::nrcToString(quint8 nrc) {
  switch (nrc) {
  case 0x10: return "0x10: 通用拒绝 (GeneralReject)";
  case 0x11: return "0x11: 服务不支持 (ServiceNotSupported)";
  case 0x12: return "0x12: 子功能不支持 (SubFunctionNotSupported)";
  case 0x13: return "0x13: 报文长度不正确或格式无效 (IncorrectMessageLengthOrInvalidFormat)";
  case 0x14: return "0x14: 响应太长 (ResponseTooLong)";
  case 0x21: return "0x21: 繁忙请重复请求 (BusyRepeatRequest)";
  case 0x22: return "0x22: 执行条件不满足 (ConditionsNotCorrect)";
  case 0x24: return "0x24: 请求时序错误 (RequestSequenceError)";
  case 0x25: return "0x25: 无子网络组件响应 (NoResponseFromSubnetComponent)";
  case 0x26: return "0x26: 故障阻碍操作 (FailurePreventsExecutionOfRequestedAction)";
  case 0x31: return "0x31: 请求超出范围 (RequestOutOfRange)";
  case 0x33: return "0x33: 安全访问拒绝 (SecurityAccessDenied)";
  case 0x35: return "0x35: 密钥无效 (InvalidKey)";
  case 0x36: return "0x36: 超出尝试次数 (ExceedNumberOfAttempts)";
  case 0x37: return "0x37: 需要延时 (RequiredTimeDelayNotExpired)";
  case 0x70: return "0x70: 上传下载未接受 (UploadDownloadNotAccepted)";
  case 0x71: return "0x71: 数据传输挂起 (TransferDataSuspended)";
  case 0x72: return "0x72: 编程/擦除Flash通用失败 (GeneralProgrammingFailure)";
  case 0x73: return "0x73: 传输块序号错误 (WrongBlockSequenceCounter)";
  case 0x78: return "0x78: 请求正确接收但响应挂起 (ResponsePending)";
  case 0x7E: return "0x7E: 当前会话不支持该子功能 (SubFunctionNotSupportedInActiveSession)";
  case 0x7F: return "0x7F: 当前会话不支持该服务 (ServiceNotSupportedInActiveSession)";
  default:   return QString("0x%1: 未知负响应码").arg(nrc, 2, 16, QLatin1Char('0')).toUpper();
  }
}
