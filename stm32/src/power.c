#include "stm32l0xx_hal.h"
#include "power.h"

void power_init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    /* Allow writes to backup domain (needed for LSE and RTC config) */
    HAL_PWR_EnableBkUpAccess();
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* HSI16: system clock (16 MHz, fast startup after Stop wakeup).
       LSE: 32.768 kHz for RTC; may already be running after first boot.
       HAL_RCC_OscConfig checks LSERDY before waiting, so re-calling
       this function after Stop wakeup is safe and fast.                */
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.LSEState            = RCC_LSE_ON;
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    /* SYSCLK = HSI16 = 16 MHz, no dividers needed */
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                       | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    /* Flash latency 0 is valid at 16 MHz with Vcore range 1 */
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}

void enter_stop(void)
{
    /* Prevent SysTick from interrupting the Stop-mode entry sequence */
    HAL_SuspendTick();

    /* Stop mode with low-power regulator; EXTI5/6 (buttons) are the
       only configured wakeup sources.  RTC continues on LSE.          */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* On wakeup the hardware selects MSI as SYSCLK; restore HSI16 */
    SystemClock_Config();
    HAL_ResumeTick();
}
