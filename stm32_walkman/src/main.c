/**
 * STM32F407 Discovery Walkman Music Player - Main Application
 * BARE METAL - No HAL layer, direct register access
 * 
 * Hardware:
 * - MCU: STM32F407VGT6 Discovery board
 * - Audio: On-board WM8994 audio codec via I2S3 + I2C1
 * - Display: ILI9341 240x320 LCD via SPI5
 * - Input: 7 GPIO buttons (PA0, PA13-PA15, PD13-PD15)
 * - Storage: SD card via SDIO (built-in, no SPI needed)
 * - User LED: PD12 (green)
 * 
 * Features:
 * - Playlist management (from SD card)
 * - Play/Pause/Stop controls
 * - Volume control (0-100%) via WM8994 codec
 * - Shuffle and loop modes
 * - Real-time playback display
 * - MP3/WAV file support (via codec DAC)
 * - True stereo audio output
 * 
 * Memory Constraints (F407):
 * - RAM: 192KB total
 * - Audio buffer: 44100 samples = 1 second at 44.1kHz
 * - Audio streamed from SD card via SDIO
 * 
 * I2S Audio Chain:
 * STM32F407 I2S3 → WM8994 codec → Line-out (stereo jack)
 * I2C1 for codec configuration
 */

#include "system.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include "i2s.h"
#include "player.h"
#include "lcd_display.h"
#include "buttons.h"
#include <stdio.h>
#include <string.h>

/* Configuration */
#define UPDATE_INTERVAL_MS 100
#define VOLUME_STEP 5

/* Global state */
typedef struct {
    uint32_t last_update;
    char playlist[100][256];
    uint8_t playlist_count;
    uint8_t current_track;
} app_state_t;

static app_state_t app;
static player_t* player;

/* Forward declarations */
void app_init(void);
void app_loop(void);
void app_button_prev(button_event_t event);
void app_button_play(button_event_t event);
void app_button_next(button_event_t event);
void app_button_vol_up(button_event_t event);
void app_button_vol_down(button_event_t event);
void app_button_shuffle(button_event_t event);
void app_button_loop(button_event_t event);
void app_load_playlist(const char* directory);
void app_update_display(void);

/**
 * Main application entry point
 */
int main(void) {
    /* Initialize system clock and SysTick */
    system_init();
    
    /* Enable SysTick timer */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    /* Initialize subsystems */
    app_init();
    
    /* Main application loop */
    while (1) {
        app_loop();
    }
    
    return 0;
}

/**
 * Initialize application
 */
void app_init(void) {
    printf("STM32 Walkman Player - Initializing...\n");
    
    /* Initialize audio player */
    if (player_init() != PLAYER_OK) {
        printf("Error: Failed to initialize audio player\n");
        while (1);
    }
    printf("Audio player initialized\n");
    
    /* Initialize LCD display */
    if (lcd_init() != LCD_OK) {
        printf("Error: Failed to initialize LCD\n");
        while (1);
    }
    printf("LCD display initialized\n");
    
    /* Initialize buttons */
    if (buttons_init() != BUTTONS_OK) {
        printf("Error: Failed to initialize buttons\n");
        while (1);
    }
    printf("Buttons initialized\n");
    
    /* Register button callbacks */
    buttons_register_callback(BTN_PREVIOUS, app_button_prev);
    buttons_register_callback(BTN_PLAY_PAUSE, app_button_play);
    buttons_register_callback(BTN_NEXT, app_button_next);
    buttons_register_callback(BTN_VOL_UP, app_button_vol_up);
    buttons_register_callback(BTN_VOL_DOWN, app_button_vol_down);
    buttons_register_callback(BTN_SHUFFLE, app_button_shuffle);
    buttons_register_callback(BTN_LOOP, app_button_loop);
    
    /* Load playlist from SD card */
    app_load_playlist("/music");
    
    /* Display startup message */
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    lcd_draw_text(10, 150, "WALKMAN PLAYER", COLOR_GREEN, COLOR_BLACK, 2);
    lcd_draw_text(10, 180, "Loading...", COLOR_GRAY, COLOR_BLACK, 1);
    
    app.last_update = HAL_GetTick();
    printf("Application initialized\n");
}

/**
 * Main application loop
 */
void app_loop(void) {
    uint32_t current_time = system_get_tick();
    
    /* Poll button inputs */
    buttons_poll();
    
    /* Update display periodically */
    if ((current_time - app.last_update) >= UPDATE_INTERVAL_MS) {
        app_update_display();
        app.last_update = current_time;
    }
}

/**
 * Button callbacks
 */
void app_button_prev(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Previous\n");
        if (app.current_track > 0) {
            app.current_track--;
            player_load_file(app.playlist[app.current_track]);
            player_play();
        }
    }
}

void app_button_play(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Play/Pause\n");
        player_t* state = player_get_state();
        
        if (state->is_playing && !state->is_paused) {
            player_pause();
        } else if (state->is_paused) {
            player_resume();
        } else {
            if (app.current_track < app.playlist_count) {
                player_load_file(app.playlist[app.current_track]);
            }
            player_play();
        }
    }
}

void app_button_next(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Next\n");
        if (app.current_track < app.playlist_count - 1) {
            app.current_track++;
            player_load_file(app.playlist[app.current_track]);
            player_play();
        }
    }
}

void app_button_vol_up(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Volume Up\n");
        player_t* state = player_get_state();
        uint8_t new_vol = state->volume + VOLUME_STEP;
        if (new_vol > 100) new_vol = 100;
        player_set_volume(new_vol);
    }
}

void app_button_vol_down(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Volume Down\n");
        player_t* state = player_get_state();
        int new_vol = (int)state->volume - VOLUME_STEP;
        if (new_vol < 0) new_vol = 0;
        player_set_volume((uint8_t)new_vol);
    }
}

void app_button_shuffle(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Shuffle\n");
        player_toggle_shuffle();
    }
}

void app_button_loop(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        printf("Button: Loop\n");
        player_cycle_loop();
    }
}

/**
 * Load playlist from directory (stub - would enumerate SD card)
 */
void app_load_playlist(const char* directory) {
    /* TODO: Enumerate SD card directory and load music files */
    /* For now, add some test files */
    
    strcpy(app.playlist[0], "song1.mp3");
    strcpy(app.playlist[1], "song2.wav");
    strcpy(app.playlist[2], "song3.mp3");
    
    app.playlist_count = 3;
    app.current_track = 0;
    
    printf("Loaded %d tracks\n", app.playlist_count);
}

/**
 * Update display with current playback info
 */
void app_update_display(void) {
    player_t* state = player_get_state();
    
    if (state->current_file[0] == '\0') {
        lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
        lcd_draw_text(10, 150, "NO SONGS", COLOR_GRAY, COLOR_BLACK, 1);
        return;
    }
    
    /* Get playback position */
    uint32_t position = player_get_position();
    
    /* Extract filename for display */
    const char* filename = state->current_file;
    const char* slash = strrchr(filename, '/');
    if (slash) filename = slash + 1;
    
    /* Display song info */
    char status[32];
    if (state->is_playing) {
        if (state->is_paused) {
            sprintf(status, "▮▮ PAUSED • Vol: %d%%", state->volume);
        } else {
            sprintf(status, "▶ PLAYING • Vol: %d%%", state->volume);
        }
    } else {
        sprintf(status, "⏹ STOPPED");
    }
    
    /* Show shuffle/loop status */
    char mode_str[32] = "";
    if (state->shuffle_enabled) {
        strcat(mode_str, "🔀 ");
    }
    switch (state->loop_mode) {
        case LOOP_ONE:
            strcat(mode_str, "🔁");
            break;
        case LOOP_ALL:
            strcat(mode_str, "⟲");
            break;
        default:
            break;
    }
    
    /* Display on LCD */
    lcd_display_song_info(filename, status, 180, position);
    
    if (mode_str[0]) {
        lcd_display_status(mode_str);
    }
}

/**
 * System clock configuration for STM32F407
 * REMOVED - Now using bare metal system_init()
 * 
 * System clock is configured in system.c via direct register access
 * For details, see system_init() function
 */

/**
 * Assert failed handler
 */
void assert_failed(uint8_t* file, uint32_t line) {
    printf("Assert failed at %s:%lu\n", (char*)file, line);
}
