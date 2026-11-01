# CAN工作台全新分层架构重构与功能增强验收报告

## 📌 重构目标与核心成就

根据设计要求与 CANPilot 分层架构，本次重构达成了以下核心目标：
1. **“界面不认识硬件，引擎不认识界面”**：实现了 **界面层**、**服务层**、**引擎层**、**适配层** 四层完全解耦，每一层均可单独测试与演进。
2. **零硬件离线可测**：实现了高性能 **纯软件仿真适配器 (`SimulationCanAdapter`)**，内置虚拟 ECU 动力总成、车速、电池动态报文生成及完整的 UDS 诊断状态机应答，**不插卡也能把全部界面和业务流程跑通**！
3. **Qt 官方内置插件全面打通**：通过 `QtSerialBusAdapter` 接入 Qt 官方生态，全面支持：
   - `peakcan` (PEAK PCAN 硬件卡)
   - `vectorcan` (Vector VN 系列总线卡)
   - `passthrucan` (SAE J2534 PassThru OBD 盒)
   - `virtualcan` (Qt 官方虚拟总线)
   - `systeccan` (SYS TEC 模块)
   - `tinycan` (MHS TinyCAN 适配器)
   - `socketcan` (Linux SocketCAN)
4. **国内主流硬件无缝兼容**：原有周立功 ZLG（`zlgcan`）、创芯科技（`ControlCAN`）、同星智能（`TSMaster`）全线兼容，原有参数配置、滤波配置、树形视图 100% 保留！
5. **四大全新专属业务面板**：
   - 📈 **信号曲线台 (`CanSignalPlotPanel`)**：基于 `QCustomPlot` 与 `DbcParserEngine`，实时绘制物理量变化动态波形。
   - 🩺 **诊断台 (`CanDiagnosticPanel`)**：标准 UDS 终端、DTC 故障码树形诊断、一键清码、DID 读写。
   - ⚡ **刷写台 (`CanFlashPanel`)**：Bootloader 固件刷写（Bin/Hex/S19 解析）、8 步状态机、进度条与实时速率监控。
   - 🤖 **AI 墩墩 / 智能总线助手 (`CanAiAssistant`)**：专家知识库驱动，智能解析 DTC 故障码、UDS 负响应 NRC 与总线物理层排障建议。

---

## 🏗️ 架构落地全貌

```
PhudonTools/
├── CAN/
│   ├── Adapter/                  # 适配层 (Hardware Adapter Layer)
│   │   ├── icanadapter.h         # 统一适配器抽象基类与通用帧结构 (CanRawFrame)
│   │   ├── canadapterfactory.h/cpp # 适配器工厂与注册表
│   │   ├── simulationcanadapter.h/cpp # 纯软件仿真适配器 (含虚拟 ECU 心跳与 UDS 应答)
│   │   └── qtserialbusadapter.h/cpp   # Qt 原生插件生态桥接 (PeakCAN/Vector/PassThru等)
│   ├── Engine/                   # 引擎层 (Engine Layer)
│   │   ├── isotpengine.h/cpp     # ISO 15765-2 网络传输层协议栈 (SF/FF/CF/FC流控组包)
│   │   ├── udsengine.h/cpp       # ISO 14229-1 UDS 诊断协议栈 (会话/DID/DTC/NRC状态机)
│   │   ├── dbcparserengine.h/cpp # DBC 协议与信号解析引擎 (Intel/Motorola/因子/物理值)
│   │   ├── recordingengine.h/cpp # ASC (CANoe) / CSV 高性能录制与快慢回放索引引擎
│   │   └── knowledgeengine.h/cpp # SAE J2012 DTC 故障库与 UDS 服务字典专家库
│   ├── Service/                  # 服务层 (Service Layer)
│   │   ├── flashservice.h/cpp    # 固件解析 (Bin/Hex/S19) 与 Bootloader 任务编排
│   │   ├── diagnosticservice.h/cpp # 诊断服务、0x3E TesterPresent 保活与 DTC 调度
│   │   └── busmonitorservice.h/cpp # 总线数据中继分发与负载率 (BusLoad/FPS) 统计
│   └── UI/                       # 界面层 (Presentation Layer)
│       ├── cansignalplotpanel.h/cpp # 信号动态曲线监视面板 (基于 QCustomPlot)
│       ├── candiagnosticpanel.h/cpp # UDS 诊断工作台面板
│       ├── canflashpanel.h/cpp      # 固件刷写台面板
│       └── canaiassistant.h/cpp     # AI 墩墩总线专家助手面板
├── caninterface.h / cpp          # 升级为适配层门面 (Facade)，原有业务无缝兼容
├── candevicedialog.h / cpp       # 设备管理升级：新增仿真、Qt 内置插件、同星智能等选项
└── cantool.h / cpp               # 主工作台容器：增加四大新面板专属按钮与视图联动
```

---

## 🔍 验证与测试结果

### 1. 编译验证
- **编译器**：MinGW 8.1.0 (64-bit)，Qt 5.15.2 (`serialbus` 原生启用)
- **编译命令**：`qmake PhudonTools.pro` && `mingw32-make -j8`
- **构建结果**：`PhudonTools.exe` 构建成功，所有动态库与配置文件正常拷贝到位。

### 2. 仿真与业务全流程验证
- **默认仿真接入**：系统启动后默认装配 `SimulationCanAdapter`。在没有连接任何物理 CAN 盒的情况下：
  - 点击工具栏 **“设备管理”**，类型下拉框可见 **“🌟 纯软件仿真 (免硬件全流程测试)”** 以及 **PEAK PCAN**、**Vector VN**、**J2534** 等全部品牌。
  - 打开设备并启动通道后，底层自动产生 0x100、0x280、0x320 等周期性仿真报文。
  - 打开 **📈 信号曲线**：勾选 `RPM`、`WaterTemp`、`Speed` 等信号，动态曲线实时平滑波动。
  - 打开 **🩺 诊断台**：可直接向虚拟 ECU 发送 0x10 会话切换、0x22 读取 VIN 码、0x19 读取 DTC，收到真实模拟的诊断应答，并支持一键呼叫 AI 墩墩出具排障报告。
  - 打开 **⚡ 刷写台**：点击“开始刷写”，离线自动走完扩展会话 -> 安全访问 -> 编程会话 -> 擦除例程 -> 数据分块下发 -> CRC校验 -> 控制器复位全流程，进度条与速率实时显示。
  - 打开 **🤖 AI 墩墩**：可交互式提问总线物理层故障、DTC 代码含义及 UDS 负响应原因分析。
