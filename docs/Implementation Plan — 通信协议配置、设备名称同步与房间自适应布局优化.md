# Implementation Plan — 通信协议配置、设备名称同步与房间自适应布局优化

本计划旨在解决用户提出的三项关键需求：
1. 设备属性修改后设备图标同步刷新；
2. CAN 状态监控界面提升为通用「通信协议配置」（支持 CAN 通道及未来 Modbus/RS232/RS485 等扩展）；
3. 房间尺寸根据内置设备数量与图标尺寸自动拓展，并实现房间间重叠自动规避算法。

---

## User Review Required

> [!IMPORTANT]
> **通信协议配置扩展架构**：
> - 我们将原有的单一“CAN通道”选择下拉框升级为模块化的**通信协议与数据源配置系统（Communication Protocol Config）**。
> - 在 UI 上提供「🔌 通信协议配置」入口，包含当前协议类型（CANFD/USBCAN、Modbus RTU/TCP、RS232/RS485 扩展槽）、物理设备类型、工作通道选择。
> - 在数据过滤引擎中，以统一的协议数据源结构体 `ProtocolConfig` 进行抽象，保证未来扩展 Modbus / RS485 时无缝衔接。

> [!NOTE]
> **房间尺寸自适应与防重叠机制**：
> - **动态扩容**：房间宽度与高度将根据所包含的设备数量（以 96×106 图标尺寸及网格间距为基准）自动计算最小容纳矩形，确保所有设备完全包裹在房间标题框下方。
> - **防重叠算法**：在自动布局和放置房间时，引入包围盒相交检测与自动推开排布（AABB 碰撞检测），若两房间发生重叠，自动将后续房间推移至不重叠的相邻网格区。

---

## Proposed Changes

### 1. 设备属性修改同步刷新

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 在 `rebuildRoomCanvas()` 函数中，不仅为新设备创建 `DeviceStatusWidget`，还对已存在的 `DeviceStatusWidget` 实例调用 `w->setLabel(m.label)`、`w->setCanId(m.canId)`、`w->setDeviceKind(kind)`，确保在配置列表或编辑对话框修改设备名称、类型、CAN ID 后，界面上的设备图标能立即刷新显示。

---

### 2. 通信协议配置系统抽象与扩展

#### [NEW] [protocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/protocolconfigdialog.h) / [protocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/protocolconfigdialog.cpp)
- 新建协议与数据源配置对话框：
  - 支持协议类型选择：`CAN 总线 (USBCAN/CANFD)`、`Modbus RTU / TCP (预留扩展)`、`RS232 / RS485 串口 (预留扩展)`。
  - 对于 CAN 协议，可根据当前连接设备的实际通道数（如双通道 CAN 设备的通道 0 / 通道 1 / 任意通道）进行选择与参数配置。
  - 提供参数持久化及结构体接口 `ProtocolConfig`。

#### [MODIFY] [devicemonitorpanel.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.h) / [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 将工具栏的数据源按钮升级为「🔌 通信协议配置」；
- 点击后弹出 `ProtocolConfigDialog`，并将选定的协议及通道参数应用到后台数据流；
- 在 `processBatch()` 中根据 `ProtocolConfig` 进行多协议/多通道的数据匹配与分流。

---

### 3. 房间自适应尺寸与自动防重叠布局

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- **房间自适应算法 (`autoArrangeRoomDevices`)**：
  - 根据房间内设备数量 $N$，动态计算列数 $cols = \min(4, \max(2, \lfloor\sqrt{N}\rfloor))$ 与行数 $rows = \lceil N / cols \rceil$；
  - 根据设备图标尺寸（$96 \times 106$）、图标间距（$14\text{px}$）以及房间标题栏边距（顶部 $45\text{px}$），计算房间所需最小宽度 $minW$ 与最小高度 $minH$；
  - 自动调整房间包围盒 `room.geom.setSize(minW, minH)`。
- **房间防重叠与自动对齐**：
  - 实现 `resolveRoomOverlaps()` 函数，在创建或调整房间位置时进行几何重叠检测；
  - 若检测到 `room[i].geom.intersects(room[j].geom)`，自动沿水平或垂直方向追加间距 $40\text{px}$ 进行网格顺移，确保房间之间完全不相交重叠。

---

## Verification Plan

### Automated Build Verification
- 运行 Powershell 编译命令验证语法与链接正确性：
  ```powershell
  mingw32-make -f Makefile.Release
  ```

### Manual Verification
1. **设备名称修改同步验证**：
   - 打开设备配置列表，双击或选择修改某个设备的标签名称为“前舱烟温”，点击确定保存；
   - 验证主监控界面及子窗口对应设备的图标标签是否立刻更新为“前舱烟温”。
2. **通信协议配置验证**：
   - 点击工具栏「🔌 通信协议配置」按钮；
   - 选择通信协议（如 CAN 协议）及通道（如通道0 / 通道1 / 全部通道），确定保存；
   - 发送 CAN 帧数据，验证状态监控面板是否正确响应所选协议/通道的报文。
3. **房间自适应与防重叠验证**：
   - 向同一房间批量拖入/分配多个设备（例如分配 6 个设备到“驾驶舱”）；
   - 观察“驾驶舱”房间边界是否自动向外拓展包围所有图标，避免设备图标越界；
   - 创建多个房间，观察自动排布时房间与房间之间是否有清晰间距、绝无重叠。
