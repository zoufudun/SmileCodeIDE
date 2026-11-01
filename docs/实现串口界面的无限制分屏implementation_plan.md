# 实现串口界面的无限制分屏

为了实现“将整个串口界面完整复制一份”并且支持无限制的水平和垂直分屏以及关闭操作，我们需要引入一个新的容器类来管理多个 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850) 实例和分屏逻辑。

## Proposed Changes

### [MODIFY] [serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h)
- 在文件中增加一个新类 `SerialPortContainer` 继承自 `QWidget`。
- 在 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850) 类中新增三个信号：
  - `void requestSplitHorizontal(SerialPortPlot* plot);`
  - `void requestSplitVertical(SerialPortPlot* plot);`
  - `void requestCloseSplit(SerialPortPlot* plot);`

### [MODIFY] [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp)
- 在 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850) 的工具栏 (Toolbar) 中添加三个动作按钮：
  - “水平分屏”
  - “垂直分屏”
  - “关闭分屏”
- 将这三个按钮的点击事件连接到上述新加的信号上。
- 实现 `SerialPortContainer` 的具体逻辑：
  - 内部使用 `QSplitter` 作为管理树的根节点。
  - 对于触发分屏操作的 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850)，识别其父级的 `QSplitter`，如果是同方向的分屏则在相邻位置插入一个新的 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850)；否则将当前 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850) 替换为一个新的子 `QSplitter` 并将当前的和新建的 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850) 放入其中。
  - 对于关闭分屏的操作，直接 `deleteLater()` 对应的 [SerialPortPlot](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1701-1850)（如果界面中只剩下一个串口，则不销毁）。

### [MODIFY] [mainwindow.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/mainwindow.h)
- 将成员变量 `SerialPortPlot *m_serialPlot;` 改为 `SerialPortContainer *m_serialPlot;`。
- 添加 `class SerialPortContainer;` 前置声明。

### [MODIFY] [mainwindow.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/mainwindow.cpp)
- 在 `MainWindow::openSerialTool()` 中，实例化 `m_serialPlot = new SerialPortContainer();`，并将其作为串口调试助手的主窗口显示。

## Verification Plan
### Automated Tests
- 本次更改属于 UI 交互重构，暂无相关单元测试。

### Manual Verification
1. 编译并运行应用程序。
2. 打开“串口调试助手”。
3. 在工具栏上点击**“水平分屏”**，确认界面一分为二（左右排列），且左右两侧均为完整的串口调试界面。
4. 再点击某一个界面上的**“垂直分屏”**，确认该区域再次一分为二（上下排列）。
5. 验证是否可以继续无限制分屏。
6. 点击某一个界面上的**“关闭分屏”**，确认该界面被关闭，而剩下的界面布局自动恢复并填充空间。
7. 确认最后剩下的一个串口界面无法被关闭分屏（或者关闭分屏按钮无效），避免整个窗口空白。
