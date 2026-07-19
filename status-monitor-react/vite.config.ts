import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import electron from 'vite-plugin-electron/simple';

// 通过 VITE_NO_ELECTRON 环境变量控制是否加载 Electron 插件
const isElectron = !process.env.VITE_NO_ELECTRON;

export default defineConfig({
  plugins: [
    react(),
    ...(isElectron
      ? [
          electron({
            main: {
              entry: 'electron/main.cjs',
            },
          }),
        ]
      : []),
  ],
  // Electron 模式下 base 必须是相对路径
  base: isElectron ? './' : '/',
});
