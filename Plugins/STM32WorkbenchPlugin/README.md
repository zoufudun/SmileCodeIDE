# ⚡ STM32WorkbenchPlugin / STM32 独立嵌入式开发与硬件调试工作台 ⚡

> **SmileCodeIDE 模块化独立套件**：涵盖 GNU Arm GCC/Make 一键编译构建、固件烧录（OpenOCD/J-Link/ST-Link）与硬件在线 GDB 调试的生产级全流程解决方案。

---

## 📖 模块定位与架构设计 (Architecture & Overview)

本项目将嵌入式开发中割裂的“编译”、“构建”、“烧录”、“调试”全流程抽离解耦，打造为一个**高内聚、零依赖业务逻辑的独立子系统**。

既可以作为 **Qt 原生动态扩展插件 (DLL/SO)** 运行在 SmileCodeIDE 应用中心中，也可以作为 **独立轻量级 IDE 可执行文件 (`STM32Workbench.exe`)** 单独分发与运行。

### 1. 整体分层架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   STM32 Embedded Workbench Architecture                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                          [1. 交互表现层 UI Layer]                             │
│  • STM32WorkbenchWidget : 一体化工作台总览，一键构建/烧录/调试多核调度      │
│  • GdbDebugPanel        : 硬件调试观测窗 (调用栈/局部变量/寄存器/断点/终端) │
│  • ToolchainConfigDialog: 跨平台工具链一键环境自检与路径配置                │
│  • FlashConfigDialog    : 多硬件烧录器与 Flash 参数配置                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                          [2. 核心调度层 Core Engine]                         │
│  • ToolchainDetector    : ARM-GCC / GDB / Make / OpenOCD / J-Link 智能探测  │
│  • STM32ProfileManager  : Cortex-M0/M3/M4/M7 芯片档案与链接脚本自动生成工厂│
│  • MakefileGenerator    : 跨平台依赖追踪 Makefile 自动生成引擎 (-MMD -MP)   │
│  • BuildManager         : 异步多线程并行构建调度器与 GCC 错误行号跳转解析   │
│  • FlashProgrammer      : OpenOCD / J-Link Commander / ST-Link 多协议烧录   │
│  • GdbServerManager     : OpenOCD / J-Link GDB Server 守护进程与端口探活   │
│  • GdbMiEngine          : 基于 GDB/MI 协议的异步双工调试状态机引擎          │
├─────────────────────────────────────────────────────────────────────────────┤
│                          [3. 宿主与独立运行适配层]                          │
│  • IAppPlugin           : 遵循 SmileCodeIDE 插件规范，实现免修改热插拔      │
│  • Standalone App       : 包含独立 main.cpp 与工程，支持脱离主 IDE 独立运行 │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 🌟 核心功能特性 (Key Features)

### 1. ⚙️ GNU Arm GCC / Make 一键编译构建
- **智能环境自检**：自动扫描系统环境变量 `PATH` 以及常见安装目录（如 `D:\Toolchains\arm-gnu-toolchain`、`D:\Toolchains\mingw64` 等），探测 `arm-none-eabi-gcc`、`g++`、`make`、`size` 等工具。
- **动态 Makefile 生成**：
  - 优先读取 Keil `.uvprojx` 工程清单，自动选用同名 GCC 启动文件；没有清单时扫描 `.c`、`.cpp`、`.s`、`.S` 等源文件与头文件目录；
  - 针对 Windows / Linux 自动适配清理指令（`cmd /c rd /s /q` vs `rm -rf`）；
  - 引入 `-MMD -MP -MF` 依赖追踪机制，修改头文件精准触发增量重编；
  - 编译完成后自动运行 `arm-none-eabi-size`，精准计算并汇报 Flash 和 SRAM 物理消耗百分比。
- **智能诊断捕获**：流式解析 GCC 编译器报错信息，匹配 `file:line:col: error/warning`，支持双击一键穿透跳转至代码编辑器定位。

### 2. ⚡ 多硬件适配固件一键烧录与下载 (Flash)
- **多烧录器支持**：
  - **OpenOCD**：支持 ST-Link V2/V3、CMSIS-DAP / DAPLink、J-Link 调试适配卡；
  - **SEGGER J-Link 官方**：动态生成 Commander 脚本执行高速 SWD 烧录与校验；
  - **STMicroelectronics STM32CubeProgrammer CLI**；
  - **开源 st-flash (stlink-tools)**。
- **丰富烧录控制**：起始基地址配置（默认 `0x08000000`）、全片擦除/扇区擦除、自动 Flash 校验 (Verify)、烧录完成后自动复位并运行 (Reset & Run)。

### 3. 🐞 基于 GDB/MI 协议的在线硬件调试 (Hardware Debug)
- **GDB Server 自动守护**：管理 OpenOCD / J-Link GDB Server 后台进程，具备端口探活、等待连接就绪后自动启动 GDB。
- **全异步 GDB/MI 状态机**：
  - 目标控制：启动运行、继续 (Continue / F5)、暂停 (Pause)、单步跳过 (Next / F10)、单步步入 (Step / F11)、单步跳出 (Finish)、硬件复位停止 (Reset Halt)；
  - 断点控制：插入断点、删除断点、与代码编辑器行号边距双向联动；
  - 调用栈观测：解析 `stack-list-frames`，呈现当前线程调用层级、对应源文件与行号，双击即跳；
  - 局部变量与监视：解析 `stack-list-locals`，支持监视任意 C 表达式求值；
  - CPU 核心寄存器：实时呈现 Cortex-M 核心 R0~R12、SP、LR、PC、xPSR 寄存器数值；
  - 原生控制台回显：支持自由执行原生 GDB 指令。

---

## 🚀 独立使用与构建方式 (How to Build & Run)

### 模式 A：作为独立应用程序运行 (Standalone App)
通过 Qt Creator 或命令行打开 `STM32WorkbenchStandalone.pro`：
```bash
qmake STM32WorkbenchStandalone.pro
mingw32-make -j4
./release/STM32Workbench.exe
```
打开后即可作为一个独立的轻量级 STM32 编译与硬件调试平台使用！

### 模式 B：作为 SmileCodeIDE 动态插件加载 (Plugin Mode)
通过 Qt Creator 或命令行打开 `STM32WorkbenchPlugin.pro`：
```bash
qmake STM32WorkbenchPlugin.pro
mingw32-make -j4
```
编译产物 `STM32WorkbenchPlugin.dll` 会自动拷贝至 `PhudonTools/plugins/` 目录。
启动 SmileCodeIDE 后，进入 **应用中心 (App Hub)** -> **嵌入式开发** 分类，即可看到并启动该工作台。
