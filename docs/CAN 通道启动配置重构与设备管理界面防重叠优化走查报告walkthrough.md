# CAN 通道启动配置重构与设备管理界面防重叠优化走查报告

## 1. 解决问题概述

### 问题 1：启动通道参数配置中，选择 CAN 时参数混乱、叠加重叠（图 1）
- **现象分析**：在启动通道对话框中，当协议切换为 CAN 时，界面上出现 CANFD 与 CAN 参数标签重叠，感觉是在 CANFD 基础上叠加了 CAN 参数。
- **排查根因**：
  1. 此前在同一个 `QFormLayout` 中通过隐藏行控件来切换，且 `scrollWidget` 设置了 `Qt::WA_TranslucentBackground`，导致 Windows 渲染引擎在控件位移和隐藏时**未清空背景重绘脏矩形（Dirty Rect）**，留下了幽灵文本；
  2. 动态隐藏 label 和 widget 无法保证各模式参数的彻底隔离与干净尺寸调整。
- **解决方案**：
  - 引入 `QStackedWidget` 架构，将 **CAN** 与 **CAN FD** 的配置完全解耦为两个相互独立的页面（`m_canPage` 和 `m_fdPage`）：
    - **选 CAN 时**：仅展示 CAN 专有参数配置页（波特率、自定义波特率、工作模式、终端电阻、发送重试、总线利用率），完全不包含任何 CANFD 标准、加速或数据域波特率，波特率标签清晰显示为“波特率”；
    - **选 CAN FD 时**：展示完整的 CANFD 参数配置页（CANFD标准、CANFD加速、仲裁域波特率、数据域波特率、自定义波特率、工作模式、终端电阻、发送重试、总线利用率）；
    - 切换协议时通过 `setCurrentWidget` 瞬间干净切换，并调用 `adjustSize()` 动态自适应对话框高度，彻底根除控件重叠与背景重绘残留；
  - 增加启动对话框的最大化/最小化支持（`Qt::WindowMinMaxButtonsHint`）。

---

### 问题 2：设备管理界面顶部文字严重重叠挤压、缺少最大化功能（图 2）
- **现象分析**：图 2 中，顶部配置栏里的“搜索”输入框与“型号”标签、“USBCANFD-200U”与“索引”标签严重粘连、重叠挤压在一起；且对话框右上角仅有 `?` 和 `X`，无法最大化全屏查看。
- **排查根因**：
  1. 顶部单行 `QHBoxLayout` 塞入了 12 个控件（厂商筛选、搜索、型号、索引、打开设备、检测接入设备、刷新、展开、收起、复制配置、云设备、关闭），在 740px 乃至 900px 宽度下物理空间不足，Qt 强行压缩导致相邻标签与输入框相互穿透；
  2. `QDialog` 默认未开启 `Qt::WindowMaximizeButtonHint`。
- **解决方案**：
  - **支持最大化/最小化**：
    设置 `setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)`，移除无用的 `?` 按钮，赋予用户随时双击标题栏或点击最大化按钮满屏使用的能力；
  - **两行式合理结构布局**：
    - **第 1 行（设备选择与主连接）**：
      `厂商:` [全部厂商下拉] `搜索:` [搜索输入框] `型号:` [型号下拉框(自动拉伸)] `索引:` [数字微调框] [打开设备/关闭设备] [检测接入设备]
    - **第 2 行（工具操作栏）**：
      [刷新] [全部展开] [全部收起] [复制配置] [云设备] `[弹簧伸缩区]` [关闭窗口]
  - **合理默认及最小尺寸**：
    将窗口最小尺寸由 `740×480` 调整为 `960×580`，默认启动尺寸 `1120×680`；树状图列自适应拉伸（列 1 随窗口最大化自动铺满）。

---

## 2. 变更文件与代码对照

### [candevicedialog.cpp](file:///m:/TOPFIRE/SmileCodeIDEUpdate/PhudonTools/candevicedialog.cpp)
1. **头文件引入**：增加 `#include <QStackedWidget>`。
2. **StartChannelDialog**：
   - 移除 `WA_TranslucentBackground`，采用独立 `m_canPage` 与 `m_fdPage`；
   - `switchProtocol` 监听协议切换，干净切换 Stack 页面并自适应调整窗口大小；
   - 增加 `setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::WindowMinMaxButtonsHint)`。
3. **CanDeviceDialog::setupUi()**：
   - 拆分单行挤压布局为 `devRow`（设备连接区）与 `toolRow`（工具区）；
   - 增加最大化与最小化窗口标志；
   - `m_deviceTypeCombo` 设置 `Expanding` 尺寸策略并在最大化时自适应拉伸；
   - 树状视图表头启用自适应伸展；
   - 顶部主按钮支持在设备打开后切换为“关闭设备”，点击可直接关闭。

---

## 3. 验证与构建结果
- **编译命令**：`mingw32-make -f Makefile.Release`
- **编译输出**：`release/PhudonTools.exe` 成功更新，退出码 `0`。
- **构建时间**：2026/9/12 13:26:54 (6,144,512 字节)。
