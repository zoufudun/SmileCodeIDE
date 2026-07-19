const { app, BrowserWindow } = require('electron');
const path = require('node:path');

// 解决 Windows 磁盘缓存权限问题
const userDataPath = path.join(__dirname, '..', '.electron-cache');
app.setPath('userData', userDataPath);

// 禁用 GPU 缓存
app.commandLine.appendSwitch('disable-gpu-cache');
// 禁用磁盘缓存迁移
app.commandLine.appendSwitch('disable-features', 'MoveCache');

function createWindow() {
  const win = new BrowserWindow({
    width: 1100,
    height: 800,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
    },
    title: 'Status Monitor - React Web版',
    backgroundColor: '#0b0f19',
    autoHideMenuBar: true,
  });

  // 开发模式加载 Vite dev server，生产模式加载打包文件
  if (process.env.VITE_DEV_SERVER_URL) {
    win.loadURL(process.env.VITE_DEV_SERVER_URL);
    win.webContents.openDevTools();
  } else {
    win.loadFile(path.join(__dirname, '..', 'dist', 'index.html'));
  }
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});
