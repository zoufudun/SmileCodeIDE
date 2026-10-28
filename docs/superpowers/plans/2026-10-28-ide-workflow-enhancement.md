# SmileCodeIDE 工作流增强实施计划

**Goal:** 修复并增强原生编译、调试、烧写工作流，统一操作图标，发布本地 v2.2.0。

**Architecture:** 保留 MainWindow / STM32DebugController 与现有插件核心引擎的关系。控制器管理任务生命周期，界面只按状态控制入口。

**Tech Stack:** C++17、Qt 6 / qmake、Qt Test、QPainter。

## 任务

- [x] 在 PhudonTools/tests 添加控制器 Qt Test 工程，验证重复打开、忙碌互斥、烧录信号与探针解析；先运行确认失败。
- [x] 修复 PhudonTools/stm32debugcontroller.cpp/.h 的生命周期、信号连接、探针和烧录地址；确保启动失败与取消恢复空闲。
- [x] 修改 PhudonTools/mainwindow.cpp/.h，增加重建、取消、暂停，统一菜单、工具栏与按钮状态；新建专用 Qt 绘制图标头文件。
- [x] 统一 PhudonTools.pro、main.cpp、关于窗口的版本来源并更新 README、CHANGELOG.zh.md。
- [x] 运行控制器回归测试及相关已有 Qt 测试，执行主程序 Release 构建，检查图标渲染、Git diff 和版本元数据。
- [x] 写入 docs/WORK_SUMMARY_2026-10-28.md，提交任务文件并创建本地 v2.2.0 标签，确认用户已有修改保留。

## 检查点

1. 测试在原实现上显示预期失败，修复后通过。
2. 全部修改编译、审查通过；硬件行为未验证时在总结中明确说明。
3. 提交与标签指向同一版本，工作树仅保留用户原有配置修改。
