# React + TypeScript 完整移植方案：CAN 设备状态监控面板

以下方案基于对现有 Electron 应用（`status-monitor-electron/`）的完整逆向分析设计，涵盖 `index.html`、`renderer.js`（约 2800 行）、`style.css`（约 1400 行）中全部功能。

------

## 1. 项目目录结构

采用 monorepo 风格，新版 React 应用与现有 Electron 应用并列，共享 `package.json` 中的 workspace 管理。



```
status-monitor-electron/          # 现有 Electron 版（保持不变）
  index.html
  main.js
  renderer.js
  style.css
  package.json

status-monitor-react/             # 新建 React + TypeScript 版
  index.html                      # Vite 入口 HTML
  package.json                    # pnpm workspace
  tsconfig.json
  tsconfig.node.json
  vite.config.ts
  electron/                       # Electron 主进程（精简版，仅窗口管理）
    main.ts
    preload.ts                    # （可选）contextBridge
  public/
    favicon.svg
  src/
    main.tsx                      # React 入口
    App.tsx                       # 根组件
    vite-env.d.ts

    types/                        # 全局类型定义
      device.ts                   # DeviceMapping, DeviceType, UpdateMessage 等
      layout.ts                   # Room, LayoutTemplate, LayoutPreset 等
      events.ts                   # LogEntry, LogType

    constants/                    # 常量
      templates.ts                # submarineTemplate, buildingTemplate, warshipTemplate
      defaults.ts                 # 默认 Layout、GridSnap 选项、主题选项
      ws-protocol.ts              # WS 消息类型常量

    store/                        # Zustand 状态管理
      useWebSocketStore.ts        # 连接状态、最新 mappings、onMessage 回调
      useDeviceStore.ts           # 设备状态（latestMappings 实时更新）
      useLayoutStore.ts           # 布局状态（rooms, devices, backgroundTemplate, editMode）
      useThemeStore.ts            # 主题选择、应用
      useLogStore.ts              # 双通道日志（alarmLogs, infoLogs）
      useUIStore.ts               # UI 状态（clock, footerHeight, drawerOpen, modalOpen...）

    hooks/                        # 自定义 Hooks
      useWebSocket.ts             # WebSocket 连接管理 + 自动重连
      useDragAndDrop.ts           # 拖拽（卡片/房间） 含网格吸附
      useRoomResize.ts            # 房间四方向 + SE 角拉伸
      useSystemClock.ts           # 时钟更新（每秒）
      useLayoutPersistence.ts     # localStorage 多 Slot + JSON 导入导出
      useRegionContainment.ts     # 房间内设备计数 + 报警状态判定
      useThemeVariables.ts        # 动态注入 CSS 变量

    components/
      layout/
        AppHeader.tsx             # 导航栏（Logo、模式切换、主题选择、时钟、连接状态）
        AppHeader.module.css
        AppNav.tsx                # 实时监控/布局设置 切换
        AppNav.module.css
        LayoutToolbar.tsx         # 布局子工具栏（模板、Slot、网格吸附、导入导出、房间操作）
        LayoutToolbar.module.css
        SystemClock.tsx
        ConnectionStatus.tsx      # 连接状态指示器（绿点/红点 + 文字）
        ConnectionStatus.module.css

      devices/
        DeviceCard.tsx            # 设备卡片（SVG 图标 + 标签 + 状态文字）
        DeviceCard.module.css
        DetectorIcon.tsx          # 烟温探测器 SVG 组件（含雷达脉冲动画）
        ValveIcon.tsx             # 阀门 SVG 组件（含流动动画、旋转）
        DeviceGrid.tsx            # 网格模式容器（auto-fill grid）
        DeviceGrid.module.css
        DeviceCanvas.tsx          # 画布模式容器（3000x2000 绝对定位）
        DeviceCanvas.module.css
        UnplacedDock.tsx          # 未摆放设备 Dock 区
        UnplacedDock.module.css

      rooms/
        RoomBox.tsx               # 房间容器（标题、计数徽标、操作按钮、拖拽、缩放手柄）
        RoomBox.module.css
        RoomResizeHandles.tsx     # 四方向 + SE 缩放手柄组件
        AddRoomModal.tsx          # 新建房间对话框
        AddRoomModal.module.css
        BackgroundTemplate.tsx    # SVG 背景模板（核潜艇/写字楼/战舰）
        templates/
          SubmarineBg.tsx         # 潜艇 SVG React 组件
          BuildingBg.tsx          # 写字楼 SVG React 组件
          WarshipBg.tsx           # 战舰 SVG React 组件

      logs/
        AlarmFooter.tsx           # 底部报警日志面板（可拖拽调整高度）
        AlarmFooter.module.css
        LogItem.tsx               # 单条日志（system/alarm/recovery/warning/raw）
        LogItem.module.css
        InfoDrawer.tsx            # 右侧信息日志抽屉
        InfoDrawer.module.css

      common/
        EmptyState.tsx            # 雷达扫描等待状态
        EmptyState.module.css
        Modal.tsx                 # 通用模态框
        Modal.module.css
        ThemeSelector.tsx         # 主题选择下拉框
        GridSnapSelector.tsx      # 网格吸附选择器
        TemplateSelector.tsx      # 模板选择器
        LayoutSlotSelector.tsx    # 已存布局 Slot 选择器

    styles/
      themes/                     # 主题 CSS 变量定义
        _cyber.css                # 科技蓝
        _emerald.css              # 翡翠绿
        _amber.css                # 琥珀金
        _ruby.css                 # 烈焰红
      _variables.css              # CSS 自定义属性（:root + 各主题）
      _animations.css             # 全局动画关键帧（雷达扫描、LED 闪烁、报警脉冲、流体流动）
      _scrollbar.css              # 滚动条样式
      _base.css                   # 全局 reset + 基础排版
      _mixins.css                 # 常用效果 mixin（毛玻璃、霓虹边框）

    utils/
      svg-helpers.ts              # 生成探测器和阀门 SVG 字符串（备用于非 React 渲染路径）
      layout-helpers.ts           # 网格吸附、碰撞检测、模板设备分配
      time-helpers.ts             # 日期格式化
      log-helpers.ts              # 日志消息翻译（translateRawMessage）
      storage-helpers.ts          # localStorage 读写封装
```

**关键文件计数**：约 55 个源文件，功能模块化，每模块独立目录。

------

## 2. 组件树



```
<App>
  <ThemeVariablesProvider />          -- 注入 CSS 变量到 document.body
  <AppHeader>
    <LogoArea />
    <AppNav />                        -- 实时监控 / 布局设置 / 信息日志 / 报警面板 按钮
    <HeaderInfoGroup>
      <ThemeSelector />               -- 4 种主题下拉菜单
      <SystemClock />                 -- 实时时钟（每秒更新）
      <ConnectionStatus />            -- WebSocket 连接状态指示器
    </HeaderInfoGroup>
  </AppHeader>

  <LayoutToolbar />                    -- 仅在布局模式下显示
    <TemplateSelector />
    <LayoutSlotSelector />
    <GridSnapSelector />
    [导入/导出/新建房间/重置/保存] 按钮

  <main className="app-main">
    <EmptyState />                    -- 无设备时显示雷达扫描动画

    <UnplacedDock />                  -- 未摆放设备（布局/画布模式）
      <DeviceCard />[]

    <DeviceGrid />                    -- 网格模式
    <OR>
    <DeviceCanvas>                    -- 画布模式 (3000x2000)
      <BackgroundTemplate />          -- SVG 背景（潜艇/写字楼/战舰）
        <RoomBox />[]                -- 房间（可拖拽、缩放）
          <RoomResizeHandles />
          <DeviceCard />[]            -- 房间内的设备
        <DeviceCard />[]             -- 未归入房间的自由设备
    </DeviceCanvas>
  </main>

  <AlarmFooter />                     -- 底部报警日志（可折叠、拖拽调整高度）
    <LogItem />[]

  <InfoDrawer />                      -- 右侧信息日志抽屉
    <LogItem />[]

  <AddRoomModal />                    -- 新建房间模态框
</App>
```

**渲染逻辑的分支路径**：

| 条件                               | 渲染内容                                              |
| ---------------------------------- | ----------------------------------------------------- |
| 无设备 (latestMappings 为空)       | `EmptyState`                                          |
| 有设备 + 非编辑模式 + 无已保存布局 | `DeviceGrid`（grid 布局）                             |
| 有设备 + 编辑模式 OR 有已保存布局  | `DeviceCanvas`（绝对定位）+ `UnplacedDock` + 背景模板 |

------

## 3. 数据流设计

### 3.1 Zustand Store 设计

**Store 拆分原则**：按功能域拆分，避免单一 Store 过大，同时保证跨 Store 访问的便捷性（Zustand 支持 `getState()` 跨 store 读取）。

#### `useWebSocketStore`



```typescript
interface WebSocketState {
  connectionStatus: 'initializing' | 'connected' | 'disconnected';
  latestMappings: DeviceMapping[];   // 从 config 消息获取的完整列表
  // actions
  connect: () => void;
  disconnect: () => void;
  setMappings: (mappings: DeviceMapping[]) => void;
  updateDeviceStatus: (deviceId: number, status: boolean) => void;
}
```

#### `useDeviceStore`



```typescript
interface DeviceState {
  // 设备状态字典，以 deviceId 为 key
  devices: Record<number, {
    mapping: DeviceMapping;
    status: boolean;
    prevStatus: boolean;
    lastUpdate: string | null;
  }>;
  // actions
  initFromMappings: (mappings: DeviceMapping[]) => void;
  updateDevice: (msg: UpdateMessage) => void;
  getDeviceById: (id: number) => DeviceStateEntry | undefined;
}
```

#### `useLayoutStore`



```typescript
interface LayoutState {
  isEditMode: boolean;
  gridSnapSize: number;              // 1 | 5 | 10 | 20
  backgroundTemplate: '' | 'submarine' | 'building' | 'warship';
  rooms: Room[];
  devicePositions: Record<number, { x: number; y: number }>; // deviceId -> position
  // actions
  setEditMode: (mode: boolean) => void;
  setGridSnap: (size: number) => void;
  addRoom: (name: string) => void;
  removeRoom: (id: string) => void;
  renameRoom: (id: string, name: string) => void;
  updateRoomGeometry: (id: string, partial: Partial<Room>) => void;
  setDevicePosition: (deviceId: number, x: number, y: number) => void;
  removeDevicePosition: (deviceId: number) => void;
  loadTemplate: (template: LayoutPreset) => void;
  loadFromStorage: () => void;
  saveToStorage: () => void;
  saveToSlot: (slotName: string) => void;
  loadFromSlot: (slotName: string) => void;
  exportToJSON: () => string;
  importFromJSON: (json: string) => boolean;
  resetLayout: () => void;
}
```

#### `useThemeStore`



```typescript
type ThemeName = 'cyber' | 'emerald' | 'amber' | 'ruby';

interface ThemeState {
  currentTheme: ThemeName;
  setTheme: (theme: ThemeName) => void;  // 同时持久化到 localStorage
  initTheme: () => void;                 // 从 localStorage 读取
}
```

#### `useLogStore`



```typescript
type LogType = 'system' | 'alarm' | 'recovery' | 'warning' | 'raw';

interface LogEntry {
  id: string;
  timestamp: string;
  content: string;
  type: LogType;
}

interface LogState {
  alarmLogs: LogEntry[];    // 底部报警日志（上限 200 条）
  infoLogs: LogEntry[];     // 右侧信息日志（上限 200 条）
  // actions
  appendLog: (content: string, type: LogType) => void;
  clearAlarmLogs: () => void;
  clearInfoLogs: () => void;
}
```

#### `useUIStore`



```typescript
interface UIState {
  footerHeight: number;         // 默认 180, 范围 80-450
  footerVisible: boolean;
  infoDrawerOpen: boolean;
  modalAddRoomOpen: boolean;
  newRoomName: string;
  // actions
  setFooterHeight: (h: number) => void;
  toggleFooter: () => void;
  toggleInfoDrawer: () => void;
  setModalOpen: (open: boolean) => void;
  setNewRoomName: (name: string) => void;
}
```

### 3.2 数据流图



```
Qt 后端 (QWebSocketServer :12345)
       |
       |  config / update 消息
       v
  useWebSocket  (自定义 hook)
       |
       ├──> useWebSocketStore (connectionStatus, latestMappings)
       |          |
       |          ├──> useDeviceStore.initFromMappings()   -- 收到 config
       |          └──> useDeviceStore.updateDevice()       -- 收到 update
       |
       ├──> useLogStore.appendLog(translatedMsg, 'raw')   -- 原始报文翻译到信息日志
       |
       └──> useLogStore.appendLog(formattedMsg, type)     -- 报警/恢复/警告到报警日志
  
  useThemeStore  ←→  localStorage('smile_code_theme')
  useLayoutStore ←→  localStorage('smile_code_layout_v1' / 'smile_code_layout_slot_*')
  
  用户交互:
  - 拖拽设备卡片 → useDragAndDrop hook → useLayoutStore.setDevicePosition()
  - 拖拽/缩放房间 → useRoomResize hook → useLayoutStore.updateRoomGeometry()
  - 模式切换 → useLayoutStore.setEditMode()
  - 新建房间 → useUIStore → AddRoomModal → useLayoutStore.addRoom()
  
  被动更新:
  - useRegionContainment hook → 计算房间内设备数、报警状态 → 更新 room badge + alarm-active class
  - useSystemClock hook → 每秒更新显示
```

### 3.3 类型定义



```typescript
// types/device.ts
export type DeviceType = 'detector' | 'valve';

export interface DeviceMapping {
  deviceId: number;
  label: string;
  deviceType: DeviceType;
  canId: number;
  byteIndex: number;
  bitIndex: number;
  status: boolean;
}

export interface UpdateMessage {
  type: 'update';
  deviceId: number;
  status: boolean;
  prevStatus: boolean;
  label: string;
  deviceType: DeviceType;
  timestamp: string;
}

export interface ConfigMessage {
  type: 'config';
  mappings: DeviceMapping[];
}

export type WSMessage = ConfigMessage | UpdateMessage;

// types/layout.ts
export interface Room {
  id: string;
  name: string;
  x: number;
  y: number;
  w: number;
  h: number;
}

export interface LayoutData {
  backgroundTemplate: '' | 'submarine' | 'building' | 'warship';
  rooms: Room[];
  devices: Record<number, { x: number; y: number }>;
}

export type LayoutPreset = LayoutData;
```

------

## 4. 路由设计

本应用是**单页应用**，无需传统路由。两个"模式"（实时监控 / 布局设置）通过 `useLayoutStore.isEditMode` 状态切换，不涉及 URL 变更。

如果需要将来扩展（例如增加历史趋势页面、设置页面），可以引入 `react-router-dom` v6：



```typescript
// 预留结构（不强制）
<Routes>
  <Route path="/" element={<MonitorPage />} />       // 当前主页面
  <Route path="/layout" element={<LayoutPage />} />  // 布局设置（可选拆分为独立页面）
  <Route path="/history" element={<HistoryPage />} /> // 未来扩展
</Routes>
```

但现在所有功能集中在一个视图，用状态切换即可，**不引入路由库**。

------

## 5. 关键技术选型和 npm 依赖

### 5.1 核心依赖

| 包名        | 版本  | 用途                                    |
| ----------- | ----- | --------------------------------------- |
| `react`     | ^18.3 | 框架核心                                |
| `react-dom` | ^18.3 | DOM 渲染                                |
| `zustand`   | ^4.5  | 状态管理（轻量，无 boilerplate）        |
| `immer`     | ^10.0 | 配合 Zustand 不可变更新（可选，但推荐） |
| `uuid`      | ^9.0  | 日志条目 ID、房间 ID 生成               |

### 5.2 开发依赖

| 包名                   | 版本   | 用途                     |
| ---------------------- | ------ | ------------------------ |
| `typescript`           | ^5.4   | 类型检查                 |
| `vite`                 | ^5.2   | 构建工具                 |
| `@vitejs/plugin-react` | ^4.2   | Vite React 支持          |
| `electron`             | ^28.2  | 桌面壳（与现有版本一致） |
| `electron-builder`     | ^24.13 | 打包（可选）             |
| `eslint`               | ^8.57  | 代码规范                 |
| `@types/react`         | ^18.2  | React 类型               |
| `@types/uuid`          | ^9.0   | UUID 类型                |

### 5.3 为什么不选某些库

- **不需 @dnd-kit**：现有拖拽逻辑是原生 mousedown/mousemove/mouseup，用自定义 hook 封装更轻量、且能精确复现现有行为（网格吸附、房间缩放的四方向逻辑）。自定义 hook 仅约 150 行。
- **不需 react-router**：只有两个模式切换，不是页面跳转。
- **不需 tailwind**：保留 CSS 变量体系是核心要求。CSS Modules 更直接，可以精确复现 1400 行现有样式，且 theme 变量天然穿透 CSS Modules。
- **不需 classnames/clsx**：可以用模板字符串或简单辅助函数。
- **不需 axios/fetch**：WebSocket 是唯一通信方式。

### 5.4 与现有 package.json 的关系

现有的 `status-monitor-electron/package.json` 保持不动。新建 `status-monitor-react/package.json`。如果是 workspace 管理，可以在根目录添加 `pnpm-workspace.yaml`：



```yaml
packages:
  - 'status-monitor-electron'
  - 'status-monitor-react'
```

------

## 6. CSS 架构

### 6.1 主题变量体系（完全保留现有方案）

在 `src/styles/_variables.css` 中定义：



```css
/* :root 默认值 (科技蓝) */
:root {
  --primary-color: #00f0ff;
  --primary-glow: rgba(0, 240, 255, 0.4);
  --primary-glow-light: rgba(0, 240, 255, 0.15);
  --primary-border: rgba(0, 240, 255, 0.25);
  --bg-color: #070b13;
  --panel-bg: rgba(15, 23, 42, 0.6);
  --panel-border: rgba(255, 255, 255, 0.05);
}

/* 各主题通过 body class 覆盖 */
.theme-cyber  { /* 同上，科技蓝 */ }
.theme-emerald { /* #10b981 系列 */ }
.theme-amber   { /* #f59e0b 系列 */ }
.theme-ruby    { /* #ef4444 系列 */ }
```

**使用方式**：在 `useThemeStore.setTheme()` 中，移除 `document.body` 的所有 `theme-*` class，添加新的 `theme-{name}`。CSS Modules 中引用 `var(--primary-color)` 等变量即可自动响应主题切换。

### 6.2 样式方案：CSS Modules

每个组件搭配 `.module.css` 文件，组件内部使用 `import styles from './Component.module.css'`。

**在 CSS Modules 中引用全局变量**：CSS Modules 不会隔离 CSS 自定义属性（var），所以 `var(--primary-color)` 在所有组件中均可直接使用。

**全局动画**：在 `_animations.css` 中定义所有 `@keyframes`（`scan`, `card-alert`, `led-blink`, `svg-radar-wave`, `fluid-flow`, `room-alarm-pulse`, `modal-fade-in` 等），在 `main.tsx` 中 `import './styles/_animations.css'`。CSS Modules 可以通过 `composes` 或 `animation-name` 直接引用全局关键帧。

**非主题相关通用样式**（如 `.btn-clear`, `.nav-item`, `.system-clock`）：在 CSS Modules 中逐组件定义，避免全局样式污染。

### 6.3 样式文件组织



```
src/styles/
  _variables.css       -- :root + body.theme-* 变量定义（被所有模块引用）
  _animations.css      -- 全局关键帧定义（import 到 main.tsx）
  _base.css            -- *, body, #root 基础样式
  _scrollbar.css       -- ::-webkit-scrollbar 样式
```

CSS Modules 全局引用方式（在 main.tsx 中）：



```typescript
import './styles/_base.css';
import './styles/_animations.css';
import './styles/_scrollbar.css';
import './styles/_variables.css';
```

组件内：



```css
/* DeviceCard.module.css */
.card {
  background: var(--panel-bg);
  border: 1px solid var(--panel-border);
}
.alarmStatus {
  animation: card-alert 1.5s infinite alternate; /* 引用全局 keyframes */
}
```

------

## 7. 实现优先级（分 4 阶段）

### 第一阶段：核心骨架与设备展示（1-2 天）

**目标**：可运行的 React 应用，显示设备卡片网格，WebSocket 连接，主题切换，日志系统。

| 序号 | 任务                                                         | 文件                                                  |
| ---- | ------------------------------------------------------------ | ----------------------------------------------------- |
| 1.1  | 初始化 Vite + React + TS 项目，安装依赖                      | `vite.config.ts`, `package.json`                      |
| 1.2  | 设置 CSS 架构：`_variables.css`, `_base.css`, `_animations.css`, `_scrollbar.css` | `src/styles/*`                                        |
| 1.3  | 实现所有类型定义                                             | `src/types/*`                                         |
| 1.4  | 实现 Zustand stores：`useWebSocketStore`, `useDeviceStore`, `useThemeStore`, `useLogStore`, `useUIStore` | `src/store/*`                                         |
| 1.5  | 实现 `useWebSocket` hook（连接、重连、消息分发）             | `src/hooks/useWebSocket.ts`                           |
| 1.6  | 实现 `useSystemClock` hook                                   | `src/hooks/useSystemClock.ts`                         |
| 1.7  | 创建 `App`、`AppHeader`、`AppNav`、`ConnectionStatus`、`SystemClock`、`ThemeSelector` | `src/components/layout/*`                             |
| 1.8  | 创建 `DeviceCard`（+ `DetectorIcon`/`ValveIcon` SVG 组件）   | `src/components/devices/*`                            |
| 1.9  | 创建 `DeviceGrid`（auto-fill grid 布局）和 `EmptyState`      | `src/components/devices/*`, `src/components/common/*` |
| 1.10 | 创建 `AlarmFooter`、`InfoDrawer`、`LogItem`                  | `src/components/logs/*`                               |
| 1.11 | 实现主题切换逻辑（`useThemeStore.setTheme()` + `useThemeVariables`） | `src/store/useThemeStore.ts`                          |

**可验证结果**：应用启动后可看到科技风界面，连接 WebSocket 后显示设备卡片网格，报警日志和右侧信息日志可操作，4 种主题可切换。

### 第二阶段：布局系统与画布模式（2-3 天）

| 序号 | 任务                                                         | 文件                                      |
| ---- | ------------------------------------------------------------ | ----------------------------------------- |
| 2.1  | 实现 `useLayoutStore` 全部功能                               | `src/store/useLayoutStore.ts`             |
| 2.2  | 实现 `useDragAndDrop` hook（网格吸附、边界约束）             | `src/hooks/useDragAndDrop.ts`             |
| 2.3  | 实现 `useRoomResize` hook（四方向 + SE 角）                  | `src/hooks/useRoomResize.ts`              |
| 2.4  | 创建 `DeviceCanvas`（3000x2000 画布容器）                    | `src/components/devices/DeviceCanvas.tsx` |
| 2.5  | 创建 `RoomBox` + `RoomResizeHandles`                         | `src/components/rooms/*`                  |
| 2.6  | 创建 `UnplacedDock`                                          | `src/components/devices/UnplacedDock.tsx` |
| 2.7  | 创建 `LayoutToolbar` 及其子组件（模板/网格/Slot 选择器）     | `src/components/layout/LayoutToolbar.tsx` |
| 2.8  | 创建三个 SVG 背景模板（`SubmarineBg`, `BuildingBg`, `WarshipBg`） | `src/components/rooms/templates/*`        |
| 2.9  | 创建 `AddRoomModal`                                          | `src/components/rooms/AddRoomModal.tsx`   |
| 2.10 | 实现 `useRegionContainment` hook（房间设备计数 + 报警检测）  | `src/hooks/useRegionContainment.ts`       |
| 2.11 | 实现 `useLayoutPersistence`（localStorage 读写、多 Slot、JSON 导入导出） | `src/hooks/useLayoutPersistence.ts`       |

**可验证结果**：可在网格/画布模式间切换，拖拽设备卡片和房间，缩放房间，加载 3 种模板，创建/删除/重命名房间，设备放入房间后 badge 更新，房间报警高亮，布局可保存和导入导出。

### 第三阶段：动画、细节与 Electron 集成（1-2 天）

| 序号 | 任务                                                         | 文件                                           |
| ---- | ------------------------------------------------------------ | ---------------------------------------------- |
| 3.1  | 精调所有 CSS 动画：雷达扫描、报警脉冲、LED 闪烁、阀体旋转 90 度、流体流动虚线动画 | `src/styles/_animations.css`                   |
| 3.2  | 实现设备状态转换动画（卡片 border-color 过渡、svg 旋转过渡） | `src/components/devices/DeviceCard.module.css` |
| 3.3  | 实现房间 alarm-active 脉冲动画                               | `src/components/rooms/RoomBox.module.css`      |
| 3.4  | 创建 `electron/main.ts`（精简主进程，复现现有 main.js 逻辑） | `electron/main.ts`                             |
| 3.5  | 配置 Vite 的 electron 集成（`vite-plugin-electron` 或类似方案） | `vite.config.ts`                               |
| 3.6  | 实现日志条目上限 200 条时的自动清理                          | `src/store/useLogStore.ts`                     |
| 3.7  | 实现底部 footer 拖拽调整高度（80-450px 范围）                | `src/components/logs/AlarmFooter.tsx`          |
| 3.8  | 实现 info drawer 滑入/滑出动画                               | `src/components/logs/InfoDrawer.tsx`           |
| 3.9  | 实现 modal 淡入动画                                          | `src/components/common/Modal.tsx`              |

**可验证结果**：全部动画与现有版行为一致，可作为 Electron 应用运行，footer 可拖拽，drawer 可开关，modal 动画正常。

### 第四阶段：打磨与共存（1 天）

| 序号 | 任务                                         | 文件                                      |
| ---- | -------------------------------------------- | ----------------------------------------- |
| 4.1  | 添加错误边界（Error Boundary）               | `src/components/common/ErrorBoundary.tsx` |
| 4.2  | 添加 loading / 骨架屏状态                    | `src/components/common/Skeleton.tsx`      |
| 4.3  | 日志条目性能优化（虚拟滚动，如果 > 100 条）  | `src/components/logs/LogListVirtual.tsx`  |
| 4.4  | 布局撤销/重做（可选增强）                    | `src/store/useLayoutStore.ts` 扩展        |
| 4.5  | 端到端测试：连接真实 Qt 后端 + Electron 窗口 | 手动测试                                  |
| 4.6  | README 和文档更新                            | `README.md`                               |

------

## 8. 与现有 Electron 版的关系

### 共存策略

**两个版本完全独立并存**，互不干扰：



```
status-monitor-electron/    -- 现有版本（vanilla JS，完全保留）
status-monitor-react/       -- 新版本（React + TypeScript）
```

- 两个目录各有独立的 `package.json`、`node_modules`、构建配置。
- 现有版本的 `main.js` 和 `renderer.js` **不做任何修改**。
- 旧版 Electron 窗口加载 `index.html`，新版 Electron 窗口加载 `dist/index.html`（Vite 构建产物）。
- 新版本的 Electron 主进程 `electron/main.ts` 复刻现有 `main.js` 的逻辑（窗口尺寸、userData 路径、GPU 缓存禁用等），用 TypeScript 重写。

### 共享资源

| 资源                           | 处理方式                                                     |
| ------------------------------ | ------------------------------------------------------------ |
| Qt 后端 WebSocket 端口 (12345) | 完全共享，两个版本可同时连接（Qt 端 QWebSocketServer 支持多客户端） |
| localStorage key 名称          | **保持完全一致**：`smile_code_layout_v1`、`smile_code_layout_slot_*`、`smile_code_theme`。新旧版本可以共享同一份持久化数据。 |
| SVG 图案                       | 从 `renderer.js` 中的模板字符串提取为独立 React 组件，保持视觉一致 |
| CSS 变量值                     | 颜色值完全沿用现有 `style.css` 中的定义                      |
| 字体                           | 沿用 `Orbitron`、`Share Tech Mono`、`Inter`（Google Fonts）  |

### 渐进式迁移路径

1. **第一阶段**：新版 React 应用开发、调试，运行在浏览器中 (`http://localhost:5173`)
2. **第二阶段**：React 应用通过 Electron 打包运行（`electron/main.ts` 加载 Vite 构建产物）
3. **第三阶段**：用户可任意选择启动旧版或新版
4. **未来**：当新版稳定后，可以淘汰旧版 `status-monitor-electron/`，将新版重命名为 `status-monitor-electron/`

### 开发工作流



```bash
# 启动 Vite 开发服务器（浏览器预览）
cd status-monitor-react
pnpm run dev            # → http://localhost:5173

# 启动 Electron + Vite（桌面预览）
pnpm run dev:electron   # electron 进程加载 Vite dev server URL

# 生产构建
pnpm run build          # → dist/
pnpm run electron:build # → 可分发 exe

# 旧版照旧运行
cd ../status-monitor-electron
npm start               # 完全不受影响
```

### 关键注意事项

1. **contextIsolation 和 nodeIntegration**：新版 Electron 建议使用 `contextIsolation: true` 和 `preload.ts`，与旧版 `false` 的设置不同。新版只需加载静态文件，不需要 Node 集成。
2. **vite-plugin-electron**：推荐使用 `vite-plugin-electron` 或 `electron-vite` 来管理 Electron 主进程的 TypeScript 编译和热重载。
3. **打包**：新版独立打包，不影响旧版的分发流程。

------

## 关键迁移点对照表

| 功能点         | 旧版实现位置                                | 新版组件/Hook                                          |
| -------------- | ------------------------------------------- | ------------------------------------------------------ |
| WebSocket 连接 | `renderer.js:1-221`                         | `useWebSocket.ts`                                      |
| 设备卡片渲染   | `renderer.js:430-476`                       | `DeviceCard.tsx` + `DeviceGrid.tsx`                    |
| 状态更新       | `renderer.js:479-520`                       | `useDeviceStore.ts` + `DeviceCard` 响应式              |
| 网格模式       | `renderer.js:396-403`                       | `DeviceGrid.tsx`（CSS grid）                           |
| 画布模式       | `renderer.js:397-398`                       | `DeviceCanvas.tsx`（绝对定位）                         |
| 拖拽           | `renderer.js:522-615`                       | `useDragAndDrop.ts`                                    |
| 房间缩放       | `renderer.js:618-707`                       | `useRoomResize.ts` + `RoomResizeHandles.tsx`           |
| 房间报警检测   | `renderer.js:710-766`                       | `useRegionContainment.ts`                              |
| 布局持久化     | `renderer.js:107-141,1110-1135`             | `useLayoutPersistence.ts`                              |
| 主题系统       | `renderer.js:1153-1175` + `style.css:12-46` | `useThemeStore.ts` + `_variables.css`                  |
| SVG 图标       | `renderer.js:807-851`                       | `DetectorIcon.tsx` + `ValveIcon.tsx`                   |
| 模板 SVG 背景  | `renderer.js:291-381`                       | `SubmarineBg.tsx` / `BuildingBg.tsx` / `WarshipBg.tsx` |
| 日志系统       | `renderer.js:778-804`                       | `useLogStore.ts` + `LogItem.tsx`                       |
| 时钟           | `renderer.js:91-105`                        | `useSystemClock.ts` + `SystemClock.tsx`                |

------

### Critical Files for Implementation

- `m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-react/src/hooks/useWebSocket.ts` — WebSocket 连接管理和消息分发，是整个应用的数据入口，决定了 config/update 消息如何流转到各 store
- `m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-react/src/store/useLayoutStore.ts` — 布局状态的核心枢纽，管理 rooms、devicePositions、editMode、模板加载、localStorage 持久化等最复杂的业务逻辑
- `m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-react/src/components/devices/DeviceCard.tsx` — 设备卡片组件，集成了 SVG 图标渲染、状态样式切换（detector/valve 各 2 种状态 x 4 种主题 = 8 种视觉组合）、所有 CSS 动画
- `m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-react/src/hooks/useDragAndDrop.ts` — 拖拽 hook（卡片和房间复用），包含网格吸附算法、边界约束、从 Dock 拖入画布的逻辑
- `m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-react/src/styles/_variables.css` — 主题 CSS 变量定义文件，所有 4 套主题的颜色值均在此定义，被所有 CSS Modules 引用

所有分析完成。现在将最终方案写入计划文件。

**Write** [C:\Users\Phodon\.claude\plans\shiny-soaring-chipmunk.md](vscode-webview://0svip8crmts2ldnvu6u36pggq29r949mtb96b2us9ovq8huoc77n/index.html?id=2d9f8c45-39a2-48c5-8fbf-4d038ceecf0b&parentId=1&origin=9f12a6b0-ae0d-4fcb-857d-9a5d52ad5882&swVersion=5&extensionId=Anthropic.claude-code&platform=electron&vscode-resource-base-authority=vscode-resource.vscode-cdn.net&parentOrigin=vscode-file%3A%2F%2Fvscode-app&purpose=webviewView&session=68394636-a3cd-4a43-8ab8-cbe4305db605#)

285 lines

方案已编写完成，现在提交审批。

**Claude’s Plan** [C:\Users\Phodon\.claude\plans\shiny-soaring-chipmunk.md](vscode-webview://0svip8crmts2ldnvu6u36pggq29r949mtb96b2us9ovq8huoc77n/index.html?id=2d9f8c45-39a2-48c5-8fbf-4d038ceecf0b&parentId=1&origin=9f12a6b0-ae0d-4fcb-857d-9a5d52ad5882&swVersion=5&extensionId=Anthropic.claude-code&platform=electron&vscode-resource-base-authority=vscode-resource.vscode-cdn.net&parentOrigin=vscode-file%3A%2F%2Fvscode-app&purpose=webviewView&session=68394636-a3cd-4a43-8ab8-cbe4305db605#)

User approved the plan

方案已批准，开始实施第一阶段：核心骨架搭建。

**Update Todos**

- 初始化 Vite + React + TypeScript 项目并安装依赖
- 创建类型定义和常量文件 (types/, constants/)
- 搭建 CSS 架构 (_variables.css, _animations.css, _base.css, _scrollbar.css)
- 实现 6 个 Zustand Store
- 实现 useWebSocket + useSystemClock hooks
- 实现工具函数 (utils/)
- 创建布局组件 (AppHeader, AppNav, ConnectionStatus, SystemClock, ThemeSelector)
- 创建设备组件 (DeviceCard, DetectorIcon, ValveIcon, DeviceGrid, EmptyState)
- 创建日志组件 (AlarmFooter, InfoDrawer, LogItem)
- 组装 App.tsx 并验证运行