# React + TypeScript CAN 设备状态监控面板 实施方案

## Context

现有 `status-monitor-electron/` 是基于原生 HTML/CSS/JS + Electron 的 CAN 设备监控面板（约 2800 行代码）。用户要求**保留现有版本**，新增第二种实现方案：采用 **React 18 + TypeScript + Vite + Zustand** 现代前端架构，功能完全等价但代码模块化、可维护性更强。

Qt 后端通过 `QWebSocketServer`（端口 `ws://localhost:12345`）广播两种 JSON 消息：
- **config**：客户端连接时发送完整设备映射列表
- **update**：设备状态变化时实时广播

两个版本共享同一个 WebSocket 服务端和 localStorage 键名，可并行运行。

## 项目目录结构

```
status-monitor-react/          # 新建目录，与 status-monitor-electron/ 并列
├── index.html                 # Vite 入口
├── package.json
├── tsconfig.json
├── tsconfig.node.json
├── vite.config.ts
├── electron/
│   └── main.ts                # Electron 主进程（窗口 + 缓存修复）
└── src/
    ├── main.tsx               # React 入口
    ├── App.tsx
    ├── App.module.css
    ├── types/
    │   ├── device.ts          # DeviceMapping, UpdateMessage, ConfigMessage
    │   └── layout.ts          # Room, LayoutData, LayoutPreset, LogEntry
    ├── constants/
    │   ├── templates.ts       # 3 种预设模板数据
    │   └── defaults.ts        # 默认值常量
    ├── store/
    │   ├── useWebSocketStore.ts
    │   ├── useDeviceStore.ts
    │   ├── useLayoutStore.ts  # 最复杂：rooms、devicePositions、持久化
    │   ├── useThemeStore.ts
    │   ├── useLogStore.ts     # 双通道日志（alarmLogs + infoLogs，上限 200 条）
    │   └── useUIStore.ts      # footerHeight、drawerOpen、modalOpen
    ├── hooks/
    │   ├── useWebSocket.ts        # 连接管理、自动重连（3s）、消息分发
    │   ├── useDragAndDrop.ts      # 原生 mousedown/move/up + 网格吸附 + 边界约束
    │   ├── useRoomResize.ts       # 四方向 + SE 角拉伸
    │   ├── useSystemClock.ts      # 每秒更新时钟
    │   ├── useLayoutPersistence.ts # localStorage 多 Slot + JSON 导入/导出
    │   └── useRegionContainment.ts # 房间内设备计数 + 报警状态判定
    ├── components/
    │   ├── layout/
    │   │   ├── AppHeader.tsx / .module.css   # Logo + 导航 + 时钟 + 连接状态
    │   │   ├── AppNav.tsx / .module.css      # 模式切换按钮组
    │   │   ├── LayoutToolbar.tsx / .module.css # 模板/网格/导入导出/房间操作
    │   │   ├── SystemClock.tsx
    │   │   ├── ConnectionStatus.tsx / .module.css
    │   │   └── ThemeSelector.tsx
    │   ├── devices/
    │   │   ├── DeviceCard.tsx / .module.css      # 核心卡片组件
    │   │   ├── DetectorIcon.tsx                   # 探测器 SVG（雷达动画）
    │   │   ├── ValveIcon.tsx                      # 阀门 SVG（流动+旋转动画）
    │   │   ├── DeviceGrid.tsx / .module.css       # 网格模式（auto-fill grid）
    │   │   ├── DeviceCanvas.tsx / .module.css     # 画布模式（3000x2000 绝对定位）
    │   │   └── UnplacedDock.tsx / .module.css     # 未摆放设备 Dock
    │   ├── rooms/
    │   │   ├── RoomBox.tsx / .module.css          # 房间容器（标题+徽标+操作+拖拽+缩放）
    │   │   ├── RoomResizeHandles.tsx              # 5 个缩放手柄
    │   │   ├── AddRoomModal.tsx / .module.css
    │   │   ├── BackgroundTemplate.tsx
    │   │   └── templates/
    │   │       ├── SubmarineBg.tsx                # 潜艇 SVG 背景
    │   │       ├── BuildingBg.tsx                 # 写字楼 SVG 背景
    │   │       └── WarshipBg.tsx                  # 战舰 SVG 背景
    │   ├── logs/
    │   │   ├── AlarmFooter.tsx / .module.css      # 底部报警日志（可拖拽高度 80-450px）
    │   │   ├── LogItem.tsx / .module.css
    │   │   └── InfoDrawer.tsx / .module.css       # 右侧信息日志抽屉
    │   └── common/
    │       ├── EmptyState.tsx / .module.css       # 雷达扫描等待动画
    │       └── Modal.tsx / .module.css
    ├── styles/
    │   ├── _variables.css     # :root 默认 + 4 套 theme-* CSS 变量
    │   ├── _animations.css    # 全局 keyframes（scan, card-alert, led-blink, fluid-flow 等）
    │   ├── _base.css          # reset + body 基础样式
    │   └── _scrollbar.css     # WebKit 滚动条统一样式
    └── utils/
        ├── layout-helpers.ts  # 网格吸附算法、房间碰撞检测、模板设备分配
        ├── time-helpers.ts    # 日期/时间格式化
        ├── log-helpers.ts     # 报文翻译（translateRawMessage → 中文自然语言）
        └── storage-helpers.ts # localStorage 封装
```

## 组件树

```
<App>
  <AppHeader>
    <LogoArea />
    <AppNav />                  {/* 实时监控 | 布局设置 | 信息日志 | 报警面板 */}
    <HeaderInfoGroup>
      <ThemeSelector />         {/* 科技蓝/翡翠绿/琥珀金/烈焰红 */}
      <SystemClock />           {/* YYYY-MM-DD HH:mm:ss */}
      <ConnectionStatus />      {/* 绿点/红点 + 连接文字 */}
    </HeaderInfoGroup>
  </AppHeader>

  <LayoutToolbar />             {/* 仅 isEditMode=true 时显示 */}

  <main>
    {无设备 → <EmptyState />}    {/* 雷达扫描动画 */}
    {网格模式 → <DeviceGrid>     {/* auto-fill grid */}
                 <DeviceCard />[]
               </DeviceGrid>}
    {画布模式 → <DeviceCanvas>   {/* 3000x2000 绝对定位 */}
                 <BackgroundTemplate />  {/* SVG 矢量背景 */}
                 <RoomBox />[]          {/* 房间 + 内部卡片 */}
                 <DeviceCard />[]       {/* 自由设备 */}
               </DeviceCanvas>}
    <UnplacedDock />            {/* 未摆放设备 */}
  </main>

  <AlarmFooter>                 {/* 可折叠/拖拽高度 */}
    <LogItem />[]               {/* system/alarm/recovery/warning */}
  </AlarmFooter>

  <InfoDrawer>                  {/* 右侧滑入/滑出 */}
    <LogItem />[]               {/* raw 类型日志 */}
  </InfoDrawer>

  <AddRoomModal />              {/* 新建房间对话框 */}
</App>
```

## 数据流设计

### Zustand Store 拆分（6 个独立 store）

| Store | 核心状态 | 关键 Actions |
|---|---|---|
| `useWebSocketStore` | connectionStatus, latestMappings | connect, setMappings, updateDeviceStatus |
| `useDeviceStore` | devices (Record<id, {mapping, status, prevStatus}>) | initFromMappings, updateDevice |
| `useLayoutStore` | isEditMode, gridSnapSize, backgroundTemplate, rooms[], devicePositions{} | 全套布局 CRUD + 持久化 + 模板加载 + JSON 导入导出 |
| `useThemeStore` | currentTheme ('cyber'\|'emerald'\|'amber'\|'ruby') | setTheme（同步更新 body class + localStorage） |
| `useLogStore` | alarmLogs[], infoLogs[]（各上限 200 条） | appendLog(content, type), clearAlarmLogs, clearInfoLogs |
| `useUIStore` | footerHeight, footerVisible, infoDrawerOpen, modalAddRoomOpen | 对应 toggle/set 方法 |

### 数据流路径

```
Qt QWebSocketServer :12345
  │ config / update JSON
  ▼
useWebSocket hook
  ├─ config  → useWebSocketStore.setMappings()
  │              └─ useDeviceStore.initFromMappings()
  │              └─ useLogStore.appendLog(translated, 'raw')
  ├─ update  → useWebSocketStore.updateDeviceStatus()
  │              └─ useDeviceStore.updateDevice()
  │              └─ useLogStore.appendLog(formatted, type)  // alarm/recovery/warning
  └─ 断线    → 3s 自动重连

用户交互:
  拖拽设备  → useDragAndDrop → useLayoutStore.setDevicePosition()
  拖拽房间  → useDragAndDrop → useLayoutStore.updateRoomGeometry()
  缩放房间  → useRoomResize  → useLayoutStore.updateRoomGeometry()
  切换主题  → useThemeStore.setTheme() → body class 更新 → CSS 变量生效
  加载模板  → useLayoutStore.loadTemplate() → rooms[] + devicePositions{} 更新
```

### 关键类型定义

```typescript
// types/device.ts
type DeviceType = 'detector' | 'valve';
interface DeviceMapping { deviceId: number; label: string; deviceType: DeviceType; canId: number; byteIndex: number; bitIndex: number; status: boolean; }
interface UpdateMessage { type: 'update'; deviceId: number; status: boolean; prevStatus: boolean; label: string; deviceType: DeviceType; timestamp: string; }
interface ConfigMessage { type: 'config'; mappings: DeviceMapping[]; }

// types/layout.ts
interface Room { id: string; name: string; x: number; y: number; w: number; h: number; }
interface LayoutData { backgroundTemplate: '' | 'submarine' | 'building' | 'warship'; rooms: Room[]; devices: Record<number, { x: number; y: number }>; }
```

## 技术选型

| 类别 | 选型 | 理由 |
|---|---|---|
| 框架 | React 18 + TypeScript 5 | 用户要求 |
| 构建 | Vite 5 | 快速 HMR，Electron 插件生态 |
| 状态管理 | Zustand 4 | 轻量无 boilerplate，支持跨 store 读取 |
| 样式方案 | CSS Modules + CSS 自定义属性 | 保留现有 4 套主题变量体系，var() 可穿透 CSS Modules |
| 拖拽 | 自定义 hook（useDragAndDrop） | 现有逻辑是原生 mousedown/move/up，约 150 行即可复现 |
| 桌面壳 | Electron 28 + vite-plugin-electron | 与旧版版本号一致 |
| 包管理 | pnpm | 用户偏好 |

**不引入**：react-router（单页模式切换）、@dnd-kit（自定义更轻量）、Tailwind（保留 CSS 变量体系）、axios（仅 WebSocket 通信）

## CSS 架构

### 主题变量体系（完整保留现有方案）

```css
/* _variables.css */
:root {
  --primary-color: #00f0ff;
  --primary-glow: rgba(0, 240, 255, 0.4);
  --primary-glow-light: rgba(0, 240, 255, 0.15);
  --primary-border: rgba(0, 240, 255, 0.25);
  --bg-color: #070b13;
  --panel-bg: rgba(15, 23, 42, 0.6);
}
.theme-cyber  { /* 同上 */ }
.theme-emerald { --primary-color: #10b981; --bg-color: #061712; ... }
.theme-amber   { --primary-color: #f59e0b; --bg-color: #140d04; ... }
.theme-ruby    { --primary-color: #ef4444; --bg-color: #140404; ... }
```

`useThemeStore.setTheme()` → 操作 `document.body.classList` 替换 `theme-*` → 所有 CSS Modules 中的 `var(--primary-color)` 自动响应。

### CSS Modules 引用全局变量示例

```css
/* DeviceCard.module.css */
.card {
  background: var(--panel-bg);
  border: 1px solid var(--primary-border);
  transition: border-color 0.3s, box-shadow 0.3s;
}
.card:hover { border-color: var(--primary-color); box-shadow: 0 0 8px var(--primary-glow); }
.alarmCard { animation: card-alert 1.5s infinite alternate; } /* 引用 _animations.css 中的全局 keyframes */
```

全局 CSS（`_base.css`, `_animations.css`, `_variables.css`, `_scrollbar.css`）在 `main.tsx` 中 `import` 一次。

## 实现分阶段

### 第一阶段：核心骨架（1-2 天）
- Vite + React + TS 项目初始化、依赖安装
- 所有 types/constants 定义
- 6 个 Zustand store 实现
- `useWebSocket` + `useSystemClock` hooks
- AppHeader、AppNav、ConnectionStatus、SystemClock、ThemeSelector
- DeviceCard（含 DetectorIcon/ValveIcon SVG）、DeviceGrid、EmptyState
- AlarmFooter、InfoDrawer、LogItem
- CSS 架构搭建（_variables.css、_animations.css、_base.css）
- **可验证**：应用启动、WebSocket 连接、设备卡片网格显示、主题切换、日志工作

### 第二阶段：布局系统（2-3 天）
- `useLayoutStore` 完整实现
- `useDragAndDrop` + `useRoomResize` hooks
- DeviceCanvas（画布模式）+ UnplacedDock
- RoomBox + RoomResizeHandles
- LayoutToolbar + 子选择器（模板/网格/Slot）
- 3 种 SVG 背景模板组件（SubmarineBg/BuildingBg/WarshipBg）
- AddRoomModal
- `useRegionContainment` + `useLayoutPersistence` hooks
- **可验证**：模式切换、拖拽设备/房间、缩放房间、加载模板、导入导出布局

### 第三阶段：动画 + Electron 集成（1-2 天）
- 所有 CSS 动画精调（雷达、报警、LED、阀体旋转、流体流动、房间脉冲）
- electron/main.ts 主进程
- vite-plugin-electron 集成
- 底部 footer 拖拽调整高度 + 右侧 drawer 滑入动画 + Modal 淡入动画
- 日志 200 条上限自动清理
- **可验证**：完整 Electron 应用运行，所有动画与旧版一致

### 第四阶段：打磨（1 天）
- ErrorBoundary 错误边界
- localStorage 键名与旧版完全一致（可共享持久化数据）
- 与真实 Qt 后端联调测试
- README 文档

## 与现有 Electron 版的关系

- **完全独立并存**：`status-monitor-electron/` 不修改，`status-monitor-react/` 全新目录
- **共享资源**：同一 WebSocket 端口（12345）、同一 localStorage 键名（smile_code_layout_v1 等）
- **独立运行**：`cd status-monitor-electron && npm start` 与 `cd status-monitor-react && pnpm run dev:electron` 互不干扰

## 验证方式

1. `pnpm run dev` → 浏览器打开 `http://localhost:5173`，确认界面渲染、主题切换
2. 启动 Qt CANTool → WebSocket 连接 → 确认设备卡片显示、实时状态更新、日志输出
3. `pnpm run dev:electron` → Electron 窗口确认
4. 拖拽设备、创建房间、加载模板 → 确认布局功能完整
5. 切换主题 → 确认 4 套主题颜色生效
6. 关闭 Qt → 确认重连倒计时 → 重启 Qt → 确认恢复连接