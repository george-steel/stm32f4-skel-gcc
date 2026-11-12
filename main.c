#include "hal.h"
#include "stm32f411xe.h"

const uint32_t flash_delays[] = {500, 250, 125};
static volatile uint32_t flash_speed = 0;

// Using a key connected to PA0. You will need to change this to match your board.
// Some EXTI lines have a dedicated IRQ, but others are grouped together.
// Handlers are weakly declared in startup_xxx.s and referred to by the vector table.
void EXTI0_IRQHandler() {
  // EXTI handlers are level-triggred by EXTI_PR, even for IRQs with just a single line.
  if (EXTI->PR & EXTI_PR_PR0) {
    flash_speed = (flash_speed + 1) % 3;
    // as this register is rc_w1, we clear the pending bit by seting it.
    EXTI->PR = EXTI_PR_PR0;
  }
}

// Basic implementation of a multi-speed blinky.
// Replace this with your own program.
int main(void) {
  // LED on pin C13
  gpio_set_output(GPIOC, 13, GPIO_OSPEED_FULL, false);
  gpio_write(GPIOC, 13, true);
  // Key on pin A0 triggering interrupt
  gpio_set_input(GPIOA, 0, GPIO_PUPD_UP);
  delay(2); // allow the pin to settle after enabling pullup
  gpio_set_exti(GPIOA, 0, false, true);
  NVIC_EnableIRQ(EXTI0_IRQn);

  bool led_state = true;
  while (true) {
    gpio_write(GPIOC, 13, led_state);
    led_state = !led_state;
    delay(flash_delays[flash_speed]);
  }
}