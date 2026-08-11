# Implementation Plan - QT CAN Device Status Monitor Multi-Screen Linkage & Interface Segmentation

Implement multi-screen linked display and display area segmentation for the Qt-based CAN Device Status Monitoring interface (`DeviceMonitorPanel`).

## User Review Required

> [!IMPORTANT]
> **Key Architectural Features**:
> 1. **Multi-Interface Tab Segmentation (多界面分割与标签页切换)**:
>    - The monitor panel split area is managed via a tech-styled Tab Bar (`QTabWidget`).
>    - Users can set the number of split areas ($1 \sim N$) and assign custom names (e.g. "界面1: 驾驶舱", "界面2: 动力舱", "界面3: 集控室").
>    - `DeviceBitMapping` is extended with a `targetView` (所属界面) property. Device icons are filtered and rendered under their specified sub-interface tab.
> 2. **Multi-Screen Linkage (多屏联动显示)**:
>    - Uses `QGuiApplication::screens()` to detect physical monitors and listens to screen hot-plugging (`screenAdded` / `screenRemoved`).
>    - Supports a **"🖥️ 多屏联动"** toggle button. When enabled, each sub-interface tab pops out into an independent top-level sub-window (`SubMonitorWindow`), automatically positioned onto connected physical screens (Monitor 1, Monitor 2, Monitor 3...).
>    - Real-time CAN data frame dispatching (`processBatch`) updates device status widgets seamlessly across all detached multi-screen sub-windows.

## Proposed Changes

### PhudonTools Component

#### [MODIFY] [canprotocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.h)
#### [MODIFY] [canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- Add `QString targetView` field to `DeviceBitMapping` (defaulting to `"界面1"`).
- Extend `CanProtocolConfigDialog` table widget with a **"所属界面" (Belonging Interface)** column.
- Update table row addition, editing controls, and JSON serialization (`toJson()` / `fromJson()`) to persist `targetView`.

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **Interface Segmentation & Tab Bar**:
  - Add `QTabWidget` / `QTabBar` (`m_viewTabBar`) above grid/canvas area.
  - Implement sub-interface configuration dialog ("界面分割设置"), allowing users to adjust the split count and edit interface names (e.g. `["界面1", "界面2", "界面3"]`).
  - Filter `m_deviceWidgets` rendering based on the currently selected tab's `targetView`.
- **Multi-Screen Linkage System**:
  - Define `SubMonitorWindow` class (or inner window class) inherited from `QMainWindow`/`QDialog` to represent detached monitor views for secondary/tertiary screens.
  - Add `QPushButton *m_btnMultiScreen` to toolbar.
  - Dynamically detect connected physical screens via `QGuiApplication::screens()`.
  - When multi-screen mode is toggled, pop out each non-active tab into a `SubMonitorWindow` and place it on corresponding physical screen bounds (`screen->geometry()`).
  - Listen for screen hot-plugging (`screenAdded` / `screenRemoved`). When a screen is detached, automatically recall its sub-window back to the main panel as a tab.
  - Synchronize batch CAN updates (`processBatch`), alarm events, and logs across main panel and all active multi-screen sub-windows.

#### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h)
#### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- Add `targetView` field display and edit options in context menu / double-click edit dialog, allowing users to quickly assign device icons to different sub-interfaces.

---

## Verification Plan

### Automated / Build Verification
- Compile `PhudonTools` with Qt build system to verify header dependencies and MOC generation.

### Manual Verification
1. **Interface Segmentation Test**:
   - Open Device Configuration dialog. Add devices and set their "所属界面" to "界面1", "界面2", "界面3".
   - Switch tabs in `DeviceMonitorPanel` and verify that only device icons assigned to the active tab are displayed.
2. **Split Config Test**:
   - Click "界面分割设置" button, change interface count to 3, rename tabs to "驾驶舱", "动力舱", "集控中心".
   - Confirm tabs update dynamically and devices retain their assigned views.
3. **Multi-Screen Linkage Test**:
   - Click "🖥️ 多屏联动" button.
   - Verify that sub-interface windows pop out onto separate physical monitors (or tiled sub-windows on single monitor setup).
   - Send CAN frames and verify real-time status updates sync across all multi-screen sub-windows simultaneously.
