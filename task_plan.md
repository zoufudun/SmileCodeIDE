# SmileCodeIDE 专项升级与问题攻坚任务计划 (task_plan.md)

## 一、 标准工作流规范 (Standard Operating Procedure)

根据用户明确要求，从即日起，每次问题解决均严格执行以下标准化闭环流程：

1. **阶段 1：问题分析与定位 (Problem Analysis & Root Cause)**
   - 深入分析用户反馈的故障现象、外部环境与异常日志；
   - 结合源代码、调用栈、底层汇编或硬件调试器（GDB / 寄存器现场）精准锁定根本原因，杜绝主观臆测。
2. **阶段 2：修改计划与方案 (Modification Plan)**
   - 制定严密具体的实施计划，明确涉及的代码文件、预期修改点、风险规避与交付验证标准。
3. **阶段 3：具体修改措施与总结报告 (Implementation & Documentation)**
   - 给出清晰、完整、逐项对照的修改措施与技术原理总结；
   - **严格按照每次具体解决的问题独立命名归档文件**（存放于 `docs/` 目录），方便下载与长期保存。
4. **阶段 4：Git 版本控制与版本号策略 (Git Version Control & Bumping)**
   - **大修正 (Major / Minor)**：涉及重大机制调整、底层架构重构、关键外设/系统故障修复，递增主版本或次版本号（例如 `v2.2.0` $\rightarrow$ `v2.3.0`）；
   - **小修正 (Patch)**：涉及局部 UI 微调、样式优化、特定环境兼容小修，递增补丁版本号（例如 `v2.3.0` $\rightarrow$ `v2.3.1`）；
   - 同步更新项目版本配置（如 `PhudonTools.pro` 的 `VERSION` 与 `plugin.json` 的 `version`）；
   - 提交 Git Commit 并打上对应版本的清晰 Tag 说明。

---

## 二、 阶段执行与闭环记录

### 阶段 1：野火 STM32H743 触摸画板工程烧录后板卡无效果排查与解决【完成】
- **归档文档**：[docs/STM32H7烧写成功但黑屏不运行问题排查与编译优化级别修复.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/STM32H7烧写成功但黑屏不运行问题排查与编译优化级别修复.md)

### 阶段 2：C 语言语法与 GCC 14 宽容度兼容【完成】
- **归档文档**：[docs/GCC14编译报错(语法冲突与未声明函数)兼容性修复.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/GCC14编译报错(语法冲突与未声明函数)兼容性修复.md)

### 阶段 3：UI 主题与体验深度优化【完成】
- **归档文档**：[docs/IDE弹窗主题暗色异色消除与下拉框平滑倒三角重构.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/IDE弹窗主题暗色异色消除与下拉框平滑倒三角重构.md)

### 阶段 4：工作流优化与稳定性防护【完成】
- **归档文档**：[docs/主界面打开项目记忆最近文件夹与视图重复及编译崩溃排查.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/主界面打开项目记忆最近文件夹与视图重复及编译崩溃排查.md)

### 阶段 5：编译器优化等级与高级构建选项可视化设置界面【已完成】
- **归档文档**：[docs/编译器优化等级与高级构建选项可视化设置界面设计与实现.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/编译器优化等级与高级构建选项可视化设置界面设计与实现.md)
- **完成项**：
  1. [x] 在 `MakefileGenerator.h` 与 `MakefileGenerator.cpp` 中定义并实现 `CompilerBuildOptions`，动态组装并注入 `CFLAGS`；
  2. [x] 在 `BuildManager.h` 与 `BuildManager.cpp` 中支持全动态构建选项传递；
  3. [x] 在 `ToolchainConfigDialog` 中实现 Group 4 编译器优化专属面板，支持 `-O1`, `-O0`, `-O2`, `-O3`, `-Os`, `-Og` 细粒度选择与自适应动态说明卡片；
  4. [x] 在 `STM32WorkbenchWidget` 工具栏构建下拉框中扩展细化选项并与设置保持双向同步；
  5. [x] 新增自动化单元测试（8 项测试全 PASS）；
  6. [x] 双主题离线渲染截图实测通过（Dark/Light 视觉完美）；
  7. [x] 版本号由 `v2.3.0` 升级为 `v2.3.1`（插件 `v1.1.1`）。

---

### 阶段 6：SmileCodeIDE 官方网站设计与 GitHub 发布及 Pages 部署【已完成】
- **归档文档**：[docs/SmileCodeIDE官方网站设计与GitHub发布及Pages部署.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE官方网站设计与GitHub发布及Pages部署.md)
- **完成项**：
  1. [x] 深度对齐 [CC Switch](https://ccswitch.io/zh/) 与 [恒基衡迹](https://zoufudun.github.io/hengji/) 视觉系统，完成 `website/`（index.html, style.css, app.js, assets/）；
  2. [x] 制作并完成 Windows 绿色便携包一键打包脚本（`scripts/package_release.ps1`），测试生成 `dist/SmileCodeIDE-v2.3.1-Windows-x64.zip` (36.41 MB)；
  3. [x] 编写正式 Release 说明文档 `RELEASE_NOTES_v2.3.1.md`，配置 `.github/workflows/release.yml`；
  4. [x] 编写 GitHub Pages 自动化部署工作流 `.github/workflows/deploy-pages.yml`；
  5. [x] 本地提交、推送至远程仓库 `origin/V10.0`，并建立 `gh-pages` 独立分支推送到 `origin/gh-pages` 实现多通道 Pages 部署。

---

### 阶段 7：免安装包、EXE 安装包与 MSI 安装包全格式打包与 GitHub 发布【已完成】
- **归档文档**：[docs/SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布.md)
- **完成项**：
  1. [x] 构建直接在任何电脑运行的免安装绿色包 `dist/SmileCodeIDE-v2.3.1-Windows-x64.zip` (55.82 MB)，集成 Qt6、MinGW 运行时、全部外设驱动与动态库；
  2. [x] 基于 Qt Installer Framework 构建带图形向导、桌面快捷方式与卸载器的 `dist/SmileCodeIDE-v2.3.1-Setup.exe` (74.80 MB)；
  3. [x] 基于 WiX Toolset 构建标准微软 Windows Installer `dist/SmileCodeIDE-v2.3.1-Setup.msi` (46.01 MB)，支持企业域批量与静默安装；
  4. [x] 自动生成 SHA-256 和 MD5 校验和报告 `dist/checksums.txt`；
  5. [x] 更新官网下载专区，提供全套三款安装包的下载卡片与校验和展示；
  6. [x] 更新 GitHub Release 说明与自动化工作流，同步推送至 GitHub 远程仓库与 `gh-pages` 分支。

---

### 阶段 8：EXE 安装向导修复、客户端关于更新中心、双主题官网与全插件主页矩阵【进行中】
- **归档文档**：[docs/SmileCodeIDE软件更新中心与双主题官网及全插件展示主页设计.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE软件更新中心与双主题官网及全插件展示主页设计.md)
- **核心目标**：
  1. [x] 修复 EXE 安装向导权限问题（目标路径设为免提权用户目录，修复快捷方式编码，配置安装完成自动启动）；
  2. [x] 实现客户端“关于与软件更新中心”（包含最外层应用市场容器更新、GitHub Star 引导卡片、各插件运行状态与 Changelog），并成功编译集成到 `PhudonTools.exe`；
  3. [x] 官网主页全面对标 CC Switch 风格（图 1 简约突出与拟物容器窗体、图 2 右上角三胶囊、图 4 导航产品 Mega Menu）；
  4. [x] 实现极客暗黑与暖羊皮纸（warm parchment）双主题引擎切换与本地持久化；
  5. [x] 建立 5 大核心插件专属独立主页（`stm32-workbench.html`、`oscilloscope.html`、`can-analyzer.html`、`uds-diagnostic.html`、`serial-assistant.html`），各具动态视觉展台（图 5 风格）；
  6. [ ] 重新生成全套分发包（ZIP、EXE、MSI）并更新校验哈希；
  7. [ ] 推送 GitHub 仓库 `V10.0`、更新 `v2.3.1` Release 与 `gh-pages` 部署。

## 2026-11-05 Keil 工程导航
- [x] 有序 uvprojx 解析与回归测试
- [x] 项目/函数双视图与跳转
- [x] 主窗口导入、持久化与刷新集成
- [x] 编译、回归测试、代码审查与归档

## 2026-11-05 Debug 启动崩溃
- [x] 开启 Qt 断言的最小编辑器启动测试复现
- [x] 修复 lambda 使用 UniqueConnection，并避免主题切换重复连接
- [x] 重建 Qt Creator Debug 目标、运行回归、真实入口验证并归档
