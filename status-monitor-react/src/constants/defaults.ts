import type { LayoutData, ThemeName, GridSnapSize } from '../types/layout';

/** 默认布局 */
export const DEFAULT_LAYOUT: LayoutData = {
  backgroundTemplate: '',
  rooms: [],
  devices: {},
};

/** 默认主题 */
export const DEFAULT_THEME: ThemeName = 'cyber';

/** 默认网格吸附 */
export const DEFAULT_GRID_SNAP: GridSnapSize = 10;

/** 主题选项 */
export const THEME_OPTIONS: { value: ThemeName; label: string }[] = [
  { value: 'cyber', label: '🌌 科技蓝' },
  { value: 'emerald', label: '🟢 翡翠绿' },
  { value: 'amber', label: '🟠 Amber 琥珀金' },
  { value: 'ruby', label: '🔴 烈焰红' },
];

/** 网格吸附选项 */
export const GRID_SNAP_OPTIONS: { value: GridSnapSize; label: string }[] = [
  { value: 1, label: '自由吸附' },
  { value: 5, label: '5 px' },
  { value: 10, label: '10 px' },
  { value: 20, label: '20 px' },
];

/** WebSocket 服务器 URL */
export const WS_URL = 'ws://localhost:12345';

/** 自动重连间隔（毫秒） */
export const RECONNECT_INTERVAL = 3000;

/** 日志最大条数 */
export const MAX_LOG_ENTRIES = 200;

/** 画布尺寸 */
export const CANVAS_WIDTH = 3000;
export const CANVAS_HEIGHT = 2000;

/** Footer 高度范围 */
export const FOOTER_MIN_HEIGHT = 80;
export const FOOTER_MAX_HEIGHT = 450;
export const FOOTER_DEFAULT_HEIGHT = 180;

/** 房间最小尺寸 */
export const ROOM_MIN_WIDTH = 150;
export const ROOM_MIN_HEIGHT = 100;
