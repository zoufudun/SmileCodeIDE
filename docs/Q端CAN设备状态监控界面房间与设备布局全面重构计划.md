# Q端CAN设备状态监控界面房间与设备布局全面重构计划

## 背景与问题分析
在当前Qt端CAN设备状态监控界面中存在以下问题：
1. **房间无法自动扩容包容设备**：当房间内设备增多（如4个设备排成两行）时，房间尺寸未随设备行数动态自适应扩大，导致第2行设备突出于房间边框之外，卡片与房间虚线边框严重重叠交合。
2. **设备图标可能产生坐标偏移与交合**：内部卡片起始坐标与房间顶部标题栏/工具按钮冲突，缺少严密的内边距计算。
3. **野图标问题**：未归属房间或所属房间为空的设备可能错误遗留在画布上。
4. **房间与房间之间可能重叠**。

---

## 用户核心需求与重构设计

### 1. 房间自动根据放置的设备图标数量来扩容
- 建立统一的布局常量：
  - 卡片标准尺寸：`CARD_W = 96`, `CARD_H = 106`
  - 设备水平间距：`GAP_X = 20`，垂直间距：`GAP_Y = 20`
  - 房间内边距：`PAD_LEFT = 20`, `PAD_RIGHT = 20`, `PAD_TOP = 52`（避开顶部标题栏、改名/锁定/删除按钮与计数器），`PAD_BOTTOM = 20`
  - 房间基础最小尺寸：`MIN_ROOM_W = 280`, `MIN_ROOM_H = 200`
- 智能列数与行数计算：
  - 设备数 $N \le 2$：$\text{cols} = \max(1, N)$
  - 设备数 $N \in [3, 4]$：$\text{cols} = 2$
  - 设备数 $N \in [5, 6]$：$\text{cols} = 3$
  - 设备数 $N \in [7, 8]$：$\text{cols} = 4$
  - 设备数 $N > 8$：$\text{cols} = 4$（或智能分配）
  - 行数 $\text{rows} = \lceil N / \text{cols} \rceil$
- 房间精确包围盒：
  - $\text{reqW} = \max(\text{MIN\_ROOM\_W}, \text{PAD\_LEFT} + \text{PAD\_RIGHT} + \text{cols} \times \text{CARD\_W} + (\text{cols}-1) \times \text{GAP\_X})$
  - $\text{reqH} = \max(\text{MIN\_ROOM\_H}, \text{PAD\_TOP} + \text{PAD\_BOTTOM} + \text{rows} \times \text{CARD\_H} + (\text{rows}-1) \times \text{GAP\_Y})$
  - 房间尺寸自动扩容至 $\ge \text{reqW}$ 且 $\ge \text{reqH}$，绝不允许设备溢出。

### 2. 严禁设备图标互相重叠、房间之间重叠，按规范间距排列
- **设备间**：房间内按 $(row, col)$ 矩阵严格分配：
  - $X = \text{roomX} + \text{PAD\_LEFT} + col \times (\text{CARD\_W} + \text{GAP\_X})$
  - $Y = \text{roomY} + \text{PAD\_TOP} + row \times (\text{CARD\_H} + \text{GAP\_Y})$
  - 设备之间水平间距严格为 20px，垂直间距严格为 20px。
- **房间之间**：
  - 房间流式防重叠算法：多行多列排布，横向间距 40px，换行纵向间距 40px，杜绝房间之间互相覆盖。

### 3. 严禁设备图标和房间之间有重叠和交合
- 设备坐标严格处于房间内部安全区域：$[X_{\min} + 20, Y_{\min} + 52] \sim [X_{\max} - 20, Y_{\max} - 20]$，四周均有充足留白，完全包含在虚线框内。

### 4. 严禁设备图标出现在房间外面（野图标）
- 只有满足 `targetRoom` 有效且在当前界面存在对应房间的设备，才会被赋予画布坐标并显示在房间内。
- 任何孤立、未分配或所属房间不存在的设备，彻底清除画布坐标，绝不出现在房间外空白画布上。

### 5. 严格按照所属房间属性放置，若为空则放置于未摆放区域
- `targetRoom` 非空的设备严格放置到对应名称的房间中。
- `targetRoom` 为空的设备统一移至底部的【未摆放设备停靠区（Unplaced Dock）】中，可供用户在布局模式下拖动到房间中进行分配。
- 设备从房间拖出时，`targetRoom` 置空并退回未摆放区；拖入房间时，自动归属该房间并触发重新排布和房间自适应扩容。

### 6. 默认将存在的房间都进行显示
- 无论新创建还是从配置载入的房间，`visible` 默认置为 `true`。
- 在切换标签页视图（如“界面1”、“界面2”等）时，当前界面下的所有房间均正常显示。
- 副屏/多屏联动（`SubMonitorWindow`）同步应用该套排布和扩容规则。

---

## 涉及修改的文件

1. [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
   - 声明布局常量及重构后的排布算法函数。
2. [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
   - 重构 `autoArrangeRoomDevices()`：实现自动扩容、设备网格计算、野图标清洗、空房间与多设备房间自适应。
   - 重构 `resolveRoomOverlaps()`：实现房间流式防重叠布局。
   - 重构 `rebuildRoomCanvas()`：严格按房间与未摆放区渲染设备，保证 zoomLevel 正确同步。
   - 重构 `onDeviceDragged()`：拖入房间自动归属与扩容，拖出房间自动归未摆放区。
   - 重构 `SubMonitorWindow::rebuildDevicesAndRooms()`：确保副屏多屏联动与主屏布局完全一致。
3. [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp)
   - 优化标题与按钮布局，保证标题栏在扩容和缩放时自适应。

---

## 验证计划
1. 编译验证：确保 C++ 代码无任何语法错误与编译告警。
2. 功能测试：
   - 测试房间内放置 0、1、2、4、6、8 个设备时的房间自适应扩容尺寸与网格位置。
   - 验证无重叠、无交合、无野图标，设备完全位于房间虚线框内部。
   - 验证所属房间为空的设备正确出现在未摆放停靠区。
   - 验证多屏联动副屏窗口与主屏一致。
