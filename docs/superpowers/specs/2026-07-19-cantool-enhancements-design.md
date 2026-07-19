# CANTool 功能增强与布局优化设计规约

本文档详述了 CANTool 调试助手的界面与底层交互优化设计，涵盖发送方式扩展、多视图分屏水平分割、白条去除与 Status Monitor 移动等需求。

---

## 1. 核心设计与数据流向

### 1.1 发送方式 (Transmit Mode) 扩展逻辑
ZLG CAN 驱动库本身支持在 `ZCAN_Transmit_Data` 和 `ZCAN_TransmitFD_Data` 结构体中设定 `transmit_type`。定义映射如下：
- `0`: 正常发送 (Normal)
- `1`: 单次发送 (Single)
- `2`: 自发自收 (Self Receive)
- `3`: 单次自发自收 (Single Self Receive) —— 对应用户界面词汇 “单次自发只收”

我们将此项参数内置到 `CanFrame` 数据帧结构体中，以便于在单帧立即发送和列表循环发送时都能保持高度内聚。

```
[UI 界面发送方式下拉框] ---> [立即发送] ---> [CanFrame.transmitType]
      |                                              |
      |                                              v
      +--> [添加到列表] ---> [QTableWidget 发送方式列] ---> [RowSender] ---> [CanFrame]
                                                                          |
                                                                          v
                                                          [m_can->sendFrame 内的 ZCAN_Transmit]
```

### 1.2 水平分割多视图 (Multi-View Splitter) 布局
将原有的单实例 `QTabWidget` 替换为以水平分割器 (`QSplitter`) 为主体的多视图展示架构。

```
[CANTool 主窗口] ---> [水平布局] ---> [QSplitter 容器]
                                         |
                                         +---> [CanViewPanel 1]
                                         +---> [CanViewPanel 2]
                                         +---> [CanOpenViewPanel 1]
```

每个视图节点（`CanViewPanel` 或 `CanOpenViewPanel`）都是一个自治的子控件，内置独特的头部控制条（包括视图名和关闭按钮 ✕）。

---

## 2. 详细接口设计与文件修改说明

### 2.1 底层适配 (`caninterface.h` / `caninterface.cpp`)
- **[MODIFY]** `CanFrame` 结构体：
  增加整型成员变量 `int transmitType = 0;`。
- **[MODIFY]** `CanInterface::sendFrame`：
  在 `tx.transmit_type` 赋值的地方，将硬编码的 `0` 替换为 `frame.transmitType`：
  ```cpp
  tx.transmit_type = frame.transmitType;
  ```
- **[MODIFY]** `CanInterface` 增加两个获取当前设备索引和类型的 getter：
  ```cpp
  quint32 deviceType() const;
  int deviceIndex() const;
  ```

### 2.2 普通发送弹窗 (`normalsenddialog.h` / `normalsenddialog.cpp`)
- **[MODIFY]** `NormalSendPage::setupUi`：
  1. 重新调整格点布局，在第四行（Row 4）加入 “发送方式” `QComboBox`，包含选项：`正常发送`, `单次发送`, `自发自收`, `单次自发只收`。
  2. 将 `m_tableWidget` 的列数由 `11` 修改为 `12`，在表头末尾新增 “发送方式” 标题。
  3. 将底部按钮行移动到第五行（Row 5），腾出更宽大的自适应拉伸空间。
- **[MODIFY]** `NormalSendPage` 业务函数：
  1. `onAddToList()`：新增将 “发送方式” 下拉框当前文本写入第 11 列（索引 11）的逻辑。
  2. `onImportList()` 与 `onExportList()`：在读写 JSON 时，新增 `transmitType` 键值的保存与解析。
  3. `executeNextSerial()` 与 `startParallelSend()`：读取第 11 列的文本并解析为对应的整型 `transmitType`，传递给 `RowSender` 构造函数。
  4. `refreshChannels()`：监听 `CanInterface::connected` 与 `disconnected` 信号，动态拉取已打开设备的信息，组合成：
     `设备名称 设备号 通道号` (例如 `USBCANFD-200U 设备0 通道0`) 作为通道选择器的选项。
- **[MODIFY]** `RowSender` 构造函数与发送逻辑：
  在构造函数中增加 `int transmitType` 实参，并在 `sendOnce()` 内设置 `frame.transmitType = m_transmitType;`。
- **[MODIFY]** `NormalSendDialog`：
  1. 设置窗口标志 `Qt::WindowMinMaxButtonsHint` 以支持最大化。
  2. 新建 Tab 默认标题修改为 `通道名称 %1`，标签文字统一化。

### 2.3 监视主界面 (`cantool.h` / `cantool.cpp`)
- **[MODIFY]** `CANTool::setupUi`：
  1. 移除 `m_tabs` (QTabWidget)。
  2. 实例化 `QSplitter *m_splitter`，并添加为中心布局控件。
  3. 窗口标志设置：`setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);` 增加窗口最大化按钮。
  4. 启动时，调用 `onNewViewTriggered` 默认添加一个 `CanViewPanel` 作为主监视区。
- **[MODIFY]** `CANTool::createToolbar`：
  1. 将菜单项 “报文收发视图” 修改为 “新建CAN视图”；“CANopen 测试视图” 修改为 “新建CANopen视图”。
  2. 在工具栏最右侧（`layout->addStretch()` 之后）添加一个独立的 `Status Monitor` 工具按钮。
- **[NEW]** `CanViewPanel` 与 `CanOpenViewPanel` 类：
  - 继承自 `QWidget`，承载独立的收发监视表格和发送命令区。
  - 拥有自定义标题头布局（如 `background-color: #2f343f`，标题为 `视图N:CAN 视图`，右侧带 `✕` 按钮）。
  - `✕` 按钮被点击时，触发自身析构并从 `QSplitter` 中移除。
  - 收到数据帧时，对比自身选中的通道，决定是否上报显示。
- **[NEW]** `DeviceMonitorDialog` 类：
  - 继承自 `QDialog`，将 `DeviceMonitorPanel` 封装在其中，作为点击工具栏 `Status Monitor` 按钮时弹出的悬浮状态查看器。

---

## 3. 测试与验证细节
1. **最大化测试**：启动 CANTool 弹窗，点击右上角最大化，观察主分割器（Splitter）中的多视图是否水平按比例拉伸，字体、间距是否自适应放大。
2. **多视图并排测试**：多次点击 “新建视图” -> “新建CAN视图”，确认右侧能水平并列铺开多个视图，并能通过拖动分割线调整各自占比；点击右上角 `✕` 可随时关闭视图。
3. **发送方式验证**：在普通发送中选择 “自发自收”，发送后检查左侧监视视图中该通道是否能同时出现 “发送” 与 “接收” 两条相同 ID 的报文。
4. **通道名称显示**：打开设备后，打开普通发送窗口，通道下拉框应显示完整的 `USBCANFD-200U 设备0 通道0`。
