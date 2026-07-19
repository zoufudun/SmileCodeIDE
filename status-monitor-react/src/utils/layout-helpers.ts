import type { Room } from '../types/layout';
import { CANVAS_WIDTH, CANVAS_HEIGHT } from '../constants/defaults';

/** 网格吸附：将坐标吸附到最近的网格点 */
export function snapToGrid(value: number, gridSize: number): number {
  if (gridSize <= 1) return value;
  return Math.round(value / gridSize) * gridSize;
}

/** 约束坐标在画布范围内 */
export function clampPosition(
  x: number,
  y: number,
  itemW: number,
  itemH: number
): { x: number; y: number } {
  return {
    x: Math.max(0, Math.min(x, CANVAS_WIDTH - itemW)),
    y: Math.max(0, Math.min(y, CANVAS_HEIGHT - itemH)),
  };
}

/** 检测卡片中心点是否在房间矩形内 */
export function isCardInRoom(
  cardX: number,
  cardY: number,
  cardW: number,
  cardH: number,
  room: Room
): boolean {
  const cx = cardX + cardW / 2;
  const cy = cardY + cardH / 2;
  return cx >= room.x && cx <= room.x + room.w && cy >= room.y && cy <= room.y + room.h;
}

/**
 * 计算每个房间内的设备数和报警状态
 * cards: Array<{ x: number, y: number, w: number, h: number, isAlarming: boolean }>
 * rooms: Room[]
 * 返回: Map<roomId, { count: number, hasAlarm: boolean }>
 */
export function computeRoomContainment(
  cards: Array<{ x: number; y: number; w: number; h: number; isAlarming: boolean }>,
  rooms: Room[]
): Record<string, { count: number; hasAlarm: boolean }> {
  const result: Record<string, { count: number; hasAlarm: boolean }> = {};

  rooms.forEach((room) => {
    result[room.id] = { count: 0, hasAlarm: false };
  });

  cards.forEach((card) => {
    for (const room of rooms) {
      if (isCardInRoom(card.x, card.y, card.w, card.h, room)) {
        result[room.id].count++;
        if (card.isAlarming) {
          result[room.id].hasAlarm = true;
        }
        break; // 一个卡片只属于一个房间
      }
    }
  });

  return result;
}
