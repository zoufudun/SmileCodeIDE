# STM32 工作台实施 walkthrough（2026-10-11）

## 本轮交付边界

本轮选择用户允许的“实现插件功能”路径，尚未交付全型号离线 `.ld` 包。已修复截图中的 H743 链接错误，并完善 ST-Link V2/CMSIS-DAP 的 OpenOCD 烧录前检查与无序列号设备选择。当前版本仍只有少量内置芯片档案；`data/cubemx-6.18.0/manifest.json` 明确标注 H743VITx 样例覆盖，不能声称全种类 STM32 已支持。

## 核心发现与修复

1. 本机 STM32CubeMX 6.18.0 命令行生成 `STM32H743VITX_FLASH.ld` 和 RAM 变体。FLASH 文件 SHA-256 为 `A80674F7089FED7E7E0924E9F6742D70915CFB259D908A7B8EAA7F4EF4779BCF`，与 CubeIDE linkerdb 的 `stm32h743xi_flash.ld` 逐字节一致。已将 CubeMX 输出原字节和许可随插件资源打包。
2. 截图工程 `K:/ARM/CAN/Makefile` 指定 H743，却错误使用目录中唯一的 `STM32F103C8Tx_FLASH.ld`；该文件没有 `_estack`。根因是 `MakefileGenerator` 会选第一个现存 `.ld`，不检查芯片名。现在忽略文件名明确属于其他 STM32 型号的脚本，为 H743 安装 CubeMX 原版文件；无型号前缀的用户脚本仍可使用，多个候选返回明确错误。原工程未改动。
3. 未知芯片原先静默回退 STM32F103C8。现在只允许精确档案匹配，未知型号包括伪造的 H743 后缀均返回空档案；OpenOCD target 推荐也不再回退 F1。内置 CubeMX 脚本复制前核对 SHA-256。
4. 若默认文件名已存在旧版 SmileCodeIDE 自动生成脚本，生成器会返回迁移提示并保留原文件；同一工程有多个适用候选脚本也会要求显式选择。
   未打包 CubeMX 原版脚本的其他档案，只有工程自带或显式指定 `.ld` 时才能构建；不再自动合成一个近似脚本。
5. ST-Link V2 和 CMSIS-DAP 的烧录驱动入口原已存在。现在启动 OpenOCD 前检查 interface/target 脚本是否存在、目标是否有已知配置，并兼容 ../share/openocd/scripts 与 ../scripts 两种布局。对于无可读 USB 序列号的设备，只有一台兼容下载器时可留空；多台兼容设备时仍要求序列号。界面提示同步更新。

## 验证证据

- `ToolchainDetectorTest`：18 通过、0 失败、0 跳过；包括真实复制工程构建、脚本哈希、异芯片 `.ld` 排除、旧脚本保留、歧义拒绝及缺失原版脚本时阻止合成。
- `ProbeDetectorTest`：14 通过、0 失败、1 跳过；跳过项要求下载器具有可读序列号。单台无序列号 ST-Link 和两种 OpenOCD 路径布局的测试通过。
- 将 `K:/ARM/CAN` 的源文件复制到工作区隔离目录，排除旧 build/Output/Listing 后用 GNU Arm 编译：ELF 构建成功，H743 `.ld` 的哈希与 CubeMX 输出一致。ELF 向量表初始 SP 为 `0x24080000`，即原版脚本定义的 RAM_D1 顶端；text/data/bss 为 27972/104/3192 字节。
- Qt 6.11.2 + MinGW 13.1 插件构建成功；构建 DLL 已部署至截图所示 SmileCodeStudio 插件目录。正在运行的 IDE 需要重启才能加载它。
- 本机检测到 ST-LINK `0483:3748`，没有可读序列号；OpenOCD 0.12.0 的 `init` 只读连接返回 `Error: open failed`，在允许访问 USB 的执行环境中仍相同。尚未验证真实 ST-Link/CMSIS-DAP 烧录；也未观察到 CMSIS-DAP 设备。
- CubeMX 在生成未安装本地固件包的 F0 型号时开始下载 `STM32Cube_FW_F0`，但未完成 `.ld` 输出，探测已中止。本机仓库只已有 F1/F4/F7/H7 包；构建全型号包需先取得对应系列固件包并再做逐型号生成与审计。

## 下次会话要继续完成的原始目标

1. 按[设计文档](../../docs/superpowers/specs/2026-10-11-stm32-cubemx-catalog-design.md)与[实施计划](../../docs/superpowers/plans/2026-10-11-stm32-catalog-implementation.md)制作**全型号**离线包。当前 CubeMX 设备选择数据库 `C:/Users/Administrator/.stmcufinder/plugins/mcufinder/mcu/cube-finder-db.db` 中有约 5091 个非 MP 可选 CPN，约 2730 个不同 `refname`；先前从 `families.xml` 得到的 2666 条只是另一层设备目录，不可作为唯一覆盖分母。
2. 逐目标/核心/安全状态/FLASH-RAM 上下文调用 CubeMX，保存实际 `.ld`、编译参数与字段级来源，做 SHA-256 和覆盖报告。CubeIDE linkerdb 仅有 317 个脚本，不能直接作为全覆盖证明。
3. 将完整清单接入芯片搜索、上下文选择和 `LinkerScriptRepository`，废除其余型号的通用脚本生成；处理项目自带多个候选 `.ld` 的显式选择。
4. 诊断 ST-LINK `open failed` 的实际驱动/硬件原因；如有 CMSIS-DAP 硬件，再做连接和烧录验证。不要把 OpenOCD 参数测试表述成真实硬件验证。

## 文件与状态

主要实现：`Core/STM32Profile.*`、`Core/MakefileGenerator.cpp`、`Core/FlashProgrammer.*`、`UI/FlashConfigDialog.cpp`、`cubemx_linkers.qrc`、`data/cubemx-6.18.0/`。测试：`tests/ToolchainDetectorTest.cpp`、`tests/ProbeDetectorTest.cpp`。临时验证目录：`cubemx-probe/`、`build-implementation-tests/`、`build-implementation-plugin/`；它们是工作区内生成物，不应作为源码提交。
