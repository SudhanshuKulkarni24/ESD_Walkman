/**
 * STM32F401RE PWM Audio Implementation
 * 
 * Hardware:
 * - MCU: STM32F401RET6
 * - System Clock: 84 MHz
 * - PWM Output: PA0 (TIM2_CH1)
 * - RC Filter: R=10kΩ, C=100nF, fc=159kHz
 * - Sample Rate: 44100 Hz
 * 
 * Pin Configuration:
 * PA0 → PWM output (via 10kΩ resistor)
 *       → Capacitor to GND (100nF)
 *       → Audio jack output from junction
 * 
 * Timer Configuration:
 * - TIM2: PWM generation at 2MHz (84MHz / 42 = 2MHz)
 * - Period: 21 counts (0-20)
 * - Channel 1: PWM output
 * 
 * Interrupt Configuration:
 * - TIM2 base timer for sampling at 44.1kHz
 * - Prescaler: 0
 * - Period: 952 counts (84MHz / 952 ≈ 44.1kHz)
 */

#include "pwm_audio.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============ Configuration Constants ============ */

#define SAMPLE_RATE         44100
#define PWM_FREQUENCY       2000000  // 2 MHz
#define PWM_PERIOD          21       // (84MHz / 2MHz) - 1
#define TIMER_PERIOD        952      // (84MHz / 44100) - 1

/* ============ Timer Handles ============ */

static TIM_HandleTypeDef htim2_pwm;  // PWM generation timer
static TIM_HandleTypeDef htim2_sample;  // Sampling timer

/* ============ Audio Playback State ============ */

static struct {
    volatile int16_t *buffer;
    volatile uint32_t buffer_size;
    volatile uint32_t sample_index;
    volatile uint8_t is_playing;
    volatile uint8_t is_paused;
    uint8_t volume;  // 0-100
} audio_state = {
    .buffer = NULL,
    .buffer_size = 0,
    .sample_index = 0,
    .is_playing = 0,
    .is_paused = 0,
    .volume = 70
};

/* ============ Volume Table ============ */

// Volume scaling from 0-100 to 0-1.0
static inline float get_volume_scale(uint8_t vol) {
    return (float)vol / 100.0f;
}

/* ============ Initialization ============ */

/**
 * Initialize GPIO PA0 as PWM output
 */
static void pwm_gpio_init(void) {
    // Enable GPIOA clock
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA0 as alternate function (TIM2_CH1)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * Initialize TIM2 for PWM generation (2MHz)
 */
static void pwm_timer_init(void) {
    // Enable TIM2 clock
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    // Configure TIM2 for PWM
    htim2_pwm.Instance = TIM2;
    htim2_pwm.Init.Prescaler = 0;  // No prescaling
    htim2_pwm.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_pwm.Init.Period = PWM_PERIOD - 1;  // Period = 21 counts
    htim2_pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2_pwm.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim2_pwm) != HAL_OK) {
        return;  // Error
    }
    
    // Configure PWM output on channel 1
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 10;  // 50% duty initially
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    if (HAL_TIM_PWM_ConfigChannel(&htim2_pwm, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return;  // Error
    }
    
    // Start PWM output
    HAL_TIM_PWM_Start(&htim2_pwm, TIM_CHANNEL_1);
}

/**
 * Initialize TIM2 base timer for 44.1kHz sampling interrupt
 */
static void sample_timer_init(void) {
    // TIM2 clock is already enabled
    
    // Configure TIM2 for interrupt at 44.1kHz
    htim2_sample.Instance = TIM2;
    htim2_sample.Init.Prescaler = 0;  // No prescaling
    htim2_sample.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_sample.Init.Period = TIMER_PERIOD - 1;  // Count to 952
    htim2_sample.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2_sample.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    if (HAL_TIM_Base_Init(&htim2_sample) != HAL_OK) {
        return;  // Error
    }
    
    // Enable TIM2 interrupt with highest priority
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    
    // Start timer with interrupt
    HAL_TIM_Base_Start_IT(&htim2_sample);
}

/**
 * Main initialization function
 */
void pwm_audio_init(void) {
    pwm_gpio_init();
    pwm_timer_init();
    sample_timer_init();
}

/* ============ Audio Output ============ */

/**
 * Output audio sample via PWM
 * 
 * Input: 16-bit signed audio sample (-32768 to 32767)
 * Maps to PWM duty cycle (0 to 20 counts out of 21)
 * Center (0) = 50% duty (10 counts)
 */
void audio_output_sample(int16_t sample) {
    // Apply volume scaling
    float volume_scale = get_volume_scale(audio_state.volume);
    int32_t scaled_sample = (int32_t)(sample * volume_scale);
    
    // Clamp to 16-bit range
    if (scaled_sample > 32767) scaled_sample = 32767;
    if (scaled_sample < -32768) scaled_sample = -32768;
    
    // Convert from signed (-32768 to 32767) to unsigned (0 to 21)
    // Formula: (sample + 32768) * PWM_PERIOD / 65536
    uint32_t pwm_duty = ((int32_t)scaled_sample + 32768) * PWM_PERIOD / 65536;
    
    // Clamp to valid PWM range
    if (pwm_duty > PWM_PERIOD - 1) pwm_duty = PWM_PERIOD - 1;
    if (pwm_duty < 0) pwm_duty = 0;
    
    // Set PWM compare register
    __HAL_TIM_SET_COMPARE(&htim2_pwm, TIM_CHANNEL_1, pwm_duty);
}

/* ============ Playback Control ============ */

/**
 * Start playing audio from buffer
 * 
 * @param buffer: Pointer to 16-bit signed audio samples
 * @param size: Number of samples in buffer
 */
void audio_play(int16_t *buffer, uint32_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }
    
    audio_state.buffer = buffer;
    audio_state.buffer_size = size;
    audio_state.sample_index = 0;
    audio_state.is_playing = 1;
    audio_state.is_paused = 0;
}

/**
 * Stop audio playback immediately
 */
void audio_stop(void) {
    audio_state.is_playing = 0;
    audio_state.is_paused = 0;
    audio_state.sample_index = 0;
    
    // Output silence (50% duty = zero)
    __HAL_TIM_SET_COMPARE(&htim2_pwm, TIM_CHANNEL_1, PWM_PERIOD / 2);
}

/**
 * Pause audio playback
 */
void audio_pause(void) {
    if (audio_state.is_playing) {
        audio_state.is_paused = 1;
    }
}

/**
 * Resume audio playback from pause
 */
void audio_resume(void) {
    if (audio_state.is_paused) {
        audio_state.is_paused = 0;
    }
}

/* ============ Status Functions ============ */

/**
 * Check if audio is currently playing
 */
uint8_t audio_is_playing(void) {
    return audio_state.is_playing && !audio_state.is_paused;
}

/**
 * Get current sample position
 */
uint32_t audio_get_position(void) {
    return audio_state.sample_index;
}

/**
 * Get total duration in samples
 */
uint32_t audio_get_duration(void) {
    return audio_state.buffer_size;
}

/**
 * Get current playback time in seconds
 */
float audio_get_time(void) {
    if (audio_state.buffer_size == 0) return 0.0f;
    return (float)audio_state.sample_index / SAMPLE_RATE;
}

/* ============ Volume Control ============ */

/**
 * Set volume level (0-100)
 */
void audio_set_volume(uint8_t volume) {
    if (volume > 100) {
        audio_state.volume = 100;
    } else {
        audio_state.volume = volume;
    }
}

/**
 * Get current volume level
 */
uint8_t audio_get_volume(void) {
    return audio_state.volume;
}

/* ============ Interrupt Handler ============ */

/**
 * TIM2 IRQ Handler
 */
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        // Clear interrupt flag
        TIM2->SR &= ~TIM_SR_UIF;
        
        // Output next audio sample
        if (audio_state.is_playing && !audio_state.is_paused) {
            if (audio_state.sample_index < audio_state.buffer_size) {
                int16_t sample = audio_state.buffer[audio_state.sample_index++];
                audio_output_sample(sample);
            } else {
                // Playback finished
                audio_state.is_playing = 0;
            }
        }
    }
}

/* ============ Helper Functions ============ */

/**
 * Generate test sine wave
 * Used for testing PWM audio output
 */
void audio_generate_sine_wave(int16_t *buffer, uint32_t samples, float frequency) {
    for (uint32_t i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sine = 2.0f * 3.14159265359f * frequency * t;
        buffer[i] = (int16_t)(sin(sine) * 32767);
    }
}

/**
 * Generate square wave (for testing)
 */
void audio_generate_square_wave(int16_t *buffer, uint32_t samples, float frequency) {
    for (uint32_t i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float phase = frequency * t;
        phase -= (int)phase;  // Get fractional part
        buffer[i] = (phase < 0.5f) ? 32767 : -32768;
    }
}
