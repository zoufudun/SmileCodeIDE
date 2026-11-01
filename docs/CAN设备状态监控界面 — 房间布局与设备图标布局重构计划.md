# CAN设备状态监控界面 — 房间布局与设备图标布局重构计划

本计划针对 CAN 设备状态监控界面（`DeviceMonitorPanel` 及 Electron 监控面板）的房间布局与设备图标布局进行彻底重构，完全解决图标重叠、房间交合、野图标游离及尺寸容量不足等问题。

---

## 🎯 重构目标与核心设计原则

### 1. 房间自动依设备数量动态扩容 (Requirement 1)
- 根据每个房间内绑定的设备数量 `count` 动态计算最合适网格行列数：
  - `cols = Math.max(2, Math.min(4, Math.ceil(Math.sqrt(count))))`
  - `rows = Math.ceil(count / cols)`
- 结合设备卡片标准尺寸（宽 96px，高 106px）、间距（20px）、房间标题栏安全高度（52px）及四周边距（20px），精准计算房间所需的极小几何宽高度：
  - `reqW = max(320, paddingLeft + paddingRight + cols * cardW + (cols - 1) * spacingX)`
  - `reqH = max(200, headerH + paddingBottom + rows * cardH + (rows - 1) * spacingY)`
- 房间几何尺寸跟随内部设备动态扩充与自适应调整。

### 2. 严禁重叠 & 网格与流式双重防碰撞 (Requirement 2)
- **房间与房间防重叠**：采用基于流式网格（Flow Grid Layout）与 AABB 碰撞检测算法的 `resolveRoomOverlaps()` 机制。对于同一视图内的多个房间，按固定间距（X轴 40px，Y轴 40px）从左至右、从上至下整齐排布，超出画布宽限时自动折行。
- **设备与设备防重叠**：房间内部设备按照严格的网格单元 `(rowIdx, colIdx)` 定位，坐标为 `(r.x + 20 + colIdx * 116, r.y + 52 + rowIdx * 126)`。每个单元物理隔离，100% 避免设备图标互相遮挡与重叠。

### 3. 严禁设备与房间交合切边 (Requirement 3)
- 设备图标起始 Y 坐标统一设定为 `r.y + 52`（严格位于房间 Header 标题栏下方），X 轴左右保留 20px 边距。
- 房间边框与设备图标边缘保持足够的内边距与外边距，严禁设备与房间边框、标题栏或邻近房间重叠。

### 4. 彻底杜绝“野图标”（房间外游离图标）(Requirement 4)
- 画布开放区域严禁出现没有归属房间的游离图标（野图标）。
- 所有在画布上绘制的设备必须 100% 存在于某个具体房间内部。

### 5. 严格依 `targetRoom` 属性归类摆放 (Requirement 5)
- 若设备的 `targetRoom` 属性非空：
  - 若对应房间已存在，自动将设备纳入该房间网格；
  - 若对应房间不存在，系统自动根据 `targetRoom` 名称在当前视图创建新房间并整齐排列设备。
- 若设备的 `targetRoom` 属性为空：
  - 严禁放置在画布上，自动放入界面上方的**未摆放设备停靠区（Unplaced Dock）**。

### 6. 默认显示所有存在的房间 (Requirement 6)
- 房间加载与创建时，默认 `visible = true`。
- 当前活动视图绑定的所有房间默认全部渲染并可见。

---

## 🛠️ 拟修改文件与模块

### [PhudonTools] (Qt C++ 核心应用)
#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
- 更新 `resolveRoomOverlaps` 与 `autoArrangeRoomDevices` 声明及辅助算法定义。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **`autoArrangeRoomDevices()`**：
  - 自动创建缺失房间；
  - 动态计算并设置房间 `reqW` 和 `reqH` 扩容尺寸；
  - 按照二维网格算法精确给所属设备计算相对房间的物理坐标 `m_deviceRoomPos`。
- **`resolveRoomOverlaps()`**：
  - 彻底推倒重写房间排列逻辑，采用多行流式矩阵算法与多轮 AABB 碰撞校正，保证房间之间横向与纵向间距均 $\ge 40\text{px}$。
- **`rebuildRoomCanvas()` & `updateUnplacedDock()`**：
  - 过滤空 `targetRoom` 设备，将其强制推送到顶部未摆放区；
  - 确保画布上的设备均在所属房间内部，完全清理游离野图标；
  - 默认将属于当前视图的所有 `visible` 房间进行展示。

### [status-monitor-electron] (Electron Web 监控应用)
#### [MODIFY] [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)
- 重构 `renderWorkspace()` 中房间自动扩容与网格排列逻辑，同步 C++ 端的房间流式防重叠算法与设备未摆放区归流规则。

---

## 🧪 验证计划

### 1. 编译与语法校验
- 在 Windows 环境下使用 `run_command` 执行 `qmake` + `mingw32-make` 编译 `PhudonTools`，验证 0 错误 0 警告。

### 2. 功能自动化与逻辑验证
- **扩容测试**：向某个房间分配 1 个、4 个、9 个设备，验证房间宽高是否自动扩容，保持内边距。
- **重叠测试**：创建 5 个以上房间，验证房间之间是否有 40px 间距，不存在交合/重叠。
- **野图标清理与空房间测试**：将部分设备的 `targetRoom` 清空，验证其是否自动退回未摆放区，画布无野图标。
- **默认显示测试**：刷新与切换标签页，验证现有房间均默认展示。
