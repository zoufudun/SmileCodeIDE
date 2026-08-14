# Implementation Plan — 设备房间交合自动入舱、设备统一尺寸与启动界面美化 (图1 & 图2)

根据您提交的两张截图（图 1 与 图 2），本计划针对设备与房间相交重叠、图标尺寸不一以及启动界面杂乱的问题，提出以下修复与优化方案：

---

## User Review Required

> [!IMPORTANT]
> **图 1：设备与房间交合自动入舱与尺寸严格一致**
> - **问题现象**：设备图标在拖拽或放置时，边缘与房间边界重叠交合（如图 1 所示），且未被纳入房间网格；部分设备卡片尺寸偏大或与房间比例不符。
> - **解决方案**：
>   1. **交合自动入舱算法**：在 `onDeviceDragged` 与拖拽 drop 事件中，将相交判断升级为包围盒相交检测 `r.geom.intersects(devRect)`。只要设备图标与房间有任何交合或重叠，系统立即将该设备自动归入该房间内部 (`m.targetRoom = r.name`)，并自动重新触发房间内置网格对齐，使设备 100% 居中排列在房间内部，绝对不会落在房间边框上重叠。
>   2. **设备图标严格统一尺寸**：彻底消除代码中残留的 `128x155` 旧尺寸，将主画布、未摆放侧边栏、子窗口中的所有设备图标规格强制统一为 **`96px × 106px`**（缩放时严格按 `96 * zoomLevel × 106 * zoomLevel` 等比例同频缩放）。

> [!NOTE]
> **图 2：启动界面尺寸调整与防杂乱悬浮错位修复**
> - **问题现象**：
>   1. 界面启动时日志窗口 `[WebSocket] 服务器已启动...` 浮在左上角，遮挡了顶部切页 TabBar 和工具栏按钮；
>   2. 启动时初始窗口尺寸过小 (`850x600`)，导致画布被迫缩小到 51% 缩放比例，呈现杂乱塌陷。
> - **解决方案**：
>   1. **启动窗口合适尺寸**：将监控对话框启动默认尺寸调增至 **`1360px × 850px`**，避免首次打开时界面过于拥挤；
>   2. **切页 TabBar 嵌入工具栏**：将视图切页按钮栏直接内嵌至顶部工具栏 `m_toolbar` 中整齐排列，不再浮在画布左上角遮挡按钮；
>   3. **日志框定位归位**：修复 `repositionFloatingWidgets()`，强制将日志悬浮框锁定在界面**右下角**，绝对禁止遮挡顶部工具栏与切页按钮。

---

## Proposed Changes

### 1. 设备与房间交合自动入舱与统一尺寸 (图 1)

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 修改 `onDeviceDragged()`：
  - 更新相交逻辑：使用 `r.geom.intersects(devRect)` 替代单点 `contains(devCenter)`；
  - 只要设备与房间相交，立即更新 `m.targetRoom = r.name`，并调用 `autoArrangeRoomDevices()` 将其无缝吸附并排布在房间内网格中；
  - 清除 `128x155` 旧尺寸代码，全线使用 `96x106` 标准卡片尺寸。

#### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h) / [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- 确保 `sizeHint()` 与 `minimumSizeHint()` 严格返回 `QSize(96, 106)`，消除任何不一致的膨胀绘制。

---

### 2. 启动界面尺寸与悬浮组件布局对齐 (图 2)

#### [MODIFY] [devicemonitordialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitordialog.cpp)
- 将构造函数中的窗口初始尺寸从 `resize(850, 600)` 调整为 `resize(1360, 850)`。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **切页栏嵌入工具栏**：在 `setupUi()` 中将切页 Tab 按钮组 directly 放入 `tbLayout` 工具栏布局中，避免悬浮在画布上方；
- **日志框右下角强锁定**：在 `repositionFloatingWidgets()` 中，重新计算 `m_logWrapper` 坐标，确保其初始与拖拽约束均位于右下角 `(width() - logW - 20, height() - logH - 45)`，不得遮挡顶部导航。
- **画布基础尺寸精细化**：根据房间与设备的实际包围盒动态计算 `m_baseCanvasW` / `m_baseCanvasH`，使启动时默认缩放比例保持为 **100% (1.0)**。

---

## Verification Plan

### Automated Build Verification
- 执行 Makefile 编译构建：
  ```powershell
  mingw32-make -f Makefile.Release
  ```

### Manual Verification
1. **图 1 校验（设备入舱与尺寸）**：
   - 拖拽任意设备图标边缘触碰房间边界；
   - 验证设备是否会自动落入该房间内部网格并吸附对齐，不再出现设备与房间线条相交重叠的情况；
   - 检查界面上所有设备图标，确认其尺寸完全一致（96x106 比例）。
2. **图 2 校验（启动界面整洁度）**：
   - 以默认模式启动设备状态监控界面；
   - 验证窗口尺寸是否充裕 (1360x850)，顶部工具栏与视图切页 Tab 栏是否清晰排列且无任何遮挡；
   - 确认日志框位于右下角，无浮在左上角遮挡导航按钮的现象。
