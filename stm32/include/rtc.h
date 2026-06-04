#ifndef RTC_H
#define RTC_H

#include <stdint.h>

typedef struct {
    uint8_t hours;    /* 0–23 */
    uint8_t minutes;  /* 0–59 */
} watch_time_t;

void rtc_init(void);
void rtc_get_time(watch_time_t *t);
void rtc_set_time(const watch_time_t *t);

#endif /* RTC_H */
