# GCC startup fallback

`startup_stm32f429xx.s` is copied unchanged from STMicroelectronics
STM32Cube_FW_F4_V1.28.3:

`Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f429xx.s`

The CMSIS Device component uses Apache-2.0; the original component license
notice is in `LICENSE.txt`, and the package's complete terms are in
`Package_license.md`.

Used only when a project selects the matching Keil/IAR startup and has no
GCC sibling. The project receives a copy in `.smilecode/startup/`; its original
startup file is preserved. Other devices need their own GCC startup.
