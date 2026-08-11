# 设备标签扩容、一键界面规整与逼真全屏场景模板实施计划

本计划旨在实现用户提出的 3 项全新需求：
1. 设备图标标签名称扩大支持最高 **32个中文字符**（包含弹窗编辑限制、卡片多行/省略绘制、Tooltip 悬浮全称提示）。
2. 新增 **“一键整理”** 功能，一键自动规整当前界面的所有房间与内部设备图标。
3. 升级 **导入模板功能**：导入特定场景时，场景逼真矢量轮廓背景图**最大化全屏铺满**整个画布，轮廓精细逼真。

---

## 🎯 需求拆解与架构设计

### 1. 设备图标标签名称支持 32 个中文字符 (Requirement 1)
- **编辑输入限制**：
  - 在 `DeviceMonitorPanel::onEditDeviceRequested` 与 `CanProtocolConfigDialog` 的标签输入框 (`QLineEdit`) 中，设置 `setMaxLength(32)`，允许用户输入最高 32 个字符。
- **卡片控件渲染与悬浮提示 (`DeviceStatusWidget`)**：
  - 设置完整 `m_label` 并更新悬浮 Tooltip：格式为 `[32字完整标签]\nID: #X | CAN: 0xYYY`。
  - 在 `renderCache()` 离屏绘制中，优化名称绘制逻辑：采用 `TextWordWrap` 结合 `QFontMetrics::elidedText`，使长标签在卡片上优雅展示，不超出卡片边界。

---

### 2. “一键整理”界面房间与设备 (Requirement 2)
- **计算逻辑 (`autoArrangeRoomsAndDevices`)**：
  - 遍历当前界面中所有可见房间 `m_rooms`。
  - 按标准网格（例如 2 列布局，房间宽高 `380x260`，间距 `40px`）重新计算并对齐房间坐标 `geom`。
  - 对每个房间调用 `autoArrangeRoomDevices()`，将其内部关联的所有设备图标按 2 列阵列规整排列于房间内部，并根据设备数量自动自适应扩展房间高度。
  - 自动重绘画布、更新视口缩放与未摆放设备停靠区，并触发 JSON 配置保存。
- **UI 入口**：
  - 在布局模式悬浮控制面板 (`m_layoutFloatingDialog`) 中添加 `🧹 一键整理` 按键。
  - 在主面板工具栏添加 `🧹 一键整理` 快捷按钮。

---

### 3. 全屏最大化逼真场景模板背景 (Requirement 3)
- **画布最大化**：
  - 当选择或导入特定场景模板（核潜艇、写字楼、水面舰船、航母）时，画布 (`m_gridContainer`) 自动拉伸铺满全屏视口（例如 `1920x1080`），实现场景的全屏置顶最大化展示。
- **逼真轮廓矢量绘制 (`paintTemplateBackground`)**：
  - 重写/扩展画布的背景绘制逻辑，用高精度 `QPainterPath` 绘制具象轮廓背景：
    - **🚢 核潜艇 (Submarine)**：逼真流线型水滴艇体、艇艏鱼雷仓、指挥塔水翼、反应堆屏蔽层、艇艉七叶大侧斜螺旋桨，配以深蓝发光水质线条。
    - **🏢 写字楼 (Building)**：立体建筑楼层轮廓、玻璃幕墙分割、电梯井道与服务器机房结构线条。
    - **🛥️ 水面舰船 (Warship)**：隐身驱逐舰艇艏、主炮塔轮廓、舰桥甲板、烟囱排气口与直升机起降甲板。
    - **🛫 航母 (Carrier)**：带斜角飞行甲板的巨型航母外廓、弹射器导轨线、舰岛指挥塔、降落阻拦索与机库升降机切边。

---

## 🛠️ 涉及修改的文件

1. **[PhudonTools/devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h) & [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)**
   - 支持最高 32 字符，升级 Tooltip 与离屏 TextWordWrap/Elide 渲染。
2. **[PhudonTools/devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) & [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)**
   - 实现 `autoArrangeRoomsAndDevices()` 一键整理算法。
   - 升级 `applyLayoutTemplate()`，最大化全屏铺满画布，实现 4 种高精逼真场景矢量背景绘制。
3. **[PhudonTools/canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)**
   - 标签输入框限制统一升级为 32 字符。

---

## 🧪 验证计划

1. **32 字符设备名称测试**：
   - 输入长达 32 个中文字符的设备标签（如`1F大厅3号管道区域智能式烟雾感应火灾探测报警器001`）。
   - 验证配置保存、卡片绘制与鼠标悬浮 Tooltip 是否完整展示。
2. **一键整理测试**：
   - 将房间和设备拖拽至交错混乱状态，点击 `🧹 一键整理`。
   - 验证所有房间是否自动整齐排列为多列网格，内部设备是否自动对齐。
3. **逼真全屏场景模板测试**：
   - 切换导入“🚢 核潜艇”、“🏢 写字楼”、“🛥️ 水面舰船”、“🛫 航母”等模板。
   - 验证背景是否铺满整个全屏画布，轮廓是否逼真且房间与背景契合。
