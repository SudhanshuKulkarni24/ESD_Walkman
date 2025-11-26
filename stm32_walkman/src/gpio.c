/**
 * Bare Metal GPIO Implementation - STM32F407
 * Direct register access to GPIO peripheral
 */

#include "gpio.h"
#include "stm32f407xx.h"

/* GPIO base addresses */
static GPIO_TypeDef* const gpio_bases[9] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI
};

/**
 * Initialize GPIO port clock
 */
void gpio_init_port(gpio_port_t port) {
    if (port >= 9) return;
    
    /* Enable clock for the GPIO port */
    RCC->AHB1ENR |= (1 << port);
    
    /* Read back for synchronization */
    (void)RCC->AHB1ENR;
}

/**
 * Configure GPIO pin
 * 
 * Parameters:
 * - port: GPIO port (0-8, A-I)
 * - pin: Pin number (0-15)
 * - mode: Input/Output/Alt-Func/Analog
 * - output_type: Push-pull/Open-drain
 * - speed: Speed selection
 * - pull: Pull-up/Pull-down/None
 */
void gpio_config(gpio_port_t port, gpio_pin_t pin, gpio_mode_t mode,
                 gpio_output_t output_type, gpio_speed_t speed, gpio_pull_t pull) {
    if (port >= 9 || pin >= 16) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    
    /* Initialize port clock if not already done */
    gpio_init_port(port);
    
    /* Configure mode bits (MODER register) */
    gpio->MODER &= ~(3 << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));
    
    /* Configure output type (OTYPER register) */
    if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_ALT_FUNC) {
        gpio->OTYPER &= ~(1 << pin);
        gpio->OTYPER |= (output_type << pin);
    }
    
    /* Configure speed (OSPEEDR register) */
    gpio->OSPEEDR &= ~(3 << (pin * 2));
    gpio->OSPEEDR |= (speed << (pin * 2));
    
    /* Configure pull (PUPDR register) */
    gpio->PUPDR &= ~(3 << (pin * 2));
    gpio->PUPDR |= (pull << (pin * 2));
}

/**
 * Configure GPIO alternate function
 * 
 * STM32F407 alternate functions:
 * AF0: MCO1, SWDIO, etc
 * AF1: TIM1, TIM2
 * AF2: TIM3, TIM4, TIM5
 * AF3: TIM8, TIM9, TIM10, TIM11
 * AF4: I2C1, I2C2, I2C3
 * AF5: SPI1, SPI2, SPI3, SPI4, SPI5
 * AF6: SPI3, SAI1
 * AF7: UART1-3, UART4-5
 * AF8: UART4-5, UART7-8, USART1-3, USART6
 * AF9: CAN1, CAN2, TIM12, TIM13, TIM14
 * AF10: OTG_FS, OTG_HS
 * AF11: ETH
 * AF12: FSMC, FMC, SDIO
 * AF13: DCMI
 * AF14: LTDC
 * AF15: EVENTOUT
 */
void gpio_config_alt_func(gpio_port_t port, gpio_pin_t pin, uint8_t alt_func) {
    if (port >= 9 || pin >= 16 || alt_func > 15) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    
    /* Set the alternate function */
    if (pin < 8) {
        /* AFRL (pins 0-7) */
        gpio->AFR[0] &= ~(15 << (pin * 4));
        gpio->AFR[0] |= (alt_func << (pin * 4));
    } else {
        /* AFRH (pins 8-15) */
        gpio->AFR[1] &= ~(15 << ((pin - 8) * 4));
        gpio->AFR[1] |= (alt_func << ((pin - 8) * 4));
    }
}

/**
 * Set GPIO pin (output high)
 */
void gpio_set(gpio_port_t port, gpio_pin_t pin) {
    if (port >= 9 || pin >= 16) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    gpio->BSRR = (1 << pin);  /* Set bit - atomic write */
}

/**
 * Clear GPIO pin (output low)
 */
void gpio_clear(gpio_port_t port, gpio_pin_t pin) {
    if (port >= 9 || pin >= 16) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    gpio->BSRR = (1 << (pin + 16));  /* Reset bit - atomic write */
}

/**
 * Toggle GPIO pin
 */
void gpio_toggle(gpio_port_t port, gpio_pin_t pin) {
    if (port >= 9 || pin >= 16) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    gpio->ODR ^= (1 << pin);
}

/**
 * Read GPIO pin input value
 */
uint8_t gpio_read(gpio_port_t port, gpio_pin_t pin) {
    if (port >= 9 || pin >= 16) return 0;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    return (gpio->IDR >> pin) & 1;
}

/**
 * Write GPIO pin (0 or 1)
 */
void gpio_write(gpio_port_t port, gpio_pin_t pin, uint8_t value) {
    if (port >= 9 || pin >= 16) return;
    
    if (value) {
        gpio_set(port, pin);
    } else {
        gpio_clear(port, pin);
    }
}

/**
 * Configure external interrupt for GPIO pin
 * This configures EXTI + SYSCFG for interrupt handling
 */
void gpio_config_interrupt(gpio_port_t port, gpio_pin_t pin, gpio_int_trigger_t trigger) {
    if (port >= 9 || pin >= 16) return;
    
    GPIO_TypeDef* gpio = gpio_bases[port];
    
    /* Configure pin as input first */
    gpio_config(port, pin, GPIO_MODE_INPUT, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_PULL_UP);
    
    /* Enable SYSCFG clock for EXTI configuration */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    
    /* Configure SYSCFG EXTI for the pin */
    uint32_t exti_reg = EXTI1_Pos + (pin / 4);
    uint32_t exti_shift = (pin % 4) * 4;
    
    SYSCFG->EXTICR[pin / 4] &= ~(15 << exti_shift);
    SYSCFG->EXTICR[pin / 4] |= (port << exti_shift);
    
    /* Configure EXTI trigger */
    uint32_t exti_line = 1 << pin;
    
    if (trigger == GPIO_INT_RISING) {
        EXTI->RTSR |= exti_line;  /* Rising edge */
        EXTI->FTSR &= ~exti_line;
    } else if (trigger == GPIO_INT_FALLING) {
        EXTI->FTSR |= exti_line;  /* Falling edge */
        EXTI->RTSR &= ~exti_line;
    } else if (trigger == GPIO_INT_BOTH) {
        EXTI->RTSR |= exti_line;  /* Both edges */
        EXTI->FTSR |= exti_line;
    }
    
    /* Enable EXTI interrupt */
    EXTI->IMR |= exti_line;
    
    /* Enable NVIC interrupt for this EXTI line */
    if (pin < 5) {
        NVIC_EnableIRQ(EXTI0_IRQn + pin);
    } else if (pin < 10) {
        NVIC_EnableIRQ(EXTI9_5_IRQn);
    } else {
        NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
}

/**
 * Clear external interrupt pending flag
 * Call this in interrupt handler to clear the flag
 */
void gpio_exti_clear(gpio_pin_t pin) {
    if (pin >= 16) return;
    EXTI->PR |= (1 << pin);  /* Clear pending flag */
}
