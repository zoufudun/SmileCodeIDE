import type { BackgroundTemplate as BgType } from '../../types/layout';
import { SubmarineBg } from './templates/SubmarineBg';
import { BuildingBg } from './templates/BuildingBg';
import { WarshipBg } from './templates/WarshipBg';
import styles from './BackgroundTemplate.module.css';

interface BackgroundTemplateProps {
  type: BgType;
}

const BG_MAP: Record<string, React.FC> = {
  submarine: SubmarineBg,
  building: BuildingBg,
  warship: WarshipBg,
};

export function BackgroundTemplate({ type }: BackgroundTemplateProps) {
  const Component = BG_MAP[type];
  if (!Component) return null;

  return (
    <div className={styles.bg}>
      <Component />
    </div>
  );
}
