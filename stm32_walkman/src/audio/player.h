/**
 * STM32 Audio Player Header
 */

#ifndef __PLAYER_H
#define __PLAYER_H

#include <stdint.h>

#define MAX_FILENAME_LEN 256
#define MAX_PLAYLIST_SIZE 100

typedef enum {
    PLAYER_OK = 0,
    PLAYER_ERROR = 1,
    PLAYER_ERROR_NO_FILE = 2,
    PLAYER_ERROR_UNSUPPORTED = 3
} player_status_t;

typedef enum {
    LOOP_OFF = 0,
    LOOP_ALL = 1,
    LOOP_ONE = 2
} loop_mode_t;

typedef struct {
    uint8_t is_playing;
    uint8_t is_paused;
    uint8_t shuffle_enabled;
    loop_mode_t loop_mode;
    uint8_t current_track;
    uint8_t volume;  // 0-100
    char current_file[MAX_FILENAME_LEN];
} player_t;

/* Player control functions */
int player_init(void);
int player_load_file(const char* filename);
int player_play(void);
int player_pause(void);
int player_resume(void);
int player_stop(void);
int player_set_volume(uint8_t volume);
int player_toggle_shuffle(void);
int player_cycle_loop(void);
player_t* player_get_state(void);
uint32_t player_get_position(void);

#endif /* __PLAYER_H */
