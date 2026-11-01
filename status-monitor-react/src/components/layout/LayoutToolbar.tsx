import { useRef, useState, useEffect } from 'react';
import { useLayoutStore } from '../../store/useLayoutStore';
import { useUIStore } from '../../store/useUIStore';
import { useWebSocketStore } from '../../store/useWebSocketStore';
import { useLogStore } from '../../store/useLogStore';
import { useLayoutPersistence } from '../../hooks/useLayoutPersistence';
import { GRID_SNAP_OPTIONS } from '../../constants/defaults';
import type { GridSnapSize } from '../../types/layout';
import styles from './LayoutToolbar.module.css';

export function LayoutToolbar() {
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const gridSnapSize = useLayoutStore((s) => s.gridSnapSize);
  const setGridSnap = useLayoutStore((s) => s.setGridSnap);
  const setEditMode = useLayoutStore((s) => s.setEditMode);
  const loadTemplate = useLayoutStore((s) => s.loadTemplate);
  const saveToStorage = useLayoutStore((s) => s.saveToStorage);
  const resetLayout = useLayoutStore((s) => s.resetLayout);
  const loadFromSlot = useLayoutStore((s) => s.loadFromSlot);

  const setModalAddRoomOpen = useUIStore((s) => s.setModalAddRoomOpen);
  const latestMappings = useWebSocketStore((s) => s.latestMappings);
  const appendLog = useLogStore((s) => s.appendLog);

  const { exportToFile, importFromFile, getAllSlots } = useLayoutPersistence();
  const fileInputRef = useRef<HTMLInputElement>(null);

  const [slots, setSlots] = useState<string[]>([]);

  useEffect(() => {
    setSlots(getAllSlots());
  }, [getAllSlots]);

  const handleLoadTemplate = (templateName: string) => {
    if (!templateName) return;
    if (confirm('确定要加载所选的布局模版吗？这将覆盖您当前的房间分布和坐标。')) {
      const deviceIds = latestMappings.map((d) => d.deviceId);
      loadTemplate(templateName, deviceIds);
      appendLog('✓ 成功载入模版，建议在此基础上进行手动拖拽划分，完成后点击"保存当前"。', 'system');
    }
  };

  const handleSave = () => {
    saveToStorage();
    appendLog('✓ 布局和房间划分已成功保存到默认配置！', 'system');
    setEditMode(false);
  };

  const handleReset = () => {
    if (confirm('确定要重置当前布局吗？这将会删除所有的自定义分区和坐标摆放。')) {
      resetLayout();
      appendLog('布局已重置为默认网格摆放。', 'system');
    }
  };

  const handleLoadSlot = (slotName: string) => {
    if (!slotName) return;
    const ok = loadFromSlot(slotName);
    if (ok) {
      appendLog(`✓ 已成功载入已存布局: "${slotName}"`, 'system');
    } else {
      appendLog(`❌ 载入已存布局 "${slotName}" 失败`, 'warning');
    }
  };

  const handleImportFile = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    const success = await importFromFile(file);
    if (success) {
      appendLog('✓ 成功从文件导入布局，当前处于编辑状态，请点击保存当前。', 'system');
    } else {
      alert('无效的布局文件格式！');
    }
    if (fileInputRef.current) fileInputRef.current.value = '';
  };

  if (!isEditMode) return null;

  return (
    <div className={styles.toolbar}>
      {/* 模板管理 */}
      <div className={styles.group}>
        <span className={styles.label}>📂 模板管理:</span>
        <select
          className={styles.select}
          defaultValue=""
          onChange={(e) => { handleLoadTemplate(e.target.value); e.target.value = ''; }}
        >
          <option value="" disabled>载入预设模版...</option>
          <option value="submarine">🚢 核潜艇船显布局</option>
          <option value="building">🏢 写字楼垂直布局</option>
          <option value="warship">🛥️ 水面战舰甲板布局</option>
        </select>
        <select
          className={styles.select}
          defaultValue=""
          onChange={(e) => { handleLoadSlot(e.target.value); e.target.value = ''; }}
        >
          <option value="" disabled>载入已存布局...</option>
          {slots.map((name) => (
            <option key={name} value={name}>📁 {name}</option>
          ))}
        </select>
      </div>

      <div className={styles.divider} />

      {/* 网格对齐 */}
      <div className={styles.group}>
        <span className={styles.label}>📐 网格对齐:</span>
        <select
          className={styles.select}
          value={gridSnapSize}
          onChange={(e) => setGridSnap(Number(e.target.value) as GridSnapSize)}
        >
          {GRID_SNAP_OPTIONS.map((opt) => (
            <option key={opt.value} value={opt.value}>{opt.label}</option>
          ))}
        </select>
      </div>

      <div className={styles.divider} />

      {/* 布局编辑 */}
      <div className={styles.group}>
        <span className={styles.label}>⚙️ 布局编辑:</span>
        <button className={`${styles.btn} ${styles.secondary}`} onClick={() => fileInputRef.current?.click()}>
          📥 导入
        </button>
        <input
          ref={fileInputRef}
          type="file"
          accept=".json"
          style={{ display: 'none' }}
          onChange={handleImportFile}
        />
        <button className={`${styles.btn} ${styles.secondary}`} onClick={exportToFile}>
          📤 导出
        </button>
        <button className={styles.btn} onClick={() => setModalAddRoomOpen(true)}>
          ＋ 新建房间
        </button>
      </div>

      <div className={`${styles.group} ${styles.end}`}>
        <button className={`${styles.btn} ${styles.secondary}`} onClick={handleReset}>
          ↺ 重置
        </button>
        <button className={`${styles.btn} ${styles.success}`} onClick={handleSave}>
          ✓ 保存当前
        </button>
      </div>
    </div>
  );
}
