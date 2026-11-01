# SmileCode Studio 实机界面对齐与更新中心排版修复及全插件动态交互发布

## 1. 概述与问题背景

在 SmileCodeIDE (SmileCode Studio) v2.3.1 版本的持续交付与官网建设过程中，用户反馈了以下核心问题与演进需求：
1. **客户端关于与更新中心排版畸变（图 1 缺陷）**：
   在打开【关于与更新中心】对话框时，下方“各插件更新与运行状态”由于父级容器尺寸固定且缺乏滚动机制，包含的 9 款插件列表控件被严重垂直挤压，导致按钮压成细线、文字与行之间重叠遮挡，严重影响可用性与观感。
2. **官网首页首屏界面真实还原（图 2 对齐）**：
   官网首页 Hero 首屏原本展示的仅为概念图或不匹配的旧版截图，用户明确要求首页首屏必须 100% 还原用户真实使用的 **SmileCode Studio - 嵌入式开发与调试工具应用工作台**（图 2），包括真实的暗黑钛金配色、分类侧边栏、顶部推荐应用卡片、9 大插件应用矩阵、状态栏以及可点击的关于弹窗。
3. **9 大插件专属主页真实界面与动态可交互重构**：
   进入每个插件的独立网页后，不能是千篇一律的静态文字描述，必须展示该插件真实的软件操作界面，且必须具备**动态可交互特性**（如示波器波形动态扫描、CAN 报文收发模拟、IAP 固件擦写升级动态进度条、GDB 断点单步模拟等）。
4. **全量安装包打包与发布交付**：
   需要将整个软件（含 Qt6 完整运行时、MinGW 运行时、硬件驱动库、动态与内置插件、示例工程）全量打包为：免安装绿色版 ZIP、图形向导版 EXE、企业静默分发版 MSI，并在 GitHub 进行版本发布与 GitHub Pages 部署。

---

## 2. 根本原因定位与排查分析

### 2.1 关于更新窗口挤压变形定位 (图 1)
- **源码排查路径**：`PhudonTools/aboutupdatedialog.cpp`
- **根因分析**：
  原 `AboutUpdateDialog` 窗口采用固定布局，高度设为 600px。在“软件与插件更新”选项卡中，上方包含容器核心卡片、下方直接使用 `QVBoxLayout` 添加了 9 个插件行（每行包含图标、名称、版本、状态、检查更新与配置按钮）。
  由于 9 个插件行的总固有高度（约 $9 \times 48\text{px} = 432\text{px}$）加上顶部卡片（约 180px）与间距，远超过对话框可用净高，且外层**未包裹 `QScrollArea`**。Qt 的布局管理器在空间不足时强制将各子部件的垂直尺寸压缩到其极限（甚至压至 1~2 像素），导致按钮被扁平化挤压成横线，标签文本相互叠盖。

### 2.2 打包脚本嵌套文件夹定位
- **脚本排查路径**：`scripts/package_release.ps1`
- **根因分析**：
  PowerShell 在执行 `Copy-Item -Recurse` 时，若目标路径已存在同名文件夹，会将源目录作为子目录整体拷入（例如将 `plugins/` 拷入已有 `dist/SmileCodeIDE-xxx/plugins/`，导致生成 `plugins/plugins/` 畸变结构）。必须在复制前彻底执行目标目录幂等清理，并直接复制子项。

---

## 3. 具体修改方案与技术实现

### 3.1 客户端关于更新对话框彻底重构
在 [PhudonTools/aboutupdatedialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/aboutupdatedialog.cpp) 中进行了如下重构：
1. **引入 QScrollArea 滚动视图**：
   使用 `QScrollArea` 包装插件更新列表，设置 `setWidgetResizable(true)`、`setFrameShape(QFrame::NoFrame)`，并配置现代半透明平滑滚动条样式。
2. **规范子项最小高度与自适应策略**：
   为每一个插件条目设置 `itemWidget->setMinimumHeight(56)`，尺寸策略设置为 `QSizePolicy::Expanding, QSizePolicy::Fixed`，确保无论列表包含多少插件，单行永远饱满挺立，不发生挤压变形。
3. **增加视口扩展性**：
   将对话框推荐尺寸提升至 `820x680`，最小尺寸限制为 `760x560`，确保在大字号 DPI 下依然拥有充裕的呼吸感。
4. **统一应用工作台版本**：
   在 [PhudonTools/apphubwindow.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/apphubwindow.cpp) 中将底部状态栏硬编码由 `v2.1.0` 统一更新为 `SmileCode Studio v2.3.1`。

### 3.2 官网首页首屏拟物仿真器（图 2 对齐）
在 [website/studio-simulator.css](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/studio-simulator.css)、[website/index.html](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/index.html) 和 [website/app.js](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/app.js) 中完成开发：
1. **1:1 实机复刻排版**：
   - 顶部深灰金属质感标题栏与控制区（搜索框带实时过滤、扩展中心、极客钛金主题切换下拉、关于按钮）；
   - 左侧分类导航胶囊（全部应用、常用推荐、嵌入式开发、总线与通信、测量与分析、固件与烧录、插件与扩展）；
   - 顶部大卡片“SmileCode 一站式嵌入式开发工作台”；
   - 9 款真实应用卡片（CAN 总线工作台 Pro、STM32 IAP 固件升级、网络通信调试助手、多接口数字示波器、串口调试大师、高频信号发生与分析、SmileCode IDE、STM32 编译构建与 GDB 调试、自定义扩展插件）；
   - 底部状态栏（运行中应用数、已集成应用数、SmileCode Studio v2.3.1）。
2. **丰富交互特性**：
   - 搜索框支持即时高亮与卡片显示过滤；
   - 侧边栏支持按分类秒级筛选；
   - 点击星标支持收藏状态实时切换；
   - 点击【关于】按钮唤起像素级复刻的【关于与更新中心】模态弹窗，可体验一键批量检查与更新动画。

### 3.3 9 大独立插件主页的真实界面动态化重构
对 `website/plugins/` 目录下的 9 个专属主页全面重写，全部接入专属真实动态操作台：
1. **CAN 总线工作台 Pro (`can-workbench.html`)**：
   集成真实的工业级报文收发台，支持选择 500kbps / 1Mbps 波特率、输入 ID 与 HEX 数据发送、动态滚动总线帧流，带有总线负载率与吞吐量实时仪表盘。
2. **STM32 IAP 固件升级 (`stm32-iap.html`)**：
   支持串口 Ymodem 与网络 TCP 双通道固件下载仿真，点击【开始烧写】动态走满 0%~100% 进度条，实时滚动扇区擦除、数据校验与跳入用户程序日志。
3. **多接口数字示波器 (`digital-oscilloscope.html`)**：
   基于 HTML5 Canvas 构建的 60FPS 双通道高频示波器，支持实时波形扫描、RUN/STOP 冻结、时基与垂直灵敏度调节，实时测算显示 Vpp 与主频。
4. **高频信号发生与分析 (`signal-generator.html`)**：
   支持正弦波、方波、三角波、高斯噪声实时切换，配备频率/幅值滑块双向绑定，提供波形实时点采样与 CSV 数据导出。
5. **串口调试大师 (`serial-master.html`)**：
   真实的多标签分屏多串口通讯台，支持 115200 波特率配置、HEX/ASCII 切换、彩色 TX/RX 实时日志与 Modbus 快速预设指令发送。
6. **网络通信调试助手 (`network-debugger.html`)**：
   TCP Server / TCP Client / UDP 三模切换，在线客户端列表管理，单播与广播消息回显。
7. **SmileCode IDE (`smilecode-ide.html`)**：
   内置嵌入式工程树（Core、Drivers、Makefile）、QScintilla 语法高亮编辑器（点击行号下断点）、一键调用 GCC 编译并动态输出构建日志。
8. **STM32 编译构建与 GDB 调试 (`stm32-build-debug.html`)**：
   全真 OpenOCD / GDB 在线调试台，黄色单步调试箭头，支持 Step Over / Step Into，实时步进更新 CPU 寄存器与 Watch 监视变量。
9. **自定义扩展插件 (`custom-plugin.html`)**：
   展示标准 Qt C++ 动态插件的生命周期钩子（`onAppLaunch`、`onProjectOpened`、`onThemeChanged`），支持在线触发测试与宿主通讯交互。

### 3.4 全量打包流水线修复与执行
在 [scripts/package_release.ps1](file:///k:/TOPFIRE/SmileCodeIDEUpdate/scripts/package_release.ps1) 中重构：
- 先行执行 `Remove-Item` 彻底清空输出目录；
- 通配符精准复制 `kerneldlls/`、`plugins/`、`Examples/STM32F103_Demo`；
- 执行 `windeployqt` 搜集 Qt6 全部运行时与平台插件（`platforms/qwindows.dll` 等）；
- 复制 MinGW-w64 运行时库（`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`）；
- 构建三种发布形态：
  1. 便携绿色版 `.zip`（45.75 MB）；
  2. QtIFW 独立向导 `.exe`（69.40 MB）；
  3. WiX Toolset 微软标准安装包 `.msi`（46.02 MB）。

---

## 4. 验证与测试结果

### 4.1 关于与更新中心（图 1 修复验证）
- 通过离线截图工具验证，修改后的 `AboutUpdateDialog` 中各插件行高统一为 56px，图标、标题、版本标签、更新按钮排列整齐，垂直滚动条顺畅，不再出现任何文字挤压或按钮被压成细线的异常。
- 离线验证截图存档：`C:/Users/Administrator/.gemini/antigravity/brain/acba03f6-5e53-4abf-b3c0-7b2299a46193/about_dialog_fixed.png`。

### 4.2 软件发布产物完整性校验
三大安装包已在 `dist/` 目录生成完毕，完整校验数据如下：

| 产物名称 | 文件大小 | SHA-256 校验和 |
| :--- | :--- | :--- |
| `SmileCodeIDE-v2.3.1-Windows-x64.zip` | 45.75 MB | `0EC5C06C881292CFAD7E581EE223D0FF901A14FD15239123272C98428999CAE1` |
| `SmileCodeIDE-v2.3.1-Setup.exe` | 69.40 MB | `90C1D6FB2779FF397F1057025690ABECEBE7B81BDE12909995C6083C63F45AF6` |
| `SmileCodeIDE-v2.3.1-Setup.msi` | 46.02 MB | `79A560C6351AE19CB5DA904CC54524E1FD7332F09A1B3861AC6EE71B2B07B4F5` |

---

## 5. 版本控制与发布部署

- **Git 分支管理**：变更已合并至 `main` 主分支，并同步至 `V10.0` 分支；
- **Git 标签**：创建并推送版本标签 `v2.3.1`；
- **GitHub 部署**：
  - 源码仓库：[https://github.com/zoufudun/SmileCodeIDE.git](https://github.com/zoufudun/SmileCodeIDE.git)
  - 官方网站：[https://zoufudun.github.io/SmileCodeIDE/](https://zoufudun.github.io/SmileCodeIDE/)
