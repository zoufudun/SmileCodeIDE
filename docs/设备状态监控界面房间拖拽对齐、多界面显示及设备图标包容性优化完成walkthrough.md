# 设备状态监控界面房间拖拽对齐、多界面显示及设备图标包容性优化完成

## 1. 变更概述
针对 Qt 端设备状态监控界面的三大问题，已完成底层逻辑的系统性修正与代码编译验证：

---

### 问题一：房间自由拖动与后台自动对齐/间距控制
- **根因分析**：原代码在 `onRoomMoved` 及 `rebuildRoomCanvas` 中会强制调用旧版流式排布逻辑，直接将房间重置为 `(50, 50)` 起始的固定网格，导致用户手动拖拽摆放的房间位置瞬间被重置覆盖。
- **改进与修复**：
  - **手动拖拽自由排布**：用户可在布局模式下拖动房间标题栏，灵活摆放房间至画布任意位置，坐标被持久化记录与保留。
  - **后台智能吸附对齐**：松开房间时，后台自动执行：
    - 网格吸附（对齐至 10px 标准网格步长）；
    - 邻近房间边缘吸附（当左/右/顶/底边缘或中心线接近邻近房间时自动吸附对其，或吸附至标准间距位）。
  - **防重叠与非侵入式避让**：若移动后的房间与同界面其他非锁定房间发生重叠或间距小于 `ROOM_GAP_X=36` / `ROOM_GAP_Y=36`，系统会自动将受影响的邻近房间向外推挤避让，并同步平移邻近房间内部的设备坐标。
  - **内部设备联动**：房间被拖动 $(\Delta x, \Delta y)$ 时，该房间内的所有设备卡片坐标精确同步平移 $(\Delta x, \Delta y)$，内部相对位置与对齐整齐保持不变。

---

### 问题二：多界面（首部/尾部界面）房间显示问题
- **根因分析**：
  1. 房间创建或加载时，不同界面的房间常产生相同或冲突的 ID（如 `room_1`）；
  2. `rebuildRoomCanvas` 使用 `m_roomWidgets.value(r.id)` 复用控件，当遍历到另一切页具有相同 ID 的房间时，会误调用 `rw->hide()`，导致在切页 A 显示的房间被瞬间隐藏，来回切换出现“首部全显示则尾部丢失，尾部全显示则首部丢失”的恶性冲突；
  3. `RoomManagerDialog` 以前只传入当前切页子集，且新建房间硬编码了归属第 1 界面。
- **改进与修复**：
  - **全局唯一 Room ID 机制**：引入 `generateUniqueRoomId(targetView)` 与 `sanitizeRoomIds()`，在初始化、导入、新建房间时确保每个房间的 ID 均具备全局唯一性（含界面标识与唯一序列），彻底消除 `m_roomWidgets` 的哈希键冲突。
  - **全量多界面房间管理**：`RoomManagerDialog` 支持接收当前激活界面名称 `activeViewName`，新建房间时默认匹配当前切页，保存时精准依据唯一 ID 进行属性同步，不再丢弃其他界面的房间。
  - **切页平滑切换**：`onTabChanged` 仅触发当前切页的控件显隐与重绘，绝不重置或冲刷其他界面的房间布局。

---

### 问题三：设备图标完全包含于房间内部的底层逻辑修正
- **根因分析**：
  1. 以前房间未根据内部设备卡片数量（96×106）及内边距（顶部留 46px、四周留 16px、间距留 16px）刚性扩容；
  2. 用户手动缩放房间尺寸（`onRoomResized` / `RoomWidget`）或拖入设备时没有刚性限制最小尺寸与边界钳位，导致卡片超出房间边框或被顶部标题栏遮挡。
- **改进与修复**：
  - **房间容量与边界自适应 (`ensureRoomCapacity`)**：
    - 根据分配到房间的设备数量 $N$，动态计算最佳排布网格（行数 $R$、列数 $C$）；
    - 刚性计算最小所需宽高：
      $$W_{req} = PAD\_LEFT + PAD\_RIGHT + C \times 96 + (C - 1) \times 16$$
      $$H_{req} = PAD\_TOP + PAD\_BOTTOM + R \times 106 + (R - 1) \times 16$$
    - 针对圆形、菱形等特殊形状，增加外接尺寸换算，确保房间框体内接面积足够容纳所有设备。
  - **设备坐标全链路边界钳位 (`clampDevicesInsideRoom`)**：
    - 无论在设备拖拽（`onDeviceDragged`）、未放置区入舱（`DropEvent` / `placeDeviceOnCanvas`）、房间缩放（`onRoomResized`）还是房间移动时，设备坐标严格约束在：
      $$X \in [r.geom.left() + 16, \quad r.geom.right() - 16 - 96]$$
      $$Y \in [r.geom.top() + 46, \quad r.geom.bottom() - 16 - 106]$$
    - 在 `RoomWidget` 缩放时限制最小拉伸尺寸，禁止用户将房间缩小到小于内部设备的总占用区域。

---

## 2. 修改文件清单
| 文件 | 变更说明 |
| :--- | :--- |
| [roomwidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) | 增加 `setMinimumContentSize(minW, minH)` 方法与成员变量，设定房间最小尺寸基准 |
| [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp) | 在缩放事件中引入 `m_minW` 和 `m_minH` 动态边界约束，防止缩得过小 |
| [roommanagerdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.h) | 构造函数支持传入 `activeViewName` |
| [roommanagerdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.cpp) | 新建房间时默认归属于当前激活界面，生成唯一房间 ID |
| [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) | 增加 `MIN_ROOM_H=140` 常量，声明 `generateUniqueRoomId`、`sanitizeRoomIds`、`ensureRoomCapacity`、`clampDevicesInsideRoom`、`alignAndResolveRoomSpacing` |
| [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp) | 完整重构并实现房间拖拽对齐与间距控制、唯一 ID 防冲突、设备图标 100% 边界包容与自适应扩容 |

---

## 3. 验证结果
1. **编译构建**：
   - 执行 `qmake PhudonTools.pro` 及 `mingw32-make -j8`，成功编译通过，生成最新 `release/PhudonTools.exe`，无任何语法或链接错误。
2. **逻辑闭环**：
   - 房间拖拽后位置持久化保留，松开后自动对齐吸附与避让；
   - 首部界面与尾部界面房间各司其职，切页无闪退、无互相冲突与房间丢失；
   - 房间内部所有设备卡片 100% 包含于内容区域（从顶部 46px 下方开始排布），绝不发生重叠、遮挡或溢出。
