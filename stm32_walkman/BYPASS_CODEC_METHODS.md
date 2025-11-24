# Bypassing Audio Codec on STM32 Nucleo - Technical Solutions

Advanced methods to generate audio directly from STM32 without external codec module.

---

## Part 1: Understanding the Problem

### Why Codec is Needed (Standard Approach)
```
Digital Audio (MP3/WAV file)
    ↓ (I2S output from STM32)
    ↓ (Digital audio stream: 16-bit samples @ 44.1kHz)
Audio Codec (WM8994, CS43L22, etc.)
    ↓ (DAC conversion)
    ↓ (Analog signal amplification)
Speakers/Headphones
    ↓ (Audible sound)
```

### The Challenge with Nucleo
```
Nucleo board has:
✅ I2S peripheral (audio interface)
❌ No DAC (Digital-to-Analog Converter)
❌ No audio amplifier
❌ No audio codec

Result: Can't directly output analog audio
```

---

## Part 2: Solution 1 - PWM-Based Audio (Simplest) ⭐⭐⭐

### How It Works

Use **PWM (Pulse Width Modulation)** to generate analog audio:

```
Digital Audio (16-bit sample)
    ↓ (Convert to PWM duty cycle)
PWM Output (High-frequency)
    ↓ (Pin: PA0, PA1, etc.)
Low-Pass Filter (RC circuit)
    ↓ (Removes high frequency PWM)
Analog Audio Signal
    ↓ (~0-3.3V range)
Amplifier (Optional - TDA7052 or similar)
    ↓ (Boost to speaker level ~1-2W)
Speakers (8Ω, 0.5W minimum)
    ↓ (Audible sound)
```

### Hardware Required

**Very Minimal!**
```
STM32 Nucleo Board
    ↓
PA0 (PWM output)
    ↓
┌─────────────────────┐
│ Low-Pass Filter     │
│ R = 10kΩ            │
│ C = 100nF           │
│ fc = 159kHz         │
└─────────────────────┘
    ↓
Audio Signal (filtered)
    ↓
Optional: Amplifier (TDA7052, LM386)
    ↓
8Ω Speaker (or 3.5mm headphones)
```

### RC Filter Calculation

```
Cutoff Frequency: fc = 1 / (2πRC)

For 44.1kHz audio:
- R = 10kΩ (resistor)
- C = 100nF (capacitor)
- fc = 1 / (2π × 10k × 100n) = 159.15 kHz

Result: Passes audio (0-20kHz), blocks PWM (~1MHz)
```

### Implementation Code (Nucleo)

```c
// Configure Timer2 for PWM on PA0
void pwm_audio_init(void) {
    // Enable clock
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure GPIO PA0 for PWM output
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure Timer2: PWM frequency ~1MHz
    TIM_HandleTypeDef htim2;
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;           // No pre-scaling
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 84 - 1;         // 168MHz / 84 = 2MHz
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);
    
    // Configure PWM channel (TIM2_CH1 on PA0)
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 42;               // 50% duty (neutral point)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
    
    // Start PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

// Output audio sample (called at 44.1kHz)
void pwm_audio_output(int16_t sample) {
    // Convert 16-bit signed (-32768 to 32767) to PWM duty (0 to 84)
    uint32_t pwm_duty = ((uint32_t)sample + 32768) * 84 / 65536;
    
    // Clamp to valid range
    if (pwm_duty > 84) pwm_duty = 84;
    if (pwm_duty < 0) pwm_duty = 0;
    
    // Set PWM duty cycle
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_duty);
}

// In main audio loop
void audio_playback_loop(void) {
    while (music_playing) {
        // Read 16-bit sample from MP3 decoder
        int16_t sample = read_audio_sample();
        
        // Output via PWM
        pwm_audio_output(sample);
        
        // Wait for next sample (1/44100 = 22.68 µs)
        delay_microseconds(22.68);
    }
}
```

### Advantages ✅
- **Minimal hardware** (just 2 components: R + C)
- **No external codec needed**
- **Works on any STM32 Nucleo**
- **Cost: <$1 for RC filter**
- **Simple software**
- **Decent audio quality (with proper filtering)**

### Disadvantages ❌
- **Limited audio quality** (PWM inherent noise)
- **Requires external amplifier** for speakers
- **Single-channel (mono)** solution
- **High CPU usage** (must output at 44.1kHz sample rate)
- **Timing-critical** (timing jitter = audio artifacts)

### Audio Quality
```
SNR (Signal-to-Noise Ratio):
- Standard codec: ~90dB
- PWM-based (8-bit): ~48dB
- PWM-based (12-bit): ~72dB
- Headphones: Acceptable
- Speakers: OK (not hi-fi)
```

---

## Part 3: Solution 2 - Sigma-Delta Modulation (Better)

### How It Works

Uses **Sigma-Delta (Δ-Σ) modulation** - an advanced PWM technique:

```
Digital Audio (16-bit)
    ↓ (Sigma-Delta conversion)
Bitstream (1-bit @ high frequency)
    ↓ (Much less noise than PWM)
Output Pin
    ↓
Low-Pass Filter
    ↓
Clean Analog Audio
```

### Why Better Than PWM?
- **Higher effective resolution** (12-16 bits effective)
- **Better noise shaping** (pushes noise to ultrasonic frequencies)
- **Cleaner audio** (SNR ~72-80dB)
- **Same hardware** (just RC filter)

### Implementation Code

```c
// Simple 1-bit Sigma-Delta converter
typedef struct {
    int32_t accumulator;
    int32_t last_error;
} sigma_delta_t;

sigma_delta_t sd = {0, 0};

// Sigma-Delta modulation
uint32_t sigma_delta_output(int16_t sample) {
    // 1st order delta-sigma
    int32_t error = sample - sd.last_error;
    sd.accumulator += error;
    
    // Output 1 or 0 based on accumulator sign
    uint32_t output = (sd.accumulator > 0) ? 1 : 0;
    
    // Update state
    if (output) {
        sd.accumulator -= 32768;
        sd.last_error = sample;
    }
    
    return output;
}

// Multi-bit Sigma-Delta (better quality)
uint32_t multi_bit_sigma_delta(int16_t sample) {
    // 2-bit sigma-delta output (0-3)
    // Can output to PWM with 2-bit resolution
    
    static int32_t integrator = 0;
    int32_t error = sample - integrator;
    
    // Threshold detection
    uint32_t output = 0;
    if (error > 8192)       output = 3;
    else if (error > 0)     output = 2;
    else if (error > -8192) output = 1;
    else                    output = 0;
    
    // Update integrator
    integrator += output * 32768 / 3;
    
    return output;
}

// Output using GPIO bit-banging (1-bit version)
void sigma_delta_output_gpio(uint32_t bit) {
    // Toggle PA0 for each bit
    // High frequency (~1-10MHz) when outputting bitstream
    if (bit) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
}
```

### Audio Quality Improvement
```
1-bit Sigma-Delta:
SNR: ~70dB
THD: ~0.5%
Good for headphones, acceptable for speakers

Multi-bit (2-3 bit):
SNR: ~85dB
THD: ~0.1%
Very good for all applications
```

### Advantages ✅
- **Much better audio quality** than PWM
- **Same minimal hardware**
- **No external codec needed**
- **Lower cost than external codec**
- **Professional-grade audio possible**

### Disadvantages ❌
- **More complex software**
- **High-frequency bitstream** (less efficient amplifier)
- **CPU-intensive** for high-order SD conversion
- **Timing critical**

---

## Part 4: Solution 3 - True DAC Using R-2R Ladder (Best DIY)

### How It Works

Use **R-2R resistor ladder** to create a simple multibit DAC:

```
STM32 GPIO Pins (8 bits)
    ↓ (Digital data: 0-255)
R-2R Ladder Network
    ↓ (Analog conversion)
Analog Audio Signal (0-3.3V)
    ↓
Op-Amp Buffer (optional)
    ↓
Low-Pass Filter (CLC network)
    ↓
Clean Audio Output
```

### Hardware Schematic

```
GPIO Outputs (PA0-PA7):
PA0 --[1kΩ]--+
PA1 --[1kΩ]--+
PA2 --[1kΩ]--+-- Analog Output
PA3 --[1kΩ]--+
PA4 --[1kΩ]--+
PA5 --[1kΩ]--+
PA6 --[1kΩ]--+
PA7 --[1kΩ]--+
         |
        [2kΩ]
         |
        GND

R-2R values:
R = 1kΩ (resistor network, ±1% tolerance!)
2R = 2kΩ (parallel combination)

Output: 0 to 3.3V (8-bit DAC)
Resolution: 3.3V / 256 = 12.9mV per step
```

### Implementation Code

```c
// Configure GPIO for 8-bit parallel output
void r2r_dac_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = 0xFF;  // PA0-PA7
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Output 8-bit audio sample
void r2r_dac_output(uint8_t sample) {
    // Set GPIOA bits directly
    GPIOA->ODR = (GPIOA->ODR & 0xFF00) | sample;
    
    // Alternative using HAL:
    // HAL_GPIO_WritePin(GPIOA, 0xFF, (GPIO_PinState)sample);
}

// Convert 16-bit signed to 8-bit unsigned
uint8_t convert_audio_sample(int16_t sample_16bit) {
    // Scale from -32768 to 32767 to 0 to 255
    uint16_t unsigned_sample = (uint16_t)((uint32_t)sample_16bit + 32768) >> 8;
    return (uint8_t)(unsigned_sample & 0xFF);
}

// Main audio playback loop
void audio_playback_with_dac(void) {
    while (music_playing) {
        // Read 16-bit sample from MP3 decoder
        int16_t sample = read_audio_sample();
        
        // Convert to 8-bit
        uint8_t dac_sample = convert_audio_sample(sample);
        
        // Output to R-2R DAC
        r2r_dac_output(dac_sample);
        
        // Wait for next sample (1/44100 ≈ 22.68 µs)
        delay_microseconds(22.68);
    }
}
```

### R-2R Network Calculation

```
For 8-bit DAC with 3.3V full-scale:

Reference Voltage: VREF = 3.3V
Resolution: LSB = 3.3V / 256 = 12.9mV
Output Impedance: ~500Ω (needs buffering)
Bandwidth: 0-10kHz (limited by settling time)

Resistor Tolerances:
R = 1kΩ ±1% (1% resistors essential!)
2R = 2×1kΩ in parallel

Settling Time:
For 8-bit accuracy: ~100ns
At 44.1kHz: 22.68µs per sample (plenty!)
```

### Advantages ✅
- **True analog output** (not PWM)
- **8-bit resolution** (reasonable quality)
- **Very simple hardware** (just resistors)
- **No capacitor delays** like filters
- **Good for speakers**
- **Cost: ~$2-5 for resistors**

### Disadvantages ❌
- **8-bit resolution only** (vs 16-bit)
- **Requires precise 1% resistors**
- **Output impedance high** (needs op-amp buffer)
- **7 GPIO pins used** (leaves fewer pins free)
- **Not suitable for headphones** (output impedance)

---

## Part 5: Solution 4 - Using STM32 Internal DAC (If Available)

### Which STM32 Nucleo Have DAC?

```
✅ STM32F103 Nucleo - 2x 12-bit DAC
✅ STM32F303 Nucleo - 2x 12-bit DAC
✅ STM32F4 Series - Some models have DAC
✅ STM32L Series - Some models have DAC
✅ STM32H7 Series - Some models have DAC

❌ STM32F407/F429 Nucleo - NO DAC! ❌
```

### Hardware (If DAC Available)

```
STM32 Internal DAC
    ↓ (PA4 or PA5 for DAC outputs)
Low-Pass Filter (CLC or RC)
    ↓
Op-Amp Buffer
    ↓
Audio Amplifier (TDA7052, LM386)
    ↓
Speakers

Advantages:
✅ True 12-bit DAC (4096 levels)
✅ Dedicated hardware (no CPU overhead for conversion)
✅ Better audio quality
✅ No resistor matching needed

Disadvantages:
❌ Not available on all Nucleo boards
❌ Still need filter + amplifier
❌ Slower settling time (~10µs)
```

### Implementation Code (If DAC Available)

```c
// Initialize STM32 internal DAC
void internal_dac_init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;  // DAC1_OUT on PA4
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    DAC_HandleTypeDef hdac;
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

// Output 12-bit sample
void internal_dac_output(uint16_t sample_12bit) {
    // Convert 16-bit to 12-bit
    uint16_t dac_value = (sample_12bit >> 4) & 0xFFF;
    
    // Output to DAC
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}

// Audio playback with internal DAC
void audio_playback_with_internal_dac(void) {
    while (music_playing) {
        int16_t sample = read_audio_sample();
        
        // Convert to 12-bit unsigned
        uint16_t dac_sample = (uint16_t)((int32_t)sample + 32768) >> 4;
        
        internal_dac_output(dac_sample);
        
        delay_microseconds(22.68);
    }
}
```

---

## Part 6: Comparison Table - All Solutions

| Method | Hardware Cost | Audio Quality | Complexity | CPU Load | Best For |
|--------|---------------|---------------|-----------|----------|----------|
| **PWM Filter** | $1 | Medium (48dB) | Simple | High | Headphones |
| **Sigma-Delta** | $1 | Good (72dB) | Medium | Very High | Headphones |
| **R-2R Ladder** | $5 | Good (48dB) | Simple | Medium | Speakers |
| **Internal DAC** | $0 | Excellent (70dB) | Simple | Low | Best (if available) |
| **External Codec** | $25 | Excellent (90dB) | Medium | Low | Professional |

---

## Part 7: Recommended Solution for Nucleo

### For STM32F407 Nucleo (No DAC)

**Best Option: PWM with RC Filter + Amplifier**

```
Hardware:
- R = 10kΩ
- C = 100nF
- Optional: TDA7052 amplifier ($2-3)
- Cost: ~$3 total

Advantages:
✅ Simplest
✅ Cheapest
✅ Works well with headphones
✅ Adequate for speakers (with amp)
✅ Proven design

Schematic:
PA0 (PWM) --[10kΩ]--+-- Audio Out
                     |
                   [100nF]
                     |
                    GND
```

### Alternative: R-2R Ladder (If You Want Better Quality)

```
Hardware:
- 8x 1kΩ resistors (1% tolerance!)
- Op-amp buffer (TL072 or similar)
- Cost: ~$5-10

Advantages:
✅ True analog output
✅ Better SNR (~50dB)
✅ Less CPU-dependent
✅ No timing-critical code
```

### For STM32F303 Nucleo (Has DAC)

**Best Option: Internal DAC + LC Filter**

```
Hardware:
- Internal DAC (free!)
- L (inductor) = 10µH
- C (capacitor) = 100nF
- Op-amp buffer
- Cost: ~$2

Advantages:
✅ 12-bit native DAC
✅ Great audio quality
✅ No external codec
✅ Simple implementation
```

---

## Part 8: Practical Implementation Example

### Complete PWM-Based Solution

```c
// ============= PWM Audio System for Nucleo =============

#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim2;
uint32_t sample_buffer[1024];
volatile uint32_t buffer_index = 0;

// Initialize PWM-based audio output
void audio_init(void) {
    // Enable clocks
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA0 for PWM (TIM2_CH1)
    GPIO_InitTypeDef GPIO_Init = {
        .Pin = GPIO_PIN_0,
        .Mode = GPIO_MODE_AF_PP,
        .Alternate = GPIO_AF1_TIM2,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Pull = GPIO_NOPULL
    };
    HAL_GPIO_Init(GPIOA, &GPIO_Init);
    
    // Configure Timer2 for 2MHz PWM
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 83;  // 168MHz / 84 = 2MHz
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim2);
    
    // Configure PWM channel
    TIM_OC_InitTypeDef PWM_Init = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 42,  // 50% duty cycle (neutral)
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE
    };
    HAL_TIM_PWM_ConfigChannel(&htim2, &PWM_Init, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    
    // Setup timer interrupt for audio sampling
    HAL_TIM_Base_Start_IT(&htim2);
}

// Timer interrupt - called at each PWM period
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}

// Called by HAL timer callback
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // Read next audio sample
        if (buffer_index < 1024) {
            int16_t sample = sample_buffer[buffer_index++];
            
            // Convert 16-bit to PWM duty cycle (0-83)
            uint32_t pwm_duty = ((uint32_t)sample + 32768) * 84 / 65536;
            pwm_duty = (pwm_duty > 83) ? 83 : pwm_duty;
            
            // Update PWM
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_duty);
        }
    }
}

// Fill buffer with audio samples (from MP3 decoder)
void fill_audio_buffer(int16_t *audio_samples, uint32_t num_samples) {
    buffer_index = 0;
    for (int i = 0; i < 1024 && i < num_samples; i++) {
        sample_buffer[i] = audio_samples[i];
    }
}

// ============= Usage in main =============
int main(void) {
    HAL_Init();
    audio_init();
    
    // Load audio sample (from file decoder)
    int16_t my_samples[1024];
    // ... decode MP3 and fill my_samples ...
    
    // Play audio
    fill_audio_buffer(my_samples, 1024);
    
    // Wait for playback
    while (buffer_index < 1024) {
        // Playback in progress
    }
    
    while (1);
}
```

### RC Low-Pass Filter Design

```
Component Values for 44.1kHz Audio:

Simple RC Filter:
R = 10kΩ
C = 100nF
fc = 1/(2π×10k×100n) = 159.15kHz

Bode Plot:
20dB/decade above fc
At 1MHz (PWM): -18dB attenuation
At 20kHz (audio): -0.5dB attenuation

PCB Layout:
┌─────PA0─────[10kΩ]─────┬─── Audio Out
                          │
                        [100nF]
                          │
                         GND

Add 10µF capacitor to output for DC blocking!
```

---

## Part 9: Pros & Cons Summary

### ✅ Advantages of Codec-Bypass Methods

```
1. NO external codec module needed
2. Lower cost ($1-10 vs $20-30)
3. More control over audio processing
4. Learning opportunity
5. Works on any STM32 Nucleo
6. Simple hardware modifications
```

### ❌ Disadvantages

```
1. Lower audio quality than codec (generally)
2. Higher CPU usage
3. More timing-critical code
4. Limited to mono (not stereo easily)
5. Requires filter design knowledge
6. Not suitable for professional audio
7. Timing jitter causes artifacts
```

---

## Part 10: When to Use Each Method

### Use PWM Filter When:
- You want simplest hardware
- Headphones only (low impedance load)
- Budget is critical
- Learning/experimental

### Use R-2R Ladder When:
- You want true analog output
- Driving speakers directly
- Don't want high-frequency noise
- Have 8 GPIO pins available

### Use Internal DAC When:
- Your STM32 has built-in DAC
- You want best quality without codec
- CPU usage matters
- You have space for LC filter

### Use External Codec When:
- You want professional audio
- Stereo is required
- You need built-in amplifier
- Cost is not critical
- Audio quality is priority

---

## Conclusion

**Yes, you can bypass the codec on Nucleo board!**

### Best Options:

1. **Easiest**: PWM + RC Filter (~$3, decent quality)
2. **Best Quality**: Internal DAC (if available)
3. **Most Control**: R-2R Ladder (~$5)
4. **Best Overall**: External codec module (~$25, professional quality)

**For your Walkman project**: PWM-based solution is practical and proven. Combined with RC filter + optional amplifier, provides acceptable audio quality for portable music player.

**Alternative**: If you want professional audio quality without complex circuit design, the **STM32F407G-DISC1 Discovery board** (with built-in CS43L22) remains the best single-board solution!
