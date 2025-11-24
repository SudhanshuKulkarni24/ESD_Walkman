/**
 * STM32 Audio Player Core - WAV/MP3 Playback via I2S
 * Supports playlist management and playback control
 */

#include "player.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* Audio codec I2S handle */
I2S_HandleTypeDef hi2s3;

/* Audio buffer for DMA playback */
#define AUDIO_BUFFER_SIZE 4096
static uint8_t audio_buffer[AUDIO_BUFFER_SIZE];
static uint32_t audio_pos = 0;

/* Player state */
static player_t player_state = {
    .is_playing = 0,
    .is_paused = 0,
    .current_track = 0,
    .volume = 70,  // 0-100
    .shuffle_enabled = 0,
    .loop_mode = LOOP_OFF
};

/**
 * Initialize audio player
 */
int player_init(void) {
    // Initialize I2S for audio output
    hi2s3.Instance = SPI3;
    hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    hi2s3.Init.CPOL = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource = I2S_CLOCK_EXTERNAL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    
    if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
        return PLAYER_ERROR;
    }
    
    return PLAYER_OK;
}

/**
 * Load audio file (WAV or MP3)
 * Returns PLAYER_OK on success
 */
int player_load_file(const char* filename) {
    if (filename == NULL) {
        return PLAYER_ERROR;
    }
    
    // Check file extension
    const char* ext = filename + strlen(filename) - 3;
    if (strcmp(ext, "wav") != 0 && strcmp(ext, "mp3") != 0) {
        return PLAYER_ERROR_UNSUPPORTED;
    }
    
    // Store filename
    strncpy(player_state.current_file, filename, MAX_FILENAME_LEN - 1);
    player_state.current_file[MAX_FILENAME_LEN - 1] = '\0';
    
    return PLAYER_OK;
}

/**
 * Start playback
 */
int player_play(void) {
    if (player_state.current_file[0] == '\0') {
        return PLAYER_ERROR_NO_FILE;
    }
    
    player_state.is_playing = 1;
    player_state.is_paused = 0;
    audio_pos = 0;
    
    // Start I2S DMA transfer
    HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)audio_buffer, AUDIO_BUFFER_SIZE / 2);
    
    return PLAYER_OK;
}

/**
 * Pause playback
 */
int player_pause(void) {
    if (!player_state.is_playing) {
        return PLAYER_ERROR;
    }
    
    player_state.is_paused = 1;
    HAL_I2S_DMAPause(&hi2s3);
    
    return PLAYER_OK;
}

/**
 * Resume playback
 */
int player_resume(void) {
    if (!player_state.is_paused) {
        return PLAYER_ERROR;
    }
    
    player_state.is_paused = 0;
    HAL_I2S_DMAResume(&hi2s3);
    
    return PLAYER_OK;
}

/**
 * Stop playback
 */
int player_stop(void) {
    player_state.is_playing = 0;
    player_state.is_paused = 0;
    HAL_I2S_DMAStop(&hi2s3);
    audio_pos = 0;
    
    return PLAYER_OK;
}

/**
 * Set volume (0-100)
 */
int player_set_volume(uint8_t volume) {
    if (volume > 100) {
        return PLAYER_ERROR;
    }
    
    player_state.volume = volume;
    // TODO: Implement codec volume control via I2C
    
    return PLAYER_OK;
}

/**
 * Toggle shuffle mode
 */
int player_toggle_shuffle(void) {
    player_state.shuffle_enabled = !player_state.shuffle_enabled;
    return PLAYER_OK;
}

/**
 * Cycle loop mode: OFF -> ALL -> ONE -> OFF
 */
int player_cycle_loop(void) {
    switch (player_state.loop_mode) {
        case LOOP_OFF:
            player_state.loop_mode = LOOP_ALL;
            break;
        case LOOP_ALL:
            player_state.loop_mode = LOOP_ONE;
            break;
        case LOOP_ONE:
            player_state.loop_mode = LOOP_OFF;
            break;
    }
    return PLAYER_OK;
}

/**
 * Get current player state
 */
player_t* player_get_state(void) {
    return &player_state;
}

/**
 * Get playback position in seconds
 */
uint32_t player_get_position(void) {
    // Calculate position based on audio_pos
    // Assuming 44100 Hz, 16-bit, stereo = 4 bytes per sample
    uint32_t samples = audio_pos / 4;
    return samples / 44100;
}

/**
 * DMA Transmit Complete Callback
 */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s == &hi2s3) {
        audio_pos += AUDIO_BUFFER_SIZE / 2;
        // Load next audio chunk from SD card
        // TODO: Implement audio file reading from SD card
    }
}

/**
 * DMA Transmit Half Complete Callback
 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s == &hi2s3) {
        // Refill first half of buffer
        // TODO: Read from audio file
    }
}
