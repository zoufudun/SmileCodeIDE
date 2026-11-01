# STM32H743 完整补充 walkthrough（2026-10-12）

## 本次范围与结果

本次按用户最新要求聚焦 STM32H743，不把其余 H7、F7 或 STM32MP 计为已完成。型号依据本机 STM32CubeMX 6.18.0 的 `families.xml` 与 CubeFinder `cpn` 数据库交叉核对，共 16 个 STM32H743 参考型号、21 个具体订货号。全部 16 个型号通过 CubeMX CLI 生成工程，保存 16 份 FLASH 与 16 份 RAM 链接脚本。`.ld` 保留原始字节，清单记录每份 SHA-256；`--verify-only` 结果为 `verified=16/16`、`problems=0`。

16 个参考型号：`STM32H743AGIx`、`AIIx`、`BGTx`、`BITx`、`IGKx`、`IGTx`、`IIKx`、`IITx`、`VGHx`、`VGTx`、`VIHx`、`VITx`、`XGHx`、`XIHx`、`ZGTx`、`ZITx`（后 15 个均带 `STM32H743` 前缀）。具体订货号以 `manifest.json` 的 `cpnAliases` 为准。

## 实现思路

1. `tools/generate_cube_catalog.py` 读取本机 CubeMX 型号数据库，新增 `--reference-prefix STM32H743` 精确选择与单独校验。CubeMX CLI 逐型号生成 CubeIDE 工程；生成器直接复制 `.ld`、`.ioc`，记录编译宏、FPU/ABI 与 SHA-256。超时日志保留 CubeMX 已输出的诊断。生成任务可断点续跑。
2. `data/cubemx-6.18.0-f1f4f7h7/manifest.json` 与 `cubemx_f1f4f7h7.qrc` 将已生成脚本嵌入插件。插件运行时无需 CubeMX，即可按参考型号或订货号查找档案，校验资源 SHA-256 后安装原版 FLASH `.ld`。型号下拉框允许搜索，未知型号会被拒绝。
3. 构建器读取 CubeIDE/Keil 工程源文件配置并生成 Makefile；现有链接脚本若与内置脚本不同，不会静默覆盖。旧版自动生成脚本提供显式迁移与备份。烧录设置从已校验脚本提取 FLASH 起点；ST-Link V2、CMSIS-DAP 的 OpenOCD 配置保留。

## 验证记录

- 生成器测试：9 项通过。H743 专项完整性校验：16/16，0 问题；21 个订货号别名、32 份 `.ld`，逐文件 SHA-256 无不匹配。
- `STM32H743VITx` FLASH 脚本 SHA-256：`a80674f7089fed7e7e0924e9f6742d70915cfb259d908a7b8eaa7f4ef4779bcf`，与用户原工程 `K:/ARM/CAN/STM32H743VITx_FLASH.ld` 一致。
- Qt 工具链全套：31 通过、0 失败、6 跳过。H743 参考型号/订货号运行时验收通过。用 `K:/ARM/CAN` 的源文件副本、插件安装的 CubeMX 链接脚本和 GNU Arm 编译成功生成 `build/stm32h743vitx.elf`，`arm-none-eabi-size` 为 text 27148、data 104、bss 3192。原工程未修改。旧简称 `STM32H743VI` 的构建路径也单独通过。
- 下载器配置回归：16 通过、0 失败、1 跳过；跳过的是实际下载器序列号自动选择。未对实体芯片执行烧录。
- Qt 6.11.2 / MinGW 13.1 插件构建成功，DLL 已复制至 `PhudonTools/plugins/STM32WorkbenchPlugin.dll`，并部署到截图中的程序实际加载目录 `build/Desktop_Qt_6_11_2_MinGW_64_bit_Release/PhudonTools/release/plugins/STM32WorkbenchPlugin.dll`。两处 SHA-256 均为 `cce04b2d249b0c544e4294aca2032f6fe730b2751376b8c2201a95b68cfd07ae`。旧 DLL 分别备份于 `build-implementation-tests/pre-h743-plugin-B2A70086.dll` 与 `build-implementation-tests/pre-h743-running-app-plugin-B2A70086.dll`。部署时程序未运行；下次启动即加载新 DLL。

## 当前边界与下次会话

当前综合清单为 381/650 个参考型号：F1 125、F4 197、H7 59、F7 0；其中 H743 16/16 已完成，其余 H7 与四系列整体仍未完成。Qt 测试对未完成的全 H7/四系列覆盖作显式跳过，H743 专项检查仍是强制通过。不要将现有综合 qrc 称为全 H7 或全四系列包。

下次会话可先读本文件及 `FOUR_SERIES_PROGRESS_2026-10-12.md`，再看 `git status` 与 `data/cubemx-6.18.0-f1f4f7h7/manifest.json`。若继续其它 H7，先确认没有仍运行的 CubeMX 子进程，再按需使用 `--family STM32H7` 续跑；若仅重新核验 H743：

```powershell
python Plugins/STM32WorkbenchPlugin/tools/generate_cube_catalog.py --cubemx D:\Soft\STM32CubeMX --database C:\Users\Administrator\.stmcufinder\plugins\mcufinder\mcu\cube-finder-db.db --output Plugins/STM32WorkbenchPlugin/data/cubemx-6.18.0-f1f4f7h7 --family STM32H7 --reference-prefix STM32H743 --verify-only
```
