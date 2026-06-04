#include "stm32l0xx_hal.h"
#include "config.h"
#include "charlieplex.h"
#include "rtc.h"
#include "buttons.h"
#include "battery.h"
#include "display.h"
#include "power.h"

/* ── State machine ──────────────────────────────────────────────────── */

typedef enum {
    STATE_STARTUP_ANIM,
    STATE_SHOW_TIME,
    STATE_SET_TIME,
    STATE_SHOW_BATTERY,
    STATE_SLEEP,
} app_state_t;

static app_state_t state = STATE_STARTUP_ANIM;

/* ── Time-setting context ───────────────────────────────────────────── */

static struct {
    uint8_t      digit;         /* 0=H_tens 1=H_units 2=M_tens 3=M_units */
    uint8_t      vals[4];       /* working copy of each BCD digit         */
    watch_time_t saved;         /* snapshot to restore on cancel          */
    uint32_t     last_action;   /* HAL_GetTick() of last SW event         */
} set_ctx;

/* ── Helpers ────────────────────────────────────────────────────────── */

static void refresh_time_display(void)
{
    watch_time_t t;
    rtc_get_time(&t);
    set_leds(time_to_led_mask(&t));
}

/* ── Startup animation ──────────────────────────────────────────────── */

static void run_startup_anim(void)
{
    uint16_t mask = 0U;

    /* Sweep ON: LED1 → LED14, each step adds one LED */
    for (int i = 0; i < (int)NUM_LEDS; i++) {
        mask |= (uint16_t)(1U << i);
        set_leds(mask);
        HAL_Delay(STARTUP_LED_ON_MS);
    }

    /* Sweep OFF: LED14 → LED1, each step removes one LED */
    for (int i = (int)NUM_LEDS - 1; i >= 0; i--) {
        mask &= (uint16_t)~(1U << i);
        set_leds(mask);
        HAL_Delay(STARTUP_LED_OFF_MS);
    }
}

/* ── Time-setting helpers ───────────────────────────────────────────── */

static void set_mode_enter(void)
{
    rtc_get_time(&set_ctx.saved);
    set_ctx.digit       = 0;
    set_ctx.vals[0]     = set_ctx.saved.hours   / 10U;
    set_ctx.vals[1]     = set_ctx.saved.hours   % 10U;
    set_ctx.vals[2]     = set_ctx.saved.minutes / 10U;
    set_ctx.vals[3]     = set_ctx.saved.minutes % 10U;
    set_ctx.last_action = HAL_GetTick();
}

static void set_mode_update_leds(uint8_t blink_on)
{
    uint16_t mask = 0U;
    for (uint8_t d = 0U; d < 4U; d++) {
        /* Suppress the blinking digit when blink phase is off */
        if (d == set_ctx.digit && !blink_on) continue;
        mask |= digit_mask(d, set_ctx.vals[d]);
    }
    set_leds(mask);
}

/* Increment the active digit with BCD validity and wrap */
static void set_mode_increment(void)
{
    uint8_t d = set_ctx.digit;
    set_ctx.vals[d]++;

    uint8_t limit;
    if (d == 0U) {
        limit = 2U;  /* H_tens: 0–2 */
    } else if (d == 1U) {
        limit = (set_ctx.vals[0] == 2U) ? 3U : 9U; /* H_units: 0–3 when H_tens=2 */
    } else if (d == 2U) {
        limit = 5U;  /* M_tens: 0–5 */
    } else {
        limit = 9U;  /* M_units: 0–9 */
    }

    if (set_ctx.vals[d] > limit) set_ctx.vals[d] = 0U;
    set_ctx.last_action = HAL_GetTick();
}

/* Validate current digit and advance; save to RTC on last digit */
static void set_mode_validate(void)
{
    set_ctx.digit++;
    set_ctx.last_action = HAL_GetTick();

    if (set_ctx.digit >= 4U) {
        watch_time_t t;
        t.hours   = set_ctx.vals[0] * 10U + set_ctx.vals[1];
        t.minutes = set_ctx.vals[2] * 10U + set_ctx.vals[3];
        rtc_set_time(&t);
    }
}

/* ── Main ───────────────────────────────────────────────────────────── */

int main(void)
{
    HAL_Init();
    power_init();          /* enable PWR, backup domain access       */
    SystemClock_Config();  /* HSI16 as SYSCLK, start LSE             */

    charlie_init();        /* configure PA0-PA4 and TIM2             */
    buttons_init();        /* configure PA5-PA6 and EXTI4_15         */
    rtc_init();            /* init RTC on LSE, cold-start = 00:00    */

    charlie_start();
    run_startup_anim();

    refresh_time_display();
    state = STATE_SHOW_TIME;
    uint32_t display_deadline = HAL_GetTick() + TIME_ON_MS;

    /* Time-set blink state */
    uint8_t  blink_on      = 1U;
    uint32_t blink_tick    = HAL_GetTick();

    /* Battery display */
    uint8_t  batt_pct      = 0U;
    uint32_t batt_deadline = 0U;

    while (1) {
        btn_event_t ev = buttons_poll();

        switch (state) {

        /* ── Sleep ──────────────────────────────────────────────── */
        case STATE_SLEEP:
            set_leds(0U);
            charlie_stop();
            buttons_clear_pending();
            enter_stop();          /* blocks until EXTI wakeup      */
            charlie_start();
            refresh_time_display();
            state            = STATE_SHOW_TIME;
            display_deadline = HAL_GetTick() + TIME_ON_MS;
            break;

        /* ── Normal time display ─────────────────────────────────── */
        case STATE_SHOW_TIME:
            if (ev == BTN_SW1_LONG) {
                set_mode_enter();
                blink_on   = 1U;
                blink_tick = HAL_GetTick();
                state      = STATE_SET_TIME;
                set_mode_update_leds(blink_on);

            } else if (ev == BTN_SW2_LONG) {
                batt_pct      = battery_measure_percent();
                batt_deadline = HAL_GetTick() + BATTERY_DISP_MS;
                set_leds(battery_to_mask(batt_pct));
                state = STATE_SHOW_BATTERY;

            } else if (ev == BTN_SW1_SHORT || ev == BTN_SW2_SHORT) {
                display_deadline = HAL_GetTick() + TIME_ON_MS;
                refresh_time_display();

            } else if (HAL_GetTick() >= display_deadline) {
                state = STATE_SLEEP;
            }
            break;

        /* ── Time setting ────────────────────────────────────────── */
        case STATE_SET_TIME:
            /* 4 Hz blink: toggle every BLINK_PERIOD_MS */
            if (HAL_GetTick() - blink_tick >= BLINK_PERIOD_MS) {
                blink_on   = !blink_on;
                blink_tick = HAL_GetTick();
                set_mode_update_leds(blink_on);
            }

            if (ev == BTN_SW1_SHORT) {
                set_mode_increment();
                blink_on   = 1U;
                blink_tick = HAL_GetTick();
                set_mode_update_leds(blink_on);

            } else if (ev == BTN_SW2_SHORT) {
                set_mode_validate();
                if (set_ctx.digit >= 4U) {
                    /* All digits confirmed; return to time display */
                    refresh_time_display();
                    state            = STATE_SHOW_TIME;
                    display_deadline = HAL_GetTick() + TIME_ON_MS;
                } else {
                    blink_on   = 1U;
                    blink_tick = HAL_GetTick();
                    set_mode_update_leds(blink_on);
                }
            }

            /* No-input timeout → cancel and restore previous time */
            if (HAL_GetTick() - set_ctx.last_action >= SET_TIMEOUT_MS) {
                rtc_set_time(&set_ctx.saved);
                refresh_time_display();
                state            = STATE_SHOW_TIME;
                display_deadline = HAL_GetTick() + TIME_ON_MS;
            }
            break;

        /* ── Battery display ─────────────────────────────────────── */
        case STATE_SHOW_BATTERY:
            /* Blink the single LED when battery is critically low */
            if (batt_pct < 25U) {
                if ((HAL_GetTick() / BLINK_PERIOD_MS) & 1U) {
                    set_leds(0U);
                } else {
                    set_leds(battery_to_mask(batt_pct));
                }
            }

            if (HAL_GetTick() >= batt_deadline) {
                refresh_time_display();
                state            = STATE_SHOW_TIME;
                display_deadline = HAL_GetTick() + TIME_ON_MS;
            }
            break;

        default:
            state = STATE_SHOW_TIME;
            break;
        }

        /* Light sleep between 1 ms SysTick ticks; TIM2 IRQ keeps
           charlieplexing running while CPU is idle.                 */
        __WFI();
    }
}
