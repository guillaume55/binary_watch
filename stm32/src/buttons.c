#include "stm32l0xx_hal.h"
#include "buttons.h"
#include "config.h"

static volatile uint8_t sw1_pending = 0;
static volatile uint8_t sw2_pending = 0;

void buttons_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = SW1_PIN | SW2_PIN;
    gpio.Mode = GPIO_MODE_IT_FALLING; /* active-LOW buttons, falling edge */
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SW_GPIO_PORT, &gpio);

    /* EXTI4_15 covers both PA5 (line 5) and PA6 (line 6) */
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void buttons_clear_pending(void)
{
    sw1_pending = 0;
    sw2_pending = 0;
}

/*
 * Measure press duration after debounce.
 * Blocks while button is held, up to LONG_PRESS_MIN_MS after debounce.
 * Returns the classified event or BTN_NONE for medium-press (ignored).
 */
btn_event_t buttons_poll(void)
{
    if (!sw1_pending && !sw2_pending) return BTN_NONE;

    uint8_t  is_sw1 = sw1_pending;
    uint16_t pin    = is_sw1 ? SW1_PIN : SW2_PIN;
    sw1_pending = 0;
    sw2_pending = 0;

    /* Skip spurious glitches: wait for debounce window */
    HAL_Delay(DEBOUNCE_MS);

    /* If pin is already high, the press was extremely brief → short */
    uint32_t t0 = HAL_GetTick();
    while (HAL_GPIO_ReadPin(SW_GPIO_PORT, pin) == GPIO_PIN_RESET) {
        if (HAL_GetTick() - t0 >= LONG_PRESS_MIN_MS) {
            while (HAL_GPIO_ReadPin(SW_GPIO_PORT, pin) == GPIO_PIN_RESET);
            return is_sw1 ? BTN_SW1_LONG : BTN_SW2_LONG;
        }
    }

    uint32_t held_ms = HAL_GetTick() - t0;
    if (held_ms < SHORT_PRESS_MAX_MS) {
        return is_sw1 ? BTN_SW1_SHORT : BTN_SW2_SHORT;
    }
    return BTN_NONE; /* 800–2000 ms: medium press, ignored per spec */
}

/* ------------------------------------------------------------------ */

void EXTI4_15_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(SW1_PIN)) {
        __HAL_GPIO_EXTI_CLEAR_IT(SW1_PIN);
        sw1_pending = 1;
    }
    if (__HAL_GPIO_EXTI_GET_IT(SW2_PIN)) {
        __HAL_GPIO_EXTI_CLEAR_IT(SW2_PIN);
        sw2_pending = 1;
    }
}
