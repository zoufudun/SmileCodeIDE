import styles from './EmptyState.module.css';

export function EmptyState() {
  return (
    <div className={styles.container}>
      <div className={styles.radar}>
        <div className={styles.scanCircle} />
        <div className={styles.scanLine} />
      </div>
      <p className={styles.title}>正在等待网关数据...</p>
      <p className={styles.desc}>请在 Qt 端「打开设备」并配置设备映射映射列表</p>
    </div>
  );
}
