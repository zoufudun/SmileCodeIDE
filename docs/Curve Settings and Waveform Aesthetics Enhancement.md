# Curve Settings and Waveform Aesthetics Enhancement

## Problem Description
Users want to configure the drawing style of curves (solid line, dashed line, dotted line, etc.), line thickness, and color. In addition, there is a request for practical improvements to make the waveform plot more visually appealing.

## Proposed Changes

### 1. `CurveSettingsDialog` UI Component
- Define a new `QDialog` derived class `CurveSettingsDialog` in [serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h) and implement it in [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp).
- The dialog will feature a `QTableWidget` to list all currently created channels in the `QCustomPlot`.
- For each channel, the user can configure:
  - **Color**: Using a QPushButton that previews the current color and opens a `QColorDialog` on click.
  - **Line Width**: A `QDoubleSpinBox` for precision.
  - **Line Style**: A `QComboBox` with `Qt::PenStyle` options (Solid, Dash, Dot, DashDot, DashDotDot).
  - **Visibility**: A `QCheckBox` to show/hide the curve.
- Buttons for Apply, OK, and Cancel.

### 2. Modifications in [SerialSession](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#23-47) UI ([serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp))
- Add a new button `m_btnCurveSettings` labeled "曲线设置" (Curve Settings) into `m_settingsLayout`.
- Add a slot connected to `m_btnCurveSettings` that creates and shows the `CurveSettingsDialog` passing `m_customPlot`.

### 3. Aesthetic enhancements ([setupChart](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#629-659) and [updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1015-1107))
- Set `QCustomPlot` to antialias plottables (the curves). `m_customPlot->setAntialiasedElements(QCP::aeAll)`.
- Default line width of new dynamic graphs will be changed from `1` to `1.5` or `2.0` to make lines bolder and more professional-looking.
- Re-configure `m_customPlot` legend to allow interaction.
- Connect `legendDoubleClick` signal to toggle the visibility of the corresponding graph, adding a very practical tool to show/hide specific lines interactively.

## Verification Plan
1. Compile the code.
2. Ensure the "曲线设置" button appears in the Dock panel.
3. Simulate receiving data to add multiple channels.
4. Open the Curve Settings, change a channel's color and thickness, and verify the graph updates.
5. Double-click the legend to verify visibility toggles.
