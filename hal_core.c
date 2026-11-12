#include "hal.h"

// Wait for at least ms milliseconds, until the next SysTick
void delay(uint32_t ms) {      
  // This function waits "ms" milliseconds
  uint32_t now = ms_counter;
  // Time when we need to wake up, adding q1 to deal with the fact that we might be near the end of a tick
  uint32_t until = ms_counter + ms + 1;
  // Wait until then, dealing with wraparound.
  // __WFI puts the core to sleep until the next interrupt fires, keeping peripherals active.
  if (now > until) while (ms_counter > now) { __WFI(); } // deal with wraparound
  while (ms_counter < until) { __WFI(); }; // Loop until then
}

// Spin wait for a short amount of time, up to one SysTick interval (1ms).
void wait_us(uint32_t us) {
    uint32_t now = SysTick->VAL;
    uint32_t ticks = (SystemCoreClock / 1000000) * us;
    uint32_t then;
    if (now > ticks) {
        then = now - ticks;
        while (in_range(SysTick->VAL, then, now)) {}
    } else {
        then = now + SysTick->LOAD - ticks;
        while (SysTick->VAL <= now) {}
        while (SysTick->VAL > then) {}
    }
}

// GPIO set-up functions.
// These are not reentrant due to read-modify-write cycles in control registers
// and should not be used from interrupts.
// If reentrancy is needed, they can be rewritten to use LDREX/STREX loops (see programmer's manual).

// Enable clock distribution to a GPIO bank
// This may differ between MCUs.
void enable_gpio_bank(GPIO_TypeDef* bank) {
    uint32_t bank_num = (((uint32_t) bank) - ((uint32_t) GPIOA)) / 0x400;
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN << bank_num);
}

// Configures a GPIO pin an an output
void gpio_set_output(GPIO_TypeDef* bank, uint32_t pin, enum GPIO_OSPEED speed, bool open_drain) {
  enable_gpio_bank(bank);
  // We fully replace the relevant bits as MODER does not start zeroed on all MCUS (gotcha 70)
  bank->MODER = (bank->MODER & ~(3u << (pin * 2))) | (GPIO_MODE_OUTPUT << (pin * 2));
  bank->OSPEEDR &= (bank->OSPEEDR & ~(3u << (pin * 2))) | (speed << (pin * 2));
  bank->OTYPER = (bank->MODER & ~(1u << pin)) | (((uint32_t) !!open_drain) << pin);
}

// Configures a GPIO pin as an input
void gpio_set_input(GPIO_TypeDef* bank, uint32_t pin, enum GPIO_PUPD pullup) {
  enable_gpio_bank(bank);
  bank->MODER = (bank->MODER & ~(3u << (pin * 2))) | (GPIO_MODE_INPUT << (pin * 2));
  bank->PUPDR = (bank->PUPDR & ~(3u << (pin * 2))) | (pullup << (pin * 2));
}

// Configures a GPIO pin to be controlled by an alternate function.
// Speed can be set to zero if it is to be an input.
void gpio_set_af(GPIO_TypeDef* bank, uint32_t pin, uint32_t af_num, enum GPIO_OSPEED speed, bool open_drain) {
  enable_gpio_bank(bank);
  // We fully replace the relevant bits as MODER does not start zeroed on all MCUS (gotcha 70)
  bank->MODER = (bank->MODER & ~(3u << (pin * 2))) | (GPIO_MODE_AF << (pin * 2));
  bank->OSPEEDR &= (bank->OSPEEDR & ~(3u << (pin * 2))) | (speed << (pin * 2));
  bank->OTYPER = (bank->MODER & ~(1u << pin)) | (((uint32_t) !!open_drain) << pin);

  // AFR is an array of 4-byte values
  volatile uint32_t* afr_word = bank->AFR + (pin >> 3);
  uint32_t afr_shift = (pin & 7) * 4;
  *afr_word = (*afr_word & ~(15UL << afr_shift)) | (af_num << afr_shift);
}

// Configures external interrupt controller to monitor a pin.
// Does not configure the GPIO pin itself, use gpio_set_input for that.
// The IRQ will still need to be enabled some of those group multiple pins together.
// In STM32, the pin number also corresponds to the EXTI line, and each line can be selected between banks.
void gpio_set_exti(GPIO_TypeDef* bank, uint32_t pin, bool rising, bool falling) {
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  uint32_t bank_num = (((uint32_t) bank) - ((uint32_t) GPIOA)) / 0x400;

  // SYSCFG->EXTICR controlls a multiplecer connecting GPIO inputs to the edce detectors of EXTI.
  volatile uint32_t* crword = SYSCFG->EXTICR + (pin >> 2);
  unsigned crpos = (pin & 3u) * 4u;
  *crword = (*crword & ~(0xfu << crpos)) | ((bank_num & 0xfu) << crpos);

  if (rising) {
    EXTI->RTSR |= (1u << pin);
  }
  if (falling) {
    EXTI->FTSR |= (1u << pin);
  }
  EXTI->IMR |= (1u << pin);
}