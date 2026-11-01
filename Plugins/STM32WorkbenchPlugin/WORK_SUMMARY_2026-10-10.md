# STM32 工作台会话小结（2026-10-10）

## 本次目标

增强编译日志的详细程度与报错定位，并自动扫描本机 GCC、GDB、Make、OpenOCD、J-Link 等工具。

## 已实现

- 构建开始时显示工程目录、GCC 路径、Make 路径和完整 Make 命令；保留 Makefile 输出的每条编译、链接和尺寸命令。
- 按完整输出行解析 GCC 诊断，支持分块到达的输出、可选列号、Windows 盘符路径；构建结束显示错误和警告数量。
- 构建终端为错误、警告、提示、命令和成功消息使用不同颜色；识别到的 `文件:行号` 日志可点击跳到源码。
- 工具扫描扩展到 PATH、常见安装目录、系统安装目录和工具相关环境变量；已保存的 GCC 优先配对同目录 GDB、G++、objcopy、size。单独缺少调试工具时也会补扫。
- Windows 优先使用 `mingw32-make`。自动生成的 Makefile 会引用带引号的编译器命令，兼容安装路径中的空格。
- 工具链配置窗口重新扫描后保留未找到替代项的手工路径，并提示检查诊断报告后保存。

## 验证

- Qt 6.12.0 + 配套 MinGW 13.1 构建 `STM32WorkbenchStandalone.pro` 成功，生成 `build/stm32workbench-verification/release/STM32Workbench.exe`。
- `ToolchainDetectorTest`：6 通过、0 失败。覆盖提示目录中的 GCC/GDB、已保存 GCC 的同目录 GDB、环境变量中的 Make/OpenOCD/J-Link，以及带空格路径的 Makefile 命令。
- 尚未连接实际 STM32 硬件执行烧录或 GDB 会话。当前机器的 PATH 有 GCC、GDB、OpenOCD、Make，未查到 J-Link。

## 下次会话参考

- 实现位于 `Core/BuildManager.*`、`Core/ToolchainDetector.*`、`Core/MakefileGenerator.cpp`、`UI/STM32WorkbenchWidget.*` 和 `UI/ToolchainConfigDialog.cpp`。
- 测试位于 `tests/ToolchainDetectorTest.cpp`，qmake 项目为 `tests/ToolchainDetectorTest.pro`。
- 使用 Qt 6.12 的 qmake 时，必须将 `D:/Soft/Qt/Tools/mingw1310_64/bin` 放在 PATH 前面；系统默认的 MinGW 15.2 与此 Qt 构建不兼容，链接会报 `__imp___argc`。
- 仓库原有大量未提交修改；`Plugins/STM32WorkbenchPlugin/` 整体在当前 Git 状态中为未跟踪目录。本次没有提交或清理这些文件。

## STM32H743VI 构建故障后续修复

- 现场报错：`No rule to make target 'build/startup_armv6-m.S'`。根因是源文件递归扫描包含了 CMSIS DSP 测试套件、多个芯片启动文件和 ARM/IAR 启动文件；Makefile 对大写 `.S` 也没有正确生成对象目标。
- `MakefileGenerator` 现在优先读取项目的 `.uvprojx` 源文件和头文件目录清单，并将 Keil 工程所列的 ARM/IAR 启动文件替换为同名 GCC 版本。普通工程仍按目录扫描，Makefile 已支持 `.S`、`.asm`、`.cc`、`.cxx` 的目标映射与规则。
- 在真实工程 `E:/STM32HUB/CAN/CAN—双机通讯` 上只读扫描和 Make 预演通过；该工程的清单只选择 `startup_stm32h743xx.s` 的 GCC 版本。实际使用 `D:/Toolchains/arm-none-eabi/bin/arm-none-eabi-gcc.exe` 编译并链接 ELF 成功，全部产物写到工作区临时目录，原工程未改动。
- 自动化测试增加到 9 个（含真实工程环境变量触发的案例），并单独运行了真实工程构建案例。测试环境变量：`STM32_WORKBENCH_REAL_PROJECT`、`STM32_WORKBENCH_REAL_GCC`、`STM32_WORKBENCH_REAL_BUILD=1`。
- 独立程序用 Qt 6.12.0 重建成功；插件用 Qt 6.11.1（配套 MinGW 13.1）构建成功，并部署到 `PhudonTools/plugins/` 和 `build/Desktop_Qt_6_11_2_MinGW_64_bit_Release/PhudonTools/release/plugins/`。两个部署位置的 SHA-256 与新构建 DLL 一致。已运行的 IDE 需重新启动才会加载新 DLL。

## 烧录器设备识别与配置增强

- 新增 `Core/ProbeDetector.h/.cpp`。Windows 使用 SetupAPI/CfgMgr32 枚举当前 USB 设备及父设备，Linux 使用 `/sys/bus/usb/devices`；识别 ST-LINK、CMSIS-DAP/DAPLink、SEGGER J-Link/J-Trace，记录名称、厂商、VID/PID、序列号及设备 ID。Windows 复合设备从父 USB 节点读取真实序列号，排除 Windows 自动生成的接口实例号。
- `UI/FlashConfigDialog` 显示当前接入设备、详细信息、刷新按钮和可编辑序列号。保存所选设备与序列号；首次设置且仅接入一台时自动选中并匹配驱动，已有设置保持原选择。断开的已保存设备仍显示为离线，烧录前会检查连接状态。选择与驱动不匹配或没有序列号时给出提示。
- `Core/FlashProgrammer` 将所选序列号分别传给 OpenOCD 的 `adapter serial`、J-Link Commander 的 `-USB`、STM32CubeProgrammer 的 `sn=` 和 st-flash 的 `--serial`。SWD 速率也传给 OpenOCD、CubeProgrammer 与 st-flash。J-Link 批处理参数改为官方的 `-CommandFile`，并启用无界面及遇错退出选项。烧录前验证设备在线、设备类型、序列号及 32 位 Flash 地址；复合 USB 设备不会继续向上误取 Hub 序列号。
- OpenOCD 和 st-flash 当前不提供全片擦除选项；界面会禁用该复选框，后端也会拒绝不受支持的调用。st-flash 自身始终执行写后校验，因此该驱动的校验复选框固定为启用。J-Link 仅在用户选择“烧录后复位并运行”时执行最终复位。
- `tests/ProbeDetectorTest.cpp` 共 11 个测试通过，覆盖 USB 分类、复合设备序列号提取、烧录命令、驱动匹配、地址校验、设备扫描去重、单设备自动选择及已保存离线设备的配置恢复。在当前本机实测识别到一台 J-Trace PRO，VID/PID `1366:1020`，序列号可读取；没有接入 STM32 目标板，尚未执行真实烧录。
- Qt 6.12 独立程序及 Qt 6.11.1 插件均重新构建成功。插件 DLL 已复制到上述两个部署目录，三个 DLL SHA-256 一致；重启 IDE 后加载新版。新构建目录：`build-probe-standalone/`、`build-probe-plugin/`。

## H743VI J-Link 烧录失败修复

- 用户日志显示构建成功（错误 0、警告 0），J-Trace PRO V3 与 Cortex-M7 目标连接正常，失败在 J-Link Commander 解析 `loadbin "路径", 0x08000000`；逗号不符合命令语法。中文工程路径在 J-Link 输出中也出现乱码。
- `FlashProgrammer::jLinkCommandScript()` 现生成 SEGGER 文档支持的 `loadfile "暂存固件" 0x08000000 noreset`，不再生成带逗号的 `loadbin`/`verifybin`。`loadfile` 自带校验；J-Link 原生驱动的“校验”选项在 UI 固定为启用。固件先复制到临时短路径，命令文件按系统本地编码写入，完成或取消后清理临时文件；临时文件创建和启动失败会输出明确错误。
- 新增脚本语法回归测试后，`ProbeDetectorTest` 共 12 项通过。Qt 6.12 独立程序和 Qt 6.11.1 插件重新构建成功；新 DLL 已部署到两个现有插件目录，哈希一致。
- 对用户这次生成的 6672 字节 `stm32h743vi.bin` 做了真实硬件验证：沙箱内无法连接 J-Trace，获准访问设备后用修正后的 J-Link 命令脚本成功执行擦除、编程、校验、复位运行，J-Link 退出码 0。验证脚本与固件的临时副本已从工作区清理。IDE 进程需要重启才能加载新版插件。

## H743VI 烧录后不运行修复

- 根因是通用链接脚本把 H743 的 1024KB 片上 RAM 当成从 `0x20000000` 起连续分布，生成 `_estack=0x20100000`。H743 在 `0x20000000` 的 DTCM 仅有 128KB，其余 RAM 位于其他地址段；J-Link 首次日志中的 `SP=20100000` 因而是无效栈顶。
- `STM32ChipProfile` 新增 `linkerRamSizeBytes`。H743 的总 RAM 容量仍记录为 1024KB，供容量统计使用；默认链接 RAM 改为 `0x20000000` 起的 128KB DTCM，向量表初始 SP 变为 `0x20020000`。
- `MakefileGenerator` 会刷新同名且带 `SmileCodeIDE Auto-Generated Linker Script` 标记的旧链接脚本。用户自行维护或没有该标记的 `.ld` 文件不会被覆盖；链接脚本创建或更新失败会返回明确错误。
- 新增两个回归测试，覆盖 H743 主 RAM 区域与旧自动脚本刷新。`ToolchainDetectorTest` 结果为 10 通过、0 失败、1 跳过；Qt 6.11.1 插件构建成功并复制到 `PhudonTools/plugins/STM32WorkbenchPlugin.dll`。
- 已更新真实示例工程的自动链接脚本并强制全量重建。ELF 向量表首字为 `0x20020000`，固件大小为 text 6664、data 8、bss 1536 字节。
- 已通过 J-Trace PRO V3（S/N 933000892）把修正固件真实烧录到 STM32H743VI。运行 3 秒后读取到 `SP=0x2001FFD8`、`PC=0x08001820`，该地址映射到用户工程 `core_delay.c:115`；`CFSR=0`、`HFSR=0`，GPIOB ODR=`0x00000003`。最后再次执行 `g`，目标保持运行状态。
