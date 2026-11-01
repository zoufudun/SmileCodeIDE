# Implementation Plan — 设备房间自动扩容、无重叠排列、联动拖拽与 CAN 通道配置优化

针对用户提出的 5 项核心需求以及 Figure 1 中展示的房间设备重叠覆盖问题，制定以下重构与修复方案。

---

## 需求项解析与实施策略

### 1. 房间自动扩容与设备图标规范网格排列 (防重叠、防交合)
- **问题分析**：Figure 1 中“舱首/鱼雷舱”内设备卡片重叠、溢出房间底部边框，且卡片顶部覆盖到了房间标题栏。
- **解决方案**：
  - 在 Qt (`PhudonTools/devicemonitorpanel.cpp`) 与 Electron (`status-monitor-electron/renderer.js`) 中统一计算房间内部设备网格布局：
    - **避让标题栏**：卡片起始 $Y$ 坐标从房间顶部计算偏移 $\text{headerHeight} \ge 45\text{px}$，彻底避免设备卡片覆盖或交合房间标题栏。
    - **自动扩容**：根据房间内关联的设备数量 $N$，动态计算需要的最小宽度和高度：
      $$\text{reqW} = \text{padLeft} + \text{cols} \times \text{cardW} + (\text{cols} - 1) \times \text{spacingX} + \text{padRight}$$
      $$\text{reqH} = \text{headerH} + \text{padTop} + \text{rows} \times \text{cardH} + (\text{rows} - 1) \times \text{spacingY} + \text{padBottom}$$
      若房间当前尺寸小于所需尺寸，自动将房间宽度/高度扩展扩容。
    - **整齐网格**：每行放置 2~4 个设备卡片，间距统一，杜绝卡片重叠。

### 2. 严格按所属房间属性归位到房间内部
- 在设备属性映射（`targetRoom`）或配置文件设置了房间归属后，刷新布局时自动拉取该房间的内部坐标槽位，将设备精确放置在对应房间的内部网格点上。

### 3. 所有房间默认全显示与整齐排列
- 初始化或执行一键规整时，对所有可见房间以整齐网格模式（2 列/3 列）全显示排列，房间之间保留 $40\text{px}$ 间距，并自动消除房间重叠（`resolveRoomOverlaps()`）。

### 4. 房间移动联动内部设备同步平移 & 无房间设备在视图/布局模式下均归入未摆放区
- **房间联动移动**：当拖拽房间 (`onRoomMoved` / room drag) 时，计算房间移动增量 $(\Delta x, \Delta y)$，同步平移该房间内部包含的所有设备卡片位置，确保设置了房间属性的设备绝不会遗留在房间外部。
- **未摆放区域统一处理**：无所属房间（`targetRoom` 为空）的设备一律放置于“未摆放区域” (Unplaced Dock)。**无论是视图模式还是布局模式，统一显示与处理未摆放区域**。

### 5. 通信协议与数据源配置 CAN 通道动态显示 (默认两通道)
- 在“通信协议与数据源配置”对话框 (`protocolconfigdialog.cpp` / `canprotocolconfigdialog.cpp`) 中：
  - 根据底层连接设备的实际 CAN 通道数（从 `CanInterface` 获取或配置）动态生成下拉列表选项。
  - 默认提供“任意通道 (通道 0 + 通道 1 混合接收)”、“CAN 通道 0”、“CAN 通道 1”两通道选择选项。

---

## User Review Required

> [!IMPORTANT]
> 1. **视图模式下的未摆放区域**：原逻辑在视图模式下会隐蔽未摆放区域停靠栏，现根据需求4修改为：**无论视图模式还是布局模式**，只要存在未配置房间的设备，均展示未摆放区域以供直观监控。
> 2. **自动扩容规则**：当房间内设备增多时，房间将自动向右/向下自动调大尺寸，以保证所有设备图标整齐容纳不溢出。

---

## Proposed Changes

### Component 1: Qt C++ 端 (`PhudonTools`)

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **`autoArrangeRoomDevices()`**: 增强房间尺寸自动扩容逻辑（根据卡片列数与行数计算 `reqW` 和 `reqH`），确保卡片从 $Y + 45\text{px}$ 处开始整齐排列，不与房间标题交合。
- **`onRoomMoved(const QString &id, const QRect &newGeom)`**: 实现房间拖拽移动时，计算相对位移 $\Delta (x, y)$，同步更新房间内部所有所属设备卡片的位置 `m_deviceRoomPos[devId] += delta` 并重新渲染。
- **`updateUnplacedDock()`**: 移除仅在 `m_layoutEditingEnabled` 时才显示未摆放区域的限制，使其在视图模式与布局模式下行为统一。

#### [MODIFY] [protocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/protocolconfigdialog.cpp)
- 在 CAN 数据源配置中，根据真实连接设备动态获取 CAN 通道总数，默认列出通道 0 与通道 1（及双通道混合接收）。

---

### Component 2: Electron 端 (`status-monitor-electron`)

#### [MODIFY] [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)
- **`createRoomElement` & `setupDraggable`**: 房间拖拽过程中动态平移内部设备 Element（计算位移 $\Delta x, \Delta y$）。
- **`checkDockVisibility`**: 移除编辑模式单独约束，在视图/布局模式下均展示未摆放区域。
- **房间自适应扩容**: 增加/更新房间内部设备网格排列算法，避免设备图标重叠和遮挡房间标题。

#### [MODIFY] [style.css](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/style.css)
- 优化房间卡片 `.room-box` 内部 Padding、标题栏样式及 `.device-card` 的 Z-Index 层级，防止图标与标题冲突。

---

## Verification Plan

### Automated / Build Verification
- 编译 `PhudonTools` Qt C++ 项目（运行 `qmake` & `nmake` / `mingw32-make` 或使用 Qt Creator 构建脚本）。
- 启动 `status-monitor-electron` （`npm start` 或 Electron 实例测试）。

### Manual Verification
1. **重叠与自动扩容验证**：往同一房间分配 4 个以上设备，验证房间自动向右/向下扩容，卡片在标题栏下方（$Y \ge 45\text{px}$）以网格形式整齐排列，无任何卡片互相重叠或覆盖标题。
2. **房间移动联动验证**：拖拽房间，验证房间内部的所有设备图标跟着房间一起滑动，没有图标留在房间外。
3. **未摆放区域验证**：在视图模式与布局模式下分别测试，无房间配置的设备均停靠在未摆放区域。
4. **CAN 通道配置验证**：打开通信协议与数据源配置，验证 CAN 通道默认显示 2 通道（通道 0 / 通道 1 / 任意通道）。
