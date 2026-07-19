import { useThemeStore } from '../../store/useThemeStore';
import { THEME_OPTIONS } from '../../constants/defaults';
import type { ThemeName } from '../../types/layout';
import styles from './ThemeSelector.module.css';

export function ThemeSelector() {
  const currentTheme = useThemeStore((s) => s.currentTheme);
  const setTheme = useThemeStore((s) => s.setTheme);

  return (
    <select
      className={styles.select}
      value={currentTheme}
      onChange={(e) => setTheme(e.target.value as ThemeName)}
      title="切换系统主题"
    >
      {THEME_OPTIONS.map((opt) => (
        <option key={opt.value} value={opt.value}>
          {opt.label}
        </option>
      ))}
    </select>
  );
}
