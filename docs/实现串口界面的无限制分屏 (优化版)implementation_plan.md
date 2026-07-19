# 实现串口界面的无限制分屏 (优化版)

为了实现“将整个串口界面完整复制一份”并且支持无限制的水平和垂直分屏以及关闭操作，我们需要引入一个新的容器类来管理多个 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1700-1861) 实例和分屏逻辑。

## Proposed Changes

### [MODIFY] [serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h)
- 在 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1700-1861) 中增加 `void setToolbarVisible(bool visible);` 方法，以支持在新分屏中隐藏工具栏。
- 在 [SerialPortContainer](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h#258-279) 中添加 `QList<SerialPortPlot*> m_plotHistory;` 用来记录创建的 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1700-1861) 顺序。

### [MODIFY] [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp)
- **调整按钮位置**：将“水平分屏”、“垂直分屏”、“关闭分屏”按钮从全局的 `QToolBar` 移动到 `m_sessionTabs` 的右侧（通过 `QTabWidget::setCornerWidget(QWidget*, Qt::TopRightCorner)`）。
- **工具栏显示逻辑**：在 `SerialPortContainer::createNewPlot` 中，如果不是第一个创建的 Plot，则调用 `plot->setToolbarVisible(false)`，满足“分屏时不包含工具栏”的需求。
- **历史记录逻辑**：
  - 在 [createNewPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#2002-2014) 时将新 Plot 追加到 `m_plotHistory`。
  - 修改 `SerialPortContainer::handleCloseSplit` 逻辑，忽略传入的 `plot` 参数，而是从 `m_plotHistory` 尾部取最后一个 Plot（如果只剩一个则不处理），找到其父 `QSplitter` 并关闭该 Plot。最后从历史记录中移除。

### [MODIFY] [mainwindow.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/mainwindow.h)
- 将成员变量 `SerialPortPlot *m_serialPlot;` 改为 `SerialPortContainer *m_serialPlot;`。
- 添加 `class SerialPortContainer;` 前置声明。

### [MODIFY] [mainwindow.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/mainwindow.cpp)
- 在 `MainWindow::openSerialTool()` 中，实例化 `m_serialPlot = new SerialPortContainer();`，并将其作为串口调试助手的主窗口显示。

## Verification Plan
### Manual Verification
1. 打开“串口调试助手”。
2. 确认主界面的工具栏可见。
3. 确认标签页右侧出现了“水平分屏”、“垂直分屏”和“关闭分屏”三个按钮。
4. 依次点击分屏按钮多次创建多个分屏。
5. **确认新增的分屏中不包含工具栏**。
6. 点击“关闭分屏”，**确认总是关闭最后一次被创建的分屏界面**。
7. 确认分屏全部关闭后，剩下最初的唯一界面不会被关闭。
