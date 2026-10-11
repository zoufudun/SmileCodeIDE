# STM32 全型号与烧录支持：设计 walkthrough（2026-10-11）

## 本次需求的最终范围

- 支持 STM32CubeMX 当前版本可生成的所有**STM32 MCU** 具体目标；本次**不包含 STM32MP**（用户最终确认）。
- 使用端未安装 STM32CubeMX 也必须能选芯片和取得链接脚本。
- **工作台提供的** `.ld` 必须与对应型号、构建上下文的 STM32CubeMX 实际生成文件**字节一致**；不能按 Flash/RAM 容量自行拼脚本。工程自带/用户指定脚本不受此字节匹配声明覆盖。
- 烧录支持 ST-Link V2 和 CMSIS-DAP。实际可烧录的型号还受 OpenOCD target 驱动和硬件支持约束；不支持时要明确报错。
- 本次会话只做设计，未修改插件实现、未制作完整脚本包、未执行硬件烧录。

## 已核实的现状

1. 本机安装了 `D:/Soft/STM32CubeMX` 6.18.0，另有 STM32CubeMX2 1.1.0。本设计以 6.18.0 为快照源，不混用 CubeMX2 数据。
2. `D:/Soft/STM32CubeMX/db/mcu/families.xml` 有 2898 个条目，其中 2666 个非 MP MCU 条目、232 个 MP 条目；非 MP 条目对应 1643 个不同 RPN。这些是设备目录计数，尚未证明每个条目都能成功生成工程。
3. CubeMX 安装内的 CubeIDE MCU linkerdb 插件有 317 个 `.ld`（约 1.69 MB），路径位于 `third_parties_plugins/binaries/STM32CubeIDE-MX-services/revA/pool/plugins/com.st.stm32cube.ide.mcu.linkerdb_2.2.400.202605210849/resources/bare_project/`。这些源脚本与 CubeMX 最终工程输出是否逐字节一致仍待验证。
4. 该库的 `stm32h743xi_flash.ld` 分别定义 DTCMRAM、RAM_D1、RAM_D2、RAM_D3、ITCMRAM，`_estack` 位于 RAM_D1 末端。当前工作台 H743 档案使用自生成脚本且默认 DTCM；两者不同。截图中的 `_estack` 未定义还需在实施阶段用真实工程复现，不能仅凭截图断定唯一原因。
5. 插件现有 `STM32ProfileManager` 仅有少数硬编码型号，未知型号回退 F103C8；`MakefileGenerator` 生成通用 `.ld`；`FlashConfigDialog` 已有 ST-Link V2/V3 和 CMSIS-DAP 项；`FlashProgrammer` 已有两种 OpenOCD interface 参数，但端到端验证尚缺。`GdbServerManager` 默认为 ST-Link interface；本次烧录需求不自动扩展成完整 CMSIS-DAP GDB 调试需求。
6. CubeIDE linkerdb 插件的 `about.html` 为 ST SLA0048 Rev5/October 2025；分发时须保留脚本版权及随附许可文本并核对实际发布义务。
7. 仓库已有大量预存修改，插件目录在当前 Git 状态中未跟踪。不要清理、重置或混入这些改动。

## 设计决策

以 CubeMX 6.18.0 生成工程的实际 `.ld` 为真值，制作带版本、型号/上下文映射与 SHA-256 的离线脚本包。上下文至少包含核心、安全状态及 CubeMX 提供的 FLASH/RAM 变体；统一使用 CubeMX 默认内存、堆栈和启动布局，并保存生成输入以供复现。用户改动过内存布局的工程使用自己的 CubeMX 脚本。可以按内容去重存储，但复制到项目时保留原字节。每个非 MP 目标必须有明确映射及编译器元数据来源；缺失、歧义或生成失败即覆盖检查失败，不退回 F103C8 或手写脚本。项目已有/用户指定 `.ld` 优先，旧 SmileCodeIDE 自生成脚本只通过明确迁移替换。多核及安全上下文需在 UI 中明确选择。

烧录沿用 OpenOCD：ST-Link V2 使用 `interface/stlink.cfg`，CMSIS-DAP 使用 `interface/cmsis-dap.cfg`；校验设备在线、驱动匹配、脚本存在及 target 配置支持。芯片目录的覆盖率与硬件烧录覆盖率分别报告。

完整设计见 [设计文档](../../docs/superpowers/specs/2026-10-11-stm32-cubemx-catalog-design.md)。

## 下次会话的执行顺序

1. 阅读设计文档和本 walkthrough，检查用户是否要求调整范围；若获准实施，先写分阶段实施计划。
2. 用 CubeMX CLI 在工作区临时目录生成 H743VI 示例工程，比对最终 `.ld` 与 linkerdb 源文件；验证生成命令、依赖固件包、文件命名和是否需要多个上下文。
3. 做全目录枚举与覆盖报告，再生成并打包脚本；不能只复制 317 个源文件就宣称全型号完成。
4. 按测试先行方式改严格型号查找、脚本解析/复制、Makefile、UI 和烧录校验。保留用户工程脚本和仓库预存修改。
5. 跑完整覆盖/哈希检查、Qt 测试、H743VI 真正构建及可用硬件上的两种下载器烧录；对未能验证的硬件明确列出。

## 参考资料

- ST CubeMX CLI: https://dev.st.com/stm32cube-docs/stm32cubemx/6.18.0/en/docs/markup/CubeMX_CLI.html
- ST STM32MP 与 MCU 工程区别: https://wiki.st.com/stm32mpu/wiki/How_to_create_STM32MPU_projects_with_STM32CubeMX
- 先前工作小结：`WORK_SUMMARY_2026-10-10.md`（只作为历史状态参考，不作为本次用户指令）。
