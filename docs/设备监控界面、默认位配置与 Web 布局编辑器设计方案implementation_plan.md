# 设备监控界面、默认位配置与 Web 布局编辑器设计方案

我们将从以下三个方面改进系统：
1. **Qt C++ 监控界面美化**：优化布局与间距，引入卡片鼠标悬停发光动画，重绘高科技感探测器和阀门图标。
2. **新增“默认位/默认值”列**：在配置对话框和数据存储中增加 `defaultVal` 配置，支持在配置时对每个通道定义默认状态，在初始化时自动显示为该默认值。
3. **Electron 布局编辑器（可移动卡片与手动划分区域）**：提供“监控”与“布局编辑”双模式。在编辑模式下支持自由拖拽设备卡片，并允许手动添加、调整大小与重命名“区域底板”（Regions），布局信息本地持久化。

---

## 1. Qt C++ 状态监控界面与图标优化

### 优化方向
- **字体与对齐**：使用规范的 `Microsoft YaHei` 和 `Consolas`，统一字重与文字对齐方式。状态条在卡片底部以半透明药丸背景（Pill）呈现，文字为加粗大写。
- **微互动（悬停发光）**：重写 `DeviceStatusWidget`，重载 `enterEvent` 和 `leaveEvent` 监听悬停状态。悬停时卡片边框显示高亮青色/红色霓虹光圈。
- **重新绘制图标**：
  - **烟温探测器 (Detector)**：加入同心圆金属网格（HUD）、中央渐变发光探头、和顶部三维流线线条。
  - **阀门 (Valve)**：管道绘制添加上下对称高光渐变（3D管道效果），手轮增加阀杆传动螺纹与多方位辐条。

---

## 2. 增加“默认位/默认状态”设置

### 数据模型扩展
在 `DeviceBitMapping` 结构体和保存的数据中增加 `defaultVal` 字段：
```cpp
struct DeviceBitMapping {
  int deviceId;      // 唯一标识
  QString label;      // 名称
  QString deviceType; // 类型
  quint32 canId;      // CAN ID
  int byteIndex;      // 字节
  int bitIndex;       // 位
  int defaultVal;     // 默认值 (0: 正常/关闭, 1: 报警/开启)
};
```

### UI 与操作优化
- **配置窗口**：在 `CanProtocolConfigDialog` 的表格中增加“默认值”列（`QComboBox`，包含选项 `0 (正常/关闭)` 和 `1 (报警/开启)`）。
- **默认行配置**：在窗口上方“新增行默认值”面板中增加“默认值”下拉框，供添加新设备时作为基础参数填充。
- **数据流通**：JSON 导入/导出与 QSettings 读取均支持保存 `defaultVal`。在 Qt 和 Web 建立连接后，初始数据将直接使用该默认值。

---

## 3. Electron 端 Web 布局划分与卡片自由移动

### UI 界面改版
在 `index.html` 顶部增加模式切换控件与编辑面板：
```html
<div class="mode-selector">
  <button id="btn-mode-monitor" class="active">监控模式</button>
  <button id="btn-mode-layout">布局编辑</button>
</div>
<div class="layout-toolbar" id="layout-toolbar" style="display: none;">
  <button id="btn-add-region" class="btn-primary">＋ 添加区域</button>
  <button id="btn-reset-layout" class="btn-secondary">↺ 重置布局</button>
  <button id="btn-save-layout" class="btn-success">✓ 保存布局</button>
</div>
```

### 布局实现方式（绝对定位工作区）
- 主工作区改为 `position: relative;`。
- **自由拖拽**：在 `layout` 模式下，设备卡片和区域底板启用自定义鼠标事件拖拽。
- **自定义区域**：
  - 可以手动添加“区域面板”（Region Box）。区域面板由半透明毛玻璃面板、霓虹虚线框和顶部可编辑的标题组成。
  - 区域面板右下角有缩放拉手（Resize handle），允许调整宽高。
  - 在布局模式下可以双击区域标题重命名该区域。
- **布局持久化**：
  - 保存时，将所有区域的 `[id, name, x, y, w, h]` 以及每个设备的 `[deviceId, x, y]` 以 JSON 形式存入 `localStorage`。
  - 每次收到 Qt 推送的映射配置时，结合保存的坐标动态还原摆放。若未配置坐标，则在侧边暂存区或网格默认摆放。

---

## Proposed Changes

### [MODIFY] [canprotocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/canprotocolconfigdialog.h)
- 修改 `DeviceBitMapping` 增加 `int defaultVal` 字段。
- 声明默认值下拉框 `m_defaultValCombo`。

### [MODIFY] [canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/canprotocolconfigdialog.cpp)
- 表格列数由 `7` 扩充为 `8`。新增 “默认值” 列。
- 在 `addTableRow` 中为默认值列嵌入 `QComboBox`，当类型为 `valve` 时项为 `0 (关闭)`, `1 (开启)`；为 `detector` 时项为 `0 (正常)`, `1 (报警)`。
- 重构数据读写、JSON 序列化/反序列化以读写 `defaultVal`。

### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/devicemonitorpanel.cpp)
- 在 `rebuildGrid()` 中创建 `DeviceStatusWidget` 时，读取 `defaultVal` 并调用 `w->setStatus(m.defaultVal)` 作为初始状态。
- 在保存/读取持久化配置时支持 `defaultVal` 字段。

### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/devicestatuswidget.h)
- 增加 `bool m_hovered = false;` 变量。
- 重载 `enterEvent` 和 `leaveEvent` 方法。

### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/devicestatuswidget.cpp)
- 实现 `enterEvent` 和 `leaveEvent`，并在其中更新 `m_hovered` 并触发 `update()`。
- 在 `paintEvent` 中，当 `m_hovered` 为真时，绘制明亮的高发光霓虹边框。
- 优化 `drawDetector`：使用渐变绘制更具层次感的三维弧形透镜及防尘网格纹理。
- 优化 `drawValve`：绘制三维圆柱形阀门管道，使用环状手轮渐变及三维质感阀杆。
- 美化设备字体的展示和间距。

### [MODIFY] [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)
- 编写鼠标交互拖拽逻辑（Drag and Drop）和缩放逻辑（Resize）。
- 动态加载/保存布局坐标至 `localStorage`。
- 新增区域底板管理（添加、移动、缩放、重命名、删除区域）。

### [MODIFY] [index.html](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/index.html) & [style.css](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/style.css)
- 增加控制工具条的 HTML 标签。
- 编写区域底板、绝对定位卡片、以及控制按钮的 CSS 样式。

---

## Verification Plan

### 编译与启动验证
1. 运行 `mingw32-make` 确保无错编译。
2. 启动 `STM32IDE`，打开 CAN 调试助手，验证设备监控界面的布局、图标和字体显示是否更加美观，当鼠标悬浮在卡片上时是否有边框发光动效。
3. 点击“设备配置”，验证表格中多出“默认值”配置列，添加设备时可指定默认状态为 0 或 1。

### Web 布局划分验证
1. 启动 Electron 客户端。
2. 点击右上角“布局编辑”按钮，进入编辑模式：
   - 尝试随意拖拽设备卡片至其他位置。
   - 点击“＋ 添加区域”新建一个区域面板，拖动它并拉动右下角缩放其大小，双击标题重命名。
   - 将卡片移入区域面板内排列。
   - 点击“✓ 保存布局”。
3. 点击“监控模式”，确认此时卡片锁定，不可被拖拽。
4. 重新启动 Electron，确认之前拖动的位置和区域划分被完美恢复。
