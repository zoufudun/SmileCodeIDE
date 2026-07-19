import { useWebSocketStore } from '../../store/useWebSocketStore';
import { DeviceCard } from './DeviceCard';
import styles from './DeviceGrid.module.css';

export function DeviceGrid() {
  const latestMappings = useWebSocketStore((s) => s.latestMappings);

  if (latestMappings.length === 0) return null;

  return (
    <div className={styles.grid}>
      {latestMappings.map((device) => (
        <DeviceCard key={device.deviceId} device={device} />
      ))}
    </div>
  );
}
