# CAN设备状态监控界面：房间自动扩容、支持手动改变大小与强力一键整理实施计划

针对用户在 CAN 调试助手的设备状态监控界面中反馈的问题：
1. 房间内部设备图标拥挤重叠在一起，文本和卡片互相遮挡。
2. 房间需根据实际设备图标数量自动扩容，同时必须支持用户手动自由改变房间大小。
3. 一键整理功能需要更加强大，点击一键整理后，必须排列得整整齐齐。

---

## 🎯 根因深入剖析 (Root Cause Analysis)

通过对 [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp) 与 [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp) 的源码排查，定位到导致拥挤重叠和无法手动改变大小的核心原因：

1. **`clampDevicesInsideRoom` 坐标强制压扁导致的“叠罗汉”重叠**：
   - 当前在设备排列后或拖拽后，均会调用 `clampDevicesInsideRoom(r)`。
   - `clampDevicesInsideRoom` 计算的最大允许 Y 为：`maxY = r.geom.bottom() - PAD_BOTTOM - CARD_H`。
   - 由于房间默认高度（或加载的高度）不足以容纳多行设备（如 4 行设备需要 540px，而房间实际高度仅 260px 或 140px），导致第 2 行、第 3 行、第 4 行设备的期望 Y 坐标全部超过了 `maxY`。
   - `qBound(minY, py, maxY)` 将后续所有行的设备 Y 坐标强制压制到了同一个 `maxY` 像素点上，导致全部设备卡片上下重叠叠在一起。
2. **`ensureRoomCapacity` 无条件强制覆盖用户自定义尺寸，导致无法手动改变房间大小**：
   - 在 `onRoomResized`、`rebuildRoomCanvas`、`autoArrangeRoomDevices` 中，`ensureRoomCapacity(r)` 内部无条件执行了：
     `r.geom.setSize(stdSize);`
   - 当用户在界面上手动拉大或调整房间大小时，`onRoomResized` 虽然收到了用户的新几何尺寸，但下一行立刻被 `ensureRoomCapacity` 暴力重置回 `stdSize`，导致用户的手动缩放完全失效或被还原。
3. **“一键整理”算法粗糙无规则，未实现工业级对齐**：
   - 现有一键整理仅仅按房间在列表里的无序顺序，使用 `curX + roomW > 1400` 进行简单的横向累加流式排列。
   - 房间未进行自然字母数字排序（如 `1C`、`2C1层`、`2C2层`、`2C3层`...散乱），各房间宽度长短不一，左右边缘凹凸不齐。
   - 换行时受前一行最高房间影响，短房间下方出现大面积空白断层，下行房间的左右边线无法与上行对齐，整体杂乱无章。

---

## 🛠️ 详细重构设计方案

### 1. 房间自动扩容与设备优雅排布 (`layoutDevicesInRoom`)
- **边距与安全间距规范**：
  - 卡片标准尺寸：`CARD_W = 96`, `CARD_H = 106`
  - 卡片水平间距：`GAP_X = 16`，垂直间距：`GAP_Y = 16`
  - 房间安全内边距：`PAD_LEFT = 20`, `PAD_RIGHT = 20`, `PAD_TOP = 54`（避让顶层标题和数量标签），`PAD_BOTTOM = 20`
- **动态列数与自适应扩容算法**：
  - 根据房间实际宽度 `availW = r.geom.width() - PAD_LEFT - PAD_RIGHT`，计算当前最大可容纳列数：
    `maxColsFit = qMax(1, (availW + GAP_X) / (CARD_W + GAP_X))`
  - 确定最佳列数 `cols = qMax(1, qMin(maxColsFit, devCount))`，行数 `rows = (devCount + cols - 1) / cols`。
  - 计算所需的最小高度：
    `reqH = PAD_TOP + PAD_BOTTOM + rows * CARD_H + (rows > 1 ? (rows - 1) * GAP_Y : 0)`
  - **自动扩容**：若 `r.geom.height() < reqH`，自动将房间高度扩容至 `reqH`；若 `r.geom.width() < reqW`，自动将宽度扩容至 `reqW`。
- **全居中与均匀呼吸感**：
  - 计算整体网格尺寸与房间剩余空间的居中偏移量：
    `startY = r.geom.y() + PAD_TOP + qMax(0, (availH - gridH) / 2)`
  - 每行单独水平对称居中：若末行只有 2 个设备，自动水平居中于房间下方，绝不偏斜。
  - 所有计算得出的卡片坐标天然位于安全矩形内部，杜绝一切挤压与重叠。

### 2. 支持手动自由缩放房间大小 (`onRoomResized` & `ensureRoomCapacity`)
- **重构 `ensureRoomCapacity(RoomRegion &r)`**：
  - 改变原有的“强制覆盖尺寸”，改为“保底扩容保护”：
    ```cpp
    QSize minReq = standardRoomSizeForCount(devCount, r.shape);
    r.geom.setWidth(qMax(r.geom.width(), minReq.width()));
    r.geom.setHeight(qMax(r.geom.height(), minReq.height()));
    ```
  - 当用户手动将房间拉大时，保留用户设置的宽高；仅当房间小于容纳设备的最小尺寸时，才进行自动扩容。
- **动态重排与最小尺寸限制 (`RoomWidget`)**：
  - `RoomWidget` 的 `setMinimumContentSize(minReq.width(), minReq.height())` 随内部设备数动态更新，防止用户在拖拽时将其缩小到导致设备溢出的尺寸。
  - 当用户在布局模式下拖动缩放手柄并释放时（`onRoomResized`），根据新的宽度重新计算列数并重排设备，实现“拉宽自动增加列数，变窄自动折行扩充高度”的流畅弹性布局。

### 3. 全新强力“一键整理”功能 (`autoArrangeRoomsAndDevices`)
- **自然自然序列智能排序**：
  - 引入 `QCollator(setNumericMode(true))`，对当前界面的所有房间按名称进行自然数字字典排序：
    `1C` $\to$ `2C1层` $\to$ `2C2层` $\to$ `2C3层` $\to$ `2C4层` $\to$ `3C1层` $\to$ `3C2层` $\to$ `3C3层` $\to$ `3C4层`。
- **统一列宽、左右严格对齐（标准两列/多列仪表盘流）**：
  - 计算当前界面所有房间所需的最大宽度 `maxReqW`，统一定义标准列宽 `uniformW = qMax(560, maxReqW)`（560px 可完美容纳 4~5 列设备并留出优美边距）。
  - 第 0 列房间左边缘统一对齐 $X_0 = 40$；第 1 列房间左边缘统一对齐 $X_1 = 40 + \text{uniformW} + 40$。
  - 左右两列所有房间的外边框竖线在像素级完全重合对齐！
- **行间对齐与基线规整**：
  - 每一行两列对应的房间（如 `1C` 与 `2C1层`、`2C2层` 与 `2C3层`）顶部 Y 坐标完全一致。
  - 该行两个房间的高度统一匹配该行所需的最大高度 `maxRowH`，使同一行的两个房间拥有完全相同的矩形外观，底线完全对齐！
  - 内部设备自动居中于规整的大房间框内，呈现工业级、高科技感的监控看板！

---

## 📂 涉及修改的文件

1. **[PhudonTools/roomwidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) & [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp)**
   - 优化 `setMinimumContentSize` 与 `RoomWidget::mouseMoveEvent` 缩放限制，确保手动改变大小时边界平滑。
2. **[PhudonTools/devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)**
   - 规范布局边距常量（`PAD_TOP = 54`, `PAD_BOTTOM = 20`, `PAD_LEFT = 20`, `PAD_RIGHT = 20` 等）。
3. **[PhudonTools/devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)**
   - 重构 `standardRoomSizeForCount`：支持根据设备数自适应计算 1~5 列紧凑规整尺寸。
   - 重构 `ensureRoomCapacity`：保底扩容保护，支持手动尺寸保留。
   - 重构 `layoutDevicesInRoom`：精确几何网格居中算法，杜绝卡片重叠、溢出与压扁。
   - 重构 `autoArrangeRoomsAndDevices`：强力一键整理，自然排序、统一列宽、行基线对齐、工业级整齐看板。
   - 升级 `onRoomResized`：手动改变房间大小后内部设备动态重新计算并弹性排布。

---

## 🧪 验证与测试计划

1. **编译测试**：
   - 在 `m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools` 目录下执行 `mingw32-make -f Makefile.Release`，确保 0 编译错误。
2. **排布与防重叠验证**：
   - 验证包含 4 个设备（2C4层）、7 个设备（1C）、8 个设备（2C1层/2C3层/3C1层/3C3层）、15 个设备（3C2层）、17 个设备（2C2层）的房间。
   - 检查卡片之间是否具有明确清晰的 16px 间隔，文本和 LED 图标完全清晰展示，不发生任何层叠。
3. **自动扩容与手动缩放测试**：
   - 拖拽房间边缘的 8 个缩放手柄，验证房间尺寸可以被用户自由手动拉大并保持该尺寸，不会被重置。
   - 验证用户拉大房间后，房间内部设备会自动铺开为更多列并保持居中。
   - 验证用户缩小时，最小尺寸被设备包围盒限制，卡片永不穿透边框。
4. **一键整理效果验证**：
   - 点击“一键整理”按钮，验证当前界面的所有房间是否立刻按两列严整对齐、左边界与右边界完全笔直、行高整齐划一。
