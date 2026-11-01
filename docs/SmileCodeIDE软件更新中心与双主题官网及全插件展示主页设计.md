# SmileCodeIDE 软件更新中心与双主题官网及全插件展示主页设计与实现

**文档版本**：v2.3.1  
**归档日期**：2026-11-01  
**适用分支**：`V10.0` / `gh-pages`  
**关联代码仓库**：https://github.com/zoufudun/SmileCodeIDE.git  
**官方主页预览**：https://zoufudun.github.io/SmileCodeIDE/  

---

## 目录
1. [问题分析与定位](#一问题分析与定位)
2. [修改计划与架构设计](#二修改计划与架构设计)
3. [具体修改措施与技术落地](#三具体修改措施与技术落地)
   - [3.1 EXE 安装向导权限与快捷方式修复](#31-exe-安装向导权限与快捷方式修复)
   - [3.2 客户端“关于与软件更新中心”实现](#32-客户端关于与软件更新中心实现)
   - [3.3 官网双主题引擎与对标设计](#33-官网双主题引擎与对标设计)
   - [3.4 全插件独立介绍主页矩阵](#34-全插件独立介绍主页矩阵)
4. [全格式安装包发布与校验](#四全格式安装包发布与校验)
5. [Git 版本控制与归档总结](#五git-版本控制与归档总结)

---

## 一、问题分析与定位

根据用户最新提出的综合反馈与需求，本次任务核心聚焦于以下四个维度的痛点与功能演进：

### 1.1 EXE 安装包报错与权限痛点定位
- **现象**：之前基于 Qt Installer Framework (QtIFW) 生成的 `SmileCodeIDE-Setup.exe` 在非特权普通用户双击运行时容易报错或安装失败。
- **根因分析**：
  1. **安装路径越权**：原 `config.xml` 中配置的默认目标路径为 `@ApplicationsDir@/SmileCodeIDE`（即系统盘 `C:\Program Files\SmileCodeIDE`）。普通用户无管理员权限直接写入系统目录时，被 Windows UAC 机制直接拒绝；
  2. **脚本参数编码异常**：`installscript.qs` 中的快捷方式脚本在特定环境下传参出现非 ASCII 乱码中断；
  3. **缺少运行引导**：安装向导完成后未设置 `<RunProgram>`，用户无法在向导末页一键启动软件。

### 1.2 客户端关于界面缺乏应用容器与各插件更新状态
- **现状**：原有软件关于界面仅是一个简单的弹窗，未体现 SmileCodeIDE“应用市场容器 + 多插件”的现代化架构；
- **需求定位（对标用户参考图 3）**：
  1. **最外层容器更新卡片**：展示容器图标、当前版本号（v2.3.1）、一键【🔄 检查更新】按钮，以及【官方网站】、【更新日志】、【近期更新】跳转快捷键；
  2. **GitHub Star 激励引导**：嵌入横幅引导：“⭐ 如果 SmileCodeIDE 帮到了你，请在 GitHub 点个 Star，这对我们意义重大”，附带【去点 Star】按钮与关闭按钮；
  3. **各插件独立更新与运行状态**：按列表分别展示 STM32 开发工作台、多通道虚拟示波器、CAN 分析仪、UDS 汽车诊断套件、多协议串口助手等核心插件的实时运行状态（● 运行中 / ● 已就绪）与插件版本；
  4. **详细更新记录选项卡**：提供 Markdown/文本渲染的详细 Changelog 视窗。

### 1.3 官网界面需要全面对标 CC Switch 风格并增加暖羊皮纸主题
- **需求定位（对标用户参考图 1、图 2、图 4、图 5）**：
  1. **主页首屏（图 1 风格）**：简约而突出，左侧为版本胶囊、主标题、标语、赤陶橙主下载按钮及次级文档按钮；右侧展示应用市场容器的拟物实机窗口，重点展现外层容器调度各个插件；
  2. **双主题引擎（图 1 & 图 2）**：保留原有极客暗黑主题，新增**暖羊皮纸（warm parchment）主题**（暖米杏质感底色 `#f4efe6`、纯白容器卡片、赤陶橙聚焦色），支持一键无感切换并持久化；
  3. **右上角操作区（图 2 风格）**：采用三个圆角胶囊组件（`🌐 CN` 语言胶囊、🌙/☀️ 主题切换胶囊、GitHub 猫咪图标按钮）；
  4. **顶部导航栏（图 4 风格）**：增加【产品 ▾】多列 Mega Menu 下拉菜单，分类展示各个插件套件；
  5. **各插件独立主页（图 5 风格）**：为各个核心插件开设专属主页，居中呈现标语、操作按钮以及动态软件界面展台。

---

## 二、修改计划与架构设计

```mermaid
flowchart TD
    A[用户综合升级需求] --> B[安装包排查与修复]
    A --> C[客户端关于与更新中心]
    A --> D[官网双主题与全插件主页]
    A --> E[打包流水线与发布]

    B --> B1[调整安装路径为用户目录无需提权]
    B --> B2[修复 installscript.qs 快捷方式]
    B --> B3[配置安装完毕立即运行]

    C --> C1[设计 AboutUpdateDialog 类]
    C --> C2[集成容器更新与 Star 横幅]
    C --> C3[集成各插件状态与 Changelog]
    C --> C4[AppHubWindow 与 MainWindow 接入]

    D --> D1[双主题 CSS 引擎: Dark + Warm Parchment]
    D --> D2[右上角三胶囊与导航产品 Mega Menu]
    D --> D3[主页 Hero 呈现外层容器拟物界面]
    D --> D4[5 个插件独立页面动态界面与特性]

    E --> E1[生成免安装绿色 ZIP]
    E --> E2[编译免提权安装 EXE]
    E --> E3[构建企业部署 MSI]
    E --> E4[GitHub Pages 部署与 Release 发布]
```

---

## 三、具体修改措施与技术落地

### 3.1 EXE 安装向导权限与快捷方式修复
针对 QtIFW 配置文件进行关键重构：
1. **修改 [installer-config/config.xml](file:///k:/TOPFIRE/SmileCodeIDEUpdate/installer-config/config.xml)**：
   - 将 `<TargetDir>` 设为自适应用户目录 `@HomeDir@/AppData/Local/Programs/SmileCodeIDE`，普通双击即可免提权秒级安装；
   - 增加 `<RunProgram>@TargetDir@/SmileCodeIDE.exe</RunProgram>` 与 `<RunProgramDescription>启动 SmileCodeIDE 智能开发工作台</RunProgramDescription>`，向导结束时可直接启动软件。
2. **修改 [packages/com.smilecode.ide/meta/installscript.qs](file:///k:/TOPFIRE/SmileCodeIDEUpdate/packages/com.smilecode.ide/meta/installscript.qs)**：
   - 修复快捷方式创建逻辑，规避系统语言和字符编码干扰。

### 3.2 客户端“关于与软件更新中心”实现
创建专属的 `AboutUpdateDialog` 界面类：
1. **头文件 [PhudonTools/aboutupdatedialog.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/aboutupdatedialog.h)**：
   - 包含主容器更新卡片、Star 激励横幅、插件表格组件、更新日志文本框与各项网络跳转槽函数；
2. **实现文件 [PhudonTools/aboutupdatedialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/aboutupdatedialog.cpp)**：
   - 深度还原参考图 3 的卡片排版与圆角阴影；
   - 动态列举“STM32 全功能工作台”、“多通道虚拟示波器”、“CAN / CAN-FD 分析仪”、“UDS 汽车诊断套件”、“多协议串口助手”当前版本与实时运行状态；
   - 点击“检查更新”发起实时比对；点击“去点 Star”自动调起系统浏览器访问 GitHub 仓库；
3. **主窗口挂载**：
   - 在 [PhudonTools/apphubwindow.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/apphubwindow.cpp) 的 `onShowAboutClicked()` 与 [PhudonTools/mainwindow.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/mainwindow.cpp) 的 `showAboutDialog()` 中统一替换为调用 `AboutUpdateDialog`；
   - 成功使用 MinGW 13.1.0 重新编译生成最新 `PhudonTools.exe`（7.52MB）。

### 3.3 官网双主题引擎与对标设计
在 [website/style.css](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/style.css) 与 [website/app.js](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/app.js) 中实现双主题切换：
1. **双主题 CSS 变量定义**：
   - `body.dark-theme`：背景 `#070a12`，深邃黑晶微光，极客紫色高光；
   - `body.parchment-theme`：背景 `#f4efe6`（暖米杏色），纯白卡片 `#ffffff`，赤陶橙按键 `#ea580c`，精准还原 CC Switch 质感；
2. **右上角三胶囊组件（图 2 风格）**：
   - `🌐 CN`、🌙/☀️ 主题切换胶囊、🐱 GitHub 胶囊；
3. **顶部导航栏【产品 ▾】Mega Menu（图 4 风格）**：
   - 悬浮展开两列卡片：核心研发套件（STM32 工作台、虚拟示波器、串口助手）与车载总线分析（CAN/CAN-FD 分析仪、UDS 汽车诊断套件、编译器优化中心）；
4. **主页 Hero 拟物展示窗体（图 1 风格）**：
   - 左侧为版本药丸 `v2.3.1`、主标题、标语、赤陶橙下载按钮与文档入口；
   - 右侧直观呈现外层应用市场容器的管理界面，展示关于更新卡片、Star 引导与各插件的实时运行卡片。

### 3.4 全插件独立介绍主页矩阵与容器市场 9 大插件对齐
严格按照实际软件界面（图 1 真实界面）对齐 **SmileCode Studio（微笑代码工作站）** 容器应用市场所包含的真实 9 大核心套件：
1. **CAN 总线工作台 Pro** (`v2.2.0`)：ZLG / CXCAN 工业级 CAN/CANFD 与 CANOpen 调试；
2. **STM32 IAP 固件升级** (`v1.8.0`)：串口与网络双通道 Bootloader 固件升级与校验；
3. **网络通信调试助手** (`v2.0.0`)：TCP Client/Server 与 UDP 单播/广播/多播高并发调试；
4. **多接口数字示波器** (`v2.0.0`)：串口/TCP/UDP/WebSocket 实时多通道波形采集与 FFT 频谱；
5. **串口调试大师 Pro** (`v3.0.0`)：多标签页高速串口通信、多窗口分屏、实时曲线与 Modbus；
6. **高频信号发生器** (`v1.0.0` · Qt 动态插件)：正弦波/方波/三角波/锯齿波/白噪声实时高频仿真；
7. **SmileCode IDE** (`v2.3.1` · 核心工作台)：嵌入式 C/C++ 代码编辑器与 GDB 调试器；
8. **STM32 编译构建与 GDB 调试** (`v1.0.0` · Qt 动态插件)：GCC/Make 一键构建、固件烧录与硬件在线断点调试；
9. **自定义扩展插件** (`v1.0.0` · Qt 动态插件)：开箱即用的 Qt 动态插件开发范例与接口演示。

并在 `website/plugins/` 目录下全部建立 9 个独立专属介绍主页（如 [can-workbench.html](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/plugins/can-workbench.html)、[digital-oscilloscope.html](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/plugins/digital-oscilloscope.html)、[serial-master.html](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/plugins/serial-master.html) 等），支持双主题无缝切换与动态仿真视窗。

---

## 四、全格式安装包发布与校验

运行自动化发布流水线 [scripts/package_release.ps1](file:///k:/TOPFIRE/SmileCodeIDEUpdate/scripts/package_release.ps1) 生成的三大格式软件包规格如下：

| 软件包文件名称 | 格式类型 | 适用场景 | 架构 | 体积 |
| :--- | :--- | :--- | :--- | :--- |
| `SmileCodeIDE-v2.3.1-Windows-x64.zip` | 绿色免安装包 | 解压即用、U 盘随身携带、不改注册表 | x64 | 55.86 MB |
| `SmileCodeIDE-v2.3.1-Setup.exe` | 标准安装向导 | 桌面用户一键安装、自动创建桌面快捷方式、免提权 | x64 | 74.91 MB |
| `SmileCodeIDE-v2.3.1-Setup.msi` | 统一企业部署包 | 微软标准 Windows Installer、支持 GPO/Intune 静默推送 | x64 | 46.85 MB |

所有安装包的完整 SHA-256 与 MD5 校验和自动保存在 `dist/checksums.txt` 中。

---

## 五、Git 版本控制与归档总结

1. **版本标签**：`v2.3.1`
2. **推送目标**：
   - 主干工作分支：`origin/V10.0`
   - 官方官网部署分支：`origin/gh-pages`（自动触发 GitHub Pages 静态网站上线）
3. **成果总结**：
   - 彻底解决了 EXE 安装包因系统权限导致的安装阻断；
   - 客户端软件与官方网站形成了统一的“应用市场容器 + 插件生态”架构；
   - 官方主页与各个插件独立页面全面实现了高质感、双主题无缝切换的现代化视觉标准。

