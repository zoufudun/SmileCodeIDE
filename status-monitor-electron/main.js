const { app, BrowserWindow } = require('electron');
const path = require('path');

// 解决 Windows 上磁盘缓存权限问题
// 将 userData 设置到项目目录下的本地缓存文件夹，避免 %APPDATA% 权限冲突
const userDataPath = path.join(__dirname, '.electron-cache');
app.setPath('userData', userDataPath);

// 禁用 GPU 缓存以避免 Gpu Cache Creation failed 错误
app.commandLine.appendSwitch('disable-gpu-cache');
// 禁用磁盘缓存迁移（解决 cache_util_win 中的 "拒绝访问" 错误）
app.commandLine.appendSwitch('disable-features', 'MoveCache');

function createWindow() {
  const win = new BrowserWindow({
    width: 1100,
    height: 800,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    },
    title: "Status Monitor - Electron Web版",
    backgroundColor: '#0b0f19',
    autoHideMenuBar: true
  });

  win.webContents.on('console-message', (event, level, message, line, sourceId) => {
    console.log(`[RENDERER CONSOLE] ${message} (at ${path.basename(sourceId)}:${line})`);
  });

  win.loadFile('index.html');
//  win.webContents.openDevTools();
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
