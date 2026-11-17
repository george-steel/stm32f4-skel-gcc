# Annotated starter project for bare-metal STM32 with GCC

A guide to using the GCC toolchain for STM32 microcontrollers,
building standalone, bare-metal code without CubeIDE or CubeMX.

This project provides an annotated starter program,
implementing a multi-speed blinky (demonstrating input, output, and the SysTick timer).
Additionally, it provides a minimal set of dummy syscalls sufficient for Newlib,
and configures the MCU clock to run at full speed.
Although these steps are not needed for a blinky, they are required by most complex applications.

This project is configured for the WeAct Studio mini STM32F411CE board,
but is intended to be adaptable to other STM32 boards and MCUs.
All "magic" constants and symbols are annotated with their sources in order to enable this sort of modification.
The target board has 25MHz HSE crystal, an LED on PC13, and a button between PA0 and ground.
Pin assignments can be changed in `main.c` and the crystal frequency can be changed in `SystenInit` in `syscalls.c`

The program can be built by running `make` to build `firmware.bin` and
loaded to the device using `make flash` with an ST-Link device or `make dfu` with a direct USB connection and BOOT0 switch.

For those coming from the Atmel/Arduino ecosystem
(which had a single company producing the entire MCU design and `avr-gcc` toolchain, with tight integration),
the embedded ARM ecosystem can appear rather fragmented.
MCUs are designed by combining a licensed core with modular peripherals (often themselves licensed) using standard bus interfaces,
and there are far more distinct designs than there are for 8-bit MCUs.
These fragmented designs are reflected by fragmented toolchains, with different parts developed by different companies,
needing extensive configuration to be used with any specific MCU.

This project attempts to provide a guide through that fragmentation,
providing all necessary configuration and dependencies for a simple project,
annotated with explanations and citations so you can adjust it to fit your STM32 MCU of choice.


## System Dependencies

You will need the following system packages installed:
* `make`
* `arm-none-eabi-gcc`
* `arm-none-eabi-newlib`
* [`stlink`](https://github.com/stlink-org/stlink) or `dfu-util` (depending on how your board is connected for flashing)
* `clangd` and `bear` (optional)
* an editor of your choice

`arm-none-eabi-gcc` is a cross-compiling variant of GCC which targets 32-bit ARM microcontrollers,
using a calling convention suitable for NVIC-called interrupt handlers.
It is called by `make`, which defines configuration flags to select the core variant used
(different cores implement different subsets of the arm32 instructions).
This generally also includes binutils (`ld`, `objdump`, `objcopy`, and friends) in addition to `gcc` itself.

`arm-none-eabi-newlib` contains a variety of precompiled versions of [Newlib](https://sourceware.org/newlib/),
a compact version of the C standard library which does not assume a specific host OS,
configured for various ARM cores.
This it automatically included in the header and library search paths of `arm-none-eabi-gcc`.
The [ARM-recommended](https://developer.arm.com/community/arm-community-blogs/b/embedded-and-microcontrollers-blog/posts/shrink-your-mcu-code-size-with-gcc-arm-embedded-4-7) configuration,
known as Newlib-nano (consisting of `libc_nano.a` and `libm.a`),
omits a number of features (such as `wchar_t` support) that have large implementations and are generally irrelevant to MCU development,
and makes others (such as floating-point support in `printf`) optional using weak linkage.

To upload firmware to the MCU, we can use either `stlink` or `dfu-util`.
`stlink` (which includes the `st-flash` tool as well as a GUI) connects to the MCU over ARM SWD (Serial Wire Debug) protocol using an ST-Link device
(which can either be a standalone unit, or the top section of a Nucleo board), and can be used with all STM32 MCUs.
`dfu-util` connects to the MCU directly over USB, which requires that the MCU has a USB interface.
Using this requires that the MCU be put into an alternate boot mode (usually by triggering Reset with the BOOT0 pin pulled high),
which loads a USB-DFU bootloader from an internal, factory-programmed ROM.

`clangd` is a useful static analysis tool for C and C++ and has integrations for many editors,
and can be automatically configured by using `bear` to monitor a run of a build system.
To generate `clangd` configuration, run `make clean && bear -- make`.

## CMSIS Headers

In addition to the build tools and C library, you will also need the CMSIS headers for your microcontrolller.
These define an interface for microcontroller-specific functionality, and are divided into several modules.
Necessary modules for the STM32F411 will be fetched by make during first build (adjust the Makefile if you are using another MCU).

[CMSIS Core](https://github.com/ARM-software/CMSIS_6) is published by ARM and provides headers to let C programs access the functionality of ARM cores.
These headers include core register definitions (for the NVIC, SCB, and SysTick), utility functions to control these systems,
and intrinsics for instructions that the C compiler does not emit directly.

For each series of STM32 MCU, there is also a device component (such as [cmsis-device-f4](https://github.com/STMicroelectronics/cmsis-device-f4)),
which defines a standard header for each MCU in the series.
This header transitively includes the correct set of headers from CMSIS Core,
as well as providing register definitions for all the MCU's peripherals,
allowing register access by name instead of by address.

The CMSIS device component also provides a startup file (described below under program structure),
which defines the vector table and provides a boot routine to initialize memory and call main.

## Documentation

Unlike some MCUs (such as the Atmel ATMEGAx8), which have a single, comprehensive manual,
documentation for the STM32 ecosystem is quite fragmented.
STM32 MCUs are designed as [interconnected systems of modules](http://efton.sk/STM32/gotcha/g25.html) (a System on Chip) designed by different companies,
and the documentation s[omewhat reflects this](http://efton.sk/STM32/gotcha/g67.html).

You will need the following volumes.
Links refer to the [STM32F411](https://www.st.com/en/microcontrollers-microprocessors/stm32f411/documentation.html),
you will need to check the page for your MCU if you are using a different one.

* The [Programmer's Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf). 
  This describes the instruction set, memory model, and core peripherals.
  This is common to all MCUs sharing a core design.
  Control registers for Systick and the NVIC are found here.
* The [Reference Manual](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).
  This describes the peripheral modules and their control registers, as well as the overall architecture of the chip.
* The [Datasheet](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf).
  This contains the GPIO alternate function mapping (used to connect internal peripherals to physical pins) as well as important electrical characteristics.
* The [Errata](https://www.st.com/resource/en/errata_sheet/es0287-stm32f411xcxe-device-errata-stmicroelectronics.pdf).
  This is where corrections appear and supersedes the other three, which are rarely updated.
* The [CMSIS-Core documentation](https://arm-software.github.io/CMSIS_6/latest/Core/index.html).
* The [Newlib](https://sourceware.org/newlib/) documentation and source code. Covers the C standard library.
  The source code can come in handy if you implement custom syscalls, to understand exatly how they are used.
* EFTON's [STM32 gotchas](http://efton.sk/STM32/gotcha/index.html).
  This third party resource clarifies many aspects of STM32 MCUs that are unclear or hard to find in the official documentation.
  It is referred to quite a few times here and is worth searching when dealing with unexpected behaviour.


## Program Structure

### `Makefile`

This lists the source files and defines the build process,
calling the compiler when you run `make` to build the firmware.
If you add a file, you will need to add it to `SOURCES` or `HEADERS`.
This also defines the CFLAGS to use the instruction set of the Cortex-M4F core (as used by the STM32F4 series).
If you use another series, this will need to be adjusted, as well as the include path for the CMSIS headers.
Note that all CFLAGS are defined explicitly (instead of linking to `.specs` files) for `clangd` compatibility.

### `link.ld`

This linker script defines the MCU's memory structure,
and determines how compiled code is laid out in flash and variables are allocated in RAM.
Unlike Atmel MCUs, which have implicit linker scripts baked into `avr-gcc`,
the `arm-none-eabi-gcc` does not include any for bare-metal MCUs.

The linker script places the vector table at the start of flash, where the MCU expecets it to be at boot,
as well as defining a number of symbols used by the startup routine.
If you use an MCU with a different memory layout (such as a differing amount of RAM), you will need to edit this.
MCUs with CCMRAM might need significant changes (see a comment in the file for more details).

If you require floating-point support for `printf` and friends, you will need to uncomment a lcommand to link to it, as Newlib uses a smaller, integer-only version by default.

### `cmsis_f4/Source/Templates/gcc/startup_stm32f411xe.s`

This CMSIS-provided file (fetched during the first build) defines the vector table (defining startup and interrupts) and `Reset_Handler`,
which initializes memory and eventually calls `main()`.
This is MCU-specific and you will need to change the path in the Makefile if you use a different one.
It also declares a weak symbol for each of the other interrupt handlers. If you implement one, it will be added to the vector table.

Note that these files assume a single section each for initialized and zeroed static variables.
If you wish to split them between main SRAM and CCMRAM in the linker script (on an MCU with such a layout),
you will need to copy this file into your main repository and edit the initialization sequence in `Reset_Handler`.

### `syscalls.c`

This file defines system calls that Newlib and the startup routine hook into.

Of particular importance is `SystemInit`, which runs before `main` (and before static constructors if C++ is used).
In this program, the function is used to enable the FPU (allowing floating-point instructions to be used) and configure the clock and SysTick timer.

The clock initialization is both MCU- and board-specific, as it depends on the frequency of the HSE crystal.
It currently assumes a core clock of 96 MHz (allowing USB) and a 25MHz crystal.
Several examples for other STM34F411 board configurations are also given,
and comments explain how to update this for other MCUs using the reference manual.

The SysTick timer is configured to fire every 1ms and simply increment a counter (which can be used for delay loops).
Other functionality can be added if needed to `SysTick_Handler` here.

Of the Newlib syscalls, `_sbrk` is of particular importance, as it defines the heap used by `malloc`.
The implementation provided here starts the heap from `__heap_start` (defined in the linker script to appear after the static variables),
and grow upward (the stack starts at the top of RAM and grows downward).

If you are using a board with some sort of communication interface (such as a serial port),
you might want to implement `_write` and `_read` to send and receive data on it,
as these will enable the use of `printf` and `scanf` respectively.
The stub implementations provided are sufficient to use `snprintf`.

### `hal.h` and `hal_core.c`

The files are a good place to create abstractions around the peripherals used in your project, as well as the hardware they connect to.
Currently, this defines some basic timing (using SysTick) and GPIO functionality, which is useful in almost all projects.
These files can also be a good place to add your own abstractions,

This is not intended to be a general-purpose abstraction around all peripherals, as many of them have a huge number of different operating modes,
and abstractions specific to only those used by the project can be far simpler than general ones.

### `main.c`

This defined the `main()` routine, which is the entry point for "normal" operation.
Currently, this sets up the IO pins and interrupts, then enters an infinite loop providing the blinky behaviour.

The interrupt handler for the button is also defined here, as well as the variables these two routines used to communicate.


## Additional References Used

Although not cited directly, the following resources were used and may be helpful to you.

* Klein Embedded. [STM32 without CubeIDE](https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/). \
  Also covers how to program without CMSIS and has a C equivalent of the `startup_xxx.s` assembler file.
* Sergey Lyubka. [A bare metal programming guide](https://github.com/cpq/bare-metal-programming-guide).
* Alexander G. Dean. [Embedded Systems Fundamentals with Arm Cortex-M based Microcontrollers A Practical Approach](https://github.com/arm-education/Embedded-Systems-Fundamentals).
  Note: This assumes the STM32F0 series, but most of the peripherals it covers (DMA, GPIO AFR, and UART are major exceptions) behave similarly to the F411.
* [MaskRay Blog](https://maskray.me/blog/). Covers quite a few gaps in GCC compiler and linker documentation.

