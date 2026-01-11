# AGENTS.md - STM32IDE Development Guide

## Build Commands

This is a Qt-based C++ project using qmake as the build system.

### Full Build
```bash
# From the STM32IDE directory
qmake STM32IDE.pro
make release
```

### Debug Build
```bash
make debug
```

### Clean Build
```bash
make release-clean
make debug-clean
```

### Rebuild from Scratch
```bash
make distclean
qmake STM32IDE.pro
make release
```

### Single File Compilation (for testing)
```bash
# Compile a single test file
g++ -I"D:/Soft/Qt/6.9.0/mingw_64/include" -I"D:/Soft/Qt/6.9.0/mingw_64/include/QtCore" -I"D:/Soft/Qt/6.9.0/mingw_64/include/QtWidgets" -fno-keep-inline-dlspace -o regex_test regex_test.cpp
```

### Qt Path Configuration
- Qt 6.9.0: `D:/Soft/Qt/6.9.0/mingw_64`
- Qt 5.15.2: `D:/Soft/Qt/5.15.2/mingw81_64`
- QScintilla: `D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci`

## Code Style Guidelines

### Indentation and Spacing
- Use 4 spaces for indentation (no tabs)
- Use spaces around operators and after commas
- Opening braces on the same line as the function/class declaration
- No extra spaces before parentheses in function calls

### Naming Conventions

#### Classes
- PascalCase: `MainWindow`, `CodeEditor`, `BuildSystem`
- Header files: `.h` extension
- Implementation files: `.cpp` extension

#### Member Variables
- Prefix with `m_`: `m_process`, `m_projectPath`, `m_settings`
- Private members follow the same convention

#### Functions and Methods
- camelCase: `openProject()`, `buildProject()`, `setToolchainPath()`
- Getters: `targetChip()` (no get prefix)
- Setters: `setTargetChip(const QString &chip)`

#### Constants and Enums
- k-prefixed camelCase for constants: `kDefaultPort`
- PascalCase for enum values: `Theme::Dark`, `Theme::Light`

#### Qt-Specific
- Slots: `void openProject();` (no special naming required)
- Signals: `void buildFinished(bool success);`
- Use `Q_OBJECT` macro in all QObject-derived classes

### Header File Structure

```cpp
#ifndef CLASSNAME_H
#define CLASSNAME_H

#include <QtModule>
class ForwardDeclaration;

class ClassName : public QBaseClass {
  Q_OBJECT
public:
  explicit ClassName(QObject *parent = nullptr);
  ~ClassName();

  returnType functionName(paramType param);

signals:
  void signalName();

private:
  void privateMethod();
  returnType m_memberVariable;
};
#endif // CLASSNAME_H
```

### Include Order
1. Qt includes (alphabetical)
2. Forward declarations for Qt classes
3. Project local includes (in quotes)

### Code Patterns

#### Explicit Constructors
```cpp
explicit ClassName(QWidget *parent = nullptr);
```

#### Signal/Slot Connections
```cpp
connect(m_process, &QProcess::finished, this, &MainWindow::processFinished);
```

#### Use nullptr, not NULL or 0
```cpp
QObject *parent = nullptr;
```

#### Override Specifier
```cpp
void keyPressEvent(QKeyEvent *event) override;
```

#### Use enum class (C++11)
```cpp
enum class Theme { Dark, Light, Blue };
```

### Comments
- Use `//` for single-line comments (no `/* */` blocks)
- Header comments in source files:
  ```cpp
  /*
   * @Description:
   * @Version: 1.0
   * @Autor: AuthorName
   * @Date: YYYY-MM-DD HH:MM:SS
   */
  ```
- Avoid unnecessary comments; code should be self-documenting

### Error Handling
- Use exceptions sparingly; prefer error codes for public APIs
- Check return values and handle errors gracefully
- Use Q_ASSERT for internal invariants in debug builds
- Log errors with qWarning() or qCritical()

### Resource Management
- Use RAII (Resource Acquisition Is Initialization)
- Parent-child relationships for Qt widgets
- Use smart pointers where appropriate
- Delete or deleteLater() for QObject cleanup

### Qt Best Practices
- Use signals and slots for inter-object communication
- Use Q_PROPERTY for persistent properties
- Use Q_INVOKABLE for methods callable from QML
- Use tr() for translatable strings
- Use QVariant for property serialization

### File Organization
- Main code in root directory
- UI tools in `TOOLS/` subdirectory
- Resources in `resources/` directory
- Icons in `icons/` directory
- Build outputs in `build/`, `debug/`, `release/` directories

### Special Notes
- QScintilla integration for code editing features
- SerialPort support for embedded debugging
- BuildSystem class handles STM32 toolchain integration
- Multiple theme support for editor colors
- Project uses UTF-8 encoding throughout
