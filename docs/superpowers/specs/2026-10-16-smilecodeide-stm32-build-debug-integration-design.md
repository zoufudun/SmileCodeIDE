# SmileCodeIDE 代码编辑器深度集成 STM32 编译构建与 GDB 硬件调试系统设计

**状态**：已由用户确认批准 (Approved)  
**日期**：2026-10-16  
**设计范围**：SmileCodeIDE 主程序 (`PhudonTools`) 代码编辑器 `CodeEditor`、主窗口 `MainWindow`，以及与 `STM32WorkbenchPlugin/Core` 核心引擎的深度原生集成。

---

## 1. 背景与目标 (Background & Objectives)

### 1.1 现状与痛点
1. **代码编辑器与调试脱节**：当前的 `CodeEditor`（基于 QScintilla）虽具备语法高亮、同名符号高亮、粘性滚动等功能，但缺乏断点标记槽（Breakpoint Gutter），无法直接在代码行旁下断点，也无法在单步执行时展示当前执行代码行（黄色箭头指针）。
2. **主程序构建系统陈旧**：主窗口中的 `BuildSystem` 与 `MainWindow::buildProject()` 逻辑简陋，硬编码了 `-mcpu=cortex-m3` 和 `STM32F103xB`，缺少增量编译依赖追踪，且构建报错无法与编辑器行号形成双向联动。
3. **调试逻辑为临时阻塞草稿**：主窗口的 `MainWindow::startDebug()` 直接调用 `QThread::sleep(2)` 阻塞主 GUI 线程等待 OpenOCD，未采用 GDB/MI 结构化通信协议，无法获取调用栈、局部变量、CPU 寄存器等关键硬件调试状态。
4. **成熟资产孤岛化**：仓库中已在 `Plugins/STM32WorkbenchPlugin/Core/` 下实现了功能完善的 `GdbMiEngine`、`BuildManager`、`FlashProgrammer`、`MakefileGenerator`、`ToolchainDetector`、`STM32Profile`，但目前仅作为独立插件/独立工作台存在，未能深度赋能主 IDE 代码编辑器。

### 1.2 核心目标
1. **编辑器原生断点与执行线跟踪**：在 `CodeEditor` 左侧开辟断点槽，单击红点断点 (●)，单步执行时黄色指针 (➜) 指示当前指令行，并在源文件中居中高亮。
2. **专业调试停靠面板矩阵**：主窗口提供调用栈 (Call Stack)、局部变量与监视 (Variables & Watch)、CPU 核心寄存器 (Registers)、断点列表 (Breakpoints)、GDB 原生终端。
3. **全异步非阻塞硬件调试会话**：通过 `GdbServerManager` 探活 OpenOCD / J-Link GDB Server 端口，驱动 `GdbMiEngine` 实现 F5/F10/F11 极速交互，杜绝任何 UI 假死与僵尸进程。
4. **智能构建与工程配置持久化**：动态生成带 `-MMD -MP` 增量依赖追踪的 Makefile，支持 F1/F4/F7/H7 多架构；编译诊断错误双击跳转源码；项目配置自动存入 `.smilecode/project.json`。

---

## 2. 整体系统架构与模块拓扑 (Architecture & Topology)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           SmileCodeIDE (PhudonTools)                            │
├─────────────────────────────────────────────────────────────────────────────────┤
│                               [表现与交互层 UI Layer]                            │
│  ┌───────────────────────┐  ┌─────────────────────────────────────────────────┐ │
│  │   CodeEditor 增强     │  │          主窗口调试停靠矩阵 (Dock Widgets)        │ │
│  │ • Margin 1: 断点红点   │  │ • CallStackDock   : 调用栈帧与双击转源码        │ │
│  │ • Margin 1: 执行黄箭头 │  │ • VariablesDock   : 局部变量 + 自定义 Watch 监视 │ │
│  │ • 调试停机行背景微光   │  │ • RegistersDock   : Cortex-M 核心寄存器变色显示 │ │
│  │ • 双击诊断错误行跳转   │  │ • BreakpointsDock : 全工程断点管理与一键启闭    │ │
│  └───────────┬───────────┘  └────────────────────────┬────────────────────────┘ │
├──────────────┼───────────────────────────────────────┼──────────────────────────┤
│              │          [调度中心 Controller Layer]    │                          │
│              └────────────► STM32DebugController ◄────┘                          │
│                             (统一生命周期与状态机)                               │
├─────────────────────────────────────────────────────────────────────────────────┤
│                           [核心引擎层 Core Engines]                              │
│  (复用 Plugins/STM32WorkbenchPlugin/Core，作为原生编译单元引入 PhudonTools.pro)    │
│                                                                                 │
│  ┌─────────────────────────┐  ┌──────────────────────────┐  ┌─────────────────┐ │
│  │      BuildManager       │  │       GdbMiEngine        │  │ FlashProgrammer │ │
│  │ • MakefileGenerator    │  │ • 异步双工 GDB/MI 状态机 │  │ • ST-Link       │ │
│  │ • 诊断解析 (file:line)  │  │ • 栈帧/变量/寄存器解析   │  │ • J-Link        │ │
│  │ • Flash/RAM 占用百分比  │  │ • 端口探活与 Server 守护 │  │ • CMSIS-DAP     │ │
│  └─────────────────────────┘  └──────────────────────────┘  └─────────────────┘ │
│                                                                                 │
│  ┌─────────────────────────┐  ┌──────────────────────────┐  ┌─────────────────┐ │
│  │    ToolchainDetector    │  │       STM32Profile       │  │  ProbeDetector  │ │
│  │ • 自动扫描 GCC/GDB/Make │  │ • F1/F4/F7/H7 架构与存储 │  │ • USB VID/PID   │ │
│  └─────────────────────────┘  └──────────────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. 核心子系统详细设计 (Detailed Subsystem Design)

### 3.1 代码编辑器增强 (`CodeEditor` Breakpoint & Cursor System)

#### 3.1.1 边距与标记 (Margins & Markers) 配置
在 `CodeEditor::setupEditor(QsciScintilla *editor)` 中配置边距结构：
- **Margin 0**：行号显示栏 (`LineNumbersMargin`)，宽度根据总行数动态计算。
- **Margin 1 (断点与指示器边距，新增)**：
  - 类型：`QsciScintilla::SymbolMargin`，宽度固定为 16px。
  - 属性：启用交互敏感性 `editor->setMarginSensitivity(1, true)`。
  - 标记定义：
    - `MARKER_BREAKPOINT = 26`：形状为 `QsciScintilla::Circle`，填充色 `#E53935`，边框色 `#B71C1C`。
    - `MARKER_EXECUTION_POINTER = 27`：形状为 `QsciScintilla::RightArrow`，填充色金黄 `#FFD700`。
- **Margin 2**：代码树状折叠栏 (`BoxedTreeFoldStyle`)。
- **指示器与高亮**：
  - 当前停机执行行背景高亮：暗黑主题背景色 `#3A351C`（半透明弱琥珀黄），明亮主题背景色 `#FFF8E1`。

#### 3.1.2 断点交互与双向映射
- 监听 `QsciScintilla::marginClicked(int margin, int line, Qt::KeyboardModifiers)`：
  1. 仅响应 `margin == 1`；
  2. 检查 `editor->markersAtLine(line) & (1 << MARKER_BREAKPOINT)`；
  3. 若存在：调用 `editor->markerDelete(line, MARKER_BREAKPOINT)`，触发信号 `breakpointRemoved(const QString &file, int line)`；
  4. 若不存在：调用 `editor->markerAdd(line, MARKER_BREAKPOINT)`，触发信号 `breakpointAdded(const QString &file, int line)`；
- 快捷键支持：全局捕获 `F9` 快速切换当前光标所在行的断点状态；
- 编辑器右键菜单扩展：新增【切换断点】、【禁用/启用断点】、【清空所有断点】选项。

#### 3.1.3 调试停机与执行线自动滚动
- 当控制器收到 GDB `*stopped,reason="breakpoint-hit",frame={fullname="...",line="..."}` 或单步停机信号：
  1. 解析源文件完整物理路径与目标行号；
  2. 若该文件未在当前编辑器视口打开，则调用 `m_codeEditor->openFile(file)`；
  3. 在所有打开的编辑器中清除历史 `MARKER_EXECUTION_POINTER` 标记；
  4. 在目标行调用 `editor->markerAdd(line, MARKER_EXECUTION_POINTER)`；
  5. 调用 `editor->ensureLineVisible(line)` 确保目标行处于视口中央，并将光标移动至该行起始处；
- 当调试继续运行 (`continue`)、退出或复位时，自动清除所有编辑器的执行指针标记。

---

### 3.2 硬件 GDB 调试会话与停靠面板矩阵 (GDB Debugging Subsystem)

#### 3.2.1 状态机与生命周期 (`STM32DebugController`)
维护严格且健壮的状态机体系，杜绝状态错乱：
```
       [Idle]
         │
    (F5 / Debug) ──> [Checking / Auto-Build]
                            │
                   [Starting GDB Server] ◄── 非阻塞端口探活 (3333/2331)
                            │
                     [Launching GDB] ◄── 握手: target remote, reset halt, sync-breaks
                            │
               ┌────────────┴────────────┐
               ▼                         ▼
          [Stopped]  ◄──────────────► [Running]
        (命中/单步/暂停)   (F5 Continue)    (全速运行)
               │                         │
               └────────────┬────────────┘
                            │ (Shift+F5 / Stop)
                            ▼
                    [Terminating / Clean]
                            │
                         [Idle]
```

- **非阻塞端口探活**：`GdbServerManager` 启动 OpenOCD/J-Link 后，启动 QTimer 每 100ms 尝试 `QTcpSocket::connectToHost("127.0.0.1", port)`。一旦连接成功即断开并确认就绪，立即启动 GDB。最大超时 5 秒，超时则提示错误日志并杀死后台进程。
- **断点全量同步**：在 GDB 启动完成并连接 target 之后，将用户在源码中标记的所有断点集中通过 `-break-insert filename:line` 下发；在后续单步或运行过程中，用户对断点的增删即时下发对应的 GDB/MI 命令。
- **优雅退出保障**：停止调试时向 GDB 下发 `-gdb-exit` 并断开 socket，强制 terminate OpenOCD/J-Link 进程，确保 Windows 任务管理器中不残留后台悬挂进程。

#### 3.2.2 停靠面板矩阵 (Dock Panels)
在 `MainWindow` 中使用标准 `QDockWidget` 搭建现代化嵌入式调试工作区（支持拖拽拆分、折叠、合并标签页、保存布局）：
1. **调用栈面板 (`CallStackDock`)**：
   - 监听 GDB `-stack-list-frames`；
   - 树状列表呈现帧层级、函数符号、源文件名、行号与指令地址；
   - 双击栈帧：向 GDB 发送 `-stack-select-frame <level>`，代码编辑器自动跳转至该帧的源码行，变量面板同步切换上下文。
2. **变量观察面板 (`VariablesDock`)**：
   - **局部变量 (Locals)**：单步暂停时自动请求 `-stack-list-locals --simple-values`，表格展示变量名、类型、当前值（支持 16 进制/10 进制切换）；
   - **数值变动高亮**：与上一帧比较，数值发生变化的变量以亮橙色突出显示；
   - **监视列表 (Watch)**：允许用户输入任意合法 C 表达式（如 `*pBuffer`、`huart1.gState`、`TIM2->CNT`），暂停时执行 `-data-evaluate-expression` 自动刷新。
3. **CPU 核心寄存器面板 (`RegistersDock`)**：
   - 展示 ARM Cortex-M 核心寄存器：`R0` ~ `R12`、`SP` (MSP/PSP)、`LR`、`PC`、`xPSR`；
   - 数值变动时同样采用醒目色高亮标记。
4. **断点列表面板 (`BreakpointsDock`)**：
   - 集中罗列全工程断点：ID、状态（启用/禁用复选框）、文件路径、行号；
   - 双击直达代码行；提供一键全部禁用与全部删除按钮。
5. **GDB 交互终端与控制台**：
   - 整合到底栏中央 Tab 中，支持原生 GDB 命令下发与回显流展示。

---

### 3.3 STM32 构建引擎与工程配置持久化 (Build & Project Persistence)

#### 3.3.1 动态 Makefile 与增量构建
- 引入 `Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator` 替代旧版 `BuildSystem`；
- 芯片内核参数自动匹配：
  - Cortex-M0/M0+：`-mcpu=cortex-m0 -mthumb`
  - Cortex-M3 (STM32F1)：`-mcpu=cortex-m3 -mthumb`
  - Cortex-M4 (STM32F4)：`-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
  - Cortex-M7/H7：`-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard`
- 依赖追踪机制：生成规则带有 `-MMD -MP -MF"$(@:%.o=%.d)"`，修改任意头文件精确触发局部重编，杜绝多余全量编译；
- 跨平台清理：适配 Windows 下 `cmd.exe /c del` 与 Linux 下 `rm -rf`。

#### 3.3.2 诊断解析与双击穿透
- `BuildManager` 采用正规正则引擎捕获 GCC 输出：  
  `^([^:\n\r]+):(\d+):(\d+)?:?\s*(error|warning|fatal error|note):\s*(.+)$`
- 在底栏【编译问题 (Problems)】以列表展示严重性图标、文件名、行列号、错误内容；
- **双击穿透**：点击条目直接调用 `m_codeEditor->openFileAndGoTo(file, line, col)`，极大提升排错效率。
- **物理资源报告**：编译成功后调用 `arm-none-eabi-size`，精准计算并汇报 Flash 和 RAM 物理占用百分比。

#### 3.3.3 工程配置智能识别与持久化规范 (`.smilecode/project.json`)
- 打开工程目录时：
  1. 优先读取 `<project_root>/.smilecode/project.json`；
  2. 若不存在，遍历检测 `.ioc`（STM32CubeMX）、`.uvprojx`（Keil）或启动文件（`startup_stm32*.s`），推导目标芯片型号与启动脚本，并提示自动初始化持久化文件；
- 规范格式：
```json
{
  "projectType": "STM32",
  "targetChip": "STM32F103C8",
  "series": "F1",
  "core": "Cortex-M3",
  "probe": "STLINK",
  "openocdConfig": "target/stm32f1x.cfg",
  "flashAddress": "0x08000000",
  "customDefines": [
    "USE_HAL_DRIVER",
    "STM32F103xB"
  ],
  "toolchainPath": "D:/Soft/Toolchains/arm-none-eabi-gcc"
}
```

---

## 4. 关键快捷键与工具栏设计 (Keybindings & Actions)

| 操作 | 快捷键 | 触发行为 | 关联组件 |
| :--- | :--- | :--- | :--- |
| **一键构建工程** | `F7` / `Ctrl+B` | 增量编译并生成 ELF/HEX/BIN，汇报资源消耗 | `BuildManager` |
| **单文件编译** | `Ctrl+F7` | 仅编译当前编辑器活动的 `.c`/`.cpp` 文件 | `BuildManager` |
| **清理工程** | `Shift+F7` | 清理 build 目录与生成产物 | `BuildManager` |
| **固件烧录** | `Ctrl+F5` | 通过 ST-Link/J-Link/DAPLink 烧录并复位运行 | `FlashProgrammer` |
| **启动 / 继续调试** | `F5` | 启动 GDB Server 与 GDB 会话，或在暂停时继续运行 | `GdbMiEngine` |
| **停止调试** | `Shift+F5` | 退出 GDB，复位硬件，终止后台 Server 守护进程 | `GdbServerManager` |
| **复位并停机** | `Ctrl+Shift+F5`| 发送 `monitor reset halt`，重置到启动文件入口 | `GdbMiEngine` |
| **单步跳过 (Next)** | `F10` | 执行当前行代码，遇到函数调用直接跳过 | `GdbMiEngine` |
| **单步进入 (Step)** | `F11` | 进入当前行调用的子函数内部 | `GdbMiEngine` |
| **单步跳出 (Finish)** | `Shift+F11` | 执行完当前函数剩余语句并返回上一层调用者 | `GdbMiEngine` |
| **切换当前行断点** | `F9` | 在编辑器当前行切换设置/取消红点断点 | `CodeEditor` |

---

## 5. 验证与测试策略 (Verification & Testing)

1. **单元与集成测试**：
   - 验证 `CodeEditor` 的 `marginClicked` 断点增删信号与高亮行准确度；
   - 验证 `GdbMiEngine` 的 GDB/MI 异步指令序列解析（`*stopped`、`^done,stack=[...]`、`^done,locals=[...]`）；
   - 验证 Makefile 生成器在 Windows 中文路径与带空格路径下的正确性。
2. **端到端测试场景**：
   - **场景 A（构建验证）**：打开包含编译错误的 STM32 工程，执行 `F7` 构建，验证问题列表是否准确抓取行号，双击是否准确定位到错误代码行；
   - **场景 B（硬件调试闭环）**：在 `main.c` 核心主循环中点击 Margin 1 设置断点（红点显示），按 `F5` 启动调试，验证程序是否成功在 `main()` 暂停，黄色箭头指针是否正确定位；
   - **场景 C（单步与变量观测）**：按 `F10` 逐行单步，观察局部变量数值变动（变橙色）；观察调用栈层级跳转；按 `F5` 继续运行，命中设置的红点断点。
   - **场景 D（会话清理）**：按 `Shift+F5` 停止调试，验证 OpenOCD 与 GDB 进程是否完全退出，编辑器黄色指示箭头是否完全消失。
