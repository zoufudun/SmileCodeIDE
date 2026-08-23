<div align="center">

# ⚡ SmileCodeIDE / PhudonTools ⚡
### 一站式现代化嵌入式开发 IDE 与全功能硬件调试工作台
### Modern All-in-One Embedded IDE & Industrial Hardware Debugging Suite

[![Qt Version](https://img.shields.io/badge/Qt-5.15%20%2F%206.x-green.svg?logo=qt)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)](https://github.com/)
[![License](https://img.shields.io/badge/License-MIT%20%2F%20GPLv3-orange.svg)](https://github.com/)
[![Toolchain](https://img.shields.io/badge/Compiler-GCC%20%7C%20Clang%20%7C%20Arm--none--eabi-red.svg)](https://developer.arm.com/)
[![Theme](https://img.shields.io/badge/Theme-Dark%20%26%20Light%20Adaptive-purple.svg)](https://github.com/)

[**功能特性**](#-核心功能亮点) • [**架构模块**](#-功能模块详解) • [**快速上手**](#-快速上手与构建) • [**设计美学**](#-设计美学与交互) • [**更新日志**](#-最新更新) 

---

## 📖 项目简介 (Overview)

**SmileCodeIDE (PhudonTools)** 是一款专为**嵌入式软件工程师、硬件研发人员、物联网 (IoT) 开发者及工业自动化工程师**量身打造的跨平台一体化集成开发与硬件调试工作台。

传统嵌入式开发往往需要在文本编辑器（VS Code/Keil/IAR）、串口调试助手、波形示波器软件、CAN/CAN FD 报文分析仪、CANopen 协议配置器、IAP 烧录工具及自动化监控大屏之间来回切换，数据割裂且流程繁琐。

**SmileCodeIDE** 彻底打破工具壁垒，深度整合了 **C/C++ 专业代码编辑器、多通道高速串口示波器、CAN/CAN FD/CANopen 总线分析仪、STM32 跨平台交叉编译构建系统、MCU 固件 IAP 在线升级、自定义工业仪器仪表设计器与设备大屏监控**，带来沉浸、高效、高颜值的全流程开发体验。

---

## 🌟 核心功能亮点 (Key Features)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          SmileCodeIDE / PhudonTools                             │
├───────────────────┬───────────────────┬───────────────────┬─────────────────────┤
│ 📝 专业代码编辑器  │ 📈 串口示波器     │ 🚗 CAN / CAN-FD   │ 🛠️ 编译与 IAP 烧录  │
│ • Sticky Scroll   │ • 多通道并发会话  │ • CAN / CAN FD 驱动│ • GNU Arm 交叉编译  │
│ • 符号同名高亮    │ • 实时曲线动态绘制 │ • CANopen 主站/PDO│ • MCU Profile 配置  │
│ • 跨文件转定义(F12)│ • FFT 频谱与统计  │ • 总线负载率分析  │ • 固件 IAP 在线升级 │
│ • 面包屑与大纲树  │ • 自定义协议解析器│ • 报文过滤与定时发│ • 内嵌交互控制台    │
└───────────────────┴───────────────────┴───────────────────┴─────────────────────┘
```
---

## 🛠️ 功能模块详解 (Detailed Modules)

### 1. 📝 专业 C/C++ 代码编辑与符号导航系统 (CodeEditor)
基于 QScintilla 深度定制打造的轻量级、低延迟、现代化嵌入式代码编辑器：
- **📌 粘性滚动 (Sticky Scroll / 顶部悬挂函数头)**：
  - 在浏览超长函数或复杂类体代码时，函数/类声明行自动悬挂固定在编辑器顶部；
  - 100% 像素级对齐行号边距，支持语法着色与横向同频滚动；
  - 单击悬挂栏直达函数定义，同步聚焦与面包屑导航。
- **💡 智能符号同名高亮 (Occurrence Highlighting)**：
  - 基于语法分析与词法范围的高性能同名符号实时高亮；
  - 采用预构建偏移表与 $O(\log L)$ 二分查找算法，杜绝输入卡顿与延迟；
  - 精美圆角半透明边框呈现，消除光标所在行的遮挡问题。
- **🔍 全工程跨文件转定义 (Go to Definition, F12)**：
  - 深度扫描工程内全部 `.c`、`.cpp`、`.h`、`.hpp` 文件；
  - 精准识别类方法实现、模板函数、内联函数、`#define` 宏定义与结构体字段，支持一键平滑跳转。
- **✏️ 一键符号重命名 (Rename Symbol, F2)** 与 **全工程引用查找 (Find References, Shift+F12)**。
- **🧭 现代化面包屑导航栏 (Breadcrumb Bar)** 与 **多层级函数/类大纲树 (Outline Tree)**。
- **🔎 浮动查找与替换栏 (Find/Replace Widget)**：支持正则表达式、大小写敏感、全词匹配与全文替换。
- **📑 多视口分栏编辑**：支持自由水平分栏与垂直分栏，多文件对照编辑更直观。

---

### 2. 📈 高性能多通道串口调试与实时波形示波器 (SerialPortPlot)
工业级高性能数据吞吐与可视化分析引擎：
- **⚡ 多串口会话并发 (Multi-Session)**：支持同时开启多个物理串口或虚拟串口，独立收发互不干扰；
- **📊 实时动态波形绘制 (Real-Time Plotting)**：
  - 集成高性能 QCustomPlot 绘图核心，支持百万级数据点流畅渲染、平移与无级缩放；
  - 支持多通道曲线自由叠加、隐藏、颜色定制与标记；
- **🔬 频域与统计分析**：内置 FFT 实时频谱分析、均值、方差、峰峰值等信号处理算法；
- **🧩 智能协议解析器 (Protocol Analyzer)**：
  - 支持自定义数据帧头、帧尾、数据类型（`int8/16/32`, `float`, `double`）、字节序（大端/小端）及校验和（CRC16/CRC32/Sum）；
  - 支持将解析出的字段一键绑定到指定波形通道。

---

### 3. 🚗 CAN / CAN FD 总线诊断与 CANopen 协议分析仪 (CAN Tool)
针对车载电子、工业机器人及运动控制总线的专业诊断工具：
- **🔌 多硬件适配**：原生支持周立功 (ZLG) USBCAN、USBCAN-FD、创芯科技 CXCAN 等主流总线适配卡；
- **🚀 CAN FD 变速率通信**：支持高达 8 Mbps 的仲裁段/数据段独立波特率配置与 BRS 变速率传输；
- **🌐 CANopen 主站与对象字典分析 (CANopen Master)**：
  - 完整的 NMT 网络状态管理；
  - SDO 读写、PDO 实时映射与周期通信监控；
  - 支持导入并解析 EDS (Electronic Data Sheet) 电子数据表文件；
- **📊 总线负载率与流量监控 (Bus Utilization)**：实时统计总线帧率、丢包率、错误帧与带宽占用率；
- **🎯 报文捕获与高级过滤**：支持按 ID、数据长度 (DLC)、帧格式进行实时黑白名单过滤与周期循环发送。

---

### 4. 🚀 自动化交叉编译构建与 MCU 固件 IAP 烧录 (Build & IAP)
连接代码与硬件的闭环工具链：
- **⚙️ GNU Arm 交叉编译系统**：
  - 深度集成 `arm-none-eabi-gcc`、`make`、`ninja`、`OpenOCD`；
  - 支持一键编译、增量构建与清理，构建日志实时流式输出并智能高亮编译报错；
- **📋 MCU Profile 芯片档案管理**：便捷配置芯片内核架构（Cortex-M0/M3/M4/M7/H7 等）、Flash/RAM 布局与链接脚本（`.ld` / `.s`）；
- **📥 固件 IAP 在线升级 (In-Application Programming)**：
  - 支持通过串口、CAN 总线或自定义 Bootloader 协议进行固件下载；
  - 支持 `.bin` / `.hex` 格式固件完整性校验与在线进度条实时监控。

---

### 5. 🎛️ 工业级设备大屏监控与自定义仪表控件库 (Device Monitor & Designer)
为上位机展示与工业现场监控提供强大支持：
- **🖥️ 设备监控大屏 (Device Monitor Panel)**：多机联动状态看板，全面监控设备在线状态、关键参数与报警日志；
- **🎨 自定义仪器仪表设计器 (Custom Widget & Designer)**：
  - 内置丰富的工业仪表控件：表盘式仪表、数显屏、LED 指示灯、拨动开关、温度计、水箱液位计等；
  - 支持拖拽式布局与实时数据源绑定。

---

### 6. 🎨 现代设计美学与沉浸式主题系统 (Modern Themes)
- **🌗 经典深色与明亮浅色主题无缝切换**：基于 VS Code / Catppuccin 精心定制的高对比度、护眼配色系统；
- **✨ 细节打磨**：微阴影、平滑呼吸高亮、高精矢量 SVG 图标、平滑分栏拖拽与响应式布局。

---

## 💻 快速上手与构建 (Getting Started)

### 环境要求 (Prerequisites)
- **Qt**：Qt 5.15.2 LTS 或 Qt 6.x (推荐 MinGW 8.1.0 64-bit)
- **编译器**：GCC / MinGW-w64 8.1.0+ / MSVC 2019+
- **交叉编译链 (可选)**：GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)
- **第三方库**：QScintilla 2.11+, QCustomPlot 2.1+ (已内置/支持子模块)

### 本地编译 (Build from Source)

```bash
# 1. 克隆代码仓库
git clone https://github.com/your-username/SmileCodeIDE.git
cd SmileCodeIDE/PhudonTools

# 2. 生成 Makefile
qmake PhudonTools.pro

# 3. 编译构建
mingw32-make -j8   # Windows (MinGW)
# 或者 make -j8    # Linux / macOS
```

---

## 🗂️ 目录结构 (Repository Structure)

```
SmileCodeIDE/
├── PhudonTools/                  # 核心工程源码目录
│   ├── codeeditor.cpp / .h       # 专业代码编辑器 (Sticky Scroll / 符号高亮 / 转定义)
│   ├── serialportplot.cpp / .h   # 串口调试助手与实时波形示波器
│   ├── caninterface.cpp / .h     # CAN / CAN-FD 底层通信接口
│   ├── canopenmaster.cpp / .h    # CANopen 协议栈与主站管理
│   ├── devicemonitorpanel.cpp/.h # 工业设备运行状态监控面板
│   ├── widgetdesigner.cpp / .h   # 工业级自定义控件设计器
│   ├── buildsystem.cpp / .h      # 嵌入式交叉编译构建系统
│   ├── iaptool.cpp / .h          # 固件 IAP 在线升级工具
│   ├── idetheme.h / scopetheme.h # 全局主题配色与调色板
│   ├── USBCANFD/ & CXCAN/        # 周立功 / 创芯 CAN 驱动适配层
│   └── PhudonTools.pro           # Qt 工程配置文件
└── README.md                     # 项目说明文档
```

---

## 🤝 贡献与反馈 (Contributing)

欢迎提交 Issue 与 Pull Request！如果您有任何功能建议、Bug 报告或硬件驱动适配需求，欢迎随时发起讨论。

---

<div align="center">
Made with ❤️ for Embedded Engineers & Makers across the World.
</div>
