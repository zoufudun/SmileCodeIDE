# 房间管理优化、悬浮面板与日志框增强实施计划 (PhudonTools)

本计划旨在响应用户提出的 4 项界面与交互优化需求，彻底修复房间管理面板的功能失效问题，并全面升级控件的悬浮、定位、锁定与弹窗交互。

---

## 🎯 需求拆解与架构设计

### 1. 房间管理面板“固定”与“显示”功能修复及形象图标 (Requirement 1)
- **失效原因分析**：
  在 `RoomManagerDialog::populateTable` 中，第 2 列（显示）与第 3 列（固定）嵌入的是带居中 Layout 的容器 `QWidget`。而在按下确定保存时，代码直接对该 `QWidget` 使用 `qobject_cast<QCheckBox*>`，导致转换结果为空指针（`nullptr`），使得选中的显隐与锁定状态无法保存和应用！
- **修复方案**：
  - 在保存时正确通过 `visWidget->findChild<QCheckBox*>()` 提取 `QCheckBox` 状态。
  - 界面图标形象化升级：
    - 表头升级为 `👁️ 显示` 与 `🔒 固定`。
    - 选项文本升级为形象图标：`👁 显示` / `🙈 隐藏`；`🔒 锁定` / `🔓 自由`。

---

### 2. 布局控制悬浮面板拖动与右上角默认放置 (Requirement 2)
- **设计方案**：
  - 在 `DeviceMonitorPanel` 中开启布局编辑模式 (`toggleLayoutMode(true)`) 时，自动唤出/居顶展示悬浮布局控制盒 (`m_layoutFloatingBox`)。
  - 支持全面板**鼠标拖拽**：在悬浮控制盒标题栏添加事件监听，支持随心所欲拖拽移动。
  - **默认放置**：初始或重新打开时，定位放置于界面**右上角** (`x = width() - boxWidth - 20`, `y = toolbarHeight + 15`)。

---

### 3. 画布房间右上角增加快捷固定图标 (Requirement 3)
- **设计方案**：
  - 在 `RoomWidget` 右上角标题栏处（重命名 `✎` 按钮左侧）新增快捷固定按键 `m_btnLock` (`🔒` / `🔓`)。
  - 点击该按键时：
    - 切换 `m_locked` 状态。
    - 锁定状态下图标变为黄色 `🔒`，解锁状态下变为 `🔓`。
    - 锁定后自动禁用房间控件的边缘拉伸缩放与标题栏拖拽位移，防护误操作。
    - 触发 `roomLockToggled` 信号，`DeviceMonitorPanel` 实时保存布局并在日志中提示。

---

### 4. 事件日志面板放大缩小与右下角默认放置 (Requirement 4)
- **设计方案**：
  - **默认定位**：在 `DeviceMonitorPanel::resizeEvent` 及初始加载时，自动将 `m_logWrapper` 放置于界面**右下角** (`x = width() - logWidth - 20`, `y = height() - logHeight - bottomBarHeight - 12`)。
  - **放大/缩小控制**：在 `m_logTitleBar` 右侧控件组新增全屏/放大按键 `m_btnToggleLogSize` (`🗖` / `🗗`)。
    - 默认状态：小框模式 (尺寸 `360 x 220`)。
    - 放大状态：大框 HUD 模式 (尺寸 `640 x 420`)。
    - 切换尺寸时平滑调整其坐标位置，确保日志框始终紧贴右下角展示。

---

## 🛠️ 涉及修改的文件

### 1. [PhudonTools/roommanagerdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roommanagerdialog.cpp)
- 修复 `findChild<QCheckBox*>()` 状态提取 Bug。
- 形象化替换表头与 CheckBox 标签图标。

### 2. [PhudonTools/roomwidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) & [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp)
- 新增 `m_btnLock` 按键与 `roomLockToggled(const QString &id, bool locked)` 信号。
- 在 `RoomWidget` 右上角按钮组布局中插入 `🔒` / `🔓` 快捷切换控件。

### 3. [PhudonTools/devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) & [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 新增 `m_layoutFloatingBox` (悬浮布局控制盒)，实现拖拽与默认右上角定位。
- 增强 `m_logWrapper` 右下角自动吸附定位，添加 `m_btnToggleLogSize` 按钮与大/小框尺寸切换槽。
- 连接 `RoomWidget::roomLockToggled` 信号，实现画布锁定状态同步保存。

---

## 🧪 验证计划

### 1. 房间管理面板测试
- 打开“🏠 房间管理”弹窗，取消某个房间的“显示”，勾选另一个房间的“固定”，点击确定。
- 验证画布上的房间是否正常隐藏与锁定固定，再次打开弹窗验证勾选状态是否完好保存。

### 2. 悬浮面板拖动与定位测试
- 开启“📐 布局/视图”，验证悬浮控制盒默认出现在界面右上角。
- 鼠标拖拽悬浮控制盒标题栏，验证是否能在画布任意位置自由拖动。

### 3. 画布房间快捷锁定测试
- 直接在画布房间右上角点击 `🔓` 按钮，验证图标是否变为 `🔒` 且房间停止响应拖拽与拉伸。
- 再次点击 `🔒` 按钮，验证房间是否解除锁定恢复可拖拽。

### 4. 事件日志放大缩小与右下角定位测试
- 检查事件日志框默认是否贴合在界面右下角。
- 点击日志标题栏右上角的 `🗖` (放大) 按钮，验证日志框是否扩大至 640x420 并依然对齐右下角；点击 `🗗` (还原) 按钮，验证是否平滑恢复原始小框。
