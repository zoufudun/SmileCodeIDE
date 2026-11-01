# CAN 工作台设备管理与启动配置全面优化方案

本方案针对 CAN 工作台设备管理界面和启动参数对话框中的显示、主题适配、设备分类、硬件检测及协议动态联动问题进行全面优化。

## User Review Required

> [!IMPORTANT]
> - **型号分类与联动**：型号下拉列表将按“软件仿真”、“周立功 ZLG”、“创芯科技”、“Qt SerialBus (第三方卡)”、“同星智能”等厂商进行分类分组显示（带分类标题及缩进），且与“厂商”筛选下拉框双向联动。
> - **设备检测与提示**：对话框顶部将新增检测提示栏，自动探测已连接接入的 USB CAN 硬件（ZLG/创芯/Qt 支持的 PCAN 等），并提示用户、自动定位选中已接入的设备。
> - **启动对话框协议联动**：选择协议为“CAN”时，完全隐藏“CANFD标准”、“CANFD加速”、“数据域波特率”整行控件与标签，波特率标签自动切换为“波特率”；选择“CAN FD”时恢复显示。
> - **不同主题文字与白框全面修正**：修复 Qt 样式表中 `selection-background-color` 渐变语法导致的解析错误（致使下拉框回退到系统纯白底色），并为所有下拉框启用 `QListView` 视图代理，彻底消除各主题下的白框与文字不可见问题。

---

## Proposed Changes

### 1. 主题样式体系与 QComboBox 修复 (`idetheme.h`)

#### [MODIFY] [idetheme.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/idetheme.h)
- 修复 `QComboBox QAbstractItemView` 中 `selection-background-color: {{ACCENT_GRAD}};` 语法错误（Qt CSS 规范中 selection-background-color 仅支持纯色，不支持 gradient；遇到 gradient 会导致整条样式规则被解析器抛弃，进而回退为 Windows 原生白色弹窗底色）。将其修正为纯色 `{{SELECTION_BG}}`。
- 全面完善 QComboBox 下拉菜单及项样式：
  ```css
  QComboBox QAbstractItemView, QComboBox QListView {
    background-color: {{CARD_BG}};
    color: {{TEXT_MAIN}};
    border: 1px solid {{BORDER_LIGHT}};
    selection-background-color: {{SELECTION_BG}};
    selection-color: {{SELECTION_TEXT}};
    outline: 0;
    padding: 4px;
    border-radius: 6px;
  }
  QComboBox QAbstractItemView::item, QComboBox QListView::item {
    color: {{TEXT_MAIN}};
    background-color: transparent;
    min-height: 26px;
    padding: 4px 8px;
    border-radius: 4px;
  }
  QComboBox QAbstractItemView::item:hover, QComboBox QAbstractItemView::item:selected,
  QComboBox QListView::item:hover, QComboBox QListView::item:selected {
    background-color: {{ACCENT}};
    color: {{ACCENT_TEXT}};
  }
  ```
- 确保 `QDialog`、`QScrollArea` 在所有 8 种主题（钛金、赛博霓虹、熔岩黑金、星云紫晶、碧海深渊、极客翡翠、纯白曜石、暖阳羊皮）下具备统一的背景与文字对比度。

---

### 2. CAN 接口层检测接入设备扩展 (`caninterface.h`, `caninterface.cpp`)

#### [MODIFY] [caninterface.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/caninterface.h)
- 声明检测到的接入设备结构体：
  ```cpp
  struct CanDetectedDevice {
    quint32 deviceType = 0;
    int deviceIndex = 0;
    QString vendor;
    QString modelName;
    QString detail;
  };
  ```
- 增加方法：`QList<CanDetectedDevice> detectConnectedDevices();`

#### [MODIFY] [caninterface.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/caninterface.cpp)
- 实现安全快速的设备检测（非阻塞）：
  - 扫描 Qt SerialBus 插件可用设备（`peakcan`, `vectorcan`, `systeccan` 等）；
  - 探测 ZLG USBCANFD / USBCAN 驱动设备是否在线（轻量探测索引0）；
  - 探测 创芯科技 ControlCAN 设备是否在线（轻量探测索引0）；
  - 始终标明纯软件仿真设备可用；
  - 若当前已有设备处于打开状态，直接返回该已连接设备。

---

### 3. 设备管理界面与启动配置界面重构 (`candevicedialog.h`, `candevicedialog.cpp`)

#### [MODIFY] [candevicedialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/candevicedialog.h)
- 添加提示横幅控件 `QLabel *m_detectBannerLabel;`
- 添加分类配置型号下拉列表的方法 `void populateDeviceTypeCombo(int vendorFilterIdx = 0);`
- 保存当前应用样式 `QString m_currentStyle;`

#### [MODIFY] [candevicedialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/candevicedialog.cpp)
1. **型号下拉列表按照厂商分类显示**：
   - 按厂商分块：【软件仿真】、【周立功 ZLG】、【创芯科技】、【Qt SerialBus (第三方卡)】、【同星智能】；
   - 分类标题作为不可选中的分隔头；子项进行文字前缀缩进；
   - 与 `m_vendorFilterCombo`（厂商筛选下拉框）动态联动，当选择具体厂商时只列出该厂商型号，选择“全部厂商”时显示完整分类列表；
   - 设置 `m_deviceTypeCombo->setView(new QListView(m_deviceTypeCombo));`。
2. **检测当前已接入设备并提示**：
   - 在界面顶部新增状态横幅 `m_detectBannerLabel`；
   - 自动运行设备检测，若检测到硬件接入（例如创芯 USBCAN 或 ZLG 或 PCAN），显示醒目提示：
     `🟢 已检测到接入设备：【创芯 USBCAN-2C (设备0)】，已自动定位，点击[打开设备]即可连接`，并自动选择该型号；
   - 若未检测到物理设备接入，提示：
     `💡 未检测到接入的物理硬件，已为您就绪【🌟 纯软件仿真】模式，可随时免硬件测试`；
   - 在型号下拉框和设备目录树中，为检测到的设备添加 `✔ [已接入]` 标签与绿色高亮。
3. **设备连接后自动读取设备信息**：
   - 在 `onOpenDeviceClicked()` 成功连接后，自动调用 `m_can->getDeviceInformation(...)`；
   - 在界面摘要栏和状态提示中展示已读取的设备信息（硬件版本、固件版本、驱动版本、序列号、通道数量）；
   - 在设备树节点 column 0 中展示 `[在线 · %1 路通道]`，并设置完整信息 ToolTip；
   - 重构 `onShowDeviceInfoClicked()`：彻底移除写死的白色背景（`background-color: white`）和浅蓝按钮，换为与当前主题完全融合的卡片样式。
4. **修复“停止按钮出底部有文字”问题（图1）**：
   - 移除 `devItem->setText(1, QString("在线 · %1 路通道").arg(canNum));`；
   - 将在线状态移至 column 0：`devItem->setText(0, QString("%1 设备%2  [在线 · %3 路通道]").arg(...));`；
   - column 1 清空文本（`setText(1, "")`），仅承载操作按钮，彻底杜绝文字穿透按钮缝隙。
5. **修复协议下拉框白框及主题文字融合问题（图2）**：
   - 在 `StartChannelDialog` 中对所有 `QComboBox` 设置 `combo->setView(new QListView(combo));`；
   - 移除 `scrollWidget` 上的 `QWidget { background: transparent; }` 污染性内联样式；
   - 将父窗口完整样式表传递给 `StartChannelDialog` 及 `FilterSettingsDialog`。
6. **启动参数对话框：协议为 CAN 时隐藏 CANFD 选项**：
   - 当 `m_protocolCombo` 选择为 “CAN” 时：
     - 完全隐藏（Hide / `setVisible(false)`）以下行：
       - `m_standardCombo`（CANFD标准）及其对应 Label
       - `m_accelCombo`（CANFD加速）及其对应 Label
       - `m_dbitBaudCombo`（数据域波特率）及其对应 Label
     - 将 `m_abitBaudCombo` 对应 Label 切换显示为 “波特率”
   - 当选择为 “CAN FD” 时：
     - 显示上述三行设置，并将 Label 切换为 “仲裁域波特率”
   - 对话框自适应内容尺寸。

---

## Verification Plan

### 编译验证
- 执行 `mingw32-make -f Makefile.Release` 验证代码无语法错误并成功编译。

### 功能验证
1. **型号下拉列表分类**：
   - 检查下拉列表是否清晰按厂商分组呈现，切换厂商过滤时下拉列表能否正确同步。
2. **设备检测与提示**：
   - 检查界面是否正确显示当前设备接入提示，是否正确标明仿真设备与硬件接入状态。
3. **设备连接与信息读取**：
   - 连接设备后，验证是否自动读取版本与序列号，并正确更新至界面与设备信息弹窗；
   - 验证设备信息弹窗在暗色/翡翠/明亮主题下均正常显示，无刺眼白底。
4. **停止按钮文字修复**：
   - 连接设备后观察设备行，启动与停止按钮之间及停止按钮底部无多余文字重叠。
5. **协议下拉框与暗色主题**：
   - 打开“启动”对话框，展开“协议”下拉框，确认下拉列表具备深色背景、清晰白色文字及高亮选中效果，不再是纯白框。
6. **CAN 协议隐藏 CANFD 选项**：
   - 切换协议为 “CAN”，检查“CANFD标准”、“CANFD加速”、“数据域波特率”是否完全消失，“仲裁域波特率”是否变为“波特率”；切换回 “CAN FD” 时是否正常恢复。
