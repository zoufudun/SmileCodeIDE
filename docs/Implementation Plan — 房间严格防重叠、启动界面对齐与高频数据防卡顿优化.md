# Implementation Plan — 房间严格防重叠、启动界面对齐与高频数据防卡顿优化

针对您反馈的三项核心体验问题，本计划提出针对性的解决方案与架构优化：

---

## User Review Required

> [!IMPORTANT]
> **高频 CAN 数据防卡顿优化（1秒大流量数据流）**：
> - 当 CAN 调试助手以毫秒/秒级频率高并发发送报文时，主线程过去会对每一帧数据逐一执行逻辑解析、界面贴图绘制、DOM 日志追加及 JSON 序列化广播，导致主线程顿挫。
> - **解决方案**：在批处理引擎 `processBatch()` 中引入**批次状态归并（Batch Deduplication）与日志防抖（Log Rate Throttling）**。
>   1. **帧去重归并**：在每个 50ms 的批处理周期内，提取各设备在当前批次中的**最终 net 状态**，单设备在单批次内至多刷新 1 次 UI；
>   2. **日志高效追加**：高频数据下对批量日志进行集中一次性写入，避免文本框 DOM 频繁重绘；
>   3. **离屏缓存防护**：`DeviceStatusWidget::setStatus()` 增加状态比对防重，只有状态真实翻转时才触发重绘。

> [!NOTE]
> **房间绝对防重叠排列算法**：
> - 升级 `resolveRoomOverlaps()` 碰撞推开算法。
> - 任何时候（拖拽房间、缩放房间、新建房间、初始化加载），只要两房间包围盒相交，系统立即沿 X 轴向右（间距 $40\text{px}$）或换行向下顺移，彻底禁止房间重叠。

---

## Proposed Changes

### 1. 房间防重叠整齐排列算法优化

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **强化 `resolveRoomOverlaps()` 算法**：
  - 对同切页下的所有房间按坐标排序；
  - 迭代检测房间包围盒 `geom.intersects()`；
  - 发生碰撞时，将重叠房间按标准水平间距（$40\text{px}$）右移，若超过画布边界（$1500\text{px}$），自动换行下移；
  - 循环迭代直至所有房间重叠数量为 0。
- **触发点覆盖**：在 `onRoomMoved()`、`onRoomResized()`、`autoArrangeRoomDevices()`、`rebuildRoomCanvas()` 与 `onAddRoom()` 中全面接入该算法。

---

### 2. 界面启动非最大化显示混乱修复

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **补全 `resizeEvent(QResizeEvent *event)` 实现**：实现窗口调整大小时自动重新计算悬浮组件位置及画布缩放对齐。
- **延迟对齐对策 (`showEvent`)**：在 `showEvent()` 中增加 `QTimer::singleShot(50, ...)` 延迟调用 `zoomFit()` 与 `rebuildRoomCanvas()`，等待 Qt 窗口管理器完全确立视口宽高后，自动计算出完美匹配当前视口尺寸的比例，解决默认窗口启动时界面塌陷混乱的问题。

---

### 3. 高频 CAN 数据处理防卡顿性能优化

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **批次去重与归并 (`processBatch`)**：
  - 在 `processBatch()` 中，先将 `batch` 帧按 `deviceId` 提取最新的状态 `lastBitVal` 与帧快照；
  - 每个设备在当前 50ms 批处理中最多只调用一次 `widget->setStatus()`，消灭冗余重绘；
- **日志渲染防抖与批处理**：
  - 日志追加改用 `m_log->setUpdatesEnabled(false)` 批量 append，高频报文下限制单批次日志最大条数，避免文本 DOM 刷新拖垮 GUI 主线程。

---

## Verification Plan

### Automated Build Verification
- 运行 Powershell 编译命令：
  ```powershell
  mingw32-make -f Makefile.Release
  ```

### Manual Verification
1. **房间防重叠测试**：
   - 尝试手动将一个房间拖拽遮挡到另一个房间上方，释放鼠标；
   - 验证被拖拽/被遮挡的房间是否会自动弹开并以 $40\text{px}$ 间距整齐并排。
2. **启动显示测试**：
   - 以普通窗口模式（非最大化）启动程序；
   - 验证界面布局是否清晰、自适应居中，设备与房间显示整齐，无混乱塌陷现象。
3. **高频 CAN 接收流畅度测试**：
   - 使用 CAN 调试助手以 1 秒/10 毫秒的极快频率批量发送大量 CAN 帧；
   - 观察监控界面图标刷新与面板操作（如拖拽、缩放、点击菜单）是否依然保持极度丝滑流畅，无明显卡顿。
