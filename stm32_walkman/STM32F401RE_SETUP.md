# STM32F401RE Nucleo Board - Walkman Player Setup Guide

Complete guide for implementing audio output and music player on **STM32F401RE Nucleo** board using bypass methods (no external codec required).

---

## Part 1: STM32F401RE Board Overview

### 1.1 Specifications

| Specification | Details |
|---------------|---------|
| **MCU** | STM32F401RET6 |
| **Core** | 32-bit ARM Cortex-M4 |
| **Clock** | 84 MHz (can be overclocked to 100+ MHz with care) |
| **Flash Memory** | 512 KB |
| **RAM** | 96 KB |
| **GPIO Pins** | 50 available |
| **Timers** | 7 timers (TIM1, TIM2-TIM5, TIM9-TIM11) |
| **Interfaces** | SPI, I2C, USART, USB OTG |
| **Built-in Debug** | ST-Link/V2 USB interface |
| **Power** | 3.3V (USB powered) |

### 1.2 Audio Capabilities

```
STM32F401RE Feature Analysis:

✅ What It HAS:
  - 7 Timers (can use for PWM or interrupt timing)
  - Multiple SPI interfaces (for LCD)
  - GPIO pins (for buttons and audio output)
  - 96KB RAM (enough for audio buffers)

❌ What It DOESN'T Have:
  - No I2S interface ❌
  - No SAI interface ❌
  - No DAC (Digital-to-Analog Converter) ❌
  - No audio codec ❌

Result: CANNOT use I2S audio directly!
Solution: Use GPIO-based audio methods (PWM or R-2R)
```

### 1.3 Why No I2S on F401?

```
STM32 Family Audio Support:

F401 Series:
  - Entry-level Cortex-M4
  - Fewer peripherals
  - NO I2S, NO DAC, NO audio codec
  - Audio requires GPIO-based workarounds

F407/F429 Series:
  - Full-featured Cortex-M4
  - HAS I2S3 interface
  - Optional external codec
  - Better for audio applications

F446/F469 Series:
  - Advanced Cortex-M4
  - SAI1/SAI2 interfaces (better than I2S)
  - Still need external codec

Lesson: If audio is critical, use F407/F429 or Discovery board!
For learning, F401 works fine with GPIO methods.
```

---

## Part 2: Audio Output Options for F401RE

### 2.1 Recommended: PWM-Based Audio ⭐⭐⭐

```
Why PWM is best for F401:

✅ ADVANTAGES:
  - Uses built-in timer (TIM2, TIM3, etc.)
  - Simple software implementation
  - Minimal hardware needed (just 1 RC filter)
  - Works immediately
  - Good enough for headphones
  - Low cost ($1-2)

❌ DISADVANTAGES:
  - Lower audio quality (~48dB SNR)
  - CPU-intensive (timer interrupts at 44.1kHz)
  - Requires low-pass filter
  - Not ideal for speakers alone (need amp)

VERDICT: Use this! It's the easiest.
```

### 2.2 Alternative: R-2R Ladder (More Complex)

```
✅ ADVANTAGES:
  - True analog output (not PWM)
  - Better audio quality (~50dB SNR)
  - Less CPU dependent
  - Good for speakers

❌ DISADVANTAGES:
  - 8 GPIO pins needed (PA0-PA7)
  - More components (16 resistors)
  - Needs op-amp buffer
  - More wiring complexity
  - Requires 1% resistors (precision)

VERDICT: Skip this for F401. PWM is easier.
```

### 2.3 Not Recommended: External Codec

```
❌ Why NOT to use external codec on F401:

1. NO I2S interface - must bit-bang I2S in software
   - Very CPU intensive
   - Timing critical
   - Unreliable

2. NO I2C (built-in I2C exists, but limited)
   - Codec control possible but slow

3. Better options exist:
   - Use Discovery board (has codec built-in)
   - Use F407 Nucleo (has I2S)
   - Use GPIO methods (PWM works fine)

VERDICT: Not worth the effort!
```

---

## Part 3: PWM Audio on STM32F401RE

### 3.1 Pin Selection

**Available PWM Pins on F401RE:**

```
Timer 2 (TIM2):
  ✅ TIM2_CH1 → PA0  (BEST for audio)
  ✅ TIM2_CH2 → PA1
  ✅ TIM2_CH3 → PA2
  ✅ TIM2_CH4 → PA3

Timer 3 (TIM3):
  ✅ TIM3_CH1 → PA6
  ✅ TIM3_CH2 → PA7
  ✅ TIM3_CH3 → PB0
  ✅ TIM3_CH4 → PB1

Timer 4 (TIM4):
  ✅ TIM4_CH1 → PB6
  ✅ TIM4_CH2 → PB7
  ✅ TIM4_CH3 → PB8
  ✅ TIM4_CH4 → PB9

Timer 5 (TIM5):
  ✅ TIM5_CH1 → PA0
  ✅ TIM5_CH2 → PA1
  ✅ TIM5_CH3 → PA2
  ✅ TIM5_CH4 → PA3

Nucleo Pinout (Arduino Compatible):
  PA0  ←→ Arduino A0
  PA1  ←→ Arduino A1
  PA4  ←→ Arduino A2
  PA5  ←→ Arduino A3
  PA6  ←→ Arduino A4
  PA7  ←→ Arduino A5
  PB0  ←→ Arduino A6
  PB1  ←→ Arduino A7

RECOMMENDATION:
Use PA0 (TIM2_CH1) - most accessible, standard pin
```

### 3.2 Hardware Schematic

```
Simple PWM Audio Output:

STM32F401RE
┌──────────────────┐
│ PA0 (PWM output) ├────[10kΩ]────┬──────► Audio Out
│                  │               │
└──────────────────┘              [100nF]
                                   │
                                  GND


Optional Amplifier for Speakers:

Audio Out ────┬────────────────────┬─────► Speaker+
              │   TDA7052 or       │
             [1µF]   LM386        [8Ω]
              │   Amplifier        │
             GND                   │
                                   ↓
                               Speaker-


RC Filter Values:
R = 10kΩ
C = 100nF
Cutoff freq: fc = 1/(2πRC) = 159 kHz
Attenuation at 1MHz: -18dB ✓
```

### 3.3 Complete C Code for PWM Audio

```c
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <math.h>

// ============ PWM Audio System for STM32F401RE ============

#define SAMPLE_RATE 44100
#define PWM_FREQUENCY 2000000  // 2 MHz PWM frequency

// Timer handle
TIM_HandleTypeDef htim2;

// Audio playback state
volatile uint8_t audio_playing = 0;
volatile uint32_t sample_index = 0;
int16_t *audio_buffer = NULL;
uint32_t audio_buffer_size = 0;

// ============ Initialization ============

// Configure PWM output on PA0 using TIM2_CH1
void pwm_audio_init(void) {
    // Enable clock for GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Enable clock for TIM2
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    // Configure PA0 as PWM output (Alternate Function)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;           // Alternate Function Push-Pull
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;       // TIM2 alternate function
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure TIM2 for PWM
    // F401 APB1 clock = 42 MHz
    // Desired PWM freq: 2 MHz
    // Prescaler: 0 (no division)
    // Period: 42MHz / 2MHz = 21 counts
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 21 - 1;  // 0-20, gives 21 counts = 2MHz
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        // Error: Timer initialization failed
        return;
    }
    
    // Configure PWM channel 1
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 10;  // 50% duty cycle initially
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        // Error: Channel configuration failed
        return;
    }
    
    // Start PWM output
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

// Configure Timer2 to generate interrupt at 44.1kHz for audio sampling
void audio_timer_init(void) {
    // F401 APB1 clock = 42 MHz
    // Desired: 44100 samples/sec
    // Period = 42MHz / 44100 = 952.38 ≈ 952
    
    TIM_HandleTypeDef htim2_sample;
    htim2_sample.Instance = TIM2;
    htim2_sample.Init.Prescaler = 0;
    htim2_sample.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_sample.Init.Period = 952 - 1;  // Count to 952
    htim2_sample.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2_sample.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    HAL_TIM_Base_Init(&htim2_sample);
    
    // Enable TIM2 interrupt
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);  // Highest priority for audio timing
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    
    // Start timer with interrupt
    HAL_TIM_Base_Start_IT(&htim2_sample);
}

// ============ Audio Sample Conversion & Output ============

// Convert 16-bit signed audio sample to PWM duty cycle
// Input: -32768 to 32767
// Output: PWM duty (0 to 20 for 2MHz PWM with period 21)
void output_audio_sample(int16_t sample) {
    // Scale from 16-bit signed to PWM duty cycle
    // Center point: 0 → duty = 10 (50%)
    // Max positive: 32767 → duty = 20 (100%)
    // Max negative: -32768 → duty = 0 (0%)
    
    int32_t pwm_duty = ((int32_t)sample + 32768) * 21 / 65536;
    
    // Clamp to valid range
    if (pwm_duty > 20) pwm_duty = 20;
    if (pwm_duty < 0)  pwm_duty = 0;
    
    // Set PWM duty cycle
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_duty);
}

// Convert 16-bit signed to properly scaled format
uint16_t convert_audio_sample(int16_t raw_sample) {
    // Convert from signed (-32768 to 32767) to unsigned (0 to 21)
    uint32_t unsigned_sample = (uint32_t)(raw_sample + 32768);
    unsigned_sample = (unsigned_sample * 21) >> 16;  // Scale to 0-21
    return (uint16_t)unsigned_sample;
}

// ============ Playback Control ============

// Start playing audio from buffer
void audio_play(int16_t *buffer, uint32_t size) {
    audio_buffer = buffer;
    audio_buffer_size = size;
    sample_index = 0;
    audio_playing = 1;
}

// Stop audio playback
void audio_stop(void) {
    audio_playing = 0;
}

// Check if audio is currently playing
uint8_t audio_is_playing(void) {
    return audio_playing;
}

// Get current playback position
uint32_t audio_get_position(void) {
    return sample_index;
}

// Get playback duration in seconds
float audio_get_duration(void) {
    if (audio_buffer_size == 0) return 0.0f;
    return (float)audio_buffer_size / SAMPLE_RATE;
}

// Get current playback time in seconds
float audio_get_time(void) {
    if (audio_buffer_size == 0) return 0.0f;
    return (float)sample_index / SAMPLE_RATE;
}

// ============ Interrupt Handler ============

// TIM2 Interrupt Handler - called at 44.1kHz
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}

// HAL callback when timer period elapses
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // Output next audio sample
        if (audio_playing && sample_index < audio_buffer_size) {
            int16_t raw_sample = audio_buffer[sample_index++];
            output_audio_sample(raw_sample);
        } else if (sample_index >= audio_buffer_size) {
            audio_playing = 0;  // Playback finished
        }
    }
}

// ============ Test Functions ============

// Generate a simple test sine wave
void generate_sine_wave(int16_t *buffer, uint32_t samples, float frequency) {
    for (uint32_t i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sine = sinf(2.0f * 3.14159265f * frequency * t);
        buffer[i] = (int16_t)(sine * 32767);  // Scale to 16-bit signed
    }
}

// ============ Main Application ============

int main(void) {
    // Initialize HAL
    HAL_Init();
    
    // Configure system clock (default 16MHz on F401)
    // Could be configured for higher speed if needed
    
    // Initialize USART for debugging (optional)
    // debug_uart_init();
    
    // Initialize PWM for audio output
    pwm_audio_init();
    
    // Initialize timer for audio sampling (44.1kHz)
    audio_timer_init();
    
    // Create buffer for 1 second of audio
    int16_t audio_data[44100];
    
    // Generate test sine wave (1kHz)
    generate_sine_wave(audio_data, 44100, 1000.0f);
    
    // Start playback
    audio_play(audio_data, 44100);
    
    // Wait for playback to complete
    while (audio_is_playing()) {
        // Could do other tasks here
        // Or check buttons for player controls
    }
    
    // Main loop
    while (1) {
        // Application code here
        // Audio playback happens in interrupt handler
    }
}
```

---

## Part 4: Pin Configuration for Music Player

### 4.1 Complete Pin Assignment

```
STM32F401RE Nucleo Pin Layout for Walkman Player:

┌─────────────────────────────────────┐
│     Audio Output                    │
├─────────────────────────────────────┤
│ PA0 → PWM Audio Out (→ RC Filter)   │
│ GND → Ground return                 │

┌─────────────────────────────────────┐
│     LCD Display (SPI1)              │
├─────────────────────────────────────┤
│ PA5 → SPI1_CLK     (D13 Arduino)    │
│ PA6 → SPI1_MISO    (D12 Arduino)    │
│ PA7 → SPI1_MOSI    (D11 Arduino)    │
│ PA4 → SPI1_NSS     (D10 Arduino)    │ (or any GPIO)
│ PB6 → GPIO for LCD DC              │
│ PB7 → GPIO for LCD RST             │

┌─────────────────────────────────────┐
│     Control Buttons                 │
├─────────────────────────────────────┤
│ PC13 → User Button (built-in)       │
│ PB3  → Custom Button 1 (Play/Pause) │
│ PB4  → Custom Button 2 (Next)       │
│ PB5  → Custom Button 3 (Previous)   │
│ PB8  → Custom Button 4 (Vol+)       │
│ PB9  → Custom Button 5 (Vol-)       │
│ PA8  → Custom Button 6 (Shuffle)    │
│ PA9  → Custom Button 7 (Loop)       │

┌─────────────────────────────────────┐
│     SD Card (SPI2)                  │
├─────────────────────────────────────┤
│ PB13 → SPI2_CLK                     │
│ PB14 → SPI2_MISO                    │
│ PB15 → SPI2_MOSI                    │
│ PB12 → SPI2_NSS (CS)                │

┌─────────────────────────────────────┐
│     Debug Serial (USART2)           │
├─────────────────────────────────────┤
│ PA2  → USART2_TX   (D1 Arduino)     │
│ PA3  → USART2_RX   (D0 Arduino)     │
│ 115200 baud, 8N1                    │
```

### 4.2 F401RE Arduino Pin Mapping

```
F401RE → Arduino Compatible Pins:

Digital Pins:
D0  → PA3  (RX)
D1  → PA2  (TX)
D2  → PA10
D3  → PB3
D4  → PB5
D5  → PB4
D6  → PB10
D7  → PA8
D8  → PA9
D9  → PC7
D10 → PA4  (SPI CS)
D11 → PA7  (SPI MOSI)
D12 → PA6  (SPI MISO)
D13 → PA5  (SPI CLK)

Analog Pins:
A0  → PA0
A1  → PA1
A2  → PA4
A3  → PA5
A4  → PA6
A5  → PA7
A6  → PB0
A7  → PB1
```

---

## Part 5: Comparing Audio Methods for F401RE

### 5.1 Decision Matrix

```
                    PWM    R-2R   Codec
Cost                $2     $10    $25
Complexity          ★★     ★★★    ★★★★
Audio Quality       ★★★    ★★★★   ★★★★★
CPU Load            ★★★★   ★★     ★★
Setup Time          15min  1hour  2hours
Learning Value      ★★★    ★★★★   ★★
Reliability         ★★★★   ★★★★   ★★★
Components          2      20     5+

RECOMMENDATION FOR F401RE:
Use PWM! It's the fastest to implement
and works well for headphones.
```

### 5.2 Audio Quality Comparison

```
PWM Method:
  SNR: ~48dB (noticeable noise)
  Frequency: 20Hz - 20kHz (full audio range)
  Best for: Headphones, learning
  
R-2R Ladder:
  SNR: ~50dB (slightly better)
  Frequency: 20Hz - 20kHz
  Best for: Speakers, professional projects
  
External Codec:
  SNR: ~90dB (professional)
  Frequency: 20Hz - 48kHz
  Best for: Professional audio
  
For Music Player:
  PWM is acceptable. Users won't notice much quality loss
  compared to codec for compressed MP3 audio.
```

---

## Part 6: LCD Display Connection

### 6.1 SPI1 Configuration for ILI9341 LCD

```c
void lcd_spi_init(void) {
    // Enable clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    // Configure GPIO pins for SPI1
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PA5 (SCK), PA6 (MISO), PA7 (MOSI)
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA4 (NSS/CS) - using GPIO mode for manual control
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure SPI1
    SPI_HandleTypeDef hspi1;
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // 21MHz for LCD
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    HAL_SPI_Init(&hspi1);
}
```

---

## Part 7: Building the Complete Music Player

### 7.1 Application Structure

```c
#include "stm32f4xx_hal.h"

// ============ Module Includes ============

// Audio output (PWM-based)
#include "audio.h"           // PWM audio functions
#include "player.h"          // Music playback logic
#include "buttons.h"         // Button input handling
#include "lcd_display.h"     // LCD display driver
#include "sd_card.h"         // SD card file handling

// ============ Application State ============

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} player_state_t;

typedef struct {
    player_state_t state;
    uint32_t current_volume;
    uint8_t shuffle_enabled;
    uint8_t loop_enabled;
    char current_file[256];
} player_t;

player_t player = {
    .state = PLAYER_STOPPED,
    .current_volume = 100,
    .shuffle_enabled = 0,
    .loop_enabled = 0
};

// ============ Main Application ============

void init_all_systems(void) {
    HAL_Init();
    
    // Initialize audio (PWM)
    pwm_audio_init();
    audio_timer_init();
    
    // Initialize LCD
    lcd_init();
    
    // Initialize buttons
    buttons_init();
    
    // Initialize SD card
    sd_card_init();
    
    // Initialize UART for debug
    debug_uart_init();
}

void player_button_handler(button_press_t button) {
    switch (button) {
        case BUTTON_PLAY:
            if (player.state == PLAYER_PLAYING) {
                audio_stop();
                player.state = PLAYER_PAUSED;
            } else {
                // Resume or start playing
                audio_play(current_buffer, buffer_size);
                player.state = PLAYER_PLAYING;
            }
            break;
            
        case BUTTON_NEXT:
            // Load next track
            load_next_song();
            break;
            
        case BUTTON_PREV:
            // Load previous track
            load_previous_song();
            break;
            
        case BUTTON_VOL_UP:
            player.current_volume += 10;
            if (player.current_volume > 100)
                player.current_volume = 100;
            update_volume();
            break;
            
        case BUTTON_VOL_DOWN:
            if (player.current_volume >= 10)
                player.current_volume -= 10;
            else
                player.current_volume = 0;
            update_volume();
            break;
            
        case BUTTON_SHUFFLE:
            player.shuffle_enabled = !player.shuffle_enabled;
            break;
            
        case BUTTON_LOOP:
            player.loop_enabled = !player.loop_enabled;
            break;
    }
    
    // Update display
    lcd_update_player_state(&player);
}

int main(void) {
    init_all_systems();
    
    // Main loop
    while (1) {
        // Poll buttons
        button_press_t button = buttons_check();
        if (button != BUTTON_NONE) {
            player_button_handler(button);
        }
        
        // Update display
        if (audio_is_playing()) {
            lcd_update_progress(audio_get_time(), audio_get_duration());
        }
        
        // Check if song finished
        if (!audio_is_playing() && player.state == PLAYER_PLAYING) {
            if (player.loop_enabled) {
                // Restart current song
                audio_play(current_buffer, buffer_size);
            } else {
                // Play next song
                load_next_song();
            }
        }
    }
}
```

---

## Part 8: STM32CubeIDE Setup for F401RE

### 8.1 Project Configuration

```
1. File → New → STM32 Project
2. Select Board:
   - Search: "STM32F401RE"
   - Select: "NUCLEO-F401RE"
   - Target Language: C
   - Toolchain: STM32CubeIDE

3. Clock Configuration (in CubeMX):
   - HSE: Disable (F401RE uses HSI 16MHz internal clock)
   - System Clock: 84MHz (maximum for F401RE)
   - APB1: 42MHz
   - APB2: 84MHz

4. Peripherals to Enable:
   ✅ USART2 (Debug serial) @ 115200
   ✅ TIM2 (Audio PWM output)
   ✅ SPI1 (LCD display)
   ✅ GPIO (Buttons)
   Optional:
   ⭕ SPI2 (SD card)
   ⭕ I2C1 (Expansion)

5. Pin Configuration:
   PA0  → TIM2_CH1 (PWM Audio)
   PA2  → USART2_TX
   PA3  → USART2_RX
   PA4  → SPI1_NSS (manual GPIO for LCD CS)
   PA5  → SPI1_CLK
   PA6  → SPI1_MISO
   PA7  → SPI1_MOSI
   PB3  → GPIO_Input (Button)
   PB4  → GPIO_Input (Button)
   PB5  → GPIO_Input (Button)
   PB6  → GPIO_Output (LCD DC)
   PB7  → GPIO_Output (LCD RST)

6. Generate Code
```

---

## Part 9: Key Differences F401RE vs F407

### Comparison Table

| Feature | F401RE | F407 | Impact |
|---------|--------|------|--------|
| **Clock** | 84MHz | 168MHz | F401 slower (half speed) |
| **Flash** | 512KB | 1MB | F401 less storage |
| **RAM** | 96KB | 192KB | F401 smaller buffers |
| **I2S** | ❌ None | ✅ I2S3 | F401 needs GPIO audio |
| **DAC** | ❌ None | ❌ None | Both need external |
| **SPI** | 2 (SPI1, SPI2) | 3 (SPI1-3) | F407 more flexible |
| **Timers** | 7 | 14 | F401 has enough |

### For Walkman Player

```
F401RE Limitations:
❌ No I2S (must use PWM)
❌ Slower CPU (84MHz vs 168MHz)
❌ Less RAM (96KB vs 192KB)

Workarounds:
✅ Use PWM for audio (still works)
✅ PWM works fine at 84MHz
✅ 96KB RAM enough for audio buffers
   (Can store ~48 seconds of 44.1kHz mono)

Result: F401RE CAN run Walkman player,
but F407 is more suitable (better performance).
```

---

## Part 10: Troubleshooting F401RE Audio

### Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| **No audio output** | PA0 not configured as PWM | Check GPIO alternate function |
| **Distorted audio** | Sample rate incorrect | Verify TIM2 period = 952 for 44.1kHz |
| **Timing jitter** | Interrupt priority too low | Set TIM2 interrupt priority to 0 |
| **PWM noise audible** | RC filter cutoff too low | Increase C value or decrease R |
| **Audio too quiet** | PWM duty range wrong | Check conversion formula |
| **Audio too loud** | Samples clipping | Add volume scaling to samples |

---

## Part 11: Performance Considerations

### CPU Usage

```
F401RE @ 84MHz PWM Audio:

Audio Timer Interrupt (44.1kHz):
- Interrupt overhead: ~5µs
- Sample output: ~2µs
- Total per sample: ~7µs
- Total CPU: 7µs × 44.1kHz = ~31% CPU

Remaining CPU (69%):
- LCD updates
- Button input
- SD card reading
- Other tasks

Result: Plenty of CPU available!
(F407 @ 168MHz would use only 15% for audio)
```

### Memory Usage

```
F401RE 96KB RAM breakdown:

Used by system:
- HAL structs: ~10KB
- Stack: ~10KB
- Available: ~76KB

Audio buffer allocation:
- 1 second mono @ 44.1kHz: 88.2KB ❌ (too big!)
- 500ms mono @ 44.1kHz: 44.1KB ✓ (good)
- 250ms mono @ 44.1kHz: 22.05KB ✓ (safe)

Recommendation:
Stream audio from SD card in chunks
Instead of loading entire song to RAM
```

---

## Part 12: Complete Minimal Example

### Single-File PWM Audio Player

```c
// ============ MINIMAL PWM AUDIO FOR STM32F401RE ============

#include "stm32f4xx_hal.h"
#include <math.h>

TIM_HandleTypeDef htim2;
volatile int playing = 0;
volatile int idx = 0;
int16_t *buf;
int bsize;

// Generate sine wave
void gen_sine(int16_t *b, int s, float f) {
    for (int i = 0; i < s; i++) {
        float t = (float)i / 44100;
        b[i] = (int16_t)(sin(2 * 3.14159 * f * t) * 32767);
    }
}

// Output sample
void out_sample(int16_t sample) {
    int pwm = ((int32_t)sample + 32768) * 21 / 65536;
    if (pwm > 20) pwm = 20;
    if (pwm < 0) pwm = 0;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm);
}

// Timer interrupt
void TIM2_IRQHandler(void) {
    if (playing && idx < bsize) {
        out_sample(buf[idx++]);
    } else {
        playing = 0;
    }
    TIM2->SR = 0;
}

// Init
void init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    GPIO_InitTypeDef g = {
        .Pin = GPIO_PIN_0,
        .Mode = GPIO_MODE_AF_PP,
        .Alternate = GPIO_AF1_TIM2,
        .Speed = GPIO_SPEED_FREQ_HIGH
    };
    HAL_GPIO_Init(GPIOA, &g);
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.Period = 952 - 1;  // 44.1kHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&htim2);
    
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_TIM_Base_Start_IT(&htim2);
}

// Play
void play(int16_t *b, int s) {
    buf = b;
    bsize = s;
    idx = 0;
    playing = 1;
}

int main(void) {
    HAL_Init();
    init();
    
    int16_t wave[44100];
    gen_sine(wave, 44100, 1000);  // 1kHz
    
    play(wave, 44100);
    while (playing);
    
    while (1);
}
```

---

## Part 13: Next Steps

### Immediate (Get Audio Working)
1. ✅ Implement PWM audio (Part 3.3)
2. ✅ Connect RC filter to PA0
3. ✅ Test with sine wave
4. ✅ Listen with headphones

### Short-term (Full Music Player)
5. Connect LCD via SPI1
6. Implement button input
7. Add MP3 decoder (libmad or helix)
8. Integrate SD card support
9. Build UI on LCD display

### Long-term (Improvements)
10. Optimize audio quality
11. Add shuffle/loop features
12. Implement volume control
13. Add battery support
14. Consider firmware upgrade to F407 (better specs)

### If Audio Quality Issues
- Use R-2R Ladder method (more complex, better quality)
- Add external amplifier for speaker output
- Upgrade to F407 with I2S and external codec
- Use Discovery board instead (has codec built-in)

---

## Conclusion

**STM32F401RE CAN run a Walkman music player!**

### Best Approach for F401RE:
```
✅ PWM-based audio output on PA0
✅ Simple RC filter ($1-2)
✅ Stream MP3 from SD card
✅ ILI9341 LCD display via SPI1
✅ 7 buttons for control
✅ Acceptable audio quality for portable player

Setup Time: ~2-3 days
Difficulty: Intermediate
Cost: ~$30-50 total
```

### When to Use F401RE:
- ✅ Learning/hobby project
- ✅ Budget constrained
- ✅ Portability important
- ✅ Headphone-only output acceptable

### When to Upgrade to F407:
- ❌ Professional audio quality needed
- ❌ Speaker output with low noise critical
- ❌ Stereo audio required
- ❌ Performance-sensitive application

---

**Happy coding! Your F401RE can become a music player! 🎵**
