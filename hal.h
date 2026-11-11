//#include <stdint.h>
#include "stm32f411xe.h"


// Hardware Abstraction Layer
//
// Unlike the STM HAL library, this does not try to do everything.
// Many peripherals are extremely configurable and can work in many different ways,
// so it can be quite helpful to create abstractions for the sprcific ways we use them.
// Some basic timing and GPIO functionality is introduced here,
// with longer functions defined in hal_core.c.
// Add functions here, or put function definitions for more complex preipherals in their own c files.
//
// A number of functions (especially those that are often called with constant values)
// are defined as static inline to enable fusion, letting the compiler vastly simplify them.


// Bit-banded pointers to the IO memory region.
// The pointer returned from this acts as an alias referring to
// a single bit of the target register.
// This is not valid on arbitrary targets and must only be applied to targets on
// AHB1, APB1, and APB2 (on the F411).
// See the bit-banding section in the programmer's manual for the formula and range,
// and the memory map section of reference manual to see which busses fall onto that range.
static inline bool* BITBAND_IO(uint32_t* reg, uint32_t bit) {
    intptr_t offset = reg - ((uint32_t*) PERIPH_BASE); // in words
    uint32_t* alias = ((uint32_t *) PERIPH_BB_BASE) + (offset * 32) + bit;
    return (bool*) alias;
}

////////////////////////////////////////////////////////////////////////////////
// Timing functions

// defined in syscalls.c, incremented in SysTick handler
extern volatile uint32_t ms_counter;
// Wait for at least ms milliseconds, until the next SysTick
void delay(uint32_t ms);
// Spin wait for a short amount of time, up to one SysTick interval (1ms).
void wait_us(uint32_t us);

////////////////////////////////////////////////////////////////////////////////
// GPIO functions

// Atomically write to a single GPIO pin using the BSRR,
// Which allows enables partial writes to ODR.
// On some MCUs (but not all, see gotcha 107), bit-banding on ODR will also work, but is less portable.
static inline void gpio_write(GPIO_TypeDef* bank, uint32_t pin, bool val) {
  bank->BSRR = 1ul << (pin + (val ? 0ul : 16ul));
}

static inline bool gpio_read(GPIO_TypeDef* bank, uint32_t pin) {
  return !!(bank->IDR & (1 << pin));
}

// GPIO mode enum, used in GPIO_MODER_MODERy fields.
// Found in reference manual, but missing from CMSIS headers
enum GPIO_MODE: uint32_t {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG};

// For GPIO_PUPDR_PUPDRy fields
enum GPIO_PUPD: uint32_t {GPIO_PUPD_NONE, GPIO_PUPD_UP, GPIO_PUPD_DOWN};

// GPIO Speed enum, used in GPIO_OSPPED_OSPPEDy fields.
// This controls the impedance of the pin (which is driven by multiple transistors in parallel) when in output mode,
// and can be used to control signal slew rates in order to minimize EMI.
// The timings given are from the I/O AC Characteristics of the Datasheet
// and refer to minimaly-loaded (30-50pF) conditions.
// Pins with a load drawing significant current should use GPIO_SPEED_FULL (Gotcha 84).
enum GPIO_OSPEED: uint32_t {GPIO_OSPEED_100NS, GPIO_OSPEED_10NS, GPIO_OSPEED_6NS, GPIO_OSPEED_FULL};


// GPIO set-up functions defined in hal_core.c

void enable_gpio_bank(GPIO_TypeDef* bank);
void gpio_set_output(GPIO_TypeDef* bank, uint32_t pin, enum GPIO_OSPEED speed, bool open_drain);
void gpio_set_input(GPIO_TypeDef* bank, uint32_t pin, enum GPIO_PUPD pullup);
void gpio_set_af(GPIO_TypeDef* bank, uint32_t pin, uint32_t af_num, enum GPIO_OSPEED speed, bool open_drain);
void gpio_set_exti(GPIO_TypeDef* bank, uint32_t pin, bool rising, bool falling);

////////////////////////////////////////////////////////////////////////////////
// General utilities

static inline bool in_range(uint32_t x, uint32_t lower, uint32_t upper) {
  return x > lower && x < upper;
}

