# Specifies the instruction set used by the compiler.
# See the functional descripion in datacheet as well as
# https://support.arm.com/documentation/100166/0001/Functional-Description/Processor-features
# or the corresponding version for your CPU core.
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
# Full warnings to help eliminate footguns.
# Wdouble-promotion is especially relevant for STM32F4 since it has single-precision instructions but not double-precision
# Wconversion requires explicit downcasting between numeric types.
CFLAGS += -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Wformat-truncation -Wconversion -fno-common
# Search for headers from project root, an include directory (if available), and CMSIS dependencies
CFLAGS += -I. -Iinclude -Icmsis_core/CMSIS/Core/Include -Icmsis_f4/Include
# Additional compiler flags:
# g3 puts debug info into the elf file
# Os enables most compiler optimizations and tunes for code size. Does not unroll loops
# ffunction-sections and fdata-sections allow the linker to rearrange functions and variables
# within a translation unit and allows gc-sections to eliminate dead code
CFLAGS += -g3 -Os -ffunction-sections -fdata-sections $(EXTRA_CFLAGS)

# Linker flags.
# Use our custom linker script and do not add default start files (we explicitly add the correct one to SOURCES).
# Use newlib's libc_nano instead of the full libc.
# gc-sections discards symbols that are not transitively reachable from the vector table.
LDFLAGS = -Tlink.ld -nostartfiles -nostdlib -lc_nano -lm -lgcc -Wl,--gc-sections -Wl,-Map=$@.map


# Project source files. Add any that you create to this list.
SOURCES = main.c syscalls.c
# Vector table and Reset_Handler defined in CMSIS.
# This must match your MCU as vector table layouts differ between models.
SOURCES += cmsis_f4/Source/Templates/gcc/startup_stm32f411xe.s

# Project header files. We need to rebuild if these change.
HEADERS = hal.h


# Default target when make is called with no args.
build: firmware.bin

# Compile all sources into an ELF executable.
# On small projects, split compilation complicates things for very little benefit.
firmware.elf: cmsis_core cmsis_f4 link.ld Makefile $(SOURCES) $(HEADERS)
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

# Copies loadable sections of firmware into a contiguous raw binary.
# This gets flashed onto the MCU.
firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary $< $@


# Fetch CMSIS dependencies
# Create a long-lived partial clone that can be updated using fetch and checkout.
# --depth 1 may be used if builds are done in CI only.
# see https://github.blog/open-source/git/get-up-to-speed-with-partial-clone-and-shallow-clone/
cmsis_core:
	git clone -q --filter=blob:none -b v6.4.0 https://github.com/ARM-software/CMSIS_6 $@
cmsis_f4:
	git clone -q --filter=blob:none -b v2.6.8 https://github.com/STMicroelectronics/cmsis_device_f4 $@


# flash the MCU
# The address here refers to the start of flash, and must match the datasheet and linker script.
flash: firmware.bin
	st-flash --reset write $< 0x08000000

clean:
	rm firmware.* *.o *.map