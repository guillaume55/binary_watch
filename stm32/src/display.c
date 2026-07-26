#include "display.h"
#include "config.h"
#include <stdint.h>

/*
 * 16-bit LED mask layout (bit N = L(N+1), matches led_table order):
 *
 *  bit  0 : L1  — hours tens,   2^0 (LSB)
 *  bit  1 : L2  — hours tens,   2^1 (MSB)
 *  bit  2 : L3  — SPARE        [stripped by set_leds()]
 *  bit  3 : L4  — battery      [managed by set_battery_led()]
 *  bit  4 : L5  — hours units,  2^0 (LSB)
 *  bit  5 : L6  — hours units,  2^1
 *  bit  6 : L7  — hours units,  2^2
 *  bit  7 : L8  — hours units,  2^3 (MSB)
 *  bit  8 : L9  — min tens,     2^0 (LSB)
 *  bit  9 : L10 — min tens,     2^1
 *  bit 10 : L11 — min tens,     2^2
 *  bit 11 : L12 — min tens,     2^3 (MSB)  [never set: max mt=5]
 *  bit 12 : L13 — min units,    2^0 (LSB)
 *  bit 13 : L14 — min units,    2^1
 *  bit 14 : L15 — min units,    2^2
 *  bit 15 : L16 — min units,    2^3 (MSB)
 *
 * Hours tens special case: only L1/L2 exist (values 0/1/2).
 *   ht=0 → neither lit,  ht=1 → L1 (bit 0),  ht=2 → L2 (bit 1).
 *   Standard binary encoding covers this: (ht & 0x3) << 0.
 */

uint16_t time_to_led_mask(const watch_time_t *t)
{
    const uint8_t ht = t->hours   / 10U;
    const uint8_t hu = t->hours   % 10U;
    const uint8_t mt = t->minutes / 10U;
    const uint8_t mu = t->minutes % 10U;

    uint16_t mask = 0U;
    mask |= (uint16_t)((ht & 0x03U) << 0U);  /* L1–L2   bits 0-1  */
    /* bits 2,3 intentionally skipped: L3 spare, L4 battery         */
    mask |= (uint16_t)((hu & 0x0FU) << 4U);  /* L5–L8   bits 4-7  */
    mask |= (uint16_t)((mt & 0x0FU) << 8U);  /* L9–L12  bits 8-11 */
    mask |= (uint16_t)((mu & 0x0FU) << 12U); /* L13–L16 bits 12-15*/
    return mask;
}

/* Battery is a single dedicated LED (L4); drive via set_battery_led().
   This stub is kept for call-site compatibility but returns 0.          */
uint16_t battery_to_mask(uint8_t percent)
{
    (void)percent;
    return 0U; /* caller must use set_battery_led() for L4 */
}

uint16_t digit_mask(uint8_t digit_idx, uint8_t val)
{
    switch (digit_idx) {
    case 0: return (uint16_t)((val & 0x03U) << 0U);  /* H_tens  L1-L2   bits 0-1  */
    case 1: return (uint16_t)((val & 0x0FU) << 4U);  /* H_units L5-L8   bits 4-7  */
    case 2: return (uint16_t)((val & 0x0FU) << 8U);  /* M_tens  L9-L12  bits 8-11 */
    case 3: return (uint16_t)((val & 0x0FU) << 12U); /* M_units L13-L16 bits 12-15*/
    default: return 0U;
    }
}
