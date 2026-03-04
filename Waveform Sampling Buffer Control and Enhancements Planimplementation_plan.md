# Waveform Sampling Buffer Control and Enhancements Plan

## Goal Description
Enhance the serial port debugging tool's waveform display by introducing historical buffer control, a scrollbar for navigating past data, horizontal axis time unit configuration, and explicit channel visibility controls using an eye icon.

## User Review Required
> [!IMPORTANT]
> - **Time Units Logic:** The horizontal axis currently increments by `1` for each data frame. To support actual "Time Units" (e.g., ms, s), does the software assume a fixed sampling interval (e.g., 1 point = 1 ms), or should the UI just change the axis label and let the user interpret it based on their send rate? I propose adding a generic multiplier or simply appending the chosen unit to the axis label text for now.
> - **Eye Icon Location:** `QCustomPlot`'s built-in legend does not natively support placing standard Qt widgets (like an eye icon button) directly inside it. I propose adding a dynamic "通道管理" (Channel Management) list inside the `绘图设置` (Settings) Dock on the left side. Each channel will have a name, color indicator, and an eye icon toggle button. Is this acceptable?
> - **Scrollbar placement:** I will embed a `QScrollBar` immediately below the waveform drawing area (`QCustomPlot`).

## Proposed Changes

### UI & Architecture Updates
#### [MODIFY] serialportplot.h
- **Scrollbar**: Add `QScrollBar *m_scrollbarWaveform;` and a layout wrapper to contain both `m_customPlot` and the scrollbar.
- **Buffer Control**: Add `QSpinBox *m_spinBufferLimit;` for max history size, and rename the logic of `m_spinPoints` to mean "X-axis View Width ($\Delta t$)".
- **Time Units**: Add `QComboBox *m_comboTimeUnit;` (options: Points, ms, s).
- **Channel List**: Add a `QVBoxLayout *m_channelsLayout;` to hold dynamically generated channel toggles with eye icons.

#### [MODIFY] serialportplot.cpp
- **[setupUi](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#54-632) / Dock Settings Setup**:
  - Insert the `m_spinBufferLimit` and `m_comboTimeUnit` into `m_dockSettings`.
  - Wrap `m_customPlot` in a `QVBoxLayout` alongside `m_scrollbarWaveform`.
  - Add a "通道管理" GroupBox into `m_dockSettings` to list the channels and their eye toggle buttons.
- **[updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1049-1143)**:
  - Modify `data()->removeBefore(...)` to use `m_spinBufferLimit->value()` instead of `m_spinPoints->value()`.
  - If the scrollbar is at its physical maximum (tracking latest data), update the scrollbar max and value automatically, keeping the view fixed to the latest `m_spinPoints` width.
  - If not at maximum (user scrolled back), freeze auto-scrolling and only update the historical X-axis range based on the scrollbar's value.
  - Dynamically instantiate row items with eye icons in `m_channelsLayout` when new channels are created.

## Verification Plan

### Automated Tests
- N/A (UI and serial port features usually require manual validation in this project framework).

### Manual Verification
1. Open the Serial Session and enable the waveform plot.
2. Send `$ 100 200 ;` continuously to simulate data.
3. **Buffer Limit test**: See if memory usage stabilizes and if history is properly truncated when exceeding the configured buffer limit.
4. **Scrollbar $\Delta t$ test**: Scroll back using the new scrollbar. Verify that the plot pauses auto-updating its display (so you can view history), and that the viewable window size matches the $\Delta t$ setting. 
5. **Time Unit Config test**: Change the Time Unit Combobox and verify the X-axis label/ticks update accordingly.
6. **Eye Icon Channel Toggle test**: Click the eye icon next to a channel in the newly added channel list and verify the curve disappears/reappears.
