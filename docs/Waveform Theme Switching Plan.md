# Waveform Theme Switching Plan

## Goal Description
The user wants to add an option to switch between different visual themes for the waveform interface. The requested themes include the original white background, the recently added neon dark style, and a new "Sci-Fi Oscilloscope" style that maximizes the high-tech, glowing aesthetic.

## Proposed Changes

### [MODIFY] serialportplot.h
- Add a `QComboBox *m_comboChartTheme` to store the theme selector.
- Add a new private slot `void onChartThemeChanged(int index)` to handle theme changes.
- Add a private method `void applyChartTheme(int index)` to apply the specific QCustomPlot styling.

### [MODIFY] serialportplot.cpp (UI Setup & Logic)
1. **Ui Setup ([setupUi](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#54-632))**: Add `m_comboChartTheme` to the settings layout (e.g., in the Curve Settings area). Add options: "亮色主题" (Light), "暗黑炫光" (Neon), "科幻示波器" (Sci-Fi).
2. **Connections ([setupConnections](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#777-956))**: Connect `m_comboChartTheme` `currentIndexChanged` to `onChartThemeChanged`.
3. **Theme Logic (`applyChartTheme`)**:
    - Extract the styling logic currently inside [setupChart](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#693-776) and move it into `applyChartTheme(int styleIndex)`.
    - **Style 0 (Light)**: White background, black axes, gray grids.
    - **Style 1 (Neon)**: Dark gradient background (`#1E1E28` to `#282836`), cyan axes, bright HSV colors for lines.
    - **Style 2 (Sci-Fi Oscilloscope)**: Deep black background. Intense glowing green or bright cyan axes (`#00FF41`). Dense, glowing dot/dash subgrids simulating a CRT overlay. Data lines will be extremely vibrant.
4. **Channel Colors**: Since channel colors are set when they are added, we need a method to refresh all existing graphs' pen colors when the theme changes, matching the vibrancy of the current theme.

## Verification Plan
### Manual Verification
1. Run the application.
2. Locate the new theme dropdown in the settings dock.
3. Switch between the three themes and verify the background, grids, axes, and line colors update immediately and gracefully.
