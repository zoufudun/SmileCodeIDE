# 工业级示波器架构与多模块性能优化实施计划

本计划针对用户提出的“数字示波器 / 实时趋势图”性能要求，在**串口示波器 (`SerialPortPlot`)**、**CAN 收发数据监控 (`CanViewPanel`)** 和 **CAN 状态监视界面 (`DeviceMonitorPanel`)** 三大核心模块中，落地高性能**生产者-消费者 (Producer-Consumer) 解耦架构**、**无锁环形缓冲区 (`LockFreeRingBuffer`)**、**Min-Max 极值抽样压缩算法**、**OpenGL 硬件加速与双缓冲**以及**多线程采集-渲染分离**策略，彻底消除高帧率、大数据量涌入时的卡顿、掉帧与内存泄漏风险。

---

## 1. 核心架构设计

```
[数据源: 串口/CAN硬件] 
       │ (高频并发数据流)
       ▼
[无锁环形缓冲区 (LockFreeRingBuffer)] ────> 避免堆内存频繁分配与锁竞争
       │ 
       ▼
[后台解析与抽样线程 (Worker Thread)] ────> Min-Max 降采样 (极值保留, 像素桶压缩)
       │ (主界面事件解耦)
       ▼
[双缓冲/离屏缓存 (Offscreen Pixmap / OpenGL)] ────> GPU/QPixmap 离屏渲染加速
       │ (帧率定时器 30~60 FPS 局部刷)
       ▼
[UI 界面显示 (QCustomPlot / DevicePanel)] ────> 丝滑流畅, 零卡顿零掉帧
```

### 关键优化维度

| 模块 | 原瓶颈/隐患 | 优化后方案 |
| :--- | :--- | :--- |
| **串口示波器 (`SerialPortPlot`)** | 主线程边接收边解析字符串并直接添加坐标点到 `QCustomPlot`，点数达数万时 CPU / UI 严重掉帧。 | 串口接收存入 `LockFreeRingBuffer`；后台线程解析；前端采用 **Min-Max 极值抽样算法**（按像素宽度抽样 Peak/Trough）；使能 `QCustomPlot` OpenGL 渲染加速。 |
| **CAN 收发面板 (`CanViewPanel`)** | 频繁调用 `QTableWidget` 插入行，高帧率（>10,000 帧/秒）直接挂起 Qt 事件循环。 | CAN 帧写入 `LockFreeRingBuffer`；主定时器（30ms 间隔）批量 dequeue 出数据，限制单次刷新上限并更新统计 UI；超额数据自动滑动清理。 |
| **CAN 状态监视 (`DeviceMonitorPanel`)** | 高速 CAN 传感器帧直接在 UI 槽函数中触发重绘与布局计算。 | 报文解析与设备状态映射存入环形缓冲区，UI 维持 2Hz 定时红灯闪烁与离屏缓存画板更新，CPU 占用保持极低。 |

---

## User Review Required

> [!IMPORTANT]
> 1. **OpenGL 渲染能力依赖**：`QCustomPlot` 的 OpenGL 模式需本地 Graphics Driver 支持。若显卡驱动不支持 OpenGL，程序会自动回退到极值抽样优化后的传统软件渲染，确保兼容性。
> 2. **数据保留策略与内存控制**：Min-Max 抽样算法不会改变原始导出/保存的数据精度，但在画面滚动的渲染层，点数上限控制在界面像素维度的 2~4 倍（例如 2,000 个采样点代表 100,000 个原始点），从而将显存和 CPU 占用降低 95% 以上。

---

## Proposed Changes

### Component 1: 算法与缓冲区底层基础设施 (`ringbuffer.h`)

#### [MODIFY] [ringbuffer.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/ringbuffer.h)
- 扩展 `LockFreeRingBuffer` 类，增加批量读取与滑动窗口截取接口。
- 增加 `MinMaxDownsampler` 极值抽样静态算法工具类，支持将任意长度的坐标序列 `(keys, values)` 快速降采样为适合当前像素宽度的桶极值点对（Min, Max），保留所有波峰波谷与毛刺特征。

---

### Component 2: 串口示波器 (`serialportplot.h`, `serialportplot.cpp`)

#### [MODIFY] [serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.h)
- 引入 `LockFreeRingBuffer<char, 65536> m_rxRingBuffer;` 存放原始串口字节流。
- 增加解析工作线程 / Worker 对象，将串口字节切帧与浮点解析从主界面线程解耦。
- 增加 Min-Max 抽样数据缓存结构，并添加 `m_customPlot->setOpenGl(true)` 开关与控制逻辑。

#### [MODIFY] [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.cpp)
- `onReadyRead()` 仅将数据无锁 `push` 到环形缓冲区，主线程耗时从毫秒级降至微秒级。
- 建立独立解析/分帧流程，提取数据点后存入通道缓冲。
- 在 `onReplotTimeout()` 中触发 `MinMaxDownsampler::process()`，生成精简绘制数据集后更新到 `QCustomPlot` 并执行离屏渲染重绘。
- 在 `setupChart()` 中初始化 `m_customPlot->setOpenGl(true)` 与高性能网格参数。

---

### Component 3: CAN 收发监控 (`canviewpanel.h`, `canviewpanel.cpp`)

#### [MODIFY] [canviewpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canviewpanel.h)
- 优化 `LockFreeRingBuffer<PendingFrame, 16384> m_frameRingBuffer;` 的压栈与弹出逻辑。
- 增加 `QTimer m_uiFlushTimer;` 批量刷新定时器。

#### [MODIFY] [canviewpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canviewpanel.cpp)
- `onFrameReceived` 仅进行 `m_frameRingBuffer.push()`，不做 UI 节点创建。
- 定时器批量消费环形缓冲区，单次刷新上限设为 200 帧（超过部分汇总计入统计计数，避免 UI 挂起），刷新完毕后触发离屏局部重绘。

---

### Component 4: CAN 设备状态监视 (`devicemonitorpanel.h`, `devicemonitorpanel.cpp`)

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
- 使用无锁队列接收传感器状态，结合现有的离屏 QPixmap 缓冲与闪烁定时器。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 优化 `DeviceStatusWidget` 的双缓冲重绘（Offscreen QPixmap Cache），避免重绘未变更的静态元件部件。

---

## Verification Plan

### Automated Build & Compilation
- 执行 `qmake PhudonTools.pro`
- 执行 `mingw32-make -f Makefile.Release`
- 验证零编译错误与警告。

### Performance & Smoothness Verification
- 模拟高频串口波形数据流入（>1,000 帧/秒，多通道波形），观察界面响应速度与 CPU 占用率。
- 验证开启 Min-Max 极值抽样后，正弦波、方波与高频噪声毛刺能否 100% 准确显示（不丢峰值）。
- 测试切换全屏/滚动界面时无任何掉帧与堆内存膨胀现象。
