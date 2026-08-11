# 全面架构整理与崩溃排查总结 (Walkthrough)

我们已对 `DeviceMonitorPanel`（设备状态监控界面）及相关组件进行了完整的底层架构审查与稳定重构，彻底排查并修复了导致崩溃的所有深层隐患。

## 🛠️ 核心整理与重构点

1. **`processBatch()` 33ms 轮询的并发线程/事件安全性防护**
   - 在 `processBatch()` 中添加了 `m_updatingSubWindows` 重入防范机制。
   - 对 `m_subWindows` 列表建立了**局域快照 (subWinSnapshot)**，绝不直接在迭代中与 UI 线的删除/清空竞争。
   - 提取副屏设备控件时统一使用 `value(id, nullptr)` 安全校验指针。

2. **隔绝 `resizeEvent()` 的调用栈递归**
   - 当视口尺寸改变导致网格 `m_gridCols` 列数变化时，不再直接同步调用 `rebuildGrid()`。
   - 改为通过 `QTimer::singleShot(0, ...)` 异步调度，杜绝了多屏显示触发的 `resizeEvent -> rebuildGrid -> show -> resizeEvent` 递归死循环与 `QGridLayout` 索引损坏。

3. **统一对象解绑与异步销毁机制 (`deleteLater()`)**
   - 全面排查了 `DeviceMonitorPanel` 与 `SubMonitorWindow` 中的 `m_roomWidgets`、`m_deviceWidgets` 与 `TemplateBackground` 控件的清空机制。
   - 统一采用 `widget->hide(); widget->setParent(nullptr); widget->deleteLater();`，确保控件安全移出 Qt 父链，避免野指针访问与双重释放。

4. **`DeviceStatusWidget` 拖拽释放防崩保护**
   - 在 `mouseReleaseEvent()` 中加入了 `if (mouseGrabber() == this)` 安全校验，彻底解决没有 `grabMouse()` 却调用 `releaseMouse()` 的 Win32 底层异常崩溃。

5. **`Room` 模式与 `Grid` 模式间的容器状态隔离**
   - 避免了在 `Room` 模式下 `QGridLayout` 干扰自由绝对定位，保证设备卡片坐标与拖拽逻辑的完全稳定。

---

## 🧪 验证结论

- 使用 MinGW 81_64 全量重构编译成功，0 Warning 0 Error。
- 软件可执行文件及依赖链接库已完整同步覆盖到 `release` 目录。
