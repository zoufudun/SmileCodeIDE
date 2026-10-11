# CAN工作台全面重构设计方案（CANPilot 四层架构）

## 架构愿景与设计原则

> **核心原则：界面不认识硬件，引擎不认识界面 —— 每一层都能单独测试与独立演进。**
> **仿真适配器不插卡也能把全部界面和流程跑通 —— 所有离线功能一块硬件都不需要。**

---

## 总体分层架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             1. 界面层 (Presentation Layer)                    │
│   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌───────────┐ ┌──────────┐ │
│   │ 报文跟踪     │ │ 信号曲线     │ │ 刷写台       │ │ 诊断台    │ │ AI 智能  │ │
│   │ (TracePanel) │ │ (SignalPlot) │ │ (FlashPanel) │ │ (DiagView)│ │ (Copilot)│ │
│   └───────┬──────┘ └──────┬───────┘ └──────┬───────┘ └─────┬─────┘ └────┬─────┘ │
└───────────┼───────────────┼────────────────┼───────────────┼────────────┼───┘
            ▼               ▼                ▼               ▼            ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                             2. 服务层 (Service Layer)                         │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │ FlashService: 刷写工程配置、固件解析、安全解锁、任务编排、断点续传状态机  │  │
│  ├───────────────────────────────────────────────────────────────────────┤  │
│  │ DiagnosticService: UDS会话管理、22/2E数据读写、DTC故障码查询与清除流程 │  │
│  ├───────────────────────────────────────────────────────────────────────┤  │
│  │ TraceService / BusMonitorService: 报文分发路由、总线负载率统计与过滤调度  │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                             3. 引擎层 (Engine Layer)                          │
│  ┌────────────────────┐ ┌────────────────────┐ ┌─────────────────────────┐  │
│  │ IsoTpEngine        │ │ DbcParserEngine    │ │ RecordingEngine         │  │
│  │ (ISO 15765-2 传输: │ │ (DBC协议解析:      │ │ (录制回放与索引:        │  │
│  │  SF/FF/CF/FC组包)  │ │  物理值/信号/因子) │ │  ASC/BLF/CSV/时间索引)  │  │
│  ├────────────────────┤ └────────────────────┘ ├─────────────────────────┤  │
│  │ UdsEngine          │                        │ KnowledgeEngine         │  │
│  │ (ISO 14229-1 诊断: │                        │ (DTC故障库/标准UDS字典/ │  │
│  │  SID/NRC/会话保持) │                        │  总线排障专家库)        │  │
│  └────────────────────┘                        └─────────────────────────┘  │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                             4. 适配层 (Hardware Adapter Layer)                │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │ ICanAdapter (统一适配接口: openDevice, startChannel, sendFrame, 统计等) │  │
│  │ CanAdapterFactory (适配器注册与动态插件工厂)                          │  │
│  └───────────────────────────────────┬───────────────────────────────────┘  │
│         ┌──────────────┬─────────────┴────────────┬──────────────┐          │
│         ▼              ▼                          ▼              ▼          │
│  ┌──────────────┐┌──────────────┐        ┌──────────────────┐┌───────────┐  │
│  │ 仿真适配器   ││ ZLG / 创芯   │        │ QtSerialBus 桥接 ││ 同星智能  │  │
│  │ Simulation   ││ ZlgAdapter   │        │ (Qt 官方 CAN 插件)││ Tosun     │  │
│  │ (纯软件回环/ ││ ChuangXin    │        ├──────────────────┤│ Adapter   │  │
│  │ 虚拟ECU心跳/ ││ Adapter      │        │ • peakcan (PCAN) ││ (TSMaster │  │
│  │ 故障注入)    ││              │        │ • vectorcan (VN) ││  SDK)     │  │
│  │              ││              │        │ • passthrucan    ││           │  │
│  │              ││              │        │ • systeccan      ││           │  │
│  │              ││              │        │ • tinycan        ││           │  │
│  │              ││              │        │ • virtualcan     ││           │  │
│  │              ││              │        │ • socketcan      ││           │  │
│  └──────────────┘└──────────────┘        └──────────────────┘└───────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 各层详细设计规格

### 4. 适配层（Hardware Adapter Layer）
适配层对下屏蔽各品牌硬件 SDK 的差异，对上提供统一、标准化的异步接口与事件通知。

#### 4.1 统一核心抽象接口 `ICanAdapter`
- **通道/设备生命周期管理**：
  - `virtual bool openDevice(const CanDeviceParams &params) = 0;`
  - `virtual void closeDevice() = 0;`
  - `virtual bool isDeviceOpen() const = 0;`
  - `virtual bool startChannel(int channel, const CanChannelConfig &cfg) = 0;`
  - `virtual bool stopChannel(int channel) = 0;`
  - `virtual bool isChannelRunning(int channel) const = 0;`
- **收发数据接口**：
  - `virtual bool sendFrame(int channel, const CanRawFrame &frame) = 0;`
  - `virtual bool sendFrames(int channel, const QList<CanRawFrame> &frames, int *sentCount = nullptr);`
- **配置与滤波**：
  - 波特率设置（标准波特率与 CAN FD 数据域仲裁域波特率、终端电阻设置、单次发送/正常模式）。
  - 硬件滤波或驱动层软滤波规则下发。
- **总线诊断与指标**：
  - 获取总线负载率、错误帧计数（TEC/REC、总线关闭状态、超限计数）。
- **信号与事件**：
  - `void frameReceived(const CanRawFrame &frame);`
  - `void frameSent(const CanRawFrame &frame);`
  - `void busStatusChanged(int channel, CanBusStatus status);`
  - `void errorOccurred(const QString &err);`

#### 4.2 适配器驱动矩阵
1. **`SimulationCanAdapter` (核心高优先级)**：
   - **零硬件离线开发支持**：纯内存回环与虚拟总线。
   - **内置虚拟 ECU 模拟机**：
     - 自动发送 0x100、0x280、0x320 周期报文（含发动机转速、车速、正弦波信号变化）。
     - 支持模拟 UDS 诊断响应（对 0x7DF / 0x7E0 做出 0x7E8 回复，支持 0x10 会话切换、0x22 读数据、0x27 伪随机数种子与密钥校验）。
     - 故障注入能力（模拟总线高负载、丢包、错误帧）。
2. **`QtSerialBusAdapter` (Qt 官方插件生态桥接)**：
   - 使用 Qt 自带的 `QCanBus` 与 `QCanBusDevice`，统一桥接：
     - `peakcan`: PEAK PCAN-USB / PCAN-PCI
     - `vectorcan`: Vector VN 系列驱动
     - `passthrucan`: J2534 PassThru OBD 适配盒
     - `systeccan`: SYS TEC USB-CANmodul
     - `tinycan`: MHS TinyCAN
     - `virtualcan`: Qt 本地虚拟 CAN
     - `socketcan`: Linux 环境 SocketCAN
3. **`ZlgCanAdapter`**：
   - 适配周立功 `zlgcan.dll`（USBCAN-I/II、USBCANFD-200U/400U 等）。
4. **`ChuangXinCanAdapter`**：
   - 适配创芯科技 `ControlCAN.dll`（USBCAN-2C / USBCAN-E-U 等）。
5. **`TosunCanAdapter`**：
   - 适配同星智能 TSMaster API（TC1014 / TC1016 等）。

---

### 3. 引擎层（Engine Layer）
纯粹的业务与协议引擎，完全独立于 UI 控件，可作为纯命令行工具或单元测试运行。

1. **`IsoTpEngine` (ISO 15765-2)**：
   - 实现网络层分包与重组逻辑：
     - 单帧 Single Frame (SF, 数据 <= 7 字节或 CAN FD <= 63 字节)
     - 首帧 First Frame (FF, 包含总长 DL)
     - 连续帧 Consecutive Frame (CF, 带序列号 SN 0~15 翻转)
     - 流控帧 Flow Control (FC, FS=CTS/WT/OVFLW, BS块大小, STmin分离时间)
   - 异步超时定时器控制（N_Ar, N_As, N_Br, N_Bs, N_Cr, N_Cs）。
2. **`UdsEngine` (ISO 14229-1 / ISO 14229-2)**：
   - 诊断请求/响应状态机：
     - 会话控制 (0x10: 默认、编程、扩展会话)
     - 安全访问 (0x27: Seed & Key 计算插件机制)
     - 数据读写 (0x22 / 0x2E: 按 DID 寻址)
     - 故障码操作 (0x19 / 0x14: 读 DTC、清除 DTC)
     - 固件传输 (0x34 RequestDownload, 0x36 TransferData, 0x37 RequestTransferExit)
     - 例程控制 (0x31: 擦除 Flash、校验 CRC)
     - 负响应处理 (0x7F Negative Response Code, 如 0x78 请求已正确接收但响应挂起)
3. **`DbcParserEngine`**：
   - 纯 C++ 实现的高性能 DBC 文件解析器。
   - 解析节点 (BU_)、报文 (BO_)、信号 (SG_)、起始位、长度、Byte Order (Intel/Motorola)、符号类型、Factor/Offset、Min/Max、Unit、枚举值表 (VAL_)、多路复用 Multiplexor (M/m)。
   - 报文原始字节流到物理值信号映射。
4. **`RecordingEngine`**：
   - 异步无锁/高性能双缓冲录制写入器，支持 ASC、CSV、BLF（可扩展）格式。
   - 离线日志索引引擎，基于毫秒时间戳的快速二分检索与回放生成器。
5. **`KnowledgeEngine`**：
   - 内置 SAE J2012 / ISO 14229 标准 DTC 字典与 UDS 服务字典。
   - 故障代码智能解释与专家排障指引。

---

### 2. 服务层（Service Layer）
连接界面与引擎，负责业务流程与生命周期。

1. **`FlashService`**：
   - **工程配置管理**：支持刷写工程导入导出（JSON格式，包含波特率、DID参数、Flash驱动、安全算法dll/脚本、Hex/Bin路径）。
   - **会话与状态机**：
     `Idle` -> `EnterExtendedSession` -> `SecurityAccess` -> `EnterProgrammingSession` -> `EraseMemory` -> `DownloadFlashDriver` -> `TransferBlocks` -> `VerifyChecksum` -> `EcuReset`。
   - **任务编排器**：支持步骤前后钩子、进度计算 (0-100%)、速度估计 (KB/s)、剩余时间、失败回滚与重试机制。
2. **`DiagnosticService`**：
   - 提供给诊断界面的统一服务，封装常用会话管理、周期性 0x3E TesterPresent 会话保持定时器、DID 批量读取、DTC 自动化扫描。
3. **`BusMonitorService`**：
   - 总线报文分发中心（Publisher/Subscriber 模型），支持多视窗订阅。
   - 智能频次聚合、统计总线负载率、峰值 FPS。

---

### 1. 界面层（Presentation Layer）
现代美观、高帧率刷新、高交互性的 CAN 工作台面板：

1. **报文跟踪台 (`CanTracePanel`)**：
   - 支持滚动模式 (Scroll View) 与 覆盖聚合模式 (Overwrite View 按 ID 合并最新状态)。
   - 支持高亮变化字节、十六进制/ASCII 显示、DBC 展开信号值树状结构。
   - 丰富的快速过滤器（按 ID、通道、扩展帧/FD、方向 Tx/Rx、掩码屏蔽）。
2. **信号曲线台 (`CanSignalPlotPanel`)**：
   - 基于 `QCustomPlot`，用户选择 DBC 中的某几个信号（如转速、车速、电压、温度），实时绘制动态波形曲线。
   - 支持波形暂停、滚轮缩放、游标测量（Cursor）与极值统计。
3. **刷写工作台 (`CanFlashPanel`)**：
   - 固件文件拖拽（支持 S19, Intel Hex, Bin），解析显示各段起始地址与大小。
   - 一键刷写、进度条、实时速率、阶段状态提示、日志详情输出。
4. **诊断工作台 (`CanDiagnosticPanel`)**：
   - 标准 UDS 调试终端，支持快捷输入命令、DID 读取树、DTC 读取面板（支持读取当前故障、历史故障、冻结帧数据）、一键清码。
5. **AI 智能助手 (`CanAiAssistant`)**：
   - “AI 墩墩” 智能总线助理：报文异常检测、未定义 ID 逆向提示、DTC 故障诊断与排查方案生成、刷写失败原因智能分析。

---

## User Review Required

> [!IMPORTANT]
> **关于 Qt 依赖与 `serialbus` 模块：**
> - Qt 的 `serialbus` 模块是官方内置的（从 Qt 5.8+ 到 Qt 6 均标配），在 `.pro` 中增加 `QT += serialbus` 即可原生支持 `peakcan`, `vectorcan`, `passthrucan`, `systeccan`, `tinycan`, `virtualcan`, `socketcan`。
> - 周立功 ZLG 与 创芯 ControlCAN、同星智能等国内硬件则通过适配层的动态库加载机制无缝并存。
> - 纯软件仿真 `SimulationCanAdapter` 不需要任何硬件或驱动 dll，编译后即可全链路跑通。

> [!NOTE]
> **重构实施策略：平滑演进与向下兼容**
> 1. 原有的 `CanInterface` 可以保留为向上兼容门面 (Facade)，或者内部委托给新的 `ICanAdapter`。
> 2. 原有的 `canviewpanel.cpp`、`candevicedialog.cpp`、`cantool.cpp` 逐步切换到适配层与服务层，保证原有项目不中断。

---

## Proposed Changes

我们将分层次建立清晰的目录结构：
`PhudonTools/CAN/`
- `Adapter/`: 硬件适配层 (`icanadapter.h`, `canadapterfactory.h/cpp`, `simulationcanadapter.h/cpp`, `qtserialbusadapter.h/cpp`, `zlgcanadapter.h/cpp`, `chuangxincanadapter.h/cpp`, `tosuncanadapter.h/cpp`)
- `Engine/`: 协议与算法引擎 (`isotpengine.h/cpp`, `udsengine.h/cpp`, `dbcparserengine.h/cpp`, `recordingengine.h/cpp`, `knowledgeengine.h/cpp`)
- `Service/`: 业务与会话服务 (`flashservice.h/cpp`, `diagnosticservice.h/cpp`, `busmonitorservice.h/cpp`, `canprojectmanager.h/cpp`)
- `UI/`: 现代工作台界面 (`cantracepanel.h/cpp`, `cansignalplotpanel.h/cpp`, `canflashpanel.h/cpp`, `candiagnosticpanel.h/cpp`, `canaiassistant.h/cpp`, `candevicemanagerdialog.h/cpp`)

### 核心阶段规划

#### Phase 1: 适配层核心抽象与仿真/Qt/ZLG适配器
- 定义 `CanRawFrame`、`CanChannelConfig`、`ICanAdapter` 抽象基类。
- 实现 `CanAdapterFactory` 统一管理。
- 实现 `SimulationCanAdapter`（内置虚拟ECU模拟器，具备离线可运行能力）。
- 实现 `QtSerialBusAdapter`（接入 `peakcan`, `vectorcan`, `passthrucan`, `systeccan`, `tinycan`, `virtualcan`, `socketcan`）。
- 将现有的 ZLG 与 创芯代码重构成 `ZlgCanAdapter` 与 `ChuangXinCanAdapter`。
- 重构 `candevicedialog` 为全新的 `CanDeviceManagerDialog`，支持主流硬件与仿真模式的直观切换。

#### Phase 2: 引擎层实现（ISO-TP、UDS、DBC、录制）
- 实现 `IsoTpEngine`（完整的网络层分包与流控）。
- 实现 `UdsEngine`（UDS 14229 核心状态机与协议栈）。
- 实现 `DbcParserEngine`（DBC 信号解析器）。
- 实现 `RecordingEngine`（ASC/CSV 日志录制与回放）。

#### Phase 3: 服务层实现（FlashService 刷写编排、诊断服务、总线监控服务）
- 实现 `FlashService` 任务编排状态机与 S19/Hex/Bin 传输。
- 实现 `DiagnosticService` 诊断自动化调度。
- 实现 `BusMonitorService` 报文订阅中心。

#### Phase 4: 界面层重构与 AI 智能体集成
- 重塑 `CANTool` 工作台容器，支持多面板布局：
  - 报文跟踪 (`CanTracePanel`)
  - 信号曲线 (`CanSignalPlotPanel`)
  - 刷写台 (`CanFlashPanel`)
  - 诊断台 (`CanDiagnosticPanel`)
  - AI 墩墩总线助手 (`CanAiAssistant`)

---

## Verification Plan

### 1. 离线仿真自测 (无需硬件)
- 启动 `SimulationCanAdapter`：
  - 验证报文跟踪面板能否平稳接收到虚拟 ECU 的周期报文。
  - 验证 ISO-TP 引擎与虚拟 ECU 之间的多帧数据交互（分包/流控/组包）。
  - 验证 UDS 诊断控制台发送 0x10、0x22、0x27 时能否得到正确的虚拟回复。
  - 验证离线刷写流程能否跑通完整的状态机与进度动画。

### 2. Qt 官方内置插件测试
- 通过 `QCanBus::instance()->plugins()` 列出本机支持的插件。
- 测试 `virtualcan` 插件本机收发互通。
- 若连接了 PCAN 或 Vector 卡，测试 `peakcan` / `vectorcan` 的硬件识别与通信。

### 3. ZLG / 创芯硬件兼容性测试
- 加载现有 `zlgcan.dll` / `ControlCAN.dll`，验证真实硬件的打开、通道启停、收发功能。
