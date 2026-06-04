#include "stm32l0xx_hal.h"
#include "charlieplex.h"
#include "config.h"

/*
 * Charlieplexing pin table — 14 LEDs on PA0–PA4 (5 pins, 5×4 = 20 max).
 *
 * At any instant exactly ONE pin is HIGH (anode), ONE pin is LOW (cathode),
 * and the remaining THREE pins are INPUT / Hi-Z to avoid unintended paths.
 *
 * Convention: entry {hi, lo}  where hi/lo are pin numbers 0–4 (= PA0–PA4).
 *
 *  idx | LED  | hi→lo  | time field        | BCD bit
 *  ----+------+--------+-------------------+--------
 *   0  | LED1 | PA0→PA1| hours tens        | bit 0
 *   1  | LED2 | PA1→PA0| hours tens        | bit 1
 *   2  | LED3 | PA0→PA2| hours units       | bit 0
 *   3  | LED4 | PA2→PA0| hours units       | bit 1
 *   4  | LED5 | PA0→PA3| hours units       | bit 2
 *   5  | LED6 | PA3→PA0| hours units       | bit 3
 *   6  | LED7 | PA0→PA4| minutes tens      | bit 0
 *   7  | LED8 | PA4→PA0| minutes tens      | bit 1
 *   8  | LED9 | PA1→PA2| minutes tens      | bit 2
 *   9  | LED10| PA2→PA1| minutes units     | bit 0
 *  10  | LED11| PA1→PA3| minutes units     | bit 1
 *  11  | LED12| PA3→PA1| minutes units     | bit 2
 *  12  | LED13| PA1→PA4| minutes units     | bit 3
 *  13  | LED14| PA4→PA1| battery indicator | —
 */
typedef struct { uint8_t hi; uint8_t lo; } led_pair_t;

static const led_pair_t led_table[NUM_LEDS] = {
    {0, 1}, {1, 0},   /* LED1–2   hours tens          */
    {0, 2}, {2, 0},   /* LED3–4   hours units b0–b1   */
    {0, 3}, {3, 0},   /* LED5–6   hours units b2–b3   */
    {0, 4}, {4, 0},   /* LED7–8   minutes tens b0–b1  */
    {1, 2},            /* LED9     minutes tens b2     */
    {2, 1}, {1, 3},   /* LED10–11 minutes units b0–b1 */
    {3, 1}, {1, 4},   /* LED12–13 minutes units b2–b3 */
    {4, 1},            /* LED14    battery             */
};

static volatile uint16_t active_mask = 0;
static volatile uint8_t  scan_idx    = 0;

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

void set_leds(uint16_t mask)
{
    active_mask = mask & ((1U << NUM_LEDS) - 1U);
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
       Wrap-around is bounded: worst case visits all 14 entries once.   */
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
