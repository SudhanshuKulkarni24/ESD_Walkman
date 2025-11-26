/**
 * STM32 Button Handler - GPIO-based button input
 * BARE METAL - No HAL, direct register access via bare metal GPIO driver
 * Buttons: Previous, Play/Pause, Next, Volume Up, Volume Down
 */

#include "buttons.h"
#include "gpio.h"
#include "system.h"
#include <string.h>

/* Button configuration */
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    button_t id;
    uint8_t state;
    uint32_t press_time;
    uint8_t debounced;
} button_config_t;

/* Button definitions - STM32F407 Discovery board */
static button_config_t buttons[NUM_BUTTONS] = {
    {GPIOD, GPIO_PIN_13, BTN_PREVIOUS, 0, 0, 0},      // Previous track (PD13)
    {GPIOD, GPIO_PIN_14, BTN_PLAY_PAUSE, 0, 0, 0},    // Play/Pause (PD14)
    {GPIOD, GPIO_PIN_15, BTN_NEXT, 0, 0, 0},          // Next track (PD15)
    {GPIOA, GPIO_PIN_0, BTN_VOL_UP, 0, 0, 0},         // Volume Up (PA0 - User button)
    {GPIOD, GPIO_PIN_0, BTN_VOL_DOWN, 0, 0, 0},       // Volume Down (PD0)
    {GPIOD, GPIO_PIN_1, BTN_SHUFFLE, 0, 0, 0},        // Shuffle (PD1)
    {GPIOD, GPIO_PIN_2, BTN_LOOP, 0, 0, 0}            // Loop (PD2)
};

/* Button callbacks */
static button_callback_t button_callbacks[NUM_BUTTONS] = {NULL};

/* Interrupt flag for debouncing */
static volatile uint32_t button_interrupt_flags = 0;

/* Debounce timing */
#define DEBOUNCE_TIME_MS 20
#define LONG_PRESS_TIME_MS 1000

/**
 * Initialize button inputs
 * Uses bare metal GPIO driver with EXTI interrupts
 */
int buttons_init(void) {
    /* Initialize GPIO ports for buttons */
    gpio_init_port(GPIO_PORT_A);  /* PA0 - User button */
    gpio_init_port(GPIO_PORT_D);  /* PD0-2, PD13-15 - Custom buttons */
    
    /* Configure PA0 (Volume Up - User button) with interrupt */
    gpio_config_interrupt(GPIO_PORT_A, 0, GPIO_INT_FALLING);
    
    /* Configure PD0 (Volume Down) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 0, GPIO_INT_FALLING);
    
    /* Configure PD1 (Shuffle) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 1, GPIO_INT_FALLING);
    
    /* Configure PD2 (Loop) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 2, GPIO_INT_FALLING);
    
    /* Configure PD13 (Previous) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 13, GPIO_INT_FALLING);
    
    /* Configure PD14 (Play/Pause) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 14, GPIO_INT_FALLING);
    
    /* Configure PD15 (Next) with interrupt */
    gpio_config_interrupt(GPIO_PORT_D, 15, GPIO_INT_FALLING);
    
    /* Set interrupt priorities (lower number = higher priority) */
    NVIC_SetPriority(EXTI0_IRQn, 5);
    NVIC_SetPriority(EXTI1_IRQn, 5);
    NVIC_SetPriority(EXTI2_IRQn, 5);
    NVIC_SetPriority(EXTI15_10_IRQn, 5);
    
    return BUTTONS_OK;
}

/**
 * Register callback for button event
 */
int buttons_register_callback(button_t button, button_callback_t callback) {
    if (button >= NUM_BUTTONS) {
        return BUTTONS_ERROR;
    }
    
    button_callbacks[button] = callback;
    return BUTTONS_OK;
}

/**
 * Poll buttons for changes (call from main loop)
 * This handles debouncing and long-press detection after interrupt
 * Uses bare metal GPIO reading
 */
void buttons_poll(void) {
    uint32_t current_time = system_get_tick();
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_config_t* btn = &buttons[i];
        
        /* Read current GPIO state (0 = released, 1 = pressed with pull-up) */
        uint8_t pin_state = gpio_read(btn->port, btn->pin);
        uint8_t current_state = pin_state ? 0 : 1;  /* Invert for active-low logic */
        
        /* Debounce logic with hysteresis */
        if (current_state != btn->state) {
            /* State changed, start debounce timer */
            if (!btn->debounced) {
                btn->press_time = current_time;
                btn->debounced = 0;
            } else if ((current_time - btn->press_time) >= DEBOUNCE_TIME_MS) {
                /* Debounce time elapsed, confirm state change */
                btn->debounced = 1;
                btn->state = current_state;
                
                /* Generate callback event */
                if (button_callbacks[i] != NULL) {
                    if (current_state == 1) {
                        button_callbacks[i](BUTTON_PRESSED);
                    } else {
                        button_callbacks[i](BUTTON_RELEASED);
                    }
                }
            }
        } else if (btn->debounced == 0 && (current_time - btn->press_time) >= DEBOUNCE_TIME_MS) {
            /* State stable after debounce time */
            btn->debounced = 1;
        } else if (btn->state == 1 && btn->debounced == 1 && 
                   (current_time - btn->press_time) >= LONG_PRESS_TIME_MS) {
            /* Long press detected */
            if (button_callbacks[i] != NULL) {
                button_callbacks[i](BUTTON_LONG_PRESSED);
            }
            btn->debounced = 2;  /* Mark as long press reported */
        }
    }
}

/**
 * Check if button is currently pressed
 * Bare metal GPIO reading
 */
int buttons_is_pressed(button_t button) {
    if (button >= NUM_BUTTONS) {
        return 0;
    }
    
    uint8_t pin_state = gpio_read(buttons[button].port, buttons[button].pin);
    return pin_state ? 0 : 1;  /* Invert for active-low logic */
}

/**
 * Get button state
 */
button_event_t buttons_get_state(button_t button) {
    if (button >= NUM_BUTTONS) {
        return BUTTON_RELEASED;
    }
    
    return buttons[button].state ? BUTTON_PRESSED : BUTTON_RELEASED;
}

/**
 * External interrupt handlers for button presses
 * STM32F407 uses PD0-2, PD13-15 and PA0 for buttons
 * Bare metal EXTI handling with direct register access
 */

void EXTI0_IRQHandler(void) {
    /* Clear pending flag */
    gpio_exti_clear(0);
    button_interrupt_flags |= (1 << 0);
}

void EXTI1_IRQHandler(void) {
    /* Clear pending flag */
    gpio_exti_clear(1);
    button_interrupt_flags |= (1 << 1);
}

void EXTI2_IRQHandler(void) {
    /* Clear pending flag */
    gpio_exti_clear(2);
    button_interrupt_flags |= (1 << 2);
}

void EXTI15_10_IRQHandler(void) {
    /* Handles EXTI10-15 for PD13, PD14, PD15 */
    if (EXTI->PR & (1 << 13)) {
        gpio_exti_clear(13);
        button_interrupt_flags |= (1 << 3);
    }
    if (EXTI->PR & (1 << 14)) {
        gpio_exti_clear(14);
        button_interrupt_flags |= (1 << 4);
    }
    if (EXTI->PR & (1 << 15)) {
        gpio_exti_clear(15);
        button_interrupt_flags |= (1 << 5);
    }
}
