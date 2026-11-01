# STM32 F1/F4/F7/H7 型号包实施 walkthrough（2026-10-12）

> 此文件记录原四系列目标的中间阶段。最新已完成范围为 H743 16/16；请先读 `H743_WALKTHROUGH_2026-10-12.md`，不要使用下文旧计数判断当前覆盖情况。

## 目标与边界

本轮目标是为 STM32CubeMX 6.18.0 可选的 STM32F1、STM32F4、STM32F7、STM32H7 MCU 提供具体型号选择和离线 `.ld`。STM32MP 暂不包含。用户要求 `.ld` 与 CubeMX 输出一致，因此不使用内存容量模板合成脚本。

## 型号来源与生成方法

1. 从本机 CubeFinder 数据库 `C:/Users/Administrator/.stmcufinder/plugins/mcufinder/mcu/cube-finder-db.db` 的 `cpn` 表读取具体订货号与 CubeMX 参考型号，再与 `D:/Soft/STM32CubeMX/db/mcu/families.xml` 交叉核对。得到 650 个参考型号、1204 个订货号：F1 125/309，F4 202/431，F7 123/204，H7 200/260。H7 中 29 个参考型号为 CM7+CM4 双核。
2. `tools/generate_cube_catalog.py` 使用本机 CubeMX CLI，针对每个参考型号调用 `load`、设置 CubeIDE 工具链及 `project generate`。脚本保存生成的所有 `.ld` 原始字节和 `.ioc`，为每个文件记录 SHA-256，并从对应 `.cproject` 提取编译宏、FPU 及浮点 ABI。
3. 生成器将已完成型号持久化于 `data/cubemx-6.18.0-f1f4f7h7/manifest.json`，并生成 `cubemx_f1f4f7h7.qrc`。重新运行时跳过已完成型号。`--verify-only` 验证 650 个目标的覆盖、脚本哈希、设备宏和双核上下文。
4. 插件运行时从内置 JSON 清单建立芯片档案。型号框列出 CubeMX 参考型号及具体订货号，支持不区分大小写的包含式搜索。双核型号以 `@CM4`、`@CM7` 区分，FLASH 脚本安装在独立子目录；固件也分别写到 `build/cm4` 和 `build/cm7`。
5. CubeIDE 工程的 `.project` 记录实际启用的链接源文件，`.cproject` 记录包含目录。构建器现在优先读这些配置，避免把驱动包里未启用的源文件全部编译；仍保留原 Keil 工程扫描及普通目录扫描路径。
6. 项目根目录只有一份 Makefile。双核工程切换 CM4/CM7 后，清理操作直接删除当前核心的构建目录，避免误执行另一核心留下的 Makefile；重新构建先做相同的定向清理，再生成当前核心 Makefile。
7. H745 CM4 的 CubeMX FLASH 起点是 `0x08100000`，CM7 是 `0x08000000`。运行时从已校验 SHA-256 的内置 `.ld` 解析 FLASH 起点；烧录对话框按具体型号及核心选取默认地址，并将手动地址保存为该型号/核心的独立设置。
8. 型号框允许输入具体订货号。无效输入时，构建管理器拒绝清理，烧录器拒绝下载，避免使用无效档案触发旧构建产物或默认设备行为。

## 当前验证与待完成项

- 生成任务于 2026-10-12 06:28（北京时间）以 10 型号/批、两路 CubeMX 并发启动。F1 已完成 125/125 参考型号、309 个订货号及 139 份 `.ld`；逐文件 SHA-256 校验无不匹配，设备宏齐全。准确全局完成数与失败数以清单及生成器输出为准；**在 650/650 且 `problems=0` 前不可称为完整包**。
- 第一轮批量运行在清单 172/650 后遇到 Windows 临时目录文件锁：生成目录继续增加但清单不更新。新增测试复现了清理失败导致已提取结果丢失，生成器现忽略临时目录清理失败并保留批次结果。确认进程树后停止旧任务，使用相同命令从 172 项落盘清单续跑；未落盘的目标会重新生成。
- 生成器单元测试已增至 3 通过，新增用例复现并修复 Windows 临时工程清理失败。Qt 工具链测试在增加覆盖门槛前，连同 F1 实际构建和 H745 双核源扫描为 26 通过、0 失败、2 跳过。新增 `catalogCoversFourCompleteFamilies` 要求 650 个参考型号及 CPN 别名，目前在部分包 212/650 时按预期失败；待全量完成后重跑。
- 插件源码已通过 Qt 6.11.2 / MinGW 13.1 编译。编译时的后置步骤会复制 DLL；部分资源包构建结束后，已把使用中的 `PhudonTools/plugins/STM32WorkbenchPlugin.dll` 恢复为原有部署副本 SHA-256 `3B57D84A5D98AD28FC020A1088DC0FA8AF6AFB6625934DE9F04F6BDECFAAD0A`，待完整包验证后再部署。
- 将本机 CubeMX 生成的 F100C4 CubeIDE 工程复制到工作区测试目录，使用插件 Makefile 和 `D:/Toolchains/xpack-arm-none-eabi/bin/arm-none-eabi-gcc.exe` 构建，ELF 成功生成。安装在工程根目录的 `STM32F100C4TX_FLASH.ld` SHA-256 为 `a66dd9a0dca3f7661b7453a86a1e5ad73eedd6ffcc770a18887886b13de54966`，与清单一致。
- 下载器回归测试 `ProbeDetectorTest`：16 通过、0 失败、1 项因缺少唯一且可读序列号的实际下载器而跳过。该测试覆盖 ST-Link、CMSIS-DAP 的配置路径、参数、分核心地址默认值及无效型号拒绝；未执行真实烧录。
- 全量生成完成后，运行 `--verify-only`，核查 F1/F4/F7/H7 数量、订货号别名、H7 双核每核宏和 FPU/ABI，再重新构建 Qt 测试、插件及独立版。用四系列实际工程验证构建与 `.ld` 哈希。随后更新本 walkthrough 的最终证据与剩余限制。

## 关键文件

- 生成器：`tools/generate_cube_catalog.py`；生成器测试：`tests/test_generate_cube_catalog.py`。
- 运行时目录：`Core/STM32Profile.*`、`Core/MakefileGenerator.cpp`、`Core/BuildManager.cpp`；Qt 测试：`tests/ToolchainDetectorTest.cpp`。
- 型号界面：`UI/STM32WorkbenchWidget.cpp`、`UI/ToolchainConfigDialog.cpp`。
- 数据与资源：`data/cubemx-6.18.0-f1f4f7h7/`、`cubemx_f1f4f7h7.qrc`。
- 过程状态与恢复命令：`FOUR_SERIES_PROGRESS_2026-10-12.md`。

## 下一会话恢复

先阅读本文件和 `FOUR_SERIES_PROGRESS_2026-10-12.md`，检查 `manifest.json` 的 `targets`、`failures` 与正在运行的生成进程。若进程不在，按进度文件中的命令续跑。不要把未通过完整性验证的部分 qrc 或 DLL 当作四系列完整交付。
