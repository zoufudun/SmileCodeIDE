# CANTool UI & Functional Enhancements Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add flexible sending modes, dynamic channel strings, dialog maximization, tab renaming, and horizontally-tiled multi-view panels to the CANTool window.

**Architecture:** Extend the `CanFrame` payload struct to hold a `transmitType` field. Replace the main `QTabWidget` with a `QSplitter` to allow horizontal tiled views, and extract the monitor views into separate `CanViewPanel` and `CanOpenViewPanel` widgets.

**Tech Stack:** C++ / Qt5 (QWidget, QSplitter, QDialog, QHBoxLayout, QVBoxLayout)

---

## User Review Required

> [!IMPORTANT]
> The central layout structure of CANTool will be refactored from `QTabWidget` to a horizontal `QSplitter`. This removes the static tab views, replacing them with dynamic side-by-side view panels that can be closed individually.

---

## Proposed Changes

### Task 1: Extend CanInterface & CanFrame
**Files:**
- Modify: [caninterface.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.h)
- Modify: [caninterface.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/caninterface.cpp)

- [ ] **Step 1: Add getters and fields to caninterface.h**
  Modify `CanFrame` to include `int transmitType = 0;` (0: Normal, 1: Single, 2: Self Receive, 3: Single Self Receive).
  Modify `CanInterface` class to declare public getters:
  ```cpp
  quint32 deviceType() const;
  int deviceIndex() const;
  ```
- [ ] **Step 2: Implement getters and update sending inside caninterface.cpp**
  Define getters in `caninterface.cpp`:
  ```cpp
  quint32 CanInterface::deviceType() const { return m_d->deviceType; }
  int CanInterface::deviceIndex() const { return m_d->deviceIndex; }
  ```
  Update `sendFrame(int channel, const CanFrame &frame)`:
  Inside the `frame.fd` branch:
  ```cpp
  tx.transmit_type = frame.transmitType;
  ```
  Inside the `else` branch:
  ```cpp
  tx.transmit_type = frame.transmitType;
  ```
- [ ] **Step 3: Verify build**
  Run compilation to verify that the changes in the library wrapper compile successfully.

---

### Task 2: Update Send Dialog UI Layout
**Files:**
- Modify: [normalsenddialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.h)
- Modify: [normalsenddialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.cpp)

- [ ] **Step 1: Declare sendTypeCombo and headers in normalsenddialog.h**
  Add `QComboBox *m_sendTypeCombo;` to `NormalSendPage` private variables.
- [ ] **Step 2: Set window flags for NormalSendDialog**
  In `NormalSendDialog` constructor, add:
  ```cpp
  setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
  ```
  Change tab naming logic prefix from `"Tab %1"` to `"通道名称 %1"`.
- [ ] **Step 3: Modify grid layout and add m_sendTypeCombo in normalsenddialog.cpp**
  In `NormalSendPage::setupUi()`:
  Set `m_tableWidget->setColumnCount(12);`
  Set labels: `{"选择", "状态", "ID(0x)", "协议", "长度", "名称", "数据", "帧类型", "单次发送帧数", "发送次数", "单次间隔(ms)", "发送方式"}`.
  Instantiate `m_sendTypeCombo` and populate it:
  ```cpp
  m_sendTypeCombo = new QComboBox();
  m_sendTypeCombo->addItems({"正常发送", "单次发送", "自发自收", "单次自发只收"});
  ```
  Arrange Grid Layout:
  Row 4: `发送方式:` + `m_sendTypeCombo` (col 0, 1), `名称(可选):` + `m_nameEdit` (col 2, 3), checkboxes layout (col 4, 5).
  Row 5: `btnLayout` spanning columns 0 to 5.
- [ ] **Step 4: Verify build**
  Verify the layout compilation and button styling.

---

### Task 3: Send Dialog logic for Transmit Modes
**Files:**
- Modify: [normalsenddialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.cpp)

- [ ] **Step 1: Save send mode in Table List**
  In `onAddToList()`:
  ```cpp
  m_tableWidget->setItem(row, 11, new QTableWidgetItem(m_sendTypeCombo->currentText()));
  ```
- [ ] **Step 2: Update Export and Import logic**
  In `onExportList()`:
  ```cpp
  obj["transmitType"] = m_tableWidget->item(i, 11) ? m_tableWidget->item(i, 11)->text() : "正常发送";
  ```
  In `onImportList()`:
  ```cpp
  m_tableWidget->setItem(row, 11, new QTableWidgetItem(obj.value("transmitType").toString()));
  ```
- [ ] **Step 3: Update RowSender parameter**
  Update `RowSender` constructor declaration and definition to accept `int transmitType`.
  Store it in `m_transmitType` and set `frame.transmitType = m_transmitType;` inside `sendOnce()`.
- [ ] **Step 4: Pass transmitType to RowSender in list send**
  In `executeNextSerial()` and `startParallelSend()`, parse column 11 string to get the `transmitType` int and pass it to the constructor of `RowSender`.
  In `onImmediateSend()` and `onImmediateSendTimerTick()`, assign the `transmitType` value from `m_sendTypeCombo` index directly into the `CanFrame`.

---

### Task 4: Dynamic Channel Naming Formatting
**Files:**
- Modify: [normalsenddialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.h)
- Modify: [normalsenddialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/normalsenddialog.cpp)

- [ ] **Step 1: Declare refreshChannels in normalsenddialog.h**
  Add `void refreshChannels();` in `NormalSendPage` slots.
- [ ] **Step 2: Connect connection signals and implement refreshChannels**
  In `NormalSendPage` constructor:
  ```cpp
  connect(m_can, &CanInterface::connected, this, &NormalSendPage::refreshChannels);
  connect(m_can, &CanInterface::disconnected, this, &NormalSendPage::refreshChannels);
  refreshChannels();
  ```
  In `refreshChannels()`:
  ```cpp
  void NormalSendPage::refreshChannels() {
    int prevIndex = m_channelCombo->currentIndex();
    m_channelCombo->clear();
    QString hw, fw, dr, lib, serial, typeStr;
    int canNum = 2;
    if (m_can && m_can->isDeviceOpen()) {
      m_can->getDeviceInformation(&hw, &fw, &dr, &lib, &canNum, &serial, &typeStr);
      if (typeStr.isEmpty()) typeStr = "USBCANFD-200U";
      int deviceIndex = m_can->deviceIndex();
      for (int i = 0; i < canNum; ++i) {
        m_channelCombo->addItem(QString("%1 设备%2 通道%3").arg(typeStr).arg(deviceIndex).arg(i));
      }
    } else {
      m_channelCombo->addItems({"USBCANFD-200U 设备0 通道0", "USBCANFD-200U 设备0 通道1"});
    }
    if (prevIndex >= 0 && prevIndex < m_channelCombo->count()) {
      m_channelCombo->setCurrentIndex(prevIndex);
    }
  }
  ```

---

### Task 5: Refactor CANTool Layout to Splitter & Move Status Monitor
**Files:**
- Modify: [cantool.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.h)
- Modify: [cantool.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.cpp)

- [ ] **Step 1: Update main layout container in cantool.h**
  Remove `QTabWidget *m_tabs;` and replace it with `QSplitter *m_splitter;`.
- [ ] **Step 2: Add window flag maximize hint and splitter setup in cantool.cpp**
  In `CANTool::setupUi()`:
  Add `setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);`.
  Instantiate `m_splitter = new QSplitter(Qt::Horizontal, this);` instead of `m_tabs`.
  Remove the tabs container code and add `m_splitter` to `mainLayout`.
- [ ] **Step 3: Move Status Monitor to toolbar button**
  In `CANTool::createToolbar()`:
  Add a dedicated `Status Monitor` button at the rightmost part of the toolbar.
  Connect it to slot `onStatusMonitorClicked()`.
  In `onStatusMonitorClicked()`:
  Open `DeviceMonitorPanel` inside a wrapper `QDialog` modal/modeless dialog.

---

### Task 6: Implement Modular View Panels (CanViewPanel & CanOpenViewPanel)
**Files:**
- Modify: [cantool.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.h)
- Modify: [cantool.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/STM32IDE/cantool.cpp)

- [ ] **Step 1: Define CanViewPanel and CanOpenViewPanel classes**
  Encapsulate monitor panels into `CanViewPanel` and `CanOpenViewPanel` inheriting `QWidget`.
  Each panel has a custom header with a close `✕` button. Clicking `✕` deletes the panel.
  Each CAN panel instance connects to `frameReceived` and filters frames based on its local channel selector.
- [ ] **Step 2: Wire menu additions in cantool.cpp**
  Update the "新建视图" menu:
  - "新建CAN视图" -> spawns `CanViewPanel` and appends to `m_splitter`.
  - "新建CANopen视图" -> spawns `CanOpenViewPanel` and appends to `m_splitter`.
- [ ] **Step 3: Initialize default view**
  In `CANTool::setupUi()`, trigger the creation of a default `CanViewPanel` so it starts with 1 view open.

---

## Verification Plan

### Automated Build Verification
- Propose compiling the code using `qmake` and `make` on the project environment.

### Manual Verification
1. Open the CANTool dialog. Maximize it, check window scaling.
2. Click "新建视图" -> "新建CAN视图" multiple times, check side-by-side tiling and close `✕` buttons.
3. Open "发送数据" -> "普通发送". Check if the channel formatting reads "设备名称 设备号 通道号".
4. Set sending mode to "自发自收" and test loopback.
5. Click "Status Monitor" in the top toolbar to confirm the monitor panel pops up as a dialog.
