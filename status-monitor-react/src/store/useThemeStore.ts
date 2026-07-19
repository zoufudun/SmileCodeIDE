import { create } from 'zustand';
import type { ThemeName } from '../types/layout';
import { STORAGE_KEYS } from '../types/layout';
import { DEFAULT_THEME } from '../constants/defaults';

interface ThemeState {
  currentTheme: ThemeName;
  setTheme: (theme: ThemeName) => void;
  initTheme: () => void;
}

export const useThemeStore = create<ThemeState>((set) => ({
  currentTheme: DEFAULT_THEME,

  setTheme: (theme) => {
    // 移除所有现有主题 class
    document.body.classList.forEach((cls) => {
      if (cls.startsWith('theme-')) {
        document.body.classList.remove(cls);
      }
    });
    document.body.classList.add(`theme-${theme}`);
    localStorage.setItem(STORAGE_KEYS.THEME, theme);
    set({ currentTheme: theme });
  },

  initTheme: () => {
    const saved = localStorage.getItem(STORAGE_KEYS.THEME) as ThemeName | null;
    const theme = saved && ['cyber', 'emerald', 'amber', 'ruby'].includes(saved)
      ? saved
      : DEFAULT_THEME;
    document.body.classList.add(`theme-${theme}`);
    set({ currentTheme: theme });
  },
}));
