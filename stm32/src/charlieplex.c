#include "stm32l0xx_hal.h"
#include "charlieplex.h"
#include "config.h"

/*
 * Charlieplexing pin table — 16 LEDs on PA0–PA4 (5 pins, 5×4 = 20 max).
 *
 * At any instant exactly ONE pin is HIGH (anode), ONE pin is LOW (cathode),
 * and the remaining THREE pins are INPUT / Hi-Z to avoid unintended paths.
 *
 * Convention: entry {hi, lo}  where hi/lo = PA pin number 0–4.
 *
 *  idx | LED | hi→lo   | role              | BCD bit / note
 *  ----+-----+---------+-------------------+---------------------
 *   0  | L1  | PA0→PA1 | hours tens        | bit0 (LSB)
 *   1  | L2  | PA1→PA0 | hours tens        | bit1 (MSB)
 *   2  | L3  | PA1→PA2 | SPARE             | — never driven
 *   3  | L4  | PA3→PA2 | battery indicator | — set_battery_led()
 *   4  | L5  | PA2→PA4 | hours units       | bit0 (LSB)
 *   5  | L6  | PA2→PA0 | hours units       | bit1
 *   6  | L7  | PA4→PA2 | hours units       | bit2
 *   7  | L8  | PA4→PA3 | hours units       | bit3 (MSB)
 *   8  | L9  | PA2→PA3 | minutes tens      | bit0 (LSB)
 *   9  | L10 | PA0→PA2 | minutes tens      | bit1
 *  10  | L11 | PA2→PA1 | minutes tens      | bit2
 *  11  | L12 | PA3→PA4 | minutes tens      | bit3 (MSB)
 *  12  | L13 | PA3→PA0 | minutes units     | bit0 (LSB)
 *  13  | L14 | PA0→PA3 | minutes units     | bit1
 *  14  | L15 | PA1→PA3 | minutes units     | bit2
 *  15  | L16 | PA3→PA1 | minutes units     | bit3 (MSB)
 */
typedef struct { uint8_t hi; uint8_t lo; } led_pair_t;

static const led_pair_t led_table[NUM_LEDS] = {
    {0, 1},  /* L1  PA0→PA1  hours tens  bit0          */
    {1, 0},  /* L2  PA1→PA0  hours tens  bit1          */
    {1, 2},  /* L3  PA1→PA2  SPARE — entry kept, never driven */
    {3, 2},  /* L4  PA3→PA2  battery indicator          */
    {2, 4},  /* L5  PA2→PA4  hours units bit0          */
    {2, 0},  /* L6  PA2→PA0  hours units bit1          */
    {4, 2},  /* L7  PA4→PA2  hours units bit2          */
    {4, 3},  /* L8  PA4→PA3  hours units bit3          */
    {2, 3},  /* L9  PA2→PA3  minutes tens bit0         */
    {0, 2},  /* L10 PA0→PA2  minutes tens bit1         */
    {2, 1},  /* L11 PA2→PA1  minutes tens bit2         */
    {3, 4},  /* L12 PA3→PA4  minutes tens bit3         */
    {3, 0},  /* L13 PA3→PA0  minutes units bit0        */
    {0, 3},  /* L14 PA0→PA3  minutes units bit1        */
    {1, 3},  /* L15 PA1→PA3  minutes units bit2        */
    {3, 1},  /* L16 PA3→PA1  minutes units bit3        */
};

/*
 * BCD display mask and battery LED are kept separate so that
 * set_battery_led() never races against set_leds() writes.
 * active_mask = bcd_mask | (bat_on << L4_IDX).
 */
static volatile uint16_t bcd_mask   = 0;
static volatile uint8_t  bat_on     = 0;
static volatile uint16_t active_mask = 0;
static volatile uint8_t  scan_idx   = 0;

static TIM_HandleTypeDef htim2;

/* ------------------------------------------------------------------ */

/* Set PA0–PA4 all to input mode — must precede any pin drive to prevent
   cross-conduction through two LEDs sharing a common pin.              */
static inline void set_all_hiz(void)
{
    LED_GPIO_PORT->MODER &= ~0x000003FFU; /* bits [9:0] → 00 = input */
}

/* Drive one LED: pre-load BSRR, then switch hi/lo pins to output.
   Pre-loading BSRR while still in input mode has no effect on the bus,
   but ensures the output latch holds the right level the instant MODER
   switches, avoiding even a partial-cycle glitch.                      */
static inline void drive_led(uint8_t idx)
{
    const uint8_t hi = led_table[idx].hi;
    const uint8_t lo = led_table[idx].lo;

    /* Stage output values (no bus effect yet — pins still Hi-Z) */
    LED_GPIO_PORT->BSRR = (1U << hi) | (1U << (lo + 16U));

    /* Switch hi and lo to push-pull output (mode bits = 01) */
    LED_GPIO_PORT->MODER |= (1U << (hi * 2U)) | (1U << (lo * 2U));
}

/* ------------------------------------------------------------------ */

void charlie_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio);

    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = CHARLIE_TIM_PSC;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = CHARLIE_TIM_ARR;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void charlie_start(void)
{
    scan_idx = 0;
    HAL_TIM_Base_Start_IT(&htim2);
}

void charlie_stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim2);
    set_all_hiz();
}

/* Rebuild active_mask from both sources; called after any partial update */
static inline void rebuild_mask(void)
{
    active_mask = bcd_mask | (bat_on ? (1U << L4_IDX) : 0U);
}

void set_leds(uint16_t mask)
{
    /* Strip L3 (spare) and L4 (battery) — they are never part of BCD display */
    bcd_mask = mask & ~((1U << L3_IDX) | (1U << L4_IDX));
    rebuild_mask();
}

void set_battery_led(uint8_t on)
{
    bat_on = on ? 1U : 0U;
    rebuild_mask();
}

void charlie_test_all_leds(uint32_t ms_per_led)
{
    /* Light L1–L16 sequentially for bring-up / visual validation */
    for (uint8_t i = 0U; i < NUM_LEDS; i++) {
        active_mask = (1U << i);
        HAL_Delay(ms_per_led);
    }
    active_mask = 0U;
    bcd_mask    = 0U;
    bat_on      = 0U;
}

/* ------------------------------------------------------------------ */

void charlie_tim_irq(void)
{
    if (!__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE)) return;
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    /* Release all pins before reconfiguring to avoid shorts */
    set_all_hiz();

    if (active_mask == 0U) {
        return;
    }

    /* Advance to the next active LED, skipping inactive slots.
       Wrap-around is bounded: worst case visits all 16 entries once.   */
    uint8_t start = scan_idx;
    do {
        scan_idx = (scan_idx + 1U) % NUM_LEDS;
    } while (!(active_mask & (1U << scan_idx)) && scan_idx != start);

    if (active_mask & (1U << scan_idx)) {
        drive_led(scan_idx);
    }
}

void TIM2_IRQHandler(void)
{
    charlie_tim_irq();
}
