# 设备状态监控界面四大体验与底层逻辑升级方案

## 背景与目标
根据用户提出的 4 项核心需求，对设备状态监控界面进行深度重构：
1. **统一房间大小（按设备数标准化）**：
   - 0 设备空房间尺寸完全一致；
   - 1 设备房间尺寸完全一致；
   - 2 设备房间尺寸完全一致；
   - $N$ 设备房间尺寸完全一致；
   - 房间大小由实际容纳的设备数量标准公式严格计算并自适应调整。
2. **Visio 风格智能辅助对齐线（Guide Lines）**：
   - 拖动房间时，在画布上实时显示水平对齐线、垂直对齐线、中心对齐线与标准间距线（荧光青色/金黄虚线与对齐节点）；
   - 显著增强拖拽排布的吸附对齐体验。
3. **全局连环推挤与绝对防重叠机制（Chain-Reaction Collision Resolution）**：
   - 解决“房间 1 推挤房间 2，房间 2 靠拢房间 3 导致房间 2 与房间 3 重叠”的传递失效问题；
   - 引入多轮松弛扩散连环防重叠推挤算法，确保在任何情况下、任何房间对之间均严格保持 `ROOM_GAP_X` 与 `ROOM_GAP_Y` 间距，绝对杜绝重叠。
4. **房间内设备防重叠与跨房间转移重排（Device Layout & Clean Transfer）**：
   - 房间内设备卡片绝不重合、重叠；
   - 当设备从一个房间转移到另一个房间（拖拽或编辑属性）时，源房间与目标房间自动重新计算标准尺寸，目标房间自动为新设备分配空闲网格槽位，绝不发生设备消失或隐藏在其他图标下方的问题。

---

## User Review Required
> [!NOTE]
> 1. **对齐辅助线实时绘制**：在 `RoomWidget` 拖动时发出 `roomDragging` 信号并在画布容器 `m_gridContainer` 的 `paintEvent` 中实时渲染 Visio 风格的高对比度对齐虚线与吸附标记，松开时自动淡出清除。
> 2. **房间尺寸自适应标准**：各房间尺寸将直接绑定其设备数量，增减设备时房间自动无缝缩放并重新规整内部网格。

---

## 拟定修改方案

### 1. 标准化房间尺寸计算 (`devicemonitorpanel.h`, `devicemonitorpanel.cpp`)
- 增加静态方法 `QSize standardRoomSizeForCount(int devCount, int shape = 0)`：
  - $N=0$：固定尺寸 $160 \times 140$；
  - $N=1$：固定尺寸 $160 \times 168$（cols=1, rows=1）；
  - $N=2$：固定尺寸 $240 \times 168$（cols=2, rows=1）；
  - $N=3$：固定尺寸 $352 \times 168$（cols=3, rows=1）；
  - $N=4$：固定尺寸 $240 \times 290$（cols=2, rows=2）；
  - $N=5, 6$：固定尺寸 $352 \times 290$（cols=3, rows=2）；
  - $N=7, 8$：固定尺寸 $464 \times 290$（cols=4, rows=2）；
  - $N > 8$：动态计算最佳行列数与对应标准尺寸；
  - 圆形、菱形按几何外接比例换算。
- 在 `ensureRoomCapacity(RoomRegion &r)` 中直接将 `r.geom.setSize(standardRoomSizeForCount(count, r.shape))`。

### 2. Visio 风格对齐虚线系统 (`roomwidget.h`, `roomwidget.cpp`, `devicemonitorpanel.h`, `devicemonitorpanel.cpp`)
- `RoomWidget`：
  - 在 `mouseMoveEvent` 拖动中发射 `roomDragging(m_id, geometry())`；
  - 在 `mouseReleaseEvent` 中发射 `roomDragFinished(m_id)`。
- `DeviceMonitorPanel`：
  - 定义 `AlignmentGuide` 结构体：记录方向（水平/垂直）、坐标位置、起止点及对齐类型。
  - 在 `onRoomDragging` 槽函数中：
    - 实时比对同界面其他房间的边缘与中心线（Left, Right, CenterX, Top, Bottom, CenterY, `other.right + ROOM_GAP_X`, `other.bottom + ROOM_GAP_Y`）；
    - 若距离 $< 12px$，执行实时坐标吸附，并生成对应的 `AlignmentGuide`；
    - 触发 `m_gridContainer->update()`。
  - 在 `m_gridContainer` 的 `eventFilter` 绘制阶段，用高亮半透明霓虹青色虚线（`#00D4FF`）绘制贯穿对齐线与端点标记。
  - 拖拽结束时清空辅助线列表并重绘画布。

### 3. 全局连环推挤与防重叠扩散算法 (`devicemonitorpanel.cpp`)
- 升级 `alignAndResolveRoomSpacing`：
  - 采用多轮迭代扩散算法（最大 30 轮或直到无任何相交）：
    - 遍历同界面所有房间对 $(R_A, R_B)$；
    - 若 $R_A$ 扩展间距包围盒与 $R_B$ 相交：
      - 确定推挤源与被推挤方（主动拖动/锁定方推挤未锁定方）；
      - 计算最小分离位移向量 $(shiftX, shiftY)$，推挤邻近房间；
      - 递归平移被推挤房间内部所有设备坐标；
      - 标记碰撞未完全解决，继续下一轮迭代检测。
  - 彻底杜绝连环碰撞时后续房间未被动避让导致的重叠问题。

### 4. 房间内设备防重叠与跨房间转移重排 (`devicemonitorpanel.cpp`)
- 增加 `layoutDevicesInRoom(const QString &roomName, const QString &targetView)`：
  - 重新计算该房间标准尺寸；
  - 收集该房间内所有设备，按网格 $(col \times (CARD\_W + GAP\_X), row \times (CARD\_H + GAP\_Y))$ 严格分配非重合坐标；
  - 执行 `clampDevicesInsideRoom`。
- 在 `onDeviceDragged` 中：
  - 若设备跨房间移动，分别对原房间和新房间调用 `layoutDevicesInRoom`，并自动调整两房间尺寸与避让；
  - 若在同房间内拖动，自动吸附至最近的空闲网格槽位，绝不与已有卡片重叠或被压在下方。
- 在 `onEditDeviceRequested` 与 `placeDeviceOnCanvas` 中同步使用统一排布逻辑。

---

## 验证计划
1. **编译构建**：
   - 执行 `qmake PhudonTools.pro` 及 `mingw32-make -j8` 验证编译零错误。
2. **功能验证**：
   - **尺寸一致性**：创建 0 设备、1 设备、2 设备房间，验证同设备数房间尺寸完全一致；
   - **Visio 对齐线**：拖动房间，观察画布上是否清晰实时呈现水平/垂直/居中对齐虚线；
   - **连环推挤防重叠**：将房间 1 拖向房间 2，继续推向房间 3，验证房间 2 与房间 3 会连环被动避让，任何房间对之间始终保持至少 `36px` 间距，绝对不重叠；
   - **设备跨房间移动**：将设备从 A 房间拖拽至 B 房间，观察 A 房间尺寸缩小、B 房间尺寸扩大，设备整齐排入 B 房间，无任何消失或层叠覆盖。
