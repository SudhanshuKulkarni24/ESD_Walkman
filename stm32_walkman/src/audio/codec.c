/**
 * WM8994 Audio Codec Implementation for STM32F407 Discovery
 * 
 * Communicates via:
 * - I2C1 (codec control): PB6 (SCL), PB7 (SDA)
 * - I2S3 (audio data): PC7, PC10, PC12, PA4
 * - GPIO: PD4 (codec power control)
 */

#include "codec.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* WM8994 Register Map */
#define WM8994_CHIP_ID                      0x0000
#define WM8994_POWER_MANAGEMENT_1           0x0001
#define WM8994_POWER_MANAGEMENT_2           0x0002
#define WM8994_POWER_MANAGEMENT_3           0x0003
#define WM8994_LEFT_LINE_INPUT_VOLUME       0x0004
#define WM8994_RIGHT_LINE_INPUT_VOLUME      0x0005
#define WM8994_LEFT_OUTPUT_VOLUME           0x001C
#define WM8994_RIGHT_OUTPUT_VOLUME          0x001D
#define WM8994_OUTPUT_MIXER_1               0x002D
#define WM8994_OUTPUT_MIXER_2               0x002E
#define WM8994_CLOCKING_1                   0x0100
#define WM8994_CLOCKING_2                   0x0110
#define WM8994_AUDIO_INTERFACE_1            0x0300
#define WM8994_AUDIO_INTERFACE_2            0x0301
#define WM8994_AUDIO_INTERFACE_3            0x0302
#define WM8994_AUDIO_INTERFACE_4            0x0303
#define WM8994_AIF1_CONTROL_1               0x0300
#define WM8994_AIF1_CONTROL_2               0x0301

#define WM8994_ADDR (0x1A << 1)  // I2C address (8-bit)
#define WM8994_TIMEOUT 1000

/* Global state */
static struct {
    I2C_HandleTypeDef hi2c1;
    I2S_HandleTypeDef hi2s3;
    uint8_t is_initialized;
    uint8_t is_playing;
    uint8_t volume;
    const int16_t *current_buffer;
    uint32_t buffer_size;
    uint32_t buffer_position;
} codec_state = {
    .is_initialized = 0,
    .is_playing = 0,
    .volume = 70,
    .current_buffer = NULL,
    .buffer_size = 0,
    .buffer_position = 0
};

/* ============ Low-level I2C Communication ============ */

/**
 * Read register from WM8994 via I2C
 */
codec_status_t codec_read_register(uint16_t addr, uint16_t *value) {
    uint8_t data[2] = {0, 0};
    uint8_t reg_addr = (addr >> 8) & 0xFF;
    
    if (HAL_I2C_Mem_Read(&codec_state.hi2c1, WM8994_ADDR, reg_addr, 
                         I2C_MEMADD_SIZE_8BIT, data, 2, WM8994_TIMEOUT) != HAL_OK) {
        return CODEC_TIMEOUT;
    }
    
    *value = ((uint16_t)data[0] << 8) | data[1];
    return CODEC_OK;
}

/**
 * Write register to WM8994 via I2C
 */
codec_status_t codec_write_register(uint16_t addr, uint16_t value) {
    uint8_t data[3];
    uint8_t reg_addr = (addr >> 8) & 0xFF;
    
    data[0] = (addr >> 8) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
    
    if (HAL_I2C_Master_Transmit(&codec_state.hi2c1, WM8994_ADDR, data, 3, WM8994_TIMEOUT) != HAL_OK) {
        return CODEC_TIMEOUT;
    }
    
    return CODEC_OK;
}

/* ============ GPIO and Clock Setup ============ */

/**
 * Initialize GPIO for codec control
 */
static void codec_gpio_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    /* PD4 as codec power enable (active high) */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    /* Enable codec power */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
 * Initialize I2C1 for codec control
 */
static void codec_i2c_init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Configure PB6 and PB7 as I2C1 */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* I2C configuration */
    codec_state.hi2c1.Instance = I2C1;
    codec_state.hi2c1.Init.ClockSpeed = 400000;
    codec_state.hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    codec_state.hi2c1.Init.OwnAddress1 = 0;
    codec_state.hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    codec_state.hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    codec_state.hi2c1.Init.OwnAddress2 = 0;
    codec_state.hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    codec_state.hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&codec_state.hi2c1);
}

/**
 * Initialize I2S3 for audio streaming
 */
static void codec_i2s_init(void) {
    __HAL_RCC_I2S_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Configure PC7 (I2S3_MCLK) */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* Configure PC10 (I2S3_CK), PC12 (I2S3_SD) */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* Configure PA4 (I2S3_WS) */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* I2S3 configuration */
    codec_state.hi2s3.Instance = SPI3;
    codec_state.hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    codec_state.hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    codec_state.hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    codec_state.hi2s3.Init.MCLKOutput = I2S_MCLK_ENABLE;
    codec_state.hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;  // 44.1kHz
    codec_state.hi2s3.Init.CPOL = I2S_CPOL_LOW;
    codec_state.hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
    
    HAL_I2S_Init(&codec_state.hi2s3);
}

/**
 * Configure WM8994 registers for audio output
 */
static codec_status_t codec_configure_chip(void) {
    uint16_t reg_value;
    
    /* Check chip ID */
    if (codec_read_register(WM8994_CHIP_ID, &reg_value) != CODEC_OK) {
        return CODEC_ERROR;
    }
    
    if ((reg_value & 0xFF00) != 0x8900) {
        return CODEC_ERROR;  // Not WM8994
    }
    
    /* Reset codec */
    codec_write_register(0x0000, 0x0000);
    HAL_Delay(10);
    
    /* Power management: enable core, output mixer, DAC */
    codec_write_register(WM8994_POWER_MANAGEMENT_1, 0x1003);  // VMID, BIAS
    codec_write_register(WM8994_POWER_MANAGEMENT_2, 0x0000);
    codec_write_register(WM8994_POWER_MANAGEMENT_3, 0x0000);
    
    HAL_Delay(100);
    
    /* Configure audio interface for I2S */
    codec_write_register(WM8994_AUDIO_INTERFACE_1, 0x0000);   // I2S, 16-bit
    codec_write_register(WM8994_AUDIO_INTERFACE_2, 0x4000);   // 44.1kHz
    
    /* Set volume */
    codec_set_volume(codec_state.volume);
    
    /* Output configuration */
    codec_write_register(WM8994_OUTPUT_MIXER_1, 0x0001);      // DAC to output mixer
    codec_write_register(WM8994_OUTPUT_MIXER_2, 0x0001);
    
    return CODEC_OK;
}

/* ============ Public API ============ */

/**
 * Initialize codec
 */
codec_status_t codec_init(void) {
    if (codec_state.is_initialized) {
        return CODEC_OK;
    }
    
    codec_gpio_init();
    codec_i2c_init();
    codec_i2s_init();
    
    if (codec_configure_chip() != CODEC_OK) {
        return CODEC_ERROR;
    }
    
    codec_state.is_initialized = 1;
    return CODEC_OK;
}

/**
 * Deinitialize codec
 */
codec_status_t codec_deinit(void) {
    codec_stop();
    HAL_I2S_DeInit(&codec_state.hi2s3);
    HAL_I2C_DeInit(&codec_state.hi2c1);
    codec_state.is_initialized = 0;
    return CODEC_OK;
}

/**
 * Start playback
 */
codec_status_t codec_play(const int16_t *buffer, uint32_t size) {
    if (!codec_state.is_initialized) {
        return CODEC_ERROR;
    }
    
    codec_state.current_buffer = buffer;
    codec_state.buffer_size = size;
    codec_state.buffer_position = 0;
    codec_state.is_playing = 1;
    
    /* Start I2S DMA transfer */
    if (HAL_I2S_Transmit_DMA(&codec_state.hi2s3, (uint16_t *)buffer, size) != HAL_OK) {
        return CODEC_ERROR;
    }
    
    return CODEC_OK;
}

/**
 * Stop playback
 */
codec_status_t codec_stop(void) {
    codec_state.is_playing = 0;
    HAL_I2S_DMAPause(&codec_state.hi2s3);
    HAL_I2S_DMAStop(&codec_state.hi2s3);
    return CODEC_OK;
}

/**
 * Pause playback
 */
codec_status_t codec_pause(void) {
    codec_state.is_playing = 0;
    HAL_I2S_DMAPause(&codec_state.hi2s3);
    return CODEC_OK;
}

/**
 * Resume playback
 */
codec_status_t codec_resume(void) {
    if (!codec_state.current_buffer) {
        return CODEC_ERROR;
    }
    
    codec_state.is_playing = 1;
    HAL_I2S_DMAResume(&codec_state.hi2s3);
    return CODEC_OK;
}

/**
 * Set playback sample rate
 */
codec_status_t codec_set_sample_rate(codec_sample_rate_t rate) {
    uint16_t config = 0;
    
    switch (rate) {
        case CODEC_SAMPLE_RATE_44100:
            config = 0x4000;  // AIF1 44.1kHz
            break;
        case CODEC_SAMPLE_RATE_48000:
            config = 0x4000;  // AIF1 48kHz
            break;
        case CODEC_SAMPLE_RATE_96000:
            config = 0x8000;  // AIF1 96kHz
            break;
        default:
            return CODEC_ERROR;
    }
    
    return codec_write_register(WM8994_AUDIO_INTERFACE_2, config);
}

/**
 * Set volume (0-100%)
 */
codec_status_t codec_set_volume(uint8_t volume) {
    uint16_t dac_vol;
    
    if (volume > 100) {
        volume = 100;
    }
    
    codec_state.volume = volume;
    
    /* Map 0-100 to DAC range (0-127 for max volume) */
    dac_vol = (volume * 127) / 100;
    
    /* Set left and right output volumes */
    codec_write_register(WM8994_LEFT_OUTPUT_VOLUME, 0xC0 | dac_vol);   // Unmute + volume
    codec_write_register(WM8994_RIGHT_OUTPUT_VOLUME, 0xC0 | dac_vol);  // Unmute + volume
    
    return CODEC_OK;
}

/**
 * Get current volume
 */
uint8_t codec_get_volume(void) {
    return codec_state.volume;
}

/**
 * Check if playing
 */
uint8_t codec_is_playing(void) {
    return codec_state.is_playing;
}

/**
 * Get playback position
 */
uint32_t codec_get_position(void) {
    return codec_state.buffer_position;
}

/**
 * Set input source (not implemented - for future use)
 */
codec_status_t codec_set_input_source(codec_input_source_t source) {
    (void)source;  // Unused for now
    return CODEC_OK;
}

/**
 * Set output destination (not implemented - for future use)
 */
codec_status_t codec_set_output_destination(codec_output_dest_t dest) {
    (void)dest;  // Unused for now
    return CODEC_OK;
}

/**
 * Set microphone gain (not implemented - for future use)
 */
codec_status_t codec_set_mic_gain(uint8_t gain) {
    (void)gain;  // Unused for now
    return CODEC_OK;
}

/**
 * I2S interrupt handler (called from HAL)
 */
void codec_i2s_interrupt_handler(void) {
    /* Update playback position */
    if (codec_state.is_playing) {
        codec_state.buffer_position++;
    }
}
