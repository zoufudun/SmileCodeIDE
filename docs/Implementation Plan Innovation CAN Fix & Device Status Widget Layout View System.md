# Implementation Plan: Innovation CAN Fix & Device Status Widget Layout/View System

This implementation plan addresses two key user requirements:
1. **Fix Innovation CAN Device Opening Failure** in the CAN Debug Assistant (`CANTool` / `CanInterface`).
2. **Device Status Widget Layout/View System & Crash Fix**:
   - Restore the Device Status Widget layout to its original clean state.
   - Introduce a "布局/视图" (Layout/View) button to toggle layout mode.
   - Display a floating control box ("悬浮框") when layout mode is enabled, providing quick actions for: New Room, Import Layout Template, Delete Room, Import Layout File, Export Layout File, Save Layout, etc.
   - Completely eliminate interface/tab switching crash issues.

---

## Proposed Technical Changes

### 1. [Component: CAN Device Interface (`PhudonTools/caninterface.cpp`)]

#### [MODIFY] [caninterface.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/caninterface.cpp)
- **Directory Context & DLL Search Path Fix**:
  - Move `QDir::setCurrent(appDir)` and DLL search path configuration to the beginning of `CanInterface::openDevice()`, before any call to `vciOpenDevice()` or `openDevice()`.
  - Call `SetDllDirectoryW()` for both `appDir` and `appDir/CXCAN` so Windows DLL loader can resolve ControlCAN secondary driver dependencies (`kerneldlls`, `ControlCAN.dll`).
- **Device Type Fallback & Retry Logic**:
  - For Innovation CAN (`isControlCanDevice(deviceType)`):
    1. Try `vciOpenDevice(mapCxDeviceType(deviceType), deviceIndex, 0)`.
    2. If opening fails, try `vciOpenDevice(deviceType, deviceIndex, 0)`.
    3. If both fail and `zlgcan` SDK is loaded, attempt `ZCAN_OpenDevice` as fallback.
    4. Store the actual device type code that successfully opened in `m_d->cxDevType` for matching calls to `vciInitCan`, `vciStartCan`, `vciTransmit`, `vciReceive`, and `vciReadBoardInfo`.

---

### 2. [Component: Device Monitor Panel (`PhudonTools/devicemonitorpanel.h`, `PhudonTools/devicemonitorpanel.cpp`)]

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
- Add layout mode state: `bool m_layoutEditingEnabled = false;`.
- Add floating layout dialog handle: `QDialog *m_layoutFloatingDialog = nullptr;`.
- Add layout helper methods: `toggleLayoutEditingMode(bool enable)`, `showLayoutFloatingBox()`, `clearLayoutItems(QLayout *layout)`.

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **Fix Interface Switching Crash**:
  - Fix memory corruption bug in `clearUnplacedDock()`: remove `delete lay;` which destroyed `m_unplacedContainer`'s layout pointer and caused dangling pointer crashes on layout events during tab switches.
  - Create a permanent `QHBoxLayout` for `m_unplacedContainer` once during `setupUi()`, and only clear `QLayoutItem`s without deleting the layout object.
  - Guard `onTabChanged(int index)` against out-of-bounds indices and reentrancy.
- **Restore Original Layout & View Toggle**:
  - Create a prominent "布局/视图" (Layout/View) button on the toolbar (`m_btnLayoutMenu`).
  - In View Mode (`m_layoutEditingEnabled == false`), hide room resize handles/delete buttons and keep card dock hidden or locked in original clean view.
  - In Layout Mode (`m_layoutEditingEnabled == true`), enable room handle interactions and pop up the Floating Layout Control Window ("悬浮框").
- **Floating Control Window ("悬浮框")**:
  - Implement a sleek, dark-themed floating dialog containing action buttons:
    - ➕ **新建房间** (New Room)
    - 📐 **导入布局模板** (Import Layout Template - Submarine, Building, Warship, Carrier, None)
    - 🗑️ **删除房间** (Delete Room)
    - 📥 **导入布局文件** (Import Layout File)
    - 📤 **导出布局文件** (Export Layout File)
    - 💾 **保存布局** (Save Layout)
    - ✖ **完成/关闭** (Finish / Exit Layout Mode)

---

### 3. [Component: Room Widget (`PhudonTools/roomwidget.h`, `PhudonTools/roomwidget.cpp`)]

#### [MODIFY] [roomwidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.h) & [roomwidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/roomwidget.cpp)
- Add `setEditingEnabled(bool enable)` method to show/hide edit controls (rename, delete buttons, resize handles) based on whether layout editing mode is active.

---

## Verification Plan

### Automated Verification
1. Build the application using Qt `qmake` & `make` / `mingw32-make` or verify syntax with Qt build logs.
2. Confirm clean compilation without warnings or syntax errors.

### Manual Verification
1. **Innovation CAN Verification**:
   - Open Device Management in CAN Debug Assistant.
   - Select "创芯 USBCAN-2C" / "创芯 USBCAN-1C" / "创芯 USBCAN-2E-U".
   - Click "打开设备" (Open Device) and verify that the device opens successfully.
2. **Layout/View Toggle & Floating Box Verification**:
   - Open Device Status Widget ("Status Monitor").
   - Confirm layout defaults to clean original view mode.
   - Click "布局/视图" button to enable Layout Mode.
   - Verify floating box ("悬浮框") pops up with options for New Room, Import Layout Template, Delete Room, Import Layout File, Export Layout File, and Save Layout.
3. **Tab/Interface Switch Crash Verification**:
   - Perform rapid switching between Tab 1, Tab 2, Tab 3 ("界面1", "界面2", "界面3").
   - Confirm no crashes, segfaults, or freeze issues occur.
