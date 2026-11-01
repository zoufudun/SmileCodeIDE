# CANTool UI & Functional Enhancements Implementation Plan

## Goal
Optimize device/channel configuration dialogs and monitor integration:
1. Integrate device/channel manager dialog theme to match the active system theme.
2. Support channel filter configuration in `CanDeviceDialog` once channels are started/running.
3. Relocate `Status Monitor` from toolbar button to the "工具" (Tools) menu button.
4. Implement real-time "CAN 总线利用率" (CAN Bus Utilization) monitoring panel with two side-by-side plots, rate/usage metrics, and data exporting.

---

## User Review Required

> [!IMPORTANT]
> The separate `Status Monitor` button on the main toolbar is removed and integrated as an option under the `工具` (Tools) menu.
>
> In `CanDeviceDialog` and `StartChannelDialog`, hardcoded background colors and button stylesheets are removed so they can blend seamlessly with user themes (Dark, Light, Dracula, Nord, GitHub Dark).
>
> Software-based frame filtering is applied inside the polling loop of `CanInterface` when channel filter configuration is active.

---

## Proposed Changes

### Component 1: CAN Interface (CanInterface)
#### [MODIFY] [caninterface.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.h)
#### [MODIFY] [caninterface.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.cpp)

- Add `CanFilterRule` struct representing filter mode, start ID, and end ID.
- Add fields/methods in `CanInterface` to get/set channel configs and active filter rules:
  ```cpp
  CanChannelConfig channelConfig(int channel) const;
  void setChannelConfig(int channel, const CanChannelConfig &cfg);
  void setChannelFilters(int channel, const QList<CanFilterRule> &rules);
  QList<CanFilterRule> channelFilters(int channel) const;
  ```
- Implement `bool matchesFilter(int channel, const CanFrame &frame) const` checking if a frame's ID and type match any of the configured rules for that channel.
- Update `pollReceive()` to invoke `matchesFilter()` before emitting `frameReceived()`.

---

### Component 2: Device Dialog & Channel Filter Configuration (CanDeviceDialog)
#### [MODIFY] [candevicedialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/candevicedialog.h)
#### [MODIFY] [candevicedialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/candevicedialog.cpp)

- **Theme Integration**:
  - Remove hardcoded backgrounds (e.g. `background-color: white;`) from `QDialog`, `StartChannelDialog`, `QTreeWidget`, `QComboBox`, and `QLineEdit`.
  - Remove color codes (e.g. `#D6E8FC`, `#ADC3E6`) from button styles so that buttons automatically inherit active theme styling (accent/hover/disabled colors).
  - Implement `applyThemeStyle(const QString &qss)` in `CanDeviceDialog` setting the stylesheet on itself and passing it to child dialogs.
- **Filter settings**:
  - Implement `FilterSettingsDialog` as a private class in `candevicedialog.cpp`.
    - Features: `QTableWidget` table displaying active rules, `QComboBox` for mode (`标准帧明确ID`, `扩展帧明确ID`, `标准帧段ID`, `扩展帧段ID`), `QLineEdit` for Start/End hex values (auto-disabling/enabling End ID based on mode), `添加`, `删除`, `确定`, `取消` buttons.
  - Add `btnFilter` to channel items in the device tree next to start/stop buttons.
  - Enable/disable the `滤波` button based on whether the channel is running.
  - Connect the `滤波` button to slot `onFilterSettingsClicked(int channel)` which executes `FilterSettingsDialog` and saves rules to `CanInterface`.

---

### Component 3: Toolbar & Tools Setup (CANTool)
#### [MODIFY] [cantool.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.h)
#### [MODIFY] [cantool.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.cpp)

- Remove the `Status Monitor` (index 8) button from the toolbar.
- Append `"Status Monitor"` action into `menuTools` in `createToolbar()`.
- Update `onToolsTriggered()` to open the status monitor dialog when "Status Monitor" action is clicked.
- When applying theme in `CANTool::applyTheme()`, pass `m_currentStyle` to `m_deviceDialog`.
- Refactor `onChannelUtilization()` to instantiate and display the `CanBusUtilizationDialog` rather than a static message box.

---

### Component 4: CAN Bus Utilization (CanBusUtilizationDialog)
#### [NEW] [canbusutilizationdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/canbusutilizationdialog.h)
#### [NEW] [canbusutilizationdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/canbusutilizationdialog.cpp)
#### [MODIFY] [STM32IDE.pro](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/STM32IDE.pro)

- Implement a side-by-side plot layout using `QCustomPlot`.
- Left and right charts both have:
  - Channel selection drop-down populated dynamically based on active device.
  - Labels showing the calculated `当前速率: X 帧/秒` and `当前利用率: Y.YYYY %`.
  - A real-time scrolling curve representing the utilization history.
- Bottom settings area:
  - `实时保存` button to save the monitored data points to a CSV spreadsheet.
  - `刷新时间` dropdown (combobox) with 200 ms, 500 ms, and 1000 ms options.
- Integrate the real-time frame rates computation in a timer event utilizing `QElapsedTimer`.

---

## Verification Plan

### Automated Tests
- Run `mingw32-make -j8` to compile all source files and check for compilation errors.

### Manual Verification
1. Open "设备管理" dialog. Check if background, list, buttons, and combo boxes seamlessly match the selected theme (Dark/Light/Dracula/Nord).
2. Start a channel in the tree list. Confirm that the `滤波` button for that channel becomes enabled.
3. Click `滤波`. Configure multiple standard/extended exact and range ID filter rules. Verify that only matching frames appear in information logs.
4. Click the `工具` (Tools) toolbar button. Check that `Status Monitor` is now in the drop-down menu and launches the monitor panel correctly.
5. Click `通道利用率` (Channel Utilization). Verify that the dual real-time total bus utilization charts show dynamic plots. Change refresh interval, and click `实时保存` to check exported spreadsheet data correctness.
