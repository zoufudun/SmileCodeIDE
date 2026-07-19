# Waveform UI Enhancement Plan

## Goal Description
The user wants to implement a high-tech, stunning, and modern sci-fi look for the waveform interface. We need to upgrade the `QCustomPlot` styling to match the neon glowing scrollbar we previously added.

## Proposed Changes

### [MODIFY] serialportplot.cpp (in setupChart)
We will completely overhaul the appearance of `m_customPlot`:
1. **Background**: Use a dark, slightly translucent gradient background (e.g. #1E1E28 to #282836).
2. **Axis & Grid**: 
    - Change grid lines to a subtle dashed cyan/blue color (e.g. #304050).
    - Change axis base pen lines and tick labels to a bright, readable cyan/white color.
3. **Data Lines**:
    - Make the plot lines thicker (`setWidthF(2.0f)`).
    - We will keep the distinct HSV colors but potentially adjust the saturation/value to be more neon/vibrant.
4. **Legend**: Give the legend a semi-transparent dark background and bright text, with no border.

### [MODIFY] serialportplot.cpp (in updateWaveform)
Ensure the dynamically generated channels generate bright, saturated "neon" colors so they stand out against the dark background.

## Verification Plan
### Manual Verification
1. Run the application.
2. Open the serial port and start generating waveform data.
3. Observe the aesthetics: the background should be a rich dark gradient, grids should be subtle, and channel lines should be bright and thick.
