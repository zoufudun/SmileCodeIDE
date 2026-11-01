import { AppNav } from './AppNav';
import { SystemClock } from './SystemClock';
import { ConnectionStatus } from './ConnectionStatus';
import { ThemeSelector } from './ThemeSelector';
import styles from './AppHeader.module.css';

export function AppHeader() {
  return (
    <header className={styles.header}>
      <div className={styles.logoArea}>
        <div className={styles.logoIcon} />
        <div className={styles.logoTitle}>
          <h1>STATUS MONITOR</h1>
          <span className={styles.subtitle}>CAN 2.0B 自定义协议监控网关</span>
        </div>
      </div>

      <AppNav />

      <div className={styles.infoGroup}>
        <ThemeSelector />
        <SystemClock />
        <ConnectionStatus />
      </div>
    </header>
  );
}
