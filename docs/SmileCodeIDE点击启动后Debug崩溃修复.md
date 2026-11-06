# 点击 SmileCode IDE 后 Debug 崩溃修复

## 现象和定位

工作台可以启动，在 Qt Creator 中运行 Debug 构建并点击 SmileCode IDE 时退出。Windows Application Error 1000 的记录指向 `build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/PhudonTools/debug/PhudonTools.exe`，故障模块为 Qt 6.11.1 的 Qt6Core.dll，异常代码 `0xc0000602`。

此前启动冒烟只覆盖了工作台，Release 回归也不会触发本次 Debug 断言。此次增加强制开启 Qt 断言的 `PhudonTools/tests/EditorStartupDebugTest.pro`，直接构造编辑器便可稳定重现相同异常。

## 根因

`CodeEditor::setupFunctionNameHighlighting()` 使用 lambda 作为 `textChanged` 回调，却传入 `Qt::UniqueConnection`。Qt 6.11.1 的 `qobject.h:268` 明确要求 UniqueConnection 的槽是 QObject 子类成员函数指针；Debug 构建在该模板中触发 `Q_ASSERT_X`。

失败日志：`build/startup-crash/editor-debug-red.txt`，包括完整断言文本；进程退出码与用户 Windows 错误记录一致。非零内存构造诊断未复现问题，已移除该临时诊断测试，未将其作为修复证据。

## 修改

将延迟函数高亮的 textChanged 连接移入每个编辑器只执行一次的 `setupEditor()`，使用正常的 lambda 连接，保留 QPointer 和定时器上下文保护。主题切换的高亮配置方法仅更新颜色及当前高亮，不再注册连接，避免简单去掉 UniqueConnection 后积累重复回调。

新增 Debug 启动测试在定义 QT_NO_DEBUG 时直接拒绝编译，覆盖编辑器创建、多次切换主题、文本更新，并等待延迟高亮回调执行。

## 验证

- 修复前：Debug 测试以 `0xc0000602` 退出并报告 qobject.h:268 断言。
- 修复后：Debug 启动测试通过，日志 `build/startup-crash/editor-debug-green.txt`。
- Qt Creator 实际 Debug 目标及 `build/keil-app` Release 目标均重新编译成功。
- 已在真实桌面点击 Debug 工作台的“启动代码编辑器”，确认 IDE 窗口、编辑器、项目树及工具栏正常显示；调试日志 `build/startup-crash/verify-debug.log` 没有该断言。
- 主窗口、Keil 工程及现有编辑器回归结果分别保存在对应构建目录的 `startup-fix-results.txt`。
- Debug 启动 3/3、主窗口 12/12、Keil 工程 12/12、编辑器 13/13，共 40 项通过。
- 静态代码审查通过。原有主题样式中的 QString::arg 警告与本次断言不同，未做无关样式改动。

本次没有发布或替换 `dist` 下的旧安装包。
