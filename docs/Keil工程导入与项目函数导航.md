# Keil 工程导入与项目函数导航

## 使用

1. 启动代码编辑器，选择 **文件 → 导入 Keil 工程…**，或点击工具栏同名按钮。
2. 选择原始 `.uvprojx` 文件。
3. 左侧 **项目** 页按工程、Target、Keil Group、文件排列，保留名称和原始顺序。展开 C/C++ 文件可查看函数。
4. **函数** 页汇总工程源文件及可解析的本地依赖头文件，按文件名排序；展开后显示函数名和参数，双击文件或函数打开源码并定位。
5. Keil 更新工程后自动重新读取，也可点击 **刷新 Keil 工程**。关闭工程、切换回普通文件夹、下次启动恢复均支持。

## 实现

- `PhudonTools/keilproject.h/.cpp`：读取 Project/Targets/Groups/Files，有序保留多个 Target。相对路径以 uvprojx 所在目录解析，兼容中文、反斜杠和工程外部文件。合并文件、分组、Target 层级 include 路径，不丢失顺序。
- `PhudonTools/projectnavigator.h/.cpp`：独立只读导航器，项目/函数双视图，函数延迟加载。本地头文件索引按小批次执行，循环依赖和重复文件去重，同名文件保留各自路径提示。
- 复用 CodeEditor 的静态 `symbolsForCode(text, path)`，不创建隐藏编辑器，不改变当前文件大纲。头文件支持函数声明和 inline 定义。
- 编辑器未保存文本优先；监听文本与保存点状态，覆盖撤销/重做。干净文件索引以磁盘为准，跳转时同步加载变化的内容；内容相同时保留撤销和重做历史。
- 文件与父目录同时监听，覆盖原子保存和删除后重建。XML 错误保留上次成功结构。配置哈希不变时保留项目树，刷新当前函数；每次索引重算监听集合，移除已不属于工程的路径。
- 文件和函数通过同一个安全导航入口跳转；保存、放弃和取消只询问一次。虚拟分组没有文件系统重命名或删除操作。缺失文件保留警告节点。
- 主窗口持久化原始工程文件路径。设置遵循 `QSettings::defaultFormat()`，生产默认仍为原生格式，测试使用隔离 INI。

## 边界

原 `.uvprojx` 不转换、不改写。IDE 沿用现有 `.smilecode/project.json` 侧车配置机制；保存源码仍由用户的保存操作触发。此次功能不转换 ARMCC/ARMASM 选项，不保证 Keil 专用工程由 GCC 直接构建。

函数索引基于现有文本符号解析器，不执行条件编译。依赖头文件从引用文件所在目录及工程配置中的本地 include 路径寻找；未安装的编译器标准库、未展开的 Keil Pack/环境宏路径不自动补全。

## 验证与产物

构建使用 Qt 6.11.1 与配套 MinGW 13.1 POSIX。默认环境的 MinGW 15.2 Win32 运行库与 Qt 的 POSIX 运行库不匹配，QtTest Watchdog 可因此死锁；已通过线程栈定位并改用匹配工具链。

测试覆盖：多 Target、分组顺序、中文相对路径、缺失文件、无效 XML、函数与声明识别、调用与注释过滤、本地递归 include、跳转行号、缓冲刷新、原子保存与重建、监听清理、新建文档、撤销/重做、原文件保持、导入恢复和普通目录切换。

- Keil 功能测试：`build/keil-tests/results.txt`。
- 主窗口回归：`build/keil-workflow/results.txt`。
- 编辑器回归：`build/keil-editor/results-native.txt`。其中 Doxygen 模板测试依赖原生注册表，需在允许访问注册表的环境运行，并在测试内恢复配置。
- 本地可运行程序及 Qt 运行库：`build/keil-app/release/PhudonTools.exe`。
- Fire_H7 实际样本截图：`build/keil-tests/artifacts/keil-project.png`、`keil-functions.png`。

最终结果：主程序构建退出码 0；Keil 功能 12/12、主窗口 12/12、编辑器 13/13，共 37 项通过。静态代码审查通过。可运行目录启动冒烟验证通过。测试日志保留既有样式与唯一连接警告；本次未作无关样式修改。
