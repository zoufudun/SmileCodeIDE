import { useEffect } from 'react';
import { AppHeader } from './components/layout/AppHeader';
import { LayoutToolbar } from './components/layout/LayoutToolbar';
import { DeviceGrid } from './components/devices/DeviceGrid';
import { DeviceCanvas } from './components/devices/DeviceCanvas';
import { EmptyState } from './components/common/EmptyState';
import { AlarmFooter } from './components/logs/AlarmFooter';
import { InfoDrawer } from './components/logs/InfoDrawer';
import { AddRoomModal } from './components/rooms/AddRoomModal';
import { useWebSocket } from './hooks/useWebSocket';
import { useWebSocketStore } from './store/useWebSocketStore';
import { useThemeStore } from './store/useThemeStore';
import { useLayoutStore } from './store/useLayoutStore';
import { useLogStore } from './store/useLogStore';
import styles from './App.module.css';

export default function App() {
  // WebSocket 连接
  useWebSocket();

  // 初始化主题和布局
  const initTheme = useThemeStore((s) => s.initTheme);
  const loadFromStorage = useLayoutStore((s) => s.loadFromStorage);
  const appendLog = useLogStore((s) => s.appendLog);

  useEffect(() => {
    initTheme();
    loadFromStorage();
    appendLog('系统已就绪，等待连接到本地 Qt CANTool 消息服务器...', 'system');
  }, []);

  // 数据
  const latestMappings = useWebSocketStore((s) => s.latestMappings);
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const hasSavedLayout = useLayoutStore(
    (s) => s.rooms.length > 0 || Object.keys(s.devicePositions).length > 0
  );

  const hasDevices = latestMappings.length > 0;

  // 编辑模式下给 body 添加 class
  useEffect(() => {
    if (isEditMode) {
      document.body.classList.add('edit-mode');
    } else {
      document.body.classList.remove('edit-mode');
    }
    return () => document.body.classList.remove('edit-mode');
  }, [isEditMode]);

  return (
    <div className={styles.container}>
      <AppHeader />
      <LayoutToolbar />

      <main className={styles.main}>
        {!hasDevices && <EmptyState />}

        {/* 网格模式：无已存布局且非编辑模式 */}
        {hasDevices && !hasSavedLayout && !isEditMode && (
          <div className={styles.grid}>
            <DeviceGrid />
          </div>
        )}

        {/* 画布模式：有已存布局或编辑模式 */}
        {hasDevices && (hasSavedLayout || isEditMode) && <DeviceCanvas />}
      </main>

      <AlarmFooter />
      <InfoDrawer />
      <AddRoomModal />
    </div>
  );
}
