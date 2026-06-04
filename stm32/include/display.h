#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "rtc.h"

/* Convert 24h time to 14-bit charlieplex LED mask:
     bits  0-1  : hours tens  (LED1-LED2)
     bits  2-5  : hours units (LED3-LED6)
     bits  6-8  : min   tens  (LED7-LED9)
     bits  9-12 : min   units (LED10-LED13)
     bit  13    : battery     (LED14)                                    */
uint16_t time_to_led_mask(const watch_time_t *t);

/* Battery percentage (0/25/50/75/100) → LED10-LED13 bar mask.
   Blinking when <25% is handled in the caller by toggling set_leds().  */
uint16_t battery_to_mask(uint8_t percent);

/* Mask contribution for a single time-setting digit (for blink logic).
   digit_idx: 0=H_tens, 1=H_units, 2=M_tens, 3=M_units               */
uint16_t digit_mask(uint8_t digit_idx, uint8_t val);

#endif /* DISPLAY_H */
