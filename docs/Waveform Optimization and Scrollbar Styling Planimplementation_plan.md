# Waveform Optimization and Scrollbar Styling Plan

## Goal Description
The user wants to resolve UI lag when there is a large number of data points over an extended period. Second, the newly added scrollbar should have a cool, glowing (neon) effect to match a modern sci-fi/炫酷 design language.

## Proposed Changes

### 1. Scrollbar QSS Styling
#### [MODIFY] serialportplot.cpp (in setupUi)
Apply a rich, glowing QSS directly to `m_scrollbarWaveform`. We'll give it a dark translucent background with a glowing bright-cyan handle that changes brightness on hover/press.

### 2. Waveform Buffer Optimization
#### [MODIFY] serialportplot.cpp (in updateWaveform)
The current loop processes the byte array chunk line by line, identifies channels, and immediately pushes each parsed point one-by-one into `QCustomPlot` via `addData(val)`. Furthermore, it immediately calls `removeBefore` for EACH point. When plotting 10,000 to 100,000 points, this causes massive overhead.

**Optimization strategy:**
1. We will accumulate data for each channel in temporary `QVector<double> keys` and `QVector<double> values` for the parsed frames within [updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1049-1143).
2. After exiting the `while(true)` parse loop, we push the accumulated batch into `QCustomPlot` using `graph(i)->addData(keys, values)`.
3. Then, we call `removeBefore(m_xValue - bufferLimit)` only **ONCE** per channel per visual update instead of once per frame.

## Verification Plan
### Manual Verification
1. Simulate heavy incoming data (e.g. at 5ms intervals with 10 channels).
2. Check if the waveform UI remains smooth.
3. Observe the `QScrollBar` appearance and verify it features a glowing cursor layout.
