# F1/F4/F7/H7 四系列实施进度（2026-10-12）

> 本文件保留多轮历史记录。用户最新要求聚焦 H743，最终状态见文末及 `H743_WALKTHROUGH_2026-10-12.md`。

## 目标与已确认清单

- 本轮范围为 STM32CubeMX 6.18.0 可选的 F1、F4、F7、H7 MCU，不含 STM32MP。
- CubeFinder SQLite 与 `families.xml` 交叉核对得到 650 个不同参考型号：F1 125、F4 202、F7 123、H7 200。对应订货号分别作为精确别名处理。
- 用户要求离线运行且 `.ld` 字节与 CubeMX 生成结果一致，因此必须逐参考型号保存生成物与 SHA-256；不能把少量模板推导为全系列覆盖。

## 当前已做

- `tools/generate_cube_catalog.py` 从本机数据库枚举目标，调用 CubeMX CLI，逐批保存原始 `.ld`、生成的 `.ioc`、编译宏、FPU/ABI 设置及 SHA-256；支持断点续跑和完整性验证。`tests/test_generate_cube_catalog.py` 的 2 项测试通过。
- `Core/STM32Profile.cpp` 增加内置 JSON 清单读取、参考型号和 CPN 精确别名查找；界面型号框支持搜索，并在无效型号时阻止构建。
- H745ZITx 独立探测确认 CubeMX 分别生成 CM4/CM7 的 FLASH/RAM 四个脚本。源代码将双核 FLASH 脚本安装在 `CM4/` 或 `CM7/` 子目录，并将构建产物分隔到 `build/cm4` 与 `build/cm7`，避免同名脚本和目标文件相互覆盖。
- 生成目录 `data/cubemx-6.18.0-f1f4f7h7/` 当前只有 32/650 个参考型号，均为 F1；首批 32 个的宏已重新提取。生成清单 `manifest.json`、对应资源表 `cubemx_f1f4f7h7.qrc` 均为**部分产物**，不可发布为四系列完整包。
- Qt 工具链测试在部分资源包下运行：22 通过、0 失败、2 项因外部样例工程未提供而跳过。双核产物、具体订货号及 CubeIDE 源文件选择测试均先按预期失败，再修改实现并通过。Python 生成器测试 2 项通过。

## 阻断与恢复

先前新的文件读取审批曾返回自动审查用量限制，提示“try again at 2:49 AM”；该拒绝是审批服务用量问题，不是代码或文件安全判定。之后测试构建及 CubeMX 批量生成已重新得到审批，剩余型号生成任务于 2026-10-12 06:28（北京时间）启动，仍在运行。

审批可用后，先从工作区运行以下命令，仅处理剩余目标；两路并发比四路并发更适合本机内存：

```powershell
python Plugins/STM32WorkbenchPlugin/tools/generate_cube_catalog.py --cubemx D:\Soft\STM32CubeMX --database C:\Users\Administrator\.stmcufinder\plugins\mcufinder\mcu\cube-finder-db.db --output Plugins/STM32WorkbenchPlugin/data/cubemx-6.18.0-f1f4f7h7 --qrc Plugins/STM32WorkbenchPlugin/cubemx_f1f4f7h7.qrc --batch-size 10 --jobs 2
```

该任务预计耗时较长，并在每批后持久化清单。完成后以同样参数加 `--verify-only`，要求 `verified=650/650` 且 `problems=0`。检查每系列计数、CPN 别名、各脚本 SHA-256，尤其检查 H7 双核 CM4/CM7 和其他多脚本型号。若有失败，查看 `data/cubemx-6.18.0-f1f4f7h7/logs/` 并修复后重试。

第一次连续运行在 F1 完成、F4 累计 47 个参考型号时，清单停留在 172/650，但链接脚本目录仍继续增加。Windows 文件锁导致 `TemporaryDirectory` 清理抛错，主线程等待已排队的其余任务而不再写清单；该故障已通过先失败后通过的 Python 回归测试复现并修复。验证了进程树后停止旧 Python 与两个 CubeMX 子进程，已从 172 项落盘清单续跑。未落盘的脚本会按未完成目标重新生成。

然后重新运行 Python 与 Qt 测试，构建插件及 standalone，验证真实 F1/F4/F7/H7 工程和部署 DLL。新源码与部分生成包仍未提交；现有运行中的 SmileCodeStudio 未加载本轮改动。

## 2026-10-12 19:25 更新

- 已增加旧版 SmileCodeIDE 自动链接脚本的显式迁移流程：构建失败时询问用户，确认后先用唯一 `.bak` 文件名保存旧字节，再校验内置 CubeMX 脚本 SHA-256，并用原子写入替换。用户自维护脚本、哈希错误或备份失败都不会被覆盖。构建失败/取消迁移后进度不再停留在 30%。
- 新增的 3 项迁移 Qt 测试通过；插件使用 Qt 6.11.1 / MinGW 13.1 编译成功，并由现有后置步骤复制到 `PhudonTools/plugins/`。正在运行的 IDE 仍需重启。
- 当前用户的 CubeFinder 数据库与旧进度记录不同：实际为 648 个参考型号（F1 125、F4 202、F7 121、H7 200），不是旧数据库的 650。生成器已允许审计这种目录版本变化，并支持 `--family STM32H7` 分系列续跑。
- H7 生成在 CubeMX 下载/初始化 `STM32Cube_FW_H7_V1.13.0` 时未完成；临时文件约 606 MB。清单仍只有 322 个已验证目标，另外 328 个明确记录为未生成，不能声称 H7 全量完成。
- 当前 Qt 全套结果为 26 通过、4 失败、4 跳过。迁移测试全通过；失败项是部分目录包导致的四系列覆盖/双核样例断言，以及两个既有清理测试使用了空芯片档案。完整目录生成完成前这些失败不能关闭。

## H7 单系列续跑（2026-10-12 夜间）

- 以本机当前 CubeFinder 数据库重新枚举，仍为 650 个参考型号（F1 125、F4 202、F7 123、H7 200）；H7 含 260 个订货号别名与 29 个双核参考型号。先前 648 项是数据库更新期间的短暂清单，后续覆盖校验以本次 650 项为准。
- 生成器支持 `--family STM32H7` 单系列续跑和 `--verify-only --family STM32H7` 单系列校验；其他系列的未完成记录不会影响 H7 的完成判定。Python 生成器测试现为 6 项通过。
- 已确认 `STM32H745ZITx` 可由本机 CubeMX CLI 生成 CM4/CM7 的 FLASH/RAM 原始脚本。早期 H7 批次超时主要与多个 CubeMX 进程占用内存、遗留子进程有关；清理遗留进程后，单进程 3/3、双进程各批 6/6 成功。随后启动每批 5 型号、两路并发的 H7 续跑：

```powershell
python Plugins/STM32WorkbenchPlugin/tools/generate_cube_catalog.py --cubemx D:\Soft\STM32CubeMX --database C:\Users\Administrator\.stmcufinder\plugins\mcufinder\mcu\cube-finder-db.db --output Plugins/STM32WorkbenchPlugin/data/cubemx-6.18.0-f1f4f7h7 --qrc Plugins/STM32WorkbenchPlugin/cubemx_f1f4f7h7.qrc --family STM32H7 --batch-size 5 --jobs 2 --timeout 360
```

- H7 专项 Qt 验收用例要求 200 参考型号、260 订货号、29 双核型号及所有运行时档案可查；源码已编译。双核清理测试原先构造空芯片名，与有效芯片检查相冲突，现设置真实 H745 型号。最终仍需等待 H7 全量数据、`--verify-only --family STM32H7` 零问题、Qt 专项测试与插件构建；未完成前不得宣称 H7 全量交付。

## 用户收敛至 H743 后的完成状态

- 用户最新范围为 STM32H743。已停止上述广范围 H7 生成任务，清理其 CubeMX 子进程；未继续消耗时间生成其它 H7 型号。
- 新增 `--reference-prefix STM32H743` 后，16/16 个 H743 参考型号、21 个订货号、32 份原版 `.ld` 完整保存；专项 `--verify-only` 为 0 问题。生成器测试 9 项通过。
- H743 Qt 专项覆盖、用户 CAN 工程副本的实际构建、ST-Link/CMSIS-DAP 配置回归均通过。插件 DLL 已构建并复制到 `PhudonTools/plugins`，原 DLL 在测试目录留有备份。证据与恢复步骤详见 `H743_WALKTHROUGH_2026-10-12.md`。
- 综合目录当前 381/650（F1 125、F4 197、H7 59、F7 0），其中 H743 已全量完成。其它系列及 H7 其它子系列仍为独立未完成工作，不应误报为四系列完成。
