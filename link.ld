/*
No effect on the compiled firmware (the MCU uses the vector table), but tells analysis tooling where the entry point is.
Reset_handler (and its vector table entry) are defined in cmsis_f4/.../startup_xxx.s
*/
ENTRY(Reset_Handler);

/*
Memory layout of the MCU.
Addresses of memory segments are described in the Memory Mapping section of the datasheet.
Their capabilities are listed in the System Architecture section of the reference manual.
*/
MEMORY {
  flash(rx)  : ORIGIN = 0x08000000, LENGTH = 512k
  sram(rwx) : ORIGIN = 0x20000000, LENGTH = 128k /* Main SRAM on bus matrix: general purpose */
  /* Some processors might have additional memory blocks, such as CCMRAM to be added here. */
}

/*
Start the stack at end of SRAM. This symbol is used by the vector table in startup_xxx.s.
On MCUs with CCMRAM, you might want to put the stack there for efficiency.
Doing so will prevent the DMA from accessing stack memory. (see STM32 Gotcha 5)
*/
_estack     = ORIGIN(sram) + LENGTH(sram);    /* stack points to end of SRAM */

/*
Force the inclusion of floating-point formatting for printf and friends.
If this is left out, printf will default to an integer-only version to save program space.
*/
EXTERN(_printf_float);

/*
Unfortunately, input sections emitted by GCC are not listed in normative documentation,
but do appear in several examples.
They can also be examined by runing objdump -h on a non-trivial object file.
*/
SECTIONS {
  /*
  The MCU requires the vector table to be the first thing in flash when booting in normal mode.
  KEEP also defines this table as a root for gc-sections, including all symbols transitively referred from it.
  */
  .vectors : { KEEP(*(.isr_vector)) } > flash

  /* Executable program code */
  .text : { *(.text*) } > flash

  /*
  C++ initializer table (not used for plain C).
  The symbols defined here are used by newlib's __libc_init_array, which is called by Reset_handler
  */
  .preinit_array : {
    __preinit_array_start = .;
    KEEP(*(.preinit_array))
    KEEP(*(.init_array))
    __preinit_array_end = .;
  } > flash
  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    __init_array_end = .;
  } > flash

  /* Read-only data in flash. Used for string literals and other  */ 
  .rodata : { *(.rodata*) } > flash

  /*
  Initialized static variables. 
  These get allocated to sram giving the symbols addresses there)
  and an additional copy of this block gets written to flash (using the AT command).
  _sdata, _edata, and _sidata are used by Reset_Handler to copy initialization data from flash to ram on startup.
  */
  .data : {
    _sdata = .;
    *(.data SORT(.data.*))
    _edata = .;
  } > sram AT > flash
  _sidata = LOADADDR(.data);

  /*
  Zeroed static variables.
  _sbss and _ebss are used by Reset_Handler to zero this section on startup.
  */
  .bss : {
    _sbss = .;
    *(.bss SORT(.bss.*))
    *(COMMON) /* Not needed with current CFLAGS, but thid section is emmitted in compiling with -fcommon (default in older C versions) */
    _ebss = .;
  } > sram

  /*
  Space after the static variables is free to allocate.
  The heap will grow upwards from this point, while the stack grows down from the top of sram.
  __heap_start is used by out _sbrk function, which is called by newlib's malloc.
  */
  . = ALIGN(8);
  __heap_start = .;
}
