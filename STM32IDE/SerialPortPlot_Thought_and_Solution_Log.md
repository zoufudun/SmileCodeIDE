# SerialPortPlot 思考与解决过程记录

## 1. 任务背景
- 目标：对 `serialportplot` 做完整分析，并把实际修复过程沉淀为可追溯文档。
- 范围：`serialportplot.h/.cpp`，以及其在 `MainWindow` 中的入口联动与构建验证链路。

## 2. 分析思路（高层）
1. 先做结构拆解：`SerialSession`（业务核心）-> `SerialPortPlot`（标签页）-> `SerialPortContainer`（分屏与全局工具栏）。
2. 再做调用链检查：串口收发、波形解析、发送路径、多条发送、分屏关闭、主题切换。
3. 最后按风险分级落地修复：先 P0（行为错误），再 P1（数据鲁棒性），再 P2/P3（一致性与技术债）。

## 3. 关键问题清单（发现）
1. 发送控件重复构造，成员指针被覆盖，导致逻辑与界面状态不一致。
2. 关闭最后一个真实会话会误关闭整个工具窗口。
3. 分屏关闭总是关闭“最后创建分屏”，而不是当前触发分屏。
4. 波形帧缓冲使用 `QString`，对字节流/边界情况鲁棒性不足。
5. 清空接收时未清协议缓冲、未重置波形滚动条状态。
6. 单条与多条发送换行策略不一致（`\r\n` vs `\n`）。
7. 主题变更信号未发射，容器层 `themeChanged` 链路实际未生效。
8. 多个声明/成员未落地（死声明、死字段、死函数）。

## 4. 解决过程（按阶段）

### P0：先修行为错误
1. 发送区改为“Dock 控件 + 悬浮控件镜像同步”，避免覆盖成员指针。
2. 会话关闭逻辑改为“至少保留 1 个真实会话”。
3. 分屏关闭改为按触发者关闭（`handleCloseSplit(SerialPortPlot *plot)`）。

相关位置：
- `STM32IDE/serialportplot.cpp`（发送控件同步、tab 关闭、split 关闭）
- `STM32IDE/serialportplot.h`（`handleCloseSplit` 签名）

### P1：增强数据与协议鲁棒性
1. 波形接收缓冲从 `QString` 改为 `QByteArray`。
2. 协议帧解析改为按字节缓冲做 `$...;` 解析，再转字符串分割。
3. `clearReceiveArea()` 增加 `m_rxBuffer.clear()` 和滚动条复位。
4. 多条发送换行统一为 `\r\n`（与单条发送一致）。

### P2：清理冗余与未生效逻辑
1. 去掉重复连接（`m_chkHideRxTx`）。
2. 欢迎文案直接驱动 `ScrollingLabel`，不再保留旧滚动死逻辑。
3. 让 `日志模式(m_chkRxLog)` 变成真实行为（`[LOG]` 标签 + 强制时间戳）。
4. 清理未使用成员与声明（例如 `m_chkRxNewLine`、`m_statusLabel`、`m_btnClearSend` 等）。

### P3：一致性完善
1. 时间单位切换不再只改标题，新增 `ScaledAxisTicker` 实现刻度值显示映射。
2. `applyGlobalTheme` 增加 `emit themeChanged(themeFile)`，打通主题通知链路。
3. 补连 `dockLocationChanged -> onDockLocationChanged`。

## 5. 验证过程与结果
1. 增量编译验证：
   - `mingw32-make -f Makefile.Release release/serialportplot.o`
   - `mingw32-make -f Makefile.Release release/moc_serialportplot.o`
2. 全量链路验证：
   - 初次触发 `release/STM32IDE.exe` 曾报 `QTcpSocket` 头缺失（旧 Makefile 问题）。
   - 执行 `qmake STM32IDE.pro` 重新生成后恢复正常，`mainwindow.o` 可编译通过（包含 `QtNetwork`）。
3. 当前结论：本次改动未引入编译错误，保留的主要是 Qt 头文件层警告与既有工程警告。

## 6. 产出文件
1. 已修改：`STM32IDE/serialportplot.cpp`
2. 已修改：`STM32IDE/serialportplot.h`
3. 新增回归清单：`STM32IDE/SerialPortPlot_Regression_Checklist.md`
4. 本文档：`STM32IDE/SerialPortPlot_Thought_and_Solution_Log.md`

## 7. 风险与后续建议
1. 建议执行一次真实串口长压（高频输入 + 分屏 + 主题切换并发）验证 UI 稳定性。
2. 如需进一步收敛，可把“日志模式”扩展到文件落盘（当前仅影响显示格式）。
3. 建议保留本文件作为后续 PR/迭代说明模板，按阶段持续追加。

## 8. 可复用模板（以后记录思考与解决过程）
```md
# 背景与目标
## 问题现象
## 分析思路（为什么这样排查）
## 根因定位（证据）
## 方案对比（候选方案与取舍）
## 实施步骤（按阶段）
## 验证方法与结果
## 风险与回滚点
## 后续计划
```
