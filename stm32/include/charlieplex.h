#ifndef CHARLIEPLEX_H
#define CHARLIEPLEX_H

#include <stdint.h>

void charlie_init(void);   /* configure GPIO + TIM2, does NOT start scan */
void charlie_start(void);  /* enable TIM2 interrupt scan                 */
void charlie_stop(void);   /* disable TIM2, set all pins Hi-Z            */

/* Set BCD display mask (bits 2 and 3 are silently stripped — L3 spare
   and L4 battery are managed independently).  Atomic on Cortex-M0+.    */
void set_leds(uint16_t bcd_mask);

/* Drive or release L4 (battery indicator) independently of the BCD
   display.  Can be called at any rate without disturbing time display.  */
void set_battery_led(uint8_t on);

/* Light each LED L1→L16 in turn, ms_per_led ms each, then all off.
   Blocks the caller; intended for board bring-up / visual validation.   */
void charlie_test_all_leds(uint32_t ms_per_led);

/* Forward-declared for IRQ file — call from TIM2_IRQHandler             */
void charlie_tim_irq(void);

#endif /* CHARLIEPLEX_H */
