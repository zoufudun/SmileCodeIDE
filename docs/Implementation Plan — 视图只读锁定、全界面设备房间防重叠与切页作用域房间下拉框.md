# Implementation Plan — 视图只读锁定、全界面设备/房间防重叠与切页作用域房间下拉框

针对您提出的 4 项核心需求，本计划提供全套架构修改方案：

---

## User Review Required

> [!IMPORTANT]
> **1. 视图界面默认只读（禁止拖拽设备与修改布局）**
> - **原现状**：界面卡片与房间在默认查看模式下依然响应鼠标拖拽。
> - **修改方案**：在 `DeviceStatusWidget` 和 `RoomWidget` 中强化只读锁定校验。在默认视图查看模式下（`m_layoutEditingEnabled == false`），关闭设备卡片与房间的鼠标拖动/缩放响应。只有当用户明确点击 **「✏️ 布局编辑」** 按钮开启编辑模式后，才允许拖拽排布。

> [!NOTE]
> **4. 设备属性中所属房间下拉框按当前切页界面隔离**
> - **原现状**：编辑设备属性时，所属房间下拉框展示了全系统所有界面的房间名称。
> - **修改方案**：在设备编辑对话框与 CAN 通信配置表格的 `targetRoomCombo` 初始化中，增加切页作用域过滤 (`r.targetView == item.targetView`)，下拉框仅列出**该设备所在界面**的房间，隐藏其他界面的房间。

---

## Proposed Changes

### 1. 视图模式只读锁定 (需求 1)

#### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- 在 `mousePressEvent` 与 `mouseMoveEvent` 中校验 `m_draggable`（对应 `m_layoutEditingEnabled`）。若处于只读视图模式，直接忽略拖拽事件。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 确保在非编辑模式下，画布上的 `DeviceStatusWidget` 的 `setDraggable(false)`。

---

### 2. 全界面设备防重叠、交合自动入舱与尺寸一致 (需求 2)

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 在 `autoArrangeRoomDevices()` 中，对所有切页界面的设备与房间应用同样的吸附与网格对齐逻辑；
- 统一所有设备卡片几何绘制参数为 **`96px × 106px`**。

---

### 3. 全界面房间防重叠与规范间距 (需求 3)

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 升级 `resolveRoomOverlaps()`，使其遍历系统中的所有切页界面 (`m_viewNames`)；
- 在每个切页界面内独立执行多代房间碰撞消解，相交房间保持 $40\text{px}$ 规范间距自动右移/换行。

---

### 4. 设备属性房间下拉框切页隔离 (需求 4)

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 在 `onEditDeviceRequested()` 的设备属性对话框中，过滤房间列表：
  ```cpp
  for (const auto &r : m_rooms) {
    if (r.targetView.trimmed() == m.targetView.trimmed() || r.targetView.isEmpty()) {
      targetRoomCombo->addItem(r.name, r.name);
    }
  }
  ```
- 在 `canprotocolconfigdialog.cpp` 中同步过滤房间下拉菜单。

---

## Verification Plan

### Automated Build Verification
- 执行 Powershell 编译构建：
  ```powershell
  mingw32-make -f Makefile.Release
  ```

### Manual Verification
1. **只读锁定验证 (需求 1)**：
   - 在默认视图监控模式下，尝试用鼠标按住拖拽设备图标与房间框；
   - 验证设备与房间是否保持固定不动，无法改变布局。
2. **设备入舱与统一尺寸验证 (需求 2)**：
   - 检查所有切页界面下的设备图标，确认尺寸完全一致 (96x106)；
   - 在编辑模式下拖拽设备触碰房间，验证设备是否自动入舱居中对齐，绝不重叠边框。
3. **房间防重叠验证 (需求 3)**：
   - 在不同界面中添加/拖拽房间，验证所有界面的房间均保持 $40\text{px}$ 间距，无重叠。
4. **切页隔离房间下拉框验证 (需求 4)**：
   - 右键编辑某个界面（如界面 1）上的设备属性；
   - 查看「所属房间」下拉菜单，验证是否仅显示界面 1 的房间，其他界面的房间不可见。
