# Waveform Performance Optimization Plan

## Goal Description
The user is experiencing severe UI lag when "View Width" (`m_spinPoints`) and buffer limits are set to large numbers. The root cause is that `m_customPlot->replot()` and `rescaleValueAxis()` are currently called synchronously every single time the serial port receives data ([onReadyRead](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1180-1219)). At high baud rates, this triggers hundreds of heavy frame re-draws per second.

The goal is to decouple data reception from UI rendering, enforcing a maximum frame rate (e.g., 30 FPS) for smooth visual updates without blocking the Qt event loop.

## Proposed Changes

### [MODIFY] serialportplot.h
- Add a new `QTimer *m_replotTimer` that runs continuously when the waveform page is open at a ~33ms interval (30 FPS).
- Add a boolean flag `bool m_needsReplot` to track if new data has arrived since the last frame.
- Add a private slot `void onReplotTimeout()`.

### [MODIFY] serialportplot.cpp (UI Setup & Logic)
1. **Ui Setup ([setupUi](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#54-632))**: Initialize `m_replotTimer` and set it to a 30-33ms interval.
2. **Connections ([setupConnections](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#668-833))**: Connect `m_replotTimer`'s `timeout` signal to `onReplotTimeout()`.
3. **Data Throttling ([updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1049-1143))**:
   - Instead of calling `m_customPlot->replot()` and `rescaleValueAxis()` inside the `while` frame-processing loop, simply set `m_needsReplot = true;` after `channelDataBatch` is flushed to the `QCustomPlot`.
4. **Drawing Logic (`onReplotTimeout`)**:
   - If `!m_needsReplot` or `m_waveformPage->isHidden()`, do nothing.
   - If `m_btnAutoScale->isChecked()`, call `rescaleValueAxis()` for all active graphs.
   - Finally, call `m_customPlot->replot()` and reset `m_needsReplot = false;`.
5. **Adaptive Sampling**: Ensure `setAdaptiveSampling(true)` is explicitly enabled for all graphs created in [updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1049-1143) to optimize dense data rendering.

## Verification Plan
### Manual Verification
1. Run the application and open a high-speed serial simulation.
2. Set "视窗宽度" (View Width) to >100,000 points.
3. Verify that the UI remains perfectly responsive and smoothly updates without "freezing" the application.
