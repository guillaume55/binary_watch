#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

typedef enum {
    BTN_NONE = 0,
    BTN_SW1_SHORT,
    BTN_SW1_LONG,
    BTN_SW2_SHORT,
    BTN_SW2_LONG,
} btn_event_t;

void        buttons_init(void);

/* Call once per main-loop iteration.  Blocks ≤ LONG_PRESS_MIN_MS if a
   button is pending; returns immediately (BTN_NONE) if no press.       */
btn_event_t buttons_poll(void);

/* Clear any wakeup-edge flags before entering Stop mode.               */
void        buttons_clear_pending(void);

#endif /* BUTTONS_H */
