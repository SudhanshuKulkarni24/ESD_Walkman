/**
 * Bare Metal System Implementation - STM32F407
 * Direct register access for clock configuration and system tick
 */

#include "system.h"
#include <string.h>

/* CMSIS Device Header - STM32F407 */
#include "stm32f407xx.h"

/* SysTick counter for delays */
static volatile uint32_t system_tick = 0;

/**
 * SysTick interrupt handler (1ms ticks)
 * Called every millisecond by the timer
 */
void SysTick_Handler(void) {
    system_tick++;
}

/**
 * Get system tick in milliseconds
 */
uint32_t system_get_tick(void) {
    return system_tick;
}

/**
 * Delay in milliseconds (busy wait with systick)
 */
void system_delay_ms(uint32_t ms) {
    uint32_t start = system_tick;
    while ((system_tick - start) < ms);
}

/**
 * Delay in microseconds (approximate, busy wait)
 */
void system_delay_us(uint32_t us) {
    uint32_t ticks = (us * (SYSTEM_CLOCK_HZ / 1000000)) / 3;  /* Approximate loop count */
    while (ticks--);
}

/**
 * Initialize system clock to 168 MHz
 * Uses HSI (16MHz internal oscillator) with PLL
 * 
 * Configuration:
 * HSI = 16MHz
 * VCO_in = HSI / PLLM = 16 / 16 = 1 MHz
 * VCO_out = VCO_in * PLLN = 1 * 336 = 336 MHz
 * PLLCLK = VCO_out / PLLP = 336 / 2 = 168 MHz (F407 maximum)
 * USB/SDIO = VCO_out / PLLQ = 336 / 7 = 48 MHz
 * 
 * APB1 prescaler = 4 → APB1 = 168 / 4 = 42 MHz (max 42MHz on APB1)
 * APB2 prescaler = 2 → APB2 = 168 / 2 = 84 MHz (max 84MHz on APB2)
 * 
 * Flash wait states = 5 (168MHz requires 5 wait states)
 * Voltage regulator scale = Scale 1 (highest performance for 168MHz)
 */
void system_init(void) {
    uint32_t timeout = 0;
    
    /* 1. Enable power control clock */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    
    /* 2. Set voltage regulator to Scale 1 (max performance at 168MHz) */
    PWR->CR |= PWR_CR_VOS;  /* VOS = Scale 1 */
    
    /* 3. Set flash memory wait states to 5 (required for 168MHz) */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
    
    /* 4. Enable HSI oscillator */
    RCC->CR |= RCC_CR_HSION;
    timeout = 0;
    while (!(RCC->CR & RCC_CR_HSIRDY) && timeout < 1000000) timeout++;
    
    /* 5. Configure PLL */
    /* Note: PLL must be disabled before configuration */
    RCC->CR &= ~RCC_CR_PLLON;
    timeout = 0;
    while ((RCC->CR & RCC_CR_PLLRDY) && timeout < 1000000) timeout++;
    
    /* Configure PLL parameters */
    RCC->PLLCFGR = 0;  /* Reset to default */
    RCC->PLLCFGR |= (RCC_PLLCFGR_PLLSRC_HSI);  /* HSI as source */
    RCC->PLLCFGR |= (16 << RCC_PLLCFGR_PLLM_Pos);  /* PLLM = 16 */
    RCC->PLLCFGR |= (336 << RCC_PLLCFGR_PLLN_Pos);  /* PLLN = 336 */
    RCC->PLLCFGR |= (0 << RCC_PLLCFGR_PLLP_Pos);  /* PLLP = DIV2 */
    RCC->PLLCFGR |= (7 << RCC_PLLCFGR_PLLQ_Pos);  /* PLLQ = 7 */
    
    /* 6. Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    timeout = 0;
    while (!(RCC->CR & RCC_CR_PLLRDY) && timeout < 1000000) timeout++;
    
    /* 7. Configure AHB and APB prescalers before switching system clock */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= (0 << RCC_CFGR_HPRE_Pos);   /* AHB prescaler = 1 (no division) */
    RCC->CFGR |= (4 << RCC_CFGR_PPRE1_Pos);  /* APB1 prescaler = 4 (168/4 = 42MHz) */
    RCC->CFGR |= (2 << RCC_CFGR_PPRE2_Pos);  /* APB2 prescaler = 2 (168/2 = 84MHz) */
    
    /* 8. Switch system clock to PLL output */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    timeout = 0;
    while (((RCC->CFGR & RCC_CFGR_SWS) >> 2) != 2 && timeout < 1000000) timeout++;
    
    /* 9. Configure SysTick for 1ms interrupts */
    /* SysTick frequency = SYSTEM_CLOCK / prescaler
     * For 1ms tick: 168MHz / 168000 = 1kHz
     */
    uint32_t systick_reload = (SYSTEM_CLOCK_HZ / TICK_FREQ_HZ) - 1;
    SysTick->LOAD = systick_reload;
    SysTick->VAL = 0;
    SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk |  /* Use processor clock */
                     SysTick_CTRL_TICKINT_Msk |     /* Enable interrupt */
                     SysTick_CTRL_ENABLE_Msk);      /* Enable counter */
    
    /* 10. Set SysTick interrupt priority (lowest) */
    NVIC_SetPriority(SysTick_IRQn, 15);
    
    /* 11. Disable the systick during init - enable only when needed */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    
    system_tick = 0;
}

/**
 * Error handler for system faults
 */
void HardFault_Handler(void) {
    while (1);
}

/**
 * Memory fault handler
 */
void MemManage_Handler(void) {
    while (1);
}

/**
 * Bus fault handler
 */
void BusFault_Handler(void) {
    while (1);
}

/**
 * Usage fault handler
 */
void UsageFault_Handler(void) {
    while (1);
}
