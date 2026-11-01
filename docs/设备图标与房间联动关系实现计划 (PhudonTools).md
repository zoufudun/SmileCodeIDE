# 设备图标与房间联动关系实现计划 (PhudonTools)

本计划旨在满足用户提出的 4 项核心需求：实现设备图标与房间的深度联动机制（所属关系）、自动房间创建与整齐排列、未摆放设备区域归属管理以及房间移动时内部设备的同步联动。

---

## 🎯 需求拆解与架构设计

### 1. 设备图标拖拽归属联动 (`onDeviceDragged`)
- **需求描述**：若将某个设备图标移动至某个房间内，则该设备图标的属性房间号 (`targetRoom`) 相应修改；若拖出所有房间，则清除房间号属性。
- **设计逻辑**：
  - 在 `onDeviceDragged(int deviceId, const QPoint &newPos)` 中，计算设备图标的中心点坐标 (`devCenter`)。
  - 遍历当前界面下的所有房间 `m_rooms`：
    - 若 `devCenter` 处于房间 `r` 的 `geom` 范围内：将该设备的 `mapping.targetRoom` 更新为 `r.name`，并保存 `m_deviceRoomPos[deviceId]`。
    - 若 `devCenter` 不在任何房间内部：清除该设备的 `mapping.targetRoom`（置空 `""`），并从 `m_deviceRoomPos` 中移除。
  - 触发 `rebuildRoomCanvas()` 与 `updateUnplacedDock()`，确保界面与未摆放区域实时准确同步。

---

### 2. 属性修改自动建房与网格整齐排列 (`autoArrangeRoomDevices` / `onEditDeviceRequested`)
- **需求描述**：若将设备图标所属房间属性修改为某个房间，若该房间已存在则直接移动至该房间；若不存在则自动新建该房间并整齐排列。
- **设计逻辑**：
  - 当在编辑对话框或协议配置表格中修改了 `targetRoom`：
    - 若 `targetRoom` 非空，调用 `autoArrangeRoomDevices()`：
      1. 检查当前界面是否存在同名房间。若不存在，自动创建 `RoomRegion`（分配默认尺寸 350x250 及合理的非重叠坐标）。
      2. 收集该房间内所有设备，按照双列网格（`cols = 2`）在房间内部重新计算居中整齐排列坐标（`posX = r.x + 20 + col * 140`, `posY = r.y + 45 + row * 160`）。
      3. 若设备数量过多，动态扩展房间的 `height` 保证控件不超出房间边界。
    - 若 `targetRoom` 被清空或设置为 `"(未指定/未放置区)"`：从 `m_deviceRoomPos` 中移除该设备。

---

### 3. 双向联动与未摆放设备区域归属管理
- **需求描述**：实现设备图标与房间的联动所属关系，没有设置房间属性的图标放置于未摆放设备区域。
- **设计逻辑**：
  - **规则绑定**：
    - `targetRoom` 有值 ↔ 设备位于画布指定房间内。
    - `targetRoom` 为空 ↔ 设备存放在顶部的“未摆放设备区域” (`m_unplacedContainer`)。
  - **房间重命名联动 (`roomRenameRequested`)**：
    - 当房间从 `oldName` 重命名为 `newName` 时，同步更新所有关联设备的 `mapping.targetRoom = newName`。
  - **房间删除联动 (`roomDeleteRequested`)**：
    - 当房间被删除时，自动将该房间内所有设备的 `mapping.targetRoom` 清空，并从 `m_deviceRoomPos` 移除，设备自动安全回退至未摆放设备区域。

---

### 4. 房间移动联动内部设备 (`onRoomMoved`)
- **需求描述**：移动某个房间，同时将该房间内的设备图标一起移动。
- **设计逻辑**：
  - 在 `onRoomMoved(const QString &id, const QRect &newGeom)` 中：
    - 计算房间移动的增量位移 `dx = unzoomedNewX - oldX`, `dy = unzoomedNewY - oldY`。
    - 若 `dx != 0 || dy != 0`：
      - 遍历属于该房间的所有设备（`mapping.targetRoom == room.name` 或坐标原先在 `oldGeom` 内的设备）。
      - 将设备的 `m_deviceRoomPos[dId]` 偏移 `+ QPoint(dx, dy)`。
      - 若对应的 UI 控件 `DeviceStatusWidget` 已在画布上，直接同步其界面坐标 `w->move(w->x() + dx * zoom, w->y() + dy * zoom)`，避免画面闪烁。

---

## 🛠️ 涉及修改的文件

### [PhudonTools/devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
- 确认与补充关联函数声明（如 `autoArrangeRoomDevices()`、`onRoomMoved()`、`onDeviceDragged()`）。

### [PhudonTools/devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **`onDeviceDragged`**：增加碰撞检测逻辑，拖入房间修改 `targetRoom`，拖出房间清空 `targetRoom` 并回退至未摆放区域。
- **`autoArrangeRoomDevices`**：完善自动建房判断、双列网格对齐算法及房间高度自适应。
- **`onRoomMoved`**：实现增量 `(dx, dy)` 移动房间内所有设备的关联逻辑。
- **`onEditDeviceRequested`**：优化编辑对话框提交后的整齐排列与房间切换处理。
- **房间重命名/删除 Lambda**：添加设备 `targetRoom` 同步更新与删除退回未摆放区逻辑。

### [PhudonTools/canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- 确保批量修改 `targetRoom` 列后，能够正确反映至设备结构中。

---

## 🧪 验证计划

### 1. 拖拽联动测试
- 拖拽未摆放区域设备进入“1号机房”，右键/双击编辑该设备，验证“所属房间”属性自动变为“1号机房”。
- 将“1号机房”内的设备拖出到房间外的空白画布，验证设备自动存入“未摆放设备区域”，且属性清除。

### 2. 属性修改与自动建房测试
- 编辑设备属性，在“所属房间”中输入全新的房间名“控制中心”。
- 验证系统是否自动生成“控制中心”房间，且该设备自动对齐排列在“控制中心”内。
- 将多个设备的房间属性批量修改为“控制中心”，验证设备是否在房间内整齐网格排列。

### 3. 房间移动测试
- 拖拽移动“1号机房”标题栏，验证房间内所有设备图标是否平滑同步移动，相对位置保持不变。

### 4. 房间重命名与删除测试
- 将“1号机房”重命名为“主控室”，验证内部设备的属性同步变为“主控室”。
- 删除“主控室”，验证原先在“主控室”内的所有设备是否自动安全回收至顶部“未摆放设备区域”。
