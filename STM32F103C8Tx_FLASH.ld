/*
 * STM32F103C8Tx¨¦????£¤¨¨?????
 * ????-?????¡À????
 * FLASH: 64KB (0x10000)
 * RAM: 20KB (0x5000)
 */

/* ??£¤??¡ê??1 */
ENTRY(Reset_Handler)

/* ????-?????1? */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 20K
}

/* ???????¡è¡ì?¡ã?????1? */
_Min_Heap_Size = 0x200;  /* ????¡ã?????¡è¡ì?¡ã? */
_Min_Stack_Size = 0x400; /* ????¡ã?????¡è¡ì?¡ã? */

/* ????1?¨¨????o??¦Ì */
SECTIONS
{
  /* ??-??-???¨¦??¨¨?¡§?????¡ê?????¦Ì */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* ??¡ê?????¦Ì */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* ??a¨¨¡¥???¡ã?????¦Ì */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* ??¡§?o?¨¨¡ã???¡§¨¦????????¨¦???????¡ã?????¦Ì */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* ??¡ã?????¦Ì */
  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* BSS??¦Ì */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* ??¡§??¡¤????????¦Ì */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Discard unused sections */
  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
