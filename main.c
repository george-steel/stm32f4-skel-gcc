#include "hal.h"

//static inline void spin(volatile uint32_t count) {
//  while (count--) {};
//}

int main(void) {
  gpio_set_output(GPIOC, 13, GPIO_OSPEED_FULL, false);
  gpio_write(GPIOC, 13, true);
  bool led_state = true;
  while (true) {
    gpio_write(GPIOC, 13, led_state);
    led_state = !led_state;
    delay(250);
  }
}