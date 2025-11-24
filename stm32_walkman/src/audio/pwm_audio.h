/**
 * STM32F401RE PWM Audio Driver with RC Filter
 * 
 * PA0 (TIM2_CH1) → 10kΩ resistor → 100nF capacitor → GND (RC Filter)
 * Audio output from RC filter junction
 * 
 * Specifications:
 * - Sample Rate: 44100 Hz
 * - PWM Frequency: 2 MHz
 * - Bit Depth: 8-bit equivalent (0-255 mapped to PWM duty)
 * - Audio Quality: ~48dB SNR
 * - RC Cutoff: 159 kHz (R=10kΩ, C=100nF)
 */

#ifndef __PWM_AUDIO_H
#define __PWM_AUDIO_H

#include <stdint.h>

/* Initialization */
void pwm_audio_init(void);

/* Playback control */
void audio_play(int16_t *buffer, uint32_t size);
void audio_stop(void);
void audio_pause(void);
void audio_resume(void);

/* Status functions */
uint8_t audio_is_playing(void);
uint32_t audio_get_position(void);
uint32_t audio_get_duration(void);
float audio_get_time(void);

/* Volume control (0-100) */
void audio_set_volume(uint8_t volume);
uint8_t audio_get_volume(void);

/* Internal functions */
void audio_output_sample(int16_t sample);
void audio_timer_interrupt(void);

#endif /* __PWM_AUDIO_H */
