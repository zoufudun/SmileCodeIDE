# Status Monitor - React Web 版

基于 **React 19 + TypeScript + Vite + Zustand** 的 CAN 设备状态监控面板。

通过 WebSocket 连接 Qt 端 `QWebSocketServer`（端口 `ws://localhost:12345`），实时显示烟温探测器和阀门设备的状态。

## 快速开始

```bash
# 安装依赖
npm install

# 浏览器开发模式
npm run dev:web         # → http://localhost:5173

# Electron 桌面开发模式
npm run dev             # → 自动打开 Electron 窗口

# 生产构建
npm run build           # → dist/ + dist-electron/
```

## 项目结构

```
src/
├── main.tsx                # React 入口
├── App.tsx                 # 根组件
├── types/                  # TypeScript 类型定义
│   ├── device.ts           # DeviceMapping, UpdateMessage, ConfigMessage
│   └── layout.ts           # Room, LayoutData, LogEntry
├── constants/              # 常量
│   ├── templates.ts        # 3 种预设模板（核潜艇/写字楼/战舰）
│   └── defaults.ts         # 默认值（WS URL、尺寸、上限等）
├── store/                  # Zustand 状态管理（6 个独立 Store）
├── hooks/                  # 自定义 Hooks（WS、拖拽、缩放、布局等）
├── components/
│   ├── layout/             # AppHeader, AppNav, LayoutToolbar
│   ├── devices/            # DeviceCard, DeviceGrid, DeviceCanvas
│   ├── rooms/              # RoomBox, AddRoomModal, SVG 背景模板
│   ├── logs/               # AlarmFooter, InfoDrawer, LogItem
│   └── common/             # EmptyState, Modal, ErrorBoundary
├── styles/                 # CSS 变量体系 + 动画 + 基础样式
└── utils/                  # 工具函数
```

## 技术栈

| 类别 | 选型 |
|---|---|
| 框架 | React 19 + TypeScript |
| 构建 | Vite 8 |
| 状态管理 | Zustand 4 |
| 样式 | CSS Modules + CSS 自定义属性（4 套主题） |
| 桌面壳 | Electron 28 + vite-plugin-electron |
| 拖拽 | 自定义原生 hook（无第三方依赖） |

## WebSocket 协议

Qt 后端发送两种 JSON 消息：

**config**（客户端连接时发送完整设备列表）：
```json
{
  "type": "config",
  "mappings": [{
    "deviceId": 1, "label": "1F大厅烟感01",
    "deviceType": "detector", "canId": 256,
    "byteIndex": 0, "bitIndex": 0, "status": false
  }]
}
```

**update**（设备状态变化时实时广播）：
```json
{
  "type": "update", "deviceId": 1, "status": true,
  "prevStatus": false, "label": "1F大厅烟感01",
  "deviceType": "detector", "timestamp": "2026-07-19 14:30:25"
}
```

## 功能特性

- 📊 实时监控：WebSocket 自动连接/重连（3s），设备状态实时更新
- 🎨 4 套主题：科技蓝 / 翡翠绿 / 琥珀金 / 烈焰红，CSS 变量切换
- 🏗️ 布局编辑：网格模式 / 无限画布模式（3000x2000），设备自由拖拽
- 📐 房间系统：增删改、重命名、四方向 + SE 角缩放、3 种预设模板
- 🚢 SVG 背景：核潜艇 / 写字楼 / 水面战舰 高保真矢量图
- 📋 双通道日志：底栏报警日志（可拖拽高度 80-450px）+ 右侧信息抽屉
- ✨ 动画：雷达扫描、报警脉冲、LED 闪烁、阀体旋转 90°、流体流动虚线

## 与 Electron 版的关系

两个版本**完全独立并存**：

```
status-monitor-electron/   # 旧版：原生 HTML/CSS/JS + Electron
status-monitor-react/      # 新版：React + TypeScript + Vite + Electron
```

- 共享同一 Qt 后端（端口 12345）+ localStorage 键名
- 可同时运行，互不干扰

## 许可证

ISC
