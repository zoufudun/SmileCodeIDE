# Further Optimization for High Data Volumes

## Goal Description
The user reported that when receiving massive amounts of data, the UI still stutters, and attempting to interact with the UI (like dragging the scrollbar or clicking other elements) causes the application to crash.

## Analysis of the Problem
1. **Scrollbar Emitting Replot**: While we throttled [updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1285-1454), the user interacting with `m_scrollbarWaveform` triggers [onWaveformScroll](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1472-1484) which *synchronously* calls `m_customPlot->replot()` for every single pixel movement. During high-speed data receiving, this creates a massive blockage on the main thread causing crashes.
2. **Buffer Limits**: Large buffer limits (>100,000 points) will still inherently cause CPU spikes when QCustomPlot calculates pixel points, especially since we are looping over multiple channels to call `removeBefore`.

## Proposed Changes

### [MODIFY] serialportplot.cpp (Scrollbar Throttling)
- In [onWaveformScroll](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1472-1484), do **NOT** call `m_customPlot->replot()`.
- Instead, simply set `m_needsReplot = true;` so that the 30 FPS timer [onReplotTimeout](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1455-1471) handles the drawing seamlessly.

### [MODIFY] serialportplot.cpp (QCustomPlot Adaptive Sampling)
- High channel volumes can cause lag. In [setupChart()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#708-791), explicitly ensure `setUseOpenGL(true)` is attempted (though it depends on QCustomPlot support enabled during compilation). More importantly, ensure QCustomPlot isn't calculating layout elements excessively.

### [MODIFY] serialportplot.cpp (Data Removal Batching)
- Calling `removeBefore` for every batch received creates heavy array manipulation. We should modify [updateWaveform](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#1285-1454) so we only call `removeBefore` when the data buffer size exceeds the limit by a certain chunk size (e.g., limit + limit/10), to reduce array shifting frequency.

## Verification Plan
1. Simulate high-speed serial data (e.g. 5 channels, 100K buffer limit).
2. Grab and drag the bottom scrollbar aggressively while data is coming in.
3. Verify the application does not crash and the UI remains perfectly responsive.
