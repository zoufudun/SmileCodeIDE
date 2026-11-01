# 目标芯片选择对话框 (TargetChipDialog) 视觉增强与暗色主题深度融合

针对用户提出的需求：**“增强实现外观，要融为一体，保证字体清晰可见”**，我们对 [TargetChipDialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/STM32WorkbenchPlugin/UI/TargetChipDialog.cpp) 进行了全方位的 UI 重构与视觉升级。

---

## 一、效果对比 (Before vs After)

````carousel
![修改前：高对比度刺眼浅色卡片、红色警示边框、列宽截断](/C:/Users/Administrator/.gemini/antigravity-ide/brain/c53f7397-8ff7-4dd2-9dd5-a1c55be517b7/target_chip_dialog_before.png)
<!-- slide -->
![修改后：深度融合现代暗色主题、字体清晰饱满、无截断、高雅质感](/C:/Users/Administrator/.gemini/antigravity-ide/brain/c53f7397-8ff7-4dd2-9dd5-a1c55be517b7/target_chip_dialog_enhanced.png)
````

---

## 二、核心改进详情

### 1. 顶部厂商/系列/器件联动卡片深度融入暗色主题
* **问题诊断**：原先顶部面板采用硬编码的浅文旦色背景（`#fdece6`），下拉框为白底（`#ffffff`），获取焦点时甚至出现刺眼的亮红色边框（`#d63031`），与 IDE 的整体暗黑风格格格不入。
* **重构方案**：
  * **背景与边框**：将容器背景设为深色子卡片背景 `#181825`，边框采用细腻低饱和度灰紫 `#313244`（圆角 `8px`）。
  * **下拉框美化**：下拉框底色调整为 `#1e1e2e`，悬浮及展开时使用品牌微蓝 `#89b4fa` 光晕边框。
  * **下三角箭头**：引入轻量级下三角资源图标 `:/stm32/ui/arrow_down.png`，替换原先扁平突兀的默认控件样式。
  * **下拉列表弹窗 (`QAbstractItemView`)**：暗色面板 `#181825` + 悬停/选中项 `#313244` 高亮，字体清晰锐利。

### 2. 快速筛选与器件表格布局及列宽优化
* **消除表头截断与省略号**：
  * 原先“CPU 内核”因列宽被硬性挤压仅显示为“CPU 内”，内容单元格更是截断成“cortex-”。
  * 移除导致列宽压缩的空表 `ResizeToContents` 误设，建立动态合理的默认列宽分配：
    * **器件型号**：`150px`（保证如 `STM32H743VIT6TR` 完整显示且有呼吸空间）
    * **所属系列**：`95px`（展示 `STM32H7xx`）
    * **CPU 内核**：`135px`（完整显示为 `Cortex-M7 (FPU)`，清晰直观）
    * **Flash 容量**：`85px`（右对齐展示 `2048 KB`）
    * **RAM 容量**：`85px` + 弹性拉伸（`QHeaderView::Stretch`，右对齐展示 `1024 KB`）
* **文字色彩层级设计**：
  * **器件型号**：纯白 `#ffffff`，加粗醒目。
  * **系列**：次要灰白 `#a6adc8`。
  * **CPU 内核**：淡蓝 `#89b4fa`。
  * **Flash / RAM**：柔和青绿 `#a6e3a1`。
  * 表格背景采用深邃交替色 `#11111b` 与 `#181825`，选中高亮为暗蓝 `#2b3b5c`。
* **消除原生滚动条与角落噪点**：
  * 针对 Windows 原生半色调点状（dithered checkerboard）滚动条，全面接管定制现代深色细滚动条（`8px`，圆角滑块 `#45475a`，悬停 `#585b70`）。

### 3. 硬件参数与调试配置面板语义化着色
* **问题诊断**：原先详情配置中所有属性名称均为昏暗无辨识度的灰色（`#576574`），右侧属性值全为单一绿色，甚至在 `J-Link` 处由于空间断行出现“J-\nLink”断裂截断。
* **重构方案**：
  * **标题与外框**：外框使用精致细边框 `#313244`，标题使用柔和高对比度亮灰紫 `#a6adc8`。
  * **目标器件徽章**：采用独立深色徽章卡片展示当前选中芯片，文字采用高辨识等宽字体与青蓝色 `#89dceb`。
  * **语义化配色层次**：
    | 参数项 | 色彩 | 说明 |
    | :--- | :--- | :--- |
    | **目标器件** | `#89dceb`（Cyan Badge） | 醒目标识当前目标芯片 |
    | **CPU 内核与硬件浮点** | `#f9e2af`（Warm Yellow） | 明确突出内核架构与浮点指令集参数 |
    | **Flash / RAM 规格** | `#a6e3a1`（Soft Green） | 容量与基地址一目了然 |
    | **编译器宏定义** | `#fab387`（Peach/Orange） | 突出编译期宏标识 |
    | **链接脚本** | `#89b4fa`（Sapphire Blue） | 突出官方原版与 ld 文件名 |
    | **调试器目标** | `#cba6f7`（Mauve Purple） | OpenOCD 与 J-Link 双行清晰排版，彻底杜绝跨行连字符截断 |

---

## 三、修改文件清单与编译验证

1. [UI/TargetChipDialog.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/STM32WorkbenchPlugin/UI/TargetChipDialog.cpp)：
   - 重构 `applyTheme()` 样式表。
   - 优化 `setupFilterPanel()`、`setupDeviceTable()`、`updateDetailsPanel()` 的排版与文字配色。
2. [cubemx_linkers.qrc](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/STM32WorkbenchPlugin/cubemx_linkers.qrc)：
   - 注册 `icons/arrow_down.png` 下拉小箭头图标资源。
3. **编译构建结果**：
   - `STM32WorkbenchPlugin.dll`：使用 Qt 6.11.2 MinGW 64-bit 成功编译并同步更新至 `PhudonTools/plugins/STM32WorkbenchPlugin.dll`。
   - `STM32Workbench.exe`（独立调试宿主）：验证编译全部通过，0 错误 0 警告。
