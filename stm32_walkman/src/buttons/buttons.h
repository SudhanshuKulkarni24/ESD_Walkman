/**
 * Button Handler Header
 */

#ifndef __BUTTONS_H
#define __BUTTONS_H

#include <stdint.h>

/* Button identifiers */
typedef enum {
    BTN_PREVIOUS = 0,
    BTN_PLAY_PAUSE,
    BTN_NEXT,
    BTN_VOL_UP,
    BTN_VOL_DOWN,
    BTN_SHUFFLE,
    BTN_LOOP,
    NUM_BUTTONS
} button_t;

/* Button event types */
typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED = 1,
    BUTTON_LONG_PRESSED = 2
} button_event_t;

/* Return status */
typedef enum {
    BUTTONS_OK = 0,
    BUTTONS_ERROR = 1
} buttons_status_t;

/* Button callback function type */
typedef void (*button_callback_t)(button_event_t event);

/* Public functions */
int buttons_init(void);
int buttons_register_callback(button_t button, button_callback_t callback);
void buttons_poll(void);
int buttons_is_pressed(button_t button);
button_event_t buttons_get_state(button_t button);

#endif /* __BUTTONS_H */
