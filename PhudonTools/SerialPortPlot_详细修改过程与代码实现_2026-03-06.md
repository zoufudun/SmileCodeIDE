# SerialPortPlot 详细修改过程与代码实现（2026-03-06）

## 1. 文档目的
1. 记录本次 `serialportplot` 从分析到落地修复的完整过程。
2. 保留关键技术决策与代码实现细节，便于后续复盘、审计、继续迭代。
3. 给测试和维护同学提供可直接对照源码的依据。

## 2. 变更范围
1. 核心变更文件：
   - `STM32IDE/serialportplot.cpp`
   - `STM32IDE/serialportplot.h`
2. 配套文档：
   - `STM32IDE/SerialPortPlot_Regression_Checklist.md`
   - `STM32IDE/SerialPortPlot_Thought_and_Solution_Log.md`

## 3. 分析与定位过程（按问题）

### 3.1 发送控件状态不一致问题
1. 现象：发送设置区和发送文本框悬浮区的“自动发送/换行”在某些场景表现不一致。
2. 根因：同名成员控件被重复 `new`，后创建对象覆盖成员指针。
3. 影响：某些启用/禁用逻辑只作用于其中一套控件，导致 UI 与行为脱节。

### 3.2 关闭会话导致整个窗口退出问题
1. 现象：关闭最后一个真实会话标签会关闭整个串口工具窗口。
2. 根因：`onTabCloseRequested` 在 `count <= 2` 时直接执行 `window()->close()`。
3. 影响：用户误操作成本高，不符合“至少保留一个可用会话”的常见串口工具行为。

### 3.3 分屏关闭目标错误问题
1. 现象：点击某分屏的“关闭分屏”，有概率关闭的不是当前分屏。
2. 根因：容器层总是关闭 `m_plotHistory.last()`，不是发起请求的 `plot`。
3. 影响：分屏操作不可预期，破坏用户心智模型。

### 3.4 波形协议缓冲鲁棒性问题
1. 现象：串口高频或非纯文本场景下，协议帧边界易受编码转换影响。
2. 根因：波形协议缓冲使用 `QString`，并在接收早期做了 `fromLocal8Bit` 转换。
3. 影响：帧解析边界不稳定，存在潜在漏帧/错帧风险。

### 3.5 日志模式未落地问题
1. 现象：UI 有“日志模式”勾选，但不影响输出格式。
2. 根因：仅创建控件，未接入收发文本渲染逻辑。
3. 影响：功能名与实际行为不一致。

### 3.6 时间单位名义切换问题
1. 现象：切换 Points/ms/s 只改了轴标签文案，刻度值未发生表达变化。
2. 根因：未替换 ticker，仅修改 `xAxis->setLabel`。
3. 影响：用户误以为单位换算已生效。

### 3.7 主题通知链路断开问题
1. 现象：容器层有 `themeChanged` 连接，但实际不触发。
2. 根因：`SerialPortPlot::applyGlobalTheme` 未 `emit themeChanged(...)`。
3. 影响：后续依赖主题通知的逻辑无法工作。

### 3.8 死代码与死声明积累
1. 现象：存在声明但无实现/无引用，存在已废弃的滚动文本函数和成员。
2. 影响：可维护性下降，阅读成本增加。

## 4. 具体修改过程与代码实现（按阶段）

## 4.1 阶段 P0：修复高影响行为错误

### 4.1.1 修复发送控件“双实例覆盖”
1. 修改点：将 Dock 侧控件改为局部变量，悬浮控件保留成员；两侧做双向同步。
2. 关键代码：
```cpp
// Dock-side controls are kept as mirrors of the floating controls below.
QCheckBox *dockChkTxNewLine = new QCheckBox("发送新行");
QCheckBox *dockChkAutoSend = new QCheckBox("自动发送(ms):");
QSpinBox *dockSpinAutoSendInterval = new QSpinBox();

// Keep dock-side and floating controls in sync without overriding members.
connect(dockChkTxNewLine, &QCheckBox::toggled, m_chkTxNewLine,
        &QCheckBox::setChecked);
connect(m_chkTxNewLine, &QCheckBox::toggled, dockChkTxNewLine,
        &QCheckBox::setChecked);
connect(dockChkAutoSend, &QCheckBox::toggled, m_chkAutoSend,
        &QCheckBox::setChecked);
connect(m_chkAutoSend, &QCheckBox::toggled, dockChkAutoSend,
        &QCheckBox::setChecked);
connect(dockSpinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
        m_spinAutoSendInterval, &QSpinBox::setValue);
connect(m_spinAutoSendInterval, QOverload<int>::of(&QSpinBox::valueChanged),
        dockSpinAutoSendInterval, &QSpinBox::setValue);
```
3. 结果：避免成员指针被覆盖，Dock 与悬浮控件状态完全同步。

### 4.1.2 修复“最后会话关闭即退出窗口”
1. 修改点：保留至少一个真实会话，不再关闭整个窗口。
2. 关键代码：
```cpp
if (m_sessionTabs->count() <= 2) {
  return;
}
```
3. 结果：用户误关最后标签时仅拦截，不会丢失整个工具窗口上下文。

### 4.1.3 修复分屏关闭目标
1. 修改点：槽函数改为带参，按发起请求的 `plot` 关闭。
2. 头文件改动：
```cpp
void handleCloseSplit(SerialPortPlot *plot);
```
3. 实现改动：
```cpp
void SerialPortContainer::handleCloseSplit(SerialPortPlot *plotToClose) {
  if (m_plotHistory.size() <= 1) {
    return;
  }
  if (!plotToClose || !m_plotHistory.contains(plotToClose)) {
    return;
  }
  QSplitter *parentSplitter =
      qobject_cast<QSplitter *>(plotToClose->parentWidget());
  if (parentSplitter) {
    plotToClose->hide();
    plotToClose->deleteLater();
    m_plotHistory.removeAll(plotToClose);
  }
}
```
4. 结果：关闭行为与用户点击对象一致。

## 4.2 阶段 P1：提升协议与数据鲁棒性

### 4.2.1 协议缓冲从 QString 改为 QByteArray
1. 头文件改动：
```cpp
#include <QByteArray>
QByteArray m_rxBuffer;
```
2. 实现改动：
```cpp
m_rxBuffer.append(data);
int startIdx = m_rxBuffer.indexOf('$');
int endIdx = m_rxBuffer.indexOf(';', startIdx);
QByteArray payloadBytes =
    m_rxBuffer.mid(startIdx + 1, endIdx - startIdx - 1).trimmed();
QString payload = QString::fromLatin1(payloadBytes);
```
3. 结果：帧边界处理更稳定，避免早期字符编码转换对字节流的干扰。

### 4.2.2 清空行为完整化
1. 修改点：清空接收区时同时重置协议缓冲和滚动条状态。
2. 关键代码：
```cpp
m_textReceive->clear();
m_rxBuffer.clear();
m_scrollbarWaveform->setMinimum(0);
m_scrollbarWaveform->setMaximum(0);
m_scrollbarWaveform->setValue(0);
```
3. 结果：清空后无残帧污染，波形视图回到初始状态。

### 4.2.3 多条发送换行统一
1. 修改点：多条发送与循环发送中的 ASCII 换行统一为 `\r\n`。
2. 关键代码：
```cpp
if (globalNewLine)
  data.append("\r\n");

if (m_chkMultiNewLine->isChecked() && !isHex)
  data.append("\r\n");
```
3. 结果：单发/多发协议一致，减少终端侧解析差异。

## 4.3 阶段 P2：功能一致性与死代码清理

### 4.3.1 日志模式落地
1. 修改点：接收、单发、多发、循环发送都接入日志标签与时间戳策略。
2. 接收实现：
```cpp
const bool logMode = m_chkRxLog->isChecked();
const bool showTimestamp = m_chkRxTime->isChecked() || logMode;
const QString rxTag = logMode ? "[LOG][RX]" : "[RX]";
```
3. 发送实现：
```cpp
const bool logMode = m_chkRxLog->isChecked();
const bool showTimestamp = m_chkTxTime->isChecked() || logMode;
const QString txTag = logMode ? "[LOG][TX]" : "[TX]";
```
4. 多发循环实现：
```cpp
const bool logMode = m_chkRxLog->isChecked();
// [LOG][TX] / [LOG][LOOP]
```
5. 结果：`日志模式` 从“仅 UI”变为“真实输出行为”。

### 4.3.2 欢迎文案逻辑清理
1. 修改点：使用 `ScrollingLabel` 自带滚动机制，移除旧的 `scrollWelcomeMessage` 死函数和 `m_scrollPos` 成员。
2. 实现：
```cpp
m_lblWelcome->setText(m_welcomeText);
// 开关串口时更新文本
m_lblWelcome->setText(m_welcomeText);
```
3. 结果：状态栏欢迎文案可见且一致，避免双机制并存。

### 4.3.3 死声明与死成员清理
1. 删除未落地接口：
```cpp
void setToolbarVisible(bool visible);
QMenu *getViewMenu() const;
QMenu *m_viewMenu;
void replaceWidgetInSplitter(...);
int getPlotCount(...);
```
2. 删除未使用成员：
```cpp
QLabel *m_statusLabel;
QCheckBox *m_chkRxNewLine;
QPushButton *m_btnClearSend;
```
3. 结果：头文件语义更干净，减少维护噪音。

## 4.4 阶段 P3：时间单位与主题链路完善

### 4.4.1 时间单位“真正生效”
1. 修改点：引入 `ScaledAxisTicker`，单位切换时同步替换 X 轴 ticker。
2. 关键实现：
```cpp
class ScaledAxisTicker : public QCPAxisTicker {
public:
  explicit ScaledAxisTicker(double divisor, int fixedPrecision = -1)
      : m_divisor(divisor > 0.0 ? divisor : 1.0),
        m_fixedPrecision(fixedPrecision) {}

  QString getTickLabel(double tick, const QLocale &locale, QChar formatChar,
                       int precision) Q_DECL_OVERRIDE {
    const QChar fmt = formatChar.isNull() ? QChar('f') : formatChar;
    const int p = (m_fixedPrecision >= 0) ? m_fixedPrecision : precision;
    return locale.toString(tick / m_divisor, fmt.toLatin1(), p);
  }
};
```
3. 切换策略：
```cpp
if (index == 0) { label = "Time (Points)"; divisor = 1.0; precision = 0; }
else if (index == 1) { label = "Time (ms)"; divisor = 1.0; precision = 0; }
else if (index == 2) { label = "Time (s)"; divisor = 1000.0; precision = 3; }
m_customPlot->xAxis->setTicker(
    QSharedPointer<QCPAxisTicker>(new ScaledAxisTicker(divisor, precision)));
```
4. 结果：切换单位后，坐标文本显示与单位含义一致。

### 4.4.2 主题信号链打通
1. 修改点：在 `applyGlobalTheme` 末尾发射 `themeChanged`。
2. 关键代码：
```cpp
emit themeChanged(themeFile);
```
3. 结果：容器层 `handleThemeChanged` 可接收主题变更并缓存当前主题。

## 4.5 构建链路问题处理（工程级）
1. 现象：`release/STM32IDE.exe` 触发时出现 `fatal error: QTcpSocket: No such file or directory`。
2. 原因：旧 `Makefile.Release` 与当前 `.pro` 状态不一致，未含 QtNetwork 相关路径。
3. 处理：
```powershell
qmake STM32IDE.pro
mingw32-make -f Makefile.Release release/mainwindow.o
```
4. 结果：重新生成后编译参数包含 `-DQT_NETWORK_LIB` 与 `.../include/QtNetwork`，问题消失。

## 5. 验证步骤（实际执行）
1. 增量编译：
```powershell
mingw32-make -f Makefile.Release release/serialportplot.o
mingw32-make -f Makefile.Release release/moc_serialportplot.o
```
2. 链路验证：
```powershell
mingw32-make -f Makefile.Release release/STM32IDE.exe
qmake STM32IDE.pro
mingw32-make -f Makefile.Release release/mainwindow.o
```
3. 结果：
   - `serialportplot` 相关对象编译通过。
   - `moc_serialportplot.o` 编译通过。
   - `qmake` 后主窗口对象编译通过（含 QtNetwork）。
   - 剩余警告为 Qt 头文件/既有工程警告，无新增致命错误。

## 6. 今日实际代码实现锚点（便于快速跳转）
1. 构造与新 ticker：`serialportplot.cpp:27-49`
2. 发送控件镜像同步：`serialportplot.cpp:301-316`, `634-649`
3. 日志模式接收输出：`serialportplot.cpp:1510-1543`
4. 波形字节流解析：`serialportplot.cpp:1616-1677`
5. 时间单位刻度映射：`serialportplot.cpp:1832-1856`
6. 发送日志输出：`serialportplot.cpp:1973-2021`
7. 清空完整重置：`serialportplot.cpp:2043-2057`
8. 多发日志与换行：`serialportplot.cpp:2276-2324`, `2334-2374`
9. 会话关闭保护：`serialportplot.cpp:2725-2733`
10. 主题信号发射：`serialportplot.cpp:2762-2779`
11. 分屏关闭目标修正：`serialportplot.cpp:3001-3015`
12. 头文件核心声明变更：`serialportplot.h:264`, `318`

## 7. 结论
1. 本次修复覆盖了行为正确性、数据鲁棒性、功能一致性、可维护性四个层面。
2. 关键用户体验问题（误关闭窗口、分屏关闭错位、日志模式无效、时间单位名义切换）已落地解决。
3. 当前代码可作为后续继续优化（文件日志、协议插件化、自动化测试）的稳定基线。

