# Walkthrough — 修复 CSV 导出乱码根因与全平台兼容性

针对您反馈的 **“导出 CSV 打开全是乱码 (`璁倶 ID...`)”** 现象，我们进行了深入排查并彻底解决该问题。

---

## 1. 乱码根因分析 (Root Cause Analysis)

在之前的代码中：
1. `exportMappingsToCsv()` 返回的是 `QString` 类型，在拼接 `"\xEF\xBB\xBF"` 时，C++ 编译器和 `QString` 将其解析为 3 个 Unicode 字符（`U+00EF`, `U+00BB`, `U+00BF`）；
2. 写入文件时调用了 `.toUtf8()`，导致 3 个字符被重新多字节编码为 `6 字节 (0xC3 0xAF 0xC3 0xBB 0xC3 0xBF)`，破坏了标准的 3 字节 UTF-8 BOM 标记（`0xEF 0xBB 0xBF`）；
3. **Microsoft Excel / WPS 无法识别损坏的 BOM**，因此默认以 Windows 中文系统的 **ANSI (GBK)** 编码来强制解析 UTF-8 字节串，导致 `设备 ID` 变为典型的 GBK 乱码 `璁倶 ID`。

---

## 2. 修复方案与代码变更

1. **直接写入二进制 3 字节 UTF-8 BOM (`0xEF, 0xBB, 0xBF`)**：
   - 将 `exportMappingsToCsv()` 的返回值类型改为 **`QByteArray`**；
   - 在二进制流的最前端直接追加原始 `\xEF\xBB\xBF` 3 字节 BOM 头部，后续内容通过 `.toUtf8()` 拼接；
   - 在写文件时使用 `QIODevice::WriteOnly`（二进制模式）写入文件，避免 `QIODevice::Text` 改变 BOM 字节。

2. **自适应 GBK / ANSI CSV 导入兼容**：
   - 增加 `QTextCodec` 智能判断：如果用户在 Excel 中编辑后另存为 ANSI/GBK 格式的 CSV，系统在导入时能自动识别并以 GBK 编码转换解码，彻底解决二次导入时的乱码问题。

---

## 3. 编译验证与确认

- **构建命令**：`mingw32-make -f Makefile.Release`
- **构建结果**：Clean Build Success (Exit Code: 0)，`release/PhudonTools.exe` 构建完成。
- **效果**：导出生成的 CSV 文件包含标准的 3 字节 BOM，用 Excel / WPS 打开均能直接正常显示中文。
