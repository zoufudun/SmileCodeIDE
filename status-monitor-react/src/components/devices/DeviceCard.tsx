import type { DeviceMapping } from '../../types/device';
import { DetectorIcon } from './DetectorIcon';
import { ValveIcon } from './ValveIcon';
import { CylinderIcon } from './CylinderIcon';
import { PumpIcon } from './PumpIcon';
import { PressureSwitchIcon } from './PressureSwitchIcon';
import { ManualCallPointIcon } from './ManualCallPointIcon';
import { MobileSprayGunIcon } from './MobileSprayGunIcon';
import styles from './DeviceCard.module.css';

interface DeviceCardProps {
  device: DeviceMapping;
  absolute?: boolean;
  position?: { x: number; y: number };
  dragProps?: { onMouseDown: (e: React.MouseEvent) => void };
}

// 设备图标映射（deviceType 对齐 Qt 后端）
const ICON_MAP: Record<string, React.FC> = {
  detector: DetectorIcon,
  valve: ValveIcon,
  manual_alarm: ManualCallPointIcon,
  gas_cylinder: CylinderIcon,
  water_pump: PumpIcon,
  pressure_switch: PressureSwitchIcon,
  mobile_spray_gun: MobileSprayGunIcon,
};

// 状态文字映射（对齐 Qt 后端设备名称）
const STATUS_TEXT_MAP: Record<string, Record<string, string>> = {
  detector:          { '0': '正常', '1': '报警' },
  valve:             { '0': '关',   '1': '开' },
  manual_alarm:      { '0': '正常', '1': '按下' },
  gas_cylinder:      { '0': '正常', '1': '泄漏' },
  water_pump:        { '0': '停止', '1': '运转' },
  pressure_switch:   { '0': '关闭', '1': '开启' },
  mobile_spray_gun:  { '0': '停止', '1': '喷射' },
};

export function DeviceCard({ device, absolute, position, dragProps }: DeviceCardProps) {
  const { deviceType, status, deviceId, label, canId } = device;
  const statusKey = status ? '1' : '0';

  const statusText = STATUS_TEXT_MAP[deviceType]?.[statusKey] ?? (status ? '报警' : '正常');
  const canIdHex = '0x' + (canId || 0).toString(16).toUpperCase().padStart(3, '0');
  const Icon = ICON_MAP[deviceType] ?? DetectorIcon;

  return (
    <div
      id={`device-card-${deviceId}`}
      data-id={deviceId}
      data-type={deviceType}
      data-label={label}
      className={`${styles.card} ${styles[`kind-${deviceType}`]} ${styles[`status-${statusKey}`]}`}
      style={absolute && position ? { position: 'absolute', left: position.x, top: position.y } : undefined}
      {...(dragProps && absolute ? dragProps : {})}
    >
      <span className={styles.canId}>CAN {canIdHex}</span>
      <span className={styles.deviceIdBadge}>#{deviceId}</span>
      <div className={styles.iconContainer}>
        <Icon />
      </div>
      <span className={styles.label} title={label}>{label}</span>
      <span className={styles.status}>{statusText}</span>
    </div>
  );
}
