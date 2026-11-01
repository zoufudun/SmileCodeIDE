# DeviceMonitorPanel 底层架构整理与崩溃排查修复计划

针对 `DeviceMonitorPanel`（设备状态监控界面）中存在的持续崩溃问题，我们经过深入底层 Qt 事件循环与内存管理审计，定位到了以下 5 个核心崩溃源头，并制定了全面、稳健的重构修复方案。

---

## 🔍 崩溃根因排查报告

### 1. `processBatch()` 高频定时器与 UI 结构修改的线程/事件竞争 (致命级)
- **分析**：`m_batchTimer` 每 33ms 触发一次 `processBatch()`，该函数会直接遍历 `m_deviceWidgets` 和 `m_subWindows` 并调用其成员函数 `setStatus()`。
- **故障场景**：当用户点击界面切换 Tab、多屏切换开关、导入布局或切换模板时，`m_deviceWidgets` 和 `m_subWindows` 正在被清空、删除或重建。33ms 定时器在此瞬间触发，访问了野指针或已被 `delete` / `deleteLater` 的控件，直接导致随机 Core Dump 崩溃。
- **解决方案**：
  1. 在任何 UI 结构重建（如 `rebuildGrid`、`rebuildRoomCanvas`、`updateSubWindows`、`onImportClicked`）开始前，临时 `stop()` 批处理定时器 `m_batchTimer`，重建完成后再恢复 `start()`。
  2. 在 `processBatch()` 内部，对 `m_subWindows` 进行局部快照拷贝，并在访问控件前进行指针有效性校验。

### 2. `resizeEvent()` 中同步调用 `rebuildGrid()` 破坏 Qt Layout Engine 布局索引 (严重级)
- **分析**：当视口宽度变化导致计算的 `cols` 发生改变时，`resizeEvent()` 直接同步调用了 `rebuildGrid()`。在 `resizeEvent()` 调用栈中同步修改 Widget 父子链、隐藏/显示控件并操作 `QGridLayout` 是 Qt 官方明令禁止的重入行为。
- **故障场景**：开启多屏联动时，`subWin->showMaximized()` 会迫使操作系统向主窗口发送 Resize 消息，引发 `resizeEvent -> rebuildGrid -> updateSubWindows -> show -> resizeEvent` 的递归死循环，导致 QGridLayout 内部索引损坏爆栈。
- **解决方案**：在 `resizeEvent()` 中绝不直接同步调用 `rebuildGrid()`，而是采用延迟定时器调度，隔绝 `resizeEvent` 的事件调用栈。

### 3. `SubMonitorWindow` 副屏控件生命周期与 Parent 销毁机制不统一 (严重级)
- **分析**：副屏 `SubMonitorWindow` 上的设备/房间/模板背景控件，部分通过 `setParent(canvasContainer)` 挂在画布上，部分被手动 `delete` / `deleteLater`，析构函数中又手动遍历删除。
- **故障场景**：当多屏联动下主屏切换 Tab 时，副屏窗口会被重构。如果旧控件还在绘图帧中，手动的 `delete` 或 `setParent(nullptr)` 会导致 Qt 父销毁链表紊乱或野指针。
- **解决方案**：规范 `SubMonitorWindow` 内部控件的生命周期，所有控件继承 `canvasContainer` 作为 Parent，由 Qt 父子树统一安全析构，清除函数中只进行 `deleteLater()` 及清空指针容器，彻底避免重复释放。

### 4. 房间 (Room) 模式与网格 (Grid) 模式下 `m_gridLayout` 的接管冲突 (中高级)
- **分析**：`m_gridLayout` 挂在 `m_gridContainer` 身上。在 Room 模式下，设备卡片使用自由绝对定位 (`setGeometry`)，但 `m_gridLayout` 内部的 Geometry 计算依然存在，导致界面切换时卡片坐标乱套或布局失效。
- **解决方案**：在 Room 模式下彻底清空 `m_gridLayout` 的 items，并将 `m_scrollArea->setWidgetResizable(false)`，确保绝对定位不受 Layout 计算干涉；在 Grid 模式下恢复网格布局接管。

### 5. `DeviceStatusWidget::mouseReleaseEvent` 无抓取释放崩塌 (中级)
- **分析**：鼠标释放事件无条件执行 `releaseMouse()`，在常规拖拽无 `grabMouse()` 的情况下触发底层系统异常。
- **解决方案**：加入 `if (mouseGrabber() == this)` 安全判断。

---

## 🛠️ 详细修改步骤

1. **`devicemonitorpanel.h` & `.cpp`**：
   - 为 UI 结构重建添加 `m_batchTimer` 的暂停与保护机制。
   - 重构 `resizeEvent()` 中的 `m_gridCols` 变更逻辑，采用异步调度。
   - 完善 `processBatch()` 的防御性指针检查。
   - 彻底统一 `Room` 模式与 `Grid` 模式的控件容器挂载关系。

2. **`devicestatuswidget.cpp`**：
   - 保护 `releaseMouse()`。

---

## 🧪 验证计划

- 使用 MinGW 81_64 进行全量编译，确保无 Warning/Error。
- 手动测试：
  1. 开启多屏联动，频繁切换末尾与首部 Tab。
  2. 导入布局模板与 2D 配置文件。
  3. 频繁在 2D 画布上拖拽移动设备图标。
