# Waveform Display Implementation Plan

## Goal Description
Enhance the serial communication tool's waveform display to parse specific formatted strings (`$v1 v2 v3;`), support multiple channels dynamically, show a channel legend, and add checkboxes to hide the raw data / rx-tx boxes for performance and cleaner UI.

## Proposed Changes

### 1. [STM32IDE/serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h)
#### [MODIFY] [serialportplot.h](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h)
- Add UI member pointers in [SerialSession](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h#46-208):
  - `QCheckBox *m_chkHideRxTx;`
  - `QCheckBox *m_chkHideRawData;`
  - `QSplitter *m_dataSplitter;` (to control visibility of the whole Rx/Tx area)

### 2. [STM32IDE/serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp)
#### [MODIFY] [serialportplot.cpp](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp)
- **[setupUi()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#51-604)**:
  - Instantiate `m_chkHideRxTx` ("隐藏收发区") and `m_chkHideRawData` ("不显原始数据").
  - Add these checkboxes to the `grpScopeLayout` (示波器设置 GroupBox).
  - Assign `m_dataSplitter = rightSplitter` so we can toggle its visibility later.
  - Connect `m_chkHideRxTx` toggled signal to `m_dataSplitter->setVisible(!checked)`.
- **[setupChart()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#605-632)**:
  - Enable legend display: `m_customPlot->legend->setVisible(true);`
- **[onReadyRead()](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#872-911)**:
  - Check `m_chkHideRawData->isChecked()`. If it is checked, skip appending `htmlLine` to `m_textReceive` to improve performance.
- **[updateWaveform(const QByteArray &data)](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.cpp#948-1017)**:
  - Replace the current dumb number extractor with a structured parser.
  - Append to a static `QString buffer`.
  - Loop while we can find `$` and `;`. Extract the payload.
  - Split payload by spaces to get channel values.
  - If `m_customPlot` doesn't have enough graphs for the channels, dynamically add them and assign different distinct colors using HSV.
  - Add the data to corresponding graphs at current `m_xValue`, and increment `m_xValue` once per frame.
  - Call `rescaleAxes` and `replot`.

## Verification Plan
1. Compile the project.
2. Open the serial tool, enable waveform display.
3. Send strings like `$10 20 30;` and verify multiple curves show up with different colors.
4. Verify legend reflects CH1, CH2, CH3.
5. Check "隐藏收发区" and verify the Rx/Tx area disappears, auto-resizing the graph.
6. Check "不显原始数据" and observe that the Rx box stops scrolling but the graph continues updating.
