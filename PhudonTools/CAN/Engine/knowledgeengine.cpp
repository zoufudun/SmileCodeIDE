#include "knowledgeengine.h"

KnowledgeEngine::KnowledgeEngine(QObject *parent) : QObject(parent) {
  initDatabase();
}

void KnowledgeEngine::initDatabase() {
  // 1. DTC 基础知识库
  auto addDtc = [&](const QString &code, const QString &cat, const QString &title,
                    const QString &desc, const QString &cause, const QString &sug) {
    DtcKnowledgeItem item;
    item.code = code;
    item.category = cat;
    item.title = title;
    item.description = desc;
    item.probableCause = cause;
    item.suggestion = sug;
    m_dtcDb[code] = item;
  };

  addDtc("P0100", "动力总成(Powertrain)", "空气流量计 (MAF) 电路故障",
         "MAF 传感器输出信号电压超出正常标定范围。",
         "传感器接插件松动、进气管道漏气、线束断路或传感器本身损坏。",
         "1. 检查空气格及进气管密封；2. 用示波器测量 MAF 信号引脚波形与地电位；3. 重新校准或更换 MAF 传感器。");

  addDtc("P0115", "动力总成(Powertrain)", "发动机冷却液温度传感器 (ECT) 电路故障",
         "水温传感器读数开路或对地短路。",
         "水温传感器阻值漂移、线束磨损短路。",
         "测量传感器常温与升温阻值曲线是否符合 NTC 特性，检查 5V 基准供电与回地线。");

  addDtc("U0100", "网络通信(Network)", "与发动机控制模块 (ECM/PCM) 失去通信",
         "网关或其它 ECU 在超时周期内未收到来自 ECM 的周期性 CAN 心跳帧。",
         "CAN 总线物理断路、终端电阻缺失导致信号反射、ECM 供电继电器掉电、总线被持续拉低。",
         "1. 测量 CAN_H 与 CAN_L 间断电电阻是否为 60Ω；2. 测量 CAN_H 与 CAN_L 对地共模电压是否为 2.5V 左右；3. 检查 ECM 保险丝与唤醒引脚。");

  addDtc("U0121", "网络通信(Network)", "与防抱死制动系统 (ABS) 失去通信",
         "底盘 CAN 总线上无法收到 ABS/ESP 控制器的周期报文。",
         "ABS 模块供电异常、轮速线短路造成控制器看门狗复位、CAN 通信线接触不良。",
         "检查 ABS 控制器插头退针及电源地线；用 CAN 抓包分析 0x280 等底盘报文是否有错误帧。");

  addDtc("U0140", "网络通信(Network)", "与车身控制模块 (BCM) 失去通信",
         "车身总线节点未能接收到 BCM 发送的网关同步报文。",
         "BCM 处于休眠未被 KL15/CAN 报文唤醒、BCM 内部晶振停振。",
         "测试 CAN 唤醒电平是否满足 ISO 11898-2 规范；检查 BCM 供电电压是否过低。");

  // 2. UDS 标准服务知识库
  m_udsDb[0x10] = {0x10, "DiagnosticSessionControl (会话控制)", "01:默认会话, 02:编程会话, 03:扩展会话", "用于在 ECU 不同的运行状态之间切换权限。"};
  m_udsDb[0x11] = {0x11, "EcuReset (控制器复位)", "01:硬复位, 02:休眠唤醒, 03:软复位", "用于完成刷写后重新引导应用程序。"};
  m_udsDb[0x14] = {0x14, "ClearDiagnosticInformation (清除故障码)", "FF FF FF:清除所有群组", "重置 ECU 存储器中的历史与当前故障记录。"};
  m_udsDb[0x19] = {0x19, "ReadDTCInformation (读故障码)", "01:读掩码计数, 02:读掩码故障码, 04:读冻结帧", "检索系统故障码及故障发生时的环境快照数据。"};
  m_udsDb[0x22] = {0x22, "ReadDataByIdentifier (按DID读取数据)", "DID 范围: 0x0000 ~ 0xFFFF", "读取 ECU 内部标定参数、传感器当前物理值、固件版本号。"};
  m_udsDb[0x27] = {0x27, "SecurityAccess (安全访问解锁)", "奇数:请求Seed, 偶数:发送Key", "用于编程、标定等敏感服务前验证上位机合法身份。"};
  m_udsDb[0x2E] = {0x2E, "WriteDataByIdentifier (按DID写入数据)", "DID 范围: 0x0000 ~ 0xFFFF", "向 ECU 写入配置参数、VIN 码、安装日期。"};
  m_udsDb[0x31] = {0x31, "RoutineControl (例程控制)", "01:启动, 02:停止, 03:获取结果", "执行擦除 Flash、计算 Checksum、自检等长耗时硬件任务。"};
  m_udsDb[0x34] = {0x34, "RequestDownload (请求下载)", "指定内存地址与数据块长度", "刷写固件第一步，通知 ECU 准备接收固件数据。"};
  m_udsDb[0x36] = {0x36, "TransferData (传输数据块)", "带连续块序号 (1~255)", "分块下发固件二进制内容。"};
  m_udsDb[0x37] = {0x37, "RequestTransferExit (退出传输)", "无", "固件块下发完成后结束传输，准备例程校验。"};
  m_udsDb[0x3E] = {0x3E, "TesterPresent (诊断仪会话保持)", "00:回正响应, 80:静默抑制正响应", "周期性发送(通常2秒)，防止会话自动退回默认模式。"};
}

bool KnowledgeEngine::hasDtc(const QString &dtcCode) const {
  QString clean = dtcCode.trimmed().toUpper();
  if (clean.contains("-")) clean = clean.split("-").first();
  return m_dtcDb.contains(clean);
}

DtcKnowledgeItem KnowledgeEngine::dtcDetails(const QString &dtcCode) const {
  QString clean = dtcCode.trimmed().toUpper();
  if (clean.contains("-")) clean = clean.split("-").first();
  return m_dtcDb.value(clean, DtcKnowledgeItem{dtcCode, "未知系统", "未收录的故障码", "系统无此故障码详细描述", "请参考主机厂特定协议规范", "建议核对原厂服务手册。"});
}

QString KnowledgeEngine::formatDtcCode(quint8 b1, quint8 b2, quint8 b3) {
  char prefix = 'P';
  quint8 highNibble = (b1 >> 6) & 0x03;
  if (highNibble == 0) prefix = 'P';
  else if (highNibble == 1) prefix = 'C';
  else if (highNibble == 2) prefix = 'B';
  else if (highNibble == 3) prefix = 'U';

  quint16 num = ((b1 & 0x3F) << 8) | b2;
  QString code = QString("%1%2").arg(prefix).arg(num, 4, 16, QLatin1Char('0')).toUpper();
  if (b3 != 0) {
    code += QString("-%1").arg(b3, 2, 16, QLatin1Char('0')).toUpper();
  }
  return code;
}

UdsServiceDoc KnowledgeEngine::udsService(quint8 sid) const {
  return m_udsDb.value(sid, UdsServiceDoc{sid, QString("自定义或未收录服务 0x%1").arg(sid, 2, 16, QLatin1Char('0')).toUpper(), "", ""});
}

QString KnowledgeEngine::generateTroubleshootingReport(const QStringList &dtcList) const {
  QString report = "### 🛠️ AI 墩墩总线专家智能诊断分析报告\n\n";
  if (dtcList.isEmpty()) {
    report += "✅ **总线状态良好**：当前未检测到活动故障码 (DTC)，总线通信正常。\n";
    return report;
  }

  report += QString("检测到 **%1** 个故障码，排查建议如下：\n\n").arg(dtcList.size());
  for (const QString &c : dtcList) {
    DtcKnowledgeItem item = dtcDetails(c);
    report += QString("#### 🔴 故障码: `%1` - %2\n").arg(item.code, item.title);
    report += QString("- **所属分类**：%1\n").arg(item.category);
    report += QString("- **现象描述**：%1\n").arg(item.description);
    report += QString("- **可能诱因**：%1\n").arg(item.probableCause);
    report += QString("- **建议排查措施**：%1\n\n").arg(item.suggestion);
  }

  return report;
}
