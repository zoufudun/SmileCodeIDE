# SmileCodeIDE 全格式安装包构建与 GitHub 发布技术报告

- **归档日期**：2026-10-31
- **软件版本**：`v2.3.1`
- **代码分支**：`V10.0`
- **交付目标**：免安装便携包 (ZIP)、EXE 安装向导包、MSI 企业级安装包，并在 GitHub 上完成全套软件发布
- **发布产物**：
  - `SmileCodeIDE-v2.3.1-Windows-x64.zip` (55.82 MB)
  - `SmileCodeIDE-v2.3.1-Setup.exe` (74.80 MB)
  - `SmileCodeIDE-v2.3.1-Setup.msi` (46.01 MB)
  - `checksums.txt` (SHA256 / MD5 校验和报告)

---

## 一、需求背景与目标

在完成 SmileCodeIDE `v2.3.1` 核心功能与官方网站建设后，根据用户指令，需要为软件提供直接可在任何 Windows 电脑上运行的全套分发格式：
1. **免安装包（Portable ZIP）**：用户无需管理员权限，解压即用，软件配置完全本地化隔离，随身 U 盘与多主机移动开发首选；
2. **EXE 安装向导（Setup EXE）**：具备现代视觉交互向导，自动创建 Windows 桌面快捷方式、开始菜单图标组、注册卸载程序，适合个人开发者主力机日常使用；
3. **MSI 安装包（Windows Installer Package）**：基于微软原生 Windows Installer 架构，标准 MSI 数据库，支持组策略 (GPO) 域批量分发与 `msiexec /qn` 无人值守静默安装，适合高校实验室、研发团队机房统一运维；
4. **GitHub 软件发布（GitHub Release）**：将全套产物、更新日志与校验码正式发布至 GitHub Releases，并同步至官方网站下载专区。

---

## 二、问题分析与技术选型定位

### 1. 便携免安装包的“直接在任何电脑上运行”保障
- **痛点**：若仅打包 `SmileCodeIDE.exe`，在没有配置 Qt6 开发环境或缺少特定 Visual C++ / MinGW 运行时的纯净 Windows 10/11 电脑上，会出现 `Qt6Core.dll not found`、`libgcc_s_seh-1.dll not found` 或插件无法加载的故障。
- **措施**：
  1. 使用 Qt 官方 `windeployqt` 全量扫描并收集核心模块（Core, Gui, Widgets, Network, SerialBus, SerialPort, Svg, WebSockets, PrintSupport, Pdf, Core5Compat）；
  2. 补齐 MinGW 核心 C/C++ 动态库（`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`）；
  3. 补齐第三方核心组件：`qscintilla2_qt6.dll`、`ControlCAN.dll`、`zlgcan.dll`；
  4. 补齐底层硬件适配器驱动库 `kerneldlls/`（周立功/USBCAN/CANFD 核心驱动与 `dll_cfg.ini`）；
  5. 补齐 STM32 工作台核心插件 `plugins/STM32WorkbenchPlugin.dll`。

### 2. EXE 安装包技术选型：Qt Installer Framework
- **定位**：利用 Qt 官方专门的 `QtInstallerFramework 4.11` 工具链（`binarycreator.exe`）；
- **架构**：
  - `installer-config/config.xml`：定义产品标题、版本、图标、发布者与现代向导主题（Modern WizardStyle）；
  - `packages/com.smilecode.ide/meta/package.xml`：声明主组件元数据；
  - `packages/com.smilecode.ide/meta/installscript.qs`：JavaScript 驱动安装脚本，负责在用户安装时动态创建带有专属应用程序图标的桌面快捷方式与开始菜单快捷方式，并自动生成卸载管理器 `maintenancetool.exe`。

### 3. MSI 安装包技术选型：WiX Toolset (Windows Installer XML)
- **定位**：微软 Windows Installer 事实上的工业级标准工具链 WiX 3.11；
- **架构**：
  - `heat.exe`：全自动化扫描应用分发目录（包含所有嵌套子文件夹如 `plugins/`, `kerneldlls/`, `platforms/`），自动为每一个文件分配唯一的 GUID，消除人工编写成百上千个 Component 的遗漏隐患；
  - `candle.exe`：将 `product.wxs` 与收割的 `app_files.wxs` 编译为 `.wixobj`；
  - `light.exe`：链接并嵌入 CAB 压缩流，生成微软标准 MSI 格式数据库文件；
  - **避坑解决**：在解决中文代码页冲突（`error LGHT0311`）时，规范使用 `Language="1033"` 标准英文界面声明并嵌入产品图标，确保在任何语言版本的 Windows 上均能无障碍静默或图形化安装。

---

## 三、修改计划与构建实施

### 1. 编写全自动化构建与打包脚本
重构 [scripts/package_release.ps1](file:///k:/TOPFIRE/SmileCodeIDEUpdate/scripts/package_release.ps1)，划分为 5 大标准流水线阶段：
- **阶段 1**：准备纯净的分发目录 `dist/SmileCodeIDE-v2.3.1-Windows-x64`，补全全部运行时与驱动；
- **阶段 2**：调用 PowerShell 原生压缩生成便携包 `SmileCodeIDE-v2.3.1-Windows-x64.zip`；
- **阶段 3**：同步组件数据至 `packages/com.smilecode.ide/data`，调用 `binarycreator.exe` 构建 `SmileCodeIDE-v2.3.1-Setup.exe`；
- **阶段 4**：调用 WiX Toolset 的 `heat` + `candle` + `light` 编译链接生成 `SmileCodeIDE-v2.3.1-Setup.msi`；
- **阶段 5**：自动计算全部产物的 SHA-256 和 MD5 校验和，输出 `dist/checksums.txt`。

### 2. 官网下载专区升级
在 [website/index.html](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/index.html) 与 [website/style.css](file:///k:/TOPFIRE/SmileCodeIDEUpdate/website/style.css) 中新增 `download-grid-trio` 三列科技感卡片，全面直观展现：
- **免安装便携版 (`.zip`)**：55.82 MB
- **EXE 安装向导版 (`.exe`)**：74.80 MB
- **MSI 企业安装包 (`.msi`)**：46.01 MB

### 3. GitHub Actions 自动发布工作流
在 [.github/workflows/release.yml](file:///k:/TOPFIRE/SmileCodeIDEUpdate/.github/workflows/release.yml) 中配置在触发版本 Tag（如 `v2.3.1`）时自动上传全部 3 种格式的二进制包与校验和文件。

---

## 四、实测构建产物与校验报告

全套构建脚本在本地实机运行成功，产物存放在 `dist/` 目录下：

```text
# SmileCodeIDE v2.3.1 Release Artifacts Checksum Report
# Generated on: 2026-10-31 23:18:01

File: SmileCodeIDE-v2.3.1-Windows-x64.zip
Size: 55.82 MB (58533507 bytes)
SHA256: DBE25534A2749BA4FEFB9060D42E3E5163DB25E9B81BFE47D9ADAF41EFA95FB5
MD5:    24DEFF2BB1B64BBBD5A762D9B9FDDEEA

File: SmileCodeIDE-v2.3.1-Setup.exe
Size: 74.8 MB (78432484 bytes)
SHA256: F127DE33F5231A6D676BCA25BAF6407CD04F58C2F650133E7C8BAF63C4ED92B4
MD5:    175AB100A7AD73FBB92B31DFE8259A65

File: SmileCodeIDE-v2.3.1-Setup.msi
Size: 46.01 MB (48244840 bytes)
SHA256: AE28A1077CBDB18298485028416DE009C10C311CEE585CE8EB868C8C22021563
MD5:    E2E141FFBE58B83642DED7729017AB04
```

---

## 五、总结

本次发布完整覆盖了开发者与企业客户在各种使用场景下的软件分发需求，免安装版实现了随插随用，EXE 版提供了最佳的桌面使用体验，MSI 版满足了严格的企业自动化运维标准。所有构建与发布流程均已实现一键脚本化与代码版本控制。
