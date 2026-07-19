import { create } from 'zustand';
import type { DeviceMapping, DeviceStateEntry, UpdateMessage } from '../types/device';

interface DeviceState {
  devices: Record<number, DeviceStateEntry>;

  initFromMappings: (mappings: DeviceMapping[]) => void;
  updateDevice: (msg: UpdateMessage) => void;
  getDeviceById: (id: number) => DeviceStateEntry | undefined;
}

export const useDeviceStore = create<DeviceState>((set, get) => ({
  devices: {},

  initFromMappings: (mappings) => {
    const devices: Record<number, DeviceStateEntry> = {};
    mappings.forEach((m) => {
      devices[m.deviceId] = {
        mapping: m,
        status: m.status,
        prevStatus: m.status,
        lastUpdate: null,
      };
    });
    set({ devices });
  },

  updateDevice: (msg) =>
    set((state) => {
      const existing = state.devices[msg.deviceId];
      if (!existing) return state;
      return {
        devices: {
          ...state.devices,
          [msg.deviceId]: {
            ...existing,
            status: msg.status,
            prevStatus: msg.prevStatus,
            lastUpdate: msg.timestamp,
          },
        },
      };
    }),

  getDeviceById: (id) => get().devices[id],
}));
