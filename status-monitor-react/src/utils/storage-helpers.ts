import { STORAGE_KEYS } from '../types/layout';
import type { LayoutData } from '../types/layout';
import { DEFAULT_LAYOUT } from '../constants/defaults';

/** 从 localStorage 加载布局 */
export function loadLayoutFromStorage(): LayoutData | null {
  try {
    const saved = localStorage.getItem(STORAGE_KEYS.LAYOUT);
    if (saved) {
      const data = JSON.parse(saved);
      return {
        backgroundTemplate: data.backgroundTemplate || '',
        rooms: data.rooms || data.regions || [],
        devices: data.devices || {},
      };
    }
  } catch (e) {
    console.error('Error parsing saved layout', e);
  }
  return null;
}

/** 保存布局到 localStorage */
export function saveLayoutToStorage(layout: LayoutData): void {
  localStorage.setItem(STORAGE_KEYS.LAYOUT, JSON.stringify(layout));
}

/** 获取所有布局槽位名称 */
export function getLayoutSlots(): string[] {
  try {
    return JSON.parse(localStorage.getItem(STORAGE_KEYS.LAYOUT_SLOTS) || '[]');
  } catch {
    return [];
  }
}

/** 保存布局到指定槽位 */
export function saveLayoutToSlot(slotName: string, layout: LayoutData): void {
  const slots = getLayoutSlots();
  if (!slots.includes(slotName)) {
    slots.push(slotName);
    localStorage.setItem(STORAGE_KEYS.LAYOUT_SLOTS, JSON.stringify(slots));
  }
  localStorage.setItem(STORAGE_KEYS.LAYOUT_SLOT_PREFIX + slotName, JSON.stringify(layout));
}

/** 从指定槽位加载布局 */
export function loadLayoutFromSlot(slotName: string): LayoutData | null {
  try {
    const saved = localStorage.getItem(STORAGE_KEYS.LAYOUT_SLOT_PREFIX + slotName);
    if (saved) {
      const data = JSON.parse(saved);
      return {
        backgroundTemplate: data.backgroundTemplate || '',
        rooms: data.rooms || data.regions || [],
        devices: data.devices || {},
      };
    }
  } catch (e) {
    console.error('Error loading layout slot', e);
  }
  return null;
}
