# SmileCodeIDE 专项升级与问题攻坚任务计划 (task_plan.md)

## 一、 标准工作流规范 (Standard Operating Procedure)

根据用户明确要求，从即日起，每次问题解决均严格执行以下四步标准化流程：

1. **阶段 1：问题分析与定位 (Problem Analysis & Root Cause)**
   - 深入分析用户反馈的故障现象、外部环境与异常日志；
   - 结合源代码、调用栈、底层汇编或硬件调试器（GDB / 寄存器现场）精准锁定根本原因，杜绝主观臆测。
2. **阶段 2：修改计划与方案 (Modification Plan)**
   - 制定严密具体的实施计划，明确涉及的代码文件、预期修改点、风险规避与交付验证标准。
3. **阶段 3：具体修改措施与总结报告 (Implementation & Walkthrough Documentation)**
   - 给出清晰、完整、逐项对照的修改措施与技术原理总结；
   - 严格落实到可下载、易保存的本地 Markdown 归档报告（如 `walkthrough.md` 及 `docs/` 副本）。
4. **阶段 4：Git 版本控制与版本号策略 (Git Version Control & Bumping)**
   - **大修正 (Major / Minor)**：涉及重大机制调整、底层架构重构、关键外设/系统故障修复，递增主版本或次版本号（例如 `v2.2.0` $\rightarrow$ `v2.3.0`）；
   - **小修正 (Patch)**：涉及局部 UI 微调、样式优化、特定环境兼容小修，递增补丁版本号（例如 `v2.3.0` $\rightarrow$ `v2.3.1`）；
   - 同步更新项目版本配置（如 `PhudonTools.pro` 的 `VERSION` 与 `plugin.json` 的 `version`）；
   - 提交 Git Commit 并打上对应版本的清晰说明。

---

## 二、 阶段执行与闭环记录

### 阶段 1：野火 STM32H743 触摸画板工程烧录后板卡无效果排查与解决【完成】
- **问题分析与定位**：
  - GDB 抓取芯片现场停在 `Handler External Interrupt(40)` 及 `i2c_Delay()`；
  - 根因：Makefile 默认 `-O0`（无优化）使得 400MHz Cortex-M7 内核上的软件循环延时放大 10 倍以上，在 `GTP_Init_Panel()` 触发 I2C 超时重试与外部中断风暴，后续 `LCD_Init()`、开启背光与画板绘制根本未被执行；Keil MDK 默认 `-O1` 故无此问题。
- **具体修改措施**：
  - 将编译优化升级为 `-O1 -g3 -gdwarf-2`（硬件现场验证通过：断点全过，背光使能 PD7=1，显存 `0xD0177000` 完整绘制白色画板与按钮）；
  - 修改 `Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.cpp` 永久默认使用 `-O1`，重新编译部署 `STM32WorkbenchPlugin.dll`。

### 阶段 2：C 语言语法与 GCC 14 宽容度兼容【完成】
- **修改措施**：
  - 修复 `gt9xx.c:811` 科学计数法词法贪婪冲突为 `0x80FE - 0x8047 + 1`；
  - 在 `bsp_lcd.h` 补全 `LCD_DispString_EN_CH` 导出声明；
  - 在 `MakefileGenerator.cpp` 的 `CFLAGS` 中注入 `-Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types` 保证与 Keil 对齐。

### 阶段 3：UI 主题与体验深度优化【完成】
- **修改措施**：
  - 重写 `idetheme.h` 中的 QComboBox 下拉倒三角绘制，用矢量抗锯齿平滑多边形替换硬编码横线；
  - 消除目标芯片选择、固件烧录、工具链配置等对话框的白底异色与高对比残留；
  - 打开项目自动记忆并优先定位到最近访问目录；
  - 修复视图重复添加与后台进程生命周期导致的崩溃隐患。

### 阶段 4：版本控制与发布归档 (Release v2.3.0)【当前进行中】
- [x] 更新 `PhudonTools/PhudonTools.pro` 版本为 `2.3.0`
- [x] 更新 `Plugins/STM32WorkbenchPlugin/plugin.json` 版本为 `1.1.0`
- [x] 生成并保存详细走查报告 `walkthrough.md` 与 `docs/` 副本
- [ ] 执行 Git 暂存与版本提交 (Commit & Tag)
