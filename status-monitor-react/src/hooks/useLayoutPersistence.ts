import { useCallback } from 'react';
import { useLayoutStore } from '../store/useLayoutStore';

/**
 * 布局持久化 hook
 * 封装 localStorage 读写和 JSON 文件导入/导出的逻辑
 */
export function useLayoutPersistence() {
  const saveToStorage = useLayoutStore((s) => s.saveToStorage);
  const loadFromStorage = useLayoutStore((s) => s.loadFromStorage);
  const saveToSlot = useLayoutStore((s) => s.saveToSlot);
  const loadFromSlot = useLayoutStore((s) => s.loadFromSlot);
  const getAllSlots = useLayoutStore((s) => s.getAllSlots);
  const exportToJSON = useLayoutStore((s) => s.exportToJSON);
  const importFromJSON = useLayoutStore((s) => s.importFromJSON);

  /** 导出布局为 JSON 文件下载 */
  const exportToFile = useCallback(() => {
    const json = exportToJSON();
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `smile_layout_${Date.now()}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [exportToJSON]);

  /** 从文件读取并导入布局 */
  const importFromFile = useCallback(
    (file: File): Promise<boolean> => {
      return new Promise((resolve) => {
        const reader = new FileReader();
        reader.onload = (e) => {
          const text = e.target?.result as string;
          const success = importFromJSON(text);
          resolve(success);
        };
        reader.onerror = () => resolve(false);
        reader.readAsText(file);
      });
    },
    [importFromJSON]
  );

  return {
    saveToStorage,
    loadFromStorage,
    saveToSlot,
    loadFromSlot,
    getAllSlots,
    exportToFile,
    importFromFile,
  };
}
