const wsUrl = "ws://localhost:12345";
let ws = null;
let reconnectTimer = null;
let latestMappings = [];

// Layout State
let isEditMode = false;
let gridSnapSize = 10;
let currentLayout = {
  backgroundTemplate: "", // "submarine" | "building" | "warship" | ""
  rooms: [],              // Array of { id, name, x, y, w, h }
  devices: {}             // Map of deviceId -> { x, y }
};

// Layout Templates Database (Coordinated with Submarine and Warship high fidelity outline shape)
const submarineTemplate = {
  backgroundTemplate: "submarine",
  rooms: [
    { id: "sub_1", name: "舱首/鱼雷舱", x: 150, y: 300, w: 180, h: 220 },
    { id: "sub_2", name: "指挥与战术中心", x: 400, y: 220, w: 220, h: 300 },
    { id: "sub_3", name: "生活休息舱", x: 680, y: 300, w: 180, h: 220 },
    { id: "sub_4", name: "反应堆舱区", x: 900, y: 280, w: 180, h: 240 },
    { id: "sub_5", name: "动力/推进舱", x: 1120, y: 300, w: 180, h: 220 }
  ],
  devices: {}
};

const buildingTemplate = {
  backgroundTemplate: "building",
  rooms: [
    { id: "bld_3", name: "3F - 云数据机房", x: 100, y: 110, w: 600, h: 100 },
    { id: "bld_2", name: "2F - 行政与会议中心", x: 100, y: 225, w: 600, h: 100 },
    { id: "bld_1", name: "1F - 研发测试中心", x: 100, y: 340, w: 600, h: 100 }
  ],
  devices: {}
};

const warshipTemplate = {
  backgroundTemplate: "warship",
  rooms: [
    { id: "ship_1", name: "舰艏武器库区", x: 60, y: 180, w: 200, h: 180 },
    { id: "ship_2", name: "舰桥驾驶控制舱", x: 280, y: 100, w: 240, h: 260 },
    { id: "ship_3", name: "舰舯机电舱室", x: 540, y: 180, w: 200, h: 180 },
    { id: "ship_4", name: "舰艉直升机库", x: 760, y: 160, w: 200, h: 200 }
  ],
  devices: {}
};

// UI Elements
const connStatus = document.getElementById("conn-status");
const emptyState = document.getElementById("empty-state");
const deviceGrid = document.getElementById("device-grid");
const logListAlarm = document.getElementById("log-list-alarm");
const logListRaw = document.getElementById("log-list-raw");
const btnClearLogs = document.getElementById("btn-clear-logs");

// Mode & Toolbar Elements
const btnMonitorMode = document.getElementById("btn-monitor-mode");
const btnLayoutMode = document.getElementById("btn-layout-mode");
const btnToggleInfo = document.getElementById("btn-toggle-info");
const btnToggleAlarm = document.getElementById("btn-toggle-alarm");
const layoutToolbar = document.getElementById("layout-toolbar");
const btnAddRegion = document.getElementById("btn-add-region");
const btnResetLayout = document.getElementById("btn-reset-layout");
const btnSaveLayout = document.getElementById("btn-save-layout");
const selectTemplate = document.getElementById("select-template");
const selectLayoutSlot = document.getElementById("select-layout-slot");
const btnExportFile = document.getElementById("btn-export-file");
const btnImportFile = document.getElementById("btn-import-file");
const fileImportInput = document.getElementById("file-import-input");
const unplacedDock = document.getElementById("unplaced-dock");
const unplacedContainer = document.getElementById("unplaced-container");

// Drawer & Modal Elements
const infoLogDrawer = document.getElementById("info-log-drawer");
const btnCloseDrawer = document.getElementById("btn-close-drawer");
const btnClearDrawerLogs = document.getElementById("btn-clear-drawer-logs");

const modalAddRoom = document.getElementById("modal-add-room");
const inputRoomName = document.getElementById("input-room-name");
const btnModalCancel = document.getElementById("btn-modal-cancel");
const btnModalCancelX = document.getElementById("btn-modal-cancel-x");
const btnModalConfirm = document.getElementById("btn-modal-confirm");

const modalRenameRoom = document.getElementById("modal-rename-room");
const inputRenameRoomName = document.getElementById("input-rename-room-name");
const btnRenameCancel = document.getElementById("btn-modal-rename-cancel");
const btnRenameCancelX = document.getElementById("btn-modal-rename-cancel-x");
const btnRenameConfirm = document.getElementById("btn-modal-rename-confirm");

// Footer Resizing & Closing Elements
const appFooter = document.getElementById("app-footer");
const resizeHandle = document.getElementById("resize-handle");
const btnCloseFooter = document.getElementById("btn-close-footer");

// System Clock display logic
function updateClock() {
  const clockEl = document.getElementById("system-clock");
  if (clockEl) {
    const now = new Date();
    const yyyy = now.getFullYear();
    const mm = String(now.getMonth() + 1).padStart(2, '0');
    const dd = String(now.getDate()).padStart(2, '0');
    const hh = String(now.getHours()).padStart(2, '0');
    const min = String(now.getMinutes()).padStart(2, '0');
    const ss = String(now.getSeconds()).padStart(2, '0');
    clockEl.textContent = `${yyyy}-${mm}-${dd} ${hh}:${min}:${ss}`;
  }
}
setInterval(updateClock, 1000);
updateClock();

// Load named layout slots dropdown option list
function loadLayoutSlots() {
  if (!selectLayoutSlot) return;
  selectLayoutSlot.innerHTML = `<option value="" disabled selected>💾 选择已存布局...</option>`;

  let slots = [];
  try {
    slots = JSON.parse(localStorage.getItem("smile_code_layout_slots") || "[]");
  } catch (e) {
    console.error(e);
  }

  slots.forEach(name => {
    const opt = document.createElement("option");
    opt.value = name;
    opt.textContent = `📁 ${name}`;
    selectLayoutSlot.appendChild(opt);
  });
}

// Load layout from LocalStorage
function loadLayoutFromStorage() {
  const saved = localStorage.getItem("smile_code_layout_v1");
  if (saved) {
    try {
      currentLayout = JSON.parse(saved);
      if (!currentLayout.rooms) currentLayout.rooms = currentLayout.regions || [];
      if (!currentLayout.devices) currentLayout.devices = {};
      if (!currentLayout.backgroundTemplate) currentLayout.backgroundTemplate = "";
    } catch (e) {
      console.error("Error parsing saved layout", e);
    }
  }
}

// Translate JSON packets into natural language
function translateRawMessage(rawData) {
  try {
    const msg = JSON.parse(rawData);
    if (msg.type === "config") {
      return `⚙️ 网关配置已同步：成功加载了 ${msg.mappings ? msg.mappings.length : 0} 个设备。`;
    }
    if (msg.type === "update") {
      const label = msg.label || `ID: #${msg.deviceId}`;
      const typeName = getDeviceTypeName(msg.deviceType);

      let stateDesc = "";
      if (msg.deviceType === "detector") {
        stateDesc = msg.status ? "(异常报警 🚨) 状态变更为: 报警" : "(复位正常 💚) 状态变更为: 复位";
      } else if (msg.deviceType === "valve" ||
        msg.deviceType === "valve_distributor" ||
        msg.deviceType === "selector_valve" ||
        msg.deviceType === "valve_zone" ||
        msg.deviceType === "zone_valve" ||
        msg.deviceType === "valve_main_isolation" ||
        msg.deviceType === "main_isolation_valve") {
        stateDesc = msg.status ? "(开启 🟢) 状态变更为: 开启" : "(关闭 🟠) 状态变更为: 关闭";
      } else if (msg.deviceType === "manual_alarm") {
        stateDesc = msg.status ? "(异常报警 🚨) 状态变更为: 报警" : "(复位正常 💚) 状态变更为: 复位";
      } else if (msg.deviceType === "gas_cylinder") {
        stateDesc = msg.status ? "(异常泄漏 🚨) 状态变更为: 泄漏" : "(压力正常 💚) 状态变更为: 正常";
      } else if (msg.deviceType === "water_pump") {
        stateDesc = msg.status ? "(启动运行 🟢) 状态变更为: 运行" : "(停止运转 🟠) 状态变更为: 停止";
      } else if (msg.deviceType === "pressure_switch") {
        stateDesc = msg.status ? "(开启 🟢) 状态变更为: 开启" : "(关闭 🟠) 状态变更为: 关闭";
      } else if (msg.deviceType === "mobile_spray_gun") {
        stateDesc = msg.status ? "(喷射中 🟢) 状态变更为: 喷射" : "(停止 🟠) 状态变更为: 停止";
      } else {
        stateDesc = msg.status ? "(异常 🚨) 状态变更为: 动作" : "(正常 💚) 状态变更为: 复位";
      }

      return `📡 设备变化：${typeName} [${label}] ${stateDesc}`;
    }
    return `📩 报文数据: ${rawData}`;
  } catch (e) {
    return `📩 原始数据: ${rawData}`;
  }
}

// Connect to Qt WebSocket Server
function connect() {
  if (ws) {
    ws.close();
  }

  appendLog("正在连接本地 Qt CAN 消息服务...", "system");
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    connStatus.className = "connection-status connected";
    connStatus.querySelector(".status-text").textContent = "已连接网关";
    appendLog("已成功连接到 Qt CAN 消息服务器！开始接收实时设备状态数据。", "system");
    if (reconnectTimer) {
      clearInterval(reconnectTimer);
      reconnectTimer = null;
    }
  };

  ws.onclose = () => {
    connStatus.className = "connection-status disconnected";
    connStatus.querySelector(".status-text").textContent = "未连接 (重试中...)";
    appendLog("与服务器断开连接，准备进行自动重连...", "warning");

    deviceGrid.style.display = "none";
    unplacedDock.style.display = "none";
    emptyState.style.display = "flex";

    if (!reconnectTimer) {
      reconnectTimer = setInterval(connect, 3000);
    }
  };

  ws.onerror = (err) => {
    console.error("WebSocket Error:", err);
  };

  ws.onmessage = (event) => {
    // Log translated websocket packets to Information Log panel
    const translatedMsg = translateRawMessage(event.data);
    appendLog(translatedMsg, "raw");

    try {
      const data = JSON.parse(event.data);
      if (data.type === "config") {
        latestMappings = data.mappings || [];
        renderWorkspace();
      } else if (data.type === "update") {
        handleUpdateMessage(data);
      }
    } catch (e) {
      console.error("Failed to parse WebSocket message:", e);
    }
  };
}

// Create Room DOM element
function createRoomElement(room) {
  const el = document.createElement("div");
  el.className = "room-box";
  el.id = `room-box-${room.id}`;
  el.dataset.id = room.id;
  el.style.left = room.x + "px";
  el.style.top = room.y + "px";
  el.style.width = room.w + "px";
  el.style.height = room.h + "px";

  el.innerHTML = `
    <div class="room-header">
      <div class="room-title-area">
        <span class="room-title">${room.name}</span>
        <span class="room-badge" id="room-badge-${room.id}">0</span>
      </div>
      <div class="room-actions">
        <button class="btn-room-rename" title="重命名">✎</button>
        <button class="btn-room-delete" title="删除">✕</button>
      </div>
    </div>
    <div class="room-resize-handle-t"></div>
    <div class="room-resize-handle-b"></div>
    <div class="room-resize-handle-l"></div>
    <div class="room-resize-handle-r"></div>
    <div class="room-resize-handle-se"></div>
  `;

  const titleEl = el.querySelector(".room-title");

  // Double click rename action (Enabled under all modes)
  titleEl.addEventListener("dblclick", (e) => {
    e.stopPropagation();
    showRenameModal(room, titleEl);
  });

  // Rename button action
  el.querySelector(".btn-room-rename").addEventListener("click", (e) => {
    e.stopPropagation();
    showRenameModal(room, titleEl);
  });

  // Delete action
  el.querySelector(".btn-room-delete").addEventListener("click", () => {
    if (confirm(`确定要删除房间 "${room.name}" 吗？该房间内的设备卡片不会被删除。`)) {
      currentLayout.rooms = currentLayout.rooms.filter(r => r.id !== room.id);
      el.remove();
      checkRegionContainment();
    }
  });

  setupDraggable(el, false);
  setupResizable(el);

  return el;
}

// Background Vectors Silhouette Getters
function getSubmarineBg() {
  return `
    <svg viewBox="0 0 2000 700" width="2000" height="700" xmlns="http://www.w3.org/2000/svg" style="position: absolute; left: 20px; top: 20px;">
      <defs>
        <!-- Cyber Blue Metallic Gradient for Submarine Hull -->
        <linearGradient id="sub-gradient" x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stop-color="#0f172a" stop-opacity="0.95"/>
          <stop offset="50%" stop-color="#1e293b" stop-opacity="0.85"/>
          <stop offset="100%" stop-color="#0b1329" stop-opacity="0.95"/>
        </linearGradient>
        <linearGradient id="reactor-glow" x1="0%" y1="0%" x2="0%" y2="100%">
          <stop offset="0%" stop-color="#ef4444" stop-opacity="0.3"/>
          <stop offset="100%" stop-color="#ef4444" stop-opacity="0.0"/>
        </linearGradient>
      </defs>
      
      <!-- Propeller Shaft & Propeller Blades -->
      <rect x="1800" y="385" width="40" height="15" fill="#334155" stroke="var(--primary-color)" stroke-width="1" opacity="0.8"/>
      <path d="M 1830,310 C 1830,310 1845,392 1810,392 M 1810,392 C 1845,392 1830,475 1830,475" stroke="var(--primary-color)" stroke-width="3" fill="none" opacity="0.6"/>
      
      <!-- Rudder / Vertical stabilizers -->
      <path d="M 1700,280 L 1750,220 L 1790,220 L 1760,340 Z" fill="#0f172a" stroke="var(--primary-color)" stroke-width="2" opacity="0.8"/>
      <path d="M 1700,500 L 1750,560 L 1790,560 L 1760,440 Z" fill="#0f172a" stroke="var(--primary-color)" stroke-width="2" opacity="0.8"/>
      
      <!-- Conning Tower / Sail / Command deck -->
      <path d="M 400,320 L 410,130 L 520,130 L 550,320 Z" fill="url(#sub-gradient)" stroke="var(--primary-color)" stroke-width="3"/>
      <!-- Sail details / Periscope masts -->
      <line x1="450" y1="130" x2="450" y2="70" stroke="var(--primary-color)" stroke-width="3" opacity="0.8"/>
      <circle cx="450" cy="70" r="3" fill="var(--primary-color)"/>
      <line x1="480" y1="130" x2="480" y2="50" stroke="var(--primary-color)" stroke-width="2" opacity="0.8"/>
      <line x1="480" y1="50" x2="490" y2="50" stroke="var(--primary-color)" stroke-width="2" opacity="0.8"/>
      
      <!-- Massive Submarine Main Outer Hull Shape -->
      <path d="M 50,392 C 70,280 250,220 500,220 L 1700,220 C 1780,220 1810,320 1810,392 C 1810,460 1780,560 1700,560 L 500,560 C 250,560 70,500 50,392 Z" 
            fill="url(#sub-gradient)" stroke="var(--primary-color)" stroke-width="3.5" filter="drop-shadow(0 0 15px var(--primary-glow))"/>
      
      <!-- Torpedo Tube Outlets (Nose) -->
      <path d="M 55,360 L 75,360 M 50,392 L 72,392 M 55,420 L 75,420" stroke="var(--primary-color)" stroke-width="2" opacity="0.7"/>
 
      <!-- Hull Internal Bulkhead Dividers -->
      <line x1="350" y1="240" x2="350" y2="540" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.3" stroke-dasharray="4, 4"/>
      <line x1="640" y1="240" x2="640" y2="540" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.3" stroke-dasharray="4, 4"/>
      <line x1="860" y1="240" x2="860" y2="540" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.3" stroke-dasharray="4, 4"/>
      <line x1="1080" y1="240" x2="1080" y2="540" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.3" stroke-dasharray="4, 4"/>
      
      <!-- Reactor Compartment Glow -->
      <rect x="865" y="240" width="210" height="300" fill="url(#reactor-glow)"/>
      <circle cx="970" cy="390" r="45" fill="none" stroke="#ef4444" stroke-width="1.5" stroke-opacity="0.4" stroke-dasharray="3, 3"/>
      <line x1="970" y1="310" x2="970" y2="370" stroke="#ef4444" stroke-width="3" stroke-opacity="0.5"/>
      <line x1="955" y1="310" x2="955" y2="370" stroke="#ef4444" stroke-width="3" stroke-opacity="0.5"/>
      <line x1="985" y1="310" x2="985" y2="370" stroke="#ef4444" stroke-width="3" stroke-opacity="0.5"/>
    </svg>
  `;
}

function getWarshipBg() {
  return `
    <svg viewBox="0 0 1100 500" width="100%" height="100%" xmlns="http://www.w3.org/2000/svg">
      <!-- Warship Hull & Superstructure -->
      <path d="M 50,300 L 120,200 L 350,200 L 370,140 L 480,140 L 500,200 L 650,200 L 670,160 L 750,160 L 770,220 L 980,220 L 1050,300 L 50,300 Z" fill="none" stroke="var(--primary-color)" stroke-width="2" stroke-opacity="0.18" stroke-dasharray="10, 5"/>
      <!-- Main deck line -->
      <line x1="50" y1="300" x2="1050" y2="300" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.15"/>
      <!-- Gun turret on bow -->
      <path d="M 170,200 L 190,185 L 220,185 L 230,200 Z" fill="none" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.15"/>
      <line x1="220" y1="190" x2="255" y2="190" stroke="var(--primary-color)" stroke-width="2" stroke-opacity="0.15"/>
      <!-- Radar mast -->
      <line x1="420" y1="140" x2="420" y2="70" stroke="var(--primary-color)" stroke-width="2" stroke-opacity="0.15"/>
      <path d="M 405,70 C 405,70 420,60 435,70" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.15"/>
    </svg>
  `;
}

function getBuildingBg() {
  return `
    <svg viewBox="0 0 1100 500" width="100%" height="100%" xmlns="http://www.w3.org/2000/svg">
      <!-- Outer building shell -->
      <rect x="80" y="80" width="640" height="380" rx="10" fill="none" stroke="var(--primary-color)" stroke-width="2" stroke-opacity="0.18" stroke-dasharray="10, 5"/>
      <!-- Floor dividers -->
      <line x1="80" y1="205" x2="720" y2="205" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.15"/>
      <line x1="80" y1="330" x2="720" y2="330" stroke="var(--primary-color)" stroke-width="1.5" stroke-opacity="0.15"/>
      <!-- Window grids in the background -->
      <line x1="200" y1="80" x2="200" y2="460" stroke="var(--primary-color)" stroke-width="1" stroke-dasharray="2, 8" stroke-opacity="0.08"/>
      <line x1="320" y1="80" x2="320" y2="460" stroke="var(--primary-color)" stroke-width="1" stroke-dasharray="2, 8" stroke-opacity="0.08"/>
      <line x1="440" y1="80" x2="440" y2="460" stroke="var(--primary-color)" stroke-width="1" stroke-dasharray="2, 8" stroke-opacity="0.08"/>
      <line x1="560" y1="80" x2="560" y2="460" stroke="var(--primary-color)" stroke-width="1" stroke-dasharray="2, 8" stroke-opacity="0.08"/>
      <!-- Rooftop antenna -->
      <line x1="400" y1="80" x2="400" y2="30" stroke="var(--primary-color)" stroke-width="2" stroke-opacity="0.15"/>
      <circle cx="400" cy="30" r="3" fill="var(--primary-color)" opacity="0.2"/>
    </svg>
  `;
}

// Render the grid / absolute canvas with configured devices
function renderWorkspace() {
  if (!latestMappings || latestMappings.length === 0) {
    deviceGrid.style.display = "none";
    unplacedDock.style.display = "none";
    emptyState.style.display = "flex";
    return;
  }

  emptyState.style.display = "none";

  const hasSavedLayout = currentLayout.rooms.length > 0 || Object.keys(currentLayout.devices).length > 0;

  if (hasSavedLayout || isEditMode) {
    deviceGrid.className = "device-grid canvas-mode";
    deviceGrid.style.display = "block";
  } else {
    deviceGrid.className = "device-grid";
    deviceGrid.style.display = "grid";
  }

  deviceGrid.innerHTML = "";
  unplacedContainer.innerHTML = "";

  // Render Silhouette Background Vector
  if (currentLayout.backgroundTemplate && (hasSavedLayout || isEditMode)) {
    const bgDiv = document.createElement("div");
    bgDiv.className = "canvas-background";
    if (currentLayout.backgroundTemplate === "submarine") {
      bgDiv.innerHTML = getSubmarineBg();
    } else if (currentLayout.backgroundTemplate === "warship") {
      bgDiv.innerHTML = getWarshipBg();
    } else if (currentLayout.backgroundTemplate === "building") {
      bgDiv.innerHTML = getBuildingBg();
    }
    deviceGrid.appendChild(bgDiv);
  }

  // Render Rooms
  if (hasSavedLayout || isEditMode) {
    currentLayout.rooms.forEach(room => {
      const roomEl = createRoomElement(room);
      deviceGrid.appendChild(roomEl);
    });
  }

  // Render Device Cards
  latestMappings.forEach(device => {
    const card = document.createElement("div");
    card.id = `device-card-${device.deviceId}`;
    card.className = `device-card kind-${device.deviceType} status-${device.status ? 1 : 0}`;
    card.dataset.id = device.deviceId;
    card.dataset.type = device.deviceType;
    card.dataset.label = device.label;

    const statusText = getStatusText(device.deviceType, device.status);
    const canIdHex = "0x" + (device.canId || 0).toString(16).toUpperCase().padStart(3, '0');
    const svgIcon = getDeviceSvg(device.deviceType);

    card.innerHTML = `
      <span class="device-canid">CAN ${canIdHex}</span>
      <span class="device-id-badge">#${device.deviceId}</span>
      <div class="device-icon-container">${svgIcon}</div>
      <span class="device-label" title="${device.label}">${device.label}</span>
      <span class="device-status">${statusText}</span>
    `;

    // Position card
    if (hasSavedLayout || isEditMode) {
      const coords = currentLayout.devices[device.deviceId];
      if (coords) {
        card.style.position = "absolute";
        card.style.left = coords.x + "px";
        card.style.top = coords.y + "px";
        deviceGrid.appendChild(card);
        setupDraggable(card, true);
      } else {
        card.style.position = "static";
        unplacedContainer.appendChild(card);
        setupDraggable(card, true);
      }
    } else {
      deviceGrid.appendChild(card);
    }
  });

  checkDockVisibility();
  checkRegionContainment();

  appendLog(`监控画布渲染完成，共加载 ${latestMappings.length} 个设备。`, "system");
}

// Update state on real-time broadcast message
function handleUpdateMessage(msg) {
  // Update state in latestMappings array
  const dev = latestMappings.find(d => d.deviceId === msg.deviceId);
  if (dev) {
    dev.status = msg.status;
  }

  const card = document.getElementById(`device-card-${msg.deviceId}`);
  if (!card) return;

  // Update card CSS class
  card.className = `device-card kind-${msg.deviceType} status-${msg.status ? 1 : 0}`;

  // Update status text
  const statusEl = card.querySelector(".device-status");
  if (statusEl) {
    statusEl.textContent = getStatusText(msg.deviceType, msg.status);
  }
  // Update region alarm trigger states and room badges
  checkRegionContainment();

  // 报警日志
  const typeName = getDeviceTypeName(msg.deviceType);
  const stText = getStatusText(msg.deviceType, msg.status);
  const icon = msg.status ? "🚨" : "✅";
  const logType = msg.status ? "alarm" : "recovery";
  appendLog(`${icon} 【${typeName}】[${msg.label}] → ${stText}`, logType);
}

// Drag & Drop Functionality
let activeDragItem = null;
let dragStartX = 0;
let dragStartY = 0;
let itemStartX = 0;
let itemStartY = 0;

function setupDraggable(element, isCard) {
  const handle = isCard ? element : element.querySelector(".room-header");
  if (!handle) return;

  handle.addEventListener("mousedown", (e) => {
    if (!isEditMode) return;
    if (e.target.tagName.toLowerCase() === "button" || e.target.classList.contains("room-title") || e.target.classList.contains("room-badge")) return;

    e.preventDefault();
    activeDragItem = element;

    dragStartX = e.clientX;
    dragStartY = e.clientY;

    const isInDock = element.parentNode.id === "unplaced-container";
    if (isInDock) {
      const rect = element.getBoundingClientRect();
      const canvasRect = deviceGrid.getBoundingClientRect();
      itemStartX = (rect.left - canvasRect.left) / zoomLevel + deviceGrid.scrollLeft;
      itemStartY = (rect.top - canvasRect.top) / zoomLevel + deviceGrid.scrollTop;

      unplacedContainer.removeChild(element);
      deviceGrid.appendChild(element);
      element.style.position = "absolute";
      element.style.left = itemStartX + "px";
      element.style.top = itemStartY + "px";

      checkDockVisibility();
    } else {
      itemStartX = parseInt(element.style.left) || 0;
      itemStartY = parseInt(element.style.top) || 0;
    }

    document.addEventListener("mousemove", onMouseMove);
    document.addEventListener("mouseup", onMouseUp);
  });
}

function onMouseMove(e) {
  if (!activeDragItem) return;
  const dx = (e.clientX - dragStartX) / zoomLevel;
  const dy = (e.clientY - dragStartY) / zoomLevel;

  let newX = itemStartX + dx;
  let newY = itemStartY + dy;

  // Snap to configured grid
  if (gridSnapSize > 1) {
    newX = Math.round(newX / gridSnapSize) * gridSnapSize;
    newY = Math.round(newY / gridSnapSize) * gridSnapSize;
  }

  const canvasWidth = deviceGrid.clientWidth;
  const canvasHeight = deviceGrid.clientHeight;
  const itemWidth = activeDragItem.offsetWidth;
  const itemHeight = activeDragItem.offsetHeight;

  newX = Math.max(0, Math.min(newX, canvasWidth - itemWidth));
  newY = Math.max(0, Math.min(newY, canvasHeight - itemHeight));

  activeDragItem.style.left = newX + "px";
  activeDragItem.style.top = newY + "px";

  checkRegionContainment();
}

function onMouseUp() {
  if (activeDragItem) {
    const isCard = activeDragItem.classList.contains("device-card");
    const id = activeDragItem.dataset.id;
    const x = parseInt(activeDragItem.style.left);
    const y = parseInt(activeDragItem.style.top);

    if (isCard) {
      currentLayout.devices[id] = { x, y };
    } else {
      const room = currentLayout.rooms.find(r => r.id === id);
      if (room) {
        room.x = x;
        room.y = y;
      }
    }
  }
  activeDragItem = null;
  document.removeEventListener("mousemove", onMouseMove);
  document.removeEventListener("mouseup", onMouseUp);
}

// Resizable regions (supports 4 directions + SE corner resize)
function setupResizable(roomElement) {
  const handles = {
    t: roomElement.querySelector(".room-resize-handle-t"),
    b: roomElement.querySelector(".room-resize-handle-b"),
    l: roomElement.querySelector(".room-resize-handle-l"),
    r: roomElement.querySelector(".room-resize-handle-r"),
    se: roomElement.querySelector(".room-resize-handle-se")
  };

  Object.entries(handles).forEach(([dir, handle]) => {
    if (!handle) return;
    handle.addEventListener("mousedown", (e) => {
      e.preventDefault();
      e.stopPropagation();

      const startWidth = roomElement.offsetWidth;
      const startHeight = roomElement.offsetHeight;
      const startLeft = roomElement.offsetLeft;
      const startTop = roomElement.offsetTop;
      const startMouseX = e.clientX;
      const startMouseY = e.clientY;

      function onResizeMouseMove(moveEvent) {
        const dx = (moveEvent.clientX - startMouseX) / zoomLevel;
        const dy = (moveEvent.clientY - startMouseY) / zoomLevel;

        let newW = startWidth;
        let newH = startHeight;
        let newL = startLeft;
        let newT = startTop;

        if (dir === "r" || dir === "se") {
          newW = startWidth + dx;
          if (gridSnapSize > 1) newW = Math.round(newW / gridSnapSize) * gridSnapSize;
          newW = Math.max(150, Math.min(newW, deviceGrid.clientWidth - startLeft));
        }
        if (dir === "b" || dir === "se") {
          newH = startHeight + dy;
          if (gridSnapSize > 1) newH = Math.round(newH / gridSnapSize) * gridSnapSize;
          newH = Math.max(100, Math.min(newH, deviceGrid.clientHeight - startTop));
        }
        if (dir === "l") {
          newW = startWidth - dx;
          if (gridSnapSize > 1) newW = Math.round(newW / gridSnapSize) * gridSnapSize;
          if (newW >= 150) {
            newL = startLeft + (startWidth - newW);
          } else {
            newW = 150;
            newL = startLeft + (startWidth - 150);
          }
          newL = Math.max(0, newL);
        }
        if (dir === "t") {
          newH = startHeight - dy;
          if (gridSnapSize > 1) newH = Math.round(newH / gridSnapSize) * gridSnapSize;
          if (newH >= 100) {
            newT = startTop + (startHeight - newH);
          } else {
            newH = 100;
            newT = startTop + (startHeight - 100);
          }
          newT = Math.max(0, newT);
        }

        roomElement.style.width = newW + "px";
        roomElement.style.height = newH + "px";
        roomElement.style.left = newL + "px";
        roomElement.style.top = newT + "px";

        checkRegionContainment();
      }

      function onResizeMouseUp() {
        const id = roomElement.dataset.id;
        const room = currentLayout.rooms.find(r => r.id === id);
        if (room) {
          room.w = roomElement.offsetWidth;
          room.h = roomElement.offsetHeight;
          room.x = roomElement.offsetLeft;
          room.y = roomElement.offsetTop;
        }
        document.removeEventListener("mousemove", onResizeMouseMove);
        document.removeEventListener("mouseup", onResizeMouseUp);
      }

      document.addEventListener("mousemove", onResizeMouseMove);
      document.addEventListener("mouseup", onResizeMouseUp);
    });
  });
}

// Room alarm containments checking and count updates
function checkRegionContainment() {
  const rooms = document.querySelectorAll(".room-box");
  const cards = document.querySelectorAll(".device-grid .device-card");

  const roomCounts = {};
  const roomAlarms = {};

  currentLayout.rooms.forEach(r => {
    roomCounts[r.id] = 0;
    roomAlarms[r.id] = false;
  });

  cards.forEach(card => {
    const cardId = card.dataset.id;
    const cardX = parseInt(card.style.left) || 0;
    const cardY = parseInt(card.style.top) || 0;
    const cardW = card.offsetWidth || 135;
    const cardH = card.offsetHeight || 155;
    const cx = cardX + cardW / 2;
    const cy = cardY + cardH / 2;

    let containerRoomId = null;
    currentLayout.rooms.forEach(room => {
      const rx = room.x;
      const ry = room.y;
      const rw = room.w;
      const rh = room.h;

      if (cx >= rx && cx <= rx + rw && cy >= ry && cy <= ry + rh) {
        containerRoomId = room.id;
      }
    });

    if (containerRoomId) {
      roomCounts[containerRoomId]++;
      if (card.classList.contains("status-1")) {
        roomAlarms[containerRoomId] = true;
      }
    }
  });

  // Apply to UI
  currentLayout.rooms.forEach(r => {
    const badge = document.getElementById(`room-badge-${r.id}`);
    if (badge) {
      badge.textContent = roomCounts[r.id];
    }
    const box = document.getElementById(`room-box-${r.id}`);
    if (box) {
      if (roomAlarms[r.id]) {
        box.classList.add("alarm-active");
      } else {
        box.classList.remove("alarm-active");
      }
    }
  });
}

function checkDockVisibility() {
  const hasUnplaced = unplacedContainer.childElementCount > 0;
  if (hasUnplaced && (isEditMode || Object.keys(currentLayout.devices).length > 0)) {
    unplacedDock.style.display = "flex";
  } else {
    unplacedDock.style.display = "none";
  }
}

// Helper: Append formatted log message
function appendLog(content, type = "system") {
  const timeStr = new Date().toLocaleTimeString();
  const logItem = document.createElement("div");
  logItem.className = `log-item ${type}`;
  logItem.innerHTML = `
    <span class="log-time">[${timeStr}]</span>
    <span class="log-content">${content}</span>
  `;

  if (type === "raw") {
    if (logListRaw) {
      logListRaw.appendChild(logItem);
      logListRaw.scrollTop = logListRaw.scrollHeight;
      while (logListRaw.childElementCount > 200) {
        logListRaw.removeChild(logListRaw.firstChild);
      }
    }
  } else {
    if (logListAlarm) {
      logListAlarm.appendChild(logItem);
      logListAlarm.scrollTop = logListAlarm.scrollHeight;
      while (logListAlarm.childElementCount > 200) {
        logListAlarm.removeChild(logListAlarm.firstChild);
      }
    }
  }
}

// ===== 重设计的 SVG 图标 =====

// 烟温探测器 — 逼真形象 3D 智能烟感
function getDetectorSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <!-- Spherical 3D light gradient -->
        <radialGradient id="det-grad" cx="50%" cy="30%" r="50%">
          <stop offset="0%" stop-color="#ffffff"/>
          <stop offset="50%" stop-color="#cbd5e1"/>
          <stop offset="90%" stop-color="#64748b"/>
          <stop offset="100%" stop-color="#475569"/>
        </radialGradient>
        <!-- Glow/Heat gradient -->
        <radialGradient id="glow-grad" cx="50%" cy="50%" r="50%">
          <stop offset="0%" stop-color="#ef4444" stop-opacity="0.85"/>
          <stop offset="100%" stop-color="#ef4444" stop-opacity="0"/>
        </radialGradient>
        <!-- Alarm Glow filter -->
        <filter id="det-alarm-glow" x="-20%" y="-20%" width="140%" height="140%">
          <feGaussianBlur stdDeviation="1.5" result="blur"/>
          <feMerge>
            <feMergeNode in="blur"/>
            <feMergeNode in="SourceGraphic"/>
          </feMerge>
        </filter>
      </defs>
      <!-- Base ring shadow -->
      <circle cx="24" cy="24" r="22" fill="#0f172a" opacity="0.6"/>
      <!-- Outer Base Ring -->
      <circle cx="24" cy="24" r="20" fill="#f8fafc" stroke="#94a3b8" stroke-width="1"/>
      
      <!-- Chamber heat/fire glow (active on alarm) -->
      <circle class="svg-detector-chamber-glow" cx="24" cy="24" r="14" fill="url(#glow-grad)" opacity="0"/>
      
      <!-- Peripheral smoke slots (curved vents) -->
      <path d="M 14 14 A 14 14 0 0 1 34 14" stroke="#1e293b" stroke-width="2.5" stroke-linecap="round"/>
      <path d="M 38 24 A 14 14 0 0 1 34 34" stroke="#1e293b" stroke-width="2.5" stroke-linecap="round"/>
      <path d="M 14 34 A 14 14 0 0 1 10 24" stroke="#1e293b" stroke-width="2.5" stroke-linecap="round"/>

      <!-- Rising Heat/Smoke Wave lines (active on alarm) -->
      <path class="svg-smoke-wave svg-smoke-wave1" d="M 18 14 Q 24 6 30 14" stroke="#ef4444" stroke-width="1.2" stroke-linecap="round" fill="none" opacity="0"/>
      <path class="svg-smoke-wave svg-smoke-wave2" d="M 18 34 Q 24 42 30 34" stroke="#f59e0b" stroke-width="1.2" stroke-linecap="round" fill="none" opacity="0"/>

      <!-- Inner Chamber 3D Dome -->
      <circle cx="24" cy="24" r="12" fill="url(#det-grad)" stroke="#94a3b8" stroke-width="0.8"/>
      
      <!-- Inner circular grille -->
      <circle cx="24" cy="24" r="7" fill="#1e293b" stroke="#475569" stroke-width="0.8"/>
      
      <!-- Dual status indicator LEDs on the side -->
      <circle class="svg-led" cx="19" cy="24" r="1.5" fill="#10b981"/>
      <circle class="svg-led" cx="29" cy="24" r="1.5" fill="#10b981"/>
      
      <!-- Concentric light guide rings -->
      <circle cx="24" cy="24" r="3.5" stroke="#cbd5e1" stroke-width="0.5" opacity="0.3"/>
    </svg>
  `;
}

// 控制分配阀 / 泵 — 水平管道 + 旋转阀芯 + 水流粒子
function getValveSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <!-- 管道金属渐变 -->
        <linearGradient id="pipe-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#475569"/>
          <stop offset="30%" stop-color="#64748b"/>
          <stop offset="70%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#334155"/>
        </linearGradient>
        <!-- 阀体渐变 -->
        <radialGradient id="valve-grad" cx="50%" cy="40%" r="50%">
          <stop offset="0%" stop-color="#64748b"/>
          <stop offset="60%" stop-color="#334155"/>
          <stop offset="100%" stop-color="#1e293b"/>
        </radialGradient>
        <!-- 流动发光 -->
        <filter id="flow-glow">
          <feGaussianBlur stdDeviation="1.5" result="blur"/>
          <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
        </filter>
      </defs>
      <!-- 左管道 -->
      <rect x="0" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad)"/>
      <!-- 右管道 -->
      <rect x="36" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad)"/>
      <!-- 水流（双向流动：开启时绿色/青色+动画虚线流动） -->
      <line class="svg-flow-path svg-flow-forward" x1="2" y1="22.5" x2="46" y2="22.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <line class="svg-flow-path svg-flow-backward" x1="2" y1="25.5" x2="46" y2="25.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <!-- 流动粒子 (开启时可见) -->
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd1" cx="8" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd2" cx="24" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd1" cx="40" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd2" cx="24" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <!-- 左法兰 -->
      <rect x="12" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- 右法兰 -->
      <rect x="33" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- 阀体圆形 -->
      <circle cx="24" cy="24" r="9" fill="url(#valve-grad)" stroke="#64748b" stroke-width="1.5"/>
      <circle cx="24" cy="24" r="7" fill="none" stroke="#475569" stroke-width="0.5" opacity="0.5"/>
      <!-- 阀芯/蝶板 — 关闭时垂直(黄色)，开启时水平(绿色) -->
      <rect class="svg-wheel" x="22.5" y="18" width="3" height="12" rx="1.5" fill="#f59e0b" stroke="#d97706" stroke-width="0.5"/>
      <!-- 阀芯中心轴 -->
      <circle cx="24" cy="24" r="2.5" fill="#cbd5e1" stroke="#94a3b8" stroke-width="0.5"/>
      <!-- 顶部手轮 -->
      <rect x="22" y="13" width="4" height="3" rx="0.5" fill="#ef4444"/>
      <rect x="20" y="10" width="8" height="3" rx="1" fill="#64748b"/>
    </svg>
  `;
}

// ===== 新增设备类型 SVG 图标 =====

// 1301气体钢瓶 — 红色瓶体 + 压力表盘 + 泄漏时表针旋转
function getCylinderSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="cyl-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#dc2626"/>
          <stop offset="30%" stop-color="#ef4444"/>
          <stop offset="70%" stop-color="#b91c1c"/>
          <stop offset="100%" stop-color="#7f1d1d"/>
        </linearGradient>
        <linearGradient id="cyl-shine" x1="0" y1="0" x2="1" y2="0">
          <stop offset="0%" stop-color="rgba(255,255,255,0.15)"/>
          <stop offset="40%" stop-color="rgba(255,255,255,0.02)"/>
          <stop offset="100%" stop-color="rgba(255,255,255,0.0)"/>
        </linearGradient>
      </defs>
      
      <!-- Gas spraying particles (active only during status-1) -->
      <g class="svg-gas-spray" opacity="0">
        <circle class="svg-gas-particle svg-gas-p1" cx="24" cy="-2" r="1.5" fill="#f1f5f9"/>
        <circle class="svg-gas-particle svg-gas-p2" cx="20" cy="-6" r="2.0" fill="#cbd5e1"/>
        <circle class="svg-gas-particle svg-gas-p3" cx="28" cy="-6" r="1.8" fill="#e2e8f0"/>
        <path class="svg-gas-cloud" d="M 18 -4 Q 24 -14 30 -4" stroke="#cbd5e1" stroke-width="1.2" stroke-linecap="round" fill="none"/>
      </g>

      <!-- 1301 标签 -->
      <rect x="20" y="12" width="8" height="4" rx="1" fill="#1e293b"/>
      <text x="24" y="15" text-anchor="middle" font-family="sans-serif" font-size="3" font-weight="bold" fill="#fbbf24">1301</text>
      <!-- 瓶体 -->
      <rect x="16" y="8" width="16" height="30" rx="8" fill="url(#cyl-grad)" stroke="#7f1d1d" stroke-width="1.5"/>
      <!-- 瓶体高光 -->
      <rect x="19" y="11" width="5" height="24" rx="2" fill="url(#cyl-shine)"/>
      <!-- 瓶颈 -->
      <rect x="19" y="3" width="10" height="7" rx="2" fill="#475569" stroke="#334155" stroke-width="1"/>
      <!-- 阀门 -->
      <rect x="18" y="0" width="12" height="4" rx="1" fill="#64748b"/>
      <circle cx="24" cy="2" r="2.5" fill="#94a3b8" stroke="#cbd5e1" stroke-width="0.5"/>
      <!-- 压力表盘 -->
      <circle cx="24" cy="27" r="6.5" fill="#0f172a" stroke="#475569" stroke-width="1.2"/>
      <circle cx="24" cy="27" r="5" fill="#1e293b"/>
      <!-- 表盘刻度 -->
      <line x1="24" y1="22" x2="24" y2="23.5" stroke="#ef4444" stroke-width="1"/>
      <line x1="20" y1="27" x2="21.5" y2="27" stroke="#10b981" stroke-width="0.8"/>
      <!-- 表针 -->
      <line class="svg-gauge-needle" x1="24" y1="27" x2="24" y2="22.5" stroke="#10b981" stroke-width="1.8" stroke-linecap="round"/>
      <!-- 表针中心 -->
      <circle cx="24" cy="27" r="1.5" fill="#cbd5e1"/>
      <!-- 标签 1301 -->
      <text x="24" y="31" text-anchor="middle" font-family="sans-serif" font-size="3.5" font-weight="bold" fill="#fbbf24">1301</text>
      <!-- 状态 LED -->
      <circle class="svg-led" cx="24" cy="42" r="2" fill="#10b981"/>
    </svg>
  `;
}

// 手动报警按钮 — 逼真消防报警按钮
function getManualCallPointSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="mcp-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#dc2626"/>
          <stop offset="50%" stop-color="#ef4444"/>
          <stop offset="100%" stop-color="#b91c1c"/>
        </linearGradient>
        <filter id="mcp-shadow">
          <feDropShadow dx="0" dy="1" stdDeviation="2" flood-color="#000" flood-opacity="0.4"/>
        </filter>
      </defs>
      <!-- 背板 -->
      <rect x="6" y="4" width="36" height="40" rx="8" fill="#1e293b" stroke="#475569" stroke-width="2.5"/>
      <!-- 背板面板纹理 -->
      <rect x="10" y="8" width="28" height="32" rx="6" fill="#0f172a" stroke="#334155" stroke-width="1.5"/>
      <!-- 按钮外圈(红色环) -->
      <circle cx="24" cy="24" r="12" fill="#7f1d1d" stroke="#991b1b" stroke-width="1"/>
      <!-- 按钮主体 -->
      <circle class="svg-mcp-button" cx="24" cy="24" r="10" fill="url(#mcp-grad)" stroke="#dc2626" stroke-width="1.5" filter="url(#mcp-shadow)"/>
      <!-- 按钮高光 -->
      <ellipse cx="21" cy="21" rx="4" ry="3" fill="rgba(255,255,255,0.2)"/>
      <!-- 按钮文字 -->
      <text x="24" y="25" text-anchor="middle" font-family="Arial, sans-serif" font-size="5.5" font-weight="bold" fill="#ffffff">FIRE</text>
      <text x="24" y="31" text-anchor="middle" font-family="Arial, sans-serif" font-size="3" font-weight="bold" fill="#fecaca">报警</text>
      <!-- 状态 LED -->
      <circle class="svg-led" cx="24" cy="42" r="2" fill="#10b981"/>
      <!-- 面板螺丝 -->
      <circle cx="12" cy="10" r="1.2" fill="#475569"/>
      <circle cx="36" cy="10" r="1.2" fill="#475569"/>
      <circle cx="12" cy="38" r="1.2" fill="#475569"/>
      <circle cx="36" cy="38" r="1.2" fill="#475569"/>
      <!-- 敲击提示符 -->
      <rect x="17" y="6" width="14" height="3" rx="1" fill="#475569"/>
    </svg>
  `;
}

// 移动喷枪 SVG
function getMobileSprayGunSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <!-- 喷枪主体 -->
      <rect x="8" y="16" width="20" height="10" rx="3" fill="#334155" stroke="#475569" stroke-width="1.5"/>
      <!-- 喷嘴 -->
      <rect x="28" y="18" width="10" height="6" rx="2" fill="#1e293b" stroke="#64748b" stroke-width="1"/>
      <rect x="38" y="19" width="6" height="4" rx="1" fill="#475569"/>
      <!-- 握把 -->
      <rect x="12" y="26" width="8" height="14" rx="2" fill="#1e293b" stroke="#475569" stroke-width="1"/>
      <rect x="14" y="28" width="4" height="10" rx="1" fill="#0f172a"/>
      <!-- 管路接口 -->
      <rect x="2" y="18" width="6" height="8" rx="2" fill="#334155"/>
      <circle cx="5" cy="22" r="2" fill="#1e293b"/>
      <!-- 喷射粒子 -->
      <circle class="svg-spray-dot svg-spray-dot1" cx="44" cy="20" r="1" fill="#00e676" opacity="0"/>
      <circle class="svg-spray-dot svg-spray-dot2" cx="46" cy="22" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-spray-dot svg-spray-dot3" cx="45" cy="24" r="0.8" fill="#34d399" opacity="0"/>
      <!-- 状态 LED -->
      <circle class="svg-led" cx="18" cy="8" r="2" fill="#10b981"/>
    </svg>
  `;
}

// 水泵 SVG
function getPumpSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="pump-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#1e293b"/>
          <stop offset="50%" stop-color="#334155"/>
          <stop offset="100%" stop-color="#0f172a"/>
        </linearGradient>
        <linearGradient id="impeller-metal" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0%" stop-color="#38bdf8"/>
          <stop offset="100%" stop-color="#0369a1"/>
        </linearGradient>
      </defs>
      <!-- Horizontal flow line (similar to valve) -->
      <line class="svg-flow-path" x1="2" y1="24" x2="46" y2="24" stroke="#475569" stroke-width="3" stroke-linecap="round"/>
      <!-- Flow particles -->
      <circle class="svg-flow-dot svg-flow-dot1" cx="8" cy="24" r="1.5" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot2" cx="40" cy="24" r="1.5" fill="#10b981" opacity="0"/>
      <!-- Pump outer ring casing -->
      <circle cx="24" cy="24" r="14" fill="url(#pump-grad)" stroke="#475569" stroke-width="1.8"/>
      <!-- Pump inner chamber -->
      <circle cx="24" cy="24" r="10" fill="#0f172a" stroke="#1e293b" stroke-width="1"/>
      <!-- Impeller (rotating group) -->
      <g class="svg-impeller">
        <circle cx="24" cy="24" r="3" fill="#e2e8f0"/>
        <!-- 3 curved blades -->
        <path d="M 24 24 C 24 20 28 17 26 12 C 22 14 22 20 24 24 Z" fill="url(#impeller-metal)"/>
        <path d="M 24 24 C 20.5 25.5 17 22.5 13.5 25.5 C 14.5 29.5 20 28 24 24 Z" fill="url(#impeller-metal)"/>
        <path d="M 24 24 C 23.5 28.5 27 29.5 28.5 34.5 C 31.5 31.5 28 26.5 24 24 Z" fill="url(#impeller-metal)"/>
      </g>
      <!-- Status LED -->
      <circle class="svg-led" cx="24" cy="41" r="2.2" fill="#64748b"/>
    </svg>
  `;
}

// 压力开关 SVG
function getPressureSwitchSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <linearGradient id="switch-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#1e293b"/>
        </linearGradient>
      </defs>
      <!-- Pressure inlet pipe -->
      <rect x="22" y="32" width="4" height="10" fill="#64748b"/>
      <line x1="21" y1="38" x2="27" y2="38" stroke="#475569" stroke-width="1.5"/>
      <!-- Sensor box main frame -->
      <rect x="10" y="6" width="28" height="28" rx="4" fill="url(#switch-grad)" stroke="#475569" stroke-width="1.8"/>
      <!-- Dial inner display -->
      <circle cx="24" cy="20" r="9" fill="#0f172a" stroke="#334155" stroke-width="1"/>
      <!-- Dial tick marks -->
      <line x1="17" y1="20" x2="19" y2="20" stroke="#475569" stroke-width="0.8"/>
      <line x1="31" y1="20" x2="29" y2="20" stroke="#475569" stroke-width="0.8"/>
      <line x1="24" y1="13" x2="24" y2="15" stroke="#ef4444" stroke-width="0.8"/>
      <!-- Needle pointer -->
      <line class="svg-gauge-needle" x1="24" y1="20" x2="24" y2="14" stroke="#00f0ff" stroke-width="1.8" stroke-linecap="round"/>
      <circle cx="24" cy="20" r="2.2" fill="#cbd5e1"/>
      <!-- Status indicator LED -->
      <circle class="svg-led" cx="24" cy="30" r="1.8" fill="#64748b"/>
    </svg>
  `;
}

// 区域阀 SVG
function getZoneValveSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <!-- 管道金属渐变 -->
        <linearGradient id="pipe-grad-zone" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#475569"/>
          <stop offset="30%" stop-color="#64748b"/>
          <stop offset="70%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#334155"/>
        </linearGradient>
        <linearGradient id="zone-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#1e293b"/>
        </linearGradient>
        <!-- 流动发光 -->
        <filter id="flow-glow">
          <feGaussianBlur stdDeviation="1.5" result="blur"/>
          <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
        </filter>
      </defs>
      <!-- 左管道 -->
      <rect x="0" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad-zone)"/>
      <!-- 右管道 -->
      <rect x="36" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad-zone)"/>
      <!-- 水流管道路径 (双向流动 / 回流) -->
      <line class="svg-flow-path svg-flow-forward" x1="2" y1="22.5" x2="46" y2="22.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <line class="svg-flow-path svg-flow-backward" x1="2" y1="25.5" x2="46" y2="25.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <!-- 流动水粒子 -->
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd1" cx="8" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd2" cx="24" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd1" cx="40" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd2" cx="24" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <!-- 左法兰 -->
      <rect x="12" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- 右法兰 -->
      <rect x="33" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- Valve rectangular base shell -->
      <rect x="15" y="15" width="18" height="18" rx="2" fill="url(#zone-grad)" stroke="#475569" stroke-width="1.5"/>
      <!-- Handwheel -->
      <ellipse cx="24" cy="11" rx="5" ry="2" fill="none" stroke="#94a3b8" stroke-width="1"/>
      <line x1="24" y1="13" x2="24" y2="15" stroke="#94a3b8" stroke-width="1"/>
      <!-- Gate slider -->
      <rect class="svg-wheel" x="22.5" y="16" width="3" height="16" rx="1.5" fill="#f59e0b" stroke="#d97706" stroke-width="0.5"/>
      <!-- Status LED -->
      <circle class="svg-led" cx="24" cy="41" r="2.2" fill="#64748b"/>
    </svg>
  `;
}

// 分配阀 SVG
function getSelectorValveSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <!-- 管道金属渐变 -->
        <linearGradient id="pipe-grad-sel" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#475569"/>
          <stop offset="30%" stop-color="#64748b"/>
          <stop offset="70%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#334155"/>
        </linearGradient>
        <linearGradient id="sel-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#334155"/>
          <stop offset="100%" stop-color="#1e293b"/>
        </linearGradient>
        <!-- 流动发光 -->
        <filter id="flow-glow">
          <feGaussianBlur stdDeviation="1.5" result="blur"/>
          <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
        </filter>
      </defs>
      <!-- 左管道 -->
      <rect x="0" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad-sel)"/>
      <!-- 右管道 -->
      <rect x="36" y="21" width="12" height="6" rx="2" fill="url(#pipe-grad-sel)"/>
      <!-- 水流管道路径 (双向流动 / 回流) -->
      <line class="svg-flow-path svg-flow-forward" x1="2" y1="22.5" x2="46" y2="22.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <line class="svg-flow-path svg-flow-backward" x1="2" y1="25.5" x2="46" y2="25.5" stroke="#475569" stroke-width="2.2" stroke-linecap="round"/>
      <!-- 流动水粒子 -->
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd1" cx="8" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd2" cx="24" cy="22.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd1" cx="40" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd2" cx="24" cy="25.5" r="1.2" fill="#10b981" opacity="0"/>
      <!-- 左法兰 -->
      <rect x="12" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- 右法兰 -->
      <rect x="33" y="18" width="3" height="12" rx="1" fill="#64748b" stroke="#475569" stroke-width="0.5"/>
      <!-- Valve circular casing -->
      <circle cx="24" cy="24" r="10" fill="url(#sel-grad)" stroke="#475569" stroke-width="1.5"/>
      <!-- Rotating handle -->
      <rect class="svg-wheel" x="22.5" y="16" width="3" height="16" rx="1.5" fill="#f59e0b" stroke="#d97706" stroke-width="0.5"/>
      <!-- Status LED -->
      <circle class="svg-led" cx="24" cy="41" r="2.2" fill="#64748b"/>
    </svg>
  `;
}

// 总管隔离阀 SVG
function getMainIsolationValveSvg() {
  return `
    <svg viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <!-- 管道金属渐变 (较粗) -->
        <linearGradient id="pipe-grad-main" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#334155"/>
          <stop offset="30%" stop-color="#475569"/>
          <stop offset="70%" stop-color="#334155"/>
          <stop offset="100%" stop-color="#1e293b"/>
        </linearGradient>
        <linearGradient id="main-grad" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#1e293b"/>
          <stop offset="50%" stop-color="#475569"/>
          <stop offset="100%" stop-color="#0f172a"/>
        </linearGradient>
        <!-- 流动发光 -->
        <filter id="flow-glow">
          <feGaussianBlur stdDeviation="1.5" result="blur"/>
          <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
        </filter>
      </defs>
      <!-- 左管道 (较粗) -->
      <rect x="0" y="20" width="12" height="8" rx="2" fill="url(#pipe-grad-main)"/>
      <!-- 右管道 (较粗) -->
      <rect x="36" y="20" width="12" height="8" rx="2" fill="url(#pipe-grad-main)"/>
      <!-- Heavy pipe flow path (双向流动 / 回流) -->
      <line class="svg-flow-path svg-flow-forward" x1="2" y1="22.0" x2="46" y2="22.0" stroke="#475569" stroke-width="3" stroke-linecap="square"/>
      <line class="svg-flow-path svg-flow-backward" x1="2" y1="26.0" x2="46" y2="26.0" stroke="#475569" stroke-width="3" stroke-linecap="square"/>
      <!-- Double flanges on sides -->
      <rect x="10" y="16" width="3" height="16" rx="0.5" fill="#64748b"/>
      <rect x="35" y="16" width="3" height="16" rx="0.5" fill="#64748b"/>
      <!-- Flow particles -->
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd1" cx="8" cy="22.0" r="1.5" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-fwd svg-flow-dot-fwd2" cx="24" cy="22.0" r="1.5" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd1" cx="40" cy="26.0" r="1.5" fill="#10b981" opacity="0"/>
      <circle class="svg-flow-dot svg-flow-dot-bwd svg-flow-dot-bwd2" cx="24" cy="26.0" r="1.5" fill="#10b981" opacity="0"/>
      <!-- Large valve body -->
      <circle cx="24" cy="24" r="11" fill="url(#main-grad)" stroke="#475569" stroke-width="1.8"/>
      <!-- Massive handwheel -->
      <ellipse cx="24" cy="10" rx="7" ry="2.5" fill="none" stroke="#94a3b8" stroke-width="1.5"/>
      <line x1="24" y1="12.5" x2="24" y2="16" stroke="#94a3b8" stroke-width="1.5"/>
      <!-- Heavy slider -->
      <rect class="svg-wheel" x="22" y="15" width="4" height="18" rx="2" fill="#f59e0b" stroke="#d97706" stroke-width="0.8"/>
      <!-- Status LED -->
      <circle class="svg-led" cx="24" cy="41" r="2.2" fill="#64748b"/>
    </svg>
  `;
}

// 统一 SVG 获取函数（deviceType 对齐 Qt 后端）
function getDeviceSvg(deviceType) {
  switch (deviceType) {
    case "detector": return getDetectorSvg();
    case "valve": return getValveSvg();
    case "valve_distributor":
    case "selector_valve": return getSelectorValveSvg();
    case "valve_zone":
    case "zone_valve": return getZoneValveSvg();
    case "valve_main_isolation":
    case "main_isolation_valve": return getMainIsolationValveSvg();
    case "manual_alarm": return getManualCallPointSvg();
    case "gas_cylinder": return getCylinderSvg();
    case "water_pump": return getPumpSvg();
    case "pressure_switch": return getPressureSwitchSvg();
    case "mobile_spray_gun": return getMobileSprayGunSvg();
    default: return getDetectorSvg();
  }
}

// 设备类型中文名映射（对齐 Qt 后端名称）
function getDeviceTypeName(deviceType) {
  const names = {
    detector: "烟温探测器",
    valve: "控制分配阀",
    valve_distributor: "分配阀",
    selector_valve: "分配阀",
    valve_zone: "区域阀",
    zone_valve: "区域阀",
    valve_main_isolation: "总管隔离阀",
    main_isolation_valve: "总管隔离阀",
    manual_alarm: "手动报警按钮",
    gas_cylinder: "1301气体钢瓶",
    water_pump: "水泵",
    pressure_switch: "压力开关",
    mobile_spray_gun: "移动喷枪"
  };
  return names[deviceType] || "未知设备";
}

function getStatusText(deviceType, status) {
  const map = {
    detector: { 0: "NORMAL", 1: "ALARM" },
    valve: { 0: "CLOSED", 1: "OPEN" },
    valve_distributor: { 0: "CLOSED", 1: "OPEN" },
    selector_valve: { 0: "CLOSED", 1: "OPEN" },
    valve_zone: { 0: "CLOSED", 1: "OPEN" },
    zone_valve: { 0: "CLOSED", 1: "OPEN" },
    valve_main_isolation: { 0: "CLOSED", 1: "OPEN" },
    main_isolation_valve: { 0: "CLOSED", 1: "OPEN" },
    manual_alarm: { 0: "NORMAL", 1: "ALARM" },
    gas_cylinder: { 0: "NORMAL", 1: "LEAK" },
    water_pump: { 0: "STOPPED", 1: "RUNNING" },
    pressure_switch: { 0: "CLOSED", 1: "OPEN" },
    mobile_spray_gun: { 0: "STOPPED", 1: "SPRAYING" }
  };
  return (map[deviceType] || map["detector"])[status ? 1 : 0];
}

// Mode Selector Listeners
btnMonitorMode.addEventListener("click", () => {
  if (!isEditMode) return;
  isEditMode = false;

  btnMonitorMode.classList.add("active");
  btnLayoutMode.classList.remove("active");
  layoutToolbar.style.display = "none";
  document.body.classList.remove("edit-mode");

  loadLayoutFromStorage();
  renderWorkspace();
});

btnLayoutMode.addEventListener("click", () => {
  if (isEditMode) return;
  isEditMode = true;

  btnLayoutMode.classList.add("active");
  btnMonitorMode.classList.remove("active");
  layoutToolbar.style.display = "flex";
  document.body.classList.add("edit-mode");

  renderWorkspace();
});

// Information Log Drawer Toggles
btnToggleInfo.addEventListener("click", () => {
  infoLogDrawer.classList.toggle("open");
  btnToggleInfo.classList.toggle("active");
});

btnCloseDrawer.addEventListener("click", () => {
  infoLogDrawer.classList.remove("open");
  btnToggleInfo.classList.remove("active");
});

if (btnClearDrawerLogs) {
  btnClearDrawerLogs.addEventListener("click", () => {
    if (logListRaw) logListRaw.innerHTML = "";
    appendLog("信息日志已清空。", "raw");
  });
}

// Alarm Panel Toggles & Drag-to-Resize
btnToggleAlarm.addEventListener("click", () => {
  if (appFooter.style.display === "none") {
    appFooter.style.display = "flex";
    btnToggleAlarm.classList.add("active");
  } else {
    appFooter.style.display = "none";
    btnToggleAlarm.classList.remove("active");
  }
});

btnCloseFooter.addEventListener("click", () => {
  appFooter.style.display = "none";
  btnToggleAlarm.classList.remove("active");
});

let isResizing = false;
let startY = 0;
let startHeight = 0;

resizeHandle.addEventListener("mousedown", (e) => {
  isResizing = true;
  startY = e.clientY;
  startHeight = appFooter.offsetHeight;
  document.addEventListener("mousemove", onFooterMouseMove);
  document.addEventListener("mouseup", onFooterMouseUp);
  e.preventDefault();
});

function onFooterMouseMove(e) {
  if (!isResizing) return;
  const dy = startY - e.clientY;
  let newHeight = startHeight + dy;
  if (newHeight < 80) newHeight = 80;
  if (newHeight > 450) newHeight = 450;
  appFooter.style.height = newHeight + "px";
}

function onFooterMouseUp() {
  isResizing = false;
  document.removeEventListener("mousemove", onFooterMouseMove);
  document.removeEventListener("mouseup", onFooterMouseUp);
}

// Custom Modal for Room Creation
btnAddRegion.addEventListener("click", () => {
  modalAddRoom.classList.add("show");
  inputRoomName.value = `房间 ${currentLayout.rooms.length + 1}`;
  inputRoomName.focus();
  inputRoomName.select();
});

function hideAddRoomModal() {
  modalAddRoom.classList.remove("show");
}

btnModalCancel.addEventListener("click", hideAddRoomModal);
btnModalCancelX.addEventListener("click", hideAddRoomModal);

btnModalConfirm.addEventListener("click", () => {
  const name = inputRoomName.value;
  if (!name || name.trim() === "") {
    alert("请输入有效的房间名称！");
    return;
  }
  const newRoom = {
    id: "room_" + Date.now(),
    name: name.trim(),
    x: 100,
    y: 100,
    w: 260,
    h: 200
  };
  currentLayout.rooms.push(newRoom);
  const el = createRoomElement(newRoom);
  deviceGrid.appendChild(el);
  checkRegionContainment();
  hideAddRoomModal();
  appendLog(`✓ 成功创建房间: "${name.trim()}"`, "system");
});

inputRoomName.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    btnModalConfirm.click();
  }
});

// Custom Modal for Room Renaming
let activeRenameRoom = null;
let activeRenameTitleEl = null;

function showRenameModal(room, titleEl) {
  activeRenameRoom = room;
  activeRenameTitleEl = titleEl;
  inputRenameRoomName.value = room.name;
  modalRenameRoom.classList.add("show");
  setTimeout(() => {
    inputRenameRoomName.focus();
    inputRenameRoomName.select();
  }, 100);
}

function hideRenameModal() {
  modalRenameRoom.classList.remove("show");
  activeRenameRoom = null;
  activeRenameTitleEl = null;
}

btnRenameCancel.addEventListener("click", hideRenameModal);
btnRenameCancelX.addEventListener("click", hideRenameModal);

btnRenameConfirm.addEventListener("click", () => {
  if (!activeRenameRoom || !activeRenameTitleEl) return;
  const newName = inputRenameRoomName.value.trim();
  if (newName === "") {
    alert("请输入有效的房间名称！");
    return;
  }
  const oldName = activeRenameRoom.name;
  if (newName !== oldName) {
    activeRenameRoom.name = newName;
    activeRenameTitleEl.textContent = newName;
    appendLog(`✓ 房间 "${oldName}" 重命名为: "${newName}"`, "system");
    saveLayoutsToLocalStorage();
  }
  hideRenameModal();
});

inputRenameRoomName.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    btnRenameConfirm.click();
  } else if (e.key === "Escape") {
    hideRenameModal();
  }
});

// Load Custom Named Slot
selectLayoutSlot.addEventListener("change", () => {
  const slotName = selectLayoutSlot.value;
  if (!slotName) return;

  const saved = localStorage.getItem(`smile_code_layout_slot_${slotName}`);
  if (saved) {
    try {
      currentLayout = JSON.parse(saved);
      if (!currentLayout.rooms) currentLayout.rooms = currentLayout.regions || [];
      if (!currentLayout.devices) currentLayout.devices = {};
      if (!currentLayout.backgroundTemplate) currentLayout.backgroundTemplate = "";

      renderWorkspace();
      appendLog(`✓ 已成功载入已存布局: "${slotName}"`, "system");
    } catch (e) {
      console.error(e);
      appendLog(`❌ 载入已存布局 "${slotName}" 失败`, "warning");
    }
  }

  selectLayoutSlot.selectedIndex = 0;
});

// Export Layout as file
btnExportFile.addEventListener("click", () => {
  const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(currentLayout, null, 2));
  const downloadAnchor = document.createElement("a");
  downloadAnchor.setAttribute("href", dataStr);
  downloadAnchor.setAttribute("download", `smile_layout_${Date.now()}.json`);
  document.body.appendChild(downloadAnchor);
  downloadAnchor.click();
  downloadAnchor.remove();
  appendLog(`✓ 布局配置已成功导出为 file`, "system");
});

// Import Layout from file
btnImportFile.addEventListener("click", () => {
  if (fileImportInput) {
    fileImportInput.click();
  }
});

if (fileImportInput) {
  fileImportInput.addEventListener("change", (event) => {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      try {
        const layout = JSON.parse(e.target.result);
        if (layout && (layout.rooms || layout.regions)) {
          currentLayout = layout;
          if (!currentLayout.rooms) currentLayout.rooms = currentLayout.regions || [];
          if (!currentLayout.devices) currentLayout.devices = {};
          if (!currentLayout.backgroundTemplate) currentLayout.backgroundTemplate = "";

          renderWorkspace();
          appendLog(`✓ 成功从文件导入布局，当前处于编辑状态，请点击保存当前。`, "system");
        } else {
          alert("无效的布局文件格式！");
        }
      } catch (err) {
        console.error(err);
        alert("读取或解析布局文件失败！");
      }
    };
    reader.readAsText(file);
    fileImportInput.value = "";
  });
}

// Template loading trigger
selectTemplate.addEventListener("change", () => {
  const templateName = selectTemplate.value;
  if (!templateName) return;

  if (confirm(`确定要加载所选的布局模版吗？这将覆盖您当前的房间分布和坐标。`)) {
    let preset = null;
    if (templateName === "submarine") {
      preset = JSON.parse(JSON.stringify(submarineTemplate));
      latestMappings.forEach((device, index) => {
        if (index === 0) preset.devices[device.deviceId] = { x: preset.rooms[0].x + 22, y: preset.rooms[0].y + 55 };
        else if (index === 1) preset.devices[device.deviceId] = { x: preset.rooms[1].x + 42, y: preset.rooms[1].y + 75 };
        else if (index === 2) preset.devices[device.deviceId] = { x: preset.rooms[2].x + 22, y: preset.rooms[2].y + 55 };
        else if (index === 3) preset.devices[device.deviceId] = { x: preset.rooms[3].x + 22, y: preset.rooms[3].y + 65 };
        else if (index === 4) preset.devices[device.deviceId] = { x: preset.rooms[4].x + 22, y: preset.rooms[4].y + 55 };
      });
    } else if (templateName === "building") {
      preset = JSON.parse(JSON.stringify(buildingTemplate));
      latestMappings.forEach((device, index) => {
        const floor = index % 3; // 3F, 2F, 1F
        const col = Math.floor(index / 3);
        if (col < 4) {
          preset.devices[device.deviceId] = {
            x: preset.rooms[floor].x + 30 + col * 140,
            y: preset.rooms[floor].y + 12
          };
        }
      });
    } else if (templateName === "warship") {
      preset = JSON.parse(JSON.stringify(warshipTemplate));
      latestMappings.forEach((device, index) => {
        const roomIdx = index % 4;
        const col = Math.floor(index / 4);
        if (col === 0) {
          preset.devices[device.deviceId] = {
            x: preset.rooms[roomIdx].x + 30,
            y: preset.rooms[roomIdx].y + 20
          };
        }
      });
    }

    if (preset) {
      currentLayout = preset;
      renderWorkspace();
      appendLog(`✓ 成功载入模版，建议在此基础上进行手动拖拽划分，完成后点击“保存当前”。`, "system");
    }
  }

  selectTemplate.selectedIndex = 0;
});

// Save current layout
btnSaveLayout.addEventListener("click", () => {
  localStorage.setItem("smile_code_layout_v1", JSON.stringify(currentLayout));
  appendLog("✓ 布局和房间划分已成功保存到默认配置！", "system");

  isEditMode = false;
  btnMonitorMode.classList.add("active");
  btnLayoutMode.classList.remove("active");
  layoutToolbar.style.display = "none";
  document.body.classList.remove("edit-mode");
  renderWorkspace();
});

// Reset current layout
btnResetLayout.addEventListener("click", () => {
  if (confirm("确定要重置当前布局吗？这将会删除所有的自定义分区和坐标摆放。")) {
    localStorage.removeItem("smile_code_layout_v1");
    currentLayout = { backgroundTemplate: "", rooms: [], devices: {} };
    isEditMode = false;
    btnMonitorMode.classList.add("active");
    btnLayoutMode.classList.remove("active");
    layoutToolbar.style.display = "none";
    document.body.classList.remove("edit-mode");
    renderWorkspace();
    appendLog("布局已重置为默认网格摆放。", "system");
  }
});

// Grid Snap Selector Event Listener
const selectGridSnap = document.getElementById("select-grid-snap");
if (selectGridSnap) {
  selectGridSnap.addEventListener("change", () => {
    gridSnapSize = parseInt(selectGridSnap.value) || 10;
    appendLog(`网格吸附大小调整为: ${gridSnapSize} px`, "system");
    // Dynamically adjust the background grid style to match
    if (gridSnapSize > 1) {
      deviceGrid.style.backgroundSize = `${gridSnapSize * 2}px ${gridSnapSize * 2}px`;
      deviceGrid.style.backgroundImage = `radial-gradient(var(--primary-glow-light) 1.5px, transparent 1.5px)`;
    } else {
      deviceGrid.style.backgroundImage = "none";
    }
  });
}

// Theme Skin Dropdown Event Listener
const selectThemeSkin = document.getElementById("select-theme-skin");

function applyTheme(theme) {
  // Remove all theme- classes from body
  document.body.className.split(" ").forEach(className => {
    if (className.startsWith("theme-")) {
      document.body.classList.remove(className);
    }
  });
  document.body.classList.add(`theme-${theme}`);
  localStorage.setItem("smile_code_theme", theme);
  if (selectThemeSkin) {
    selectThemeSkin.value = theme;
  }
}

if (selectThemeSkin) {
  selectThemeSkin.addEventListener("change", () => {
    applyTheme(selectThemeSkin.value);
    appendLog(`系统配色主题已切换。`, "system");
  });
}

// Clear alarm log list
if (btnClearLogs) {
  btnClearLogs.addEventListener("click", () => {
    if (logListAlarm) logListAlarm.innerHTML = "";
    appendLog("报警日志已清空。", "system");
  });
}

// Canvas Zoom Controller (Buttons + Ctrl + Wheel)
const btnZoomOut = document.getElementById("btn-zoom-out");
const zoomText = document.getElementById("zoom-text");
const btnZoomIn = document.getElementById("btn-zoom-in");
const btnZoomReset = document.getElementById("btn-zoom-reset");

let zoomLevel = 1.0;
const ZOOM_MIN = 0.4;
const ZOOM_MAX = 2.0;

function setZoom(level) {
  zoomLevel = Math.max(ZOOM_MIN, Math.min(ZOOM_MAX, parseFloat(level)));
  if (deviceGrid) {
    deviceGrid.style.zoom = zoomLevel;
  }
  if (zoomText) {
    zoomText.textContent = `${Math.round(zoomLevel * 100)}%`;
  }
  localStorage.setItem("canvas_zoom_level", zoomLevel);
}

if (btnZoomIn) {
  btnZoomIn.addEventListener("click", () => setZoom(zoomLevel + 0.1));
}
if (btnZoomOut) {
  btnZoomOut.addEventListener("click", () => setZoom(zoomLevel - 0.1));
}
if (btnZoomReset) {
  btnZoomReset.addEventListener("click", () => setZoom(1.0));
}

// Mouse Wheel zoom with Ctrl key (intercepted globally)
window.addEventListener("wheel", (e) => {
  if (e.ctrlKey) {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 0.05 : -0.05;
    setZoom(zoomLevel + delta);
  }
}, { passive: false });

// Run Init
const savedTheme = localStorage.getItem("smile_code_theme") || "cyber";
applyTheme(savedTheme);

const savedZoom = parseFloat(localStorage.getItem("canvas_zoom_level")) || 1.0;
setZoom(savedZoom);

loadLayoutFromStorage();
loadLayoutSlots();
connect();
