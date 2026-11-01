# SmileCodeIDE v2.3.1 正式发布说明 (Release Notes)

🎉 **SmileCodeIDE v2.3.1** 是面向嵌入式与单片机开发者的重大稳定版本！
本次发布正式提供**全套 Windows 64-bit 安装包套件**，支持任何电脑直接解压运行的**绿色免安装包**、支持桌面快捷方式与卸载向导的 **EXE 安装包**，以及支持企业静默分发的微软标准 **MSI 安装包**！

---

## 📦 官方软件发布产物与下载指引 (Release Packages)

所有产物均已完成独立环境完整性验证，内置完整 Qt6 运行时、MinGW 运行时、OpenOCD 驱动与硬件协议支持：

| 安装包类型 | 文件名称 | 格式特性 | 体积 | SHA-256 校验和 |
| :--- | :--- | :--- | :--- | :--- |
| **免安装便携版** | `SmileCodeIDE-v2.3.1-Windows-x64.zip` | 解压即用，配置隔离，随身U盘首选 | **55.86 MB** | `D2CD0BBEF07FD77213E9FC712D4B701C2414F80AB3F69C4CAAB21DD9C535C263` |
| **EXE 安装向导** | `SmileCodeIDE-v2.3.1-Setup.exe` | 图形化安装向导，已修复权限与快捷方式，默认安装至用户目录免提权，附带卸载工具 | **74.91 MB** | `F894887E41307E6C981A1F10E99D88E6F9EE20F790630F881C68B328688B6911` |
| **MSI 企业安装包** | `SmileCodeIDE-v2.3.1-Setup.msi` | 微软标准 Windows Installer，支持 `msiexec /qn` 静默安装与 GPO 组策略域分发 | **46.85 MB** | `9CDD6376422C9A4096B851AAE8C597F936393509A81417BD8C9CC33C72EE39EA` |

> [!TIP]
> - **个人开发/临时排查**：推荐下载 **免安装便携版 (`.zip`)**，无需管理员权限，解压后双击 `SmileCodeIDE.exe` 即可直接使用；
> - **日常主力开发**：推荐下载 **EXE 安装向导 (`.exe`)**，享受原生快捷方式与优雅的暗黑桌面启动图标；
> - **实验室机房/团队统一环境**：推荐使用 **MSI 安装包 (`.msi`)**，一行命令实现批量无人值守静默分发：
>   ```cmd
>   msiexec /i SmileCodeIDE-v2.3.1-Setup.msi /qn
>   ```

---

## 🌟 核心更新亮点 (Highlights)

### 1. ⚙️ 全新可视化编译器优化与高级构建选项控制台
- **六大标准优化挡位**：支持 `-O0`（无优化单步调试）、`-O1`（基础优化）、`-O2`（官方推荐平衡优化）、`-O3`（极致性能算力优先）、`-Os`（极小代码尺寸）、`-Og`（调试友好优化）；
- **高级符号与兼容开关**：
  - 勾选 `启用 DWARF2 调试符号 (-g -gdwarf-2)`：确保断点定位毫秒级响应、局部变量结构完整；
  - 勾选 `宽容编译模式 (-fcommon -fpermissive)`：容忍经典旧库多重定义与隐式函数声明；
- **智能动态说明卡片**：UI 界面根据所选优化等级，实时解析技术特点、适用场景并展示生成的 GCC `CFLAGS`；
- **配置持久化**：通过 `QSettings` 本地自动保存，多工程与再次启动无感延续；主工具栏下拉菜单与弹窗双向实时同步。

### 2. ⚡ STM32 硬件延时防死锁机制深度根治
- 深入解决从 Keil 迁移到 GCC 环境时，因编译器将空循环 `for (i=0; i<delay; i++);` 判定为无副作用而直接优化清空导致的“板卡假死不亮灯”顽疾；
- 梳理推荐硬件级 SysTick / DWT 周期延时与 `volatile` 规范，实现高优化等级与稳定执行两全。

### 3. 🛡️ GCC 14 现代编译错误深度根因分析与兼容修复
- 解决从 Keil 迁移代码在 GCC 14 中遭遇的严格语法报错：
  - `invalid suffix "-0x8047" on integer constant`：规范数字常数与负号操作符空格；
  - `implicit declaration of function 'LCD_DispString_EN_CH'`：补齐头文件显式声明，并配备 `-fcommon` 编译后备兜底。

### 4. 🎨 Qt 全局暗色主题异色重构与控件覆盖修复
- 彻底解决暗黑模式下 `QCheckBox`、`QRadioButton` 等基础控件由于层叠样式继承冲突产生的刺眼白色背景方块；
- 统合 `Dark`、`OneDark`、`Solarized`、`Light` 四大官方主题，打造沉浸式极客编码环境。

### 5. 🔍 UI 细节打磨：下拉指示器与路径记忆
- **下拉框横线缺陷修复**：重写 `QComboBox::drop-down` 样式表，使用矢量路径自绘制高保真倒三角，消除黑色短横线瑕疵；
- **最近打开目录记忆**：点击【打开项目】自动记忆上次成功加载的工程文件夹，告别每次反复从磁盘根目录翻找。

### 6. 🌐 官方网站上线与 GitHub Pages 自动化部署
- 正式建立高颜值、深色暗黑流光科技风格的官方网站（设计语言对齐 [CC Switch](https://ccswitch.io/zh/) 与 [恒基衡迹](https://zoufudun.github.io/hengji/)）；
- 在线访问地址：[https://zoufudun.github.io/SmileCodeIDE/](https://zoufudun.github.io/SmileCodeIDE/)；
- 配套全自动化 GitHub Actions 工作流（`.github/workflows/deploy-pages.yml` 与 `release.yml`）。

---

## 📚 专项技术排查与设计归档文档

本版本所有技术改造与排查均形成了严谨的独立命名的 Markdown 归档文档，欢迎查阅：
1. [《STM32软硬件开发中延时死锁与编译器优化等级关联排查》](docs/STM32软硬件开发中延时死锁与编译器优化等级关联排查.md)
2. [《GCC 14编译报错深度根因分析与兼容性修复》](docs/GCC%2014编译报错深度根因分析与兼容性修复.md)
3. [《编译器优化等级与高级构建选项可视化设置界面设计与实现》](docs/编译器优化等级与高级构建选项可视化设置界面设计与实现.md)
4. [《Qt样式表全局暗色主题异色重构与控件覆盖修复》](docs/Qt样式表全局暗色主题异色重构与控件覆盖修复.md)
5. [《下拉框横线缺陷排查与倒三角样式修复》](docs/下拉框横线缺陷排查与倒三角样式修复.md)
6. [《工程项目快速打开与最近目录记忆功能实现》](docs/工程项目快速打开与最近目录记忆功能实现.md)
7. [《SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布》](docs/SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布.md)

---

## 💻 系统要求与前置驱动

- **操作系统**：Windows 10 / Windows 11 (64-bit)
- **编译工具链**：arm-none-eabi-gcc (建议 GCC 10 或更高，原生兼容 GCC 14)
- **烧录器驱动**：使用 ST-Link 时，请通过开源驱动工具 **Zadig** 将调试器接口替换为 `WinUSB` 或 `libusb-1.0`，以确保 OpenOCD 顺畅通信。

---

感谢所有嵌入式开发者的支持与反馈！
如有任何建议或问题，欢迎提交 [GitHub Issues](https://github.com/zoufudun/SmileCodeIDE/issues)。
