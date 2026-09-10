# SmileDisplayerPro 增强与自适应布局实施方案

## 需求概述与目标
针对 CAN 总线工作台 PRO 的设备状态监控界面，实现以下 5 项关键增强：
1. **随窗口大小自动调整分布**：窗口尺寸发生改变时，自动重新计算房间与设备的行列排布与间距，保持居中对齐；
2. **自适应不同屏幕大小、不同分辨率**：结合屏幕逻辑 DPI、视口宽高与房间总数，动态推导最优列数与最佳等比缩放比例，兼顾 1080p、2K、4K 及小屏幕笔记本，杜绝截断与重叠；
3. **信息日志可以点击关闭按钮关闭**：修复悬浮日志框初始可见性与关闭按钮无法关闭的问题，增加显著交互样式与信号联动；
4. **多屏联动可以点击开启和关闭**：修复关闭时 `updateSubWindows()` 未被调用导致副屏窗口无法关闭的缺陷，实现平滑启闭与资源回收；
5. **将设备状态监控界面名称修改为 SmileDisplayerPro**：全局重命名为 `SmileDisplayerPro`（菜单项、窗口标题、状态栏标题、副屏标题）。

---

## 详细技术方案

### 1. 随窗口大小自动调整分布 & 自适应屏幕分辨率
- **防抖自适应调度**：在 `DeviceMonitorPanel` 中引入 `m_windowResizeTimer`（120ms 防抖）。在 `resizeEvent` 与 `changeEvent(WindowStateChange)` 触发且处于视图模式（`!m_layoutEditingEnabled`）时，启动防抖定时器执行 `adaptDistributionToWindow(false)`。
- **动态列数与行列推导**：
  - 读取当前 `m_scrollArea->viewport()` 宽高与屏幕 DPI 比例；
  - 根据可用视口宽度 `availW` 智能推导最优列数：
    - `< 800px`：1 列（适配窄屏/小窗口）
    - `800px ~ 1350px`：2 列（或 1 列如果仅 1 间房）
    - `1350px ~ 1920px`：3 列（2x2 经典对称或 3 列）
    - `>= 1920px`：3~4 列（大屏/4K 黄金排布）
  - 房间列宽 `uniformColW` 智能弹性适配（480px ~ 640px），各行高度根据房间内设备数精准计算；
  - 计算视口内水平与垂直居中坐标 `startX`、`startY`；
  - 动态计算最优缩放比例 `targetZoom`：若视口较小自动收敛缩放（保底 0.40），若大屏且房间少适度饱满显示（最高 1.35），默认 1.0 原生清晰度；
  - 在 `showEvent` 中通过延迟调度执行首次屏幕自适应。

### 2. 信息日志点击关闭按钮关闭
- 在 `m_logWrapper` 构造时，确保初始状态与 `m_btnInfoLog->isChecked()` 同步（调用 `m_logWrapper->hide()`）；
- 修改 `btnCloseLog` 的点击事件：直接调用 `m_logWrapper->hide()` 并同步设置 `m_btnInfoLog->setChecked(false)`；
- 优化 `btnCloseLog` 的视觉样式与悬停效果（红色高亮背景、白色十字图标），提高易用性与响应灵敏度。

### 3. 多屏联动点击开启与关闭
- 在 `onMultiScreenToggled(bool checked)` 中，无论是开启还是关闭，均直接调用 `updateSubWindows()`：
  - 当 `checked == false` 时，`updateSubWindows()` 立即调用所有副屏窗口的 `hide()` 和 `deleteLater()` 并清理 `m_subWindows` 列表；
  - 当 `checked == true` 时，创建或定位副屏并分发渲染各个界面的房间与设备卡片；
- 为 `SubMonitorWindow` 增加关闭事件同步，确保副屏被用户单独关闭时不会引发孤儿状态或逻辑冲突。

### 4. 设备状态监控界面更名为 SmileDisplayerPro
- `devicemonitordialog.cpp`：`setWindowTitle("SmileDisplayerPro")`；
- `cantool.cpp`：菜单项更名为 `"SmileDisplayerPro"`，槽函数支持该文本触发；
- `devicemonitorpanel.cpp`：底部状态栏标题更改为 `"SmileDisplayerPro  |  v2.0  |  Phudon"`，副屏标题统一采用 `SmileDisplayerPro` 前缀。

---

## 拟修改文件清单

| 文件 | 变更说明 |
| :--- | :--- |
| `PhudonTools/cantool.cpp` | 工具菜单项添加并响应 `SmileDisplayerPro` |
| `PhudonTools/devicemonitordialog.cpp` | 对话框主标题更新为 `SmileDisplayerPro` |
| `PhudonTools/devicemonitorpanel.h` | 声明 `adaptDistributionToWindow` 与 `m_windowResizeTimer` |
| `PhudonTools/devicemonitorpanel.cpp` | 实现窗口及分辨率自适应、日志关闭修复、多屏联动启闭修复、标题更名 |

---

## 验证方案
1. **编译测试**：使用 `mingw32-make -j8` 编译 `PhudonTools`，验证 0 报错；
2. **多屏联动启闭验证**：点击工具栏“🖥 多屏联动”，验证开启后副屏正确弹出，再次点击按钮关闭，验证副屏立即隐藏并释放；
3. **日志关闭验证**：打开日志，点击日志右上角 `✕` 按钮，验证日志框立即关闭且工具栏“📟 信息日志”按钮同步恢复未勾选状态；
4. **窗口与分辨率自适应验证**：调整窗口大小（拖拽拉宽拉窄、最大化、还原），验证房间排布自动切换列数并保持居中，无卡片重叠与截断；
5. **名称验证**：检查弹窗标题、工具栏按钮菜单、副屏标题与底部状态栏，均显示 `SmileDisplayerPro`。
