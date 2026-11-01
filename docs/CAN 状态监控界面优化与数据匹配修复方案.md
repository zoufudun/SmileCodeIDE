# CAN 状态监控界面优化与数据匹配修复方案

根据您的需求，需要对 CAN 状态监控界面（`DeviceMonitorPanel` / `DeviceStatusWidget` / `CanProtocolConfigDialog`）进行三项主要修改与修复：
1. 右键属性修改与配置列表编辑生效机制修复、设备图标去掉上方 ID/CAN ID、去掉底部“报警/开”文字、缩小图标卡片占用面积；
2. CAN 状态监控界面增加接收数据来源配置（指定 CAN 设备 Index 与 CAN 通道 Channel）；
3. 诊断并修复测试数据/CAN调试助手发送报文后设备状态无法匹配和更新的问题（根因：`m_canIdToMappingIndices` 哈希表未在配置加载/更新时生成，导致收帧回调时检索到的映射始终为空）。

---

## User Review Required

> [!NOTE]
> 1. **设备图标样式与尺寸调整**：图标卡片尺寸由原来的 `128x155` 缩小为约 `96x106`，移除了卡片顶部的 ID 号及 CAN ID 文本，移除了卡片底部的状态字样便利贴（如“报警/开/关”等），保留了 LED 状态指示灯与设备名称。完整信息（设备ID、CAN ID、标签等）在鼠标悬停时通过 ToolTip 浮窗显示。
> 2. **数据源过滤范围**：在监控面板设置栏中提供“数据源：任意通道 / CAN 通道 0 / CAN 通道 1”，同时在设备映射结构中增加可针对特定 CAN 通道过滤的属性。

---

## Open Questions

无。

---

## Proposed Changes

### 1. 设备图标显示与尺寸优化 (`DeviceStatusWidget`)

#### [MODIFY] [devicestatuswidget.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h)
- 将默认 `sizeHint()` 改为 `QSize(96, 106)`，`minimumSizeHint()` 改为 `QSize(85, 95)`。
- 修改 `setMinimumSize(85, 95)` 与 `setMaximumSize(150, 160)`。

#### [MODIFY] [devicestatuswidget.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- `renderCache()`:
  - 移除顶部 `#deviceId` 与 `CAN 0x...` 绘制块。
  - 移除底部状态便利贴文本（"报警"、"开"、"关"等）绘制块。
  - 调整卡片内 Icon、LED 灯与设备 Label 的居中排版与比例，确保图标精致紧凑。
  - 保持 ToolTip 的完整详细信息展示（悬停时可见完整 ID 与 CAN 地址）。

---

### 2. 设备映射与配置对话框修复 (`CanProtocolConfigDialog`)

#### [MODIFY] [canprotocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.h)
- 在 `DeviceBitMapping` 结构体中添加 `canChannel`（默认 `-1` 表示任意通道，`0` 表示通道 0，`1` 表示通道 1）与 `canDevice`（设备号）属性。

#### [MODIFY] [canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- 在表格项与编辑区域添加“CAN 通道”下拉配置项。
- 修复编辑生效机制：在点击“确定”时，若当前有选中的行且下方编辑框内容有变更，自动同步下方编辑框最新值至表格，确保修改 100% 成功应用并保存。

---

### 3. CAN 状态监控面板功能增强与 Bug 修复 (`DeviceMonitorPanel` & `CanProtocolMonitor`)

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h)
- 增加 `m_selectedSourceDevice` 与 `m_selectedSourceChannel` 成员，表示当前监控过滤的数据源通道。
- 声明 `onEditDeviceRequested(int deviceId)` 槽函数。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **解决问题 3 (状态不匹配根因)**：在 `loadConfig()`、`onConfigClicked()`、`onImportClicked()`、`rebuildRoomCanvas()` 等所有更新 `m_mappings` 的关键位置，添加 `buildMappingHash()` 调用！确保 CAN ID 索引哈希表有效生成，使接收到 CAN 报文后能够正确查找到设备映射并调用 `widget->setStatus(bitVal)`。
- **解决问题 1 (右键编辑与图标摆放)**：在 `rebuildRoomCanvas()` 中，将创建的 `DeviceStatusWidget` 的 `editRequested(int deviceId)` 信号连接到 `onEditDeviceRequested(int deviceId)`；在右键点击图标时弹出 `CanProtocolConfigDialog` 并高亮选中对应的设备行。更新组件尺寸几何设置从 `128x155` 到 `96x106`。
- **解决问题 2 (数据源配置)**：在工具栏/设置菜单增加 CAN 通道选择器（全部通道/通道 0/通道 1），并在 `processBatch()` 报文处理逻辑中加入对 `frame.channel` 数据源通道的过滤比对。
- **测试发送联动**：在绑定 `CanInterface` 信号时，除了 `frameReceived` 外，同时绑定 `frameSent` 信号到 `onFrameReceived`（或判断自发自收测试帧），确保通过 CAN 调试助手或普通发送面板发送测试帧时，监控面板状态能实时联动匹配。

#### [MODIFY] [canprotocolmonitor.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolmonitor.cpp)
- 在 `rebuildGrid()` 与 `loadConfig()` 中同步调用 `buildMappingHash()` 与 `editRequested` 信号绑定，确保独立的 `CanProtocolMonitor` 窗口也具备相同修复。

---

## Verification Plan

### Automated Tests
- 使用 Qt Creator / MinGW 编译项目，确保无编译错误或未定义符号引用。

### Manual Verification
1. **右键属性编辑验证**：
   - 打开 CAN 状态监控界面，右键点击任一设备图标，验证是否弹出配置对话框并高亮选中的设备。
   - 在设备配置对话框修改设备名称、CAN ID、字节/位索引或通道，点击“确定”，验证主界面上的设备图标属性是否即时更新并保存到 JSON 配置文件中。
2. **设备图标 UI 验证**：
   - 检查设备图标顶部是否已去除 ID 号和 CAN ID，底部是否已去除“报警”、“开”文字便利贴。
   - 验证设备图标卡片尺寸已适度缩小（约 96x106），排列更紧凑美观。
   - 将鼠标悬停在设备图标上，验证 ToolTip 是否显示完整设备名、ID 及 CAN ID。
3. **数据源配置与接收匹配验证**：
   - 在数据源设置中配置接收数据的 CAN 通道（如通道 0 / 通道 1 / 任意通道）。
   - 使用 CAN 调试助手（`NormalSendDialog`）发送 CAN ID 匹配的测试帧（控制对应 byte/bit 为 1 或 0）。
   - 观察设备状态指示灯（红/绿）与闪烁效果是否精确响应测试数据，日志区域是否实时打印“状态变为：报警/开启”等变更记录。
