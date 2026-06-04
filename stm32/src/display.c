#include "display.h"
#include "config.h"
#include <stdint.h>

/*
 * 14-bit LED mask layout (matches charlieplex led_table order):
 *
 *  Bit  0 : LED1  — hours tens,  2^0
 *  Bit  1 : LED2  — hours tens,  2^1
 *  Bit  2 : LED3  — hours units, 2^0
 *  Bit  3 : LED4  — hours units, 2^1
 *  Bit  4 : LED5  — hours units, 2^2
 *  Bit  5 : LED6  — hours units, 2^3
 *  Bit  6 : LED7  — min tens,   2^0
 *  Bit  7 : LED8  — min tens,   2^1
 *  Bit  8 : LED9  — min tens,   2^2
 *  Bit  9 : LED10 — min units,  2^0
 *  Bit 10 : LED11 — min units,  2^1
 *  Bit 11 : LED12 — min units,  2^2
 *  Bit 12 : LED13 — min units,  2^3
 *  Bit 13 : LED14 — battery indicator
 */

uint16_t time_to_led_mask(const watch_time_t *t)
{
    const uint8_t ht = t->hours   / 10U;
    const uint8_t hu = t->hours   % 10U;
    const uint8_t mt = t->minutes / 10U;
    const uint8_t mu = t->minutes % 10U;

    uint16_t mask = 0U;
    mask |= (uint16_t)((ht & 0x03U) << 0U); /* LED1-2  */
    mask |= (uint16_t)((hu & 0x0FU) << 2U); /* LED3-6  */
    mask |= (uint16_t)((mt & 0x07U) << 6U); /* LED7-9  */
    mask |= (uint16_t)((mu & 0x0FU) << 9U); /* LED10-13 */
    return mask;
}

uint16_t battery_to_mask(uint8_t percent)
{
    /* LED10–13 live at bits 9–12; fill from lowest bit upward */
    uint16_t mask = 0U;
    if (percent >= 25U) mask |= (1U << 9U);  /* LED10 */
    if (percent >= 50U) mask |= (1U << 10U); /* LED11 */
    if (percent >= 75U) mask |= (1U << 11U); /* LED12 */
    if (percent > 75U)  mask |= (1U << 12U); /* LED13 */
    return mask;
}

uint16_t digit_mask(uint8_t digit_idx, uint8_t val)
{
    switch (digit_idx) {
    case 0: return (uint16_t)((val & 0x03U) << 0U); /* H_tens  bits 0-1 */
    case 1: return (uint16_t)((val & 0x0FU) << 2U); /* H_units bits 2-5 */
    case 2: return (uint16_t)((val & 0x07U) << 6U); /* M_tens  bits 6-8 */
    case 3: return (uint16_t)((val & 0x0FU) << 9U); /* M_units bits 9-12 */
    default: return 0U;
    }
}
