#include "diagnosticservice.h"

DiagnosticService::DiagnosticService(UdsEngine *uds, KnowledgeEngine *knowledge, QObject *parent)
    : QObject(parent), m_uds(nullptr), m_knowledge(knowledge) {
  m_testerPresentTimer = new QTimer(this);
  connect(m_testerPresentTimer, &QTimer::timeout, this, &DiagnosticService::onTesterPresentTimeout);

  setUdsEngine(uds);
}

DiagnosticService::~DiagnosticService() {
  m_testerPresentTimer->stop();
}

void DiagnosticService::setUdsEngine(UdsEngine *uds) {
  if (m_uds) {
    disconnect(m_uds, &UdsEngine::responseReceived, this, &DiagnosticService::onUdsResponse);
  }
  m_uds = uds;
  if (m_uds) {
    connect(m_uds, &UdsEngine::responseReceived, this, &DiagnosticService::onUdsResponse);
  }
}

bool DiagnosticService::setSession(quint8 sessionType) {
  if (!m_uds) return false;
  emit diagnosticMessage(QString("切换会话模式: 0x%1").arg(sessionType, 2, 16, QLatin1Char('0')));
  return m_uds->diagnosticSessionControl(sessionType);
}

void DiagnosticService::setTesterPresentEnabled(bool enabled, int intervalMs) {
  if (enabled) {
    m_testerPresentTimer->start(intervalMs > 500 ? intervalMs : 2000);
    emit diagnosticMessage("已开启 0x3E TesterPresent 会话保持");
  } else {
    m_testerPresentTimer->stop();
    emit diagnosticMessage("已关闭会话保持");
  }
}

void DiagnosticService::onTesterPresentTimeout() {
  if (m_uds) {
    m_uds->testerPresent(true); // 静默模式
  }
}

bool DiagnosticService::queryDtcList(quint8 mask) {
  if (!m_uds) return false;
  emit diagnosticMessage("发送 0x19 02 读取故障码列表...");
  return m_uds->readDtcInformation(0x02, mask);
}

bool DiagnosticService::clearAllDtc() {
  if (!m_uds) return false;
  emit diagnosticMessage("发送 0x14 FF FF FF 清除所有故障码...");
  return m_uds->clearDiagnosticInformation(0xFFFFFF);
}

bool DiagnosticService::readDid(quint16 did) {
  if (!m_uds) return false;
  m_lastRequestedDid = did;
  emit diagnosticMessage(QString("读取 DID 0x%1...").arg(did, 4, 16, QLatin1Char('0')).toUpper());
  return m_uds->readDataByIdentifier(did);
}

bool DiagnosticService::writeDid(quint16 did, const QByteArray &data) {
  if (!m_uds) return false;
  emit diagnosticMessage(QString("写入 DID 0x%1 [%2]...").arg(did, 4, 16, QLatin1Char('0')).arg(QString(data.toHex(' ').toUpper())));
  return m_uds->writeDataByIdentifier(did, data);
}

void DiagnosticService::onUdsResponse(const UdsResponse &resp) {
  if (!resp.isPositive) {
    emit diagnosticMessage(QString("❌ 收到负响应: SID 0x%1, %2").arg(resp.sid, 2, 16, QLatin1Char('0')).arg(resp.nrcDescription));
    return;
  }

  // 1. 处理 0x19 02 读 DTC 响应 (0x59 02)
  if (resp.sid == 0x19 && resp.payload.size() >= 2) {
    quint8 sub = static_cast<quint8>(resp.payload.at(0));
    if (sub == 0x02) {
      quint8 statusAvailabilityMask = static_cast<quint8>(resp.payload.at(1));
      Q_UNUSED(statusAvailabilityMask);

      QList<DiagnosticDtcItem> list;
      int offset = 2;
      while (offset + 4 <= resp.payload.size()) {
        quint8 b1 = static_cast<quint8>(resp.payload.at(offset));
        quint8 b2 = static_cast<quint8>(resp.payload.at(offset + 1));
        quint8 b3 = static_cast<quint8>(resp.payload.at(offset + 2));
        quint8 status = static_cast<quint8>(resp.payload.at(offset + 3));

        QString dtcStr = KnowledgeEngine::formatDtcCode(b1, b2, b3);
        DiagnosticDtcItem item;
        item.code = dtcStr;
        item.statusMask = status;

        if (m_knowledge) {
          DtcKnowledgeItem k = m_knowledge->dtcDetails(dtcStr);
          item.category = k.category;
          item.title = k.title;
          item.description = k.description;
        }

        list.append(item);
        offset += 4;
      }

      emit dtcListUpdated(list);
      emit diagnosticMessage(QString("✅ 读取完成，共发现 %1 个故障码").arg(list.size()));
      return;
    }
  }

  // 2. 处理 0x14 清除 DTC 响应 (0x54)
  if (resp.sid == 0x14) {
    emit diagnosticMessage("✅ 故障码清除成功！");
    queryDtcList(); // 自动刷新列表
    return;
  }

  // 3. 处理 0x22 读 DID 响应 (0x62)
  if (resp.sid == 0x22 && resp.payload.size() >= 2) {
    quint16 did = (static_cast<quint8>(resp.payload.at(0)) << 8) | static_cast<quint8>(resp.payload.at(1));
    QByteArray val = resp.payload.mid(2);
    emit didDataReceived(did, val, val.toHex(' ').toUpper());
    emit diagnosticMessage(QString("✅ DID 0x%1 读取成功: %2").arg(did, 4, 16, QLatin1Char('0')).arg(QString(val.toHex(' ').toUpper())));
    return;
  }

  emit diagnosticMessage(QString("✅ 服务 0x%1 正响应成功").arg(resp.sid, 2, 16, QLatin1Char('0')));
}
