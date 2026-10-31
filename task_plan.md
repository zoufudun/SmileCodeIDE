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
