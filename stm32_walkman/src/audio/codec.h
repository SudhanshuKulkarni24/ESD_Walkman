/**
 * WM8994 Audio Codec Driver for STM32F407 Discovery
 * 
 * I2S3 Interface:
 * - PC7: I2S3_MCLK (Master Clock)
 * - PC10: I2S3_CK (Serial Clock)
 * - PC12: I2S3_SD (Serial Data)
 * - PA4: I2S3_WS (Word Select)
 * 
 * I2C1 Interface (Codec Control):
 * - PB6: I2C1_SCL
 * - PB7: I2C1_SDA
 * - Codec I2C address: 0x1A (7-bit, 0x34 in 8-bit)
 * 
 * Features:
 * - 16-bit stereo audio at 44.1kHz, 48kHz, or 96kHz
 * - 24-bit internal processing
 * - Software volume control (0-100%)
 * - Configurable input/output routing
 */

#ifndef __WM8994_CODEC_H
#define __WM8994_CODEC_H

#include <stdint.h>

/* Codec status */
typedef enum {
    CODEC_OK = 0,
    CODEC_ERROR = 1,
    CODEC_TIMEOUT = 2
} codec_status_t;

/* Sample rates */
typedef enum {
    CODEC_SAMPLE_RATE_44100 = 44100,
    CODEC_SAMPLE_RATE_48000 = 48000,
    CODEC_SAMPLE_RATE_96000 = 96000
} codec_sample_rate_t;

/* Audio input source */
typedef enum {
    CODEC_INPUT_LINE = 0,    // Line in
    CODEC_INPUT_MIC = 1      // Microphone
} codec_input_source_t;

/* Audio output destination */
typedef enum {
    CODEC_OUTPUT_LINE = 0,   // Line out (headphone jack)
    CODEC_OUTPUT_SPEAKER = 1 // Speaker
} codec_output_dest_t;

/* Function prototypes */
codec_status_t codec_init(void);
codec_status_t codec_deinit(void);

/* Playback control */
codec_status_t codec_play(const int16_t *buffer, uint32_t size);
codec_status_t codec_stop(void);
codec_status_t codec_pause(void);
codec_status_t codec_resume(void);

/* Configuration */
codec_status_t codec_set_sample_rate(codec_sample_rate_t rate);
codec_status_t codec_set_input_source(codec_input_source_t source);
codec_status_t codec_set_output_destination(codec_output_dest_t dest);

/* Volume control (0-100) */
codec_status_t codec_set_volume(uint8_t volume);
uint8_t codec_get_volume(void);
codec_status_t codec_set_mic_gain(uint8_t gain);

/* Status */
uint8_t codec_is_playing(void);
uint32_t codec_get_position(void);

/* I2S interrupt handler */
void codec_i2s_interrupt_handler(void);

/* Low-level I2C functions */
codec_status_t codec_read_register(uint16_t addr, uint16_t *value);
codec_status_t codec_write_register(uint16_t addr, uint16_t value);

#endif /* __WM8994_CODEC_H */
