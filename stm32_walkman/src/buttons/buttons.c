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

/* Button definitions - adjust pins based on your STM32 board */
static button_config_t buttons[NUM_BUTTONS] = {
    {GPIOB, GPIO_PIN_0, BTN_PREVIOUS, 0, 0, 0},      // Previous track
    {GPIOB, GPIO_PIN_1, BTN_PLAY_PAUSE, 0, 0, 0},    // Play/Pause
    {GPIOB, GPIO_PIN_2, BTN_NEXT, 0, 0, 0},          // Next track
    {GPIOB, GPIO_PIN_3, BTN_VOL_UP, 0, 0, 0},        // Volume Up
    {GPIOB, GPIO_PIN_4, BTN_VOL_DOWN, 0, 0, 0},      // Volume Down
    {GPIOB, GPIO_PIN_5, BTN_SHUFFLE, 0, 0, 0},       // Shuffle
    {GPIOB, GPIO_PIN_6, BTN_LOOP, 0, 0, 0}           // Loop
};

/* Button callbacks */
static button_callback_t button_callbacks[NUM_BUTTONS] = {NULL};

/* Debounce timing */
#define DEBOUNCE_TIME_MS 20
#define LONG_PRESS_TIME_MS 1000

/**
 * Initialize button inputs
 */
int buttons_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* Configure buttons as input with pull-up */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | 
                          GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* Configure EXTI for button interrupts */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
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
 */
void buttons_poll(void) {
    uint32_t current_time = HAL_GetTick();
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_config_t* btn = &buttons[i];
        
        /* Read current GPIO state (0 = pressed, 1 = released on pull-up) */
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin);
        uint8_t current_state = (pin_state == GPIO_PIN_RESET) ? 1 : 0;
        
        /* Debounce logic */
        if (current_state != btn->state) {
            btn->press_time = current_time;
            btn->debounced = 0;
        } else if (!btn->debounced && (current_time - btn->press_time) >= DEBOUNCE_TIME_MS) {
            btn->debounced = 1;
            btn->state = current_state;
            
            /* Button event occurred */
            if (current_state == 1) {
                /* Button pressed */
                if (button_callbacks[i] != NULL) {
                    button_callbacks[i](BUTTON_PRESSED);
                }
            } else {
                /* Button released */
                if (button_callbacks[i] != NULL) {
                    button_callbacks[i](BUTTON_RELEASED);
                }
            }
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
 * External interrupt handler for button press (optional)
 */
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
 * HAL interrupt callback
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    /* Could be used for immediate button feedback */
    /* For this player, polling is sufficient */
}
