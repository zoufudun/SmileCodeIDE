# 房间管理功能与所属房间下拉/输入选择增强计划 (PhudonTools)

本计划旨在满足用户提出的两项新需求：
1. 设备图标“所属房间”属性可智能检测当前存在的房间并提供下拉列表选择，同时支持直接手动输入新房间名称；
2. 新增独立“已建房间管理”对话框，以列表形式显示所有房间，并支持显示/隐藏、名称修改、颜色定制、形状选择、固定锁死（防拖拽）等全面管理功能。

---

## 🎯 需求拆解与架构设计

### 1. 设备“所属房间”属性下拉列表与自由输入增强 (Requirement 1)
- **设备单项编辑对话框 (`onEditDeviceRequested`)**：
  - 增强 `targetRoomCombo`：智能收集当前所有存在的房间名称（自动剔除重复项），充填至下拉列表。
  - 保持 `setEditable(true)`，支持用户直接下拉选择已有房间，或者直接输入新房间名。
- **协议批量编辑对话框 (`CanProtocolConfigDialog`)**：
  - 将 `m_targetRoomEdit` (QLineEdit) 升级为可编辑下拉框 `m_targetRoomCombo` (QComboBox, `setEditable(true)`)。
  - 打开协议配置时传入当前存在的房间列表，填充下拉选项；表格“所属房间”列同样支持下拉选择或手动输入。

---

### 2. 房间数据结构与渲染扩展 (Requirement 2 基础)
- **`RoomRegion` 结构体扩展** (`devicemonitorpanel.h`)：
  - 新增 `bool visible = true;` —— 是否显示房间。
  - 新增 `bool isLocked = false;` —— 是否固定不动（锁定防拖动/防缩放）。
  - 新增 `QColor color = QColor(0, 212, 255);` —— 房间主题/边框颜色（支持自定义 Hex/RGB）。
- **JSON 布局持久化扩展 (`saveRoomLayout` / `loadRoomLayout`)**：
  - 在 `monitor/roomLayout` JSON 中读写 `visible`、`isLocked`、`color` 字段。
- **`RoomWidget` 控件增强** (`roomwidget.h` / `roomwidget.cpp`)：
  - 支持动态颜色绘制：`paintEvent` 中使用自定义 `QColor` 绘制边框与背景。
  - 支持锁定状态 (`isLocked`)：开启锁定时，禁用鼠标拖拽 (`m_dragging`) 与边框缩放 (`m_resizing`)，并在标题栏绘制 `🔒` 锁定标记。
  - 支持可见性管理：隐藏房间时，对应的 `RoomWidget` 不挂载或隐藏，房间内设备视设置展示或归回未摆放区。

---

### 3. 已建房间管理对话框 (`RoomManagerDialog`) (Requirement 2 核心)
- **新增组件**：[roommanagerdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.h) 与 [roommanagerdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.cpp)。
- **界面设计与列表展示**：
  - 采用深色科技风 `QTableWidget` 列表，展示所有已创建房间：
    1. **房间名称**（可直接编辑，修改后同步更新房间内所有设备 `targetRoom`）
    2. **所属界面**（下拉框选择 `界面1`, `界面2`...）
    3. **显示/隐藏**（QCheckBox 勾选）
    4. **固定不动**（QCheckBox 🔒 勾选锁定）
    5. **形状**（下拉框选择：矩形, 圆形, 菱形）
    6. **主题颜色**（按钮点击调出 `QColorDialog` 色盘，显示色块预览）
    7. **操作**（删除房间按钮）
  - 底部提供“新建房间”、“批量显示/隐藏”、“确定”、“取消”按钮。
- **工具栏入口**：
  - 在 `DeviceMonitorPanel` 工具栏添加“🏠 房间管理”按钮，快捷呼出管理界面。

---

## 🛠️ 涉及修改与新增的文件

### 1. [PhudonTools/roommanagerdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.h) [NEW]
- 定义 `RoomManagerDialog` 对话框类，管理 `QList<RoomRegion>` 及其交互逻辑。

### 2. [PhudonTools/roommanagerdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.cpp) [NEW]
- 实现房间列表表格、色盘选择、锁定/显隐切换、删除/新建房间及确定应用逻辑。

### 3. [PhudonTools/roomwidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) & [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp)
- 增加 `setLocked(bool)`, `setRoomColor(const QColor &)` 接口。
- 增强 `paintEvent` 颜色绘制与锁定图标绘制。
- 在 `mousePressEvent` 中增加 `isLocked` 校验，锁定状态下阻断拖拽和缩放。

### 4. [PhudonTools/devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) & [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 扩展 `RoomRegion` 结构体字段。
- 工具栏添加“🏠 房间管理”按钮并连接槽函数 `onManageRoomsRequested()`。
- 更新 `saveRoomLayout()` / `loadRoomLayout()` 读取 JSON 中的 `visible`, `isLocked`, `color`。
- 完善 `onEditDeviceRequested` 中所属房间下拉列表的自动检测与选择。

### 5. [PhudonTools/canprotocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.h) & [canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- 传入已有房间名称列表，将“所属房间”输入框升级为可编辑下拉框。

### 6. [PhudonTools/PhudonTools.pro](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/PhudonTools.pro)
- 添加 `roommanagerdialog.h` 与 `roommanagerdialog.cpp` 到 `HEADERS` 与 `SOURCES`。

---

## 🧪 验证计划

### 1. 所属房间下拉与输入验证
- 打开设备编辑框，检查“所属房间”下拉框是否列出了当前现有的所有房间。
- 验证既能直接从下拉列表中选择已有房间，也能手动输入创建新房间。
- 打开“配置协议”批量表格，验证“所属房间”列同样支持下拉选择和手动输入。

### 2. 房间管理列表功能验证
- 点击工具栏“🏠 房间管理”按钮，弹出房间管理对话框。
- **显示/隐藏测试**：取消勾选“显示”，点击应用，验证画布上的房间是否隐藏。
- **固定不动测试**：勾选“固定不动”，回到画布拖动房间标题栏或缩放手柄，验证房间是否锁定不可移动。
- **颜色设置测试**：点击颜色按钮选择新颜色（如红色或金色），验证房间边框与标题颜色是否实时变为选定色彩。
- **形状修改测试**：将矩形切换为圆形/菱形，验证画布房间渲染形状是否更新。
- **重命名与联动测试**：在列表中将房间名修改为“主机房”，点击应用，验证画布房间标题与内部所有设备的所属房间属性是否一键同步更新。
