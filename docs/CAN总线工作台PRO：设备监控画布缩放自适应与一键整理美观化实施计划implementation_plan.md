# CAN总线工作台PRO：设备监控画布缩放自适应与一键整理美观化实施计划

## 问题背景与现状分析

在 CAN 总线工作台 PRO 的“设备状态监控”界面中，当前存在以下关键体验痛点：
1. **画布缩小（Zoom Out）时设备卡片重现重叠与溢出**：
   - 当用户滚动滚轮或缩小画布视角时，虽然房间和坐标被缩小（`pos * m_zoomLevel`），但设备卡片 `DeviceStatusWidget` 内部硬编码了 `setMinimumSize(85, 95)` 与 `minimumSizeHint() { return QSize(85, 95); }`。
   - Qt 强制禁止将控件尺寸缩小到低于 minimumSize。因此卡片尺寸死锁在 85x95，而坐标距离却被按比例缩减（如 50% 缩放时列距仅 56px），导致卡片水平/垂直直接交叉重叠，底层卡片溢出到房间外。
   - 房间标题栏内的设备计数标签 `m_countLabel` 固定位于 `y = 30` 处，在缩小画布时房间内容区高度压缩，导致第一排设备直接盖在计数标签上方。
2. **“一键整理”功能在不同画布状态下的适应性不足**：
   - 目前一键整理硬编码固定在左上角 `(40, 40)`，网格未根据视口（Viewport）水平与垂直居中，大屏幕右侧留白过大，小屏幕内容被截断。
   - 无论当前画布处于放大（如 150%）还是缩小（如 50%）或滚动偏移状态，整理后未重置或自适应调整为“适中化”视角，导致视口内视觉比例失衡。
   - 房间列数判定缺乏对房间总数与视口高宽比的精细协同计算。

---

## 优化目标与核心方案

1. **设备卡片与房间大小根据画布动态缩放（无死锁、无重叠）**：
   - 移除 `DeviceStatusWidget` 的硬编码 `setMinimumSize(85, 95)`，允许 `minimumSizeHint()` 自由下潜至 `QSize(10, 10)`。
   - 在 `updateZoom()` 与 `rebuildRoomCanvas()` 中，使用 `setFixedSize(scaledW, scaledH)` 与 `move(scaledX, scaledY)`，确保设备卡片大小、房间大小、相对间距与坐标始终保持严格数学等比缩放。
   - 优化 `DeviceStatusWidget::renderCache()`：在任意缩放比例（0.25x ~ 2.0x）下，图标、LED 指示灯、文字字号、文字换行/截断保持自适应渲染，视觉极佳。
2. **房间标题栏布局重构（单行集成，防止顶层遮挡）**：
   - 将房间内的 `m_titleLabel`（房间名）与 `m_countLabel`（设备计数）整合至顶部同一水平行（`y = 6`），节省约 24px 纵向空间，消除缩小画布时第一行设备卡片与计数标签的碰撞遮挡。
   - 房间内标题和操作按钮自适应缩放或精简。
3. **强化“一键整理”（美观化、适中化、状态自适应）**：
   - **视口自适应居中**：根据房间总数与当前视口宽高比，智能决定最佳列数（如 9 间房在大屏/宽屏下以 3×3 矩阵黄金比例呈现），计算整个网格的总体宽度与高度，在视口内**水平与垂直双向居中**摆放。
   - **适中化缩放（Best-Fit Zoom）**：无论用户此前处于何种缩放比例或画布滚动偏移，点击“一键整理”后：
     - 若当前 100% 视图可完整容纳所有房间，平滑恢复至 1.0 最佳比例并居中；
     - 若当前视口较小（如笔记本或窄屏分屏），自动计算最适中缩放比例（如 0.85 或 0.9），使整个视图无需滚动即可全景呈现，兼具呼吸感与适中度。
   - **重置滚动与画布容器**：将滚动条归零，画布容器按实际需要适度收敛，无论当前画布处于何种乱局，一键重塑为规整美观的工控看板。
4. **滚轮缩放事件可靠捕获**：
   - 为 `m_scrollArea`、`m_scrollArea->viewport()` 及 `m_gridContainer` 安装事件过滤器，确保在画布任意位置使用滚轮缩放均能精准响应，无视控件层级。

---

## 拟修改文件清单

| 文件 | 变更说明 |
| :--- | :--- |
| [`devicestatuswidget.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h) | 修改 `minimumSizeHint()` 为 `QSize(10, 10)`，解除缩小时尺寸下限拦截 |
| [`devicestatuswidget.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp) | 移除构造函数中的 `setMinimumSize(85, 95)` 和 `setMaximumSize(150, 160)`；优化 `renderCache()` 在小尺寸和各缩放级别下的自适应文字排版与图形比例 |
| [`roomwidget.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) | 增加单排标题栏自适应调整支持 |
| [`roomwidget.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp) | 将设备计数标签与标题合并至同排；在 `resizeEvent` 中动态调整标题和按钮位置，防止与设备冲突 |
| [`devicemonitorpanel.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) | 声明滚轮增强过滤与视口尺寸管理支持 |
| [`devicemonitorpanel.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp) | 1. 重构 `updateZoom()`：统一使用 `setFixedSize` 与 `move` 确保卡片和房间等比例缩小，杜绝重叠；<br>2. 强化 `autoArrangeRoomsAndDevices()`：根据视口比例智能布局、双向居中、自动计算适中缩放、重置滚动；<br>3. 在 `eventFilter` 中为视口与画布捕获滚轮缩放事件。 |

---

## 实施步骤

1. **Step 1: 改造 `DeviceStatusWidget`**
   - 彻底移除对卡片尺寸的硬性下限限制，使得卡片可以被外部自由设置为任意小尺寸。
   - 完善 `renderCache()`：动态计算文字尺寸（`qBound(6, qRound(8.0 * s), 11)`）、图标与指示灯偏移，防止小缩放下文字折行出界。
2. **Step 2: 重构 `RoomWidget` 内部布局**
   - 将 `m_countLabel` 移至与 `m_titleLabel` 同一行（顶部水平排列），并动态随房间尺寸变化。
   - 保证房间内部留出给设备卡片的纯净空间，杜绝顶部标签被设备卡片遮挡。
3. **Step 3: 改造 `DeviceMonitorPanel::updateZoom()` 与视口事件**
   - 在 `updateZoom()` 中严格根据 `m_zoomLevel` 动态设置设备卡片与房间控件的 `setFixedSize` 与 `move`。
   - 安装视口事件过滤器，捕获 Ctrl+滚轮（或画布滚轮），实现丝滑平顺缩放。
4. **Step 4: 强化 `autoArrangeRoomsAndDevices()`（一键整理适中化）**
   - 结合房间总数与视口比例动态计算最优矩阵（如 9 房间自适应 3 列 3 行）；
   - 对齐房间尺寸与内部分布；
   - 计算总占用区域并在当前视口中居中对齐（startX, startY 居中）；
   - 自动推导适中缩放比例（若超出则自适应缩小至刚好适中，若适中则设为 1.0）；
   - 复位滚动条并刷新画布。
5. **Step 5: 编译测试与验证**
   - 执行 `mingw32-make` 编译工程，确保 0 错误 0 警告。
   - 验证滚轮连续放大/缩小过程中设备卡片与房间严丝合缝、完全无重叠、无溢出；
   - 验证在不同窗口大小与不同缩放状态下点击“一键整理”的布局规整与适中化表现。

---

## 验证计划

### 编译验证
- 在 `m:\TOPFIRE\SmileCodeIDEUpdate\PhudonTools` 下运行 `mingw32-make -j8`，确认代码编译无任何报错。

### 功能验证
1. **缩放防重叠验证**：
   - 随意添加或加载 9 个房间、数十个设备；
   - 滚动滚轮将画布缩小至 75%、50%、25%，观察设备卡片是否按比例缩小且各卡片之间依然保持均匀间距，绝无重叠、无溢出房间。
   - 放大到 150%、200%，观察卡片等比例放大，间距均称。
2. **一键整理适中化验证**：
   - 在缩小状态（如 50%）下将房间随意拖拽打乱，点击“一键整理”，验证整个视图像素级整齐排列、居中呈现、且视角自动调整为最佳适中比例。
   - 在极大放大状态（如 200%）并偏移滚动条后点击“一键整理”，验证视角复位，所有房间均适中完整显示。
   - 检查房间标题与计数标签是否在顶部同一行整齐显示，不再被第一排设备覆盖。
