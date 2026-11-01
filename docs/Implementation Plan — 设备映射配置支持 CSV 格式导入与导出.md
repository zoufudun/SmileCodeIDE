# Implementation Plan — 设备映射配置支持 CSV 格式导入与导出

根据您的需求，本计划将在设备状态监控系统与协议配置对话框中，全面增加对 **CSV 格式表格文件** 的导入与导出支持。

---

## User Review Required

> [!IMPORTANT]
> **UTF-8 BOM 兼容性说明**：
> - 导出的 CSV 文件将使用包含 **UTF-8 BOM (`\xEF\xBB\xBF`)** 头的编码保存。这样可以用 Microsoft Excel、WPS 或 Notepad 直接双击打开查看/编辑中文设备名称，不会出现中文字符乱码。
> - 导入 CSV 时支持自适应识别中文列头与英文列头，且支持解析 16 进制 (`0x100`) 与 10 进制 (`256`) 的 CAN ID 格式。

---

## Proposed Changes

### 1. CSV 标准字段表头规格

导出的 CSV 文件包含以下字段列（按标准格式分界）：

```csv
设备ID,设备名称,设备类型,CAN ID,字节索引,位索引,默认状态,CAN通道,所属界面,所属房间
1,前舱烟温探测器,detector,0x100,0,0,0,-1,界面1,生活休息舱
2,控制分配阀,valve,0x101,0,1,0,0,界面1,生活休息舱
```

---

### 2. 协议配置对话框与主面板 CSV 支持

#### [MODIFY] [canprotocolconfigdialog.h](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.h) / [canprotocolconfigdialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/canprotocolconfigdialog.cpp)
- **增加 CSV 解析与生成函数**：
  - `exportMappingsToCsv()`：将 `QList<DeviceBitMapping>` 转换为带有 UTF-8 BOM 的 CSV 文本串；
  - `parseCsvToMappings()`：安全解析 CSV 文本串，自动跳过表头，提取字段生成 `QList<DeviceBitMapping>`；
- **扩展导入/导出按钮逻辑**：
  - 文件保存/打开对话框过滤器扩展为 `配置文件 (*.json *.csv);;CSV 表格 (*.csv);;JSON 文件 (*.json)`；
  - 自动识别用户选择的文件扩展名 (`.csv` 或 `.json`) 执行对应的读写引擎。

#### [MODIFY] [devicemonitorpanel.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/devicemonitorpanel.cpp)
- 在主界面工具栏的 **「导入配置」** 与 **「导出配置」** 中同步接入 `.csv` 格式识别，支持直接在主监控面板批量导入外部 CSV 设备表格文件。

---

## Verification Plan

### Automated Build Verification
- 执行 Makefile 编译构建：
  ```powershell
  mingw32-make -f Makefile.Release
  ```

### Manual Verification
1. **CSV 导出验证**：
   - 在协议配置界面点击「导出配置」，在保存类型中选择 `CSV 表格 (*.csv)`；
   - 用 Excel/WPS 打开导出的 `.csv` 文件，验证各列（设备ID、设备名称、CAN ID、所属房间等）中文显示是否清晰正确，无乱码。
2. **CSV 导入验证**：
   - 修改或使用 Excel 新建一份设备 CSV 表格，点击「导入配置」选中该 `.csv` 文件；
   - 验证表格与主监控画布是否成功载入 CSV 中定义的设备及其参数。
