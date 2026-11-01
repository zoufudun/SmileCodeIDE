# Waveform Floating Play/Pause Button Implementation Plan

## Goal Description
The user wants to add a floating play/pause button directly on top of the waveform interface (`m_customPlot`).
- By default, it will show a play icon (`e719`) and drawing will be paused.
- When clicked, it will start drawing the waveform, and the play button will disappear.
- During drawing, if the user hovers over the waveform interface, a pause icon (`e6d3`) will appear in the center. Clicking it will stop the drawing and revert to the default state.

## Proposed Changes

### [MODIFY] serialportplot.h
- Declared `QToolButton *m_btnFloatingPlay` in the private class members.
- Declared `bool eventFilter(QObject *watched, QEvent *event) override;` in the protected section.

### [MODIFY] serialportplot.cpp (UI Setup & Styling)
1. **Button Initialization ([setupChart](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#633-667))**:
   - Create `m_btnFloatingPlay` with `m_customPlot` as its parent so it overlays the graph.
   - Use `CIconFont` with size 48 to set its icons.
   - Apply a transparent background stylesheet with glowing hover effects.
   - Call `m_customPlot->installEventFilter(this);` to intercept mouse enter, leave, and resize events.
2. **Event Handling ([eventFilter](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#2202-2225))**:
   - Intercept `QEvent::Resize` on `m_customPlot` to keep the floating button dynamically centered.
   - Intercept `QEvent::Enter` to show the pause button (`e6d3`) when drawing is active.
   - Intercept `QEvent::Leave` to hide the pause button when the mouse leaves.
3. **Control Integration ([setupConnections](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#887-1072))**:
   - Connect the floating button's `clicked` signal to toggle the existing `m_btnStopWaveform` logic.
   - Connect `m_btnStopWaveform->toggled` signal to synchronize the floating button's visibility and text depending on whether it is playing or paused.
   - Set the initial state of `m_btnStopWaveform` to `true` (paused state).

## Verification Plan
1. Open the Serial port debug interface.
2. Verify the waveform interface shows a large transparent play icon by default.
3. Click the play button; verify waveform plotting starts and the button vanishes.
4. Hover over the interface during plotting; verify the pause button appears. Click it to stop plotting.
