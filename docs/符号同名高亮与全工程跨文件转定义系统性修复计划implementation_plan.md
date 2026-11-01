# 符号同名高亮与全工程跨文件转定义系统性修复计划

解决代码编辑器在符号同名高亮（Highlight Occurrences）和全工程跨文件转定义（Jump to Definition）中存在的失效、遮挡、漏判与偏移问题。

## 核心问题与根因分析

经过对 [`codeeditor.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)、[`codeeditor.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.h) 与底层 QScintilla 机制的排查，问题根源集中在以下 5 个核心技术点：

1. **预处理代码清洗（`generateCleanCppCode`）将宏定义整行置空**：
   - 导致宏符号（`#define FOO`）在同名高亮中被过滤，跨文件转定义对宏符号 100% 匹配失败。
2. **指示器绘制在底层被活动行背景覆写（`setIndicatorDrawUnder(true)`）**：
   - Scintilla 在当前行绘制活动行背景（Caret Line Background）时，直接盖住了底层的半透明圆角矩形指示器，导致光标所在行高亮完全不可见。
3. **扫描算法在循环中执行 $O(N^2)$ 子串统计，引发界面严重掉帧与定时器重置**：
   - 每次匹配都调用 `code.left(absOffset).count('\n')`，大文件单次光标移动耗费数百兆内存拷贝。
4. **跨文件转定义硬编码跳过头文件内联实现（`if (!isHeader)`）**：
   - 导致写在 `.h`/`.hpp` 中的内联函数、模板函数、单头文件库实现无法被转定义发现。
5. **类名上下文提取（`extractEnclosingClassName`）在 `.cpp` 文件中失效，且使用 `indexOf` 导致坐标漂移**：
   - `.cpp` 中无 `class` 关键字导致上下文丢失；正则使用模糊 `indexOf` 易误中类名前缀或返回值。

---

## Proposed Changes

### 代码编辑器核心模块 (`PhudonTools`)

#### [MODIFY] [`codeeditor.h`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.h)
- 增加高效行起始索引构建与行号二分查找辅助方法声明。
- 增强 `extractEnclosingClassName` 支持从 `.cpp` 的 `Class::func` 或当前作用域大纲中智能提取所属类名。

#### [MODIFY] [`codeeditor.cpp`](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)
1. **重构预处理清洗 `generateCleanCppCode`**：
   - 保留 `#define` 及其宏名称，仅清洗 `#include`、`#pragma` 等无关指令，保留代码结构与宏定义可读性。
2. **修正指示器渲染与图层配置**：
   - 设置 `setIndicatorDrawUnder(false, OCCURRENCE_INDICATOR)`，保证高亮绘制在文本图层上方；
   - 提升指示器轮廓与填充对比度（Dark: `#89B4FA`，Light: `#2563EB`，Alpha: 70，OutlineAlpha: 255）；
   - 使用 Scintilla 原生消息 `SCI_INDICATORCLEARRANGE` 全文范围清空，消除清空边界残留。
3. **优化同名高亮性能至 $O(N)$**：
   - 预构建 `lineStarts` 偏移表，通过二分查找 `std::upper_bound` 毫秒级计算行号列号，彻底消除大文件卡顿。
4. **修正语法样式与词尾判断**：
   - 触发 `SCI_COLOURISE`，增加光标停留在词尾时的回退探测与 `style & 0x3F` 掩码过滤。
5. **升级全工程跨文件转定义搜索引擎（`findSymbolDefinitionsInProject`）**：
   - 规则 1（类方法实现）：使用正则捕获组精确偏移 `m.capturedStart(2)`，废除不安全的 `indexOf`；
   - 规则 2（普通/全局/内联函数实现）：移除 `if (!isHeader)` 限制，全面支持头文件函数实现与模板实现；
   - 规则 4（宏定义）：激活针对 `#define` 的全工程扫描并赋高分权重（950 分）；
   - 规则 7（所属类名智能推导）：在 `.cpp` 文件中从当前函数名（`ClassName::Method`）提取所属类。
6. **防穿透与直接精准跳转优化**：
   - 当光标已经在当前局部变量/形参定义处时，不向全工程盲目穿透跳转；
   - 遇到唯一/高置信度候选直接平滑跳转，多个同分候选弹出选择列表。

---

## Verification Plan

### Automated Tests / Compilation
- 使用 Mingw64 工具链执行构建检查：
  ```powershell
  $env:PATH = "D:\Soft\Qt\5.15.2\mingw81_64\bin;D:\Soft\Qt\Tools\mingw810_64\bin;$env:PATH"
  D:\Soft\Qt\Tools\mingw810_64\bin\mingw32-make.exe -f Makefile.Release
  ```
- 确认编译无错误、无警告引入。

### Manual Verification
1. **符号同名高亮验证**：
   - 打开包含局部变量、全局变量、类成员函数、以及 `#define` 宏定义的文件；
   - 单击符号（如 `uint8_t`, `m_currentEditor`, `#define` 宏名），观察当前行及全文所有同名符号是否立即呈现清晰圆角高亮框；
   - 光标移至空白处，高亮立即干净清除；
   - 测试大文件（数千行），光标移动丝滑无卡顿。
2. **全工程跨文件转定义验证**：
   - **宏定义跳转**：在任意 `.c`/`.cpp`/`.h` 中按 F12 跳转至 `#define` 宏定义处；
   - **头文件函数跳转**：在 `ringbuffer.h` 或 `roomwidget.h` 中按 F12 跳转至头文件内联实现；
   - **.h 声明转 .cpp 实现**：在头文件方法声明处按 F12，直接跳转到 `.cpp` 中的函数实现体；
   - **.cpp 实现转 .h 声明**：在 `.cpp` 方法处按 F12，跳至头文件声明；
   - **局部变量定义**：在函数体内变量按 F12，平滑滚动至本函数首部变量声明行；在定义行按 F12 不会误跳到外部文件。
