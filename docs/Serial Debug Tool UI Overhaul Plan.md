# Serial Debug Tool UI Overhaul Plan

## Goal Description
Redesign the Serial Debug Tool to use a "Slice/Card" style layout for settings panels (allowing dragging and hiding), increase the send data box size, and implement floating toolbars with animated icon buttons inside the Receive and Send text areas.

## Proposed Changes

### 1. Dockable Slice/Card Layout
- **Component**: `SerialSession::setupUi`
- **Change**: Replace the left-side `QWidget` layout structure with a nested `QMainWindow` inside [SerialSession](file:///m:/TOPFIRE/SmileCodeIDE/STM32IDE/serialportplot.h#48-244). 
- **Details**:
  - The central widget of this internal `QMainWindow` will be the right-side data/waveform `QSplitter`.
  - Convert `grpPort` (Port Settings), `grpRx` (Receive Settings), `grpTx` (Send Settings), and `grpScope` (Scope Settings) into individual `QDockWidget` objects (Card style).
  - Dock them in the `Qt::LeftDockWidgetArea`. This native Qt component perfectly supports the "draggable, hideable, card-like" requirement.

### 2. Floating Controls in Data Receive Area
- **Component**: Receive Area (`m_textReceive`)
- **Change**: Overlay a floating transparent toolbar at the bottom-left of the text area.
- **Controls to add**:
  - `btnRxHexToggle` (Icon `ebbc`): Toggles ASCII/HEX mode.
  - `btnRxTimeToggle` (Icon `e676`): Toggles Timestamp visibility.
  - `btnRxPauseToggle` (Icon `e617`/`e7d8`): Plays/Pauses reception.
  - `btnRxClear` (Icon `e621`): Clears the text.
- **Styling**: Remove borders, use `CIconFont` for icons, add hover scale/color-fade animations, and checked effects.
- **Refactor**: Remove or hide the redundant controls from the `grpRx` dock widget to avoid duplication.

### 3. Floating Controls in Data Send Area (Single Send)
- **Component**: Send Area (`m_textSend`)
- **Change**: Overlay a floating toolbar at the bottom-left of the text area.
- **Controls to add**:
  - `btnTxHexToggle` (Icon `ebbc`): Toggles ASCII/HEX mode.
  - `btnTxClear` (Icon `e621`): Clears the text.
  - `chkAutoSend` (Checkbox): Enables auto-send.
  - `spinAutoSendInterval` (Spinbox): Hidden by default, appears only when `chkAutoSend` is checked.
  - `chkTxNewLine` (Checkbox): Send new line toggle.
- **Refactor**: Remove or hide the redundant controls from the `grpTx` dock widget.
- **Sizing**: Increase the stretch factor/minimum height of the bottom half (send area) in the `QSplitter` to meet the "increase send data box size" requirement.

## Verification Plan
1. Open Serial tool. Verify settings panels on the left look like independent cards that can be dragged, rearranged, and closed.
2. Verify floating icons in the receive text area exist, look clickable (visual hover effect), and correctly toggle hex/ascii, timestamps, play/pause, and clear the screen.
3. Verify floating icons and checkboxes in the send area exist and the interval spinbox dynamically shows/hides when auto-send is toggled.
4. Verify the send data box layout is noticeably taller.
