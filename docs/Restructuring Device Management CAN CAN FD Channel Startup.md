# Implementation Plan: Restructuring Device Management & CAN/CAN FD Channel Startup

This plan details the design and execution steps to restructure the CAN Device Management interface, implement individual channel startup options (with detailed baud rate, mode, resistor, and filtering configurations), and support both classic CAN and CAN FD.

---

## Proposed Changes

### [Component 1] CAN Core Interface (`CanInterface`)
We will refactor the backend API to split device opening from channel initialization. This enables starting/stopping individual channels independently and polling multiple active channels simultaneously.

#### [MODIFY] [caninterface.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.h)
- Add `int channel = 0;` to `CanFrame` to identify packet source/destination.
- Define `struct CanChannelConfig` for detailed channel parameters:
  - `bool isFd` (CAN vs CAN FD protocol)
  - `int abitBaud` (Arbitration baud rate)
  - `int dbitBaud` (Data baud rate)
  - `bool isIso` (CANFD ISO vs Non-ISO standard)
  - `bool enableBrs` (CANFD Acceleration BRS)
  - `CanMode mode` (Normal vs Listen-Only mode)
  - `bool terminalRes` (Enable/Disable internal terminal resistor)
  - `bool reportBusUsage` (Enable/Disable bus utilization reporting)
  - `int busUsagePeriod` (Bus utilization cycle in ms)
  - `int retrySend` (Retry count: till bus off, 3 times, or none)
  - `bool enableFilter` (Enable/Disable hardware filters)
- Refactor member functions:
  - `bool openDevice(quint32 deviceType, int deviceIndex);` (Opens the physical USB device handle)
  - `void closeDevice();` (Closes device and resets handles)
  - `bool isDeviceOpen() const;`
  - `bool startChannel(int channel, const CanChannelConfig &cfg);` (Initializes and starts a single CAN channel)
  - `bool stopChannel(int channel);` (Stops a CAN channel)
  - `bool isChannelRunning(int channel) const;`
  - Overload `bool sendFrame(int channel, const CanFrame &frame);` for multi-channel sends.
  - Implement `bool getDeviceInformation(QString *hwVer, QString *fwVer, QString *drVer, QString *libVer, int *canNum, QString *serial, QString *typeStr) const;` to query device info.

#### [MODIFY] [caninterface.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.cpp)
- Update `Impl` to store `QMap<int, CHANNEL_HANDLE> activeChans` mapping channel index to handles.
- Implement `openDevice` using `ZCAN_OpenDevice`.
- Implement `startChannel` using ZLG property API (`setValue` for baudrate, resistor, standard, BRS, usage, retries), followed by `ZCAN_InitCAN` and `ZCAN_StartCAN`.
- Implement `stopChannel` using `ZCAN_ResetCAN`.
- Modify `pollReceive()` to iterate through all keys in `activeChans`, receive packets, and set `frame.channel = channelIdx`.
- Implement `getDeviceInformation()` to call `ZCAN_GetDeviceInfoEx` and read `hardware_version`, `firmware_version`, `serial_number`, `can_channel_number`, etc.

---

### [Component 2] Device Management Dialog (`CanDeviceDialog`)
We will completely rewrite `CanDeviceDialog` to align with the first image. It will use a dual-column tree layout to display devices and channels alongside their startup action buttons.

#### [MODIFY] [candevicedialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/candevicedialog.h)
- Declare tree widget `QTreeWidget *m_deviceTree`.
- Declare Comboboxes for device type, device index.
- Declare buttons: "打开设备", "云设备", "关闭窗口".
- Declare helper slots:
  - `onOpenDeviceClicked()`
  - `onCloseDeviceClicked()`
  - `onStartChannelClicked(int channel)`
  - `onStopChannelClicked(int channel)`
  - `onShowDeviceInfoClicked()`
  - `onCloudDeviceClicked()`

#### [MODIFY] [candevicedialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/candevicedialog.cpp)
- Rewrite `setupUi()` to match the layout of the first image:
  - **Top Row Layout**: Device Type combo box, Device Index spinbox, "打开设备", "云设备", and "关闭窗口" buttons.
  - **Main Area Layout**: A 2-column `QTreeWidget`.
    - Column 0: Device name and child channels hierarchy.
    - Column 1: Row action buttons.
- Implement device tree population on device open:
  - Retrieve the device name and channel count from the interface.
  - Add a top-level item: `USBCANFD-200U 设备0`.
    - Add child items: `通道0`, `通道1`, etc.
  - Use `QTreeWidget::setItemWidget()` to place a custom widget on the right column containing a blue "启动" button and a grey "停止" button next to each row.
  - Add a "关闭设备" and "设备信息" button specifically next to the device parent row.
- **Nested Parameter Dialog (`StartChannelDialog`)**:
  - Build a scrollable `QDialog` matching the parameter screens (Images 2 & 3).
  - Collect: Protocol (CAN/CAN FD), Standard (ISO/Non-ISO), Acceleration (BRS), Arbitration & Data Baudrates, Work Mode, Terminal Resistor, Bus Usage Report, Period, and Retry limits.
  - Provide a Filter checkbox and a placeholder "滤波设置" button.
  - Return `CanChannelConfig` on confirm.
- Integrate clicks:
  - Clicking channel "启动" opens the dialog, starts the channel on confirmation, and toggles button states (Start becomes disabled, Stop becomes enabled).
  - Clicking "设备信息" pops up a formatted QMessageBox matching Image 4.

---

### [Component 3] Sending Dialog (`NormalSendDialog`)
We will adapt the normal sending workspace to route packets through the selected channels.

#### [MODIFY] [normalsenddialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.cpp)
- Fetch the selected channel index from `m_channelCombo->currentIndex()` inside `onImmediateSend()` and `executeNextSerial()`.
- Use the overloaded `m_can->sendFrame(channel, frame)` method to transmit packets to the designated channel.

---

## Verification Plan

### Automated Tests
- Build and verify compiling clean via `mingw32-make -j8`.

### Manual Verification
1. **Device Opening**:
   - Open Device Manager. Confirm type & index drop-downs are enabled.
   - Click "打开设备". Verify the left tree populates with device and channels, and action buttons appear.
2. **Channel Configuration & Startup**:
   - Click "启动" next to Channel 0.
   - Verify the scrollable parameters dialog pops up.
   - Select protocol `CAN FD` and verify acceleration, standard, and data baud rate options are shown. Select standard `CAN` and verify they are disabled/hidden.
   - Click "确认". Verify the channel starts. In the tree, Channel 0's "启动" becomes disabled and "停止" becomes enabled.
3. **Data Sending (CAN vs CAN FD)**:
   - Go to Normal Send Dialog. Configure a message for Channel 0.
   - Verify packets are successfully sent and printed in the monitor log.
4. **Device Information**:
   - Click "设备信息". Confirm a message box appears with versions, serial number, and channel counts matching Image 4.
