# 函数列表显示与高精符号解析及跳转优化方案

本方案针对函数大纲列表中函数的显示格式、点击精准跳转高亮、符号类型（函数、变量、宏定义、类、结构体、联合体、枚举）的准确识别与图标区分，以及头文件全符号（宏定义、结构体、联合体、枚举、函数原型、外部变量）的高精解析进行全面升级。

## 用户需求对应

1. **显示格式**：函数显示为 `函数名称 返回类型（参数类型）`；变量显示为 `变量名称 类型`；宏定义显示为 `宏名称 #define 参数/值`；结构体/枚举/类显示为 `名称 struct/enum/class`；右侧显示出现频次/引用数（如 `+9`, `6`, `2`, `1`）。
2. **准确跳转与高亮**：点击列表项时，精准定位到实际定义/声明中**函数名所在的具体行与列**，并高亮选中函数名，将视口平滑滚动至舒适阅读位置（1/3 处）。
3. **符号类型准确分类与矢量图标**：
   - 💜 **函数 (Function)**：等轴测 3D 紫色线框立方体图标 (`#A855F7`)
   - 🩵 **变量 (Variable)**：青色芯片/变量圆孔圆角方块图标 (`#06B6D4`)
   - 💛 **宏定义 (Macro)**：金黄色 `#` 方形徽标 (`#F59E0B`)
   - 💙 **类 (Class)**：深蓝/靛蓝色 `C` 字母徽章图标 (`#3B82F6`)
   - 💚 **结构体 (Struct)**：翡翠绿 `S` 字母/多节点徽章图标 (`#10B981`)
   - 🧡 **联合体 (Union)**：橙色 `U` 字母徽章图标 (`#F97316`)
   - 💜 **枚举 (Enum)**：浅紫 `E` 字母/枚举条目徽章图标 (`#8B5CF6`)
4. **头文件与源文件全符号支持**：
   - 宏定义（包含单行宏、带参函数宏、多行反斜杠宏）
   - 结构体（`struct Name`, `typedef struct { ... } Alias;`, `typedef struct Tag Alias;`）
   - 联合体（`union Name`, `typedef union { ... } Alias;`）
   - 枚举（`enum Name`, `enum class Name`, `typedef enum { ... } Alias;`）
   - 类（`class Name`, `class Name : public Base`）
   - 函数原型/声明（如 `void Delay(int ms);`, `extern void NVIC_Configuration(void);`）
   - 外部变量/全局变量（如 `extern UART_HandleTypeDef Uart2_Handle;`, `extern volatile int uart_p;`, `extern uint8_t uart_buff[1024];`）

---

## 拟修改文件

### [PhudonTools/codeeditor.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.h)
- 扩展 `SymbolType` 枚举：增加 `SymbolClass`, `SymbolStruct`, `SymbolUnion`, `SymbolEnum`。
- 在 `FunctionInfo` 中增加 `refCount` 字段，用于存储符号在文件中的引用频次。
- 声明各符号类型的矢量图标绘制函数及自定义列表委托类 `SymbolListDelegate`。

### [PhudonTools/codeeditor.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/codeeditor.cpp)
- **矢量图标绘制器**：补充实现 `createClassIcon`, `createStructIcon`, `createUnionIcon`, `createEnumIcon`，并优化 `createVariableIcon`、`createSymbolIcon`，使其与截图视觉效果 1:1 对齐。
- **参数类型清洗与提取器**：实现 `normalizeParamTypes`，将形参签名（如 `const QColor &color = QColor("#A855F7"), int size = 18`）精准清洗为纯参数类型 `(const QColor &, int)`，将 `(uint8_t *data, int len)` 清洗为 `(uint8_t *, int)`，将 `(void)` 保留为 `(void)`。
- **高精度 C/C++ 语法解析器 (`parseFunctions`)**：
  1. 字符串、原始字符串及注释的 1:1 等宽掩码净化。
  2. 宏定义高精扫描（支持带参宏与多行宏）。
  3. 结构体、联合体、枚举、类定义扫描（支持 typedef 别名与原生命名）。
  4. 函数实现体与函数声明/原型扫描（支持命名空间/类作用域、析构函数、const修饰、参数换行、多行签名）。
  5. 全局变量、静态变量与 extern 声明变量扫描（支持指针、引用、数组方括号修饰符、volatile/const 修饰符）。
  6. 符号引用计数统计：扫描文件中符号有效引用次数（排除自身定义行），>9 显示为 `+9`。
- **自定义列表委托 `SymbolListDelegate`**：
  - 绘制左侧对应符号的高清矢量图标。
  - 绘制符号名称（若 `refCount > 0` 渲染为红色 `#EF4444`，与截图一致；若 `refCount == 0` 为正常色）。
  - 绘制返回类型及参数类型（淡灰色/次级文本色）。
  - 绘制右侧红色引用频次角标（如 `+9`, `6`, `2`, `1`）。
  - 优雅的悬停与选中圆角高亮效果。
- **精准跳转定位与高亮 (`navigateToSymbol`)**：
  - 精确定位符号名所在的真实行号与起始列号。
  - `setCursorPosition` + `setSelection(line, col, line, col + len)` 精确选中函数名/符号名。
  - 将视口平滑滚动至 1/3 位置并赋予编辑器焦点。

---

## 验证计划

### 1. 代码语法解析验证
- 打开单片机工程 C 文件（如包含 `Delay(int)`, `Uart2_Handle`, `_485_Config`, `uart_buff[1024]`）：
  - 检查显示格式是否为 `Delay void (int)`、`Uart2_Handle int`、`uart_buff int[1024]`、`_485_Config void (void)`。
  - 检查右侧是否显示正确的引用数（如 `+9`、`2`、`1` 等）。
  - 检查有引用的符号名称是否显示为红色。

### 2. 头文件全符号解析验证
- 打开头文件（如 `.h`/`.hpp`）：
  - 检查 `#define` 宏定义、`typedef struct` 结构体、`enum` 枚举、`union` 联合体、`class` 类、`extern` 变量、声明的函数原型是否全部被正确分类并识别。

### 3. 点击跳转与高亮验证
- 点击函数列表各项：
  - 验证光标是否精准定位在函数名的第一字符处。
  - 验证函数名是否被准确选中高亮。
  - 验证编辑器视图是否滚动到该行。
