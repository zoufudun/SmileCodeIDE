import { create } from 'zustand';
import type { ThemeName } from '../types/layout';
import { STORAGE_KEYS } from '../types/layout';
import { DEFAULT_THEME } from '../constants/defaults';

interface ThemeState {
  currentTheme: ThemeName;
  setTheme: (theme: ThemeName) => void;
  initTheme: () => void;
}

const THEMES: ThemeName[] = ['cyber', 'emerald', 'amber', 'ruby'];

function applyTheme(theme: ThemeName) {
  document.body.classList.remove(...THEMES.map((name) => `theme-${name}`));
  document.body.classList.add(`theme-${theme}`);
}

export const useThemeStore = create<ThemeState>((set) => ({
  currentTheme: DEFAULT_THEME,

  setTheme: (theme) => {
    applyTheme(theme);
    localStorage.setItem(STORAGE_KEYS.THEME, theme);
    set({ currentTheme: theme });
  },

  initTheme: () => {
    const saved = localStorage.getItem(STORAGE_KEYS.THEME) as ThemeName | null;
    const theme = saved && THEMES.includes(saved) ? saved : DEFAULT_THEME;
    applyTheme(theme);
    set({ currentTheme: theme });
  },
}));
