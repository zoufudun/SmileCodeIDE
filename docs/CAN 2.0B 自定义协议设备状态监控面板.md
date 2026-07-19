# CAN 2.0B 自定义协议设备状态监控面板

## Context

当前项目已有 CAN/CAN FD 报文收发工具 (`CANTool`) 和 CANopen 协议测试功能。现需要新增一个**设备状态监控面板**，基于 CAN 2.0B 自定义协议，实时显示烟温探测器、阀门等设备的开关状态（0/1），使用精美的图标和颜色变化进行可视化呈现。

## 架构设计

### 新增文件（5个）

| 文件 | 职责 |
|------|------|
| `devicestatuswidget.h/cpp` | 单个设备状态指示器组件（图标 + 标签 + 闪烁动画） |
| `canprotocolmonitor.h/cpp` | 主监控面板，网格布局管理多个 DeviceStatusWidget，接收 CAN 帧并分发更新 |
| `canprotocolconfigdialog.h/cpp` | 协议配置对话框，管理 CAN ID → 设备映射表 |

### 修改文件（2个）

| 文件 | 修改内容 |
|------|---------|
| `STM32IDE.pro` | 添加新源文件和头文件 |
| `mainwindow.h/cpp` | 添加打开监控面板的菜单项/工具栏按钮 |

---

## 详细设计

### 1. 数据结构 — 设备位映射 (`canprotocolmonitor.h`)

```cpp
struct DeviceBitMapping {
    int     deviceId;       // 唯一标识
    QString label;          // 显示名称（如"1F大厅烟感01"）
    QString deviceType;     // "detector" | "valve"
    quint32 canId;          // CAN 帧 ID
    int     byteIndex;      // 数据字节索引 (0-7)
    int     bitIndex;       // 位索引 (0-7, 0=LSB)
};
```

### 2. `DeviceStatusWidget` — 单个设备状态指示器

**视觉设计**：
- 固定尺寸约 120×140px 的卡片式组件
- 顶部：设备图标（QPainter 绘制的 SVG 风格矢量图）
  - **烟温探测器**：圆形感应头 + 底座 + 指示灯
  - **阀门**：管道 + 手轮 + 阀体
- 中部：设备标签文字
- 底部：状态文字（"正常"/"报警"/"开启"/"关闭"）

**颜色逻辑**：
- 值=0：绿色主题 (#27AE60)，静态图标
- 值=1 (报警)：红色主题 (#E74C3C)，探测器图标**闪烁**（QTimer 500ms 切换透明度）

**闪烁动画**：
```cpp
// 使用 QPropertyAnimation 或 QTimer 切换红色/透明
m_flashTimer->setInterval(500);
connect(m_flashTimer, &QTimer::timeout, [this]() {
    m_alarmVisible = !m_alarmVisible;
    update(); // 触发重绘
});
```

**自定义绘制** (`paintEvent`)：
- 使用 QPainter + QPainterPath 绘制矢量图标
- 烟温探测器路径：
  - 底座矩形 + 感应头圆形 + 顶部 LED 指示灯
  - 报警时 LED 红色闪烁，正常时绿色常亮
- 阀门路径：
  - 水平管道（圆角矩形） + 垂直阀杆 + 手轮（椭圆）
  - 开启时绿色，关闭时红色

### 3. `CanProtocolMonitor` — 主监控面板

**继承**：`QDialog`

**布局**：
```
┌──────────────────────────────────────────┐
│ [配置] [导入] [导出] [清空]   连接状态 ●  │  ← 工具栏
├──────────────────────────────────────────┤
│ ┌────────┐ ┌────────┐ ┌────────┐ ┌─────┐│
│ │ 探测器 │ │ 探测器 │ │  阀门  │ │ ... ││  ← 网格布局
│ │  ✓绿   │ │  ✗红烁 │ │  ✓绿   │ │     ││     (4-5列)
│ └────────┘ └────────┘ └────────┘ └─────┘│
│ ┌────────┐ ┌────────┐                   │
│ │ 探测器 │ │  阀门  │                    │
│ │  ✓绿   │ │  ✓绿   │                    │
│ └────────┘ └────────┘                   │
├──────────────────────────────────────────┤
│ 日志：最近告警事件列表                    │  ← QPlainTextEdit
└──────────────────────────────────────────┘
```

**核心逻辑**：
```cpp
// 连接 CanInterface 信号
connect(m_can, &CanInterface::frameReceived, this, &CanProtocolMonitor::onFrameReceived);

void CanProtocolMonitor::onFrameReceived(const CanFrame &frame) {
    for (const auto &mapping : m_mappings) {
        if (frame.id != mapping.canId) continue;
        if (mapping.byteIndex >= frame.data.size()) continue;
        
        quint8 byteVal = static_cast<quint8>(frame.data[mapping.byteIndex]);
        bool bitVal = (byteVal >> mapping.bitIndex) & 0x01;
        
        // 更新对应 widget
        if (auto *widget = m_deviceWidgets.value(mapping.deviceId)) {
            widget->setStatus(bitVal);
        }
    }
}
```

**配置持久化**：使用 JSON 文件存储映射配置

### 4. `CanProtocolConfigDialog` — 协议配置对话框

**布局**：QTableWidget + 按钮
```
┌──────────────────────────────────────────────┐
│ 设备ID │ 标签 │ 类型 │ CAN ID │ 字节 │ 位 │  │
│   1    │1F烟感│探测器│ 0x100 │  0   │ 3 │[删]│
│   2    │1F阀门│ 阀门 │ 0x101 │  1   │ 0 │[删]│
│  ...   │ ...  │ ...  │  ...  │ ...  │...│... │
├──────────────────────────────────────────────┤
│ [+添加] [导入JSON] [导出JSON]  [确定] [取消]  │
└──────────────────────────────────────────────┘
```

### 5. 图标绘制详细规范

**烟温探测器** (120×100 绘制区域)：
```
      ● LED 指示灯 (8px圆)
      │
   ╭──┴──╮
   │ 感应 │ ← 圆形感应头 (60px圆)
   │ 网格 │   内部网格线
   ╰──┬──╯
   ┌──┴──┐
   │ 底座 │ ← 底座矩形 (70×16px, 圆角)
   └─────┘
```

**阀门** (120×100 绘制区域)：
```
    ╭──╮
    │手│ ← 手轮椭圆 (40×15px)
    │轮│
    ╰┬┬╯
     ││ ← 阀杆竖线
  ═══╪╪═══ ← 管道 (水平圆角矩形, 80×24px)
     ││
```

### 6. 集成方式

在 `mainwindow.cpp` 中添加类似 `openCANTool()` 的方法：
```cpp
void MainWindow::openProtocolMonitor() {
    auto *monitor = new CanProtocolMonitor(m_canInterface, this);
    monitor->show();
}
```

---

## 验证方式

1. **编译验证**：qmake + make 编译无错误
2. **UI 验证**：打开面板 → 添加配置 → 看到探测器/阀门图标正确绘制
3. **颜色验证**：模拟发送 CAN 帧，观察图标颜色切换和闪烁动画
4. **持久化验证**：导出/导入 JSON 配置，重启后配置保持
