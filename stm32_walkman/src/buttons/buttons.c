/**
 * STM32 Button Handler - GPIO-based button input
 * Buttons: Previous, Play/Pause, Next, Volume Up, Volume Down
 */

#include "buttons.h"
#include "stm32f4xx_hal.h"
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
 */
int buttons_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    
    /* Configure PA0 as input with pull-up and interrupt on falling edge (User button) */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure PD0, PD1, PD2, PD13, PD14, PD15 as input with pull-up and interrupt on falling edge */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | 
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    /* Configure EXTI interrupts */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    
    HAL_NVIC_SetPriority(EXTI2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
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
 */
void buttons_poll(void) {
    uint32_t current_time = HAL_GetTick();
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_config_t* btn = &buttons[i];
        
        /* Read current GPIO state (0 = released, 1 = pressed on pull-up with falling edge) */
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin);
        uint8_t current_state = (pin_state == GPIO_PIN_RESET) ? 1 : 0;
        
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
 */
int buttons_is_pressed(button_t button) {
    if (button >= NUM_BUTTONS) {
        return 0;
    }
    
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(buttons[button].port, buttons[button].pin);
    return (pin_state == GPIO_PIN_RESET) ? 1 : 0;
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
 * STM32F407 uses PD13-15 and PD0-2 for buttons, PA0 for user button
 */

void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    button_interrupt_flags |= (1 << 0);
}

void EXTI1_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    button_interrupt_flags |= (1 << 1);
}

void EXTI2_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    button_interrupt_flags |= (1 << 2);
}

void EXTI15_10_IRQHandler(void) {
    /* Handles EXTI10-15 for PD13, PD14, PD15 */
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
        button_interrupt_flags |= (1 << 3);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
        button_interrupt_flags |= (1 << 4);
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
        button_interrupt_flags |= (1 << 5);
    }
}

/**
 * HAL interrupt callback
 * Called by HAL_GPIO_EXTI_IRQHandler for button press
 * Triggers immediate response (interrupt context - keep it short!)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    /* Mark button as needing debounce in main loop */
    /* The actual event handling happens in buttons_poll() after debounce */
}
