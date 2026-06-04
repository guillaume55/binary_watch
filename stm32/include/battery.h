#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

/* Measure VDD via internal VREFINT bandgap; average ADC_SAMPLES reads.
   Returns one of: 100, 75, 50, 25, 0 (CR3032 voltage steps).
   ADC is powered on then off inside this call to minimise active time. */
uint8_t battery_measure_percent(void);

#endif /* BATTERY_H */
