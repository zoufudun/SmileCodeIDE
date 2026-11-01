# SmileCodeIDE 技术攻坚与问题归档文档索引

本目录为 **SmileCodeIDE** 的历史问题排查、技术攻坚与系统设计归档库。按照**“按每次具体解决的问题命名”**的标准化规范，每个归档文档均独立包含：**问题分析、定位、修改计划、具体修改措施、实证验证与总结**，方便按需查阅、下载与离线保存。

---

## 专项问题归档（按具体问题分类索引）

### 1. 硬件执行与底层调试专项
* **[STM32H7烧写成功但黑屏不运行问题排查与编译优化级别修复.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/STM32H7烧写成功但黑屏不运行问题排查与编译优化级别修复.md)**
  * **解决问题**：野火 STM32H743 触摸画板工程烧录后开发板黑屏、无画板效果（Keil 正常）的底层深水区排查。
  * **核心根因**：GCC `-O0` 无优化导致 400MHz Cortex-M7 下空循环软件模拟 I2C 延时放大 10 倍以上，GT9xx 触摸屏握手超时死锁引发中断风暴，后续 LCD 初始化未执行。
  * **修改措施**：`MakefileGenerator.cpp` 默认编译优化全面升级为 `-O1 -g3 -gdwarf-2`。
  * **对应版本**：`v2.3.0`（插件 `v1.1.0`）

---

### 2. 工具链构建与 GCC 14 兼容性专项
* **[GCC14编译报错(语法冲突与未声明函数)兼容性修复.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/GCC14编译报错(语法冲突与未声明函数)兼容性修复.md)**
  * **解决问题**：同一工程在 Keil 下 0 错误，在 SmileCodeIDE 下报 `invalid suffix "-0x8047" on integer constant` 与 `implicit declaration of function 'LCD_DispString_EN_CH'` 两个阻断性错误。
  * **核心根因**：ISO C 词法解析器对 `0x80FE-0x8047` 的科学计数法贪婪冲突，以及 GCC 14 默认将未声明函数提升为致命错误。
  * **修改措施**：规范宏运算符两侧空格隔离；导出函数原型并在 Makefile 注入宽容编译参数。
  * **对应版本**：`v2.3.0`

---

### 3. 可视化界面与编译器优化专项
* **[编译器优化等级与高级构建选项可视化设置界面设计与实现.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/编译器优化等级与高级构建选项可视化设置界面设计与实现.md)**
  * **解决问题**：针对编译器优化等级缺乏直观界面与自主可调性的痛点，新增可视化配置界面。
  * **核心设计**：在 `ToolchainConfigDialog` 新增 Group 4 编译器优化与构建选项面板，支持细粒度优化等级（`-O1`, `-O0`, `-O2`, `-O3`, `-Os`, `-Og`）、自适应动态说明卡片、DWARF2 调试符号及旧工程宽容度复选框；在主工作台工具栏实现双向联动与 `QSettings` 持久化。
  * **对应版本**：`v2.3.1`（插件 `v1.1.1`）

---

### 4. UI 主题与视觉体验专项
* **[IDE弹窗主题暗色异色消除与下拉框平滑倒三角重构.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/IDE弹窗主题暗色异色消除与下拉框平滑倒三角重构.md)**
  * **解决问题**：所有下拉框指示器变成细横线；芯片配置、烧录配置、工具链配置等对话框存在大面积白底异色。
  * **核心根因**：QSS 几何线段在高 DPI 缩放下的渲染退化，以及模态对话框内部原生控件未接入动态主题调色板。
  * **修改措施**：使用 QPainter 矢量抗锯齿多边形重构倒三角；对话框全量穿透暗色主题配色表。
  * **对应版本**：`v2.3.0`

---

### 5. 工作流效率与稳定性专项
* **[主界面打开项目记忆最近文件夹与视图重复及编译崩溃排查.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/主界面打开项目记忆最近文件夹与视图重复及编译崩溃排查.md)**
  * **解决问题**：“打开项目”每次重置路径无法记忆最近目录；菜单栏“视图”重复出现；点击编译偶发退出崩溃。
  * **核心根因**：缺少路径持久化配置；主窗口与插件视图注册未排重；异步子进程生命周期缺少安全守卫。
  * **修改措施**：接入 `QSettings` 历史目录记忆；增加唯一菜单合并过滤；引入 `ProbeProcessGuard` 进程安全守卫。
  * **对应版本**：`v2.3.0`

---

### 6. 官方网站与软件发布及部署专项
* **[SmileCodeIDE官方网站设计与GitHub发布及Pages部署.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE官方网站设计与GitHub发布及Pages部署.md)**
  * **解决问题**：软件正式发布与官网建设，对标 [CC Switch](https://ccswitch.io/zh/) 与 [恒基衡迹](https://zoufudun.github.io/hengji/) 风格，提供软件下载、痛点对比、编译器优化交互演示、全套技术归档直达，并自动化部署至 GitHub Pages 与 GitHub Release。
  * **核心设计**：采用暗夜流光科技视觉规范，打造响应式单页；配置 `.github/workflows/deploy-pages.yml` 与 `release.yml`；提供 `scripts/package_release.ps1` 自动化绿色便携包打包脚本。
  * **对应版本**：`v2.3.1`

* **[SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE免安装包与EXE及MSI安装包打包与GitHub发布.md)**
  * **解决问题**：全格式跨平台打包体系，生成直接在任何电脑运行的免安装便携包 (ZIP)、图形安装向导 (EXE) 与微软标准企业安装包 (MSI)，并完成 GitHub Release 全流程发布与哈希校验。
  * **核心设计**：结合 Qt 运行时部署、MinGW 运行时补齐、Qt Installer Framework 交互向导与 WiX Toolset 原生 MSI 编译链接；输出 SHA256/MD5 校验报告；升级官网三列安装包下载卡片。
  * **对应版本**：`v2.3.1`

* **[SmileCodeIDE软件更新中心与双主题官网及全插件展示主页设计.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeIDE软件更新中心与双主题官网及全插件展示主页设计.md)**
  * **解决问题**：修复 EXE 安装向导权限问题；在客户端“关于”增加软件更新中心（应用容器更新、GitHub Star 激励引导卡片、各插件运行与更新状态、Changelog）；官网全面深度对标 CC Switch 风格，支持极客暗黑与暖羊皮纸双主题切换、右上角三胶囊、导航【产品 ▾】Mega Menu，并为 5 个核心插件建立动态效果专属展示主页。
  * **核心设计**：实现 `AboutUpdateDialog` 客户端组件；重构 QtIFW 安装向导免提权路径；构建双主题 CSS 引擎与 5 大插件独立展示矩阵。
  * **对应版本**：`v2.3.1`

* **[SmileCodeStudio实机界面对齐与更新中心排版修复及全插件动态交互发布.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/SmileCodeStudio实机界面对齐与更新中心排版修复及全插件动态交互发布.md)**
  * **解决问题**：彻底解决关于与更新中心插件列表被挤压变形缺陷（图 1 修复）；官网首页 Hero 首屏 100% 还原用户真实使用的 SmileCode Studio 容器工作台（图 2 对齐）；9 大插件专属主页全部接入真实且可点击操作的动态仿真操作台（CAN 总线收发、示波器波形扫描、STM32 IAP 固件擦写升级、GDB 单步调试等）；全量打包排除嵌套目录缺陷，发布免安装 ZIP、向导 EXE 与微软标准 MSI。
  * **核心设计**：采用 `QScrollArea` 包装插件列表并规范行高与呼吸感；编写拟物化 `studio-simulator.css` 与 `app.js` 实时过滤器；为 9 大插件构建 HTML5 Canvas 与事件流实时交互引擎；优化全量打包流水线并发布至 GitHub。
  * **对应版本**：`v2.3.1`

---

### 7. 综合全景走查报告
* **[Walkthrough - STM32H7烧录运行、GCC14编译兼容与IDE体验全面升级.md](file:///k:/TOPFIRE/SmileCodeIDEUpdate/docs/Walkthrough%20-%20STM32H7%E7%83%A7%E5%BD%95%E8%BF%90%E8%A1%8C%E3%80%81GCC14%E7%BC%96%E8%AF%91%E5%85%BC%E5%AE%B9%E4%B8%8EIDE%E4%BD%93%E9%AA%8C%E5%85%A8%E9%9D%A2%E5%8D%87%E7%BA%A7.md)**
  * 汇集本次重大发布的全局综合验证矩阵、硬件实测调用链断点与寄存器取证记录。
