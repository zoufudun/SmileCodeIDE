# Electron监控端追加优化设计方案

我们将实现以下5项升级：

1. **简约逼真动态阀门图标**：
   - 替换为**直通式蝶阀（Butterfly Valve）**。由左右贯通管路、中置圆形阀体和核心阀板组成。
   - **开启（1）** 时，阀板指示线旋转 90 度呈水平（与水流平行），管路内绿光流动。
   - **关闭（0）** 时，阀板指示线呈垂直（阻断水流，红色），管路呈灰色静止。
2. **无限滚动画布与多布局槽位另存（Save & Save As）**：
   - 将画布的虚拟尺寸扩大至 `3000px × 2000px`，支持水平与垂直方向滚动条，实现大范围自由排布。
   - 增加 **「另存布局」** 功能，支持用户命名存入自定义 Slot。
   - 增加 **「选择存盘」** 下拉框，可在保存的多套 Slot 间实时切换载入。
   - 支持 **「另存为文件 / 导入布局文件」** 用于备份。
3. **Room 房间四方向拉伸**：
   - 弃用单一的右下角缩放手柄，在 Room 的**上（T）、下（B）、左（L）、右（R）** 四个边缘均嵌入隐形的拉伸控制条。
   - 拖拽任意边缘均可单独沿对应方向改变 Room 大小或位置，并保持 10px 网格对齐。
4. **高保真核潜艇剪影背景**：
   - 对核潜艇布局进行重构。在底层绘制一艘填充了暗蓝色金属渐变、带有亮蓝色发光边框的**三维立体质感核潜艇大轮廓**。各房间和设备有序叠放在潜艇腹腔中，使整个界面呈现为一艘潜艇形态。
5. **系统时钟与双通道日志系统**：
   - 在顶部 Header 引入秒级更新的**系统时间时钟**（如 `2026-07-18 22:04:15`）。
   - 将底部日志区升级为 **「双通道日志」**：
     - **实时报警流**：记录设备报警/恢复逻辑事件。
     - **原始信息流**：打印通过 WebSocket 接收到的 raw 数据报文（包括配置和更新帧的 JSON 文本，带时间戳）。

---

## Proposed Changes

### [MODIFY] [index.html](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/index.html)
- 顶部 Header 左侧/中部增加 `<div class="system-clock" id="system-clock"></div>` 显示实时系统时间。
- 布局工具栏 `#layout-toolbar` 中增加布局另存及 Slot 选择组件：
  - `<select id="select-layout-slot" class="select-layout-template">` 载入另存的布局槽位。
  - `<button id="btn-save-as" class="btn-layout-action secondary">另存为...</button>`。
  - `<button id="btn-export-file" class="btn-layout-action secondary">📤 导出文件</button>`。
  - `<button id="btn-import-file" class="btn-layout-action secondary">📥 导入文件</button>`（带隐藏的 `<input type="file">`）。
- 底部日志 Header 中增加 Tab 切换器：
  - `<button id="tab-alarm-logs" class="log-tab active">📋 报警事件</button>`。
  - `<button id="tab-raw-logs" class="log-tab">📟 原始报文</button>`。
  - 内部放置对应的两个滚动容器 `#log-list-alarm` 和 `#log-list-raw`。

### [MODIFY] [style.css](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/style.css)
- 添加 **系统时间时钟** 的科技感字体和微光发光效果。
- 增加 Room 边缘拖拽把手样式（`.room-resize-handle-t/b/l/r`），分别对应上下左右边缘，指针分别设为 `n-resize`, `s-resize`, `w-resize`, `e-resize`。
- 修改 `.device-grid.canvas-mode`，将其固定大小设为宽 `3000px`，高 `2000px`，父容器 `.app-main` 设为 `overflow: auto;`。
- 调整阀门 CSS 类名以支持 2.0 版简约通断效果。
- 编写日志面板双 Tab 切换的布局与高亮样式。

### [MODIFY] [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)
- 编写 `updateClock` 定时器，显示完整年月日及秒级时分秒。
- 重写 `getValveSvg`，绘制带阀体、蝶阀阀板和指示线条的简约逼真图标。
- 实现四方向拉伸逻辑：在 `setupResizable` 中分别为 4 个 handles 编写 `mousedown` 事件流，动态缩放宽度/高度及平移 `left`/`top`。
- 重绘 `getSubmarineBg()`，为 Submarine 提供实心暗色渐变、亮发光线、指挥塔、水平舵及尾部螺旋桨的高精矢量背景。
- 实现 Slot 持久化管理：
  - 保存时，默认写入 `smile_code_layout_v1`。
  - 另存为时，弹窗获取名称并存入 `smile_code_layout_slot_${name}`，更新 Slot 下拉框。
  - 重置或切换下拉框时，加载对应的 Layout 数据。
  - 绑定 JSON 文件的导出和导入读取。
- 升级日志记录：在 WebSocket `onmessage` 收到数据时，将 raw 报文实时追加到 `#log-list-raw` 列表中。

---

## Verification Plan

### 1. 简约动态阀门验证
1. 打开网页，观察阀门卡片：
   - 阀门被渲染为左右相通的横向管路，中心为一个圆形控制盘。
   - **关闭（0）** 时：中置控制针呈垂直阻断方向，颜色为红色，管路内无动画。
   - **开启（1）** 时：中置控制针平滑旋转 90 度变为水平导通方向，颜色变为绿色，且整条管路内亮起平滑滚动的绿色流体光带。

### 2. 画布滚动与另存 Slots 验证
1. 画布进入编辑模式，拉动横向/纵向滚动条，确认画布可自由拉伸滚动至 `3000x2000` 像素空间。
2. 拖拽若干卡片并点击“另存为...”，输入 `测试布局_01`。
3. 再次改变一些坐标，另存为 `测试布局_02`。
4. 展开“已存布局”下拉框，在 `测试布局_01` 和 `测试布局_02` 之间切换，确认卡片坐标和房间划分完全依照所存槽位平滑还原。
5. 点击“导出文件”，保存 JSON 到本地；清空布局后选择“导入文件”，确认完美恢复。

### 3. Room 四方向边缘拉伸验证
1. 将鼠标悬浮在 Room 的上、下、左、右边缘，指针分别变更为对应的上下左右拖动箭头。
2. 拖动上边缘：房间顶部向上扩展，高度增加；
3. 拖动左边缘：房间左侧向左平移，高度/宽度保持 10px 网格吸附。四个方向均可灵活缩放。

### 4. 实时时钟与双日志通道验证
1. 观察 Header 中部，时钟精准到秒级动态跳动。
2. 触发 CAN 数据，切换日志 Tab：
   - 点击“报警事件”：显示中文报警/恢复消息。
   - 点击“原始报文”：显示收到的 compact 压缩格式原始 JSON 信息流。
