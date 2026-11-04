# Annotated starter project for bare-metal STM32 with GCC

A guide to using the GCC toolchain for STM32 microcontrollers,
building standalone, bare-metal code without CubeIDE or CubeMX.

This project is configured for the STM32F411CE, but is adaptable for other MCU models.
All "magic" constants and symbols are annotated with their sources in order to enable this sort of modification.

## Dependencies

You will need the following system packages installed:
* make
* arm-none-eabi-gcc
* arm-none-eabi-newlib
* [stlink](https://github.com/stlink-org/stlink)
* clangd and bear (optional, bear is used to generate clangd configuration from Makefiles)

You will also need the CMSIS headers for your microcontrolller,
which consist of a core module and an MCU-specific module.
These can be fetched for the STM32F4 series using `make cmsis_core` and `make cmsis_f4`.
If using another series, you will need to adjust the URL in the Makefile.

## Documentation

Documentation for the STM32 ecosystem is quite fragmented, and you will need the following volumes. Links refer to the [STM32F411](https://www.st.com/en/microcontrollers-microprocessors/stm32f411/documentation.html), you will need to check the page for your MCU if you are using a different one.

* The [Programmer's Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf). 
  This describes the instruction set, memory model, and core peripherals.
  This is common to all MCUs sharing a core design.
  Control registers for Systick and the NVIC are found here.
* The [Reference Manual](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).
  This describes the peripheral modules and their control registers, as well as the overall architecture of the chip.
* The [Datasheet](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf).
  This contains the GPIO alternate function mapping (used to connect internal peripherals to physical pins) as well as important electrical characteristics.
* The [Errata]().
  This is where corrections appear and supersedes the other three, which are rarely updated.
* The [CMSIS-Core documentation](https://arm-software.github.io/CMSIS_6/latest/Core/index.html).
* The [Newlib](https://sourceware.org/newlib/) documentation and source code. C standard library reference, The source code can come in handy if you implement custom syscalls in order to understand how they are used.



## Additional Resources

* Klein Embedded. [STM32 without CubeIDE](https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/). \
  Also covers how to program without CMSIS and has a C equivalent of the `startup_xxx.s` assembler file.
* Sergey Lyubka. [A bare metal programming guide](https://github.com/cpq/bare-metal-programming-guide).
* Alexander G. Dean. [Embedded Systems Fundamentals with Arm Cortex-M based Microcontrollers A Practical Approach](https://github.com/arm-education/Embedded-Systems-Fundamentals). \
  Note: This assumes the STM32F0 series, but most of the peripherals it covers (DMA and UART are major exceptions).
* EFTON s.r.o. [STM32 gotchas](http://efton.sk/STM32/gotcha/index.html). Covers many aspects of the STM32 chips that are either poorly-explained or easy-to-miss in the 
* [MaskRay Blog](https://maskray.me/blog/).  Covers quite a few gaps in GCC compiler and linker documentation.

