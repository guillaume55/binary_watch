#ifndef CONFIG_H
#define CONFIG_H

/* ── Timing ─────────────────────────────────────────────────────────── */
#define TIME_ON_MS          10000U  /* display duration after short press    */
#define STARTUP_LED_ON_MS   500U    /* startup anim: ms per LED turn-on      */
#define STARTUP_LED_OFF_MS  200U    /* startup anim: ms per LED turn-off     */
#define BLINK_PERIOD_MS     125U    /* 4 Hz half-period (125 ms on/off)      */
#define SHORT_PRESS_MAX_MS  800U    /* < this → short press                  */
#define LONG_PRESS_MIN_MS   2000U   /* > this → long press                   */
#define DEBOUNCE_MS         100U    /* button debounce window                */
#define BATTERY_DISP_MS     5000U   /* battery display duration              */
#define SET_TIMEOUT_MS      10000U  /* time-set: no-input cancel timeout     */
#define ADC_SAMPLES         8U      /* ADC samples averaged for VDD measure  */

/* ── GPIO ───────────────────────────────────────────────────────────── */
#define LED_GPIO_PORT   GPIOA
#define SW_GPIO_PORT    GPIOA
#define SW1_PIN         GPIO_PIN_5   /* active LOW, EXTI5 */
#define SW2_PIN         GPIO_PIN_6   /* active LOW, EXTI6 */

/* ── Charlieplexing ─────────────────────────────────────────────────── */
#define NUM_LEDS        14U          /* LED1–LED14 on PA0–PA4 */

/* ── Charlieplex scan timer ─────────────────────────────────────────── */
/* TIM2: 16 MHz / (PSC+1) / (ARR+1) = 1 kHz  →  PSC=15, ARR=999       */
#define CHARLIE_TIM_PSC 15U
#define CHARLIE_TIM_ARR 999U

#endif /* CONFIG_H */
