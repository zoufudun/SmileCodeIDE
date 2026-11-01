#include "simulationcanadapter.h"
#include <QtMath>
#include <QDateTime>

SimulationCanAdapter::SimulationCanAdapter(QObject *parent)
    : ICanAdapter(parent) {
  m_simTimer = new QTimer(this);
  connect(m_simTimer, &QTimer::timeout, this, &SimulationCanAdapter::onSimulationTick);
}

SimulationCanAdapter::~SimulationCanAdapter() {
  closeDevice();
}

bool SimulationCanAdapter::openDevice(const CanDeviceParams &params) {
  Q_UNUSED(params);
  m_isOpen = true;
  emit deviceOpened();
  return true;
}

void SimulationCanAdapter::closeDevice() {
  if (!m_isOpen) return;
  m_simTimer->stop();
  for (auto it = m_channelRunning.begin(); it != m_channelRunning.end(); ++it) {
    if (it.value()) {
      *it = false;
      emit channelStopped(it.key());
    }
  }
  m_isOpen = false;
  emit deviceClosed();
}

bool SimulationCanAdapter::isDeviceOpen() const {
  return m_isOpen;
}

bool SimulationCanAdapter::startChannel(int channel, const CanChannelParams &cfg) {
  if (!m_isOpen) return false;
  m_configs[channel] = cfg;
  m_channelRunning[channel] = true;
  m_stats[channel] = CanBusStatistics();

  if (m_ecuSimulation && !m_simTimer->isActive()) {
    m_simTimer->start(50); // 20Hz 产生仿真报文
  }

  emit channelStarted(channel);
  return true;
}

bool SimulationCanAdapter::stopChannel(int channel) {
  if (m_channelRunning.value(channel, false)) {
    m_channelRunning[channel] = false;
    emit channelStopped(channel);
  }

  bool anyRunning = false;
  for (bool r : m_channelRunning) {
    if (r) { anyRunning = true; break; }
  }
  if (!anyRunning) {
    m_simTimer->stop();
  }
  return true;
}

bool SimulationCanAdapter::isChannelRunning(int channel) const {
  return m_isOpen && m_channelRunning.value(channel, false);
}

CanChannelParams SimulationCanAdapter::channelConfig(int channel) const {
  return m_configs.value(channel, CanChannelParams());
}

bool SimulationCanAdapter::sendFrame(int channel, const CanRawFrame &frame) {
  if (!isChannelRunning(channel)) return false;

  CanRawFrame tx = frame;
  tx.channel = channel;
  if (tx.timestamp == 0) {
    tx.timestamp = QDateTime::currentMSecsSinceEpoch();
  }

  // 更新发送统计
  auto &stat = m_stats[channel];
  stat.txFrames++;
  stat.txBytes += tx.data.size();

  emit frameSent(tx);

  // 1. 如果是自发自收 / 回环
  if (tx.transmitType == 2 || tx.transmitType == 3 || m_configs[channel].mode == CanWorkMode::Loopback) {
    CanRawFrame loopFrame = tx;
    stat.rxFrames++;
    stat.rxBytes += loopFrame.data.size();
    emit frameReceived(loopFrame);
  }

  // 2. 如果开启了虚拟 ECU 模拟且收到目标诊断报文，进行 UDS 响应模拟
  if (m_ecuSimulation) {
    processUdsSimulation(channel, tx);
  }

  return true;
}

CanBusStatistics SimulationCanAdapter::statistics(int channel) const {
  CanBusStatistics s = m_stats.value(channel, CanBusStatistics());
  // 模拟总线负载率：根据最近帧频计算
  s.busLoad = qBound(0.0, (s.rxFrames + s.txFrames) % 25 + 5.5, 95.0);
  return s;
}

bool SimulationCanAdapter::getDeviceInfo(QString *hwVer, QString *fwVer, QString *drVer,
                                        QString *libVer, int *canNum, QString *serial,
                                        QString *typeStr) const {
  if (hwVer) *hwVer = "Sim-V2.0";
  if (fwVer) *fwVer = "Sim-FW-1.0.0";
  if (drVer) *drVer = "VirtualDriver-64bit";
  if (libVer) *libVer = "CANPilot-Engine-1.0";
  if (canNum) *canNum = 2;
  if (serial) *serial = "SIM-9999-8888-7777";
  if (typeStr) *typeStr = "Pure Software Virtual CAN";
  return true;
}

void SimulationCanAdapter::setEcuSimulationEnabled(bool enabled) {
  m_ecuSimulation = enabled;
  if (!enabled) {
    m_simTimer->stop();
  } else if (m_isOpen) {
    m_simTimer->start(50);
  }
}

void SimulationCanAdapter::onSimulationTick() {
  if (!m_isOpen) return;

  m_simAngle += 0.08;
  if (m_simAngle > 2 * M_PI) m_simAngle -= 2 * M_PI;
  m_simStep++;

  qint64 now = QDateTime::currentMSecsSinceEpoch();

  // 遍历所有正在运行的通道，发射仿真报文
  for (auto it = m_channelRunning.begin(); it != m_channelRunning.end(); ++it) {
    int ch = it.key();
    if (!it.value()) continue;

    auto &stat = m_stats[ch];

    // 1. 发动机与动力总成报文 (0x100) 周期 50ms
    {
      CanRawFrame f;
      f.id = 0x100;
      f.channel = ch;
      f.extended = false;
      f.fd = m_configs[ch].isFd;
      f.timestamp = now;

      // 信号模拟：RPM 800~6000 正弦波，水温 80~105 摄氏度，节气门 0~100%
      quint16 rpm = static_cast<quint16>(3000 + 2000 * qSin(m_simAngle));
      quint8 waterTemp = static_cast<quint8>(90 + 10 * qCos(m_simAngle * 0.5));
      quint8 throttle = static_cast<quint8>(50 + 40 * qSin(m_simAngle));
      quint8 statusByte = (m_simStep % 10 == 0) ? 0x01 : 0x00;

      f.data.resize(8);
      f.data[0] = static_cast<char>((rpm >> 8) & 0xFF);
      f.data[1] = static_cast<char>(rpm & 0xFF);
      f.data[2] = static_cast<char>(waterTemp);
      f.data[3] = static_cast<char>(throttle);
      f.data[4] = static_cast<char>(statusByte);
      f.data[5] = static_cast<char>(0xAA);
      f.data[6] = static_cast<char>((m_simStep) & 0xFF); // Alive counter
      f.data[7] = static_cast<char>(0x55);               // Checksum

      stat.rxFrames++;
      stat.rxBytes += f.data.size();
      emit frameReceived(f);
    }

    // 2. 车身与速度报文 (0x280) 周期 100ms
    if (m_simStep % 2 == 0) {
      CanRawFrame f;
      f.id = 0x280;
      f.channel = ch;
      f.extended = false;
      f.fd = m_configs[ch].isFd;
      f.timestamp = now;

      // 车速 Speed (km/h) 0~180
      quint16 speed = static_cast<quint16>(80 + 50 * qSin(m_simAngle * 0.7));
      quint8 gear = static_cast<quint8>((m_simStep / 40) % 6 + 1); // 1~6挡

      f.data.resize(8);
      f.data[0] = static_cast<char>((speed >> 8) & 0xFF);
      f.data[1] = static_cast<char>(speed & 0xFF);
      f.data[2] = static_cast<char>(gear);
      f.data[3] = static_cast<char>(0x00); // 转向角
      f.data[4] = static_cast<char>(0x12);
      f.data[5] = static_cast<char>(0x34);
      f.data[6] = static_cast<char>((m_simStep >> 1) & 0xFF);
      f.data[7] = static_cast<char>(0x00);

      stat.rxFrames++;
      stat.rxBytes += f.data.size();
      emit frameReceived(f);
    }

    // 3. 电池系统报文 (0x320) 周期 200ms
    if (m_simStep % 4 == 0) {
      CanRawFrame f;
      f.id = 0x320;
      f.channel = ch;
      f.extended = false;
      f.fd = m_configs[ch].isFd;
      f.timestamp = now;

      // 电压 380V ~ 400V, 电流 -50A ~ +50A
      quint16 voltage = static_cast<quint16>(3900 + 50 * qSin(m_simAngle * 0.3)); // 0.1V/bit
      qint16 current = static_cast<qint16>(200 * qCos(m_simAngle * 0.5));        // 0.1A/bit

      f.data.resize(8);
      f.data[0] = static_cast<char>((voltage >> 8) & 0xFF);
      f.data[1] = static_cast<char>(voltage & 0xFF);
      f.data[2] = static_cast<char>((current >> 8) & 0xFF);
      f.data[3] = static_cast<char>(current & 0xFF);
      f.data[4] = static_cast<char>(85); // SOC 85%
      f.data[5] = static_cast<char>(32); // 电池温度 32℃
      f.data[6] = static_cast<char>(0x00);
      f.data[7] = static_cast<char>(0x00);

      stat.rxFrames++;
      stat.rxBytes += f.data.size();
      emit frameReceived(f);
    }
  }
}

void SimulationCanAdapter::processUdsSimulation(int channel, const CanRawFrame &frame) {
  // 识别 0x7DF (广播诊断) 或 0x7E0 (物理寻址)
  if (frame.id != 0x7DF && frame.id != 0x7E0) return;
  if (frame.data.size() < 2) return;

  quint8 dlc = static_cast<quint8>(frame.data.at(0));
  if (dlc == 0 || frame.data.size() < dlc + 1) return;

  quint8 sid = static_cast<quint8>(frame.data.at(1));
  CanRawFrame resp;
  resp.id = 0x7E8; // 虚拟 ECU 诊断应答 ID
  resp.channel = channel;
  resp.extended = false;
  resp.fd = frame.fd;
  resp.timestamp = QDateTime::currentMSecsSinceEpoch() + 10;

  switch (sid) {
  case 0x10: { // DiagnosticSessionControl (会话控制)
    quint8 sub = (frame.data.size() > 2) ? static_cast<quint8>(frame.data.at(2)) : 0x01;
    resp.data = QByteArray::fromHex("065000003201F4AA");
    resp.data[2] = static_cast<char>(sub);
    break;
  }
  case 0x27: { // SecurityAccess (安全访问)
    quint8 sub = (frame.data.size() > 2) ? static_cast<quint8>(frame.data.at(2)) : 0x01;
    if (sub == 0x01) { // 请求种子
      resp.data = QByteArray::fromHex("06670112345678AA");
      m_expectedSeed = 0x12345678;
    } else if (sub == 0x02) { // 发送密钥
      m_securityUnlocked = true;
      resp.data = QByteArray::fromHex("0267020000000000");
    }
    break;
  }
  case 0x22: { // ReadDataByIdentifier
    if (frame.data.size() >= 4) {
      quint16 did = (static_cast<quint8>(frame.data.at(2)) << 8) | static_cast<quint8>(frame.data.at(3));
      if (did == 0xF190) { // VIN 码
        resp.data = QByteArray::fromHex("0762F19056494E31"); // VIN1...
      } else if (did == 0xF188) { // 软件版本号
        resp.data = QByteArray::fromHex("0762F18853573031"); // SW01
      } else {
        resp.data = QByteArray::fromHex("0462000012340000");
        resp.data[2] = frame.data.at(2);
        resp.data[3] = frame.data.at(3);
      }
    }
    break;
  }
  case 0x19: { // ReadDTCInformation
    // 返回 2 个模拟故障码: P0100 (空气流量计故障) 与 U0100 (与ECM通信丢失)
    resp.data = QByteArray::fromHex("0759020901002F00");
    break;
  }
  case 0x14: { // ClearDiagnosticInformation (清除故障码)
    resp.data = QByteArray::fromHex("0154000000000000");
    break;
  }
  case 0x3E: { // TesterPresent (会话保持)
    resp.data = QByteArray::fromHex("027E000000000000");
    break;
  }
  case 0x31: { // RoutineControl (例程控制, 如擦除Flash)
    resp.data = QByteArray::fromHex("047101FF00000000");
    if (frame.data.size() > 4) {
      resp.data[2] = frame.data.at(2);
      resp.data[3] = frame.data.at(3);
      resp.data[4] = frame.data.at(4);
    }
    break;
  }
  case 0x34: { // RequestDownload (请求下载)
    resp.data = QByteArray::fromHex("0474200400000000"); // 允许最大单包 1024 字节
    break;
  }
  case 0x36: { // TransferData (传输数据)
    quint8 seq = (frame.data.size() > 2) ? static_cast<quint8>(frame.data.at(2)) : 0x01;
    resp.data = QByteArray::fromHex("0276000000000000");
    resp.data[2] = static_cast<char>(seq);
    break;
  }
  case 0x37: { // RequestTransferExit (退出传输)
    resp.data = QByteArray::fromHex("0177000000000000");
    break;
  }
  default:
    // 默认正响应
    resp.data = QByteArray::fromHex("0200000000000000");
    resp.data[1] = static_cast<char>(sid + 0x40);
    break;
  }

  auto &stat = m_stats[channel];
  stat.rxFrames++;
  stat.rxBytes += resp.data.size();
  emit frameReceived(resp);
}
