import { useMemo } from 'react';
import type { Room } from '../types/layout';
import { computeRoomContainment } from '../utils/layout-helpers';

interface CardInfo {
  x: number;
  y: number;
  w: number;
  h: number;
  isAlarming: boolean;
}

/**
 * 计算房间设备包含关系 hook
 * @returns roomId → { count, hasAlarm } 的映射
 */
export function useRegionContainment(
  cards: CardInfo[],
  rooms: Room[]
): Record<string, { count: number; hasAlarm: boolean }> {
  return useMemo(() => {
    if (rooms.length === 0) return {};
    return computeRoomContainment(cards, rooms);
  }, [cards, rooms]);
}
