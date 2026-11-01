/** 设备类型 */
export type DeviceType = 'detector' | 'valve';

/** Qt 服务端发送的设备映射项 */
export interface DeviceMapping {
  deviceId: number;
  label: string;
  deviceType: DeviceType;
  canId: number;
  byteIndex: number;
  bitIndex: number;
  status: boolean;
}

/** config 类型 WebSocket 消息 */
export interface ConfigMessage {
  type: 'config';
  mappings: DeviceMapping[];
}

/** update 类型 WebSocket 消息 */
export interface UpdateMessage {
  type: 'update';
  deviceId: number;
  status: boolean;
  prevStatus: boolean;
  label: string;
  deviceType: DeviceType;
  timestamp: string;
}

/** WebSocket 消息联合类型 */
export type WSMessage = ConfigMessage | UpdateMessage;

/** 设备状态入口（store 内部使用） */
export interface DeviceStateEntry {
  mapping: DeviceMapping;
  status: boolean;
  prevStatus: boolean;
  lastUpdate: string | null;
}

/** 连接状态 */
export type ConnectionStatus = 'initializing' | 'connected' | 'disconnected';
