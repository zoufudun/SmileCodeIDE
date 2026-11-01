import { useWebSocketStore } from '../../store/useWebSocketStore';
import styles from './ConnectionStatus.module.css';
import type { ConnectionStatus as ConnStatus } from '../../types/device';

const STATUS_TEXT: Record<ConnStatus, string> = {
  initializing: '正在初始化...',
  connected: '已连接网关',
  disconnected: '未连接 (重试中...)',
};

export function ConnectionStatus() {
  const connectionStatus = useWebSocketStore((s) => s.connectionStatus);

  return (
    <div className={`${styles.container} ${styles[connectionStatus]}`}>
      <span className={styles.dot} />
      <span className={styles.text}>{STATUS_TEXT[connectionStatus]}</span>
    </div>
  );
}
