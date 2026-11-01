import { create } from 'zustand';
import type { DeviceMapping, ConnectionStatus } from '../types/device';

interface WebSocketState {
  connectionStatus: ConnectionStatus;
  latestMappings: DeviceMapping[];

  setConnectionStatus: (status: ConnectionStatus) => void;
  setMappings: (mappings: DeviceMapping[]) => void;
  updateDeviceStatus: (deviceId: number, status: boolean) => void;
}

export const useWebSocketStore = create<WebSocketState>((set) => ({
  connectionStatus: 'initializing',
  latestMappings: [],

  setConnectionStatus: (status) => set({ connectionStatus: status }),

  setMappings: (mappings) => set({ latestMappings: mappings }),

  updateDeviceStatus: (deviceId, status) =>
    set((state) => ({
      latestMappings: state.latestMappings.map((d) =>
        d.deviceId === deviceId ? { ...d, status } : d
      ),
    })),
}));
