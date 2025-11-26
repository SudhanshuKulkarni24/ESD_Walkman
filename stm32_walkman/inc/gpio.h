/**
 * Bare Metal GPIO Driver - STM32F407
 * Direct register access for GPIO configuration and control
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

/* GPIO Port definition */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
    GPIO_PORT_F = 5,
    GPIO_PORT_G = 6,
    GPIO_PORT_H = 7,
    GPIO_PORT_I = 8
} gpio_port_t;

/* GPIO Pin number 0-15 */
typedef uint8_t gpio_pin_t;

/* GPIO Mode */
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_ALT_FUNC = 2,
    GPIO_MODE_ANALOG = 3
} gpio_mode_t;

/* GPIO Output type */
typedef enum {
    GPIO_OUTPUT_PP = 0,    /* Push-pull */
    GPIO_OUTPUT_OD = 1     /* Open-drain */
} gpio_output_t;

/* GPIO Speed */
typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM = 1,
    GPIO_SPEED_FAST = 2,
    GPIO_SPEED_HIGH = 3
} gpio_speed_t;

/* GPIO Pull */
typedef enum {
    GPIO_NO_PULL = 0,
    GPIO_PULL_UP = 1,
    GPIO_PULL_DOWN = 2
} gpio_pull_t;

/* GPIO Interrupt trigger */
typedef enum {
    GPIO_INT_RISING = 0,
    GPIO_INT_FALLING = 1,
    GPIO_INT_BOTH = 2,
    GPIO_INT_NONE = 3
} gpio_int_trigger_t;

/* Initialize GPIO port clock */
void gpio_init_port(gpio_port_t port);

/* Configure GPIO pin */
void gpio_config(gpio_port_t port, gpio_pin_t pin, gpio_mode_t mode, 
                 gpio_output_t output_type, gpio_speed_t speed, 
                 gpio_pull_t pull);

/* Configure GPIO alternate function */
void gpio_config_alt_func(gpio_port_t port, gpio_pin_t pin, uint8_t alt_func);

/* Set GPIO pin output high */
void gpio_set(gpio_port_t port, gpio_pin_t pin);

/* Clear GPIO pin output low */
void gpio_clear(gpio_port_t port, gpio_pin_t pin);

/* Toggle GPIO pin */
void gpio_toggle(gpio_port_t port, gpio_pin_t pin);

/* Read GPIO pin input */
uint8_t gpio_read(gpio_port_t port, gpio_pin_t pin);

/* Write GPIO pin */
void gpio_write(gpio_port_t port, gpio_pin_t pin, uint8_t value);

/* Configure external interrupt */
void gpio_config_interrupt(gpio_port_t port, gpio_pin_t pin, gpio_int_trigger_t trigger);

#endif /* __GPIO_H__ */
