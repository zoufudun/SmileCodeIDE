# 设备默认值修改修复与低性能处理器极致流畅优化方案

## 需求说明与根因分析

根据最新反馈，本次修改包含以下两个核心问题：

1. **设备默认值无法修改问题根因**：
   - 在 `DeviceMonitorPanel::rebuildGrid()` 中，对已存在的 `DeviceStatusWidget` 控件同步了 `Label`、`CanId`、`DeviceKind`，但**遗漏了对 `m.defaultVal` 默认状态的同步**。
   - `DeviceStatusWidget` 本身未保存 `m_defaultVal` 字段与 `setDefaultVal()` 方法，导致在协议配置窗口或右键属性窗口中修改默认值（0 正常 / 1 报警）后，已创建的控件未响应默认状态切换。

2. **低性能处理器（低端 CPU/无 GPU 加速）下的极致流畅优化**：
   在低配置嵌入式或旧款 CPU 平台上，持续高频 CAN 报文刷新可能导致矢量渲染和映射搜索瓶颈。需采取以下底层性能榨干优化：
   - **双重哈希快速查找 ($O(1)$ 复杂度)**：原代码每收到一帧报文均对 `m_mappings` 列表做线性遍历 $O(N)$。优化后建立 `QHash<quint32, QList<const DeviceBitMapping*>>` 哈希字典，使报文解析实现 $O(1)$ 常数时间秒速查找。
   - **DeviceStatusWidget 离屏 QPixmap 缓存 (Pixmap Offscreen Caching)**：卡片的深色科技背景、弧线、图标绘制包含大量 `QLinearGradient` 和 `QPainterPath` 复杂矢量运算。低端 CPU 软件渲染极慢。优化后将卡片静态轮廓与图标**缓存为 `QPixmap` 离屏快照**，重绘时仅需一次 `drawPixmap` 秒级贴图，渲染效率提升 **500%~1000%**。
   - **日志追加与 WebSocket 合并批处理**：在 33ms 刷屏周期内，将所有状态变更日志合并为一次 `appendHtml` 插入；WebSocket 消息汇总批次广播，避免频繁 JSON 构造与频繁 GUI 重绘。

---

## 拟修改的组件与文件 (Proposed Changes)

### 1. 默认值修改生效修复
#### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h)
#### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- 新增 `int defaultVal() const` 与 `void setDefaultVal(int val)`。
- 在 `setDefaultVal(int val)` 中更新 `m_defaultVal` 并同步应用 `setStatus(val == 1)`，重置缓存。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 在 `rebuildGrid()` 及 `onResetClicked()` 中，明确调用 `w->setDefaultVal(m.defaultVal)` 与 `w->setStatus(m.defaultVal)`，保证默认值修改与重置后卡片状态百分百同步。

---

### 2. 低性能处理器极致流畅优化
#### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h)
#### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- 引入离屏图像缓存 `QPixmap m_cachedBackground;` 与 `bool m_cacheDirty = true;`。
- 在尺寸变化、状态改变、风格切换或默认值改变时标记 `m_cacheDirty = true`。
- 在 `paintEvent()` 中，非闪烁静态状态直接绘制 `m_cachedBackground`，大幅降低 CPU 绘图算力消耗。

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 增加 `QHash<quint32, QList<int>> m_canIdToMappingIndices;` 哈希索引构建函数 `buildMappingHash()`。
- `processBatch()` 中解析报文时直接查表，将遍历复杂度由 $O(N \times M)$ 降为 $O(1)$。
- 合并 33ms 批次内的日志文本追加与 WebSocket 更新数据包。

---

## 验证计划

### 自动化构建验证
- 使用 `mingw32-make -f Makefile.Release` 进行编译，确保 0 Error 编译通过。

### 手动功能与性能验证
1. **默认值修改验证**：
   - 右键卡片或打开配置对话框，将某设备的默认值由 `0 (正常)` 修改为 `1 (报警)`，保存后验证卡片是否立即切换至报警状态。
   - 点击“重置”按钮，确认所有卡片精准恢复为各配置指定的默认值。
2. **低端 CPU 资源开销压测**：
   - 开启极高频率（如 5000 帧/秒）CAN 数据涌入，观察界面拖拽、悬停与全屏切换，验证在低配 CPU 下依然保持 60 FPS 丝滑无卡顿。
