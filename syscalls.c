#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "stm32f411xe.h"

// Well-known variable for the core clock speed in Hz.
// Referenced (but not used) in CMSIS headers.
// Expected by some debugging tools.
uint32_t SystemCoreClock = 16000000;

// This is the first thing called by Reset_Handler after initializing data and bss sections.
// Runs before _init, C++ static initializers and main.
// For microcontrollers using external DRAM, it will need to be initialized here.
// In that case, system_stm32f4xx.c in cmsis_f4 provides an implementation configured by #defines.
void SystemInit(void) {
  // Enable floating-point instructions.
  // This may be removed if the CFLAGS are changed to prevent these instructions from being emitted.
  // Documented in programmer's manual.
  SCB->CPACR = 0xf << 20;

  // Set up the clock for full-speed operation.
  // At startup the MCU always runs from the 16MHz HSI RC oscilator,
  // This is slower than the flash memory latency, and the core fetches instructions directly.

  // At full speed, the core will outrun flash memory latency (but not its bandwidth, as it uses a much wider word size than the core).
  // To ensure reads to happen correctly, we need to add wait states between starting and completing a read.
  // To allow the core to execute at full speed, we enable prefetcing as well as data and instruction caches.
  // Described by Embedded flash memory interface/Read Interface in the reference manual.
  constexpr uint32_t FLASH_CONFIG = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_3WS; // 90-100MHz at 3.3V
  FLASH->ACR = FLASH_CONFIG;
  // Set the core voltage regulator for full-speed operation
  // If running at a lower speed, this can be adjusted to reduce draw
  // This is described in Power Controller/Power Supplies/Voltage Regulator
  PWR->CR = (3 << PWR_CR_VOS_Pos);
  
  // Configure ther PLL to generate the core clock frequency and (optionally) USB clock frequency
  // from the HSE clock, which is usually a crystal oscilator.
  // The PLC operates in three stages, dividing by M, multiplying by N,
  // then dividing by 2(P+1) for the core clock and Q for the USB/SDIO clock.
  // Each the frequency after each stgage must fall into a specific range
  // described in the reference manual under the entry for the PLLCFGR register.
  // If USB is used, the USB/SDIO clock must be at 48MHz, which can restrict the choice of core clock speeds.
  // Note that the representation of this register varries between MCU models (gotcha 194).
  RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC | (4 << RCC_PLLCFGR_PLLQ_Pos) | (0 << RCC_PLLCFGR_PLLP_Pos) | (192 << RCC_PLLCFGR_PLLN_Pos) | (25 << RCC_PLLCFGR_PLLM_Pos); // 96 MHz from 25 MHz, USB compatible
  //RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC | (8 << RCC_PLLCFGR_PLLQ_Pos) | (0 << RCC_PLLCFGR_PLLP_Pos) | (160 << RCC_PLLCFGR_PLLN_Pos) | (20 << RCC_PLLCFGR_PLLM_Pos); // 100 MHz from 25 MHz, no USB
  //RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC | (8 << RCC_PLLCFGR_PLLQ_Pos) | (0 << RCC_PLLCFGR_PLLP_Pos) | (100 << RCC_PLLCFGR_PLLN_Pos) | (4 << RCC_PLLCFGR_PLLM_Pos); // 100 MHz from 8 MHz, no USB (Nucleo board)
  
  // Turn on the HSE oscilator and PLL.
  // Nucleo boards in their stock configuration will also need to set RCC_CR_HSEBYP to make the HSE input-only,
  // as the main MCU shares a crystal with the ST-Link, and the ST-Link provides the oscilator.
  RCC->CR |= RCC_CR_HSEON | RCC_CR_PLLON; // Enable HSE Bypass and PLL;
  // wait until PLL and flash controller are both ready
  while (FLASH->ACR != FLASH_CONFIG) {}
  while (!(RCC->CR & RCC_CR_HSERDY)) {}
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}
  // Switch clock source to PLL and turn on prescalar to run APB1 at half speed.
  RCC->CFGR = RCC_CFGR_PPRE1_DIV2 | (RCC_CFGR_SW_PLL);
  // Wait for clock to switch over
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
  // Update SystemCoreClock to match what we just set, so we can refer to it later.
  SystemCoreClock = 96000000;

  // Enable the SysTick timer, which periodically calls our handler defined below.
  SysTick_Config(SystemCoreClock / 1000);  // Tick every 1 ms
}


// Count the number of times the SysTick handler has been called.
// Useful for delay routines as it lets the MCU sleep for a given number of ticks.
// Variables used to communicate between the main routine and interrupts must be marked volatile
// to prevent reordering optimizations from breaking them.
volatile uint32_t ms_counter = 0;

// Runs once every milisecond.
// Currently just keeps a count, but you can add other periodic tasks to this that don't get their own timer.
// Handlers are weakly declared in startup_xxx.s and referred to by the vector table.
void SysTick_Handler() {
  ms_counter += 1;
}


////////////////////////////////////////////////////////////////////////////////
// Newlib-expected syscalls.
////////////////////////////////////////////////////////////////////////////////
//
// To be useful across a wide variety of platforms, newlib does not implement any hardware or OS access of its own.
// Instead, it uses unix-like syscall functions as an interface to call back to the program using it.
// Here, we implement a minimal set of syscalls which is enough to call malloc and snprintf.
//
// As some library functions are quite genral, many of these syscalls are linked to,
// but not used by the cases we're interested in, and can be safely stubbed.
// Stub implementations are from the newlib documentation at
// https://sourceware.org/newlib/libc.html#Stubs

// Called by __libc_init_array, which is called between SystemInit and main.
// This can be used for additional initialization of things not covered in SystemInit,
// such as setting up the clock when using the provided memoty-controller-initializing SystemInit.
// When not needed, this can be left empty.
void _init(void) {
}

#define STACK_GUARD_SPACE 512

// Called by malloc to initialize and extend the heap.
// We start near the bottom of memory after the static vatiables and grow the heap upwards.
// The stack starts at the top of memory and grows downwards.
// Ported from the suggested minimal implementation in the newlib docs.
caddr_t _sbrk(int incr) {
  extern char __heap_start; // defined by linker script

  static char *heap_end;
  char *prev_heap_end;
  char *this_frame = (char*) &prev_heap_end; // find the stack pointer
 
  if (heap_end == 0) {
    heap_end = &__heap_start;
  }
  prev_heap_end = heap_end;
  if (heap_end + incr + STACK_GUARD_SPACE > this_frame) {
    write(1, "Heap and stack collision\n", 25);
    abort();
  }

  heap_end += incr;
  return (caddr_t) prev_heap_end;
}


// File Descriptors:
//
// Using any of the printf-related functions will link to the file descriptor API,
// as many of them share a common implementation using the FILE struct.
// If only snprintf is used (which targets a provided memory buffer),
// none of these are called and they can all be stubbed, as is seen here.
// Actually implementing _write for fd=1, such as by writing to a UART,
// enables the use of printf for debugging.
// Implementing the full set as well as _open is probably unnecessary unless
// you are attaching a persistent storage driver with a filesystem.

// Blocking write to an open file descriptor.
// If your board contains a UART or other debug output,
// writing to it here will allow the use of printf.
int _write(int fd, char *ptr, int len) {
  (void)fd, (void)ptr, (void)len;
  if (fd == 1) {
    // stdout
  }
  return -1;
}

// implement this if you want scanf to connect to some sort of communications link
int _read(int fd, char *ptr, int len) {
  (void)fd, (void)ptr, (void)len;
  if (fd == 0) {
    // stdin
  }
  return 0;
}

// All file descriptors are considered to be streams
int _fstat(int fd, struct stat *st) {
  (void)fd;
  st->st_mode = S_IFCHR;
  return 0;
}

// Close a file descriptor.
int _close(int fd) {
  (void)fd;
  return -1;
}

int _isatty(int fd) {
  (void)fd;
  return 1;
}



int _lseek(int fd, int ptr, int dir) {
  (void)fd, (void)ptr, (void)dir;
  return 0;
}

// Abort functions:
// newlib uses abort() as a default case in exhaustive switch statements.
// This sends a signal to self, then calls exit.

// We don't have processes
int _getpid(void) {
  return 1;
}
int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  return -1;
}

// Exits by triggering a system reboot.
// If you have a debug output, you might want to write a notification to it here.
void _exit(int status) {
  (void)status;
  NVIC_SystemReset();
}

