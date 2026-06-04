#ifndef CHARLIEPLEX_H
#define CHARLIEPLEX_H

#include <stdint.h>

void charlie_init(void);   /* configure GPIO + TIM2, does NOT start scan */
void charlie_start(void);  /* enable TIM2 interrupt scan                 */
void charlie_stop(void);   /* disable TIM2, set all pins Hi-Z            */

/* Set active LED mask; bit N = LED(N+1), 0 = all off.
   Thread-safe: single 16-bit volatile write (atomic on Cortex-M0+).    */
void set_leds(uint16_t mask);

/* Forward-declared for IRQ file — call from TIM2_IRQHandler             */
void charlie_tim_irq(void);

#endif /* CHARLIEPLEX_H */
