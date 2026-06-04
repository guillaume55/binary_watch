#include "stm32l0xx_hal.h"
#include "rtc.h"

static RTC_HandleTypeDef hrtc;

/* HAL callback — configures LSE as RTC clock source.
   Called automatically by HAL_RTC_Init when state is RESET.            */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc_arg)
{
    (void)hrtc_arg;
    /* Backup domain access was enabled in power_init(); LSE is already
       running (started in SystemClock_Config).                          */
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    __HAL_RCC_RTC_ENABLE();
}

void rtc_init(void)
{
    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    /* LSE = 32 768 Hz; AsynchPrediv=127 → ck_spre input = 256 Hz
                         SynchPrediv=255 → calendar tick  = 1 Hz       */
    hrtc.Init.AsynchPrediv   = 127;
    hrtc.Init.SynchPrediv    = 255;
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    HAL_RTC_Init(&hrtc);

    /* Date must be set for shadow registers to unlock on STM32L0 RTC  */
    RTC_DateTypeDef date = {0};
    date.WeekDay = RTC_WEEKDAY_MONDAY;
    date.Month   = RTC_MONTH_JANUARY;
    date.Date    = 1;
    date.Year    = 0;
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

    /* Cold-start time: 00:00:00 */
    RTC_TimeTypeDef time = {0};
    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
}

void rtc_get_time(watch_time_t *t)
{
    RTC_TimeTypeDef rt = {0};
    RTC_DateTypeDef rd = {0};
    HAL_RTC_GetTime(&hrtc, &rt, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &rd, RTC_FORMAT_BIN); /* unlocks shadow reg */
    t->hours   = rt.Hours;
    t->minutes = rt.Minutes;
}

void rtc_set_time(const watch_time_t *t)
{
    RTC_TimeTypeDef rt = {0};
    rt.Hours   = t->hours;
    rt.Minutes = t->minutes;
    rt.Seconds = 0;
    HAL_RTC_SetTime(&hrtc, &rt, RTC_FORMAT_BIN);
}
