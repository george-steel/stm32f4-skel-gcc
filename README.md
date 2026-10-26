# Annotated starter project for bare-metal STM32 with GCC

A guide to the build and boot systems for developing for STM32 microcontrollers without CubeIDE.

This project is configured for the STM32F401RE, but is adaptable for other MCU models.
All "magic" constants and symbols are annotated with their sources in order to enable this sort of modification.

## Dependencies

You will need the following system packages installed:
* arm-none-eabi-gcc
* arm-none-eabi-newlib
* [stlink](https://github.com/stlink-org/stlink)
* bear (optional, used to generate clangd configuration from Makefiles)



## Resources

* Klein Embedded. [STM32 without CubeIDE](https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/). \
  Also covers how to program without CMSIS and has a C equivalent of the `startup_xxx.s` assembler file.
* Sergey Lyubka. [A bare metal programming guide](https://github.com/cpq/bare-metal-programming-guide).
* Alexander G. Dean. [Embedded Systems Fundamentals with Arm Cortex-M based Microcontrollers A Practical Approach](https://github.com/arm-education/Embedded-Systems-Fundamentals). \
  Note: This assumes the STM32F0 series, but most of the peripherals it covers (DMA and UART are major exceptions).
* EFTON s.r.o. [STM32 gotchas](http://efton.sk/STM32/gotcha/index.html). Covers many aspects of the STM32 chips that are either poorly-explained or easy-to-miss in the 
* [MaskRay Blog](https://maskray.me/blog/).  Covers quite a few gaps in GCC compiler and linker documentation.

