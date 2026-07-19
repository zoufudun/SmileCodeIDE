import { useLayoutStore } from '../../store/useLayoutStore';
import { DeviceCard } from './DeviceCard';
import type { DeviceMapping } from '../../types/device';
import styles from './UnplacedDock.module.css';

interface UnplacedDockProps {
  devices: DeviceMapping[];
}

export function UnplacedDock({ devices }: UnplacedDockProps) {
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const hasDevices = devices.length > 0;

  if (!hasDevices || !isEditMode) return null;

  return (
    <div className={styles.dock}>
      <span className={styles.title}>未摆放设备：</span>
      <div className={styles.container}>
        {devices.map((device) => (
          <DeviceCard key={device.deviceId} device={device} />
        ))}
      </div>
    </div>
  );
}
