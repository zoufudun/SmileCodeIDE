# Keil Project Navigation Implementation Plan

> **For agentic workers:** Execute inline with focused review after implementation; use QtTest for behavior verification.

**Goal:** Import original uvprojx projects and browse Keil groups and per-file functions.

**Architecture:** A read-only ordered Keil model feeds a dedicated navigator; MainWindow integrates import and persistence. Reuse the existing symbol parser via a static pure interface, with lazy symbol population and local include discovery.

**Tech Stack:** Qt 6 C++17, QXmlStreamReader, QTreeView/QStandardItemModel, QFileSystemWatcher, QtTest, qmake.

---

### Task 1: Ordered project model
- [x] Add `PhudonTools/tests/KeilProjectTest.cpp` and `.pro`: relative paths, targets/groups/order, invalid XML, missing files, sample project.
- [x] Run tests red, implement `PhudonTools/keilproject.h/.cpp`, rerun green.

### Task 2: Navigator and symbols
- [x] Test `PhudonTools/projectnavigator.h/.cpp`: project tree, sorted files, lazy symbols, navigation, refresh/clear.
- [x] Expose existing `codeeditor` parser without widget creation; test definitions, prototypes and false positives.
- [x] Implement tabs, missing-file indicators, buffered source lookup, local header dependency closure, watcher and refresh.

### Task 3: MainWindow integration
- [x] Add tests to existing workflow suite for import, restore, normal directory transition and failed import preservation.
- [x] Add menu/toolbar action and invokable import method; persist original uvprojx path and propagate directory root to editor/search/debug context.
- [x] Guard filesystem actions from virtual nodes; reset navigator when closing or opening ordinary directories.
- [x] Register sources in `PhudonTools.pro`; compile main app with installed matching Qt/compiler.

### Task 4: Verify and document
- [x] Run new and existing relevant QtTest binaries; inspect app/navigation layout.
- [x] Request focused code review and resolve material issues.
- [x] Archive implementation notes and verification results in `docs/Keil工程导入与项目函数导航.md`.

## 完成验证
- Qt6.11.1 / MinGW13.1 POSIX 主程序构建退出码 0，部署本地 Qt 运行库完成。
- KeilProjectTest 12/12；MainWindowWorkflowTest 12/12；EditorCommandsTest 13/13。
- 实际 Fire_H7 工程解析与截图通过，代码审查无未解决实质问题。
