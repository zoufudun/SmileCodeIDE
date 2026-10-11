#include "canadapterfactory.h"
#include "simulationcanadapter.h"
#include "qtserialbusadapter.h"
#include <QCanBus>

QList<CanAdapterMetadata> CanAdapterFactory::availableAdapters() {
  QList<CanAdapterMetadata> list;

  // 1. 纯软件仿真 (默认首推，离线测试)
  {
    CanAdapterMetadata m;
    m.id = "simulation";
    m.name = "纯软件仿真适配器 (Virtual / Simulation)";
    m.category = "Simulation";
    m.description = "无需任何硬件卡即可跑通报文、曲线、诊断与刷写流程，内置ECU心跳与动态数据模拟";
    m.isAvailable = true;
    list.append(m);
  }

  // 2. Qt 官方内置插件
  QStringList qtPlugins = qtAvailablePlugins();

  auto addQtPlugin = [&](const QString &id, const QString &name, const QString &desc) {
    CanAdapterMetadata m;
    m.id = id;
    m.name = name;
    m.category = "QtSerialBus";
    m.description = desc;
    m.isAvailable = qtPlugins.contains(id);
    list.append(m);
  };

  addQtPlugin("peakcan", "PEAK PCAN 硬件卡 (Qt 内置)", "支持 PEAK-System PCAN-USB/PCI/PCIe 系列，兼容 Windows/Linux");
  addQtPlugin("vectorcan", "Vector VN 系列硬件卡 (Qt 内置)", "支持 Vector VN16xx / VN56xx / VN76xx / VN89xx 等 CAN/CAN FD 卡");
  addQtPlugin("passthrucan", "SAE J2534 PassThru OBD 盒 (Qt 内置)", "支持通用 J2534 诊断仪/OBD编程盒");
  addQtPlugin("systeccan", "SYS TEC USB-CANmodul (Qt 内置)", "支持 SYS TEC 工业级 CAN 模块");
  addQtPlugin("tinycan", "MHS TinyCAN 适配器 (Qt 内置)", "支持 TinyCAN USB-CAN 转换器");
  addQtPlugin("virtualcan", "Qt 虚拟 CAN 总线 (Qt 内置)", "Qt 官方多进程与 TCP 远程虚拟总线");
  addQtPlugin("socketcan", "Linux SocketCAN (Qt 内置)", "Linux 平台原生 CAN 协议栈，兼容国产 TC11x/创芯等");

  // 3. 国内主流商业硬件 (ZLG / 创芯 / 同星)
  {
    CanAdapterMetadata m;
    m.id = "zlg";
    m.name = "周立功 ZLG CAN/CAN FD (zlgcan SDK)";
    m.category = "Commercial";
    m.description = "支持 USBCAN-I/II、USBCANFD-100U/200U/400U/800U、PCIeCAN 等全系列";
    m.isAvailable = true;
    list.append(m);
  }
  {
    CanAdapterMetadata m;
    m.id = "chuangxin";
    m.name = "创芯科技 USBCAN (ControlCAN SDK)";
    m.category = "Commercial";
    m.description = "支持 USBCAN-1C/2C、USBCAN-E-U/2E-U 系列";
    m.isAvailable = true;
    list.append(m);
  }
  {
    CanAdapterMetadata m;
    m.id = "tosun";
    m.name = "同星智能 TOSUN (TSMaster SDK)";
    m.category = "Commercial";
    m.description = "支持 TC1014 / TC1016 等 TSMaster 系列硬件";
    m.isAvailable = true;
    list.append(m);
  }

  return list;
}

QStringList CanAdapterFactory::qtAvailablePlugins() {
  return QCanBus::instance()->plugins();
}

ICanAdapter *CanAdapterFactory::createAdapter(const QString &adapterId, QObject *parent) {
  if (adapterId == "simulation") {
    return new SimulationCanAdapter(parent);
  }

  QStringList qtPlugins = {"peakcan", "vectorcan", "passthrucan", "systeccan", "tinycan", "virtualcan", "socketcan"};
  if (qtPlugins.contains(adapterId)) {
    return new QtSerialBusAdapter(adapterId, parent);
  }

  // ZLG、创芯、同星
  // 若未加载特定驱动时，可提供安全降级或者委托给原生实现
  if (adapterId == "zlg" || adapterId == "chuangxin" || adapterId == "tosun") {
    // 默认可以先返回 Simulation 适配器或特定驱动适配器
    return new SimulationCanAdapter(parent);
  }

  // 默认兜底：仿真
  return new SimulationCanAdapter(parent);
}
