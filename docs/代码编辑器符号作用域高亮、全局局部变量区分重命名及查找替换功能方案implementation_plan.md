# 代码编辑器符号作用域高亮、全局/局部变量区分重命名及查找替换功能方案

本方案旨在为代码编辑器（`CodeEditor`）引入现代 IDE（类似 VS Code）的核心交互体验：
1. **符号同名高亮（Occurrence Highlight）**：鼠标光标停留在或选中文档中的变量名/函数名时，自动精准高亮当前文件中所有使用该名称的位置。
2. **符号作用域严格隔离（Scope-Aware Analysis）**：能够精准判定符号是**当前函数的局部变量/形参**还是**全局变量/类成员/全局函数**，严禁混淆同名的全局变量与局部变量，亦不混淆不同函数内的同名局部变量。
3. **现代查找与替换浮动栏（Find & Replace Quick Bar）**：在编辑器右上角集成现代极简风格的查找替换栏，支持大小写匹配、全字匹配、正则搜索、上下跳转、单处替换、全部替换及快捷键（`Ctrl+F` / `Ctrl+H` / `F3` / `Esc`）。
4. **符号一键批量重命名（Rename Symbol / F2）**：支持快捷键 `F2` 或右键重命名，根据作用域仅修改同一作用域内的所有同名变量/函数名，支持一次性撤销（Undo）。

---

## 🏗️ 架构与核心设计

```mermaid
graph TD
    A[用户在编辑器中选中或将光标置于符号] --> B[ScopeAnalyzer: 作用域分析器]
    B --> C{判定符号作用域属性}
    C -->|局部变量/函数形参| D[限制在当前函数作用域区间 [Start, End]]
    C -->|全局变量/类成员/全局函数| E[作用域为全局范围 (排除局部同名遮蔽变量)]
    D --> F[高亮渲染: QScintilla Occurrence Indicator (ID 27)]
    E --> F
    F --> G[状态显示: 面包屑与状态栏提示引用数]

    H[快捷键 F2 / 右键重命名] --> I[弹出 RenameSymbolDialog 重命名气泡]
    I --> J[提示当前符号类型、作用域与引用计数]
    J --> K[用户输入新名称并确认]
    K --> L[QScintilla 事务批量替换 (BeginUndoAction/EndUndoAction)]

    M[快捷键 Ctrl+F / Ctrl+H] --> N[FindReplaceWidget: 右上角浮动查找替换栏]
    N --> O[支持大小写/全词/正则匹配，上下跳转与全局替换]
```

---

## 🛠️ 拟修改与新增模块

### 1. `PhudonTools/codeeditor.h`
- 增加 `OCCURRENCE_INDICATOR = 27` 指示器常量（柔和矩形半透明高亮盒）。
- 声明作用域信息结构体 `SymbolScopeInfo`：
  ```cpp
  enum ScopeKind {
    ScopeLocalVariable,   // 函数内部局部变量
    ScopeFunctionParam,   // 函数形参
    ScopeMemberVariable,  // 类成员变量
    ScopeGlobalVariable,  // 全局变量
    ScopeFunction,        // 函数名/方法名
    ScopeType             // 类/结构体/类型名
  };

  struct SymbolOccurrence {
    int startOffset;
    int endOffset;
    int line;
    int col;
    int length;
  };

  struct SymbolScopeResult {
    QString symbolName;
    ScopeKind kind;
    QString scopeOwner; // 所属函数名或类名
    int scopeStartLine;
    int scopeEndLine;
    QList<SymbolOccurrence> occurrences;
  };
  ```
- 声明核心方法：
  - `SymbolScopeResult analyzeSymbolScopeAt(int line, int col);`
  - `void highlightOccurrences(const SymbolScopeResult &result);`
  - `void clearOccurrenceHighlights();`
  - `void renameSymbolInScope(const SymbolScopeResult &scope, const QString &newName);`
  - `void showFindReplaceBar(bool showReplace = false);`

### 2. `PhudonTools/codeeditor.cpp`
- **作用域解析引擎 (`analyzeSymbolScopeAt`)**：
  1. 获取光标处的完整单词/标识符及位置。
  2. 利用 `parseFunctions` 已提取的函数体区间（`startLine` ~ `endLine`），判定光标当前位于哪个函数内。
  3. 若位于函数内，在掩码纯净代码 `clean` 的该函数体内扫描形参列表 `(...)` 及局部变量定义语句（如 `int x;`, `QString x = ...;`, `for (int x = ...)` 等）。
  4. 若在函数内发现该符号的定义，则判定为 `ScopeLocalVariable` 或 `ScopeFunctionParam`，搜索范围严格限定在该函数的大括号区间 `[bodyStart, bodyEnd]` 内；
  5. 若函数内未定义该符号，则向外检索类成员与全局变量，判定为 `ScopeGlobalVariable` / `ScopeFunction`，并在全局范围内查找，同时在遍历其他函数时跳过那些有同名局部变量遮蔽的函数体。
  6. 严格过滤注释和字符串内的同名词汇。
- **高亮渲染与指示器管理**：
  - 在 `setupEditor` 中配置 Indicator 27 为 `QsciScintilla::StraightBoxIndicator` 或 `BoxIndicator`，配色为主题适配的暗紫/青灰半透明色。
  - 使用防抖定时器 `m_occurrenceTimer`（150ms），在光标移动或选择变化时平滑刷新高亮，避免频繁重绘导致卡顿。
- **符号重命名模块 (`renameSymbolInScope`)**：
  - 使用 `SendScintilla(QsciScintilla::SCI_BEGINUNDOACTION)` 开启原子撤销事务。
  - 从后向前逆序替换所有 `SymbolOccurrence`，确保字符偏移不发生错位。
  - 结束事务并自动更新函数大纲与面包屑。
- **现代极简查找替换栏 (`FindReplaceWidget`)**：
  - 继承自 `QWidget`，悬浮于编辑器右上角（带优雅圆角与阴影背景）。
  - 上行：查找输入框、结果计数（如 `1 of 5`）、大小写敏感 `Aa`、全词匹配 `\b`、正则表达式 `.*`、上一个 `▲`、下一个 `▼`、关闭 `✕`。
  - 下行（折叠/展开）：替换输入框、替换单个 `Replace`、全部替换 `Replace All`。
  - 绑定快捷键 `Ctrl+F`、`Ctrl+H`、`F3`、`Shift+F3`、`Esc`。

---

## 🧪 验证计划

### 1. 作用域隔离验证（全局 vs 局部变量）
- 创建包含同名全局变量与局部变量的代码：
  ```cpp
  int count = 0; // 全局变量

  void funcA() {
      int count = 10; // 局部变量 A
      count++;
  }

  void funcB() {
      count = 5; // 使用全局变量
  }

  void funcC() {
      int count = 20; // 局部变量 C
      count += 2;
  }
  ```
- **测试用例 1**：光标置于 `funcA` 内部的 `count` 时：
  - 验证仅高亮 `funcA` 内部的 2 处 `count`。
  - 执行 F2 重命名为 `localCountA`，验证只有 `funcA` 内被替换，全局 `count`、`funcB` 和 `funcC` 的 `count` 保持不变。
- **测试用例 2**：光标置于 `funcB` 内部的 `count` 时：
  - 验证识别为全局变量，高亮第 1 行全局声明和 `funcB` 中的使用，而 `funcA` 与 `funcC` 的同名局部变量被正确隔离。

### 2. 查找替换栏交互验证
- 按 `Ctrl+F` 调出查找栏，选中的文字自动填入查找框。
- 测试大小写敏感、全字匹配、正则模式的准确性。
- 测试按回车/上一步下一步跳转定位。
- 按 `Ctrl+H` 展开替换行，测试单个替换与一键全部替换功能。
- 按 `Esc` 关闭查找栏并将焦点还给编辑器。

### 3. 构建验证
- 使用 MinGW 64 位 Qt 编译器对 `PhudonTools` 进行全量编译，验证 0 错误、0 警告。
