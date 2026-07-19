import { useLayoutStore } from '../../store/useLayoutStore';
import { useUIStore } from '../../store/useUIStore';
import styles from './AppNav.module.css';

export function AppNav() {
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const setEditMode = useLayoutStore((s) => s.setEditMode);
  const infoDrawerOpen = useUIStore((s) => s.infoDrawerOpen);
  const toggleInfoDrawer = useUIStore((s) => s.toggleInfoDrawer);
  const setInfoDrawerOpen = useUIStore((s) => s.setInfoDrawerOpen);
  const footerVisible = useUIStore((s) => s.footerVisible);
  const toggleFooter = useUIStore((s) => s.toggleFooter);

  return (
    <nav className={styles.nav}>
      <button
        className={`${styles.item} ${!isEditMode ? styles.active : ''}`}
        onClick={() => setEditMode(false)}
      >
        📊 实时监控
      </button>
      <button
        className={`${styles.item} ${isEditMode ? styles.active : ''}`}
        onClick={() => setEditMode(true)}
      >
        📐 布局设置
      </button>
      <button
        className={`${styles.item} ${infoDrawerOpen ? styles.active : ''}`}
        onClick={toggleInfoDrawer}
      >
        📟 信息日志
      </button>
      <button
        className={`${styles.item} ${footerVisible ? styles.active : ''}`}
        onClick={toggleFooter}
      >
        📋 报警面板
      </button>
    </nav>
  );
}
