# 串口界面“死机(Freeze)”问题深度修复计划

之前的四个修复（包括 `static` 缓冲区共享等严重 BUG）已经解决了一批直接导致进程崩溃（Crash）的代码缺陷。但根据测试，**在开启第一个串口并开启自动发送数据时，通过分屏或新标签页打开第二个串口并点击开启，程序依然会“死机”（界面彻底卡死、失去响应）。**

经过深度排查内存、UI渲染和事件循环交互，发现了导致 UI 彻底卡死的两个关键性能及事件流设计问题。

## 发现的深层卡死原因（UI Freeze）

> [!CAUTION]
> **深层 Bug 5：`QTextEdit` 接收/发送区无行数上限约束，导致内存和 UI 渲染爆炸**
>
> 位于 [setupUi()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#62-832) 中 `m_textReceive` 和 `m_textSend` 的初始化（第 314 行附近）：
> Qt 的 `QTextEdit` 默认是没有历史最大行数限制的。当串口 **1** 正在疯狂以 10ms ~ 100ms 间隔“自动发送”并接收数据时（并且可能附带 HTML 格式的高亮代码），如果有任何耗时操作（例如打开第二个串口）导致界面一瞬间卡顿，主线程会被堆积的 `append` 渲染排队直接淹没，内存占用飙升，事件循环 100% 被 UI 引擎占用，从而表现为程序的彻底死机。必须设置 `setMaximumBlockCount` 将历史行数限制在一个合理范围（如 1000 行），使得数据能够滚动刷新而不是无限制堆积。

> [!WARNING]
> **深层 Bug 6：[openClosePort()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1308-1372) 中普通开启失败时的阻塞弹窗（事件循环嵌套）**
>
> 位于 [openClosePort()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1308-1372)（第 1364 行）：
> 当用户点开第二个串口时，如果第二个串口尝试打开与第一个相同的 COM 端口，[open(QIODevice::ReadWrite)](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1308-1372) 会直接失败。此时代码调用 `QMessageBox::critical` 弹出一个模态阻塞对话框。
>
> 严重的是，由于分屏状态下串口 **1** 的计时器并没有被 [hideEvent](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1905-1913) 停止，它还在疯狂发射后台中断和后台数据，当模态对话框触发了**嵌套的本地事件循环**时，背景的串口 **1** 会继续疯狂调用 [sendData()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1809-1871) 和 [onReadyRead()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1380-1419) 往无上限的 `QTextEdit` 里灌数据…… 这一连串反应瞬间引发死锁般的 UI 假死。

## 修复方案 (Proposed Changes)

### [MODIFY] [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp)

**1. 增加接收区渲染保护（Block Count Limit）**
修改 [setupUi()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#62-832) 中的 `m_textReceive` 初始化，加入最大输出行块限制，保护 UI 线程不被日志淹没：
```cpp
  m_textReceive = new QTextEdit();
  m_textReceive->document()->setMaximumBlockCount(1000); // 新增保护
  m_textReceive->setReadOnly(true);
```

**2. 将打开失败的弹窗改为非阻塞（避免阻塞事件循环）**
修改 [openClosePort()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1308-1372)，使用 `QTimer::singleShot` 延迟弹出错误对话框：
```cpp
    } else {
      QString errorStr = m_serial->errorString();
      QTimer::singleShot(0, this, [this, errorStr]() {
        QMessageBox::critical(this, "错误", "无法打开串口:\n" + errorStr);
      });
      m_btnOpenClose->setChecked(false);
    }
```

## 验证计划 

1. 将上述限制代码打入补丁。
2. 开启第一个串口并设为 **极高频自动发送 (如 10ms)**。
3. 点击分屏，在新串口配置中选中**同一个串口 (模拟占用失败)** 并点击“打开”。
4. 预期结果：非阻塞的 QMessageBox 立即弹出提示被占用，背景的极速首个串口会因为 1000 行的渲染限制而保持稳定的 CPU 和内存使用率，无论切屏或多开都不会再造成主程序的“死机”无响应情况。
