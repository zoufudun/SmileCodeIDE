# SmileCode Qt 动态插件开发模板 (TemplatePlugin)

本项目为 **SmileCode Studio / PhudonTools** 的标准 Qt 插件开发模板工程。通过本项目，您可以快速开发出具有独立 UI、业务逻辑、与宿主服务互通的动态链接库插件（`.dll`）。

---

## 快速开发三步法

### 1. 修改插件信息与唯一 ID
在 `templateplugin.h` 与 `plugin.json` 中修改您的插件信息：
- `id()`: 唯一标识（英文字母、下划线，如 `my_serial_tool`）
- `name()`: 插件名称（如 `我的串口助手`）
- `category()`: 分类（可选：`"嵌入式开发"`、`"总线与通信"`、`"测量与分析"`、`"固件与烧录"`、`"实用工具"`）
- `version()`: 版本号（如 `v1.0.0`）
- `author()`: 作者名称
- `iconUnicode()`: 图标字体十六进制编码（如 `"0xe7d2"`）
- `colorHex()`: 主题强调色（如 `"#6c5ce7"`）

### 2. 编写您的业务界面
在 `templatewidget.h` 和 `templatewidget.cpp` 中编写您的 Qt 界面与逻辑。
可以通过 `IPluginContext` 调用宿主核心服务：
- `m_context->showToast(message, type)`: 触发宿主右上角悬浮通知
- `m_context->log(level, message)`: 记录到全局宿主日志
- `m_context->currentTheme()`: 查询当前主题

### 3. 一键编译与热加载测试
在当前目录下运行命令行：
```powershell
qmake TemplatePlugin.pro
mingw32-make
```

编译完成后：
1. 当前目录下会生成 `TemplatePlugin.dll`。
2. 脚本会自动将该 DLL 拷贝至 `PhudonTools/plugins/` 目录。
3. 启动 `PhudonTools.exe`（或者在运行中直接放入），程序将**自动发现、加载并在应用市场/工作台呈现**！

