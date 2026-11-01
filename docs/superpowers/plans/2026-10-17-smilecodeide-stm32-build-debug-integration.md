# SmileCodeIDE STM32 编译构建与 GDB 硬件调试深度集成实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 SmileCodeIDE (`PhudonTools`) 中实现与代码编辑器 `CodeEditor` 深度融合的 STM32 编译构建与硬件级 GDB 在线调试，提供行号边距断点设置、单步停机执行箭头高亮、专业调试停靠面板矩阵（调用栈、局部变量/监视、CPU 寄存器、断点管理）及智能增量 Makefile 构建系统。

**Architecture:** 
1. 将 `Plugins/STM32WorkbenchPlugin/Core/` 成熟的 `BuildManager`、`GdbMiEngine`、`GdbServerManager`、`FlashProgrammer`、`MakefileGenerator`、`STM32Profile`、`ToolchainDetector` 引入 `PhudonTools.pro` 作为原生核心组件；
2. 扩展 `CodeEditor` 的 QScintilla 边距 1 为符号断点槽，提供实心红球 (●) 断点与黄色指针 (➜) 执行线高亮；
3. 建立统一的 `STM32DebugController` 状态机总控中心，驱动全异步非阻塞 GDB/MI 通信与后台 Server 端口探活；
4. 在主窗口 `MainWindow` 搭建标准 `QDockWidget` 调试视窗矩阵并集成底栏编译诊断（Problems）双击穿透与交互式 GDB 控制台。

**Tech Stack:** C++17, Qt 5.15 / Qt 6.x Widgets, QScintilla (QsciScintilla), GNU Arm Embedded Toolchain (arm-none-eabi-gcc / gdb), OpenOCD / J-Link Commander, Make, JSON.

---

## 阶段与任务清单 (Phase & Tasks)

### Task 1: 代码编辑器边距增强 (CodeEditor Breakpoint Margin & Execution Pointer)

**Files:**
- Modify: `PhudonTools/codeeditor.h`
- Modify: `PhudonTools/codeeditor.cpp`

- [ ] **Step 1: 在 `codeeditor.h` 中定义断点标记常量、信号与公共接口**
  - 定义 `MARKER_BREAKPOINT = 26`（断点红点）
  - 定义 `MARKER_EXECUTION_POINTER = 27`（执行指针黄箭头）
  - 声明信号：
    - `void breakpointToggled(const QString &filePath, int line, bool added);`
    - `void allBreakpointsCleared(const QString &filePath);`
  - 声明公有方法：
    - `void setExecutionLine(const QString &filePath, int line);`
    - `void clearExecutionLine();`
    - `QSet<int> getBreakpoints(const QString &filePath) const;`
    - `void setBreakpoints(const QString &filePath, const QSet<int> &lines);`
    - `void openFileAndGoTo(const QString &filePath, int line, int column = 0);`

- [ ] **Step 2: 在 `codeeditor.cpp` 中初始化 Margin 1 并实现交互逻辑**
  - 在 `setupEditor(QsciScintilla *editor)` 中配置 Margin 1：
    ```cpp
    editor->setMarginType(1, QsciScintilla::SymbolMargin);
    editor->setMarginWidth(1, 16);
    editor->setMarginSensitivity(1, true);
    editor->markerDefine(QsciScintilla::Circle, MARKER_BREAKPOINT);
    editor->setMarkerBackgroundColor(QColor("#E53935"), MARKER_BREAKPOINT);
    editor->setMarkerForegroundColor(QColor("#B71C1C"), MARKER_BREAKPOINT);
    
    editor->markerDefine(QsciScintilla::RightArrow, MARKER_EXECUTION_POINTER);
    editor->setMarkerBackgroundColor(QColor("#FFD700"), MARKER_EXECUTION_POINTER);
    editor->setMarkerForegroundColor(QColor("#B8860B"), MARKER_EXECUTION_POINTER);
    ```
  - 连接 `editor->marginClicked(int margin, int line, Qt::KeyboardModifiers)`：
    - 针对 `margin == 1` 判断当前行是否有 `MARKER_BREAKPOINT`，有则删除并触发信号，无则添加并触发信号；
  - 实现快捷键 `F9` 快速切换当前行的断点状态；
  - 实现右键菜单项【切换断点】、【清空所有断点】。

- [ ] **Step 3: 实现执行指针定位与停机行高亮滚动**
  - 在 `setExecutionLine(const QString &filePath, int line)` 中：
    - 确保目标文件打开并置于活动视口；
    - 清除旧执行指针标记，并在 `line - 1` 行设置 `MARKER_EXECUTION_POINTER`；
    - 调用 `editor->ensureLineVisible(line - 1)` 并设置光标位置；
  - 在 `clearExecutionLine()` 中移除所有打开编辑器内的 `MARKER_EXECUTION_POINTER`。

- [ ] **Step 4: 编译验证 CodeEditor 语法与边距修改**
  - 执行 `g++` / `qmake` 检查语法无报错，并确保同名符号高亮、粘性滚动等既有特性不受影响。

- [ ] **Step 5: Git 提交**
  - `git add PhudonTools/codeeditor.h PhudonTools/codeeditor.cpp`
  - `git commit -m "feat(editor): add breakpoint gutter margin and execution line pointer"`

---

### Task 2: 核心引擎引入与状态机总控中心 (`STM32DebugController`)

**Files:**
- Modify: `PhudonTools/PhudonTools.pro`
- Create: `PhudonTools/stm32debugcontroller.h`
- Create: `PhudonTools/stm32debugcontroller.cpp`

- [ ] **Step 1: 在 `PhudonTools.pro` 中引入 Core 引擎源码与头文件目录**
  - 增加对 `../Plugins/STM32WorkbenchPlugin/Core/` 的头文件搜索路径：
    `INCLUDEPATH += $$PWD/../Plugins/STM32WorkbenchPlugin/Core`
  - 引入核心源文件与头文件：
    - `ToolchainDetector.cpp/.h`
    - `STM32Profile.cpp/.h`
    - `MakefileGenerator.cpp/.h`
    - `BuildManager.cpp/.h`
    - `BuildDiagnostic.h`
    - `BuildLogModel.cpp/.h`
    - `FlashProgrammer.cpp/.h`
    - `FlashLogModel.cpp/.h`
    - `ProbeDetector.cpp/.h`
    - `GdbServerManager.cpp/.h`
    - `GdbMiEngine.cpp/.h`
  - 引入 CubeMX 链接脚本资源文件 `$$PWD/../Plugins/STM32WorkbenchPlugin/cubemx_linkers.qrc`。

- [ ] **Step 2: 创建 `stm32debugcontroller.h` 声明控制器接口**
  - 继承 `QObject`，设计单例或主窗口托管模型；
  - 状态枚举：
    ```cpp
    enum class SessionState {
        Idle,
        Building,
        Flashing,
        StartingDebug,
        Stopped,
        Running
    };
    ```
  - 核心属性与方法：
    - `void openProject(const QString &projectDir);`
    - `void closeProject();`
    - `void buildProject();`
    - `void cleanProject();`
    - `void flashProject();`
    - `void startDebug();`
    - `void stopDebug();`
    - `void continueDebug();`
    - `void stepOver();`
    - `void stepInto();`
    - `void stepOut();`
    - `void resetHalt();`
    - `void executeGdbCommand(const QString &cmd);`
    - `void toggleBreakpoint(const QString &file, int line);`
    - `void setTargetChip(const QString &chipName);`
    - `void setProbeType(const QString &probeName);`
  - 核心信号：
    - `void stateChanged(SessionState state);`
    - `void targetStopped(const QString &file, int line);`
    - `void targetRunning();`
    - `void stackFramesUpdated(const QJsonArray &frames);`
    - `void localsUpdated(const QJsonArray &locals);`
    - `void registersUpdated(const QMap<QString, QString> &regs);`
    - `void breakpointsUpdated(const QJsonArray &breakpoints);`
    - `void buildOutputReceived(const QString &text, bool isError);`
    - `void buildFinished(bool success, int errors, int warnings);`
    - `void gdbConsoleOutput(const QString &text);`

- [ ] **Step 3: 实现 `stm32debugcontroller.cpp`**
  - 初始化并持有 `ToolchainDetector`、`BuildManager`、`FlashProgrammer`、`GdbServerManager`、`GdbMiEngine`、`STM32ProfileManager`；
  - 实现工程配置智能探测与读写 `.smilecode/project.json`：
    - 智能扫描 `.ioc` / `.uvprojx` / 启动脚本推断芯片架构；
  - 实现异步构建流程，连接 `BuildManager::diagnosticParsed` 与 `buildFinished`；
  - 实现全异步非阻塞硬件调试会话生命周期：
    - `GdbServerManager` 启动 OpenOCD/J-Link 后，启动定时探活 `QTcpSocket`（端口 3333/2331）；
    - 探活成功后触发启动 `GdbMiEngine`，连接 `target remote`、`monitor reset halt`；
    - 全量同步当前断点集合至 GDB；
    - 连接 GDB/MI 的 `*stopped` 异步信号，分发给 UI 层的调用栈、局部变量与寄存器更新；
    - 处理优雅停止与进程资源无死锁回收。

- [ ] **Step 4: 编译检查控制器逻辑**
  - 运行 qmake 并进行语法级编译测试，确保各 Core 引擎通信顺畅。

- [ ] **Step 5: Git 提交**
  - `git add PhudonTools/PhudonTools.pro PhudonTools/stm32debugcontroller.h PhudonTools/stm32debugcontroller.cpp`
  - `git commit -m "feat(debug): implement STM32DebugController with async GDB/MI and build pipeline"`

---

### Task 3: 调试停靠面板与诊断视图矩阵 (Debugging Dock Widgets & Problems View)

**Files:**
- Create: `PhudonTools/stm32debugdocks.h`
- Create: `PhudonTools/stm32debugdocks.cpp`
- Modify: `PhudonTools/mainwindow.h`
- Modify: `PhudonTools/mainwindow.cpp`

- [ ] **Step 1: 在 `stm32debugdocks.h/.cpp` 中实现调用栈面板 (`CallStackDock`)**
  - 采用 `QTreeWidget` / `QTableWidget` 呈现调用栈层级：`Level`、`Function`、`File`、`Line`、`Address`；
  - 双击某一行栈帧，向控制器发送 `selectFrame(level)`，触发代码编辑器跳转至该帧源码。

- [ ] **Step 2: 实现局部变量与监视面板 (`VariablesDock`)**
  - Tab 1【局部变量 (Locals)】：表格展示当前函数的变量名、类型、当前值（支持 16 进制/10 进制切换）；与前次更新比对，数值变动的单元格以亮橙色字体高亮；
  - Tab 2【表达式监视 (Watch)】：支持输入自定义变量、指针或寄存器表达式（如 `*s_pBuffer`、`RCC->CR`），单步暂停时自动更新求值。

- [ ] **Step 3: 实现 CPU 寄存器面板 (`RegistersDock`) 与断点管理面板 (`BreakpointsDock`)**
  - `RegistersDock`：展示 ARM Cortex-M 核心寄存器（`R0` ~ `R12`、`SP`、`LR`、`PC`、`xPSR`），数值变动高亮；
  - `BreakpointsDock`：展示工程内所有断点列表，提供【禁用/启用】复选框、双击跳转源码行与一键清空断点。

- [ ] **Step 4: 在 `MainWindow` 中装配停靠区域与底栏选项卡**
  - 将 `CallStackDock`、`VariablesDock`、`RegistersDock`、`BreakpointsDock` 注册为主窗口标准 `QDockWidget`，停靠在右侧/底侧，并可在菜单【视图】->【调试视窗】自由折叠与展开；
  - 升级底栏 `m_tabWidget`：
    - 选项卡 1【编译输出 (Build Output)】：显示构建日志与 Flash/RAM 占用统计；
    - 选项卡 2【问题诊断 (Problems)】：列表展示编译器报错与警告条目，双击直接调用 `openFileAndGoTo`；
    - 选项卡 3【GDB 控制台 (Debug Console)】：支持用户直接在控制台输入 GDB 原生指令并接收回显流。

- [ ] **Step 5: Git 提交**
  - `git add PhudonTools/stm32debugdocks.h PhudonTools/stm32debugdocks.cpp PhudonTools/mainwindow.h PhudonTools/mainwindow.cpp`
  - `git commit -m "feat(ui): add professional debugging dock panels and diagnostics problems view"`

---

### Task 4: 主窗口动作、快捷键与编辑器全流程闭环打通 (Action Binding & Full Lifecycle Integration)

**Files:**
- Modify: `PhudonTools/mainwindow.h`
- Modify: `PhudonTools/mainwindow.cpp`

- [ ] **Step 1: 淘汰旧版临时实现并全面对接 `STM32DebugController`**
  - 移除原 `BuildSystem *m_buildSystem` 的旧硬编码调用；
  - 重写 `MainWindow::buildProject()`、`cleanProject()`、`flashProject()`、`startDebug()`、`stopDebug()`、`continueDebug()`、`stepOver()`、`stepInto()`、`stepOut()`、`setBreakpoint()` 对接控制器对应槽函数。

- [ ] **Step 2: 绑定全套专业快捷键与工具栏状态联动**
  - `F7` / `Ctrl+B`：构建工程
  - `Shift+F7`：清理工程
  - `Ctrl+F5`：固件烧录
  - `F5`：启动调试 / 继续运行
  - `Shift+F5`：停止调试
  - `Ctrl+Shift+F5`：复位停机
  - `F10`：单步跳过 (Next)
  - `F11`：单步进入 (Step)
  - `Shift+F11`：单步跳出 (Finish)
  - `F9`：切换当前行断点
  - 状态机驱动工具栏按钮状态更新：在 `Building` / `Debugging` 状态下精准使能/禁用对应按钮。

- [ ] **Step 3: 实现编辑器与调试器的双向联动**
  - `m_codeEditor` 触发 `breakpointToggled` 时，通知控制器向 GDB 同步 `-break-insert` 或 `-break-delete`；
  - 控制器触发 `targetStopped(file, line)` 时，调用 `m_codeEditor->setExecutionLine(file, line)`；
  - 控制器触发 `targetRunning()` 或 `stateChanged(Idle)` 时，调用 `m_codeEditor->clearExecutionLine()`；
  - 问题诊断面板双击条目时，调用 `m_codeEditor->openFileAndGoTo(file, line, col)`。

- [ ] **Step 4: 适配项目打开、芯片选择与配置保存**
  - 打开工程或切换文件时，自动加载 `.smilecode/project.json`，同步芯片下拉框与烧录器下拉框；
  - 更改芯片型号或下载工具时，自动更新项目配置并同步给控制器。

- [ ] **Step 5: Git 提交**
  - `git add PhudonTools/mainwindow.h PhudonTools/mainwindow.cpp`
  - `git commit -m "feat(workflow): wire editor breakpoints, debugging actions, and project persistence"`

---

### Task 5: 整体编译验证、端到端测试与质量走查 (Build & End-to-End Verification)

**Files:**
- All modified & created files

- [ ] **Step 1: 全量编译 PhudonTools**
  - 运行 `qmake PhudonTools.pro`
  - 执行 `mingw32-make -j4`
  - 修复任何编译警告或链接错误，确保一次性干净通过。

- [ ] **Step 2: 验证工程识别与构建**
  - 打开标准 STM32 示例工程，验证 `.smilecode/project.json` 自动生成；
  - 点击【编译 (F7)】，验证增量 Makefile 生成、编译日志输出与 Flash/RAM 资源占用条呈现；
  - 故意制造语法错误，验证底栏【问题诊断】列表是否出现错误条目，双击是否准确定位到源码行和列。

- [ ] **Step 3: 验证断点与调试流程**
  - 在源码行号槽左侧点击，验证红点断点显示与取消；按 `F9` 验证当前行断点快捷切换；
  - 启动调试（F5），验证后台 Server 探活、GDB 启动握手与在 `main()` 暂停；
  - 验证黄色执行箭头 (➜) 指向当前行，视口居中；
  - 验证【调用栈】、【局部变量】、【CPU 寄存器】面板实时刷新数据；
  - 单步执行（F10/F11），验证执行箭头移动、数值变色高亮；
  - 停止调试（Shift+F5），验证黄色箭头清除，后台 OpenOCD/GDB 进程干净退出无残留。

- [ ] **Step 4: 检查非破坏性与回归安全**
  - 确保串口示波器、CAN 调试助手、IAP 工具、AppHub 应用中心等既有功能 100% 正常运行无异常。

- [ ] **Step 5: 最终 Git 提交**
  - `git add .`
  - `git commit -m "feat(stm32): complete native integration of STM32 build and GDB debug workflow in SmileCodeIDE"`
