#include "stm32l0xx_hal.h"
#include "battery.h"
#include "config.h"

/*
 * STM32L031 stores VREFINT calibration (measured at 3.0 V, 30 °C) at
 * 0x1FF8_0078 (16-bit, right-aligned, 12-bit ADC value).
 * VDD_mV = 3000 * (*VREFINT_CAL) / adc_raw
 */
#define VREFINT_CAL_ADDR    ((const uint16_t *)0x1FF80078U)
#define VREFINT_CAL_VREF_MV 3000U

static ADC_HandleTypeDef hadc;

static uint32_t sample_vrefint(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc.Instance                   = ADC1;
    hadc.Init.OversamplingMode      = DISABLE;
    hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc.Init.SamplingTime          = ADC_SAMPLETIME_160CYCLES_5;
    hadc.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
    hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc.Init.ContinuousConvMode    = DISABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc.Init.LowPowerAutoWait      = DISABLE;
    hadc.Init.LowPowerFrequencyMode = DISABLE;
    hadc.Init.LowPowerAutoPowerOff  = DISABLE;
    HAL_ADC_Init(&hadc);

    /* Enable internal bandgap reference */
    ADC->CCR |= ADC_CCR_VREFEN;

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_VREFINT;
    ch.Rank    = ADC_RANK_CHANNEL_NUMBER;
    HAL_ADC_ConfigChannel(&hadc, &ch);

    /* Average ADC_SAMPLES readings to reduce noise */
    uint32_t acc = 0;
    for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
        HAL_ADC_Start(&hadc);
        HAL_ADC_PollForConversion(&hadc, 10);
        acc += HAL_ADC_GetValue(&hadc);
        HAL_ADC_Stop(&hadc);
    }

    /* Power down ADC and VREFINT to minimise active current */
    ADC->CCR &= ~ADC_CCR_VREFEN;
    HAL_ADC_DeInit(&hadc);
    __HAL_RCC_ADC1_CLK_DISABLE();

    return acc / ADC_SAMPLES;
}

uint8_t battery_measure_percent(void)
{
    uint32_t raw = sample_vrefint();
    if (raw == 0U) return 0U;

    uint32_t vdd_mv = VREFINT_CAL_VREF_MV * (uint32_t)(*VREFINT_CAL_ADDR) / raw;

    /* CR3032 discharge curve approximation */
    if (vdd_mv >= 3000U) return 100U;
    if (vdd_mv >= 2800U) return 75U;
    if (vdd_mv >= 2600U) return 50U;
    if (vdd_mv >= 2400U) return 25U;
    return 0U;
}
