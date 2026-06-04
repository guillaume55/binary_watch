#ifndef POWER_H
#define POWER_H

/* Enable PWR clock and backup-domain access (call before SystemClock_Config). */
void power_init(void);

/* Suspend SysTick, enter Stop mode (WFI), restore HSI16 + SysTick on wakeup.
   Returns only after an EXTI (button) event.                                  */
void enter_stop(void);

/* Configure HSI16 as SYSCLK and LSE for RTC.  Also called from power_init
   and after Stop-mode wakeup to restore the clock tree.                       */
void SystemClock_Config(void);

#endif /* POWER_H */
