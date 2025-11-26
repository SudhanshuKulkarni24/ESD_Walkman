/**
 * STM32F407 Audio Player Core - Codec-based Audio Playback
 * BARE METAL - No HAL, direct register access via bare metal drivers
 * Supports playlist management and playback control
 * 
 * Audio System: WM8994 codec via I2S3 interface
 * Sample Rate: 44100 Hz (configurable to 48kHz, 96kHz)
 * Resolution: 16-bit stereo
 */

#include "player.h"
#include "codec.h"
#include "i2s.h"
#include <string.h>
#include <stdio.h>

/* Audio buffer for playback */
#define AUDIO_BUFFER_SIZE 44100  // 1 second at 44.1kHz (192KB RAM available on F407)
static int16_t audio_buffer[AUDIO_BUFFER_SIZE];
static uint32_t audio_buffer_pos = 0;
static uint32_t audio_buffer_len = 0;

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
 * 
 * Initializes WM8994 codec via I2C1 and I2S3
 */
int player_init(void) {
    // Initialize codec (I2C1 for control, I2S3 for audio)
    if (codec_init() != CODEC_OK) {
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
 * 
 * Note: For F407 with 192KB RAM, audio buffer is larger (1 second vs 500ms on F401)
 * Audio is streamed from SD card via SDIO and played through codec
 */
int player_play(void) {
    if (audio_buffer_len == 0) {
        return PLAYER_ERROR_NO_FILE;
    }
    
    player_state.is_playing = 1;
    player_state.is_paused = 0;
    audio_buffer_pos = 0;
    
    // Start codec audio playback via I2S3 DMA
    if (codec_play(audio_buffer, audio_buffer_len) != CODEC_OK) {
        return PLAYER_ERROR;
    }
    
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
    codec_pause();
    
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
    codec_resume();
    
    return PLAYER_OK;
}

/**
 * Stop playback
 */
int player_stop(void) {
    player_state.is_playing = 0;
    player_state.is_paused = 0;
    codec_stop();
    audio_buffer_pos = 0;
    
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
    codec_set_volume(volume);
    
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
    uint32_t samples = codec_get_position();
    return samples / CODEC_SAMPLE_RATE_44100;
}

/**
 * DMA Transmit Complete Callback
 * 
 * For F401RE, this would be called when streaming audio from SD card
 * TODO: Implement streaming from SD card in chunks
 */
void audio_stream_callback(void) {
    // This callback would be used for streaming audio from SD card
    // For now, playback is handled entirely in PWM interrupt
}
