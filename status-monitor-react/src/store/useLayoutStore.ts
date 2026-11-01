import { create } from 'zustand';
import type { Room, LayoutData, BackgroundTemplate, GridSnapSize } from '../types/layout';
import { STORAGE_KEYS } from '../types/layout';
import { DEFAULT_LAYOUT, DEFAULT_GRID_SNAP } from '../constants/defaults';
import { TEMPLATE_MAP } from '../constants/templates';
import { loadLayoutFromStorage, saveLayoutToStorage, getLayoutSlots, saveLayoutToSlot, loadLayoutFromSlot } from '../utils/storage-helpers';

interface LayoutState {
  isEditMode: boolean;
  gridSnapSize: GridSnapSize;
  backgroundTemplate: BackgroundTemplate;
  rooms: Room[];
  devicePositions: Record<number, { x: number; y: number }>;

  setEditMode: (mode: boolean) => void;
  setGridSnap: (size: GridSnapSize) => void;
  addRoom: (name: string) => Room;
  removeRoom: (id: string) => void;
  renameRoom: (id: string, name: string) => void;
  updateRoomGeometry: (id: string, partial: Partial<Room>) => void;
  setDevicePosition: (deviceId: number, x: number, y: number) => void;
  removeDevicePosition: (deviceId: number) => void;
  loadTemplate: (templateName: string, deviceIds: number[]) => void;
  loadFromStorage: () => void;
  saveToStorage: () => void;
  saveToSlot: (slotName: string) => void;
  loadFromSlot: (slotName: string) => boolean;
  getAllSlots: () => string[];
  exportToJSON: () => string;
  importFromJSON: (json: string) => boolean;
  resetLayout: () => void;
}

export const useLayoutStore = create<LayoutState>((set, get) => ({
  isEditMode: false,
  gridSnapSize: DEFAULT_GRID_SNAP,
  backgroundTemplate: '',
  rooms: [],
  devicePositions: {},

  setEditMode: (mode) => set({ isEditMode: mode }),

  setGridSnap: (size) => set({ gridSnapSize: size }),

  addRoom: (name) => {
    const room: Room = {
      id: 'room_' + Date.now(),
      name,
      x: 100,
      y: 100,
      w: 260,
      h: 200,
    };
    set((s) => ({ rooms: [...s.rooms, room] }));
    return room;
  },

  removeRoom: (id) =>
    set((s) => ({ rooms: s.rooms.filter((r) => r.id !== id) })),

  renameRoom: (id, name) =>
    set((s) => ({
      rooms: s.rooms.map((r) => (r.id === id ? { ...r, name } : r)),
    })),

  updateRoomGeometry: (id, partial) =>
    set((s) => ({
      rooms: s.rooms.map((r) => (r.id === id ? { ...r, ...partial } : r)),
    })),

  setDevicePosition: (deviceId, x, y) =>
    set((s) => ({
      devicePositions: { ...s.devicePositions, [deviceId]: { x, y } },
    })),

  removeDevicePosition: (deviceId) =>
    set((s) => {
      const { [deviceId]: _, ...rest } = s.devicePositions;
      return { devicePositions: rest };
    }),

  loadTemplate: (templateName, deviceIds) => {
    const preset = TEMPLATE_MAP[templateName];
    if (!preset) return;

    const layout = JSON.parse(JSON.stringify(preset)) as LayoutData;

    // 按模板自动分配设备位置
    if (templateName === 'submarine') {
      deviceIds.forEach((deviceId, index) => {
        if (index < layout.rooms.length) {
          layout.devices[deviceId] = {
            x: layout.rooms[index].x + 22,
            y: layout.rooms[index].y + 55,
          };
        }
      });
    } else if (templateName === 'building') {
      deviceIds.forEach((deviceId, index) => {
        const floor = index % 3;
        const col = Math.floor(index / 3);
        if (col < 4 && floor < layout.rooms.length) {
          layout.devices[deviceId] = {
            x: layout.rooms[floor].x + 30 + col * 140,
            y: layout.rooms[floor].y + 12,
          };
        }
      });
    } else if (templateName === 'warship') {
      deviceIds.forEach((deviceId, index) => {
        const roomIdx = index % 4;
        if (roomIdx < layout.rooms.length) {
          layout.devices[deviceId] = {
            x: layout.rooms[roomIdx].x + 30,
            y: layout.rooms[roomIdx].y + 20,
          };
        }
      });
    }

    set({
      backgroundTemplate: layout.backgroundTemplate,
      rooms: layout.rooms,
      devicePositions: layout.devices,
    });
  },

  loadFromStorage: () => {
    const layout = loadLayoutFromStorage();
    if (layout) {
      set({
        backgroundTemplate: layout.backgroundTemplate,
        rooms: layout.rooms,
        devicePositions: layout.devices,
      });
    }
  },

  saveToStorage: () => {
    const { backgroundTemplate, rooms, devicePositions } = get();
    saveLayoutToStorage({ backgroundTemplate, rooms, devices: devicePositions });
  },

  saveToSlot: (slotName) => {
    const { backgroundTemplate, rooms, devicePositions } = get();
    saveLayoutToSlot(slotName, { backgroundTemplate, rooms, devices: devicePositions });
  },

  loadFromSlot: (slotName) => {
    const layout = loadLayoutFromSlot(slotName);
    if (layout) {
      set({
        backgroundTemplate: layout.backgroundTemplate,
        rooms: layout.rooms,
        devicePositions: layout.devices,
      });
      return true;
    }
    return false;
  },

  getAllSlots: () => getLayoutSlots(),

  exportToJSON: () => {
    const { backgroundTemplate, rooms, devicePositions } = get();
    return JSON.stringify({ backgroundTemplate, rooms, devices: devicePositions }, null, 2);
  },

  importFromJSON: (json) => {
    try {
      const data = JSON.parse(json);
      const rooms = data.rooms || data.regions || [];
      if (rooms.length === 0 && Object.keys(data.devices || {}).length === 0) {
        return false;
      }
      set({
        backgroundTemplate: data.backgroundTemplate || '',
        rooms,
        devicePositions: data.devices || {},
      });
      return true;
    } catch {
      return false;
    }
  },

  resetLayout: () => {
    localStorage.removeItem(STORAGE_KEYS.LAYOUT);
    set({
      backgroundTemplate: DEFAULT_LAYOUT.backgroundTemplate,
      rooms: DEFAULT_LAYOUT.rooms,
      devicePositions: DEFAULT_LAYOUT.devices,
      isEditMode: false,
    });
  },
}));
