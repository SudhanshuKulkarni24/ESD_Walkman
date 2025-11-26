/**
 * Bare Metal System Configuration - STM32F407
 * Direct register access, no HAL layer
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdint.h>

/* System clock frequency */
#define SYSTEM_CLOCK_HZ     168000000  /* 168 MHz - F407 maximum */
#define APB1_CLOCK_HZ       42000000   /* 42 MHz */
#define APB2_CLOCK_HZ       84000000   /* 84 MHz */

/* Tick frequency */
#define TICK_FREQ_HZ        1000       /* 1ms ticks */

/* Initialize system */
void system_init(void);

/* Get system tick in milliseconds */
uint32_t system_get_tick(void);

/* Delay in milliseconds */
void system_delay_ms(uint32_t ms);

/* Delay in microseconds */
void system_delay_us(uint32_t us);

#endif /* __SYSTEM_H__ */
