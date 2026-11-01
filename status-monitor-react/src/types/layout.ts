/** 布局中的房间 */
export interface Room {
  id: string;
  name: string;
  x: number;
  y: number;
  w: number;
  h: number;
}

/** 布局背景类型 */
export type BackgroundTemplate = '' | 'submarine' | 'building' | 'warship';

/** 完整布局数据 */
export interface LayoutData {
  backgroundTemplate: BackgroundTemplate;
  rooms: Room[];
  devices: Record<number, { x: number; y: number }>;
}

/** 日志类型 */
export type LogType = 'system' | 'alarm' | 'recovery' | 'warning' | 'raw';

/** 日志条目 */
export interface LogEntry {
  id: string;
  timestamp: string;
  content: string;
  type: LogType;
}

/** 主题名称 */
export type ThemeName = 'cyber' | 'emerald' | 'amber' | 'ruby';

/** 网格吸附值 */
export type GridSnapSize = 1 | 5 | 10 | 20;

/** localStorage 键名常量 */
export const STORAGE_KEYS = {
  LAYOUT: 'smile_code_layout_v1',
  LAYOUT_SLOTS: 'smile_code_layout_slots',
  LAYOUT_SLOT_PREFIX: 'smile_code_layout_slot_',
  THEME: 'smile_code_theme',
} as const;
