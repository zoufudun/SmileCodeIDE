# 完整的STM32嵌入式开发编译系统

## TL;DR

> **快速摘要**: 基于现有BuildSystem类，扩展为完整的STM32开发编译系统，支持多MCU系列(G0/G1/F0/F1/F2/F3/F4/F7/H7)、GCC错误解析与编辑器集成、烧录与GDB调试、工具链管理。

> **交付物**:
> - MCU配置JSON模板 (G0/G1/F0/F1/F2/F3/F4/F7/H7)
> - 扩展的BuildSystem类 (多MCU支持、错误解析、进度跟踪)
> - 烧录管理器 (OpenOCD/ST-Link集成)
> - GDB调试面板 (变量/断点/调用栈)
> - 工具链管理器 (版本检测/自动下载引导)
> - 编辑器错误标记集成

> **估算工作量**: XL (大型)
> **并行执行**: YES - 多Wave
> **关键路径**: Wave 1基础 → Wave 2编译增强 → Wave 3烧录调试 → Wave 4集成验证

---

## Context

### 原始需求
用户要求实现一个"完整"的STM32嵌入式开发编译系统，需要支持：
- 全系列MCU (F0/F1/F2/F3/F4/F7/H7)
- GCC错误解析 + 跳转到错误行
- 编辑器错误标记
- 编译优化级别配置
- 编译进度指示
- 头文件依赖分析
- 烧录 + GDB调试
- 工具链版本检测 + 自动下载

### 访谈摘要

**关键讨论**:
- MCU支持: 通过JSON配置文件定义每个系列的编译参数、内存布局、链接脚本模板
- GDB调试UI: 集成面板（底部/侧边停靠），类似VS Code风格
- MCU配置存储: JSON配置文件，易于扩展和用户自定义
- 测试策略: 无单元测试，仅Agent QA场景验证

**研究结果**:
- 现有BuildSystem: 仅支持F1，Makefile生成可用，QProcess执行编译
- ToolchainDialog: QSettings持久化路径，无版本验证
- TerminalWidget: 独立Shell，不用于编译输出
- CodeEditor: 有高亮/补全/彩虹括号，无错误标记
- MainWindow: 有构建/清理按钮，连接到BuildSystem

### Metis审查

**识别的差距** (已处理):
- GDB集成需要处理复杂的进程间通信和MI模式解析
- 错误解析需要处理多种GCC输出格式变体
- JSON配置需要版本控制和向后兼容

---

## Work Objectives

### 核心目标
扩展现有BuildSystem类，实现完整的STM32开发编译、烧录、调试功能，集成到Qt桌面IDE中。

### 具体交付物
- [ ] MCU系列JSON配置文件 (resources/mcu-profiles/*.json)
- [ ] 扩展的BuildSystem类，支持多MCU、错误解析、进度跟踪
- [ ] FlashManager类，实现OpenOCD/ST-Link烧录
- [ ] GDBDebugger类，实现GDB远程调试
- [ ] GDB调试面板UI (变量/断点/调用栈)
- [ ] 工具链管理器 (版本检测、自动下载引导)
- [ ] CodeEditor错误标记集成 (QScintilla标记)
- [ ] 编译输出窗口错误点击跳转

### Definition of Done
- [ ] 可编译一个STM32F4项目并成功生成 .elf/.hex/.bin
- [ ] 点击编译错误可跳转到对应代码行
- [ ] 编辑器显示编译错误标记
- [ ] 可通过OpenOCD烧录固件到开发板
- [ ] GDB可连接并设置断点、单步执行
- [ ] 工具链未安装时显示下载引导

### Must Have
- 多MCU系列支持 (至少F1/F4)
- 编译功能完整可用
- 烧录功能可用
- GDB调试基础可用

### Must NOT Have (Guardrails)
- 不实现RTOS调试 (FreeRTOS等)
- 不实现CubeMX项目导入
- 不实现外设寄存器可视化配置
- 不实现实时数据可视化/示波器

---

## Verification Strategy

> **零人工干预** — 所有验证通过Agent执行。禁止需要人工手动测试的验收标准。

### Test Decision
- **基础设施存在**: NO
- **自动化测试**: NO (用户选择)
- **框架**: N/A
- **Agent QA**: YES (每个任务强制包含)

### QA Policy

每个任务必须包含Agent执行QA场景，证据保存到 `.sisyphus/evidence/`。

- **编译系统**: Bash运行qmake + make，验证退出码
- **UI交互**: Qt测试工具或手动验证窗口可打开
- **进程通信**: 验证QProcess正确启动/终止
- **JSON配置**: 验证JSON可解析且结构正确

---

## Execution Strategy

### 并行执行Wave

```
Wave 1 (立即启动 — 基础架构 + 配置):
├── Task 1: MCU配置JSON模板定义
├── Task 2: MCUProfileManager类
├── Task 3: 扩展BuildSystem基类支持多MCU
├── Task 4: 链接脚本模板系统
└── Task 5: 工具链版本检测模块

Wave 2 (Wave 1后 — 编译增强):
├── Task 6: GCC错误解析器
├── Task 7: 编译进度跟踪
├── Task 8: 优化级别配置UI
├── Task 9: 头文件依赖分析
└── Task 10: 编译输出窗口增强

Wave 3 (Wave 2后 — 烧录系统):
├── Task 11: FlashManager基类
├── Task 12: OpenOCD烧录实现
├── Task 13: ST-Link烧录实现
└── Task 14: 烧录UI (一键烧录按钮)

Wave 4 (Wave 3后 — GDB调试):
├── Task 15: GDBDebugger类 (MI模式)
├── Task 16: 断点管理
├── Task 17: 变量监视面板
├── Task 18: 调用栈面板
└── Task 19: GDB调试工具栏

Wave 5 (Wave 4后 — 编辑器集成):
├── Task 20: CodeEditor错误标记API
├── Task 21: 编译错误点击跳转
└── Task 22: 错误标记刷新机制

Wave 6 (Wave 5后 — 工具链管理增强):
├── Task 23: 工具链自动下载引导
└── Task 24: 工具链设置UI增强

Wave FINAL (全部后 — 验证):
├── Task F1: 端到端编译测试 (F4项目)
├── Task F2: 烧录功能验证
├── Task F3: GDB调试验证
└── Task F4: UI集成验证
```

### 依赖矩阵

- **1-5**: — — 6-10
- **6-10**: 1-5 — 11-16, 20-22
- **11-16**: 6-10 — 17-19, 23
- **17-19**: 11-16 — F
- **20-22**: 6-10 — F
- **23-24**: 5 — F

---

## TODOs

- [x] 1. **MCU配置JSON模板定义**

  **What to do**: 在 resources/mcu-profiles/ 创建G0-G7/H7的JSON配置
  - 每个JSON: cpu内核、编译flags、内存布局、链接脚本模板

  **Must NOT do**: 不创建不使用的配置文件

  **Recommended Agent Profile**: unspecified-high (JSON结构设计)

  **Parallelization**: Wave 1 (并行1-5)
  **Blocks**: 6-10 | **Blocked By**: None

  **References**: buildsystem.cpp:368-372 (现有参数), :446-587 (链接脚本)

  **Acceptance Criteria**:
  - [x] resources/mcu-profiles/f1.json, f4.json 创建
  - [x] QJson可解析 (创建了f0.json, f1.json, f4.json, h7.json及index.json)
  - [ ] resources/mcu-profiles/f1.json, f4.json 创建
  - [ ] QJson可解析

  **Commit**: NO

---

- [ ] 2. **MCUProfileManager类**

  **What to do**: mcuprofilemanager.h/cpp - 从JSON加载配置
  - getProfile(), getProfilesBySeries()接口

  **Acceptance Criteria**: [ ] getProfile("STM32F407")返回配置
  **Commit**: NO

---

- [ ] 3. **扩展BuildSystem支持多MCU**

  **What to do**: 修改buildsystem.h/cpp
  - setMcuProfile()方法
  - generateMakefile()使用配置参数

  **Must NOT do**: 不破坏现有F1编译

  **Acceptance Criteria**: [ ] F4生成-mcpu=cortex-m4
  **Commit**: YES - feat(build): multi-MCU support

---

- [ ] 4. **链接脚本模板系统**

  **What to do**: 从JSON读FLASH/RAM，生成.ld
  **Must NOT do**: 不覆盖用户自定义链接脚本
  **Acceptance Criteria**: [ ] F1/F4不同内存参数
  **Commit**: NO

---

- [ ] 5. **工具链版本检测模块**

  **What to do**: toolchainmanager.h/cpp
  - detectToolchain()检测版本

  **Must NOT do**: 不自动下载
  **Acceptance Criteria**: [ ] 返回版本信息或错误
  **Commit**: NO

---

## Wave 2: 编译增强 (6-10)

- [ ] 6. **GCC错误解析器** - errorparser.h/cpp，正则解析file:line:col error/warning
- [ ] 7. **编译进度跟踪** - 解析make输出，发送progress信号
- [ ] 8. **优化级别配置UI** - O0/O1/O2/Os/Og选项，QSettings持久化
- [ ] 9. **头文件依赖分析** - 解析.d文件，仅重新编译受影响文件
- [ ] 10. **编译输出窗口增强** - 可点击错误，双击跳转

---

## Wave 3: 烧录系统 (11-14)

- [ ] 11. **FlashManager基类** - 抽象基类IFlasher，startFlash/stopFlash
- [ ] 12. **OpenOCD烧录** - OpenOCDAdapter，调用openocd.exe
- [ ] 13. **ST-Link烧录** - STLinkAdapter，使用st-flash
- [ ] 14. **烧录UI** - 一键烧录下拉菜单，进度条

---

## Wave 4: GDB调试 (15-19)

- [ ] 15. **GDBDebugger类(MI模式)** - gdb -i mi，sendCommand MI命令
- [ ] 16. **断点管理** - addBreakpoint/removeBreakpoint/listBreakpoints
- [ ] 17. **变量监视面板** - VariablesPanel，QTreeWidget显示变量
- [ ] 18. **调用栈面板** - CallStackPanel，双击跳转源码
- [ ] 19. **GDB调试工具栏** - 开始/停止/继续/单步按钮

---

## Wave 5: 编辑器集成 (20-22)

- [ ] 20. **CodeEditor错误标记API** - addErrorMarker/clearMarkers，QsciScintilla markerAdd
- [ ] 21. **编译错误点击跳转** - QTextCursor跳转到指定行
- [ ] 22. **错误标记刷新机制** - 编译前clear，编译后添加

---

## Wave 6: 工具链管理 (23-24)

- [ ] 23. **工具链自动下载引导** - 未检测到时显示下载链接
- [ ] 24. **工具链设置UI增强** - 显示版本号和完整性状态

---

## Final Verification Wave

- [ ] F1. **端到端编译测试** — 创建一个F4测试项目，验证完整编译流程
- [ ] F2. **烧录功能验证** — 验证OpenOCD/ST-Link烧录流程
- [ ] F3. **GDB调试验证** — 验证GDB连接和基本调试功能
- [ ] F4. **UI集成验证** — 验证所有UI组件正确集成到MainWindow

---

## Commit Strategy

- **Wave 1**: `feat(build): add MCU profile system and multi-MCU support` — buildsystem.h/cpp, mcu-profiles/
- **Wave 2**: `feat(build): add error parsing and progress tracking` — errorparser.*, buildsystem enhancements
- **Wave 3**: `feat(flash): add flash manager with OpenOCD/ST-Link` — flashmanager.*, flash.ui
- **Wave 4**: `feat(debug): add GDB debugger integration` — gdbdebugger.*, debugpanels.ui
- **Wave 5**: `feat(editor): add error markers to code editor` — codeeditor enhancements
- **Wave 6**: `feat(toolchain): add toolchain manager` — toolchainmanager.*
- **Final**: `test: integrate and verify all features`

---

## Success Criteria

### Verification Commands
```bash
# 编译测试
qmake STM32IDE.pro
make -j4
# 预期: 编译成功，无错误

# 启动测试
./release/STM32IDE.exe
# 预期: 主窗口正常显示，编译按钮可点击
```

### Final Checklist
- [ ] 所有Must Have功能存在
- [ ] 所有Must NOT Have功能不存在
- [ ] 编译成功无错误
- [ ] UI组件正确集成
