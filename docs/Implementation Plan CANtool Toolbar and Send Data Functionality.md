# Implementation Plan: CANtool Toolbar and Send Data Functionality

This plan details the implementation of a custom premium toolbar in the CANtool and a modeless dialog for the "Normal Send" (普通发送) feature.

## Proposed Changes

We will introduce a top toolbar in `CANTool`, replace the old connection panel with a bottom status bar, and implement a dedicated, multi-tab `NormalSendDialog` for the normal sending feature.

---

### [Component 1] CANtool Main Window & Toolbar

We will create a custom toolbar layout at the top of `CANTool` styled similarly to the screenshot. We will also move the connection status and theme selectors to a status bar at the bottom.

#### [MODIFY] [cantool.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.h)
- Declare `createToolbar()` and add toolbar button pointers.
- Move theme and status controls to a new status bar layout.
- Add pointer for `NormalSendDialog` (using `QPointer` for safe automatic cleanup).

#### [MODIFY] [cantool.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.cpp)
- Reorganize `setupUi()` to:
  1. Add the toolbar widget at the top.
  2. Put the existing QTabWidget in the center.
  3. Create a bottom status bar layout containing the status label and theme selector.
- Add helper method to paint custom vector-drawn icons for the 7 toolbar options.
- Wire the button signals to their corresponding menus/actions (e.g. clicking "发送数据" or "普通发送" opens the new dialog).

---

### [Component 2] Normal Send Dialog (普通发送)

We will implement a new modeless dialog for normal sending. It supports multiple tabs, frame parameters configuration, immediate sending, sequence/list sending, parallel/serial modes, failure behaviors, and ID/data increments.

#### [NEW] [normalsenddialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.h)
- Define `NormalSendDialog` class inheriting from `QDialog`.
- Define `NormalSendPage` class inheriting from `QWidget` (represents each tab workspace).
- Declare structures and helper slots for sending, list operations, timers, and sequence controllers.

#### [NEW] [normalsenddialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.cpp)
- Implement `NormalSendDialog` containing a `QTabWidget`.
- Handle the "+" tab addition slot to create new `NormalSendPage` pages.
- Implement the "帧发送" parameters editing UI.
- Implement "立即发送" with support for single sends, count loop sends, and increments.
- Implement "添加到列表" to insert items to the TableWidget.
- Implement bottom actions:全选 (Select All), 反选 (Invert Selection), 上移/下移 (Move Up/Down), 删除/清空 (Delete/Clear), and JSON-based Import/Export.
- Implement list sending logic supporting:
  - **Serial Send Mode**: Sends each checked item sequentially.
  - **Parallel Send Mode**: Starts timers for all checked items simultaneously.
  - **Speed Multiplier**: Speeds up transmission intervals according to speed factors.
  - **Failure Handling**: Stoppage on failure or continuation.
  - **List Repeat and Intervals**: Multi-run loops with intermediate delay.
- Style controls dynamically using standard project theme values.

---

### [Component 3] Build Configuration & Core API

#### [MODIFY] [caninterface.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.h)
- Add public getter `int channel() const { return m_channel; }` to retrieve the currently active/opened channel.

#### [MODIFY] [STM32IDE.pro](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/STM32IDE.pro)
- Add `normalsenddialog.h` to `HEADERS`.
- Add `normalsenddialog.cpp` to `SOURCES`.

---

## Verification Plan

### Automated Tests
We will build the application using the project toolchain to ensure compile safety.
- Command: `mingw32-make` or Qt compilation command.

### Manual Verification
- Open the CANtool window, verify that the new toolbar is rendered with custom premium icons and a status bar at the bottom.
- Click the "发送数据" button or choose "普通发送" from its drop-down menu, and confirm that the "普通发送" dialog opens.
- Test adding new tabs by clicking the `+` button in the tabs bar.
- Test the "立即发送" button with standard/extended, data/remote frames, and verify that the frame is correctly received or shown in the logs.
- Add frames to the list, test list operations (Select All, Invert, Move Up/Down, Delete).
- Test Serial and Parallel sequence sending, verify speed multiplier, and confirm correct ID and data increments.
