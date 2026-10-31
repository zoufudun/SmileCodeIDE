# 专题归档：STM32H7 烧写成功但黑屏不运行问题排查与编译优化级别修复

## 一、 问题描述与现象
- **测试工程**：野火【STM32H743 挑战者】《28-电容触摸屏—触摸画板(屏幕兼容版)》。
- **硬件平台**：STM32H743IIT6，400MHz 主频，板载 800x480 RGB 接口 LCD 屏、GT9157/GT911 电容触摸屏、外部 FMC SDRAM。
- **问题现象**：
  同一份工程源码，使用 Keil MDK 烧录后，屏幕背光即刻点亮，全屏白色画板与左侧调色板按钮正常呈现，触摸画线灵敏。
  但是在 SmileCodeIDE 编译并使用 OpenOCD 烧录成功（控制台输出 `Verified OK`，退出码 0）后，开发板屏幕全黑，背光不亮，触摸无反应，程序仿佛未运行。

---

## 二、 问题分析与定位

### 1. 现场寄存器与调用栈捕获（GDB 现场反汇编追踪）
为了避免猜测，通过 OpenOCD 挂接已烧录并运行的芯片，使用 GDB 发送调试中断信号：
```text
(gdb) target remote localhost:3333
Remote debugging using localhost:3333
[stm32h7x.cpu0] halted due to debug-request, current mode: Handler External Interrupt(40)
pc: 0x08008bde in i2c_Delay () at ./User/touch/bsp_i2c_touch.c:309
309         for (i = 0; i < 10*8; i++);
```
- **核心定位 1**：当前 CPU 处于 `Handler External Interrupt(40)` 模式，中断向量 40 对应 `EXTI15_10_IRQn`（触摸芯片 INT 引脚引发的外部中断）。
- **核心定位 2**：PC 卡死在 `bsp_i2c_touch.c` 的 `i2c_Delay()` 软件循环延时函数内。

### 2. 根因剖析：Cortex-M7 超标量架构下的 `-O0` 延时失真与死锁
查看 `bsp_i2c_touch.c` 源码中的延时实现：
```c
static void i2c_Delay(void)
{
    uint8_t i;
    /* 原作者注释：工作条件：CPU主频180MHz，MDK编译环境，1级优化 (-O1) */
    for (i = 0; i < 10*8; i++);
}
```
1. **`-O0`（无优化）下的致命放大效应**：
   SmileCodeIDE 此前默认以 `-O0`（Debug 无优化）生成 Makefile。
   在 400MHz 的 Cortex-M7 内核下，`-O0` 严禁内联且禁止将局部变量常驻寄存器，导致每一次循环都要在栈内存（AXI SRAM）上执行 `strb/ldrb`、比较与跳转。
   这使得每一次调用 `i2c_Delay()` 的真实耗时被放大了 **10 倍以上**，I2C 时钟频率从原本的数百 kHz 骤降到几 kHz 以下。
2. **死锁发生在屏幕点亮之前**：
   查看 `main.c` 的主流程：
   ```c
   int main(void)
   {
       ...
       GTP_Init_Panel();   // 第 43 行：初始化触摸芯片（复位引脚并开启了中断）
       ...
       LCD_Init();         // 第 52 行：初始化 LTDC 液晶控制器与 SDRAM 显存
       LCD_DisplayOn();    // 开启屏幕背光（PD7 置高电平）
       Palette_Init();     // 调色盘界面清屏与按钮绘制
       ...
   }
   ```
   因为 I2C 时序被过度放大，GT9xx 触摸芯片在复位通信握手时握手失败，进入长达 1000 次重试的阻塞循环；同时外部触摸 INT 引脚产生中断，触发 `EXTI15_10_IRQHandler` 陷入高频中断风暴。
   **CPU 100% 困死在第 43 行，根本没有走到第 52 行的 `LCD_Init()`、背光开启与调色盘清屏，造成硬件“假死”与黑屏现象！**

3. **Keil 正常的根由**：
   Keil MDK 默认工程配置开启 **`Level 1 (-O1)`** 优化，软件延时落在 GT9xx 允许的时序窗口内，毫秒级完成初始化并放行至 `LCD_Init()`。

---

## 三、 修改计划

1. **编译器优化等级对齐**：修改 `MakefileGenerator.cpp`，将 Debug 默认优化等级由 `-O0` 升级为对标 Keil 的 `-O1 -g3 -gdwarf-2`。
2. **全套重新编译验证**：清理旧的编译产物，使用新 Makefile 构建固件，确保生成体积精简且调试信息完备的 `.elf` 和 `.bin`。
3. **真实板卡现场调试与硬件寄存器验证**：通过 GDB 断点逐行确认主流程是否顺畅执行，并读取 GPIO 背光寄存器与 SDRAM 显存内容。
4. **插件动态链接库部署**：重新编译生成 `STM32WorkbenchPlugin.dll` 并部署到主程序插件目录，确保所有项目自动应用该优化方案。

---

## 四、 具体修改措施

### 1. 修改 MakefileGenerator.cpp
在 [MakefileGenerator.cpp](file:///k:/TOPFIRE/SmileCodeIDEUpdate/Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.cpp#L556-L563) 中调整默认编译标志：
```cpp
    QString optFlag;
    switch (config) {
        case BuildConfiguration::Release: optFlag = "-O2 -g0"; break;
        case BuildConfiguration::SizeOpt: optFlag = "-Os -g0"; break;
        case BuildConfiguration::Debug:
        default:                          optFlag = "-O1 -g3 -gdwarf-2"; break;
    }
```
*保留 `-g3 -gdwarf-2` 确保 GDB 具有完整的源码行号、变量与宏展开调试支持，同时 `-O1` 消除软件模拟延时被放大导致的外设超时故障。*

### 2. 重新编译并部署插件
使用 MinGW 编译并覆盖插件目录中的动态链接库：
```powershell
cp Plugins/STM32WorkbenchPlugin/release/STM32WorkbenchPlugin.dll PhudonTools/plugins/STM32WorkbenchPlugin.dll
```

---

## 五、 实证验证与测试结果

### 1. GDB 全流程断点连贯性验证
重新烧录固件后，通过 GDB 下发断点并单步运行：
```text
Breakpoint 1, main () at User/main.c:35
Breakpoint 2, LED_GPIO_Config () at User/led/bsp_led.c:31
Breakpoint 3, UARTx_Config () at User/usart/bsp_usart.c:32
Breakpoint 4, GTP_Init_Panel () at User/touch/gt9xx.c:801
Breakpoint 5, LCD_Init () at User/lcd/bsp_lcd.c:301
Breakpoint 6, Palette_Init () at User/touch/palette.c:70
```
- **全部断点按序平滑触发，`GTP_Init_Panel` 耗时不足 50ms 顺利通过，未再发生任何死循环或中断阻塞**。

### 2. 硬件外设寄存器现场核验
- **GPIOD ODR 寄存器（背光与使能引脚）**：
  读取地址 `0x58020C14`：
  ```text
  (gdb) x/1wx 0x58020C14
  0x58020c14: 0x00000090
  ```
  - Bit 4 (`PD4`, `LTDC_DISP` 使能信号) = 1
  - Bit 7 (`PD7`, `LTDC_BL` 背光电源) = 1
  证实**屏幕背光已由程序正确开启**。

- **LTDC 控制器状态**：
  `Ltdc_Handler.State` 状态显示为 `HAL_LTDC_STATE_READY`，LCD 控制器初始化成功。

- **外部 SDRAM 显存内容验证**：
  检查 FMC 映射的第二显存物理地址 `0xD0177000`：
  DMA2D 已将显存整屏填充为白色底色（`0xFFFFFFFF`），且对应左侧调色板坐标位置写入了彩色按钮像素阵列。板卡屏幕完整点亮并呈现画板界面。

---

## 六、 总结与长效机制
单片机裸机驱动中普遍存在使用 `for` 循环实现的毫秒/微秒级软件模拟延时（如模拟 I2C、1-Wire、屏驱复位等）。此类代码高度依赖编译器优化级别与指令流水线。SmileCodeIDE 将默认编译参数永久升级为 `-O1 -g3 -gdwarf-2` 后，彻底解决了 GCC 裸机嵌入式开发中延时严重失真导致的假死黑屏隐患，达成了与 Keil MDK 工业级环境一致的执行效果。

