# Qt 5.15 与 Qt 6.11 双版本无缝兼容适配方案

全面检查整个 SmileCodeIDE / PhudonTools 项目，解决 Qt 5.15 与 Qt 6.11（及 Qt 6.x）之间的构建系统配置、核心库差异、类成员签名变更与已废弃 API 问题，确保项目在两代 Qt 环境下均能一键编译通过并稳定运行。

---

## 🔍 现状与全面检查发现的问题

经全工程扫描与 Qt 6.11.2 实际编译排查，共发现以下 5 大类、涵盖 15+ 处文件的兼容性问题：

1. **构建系统与依赖库路径硬编码 (Build System & QScintilla)**:
   - `PhudonTools/PhudonTools.pro` 硬编码了个人本地路径：
     `LIBS += -L"D:/Soft/Qt/5.15.2/mingw81_64/lib" -lqscintilla2_qt5`
     `INCLUDEPATH += "D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci"`
     在其他机器或 Qt 6.11 环境下直接报错找不到头文件和静态库。
   - QScintilla 库名在 Qt5 为 `qscintilla2_qt5`，Qt6 为 `qscintilla2_qt6`。工程根目录下已内置 `QScintilla` 完整源码，可自给自足构建。
   - 缺少顶层 `SmileCodeIDE.pro` 根工程管理文件，无法在 Qt Creator 中按序一键构建 QScintilla、主程序与插件。
   - `CONFIG += c++11` 在 Qt 6.11 下不合规（Qt6 强制要求至少 C++17），需升级为 `CONFIG += c++17`（Qt 5.15 与现代编译器完全支持 C++17）。

2. **事件虚函数签名变更 (Event Signature Changes)**:
   - Qt 6 中 `QWidget::enterEvent` 的参数类型由 `QEvent *` 更改为 `QEnterEvent *`，导致带有 `override` 关键字的子类全部编译失败（error: 'void ...::enterEvent(QEvent*)' marked 'override', but does not override）：
     - [appcardwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/appcardwidget.h) / [appcardwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/appcardwidget.cpp)
     - [devicestatuswidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h) / [devicestatuswidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
     - [toastwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/toastwidget.h) / [toastwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/toastwidget.cpp)
     - [verticaltabwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/verticaltabwidget.h) / [verticaltabwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/verticaltabwidget.cpp)
     - [codeeditor.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp) (`StickyScrollWidget`)

3. **编码支持类 QTextCodec 模块迁移 (QtCore5Compat Migration)**:
   - Qt 6 将 `QTextCodec` 从 `QtCore` 移至 `QtCore5Compat` 模块。
   - [serialportplot.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.h)、[serialportplot.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.cpp)、[canprotocolconfigdialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp) 等因缺少头文件适配在 Qt6 报错 `'QTextCodec' does not name a type`。
   - 需要在 `PhudonTools.pro` 中条件添加 `greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat`，并配合条件宏包含头文件。

4. **已被废弃或移除的 API (Deprecated/Removed APIs)**:
   - `QDesktopWidget`：在 Qt 6 中彻底移除，[CUIHelper.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/TOOLS/CUIHelper.h) 与 [CUIHelper.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/TOOLS/CUIHelper.cpp) 使用了 `QApplication::desktop()`，需通过 `QGuiApplication::primaryScreen()` 平滑替换。
   - `QRegExp` 与 `QRegExpValidator`：在 Qt 6 已废弃并由 `QRegularExpression` 全面取代：
     - [canprotocolconfigdialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)：使用了 `QRegExp` 及 `QString::SkipEmptyParts`（Qt6 中枚举已移入 `Qt::SkipEmptyParts`）。
     - [oscilloscopewindow.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/oscilloscopewindow.cpp)：使用了 `QRegExp("[,;\\s]+")`。
     - [candevicedialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/candevicedialog.cpp)：使用了 `QRegExpValidator`。
     - [terminalwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/terminalwidget.cpp)：多处 ANSI 转义替换使用 `QRegExp`，可改为直接字面值字符串替换或 `QRegularExpression`，提升性能并杜绝废弃警告。

5. **插件工程配置 (Plugins)**:
   - `Plugins/SignalGeneratorPlugin/SignalGeneratorPlugin.pro` 与 `Plugins/TemplatePlugin/TemplatePlugin.pro` 中的 `CONFIG += c++11` 统一升级为 `CONFIG += c++17`。

---

## User Review Required

> [!IMPORTANT]
> 1. **代码标准**: 我们将项目 `CONFIG += c++11` 统一升级为 `CONFIG += c++17`。C++17 是 Qt 6 的强制基准，同时被 Qt 5.15 官方全面支持（如 GCC 8+、Clang、MSVC 2017/2019/2022），不会影响任何旧版环境编译。
> 2. **内置 QScintilla 构建**: 我们将提供统一的根工程文件 `SmileCodeIDE.pro`，在 Qt Creator 中打开该根工程即可自动按序先构建 `QScintilla`，再构建 `PhudonTools`，最后构建 `Plugins`，无需手动先去外部编译 QScintilla。如果用户单独打开 `PhudonTools/PhudonTools.pro`，也会优先使用工程内置源码路径，彻底告别绝对路径失效问题。

---

## Proposed Changes

### 1. 构建系统与根工程配置 (Build System & Project Configuration)

#### [NEW] [SmileCodeIDE.pro](file:///k:/TOPFIRE/SmileCodeIDEUpdate/SmileCodeIDE.pro)
- 创建 `subdirs` 类型的顶层构建工程：
  ```qmake
  TEMPLATE = subdirs
  CONFIG += ordered
  SUBDIRS += \
      QScintilla/src \
      PhudonTools \
      Plugins
  PhudonTools.depends = QScintilla/src
  Plugins.depends = PhudonTools
  ```

#### [MODIFY] [PhudonTools.pro](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/PhudonTools.pro)
- 升级标准至 `CONFIG += c++17`。
- 添加 Qt 6 兼容模块判断：`greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat`。
- 移除硬编码路径 `D:/Soft/Qt/5.15.2/...`。
- 添加工程内自包含包含路径：`INCLUDEPATH += $$PWD/../QScintilla/src $$PWD/../QScintilla/src/Qsci`。
- 根据 `QT_MAJOR_VERSION` 动态选择库名称（Qt5 -> `qscintilla2_qt5`，Qt6 -> `qscintilla2_qt6`），并自动查找工程内输出目录。
- Windows 下后链接阶段自动检测并拷贝 `qscintilla2_qt*.dll` 到运行目录。

#### [MODIFY] [SignalGeneratorPlugin.pro](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/SignalGeneratorPlugin/SignalGeneratorPlugin.pro)
- 将 `CONFIG += c++11 plugin` 更新为 `CONFIG += c++17 plugin`。

#### [MODIFY] [TemplatePlugin.pro](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/TemplatePlugin/TemplatePlugin.pro)
- 将 `CONFIG += c++11 plugin` 更新为 `CONFIG += c++17 plugin`。

---

### 2. 事件签名兼容处理 (enterEvent Signature Compatibility)

通过 Qt 版本宏判断 `QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)` 实现双版本无缝重载。

#### [MODIFY] [appcardwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/appcardwidget.h) & [appcardwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/appcardwidget.cpp)
- 头文件添加条件宏重载：Qt6 下使用 `void enterEvent(QEnterEvent *event) override;`，Qt5 下使用 `void enterEvent(QEvent *event) override;`。
- 源文件实现条件匹配。

#### [MODIFY] [devicestatuswidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.h) & [devicestatuswidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicestatuswidget.cpp)
- 头文件与源文件适配 `enterEvent` 签名。

#### [MODIFY] [toastwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/toastwidget.h) & [toastwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/toastwidget.cpp)
- 头文件与源文件适配 `enterEvent` 签名。

#### [MODIFY] [verticaltabwidget.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/verticaltabwidget.h) & [verticaltabwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/verticaltabwidget.cpp)
- 头文件与源文件适配 `enterEvent` 签名。

#### [MODIFY] [codeeditor.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)
- `StickyScrollWidget` 内部类适配 `enterEvent` 签名。

---

### 3. QTextCodec 与编码处理 (QTextCodec Compatibility)

#### [MODIFY] [serialportplot.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.h) & [serialportplot.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/serialportplot.cpp)
- 头文件与源文件添加宏判断引入：
  ```cpp
  #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  #include <QtCore5Compat/QTextCodec>
  #else
  #include <QTextCodec>
  #endif
  ```

#### [MODIFY] [canprotocolconfigdialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- 适配 `QTextCodec` 包含路径。

---

### 4. API 废弃与移除替换 (API Modernization)

#### [MODIFY] [CUIHelper.h](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/TOOLS/CUIHelper.h) & [CUIHelper.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/TOOLS/CUIHelper.cpp)
- 移除 `<QDesktopWidget>` 强依赖，引入 `<QScreen>` 与 `<QGuiApplication>`。
- 将 `QApplication::desktop()->screenGeometry().size()` 替换为跨版本兼容的 `QGuiApplication::primaryScreen()->geometry().size()`。
- 将 `QApplication::desktop()->screen()` 在 Qt6 下规范置空或绑定到 primaryScreen。

#### [MODIFY] [canprotocolconfigdialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- 包含 `<QRegularExpression>`。
- 将 `QRegExp("[\r\n]+")` 替换为 `QRegularExpression("[\r\n]+")`。
- 将 `QString::SkipEmptyParts` 替换为 `Qt::SkipEmptyParts`。

#### [MODIFY] [oscilloscopewindow.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/oscilloscopewindow.cpp)
- 包含 `<QRegularExpression>`。
- 将 `QRegExp("[,;\\s]+")` 替换为 `QRegularExpression("[,;\\s]+")`。

#### [MODIFY] [candevicedialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/candevicedialog.cpp)
- 引入 `<QRegularExpressionValidator>`。
- 将 `QRegExpValidator(QRegExp(...))` 替换为 `QRegularExpressionValidator(QRegularExpression("^[0-9a-fA-F]{1,8}$"), this)`。

#### [MODIFY] [terminalwidget.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/terminalwidget.cpp)
- 将 ANSI 颜色转义代码的大量 `html.replace(QRegExp("\033\\[30m"), ...)` 优化为标准字符串替换 `html.replace("\033[30m", ...)`，既消除了对过时 `QRegExp` 的依赖，又极大提升了终端字符流解析性能。

---

## Verification Plan

### Automated Build Verification (全量自动构建验证)
1. **QScintilla 动态库构建测试**:
   - 验证在当前 Qt 6.11 环境下正常生成 `qscintilla2_qt6.dll` 与 `libqscintilla2_qt6.a`（前面已验证通过）。
2. **PhudonTools 全量编译测试**:
   - 执行 `qmake PhudonTools.pro` 并执行 `mingw32-make -j4` 构建整个可执行文件，确保所有 50+ 个 cpp 模块编译零错误，并成功链接生成 `PhudonTools.exe`。
3. **插件全量编译测试**:
   - 运行 `Plugins/SignalGeneratorPlugin` 和 `Plugins/TemplatePlugin` 的构建，验证生成动态库且自动拷贝至主程序 `plugins/` 目录。
4. **Qt 5.15 兼容性静态/宏规则校验**:
   - 检查所有预编译条件分支，确保在 `QT_VERSION < 0x060000` 条件下的语法及符号完全符合 Qt 5.15 规范。

### Manual Verification (运行验证)
1. 运行生成的 `PhudonTools.exe`，验证主窗口、Sticky Scroll 编辑器、示波器、CAN 总线界面及应用管理器的正常启动与交互。
