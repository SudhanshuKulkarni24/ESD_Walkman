/**
 * WM8994 Audio Codec Implementation for STM32F407 Discovery
 * BARE METAL - No HAL, direct register access via bare metal drivers
 * 
 * Communicates via:
 * - I2C1 (codec control): PB6 (SCL), PB7 (SDA)
 * - I2S3 (audio data): PC7, PC10, PC12, PA4
 * - GPIO: PD4 (codec power control)
 */

#include "codec.h"
#include "i2c.h"
#include "i2s.h"
#include "gpio.h"
#include "system.h"
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
 * Read register from WM8994 via I2C (bare metal)
 */
codec_status_t codec_read_register(uint16_t addr, uint16_t *value) {
    uint8_t reg_addr = addr & 0xFF;
    uint8_t data[2] = {0, 0};
    
    if (i2c_write_read(I2C_BUS_1, 0x1A, reg_addr, data, 2) != 0) {
        return CODEC_TIMEOUT;
    }
    
    *value = ((uint16_t)data[0] << 8) | data[1];
    return CODEC_OK;
}

/**
 * Write register to WM8994 via I2C (bare metal)
 */
codec_status_t codec_write_register(uint16_t addr, uint16_t value) {
    uint8_t data[3];
    
    data[0] = addr & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
    
    if (i2c_write(I2C_BUS_1, 0x1A, data, 3) != 0) {
        return CODEC_TIMEOUT;
    }
    
    return CODEC_OK;
}

/**
 * Initialize GPIO for codec control (bare metal)
 */
static void codec_gpio_init(void) {
    /* Initialize GPIOD for codec power control */
    gpio_init_port(GPIO_PORT_D);
    
    /* PD4 as codec power enable (active high) */
    gpio_config(GPIO_PORT_D, 4, GPIO_MODE_OUTPUT, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    
    /* Enable codec power */
    gpio_set(GPIO_PORT_D, 4);
}

/**
 * Initialize I2C1 for codec control (bare metal)
 */
static void codec_i2c_init(void) {
    /* Initialize I2C1 with 400kHz clock */
    i2c_init(I2C_BUS_1, 400000);
}

/**
 * Initialize I2S3 for audio streaming (bare metal)
 */
static void codec_i2s_init(void) {
    /* I2S3 (SPI3) is initialized via bare metal i2s driver */
    i2s_init(I2S_SR_44100);
}

/**
 * Configure WM8994 registers for audio output (bare metal)
 */
static codec_status_t codec_configure_chip(void) {
    uint16_t reg_value;
    
    /* Check chip ID */
    if (codec_read_register(WM8994_CHIP_ID, &reg_value) != CODEC_OK) {
        return CODEC_ERROR;
    }
    
    if ((reg_value & 0xFF00) != 0x8900) {
        return CODEC_ERROR;  /* Not WM8994 */
    }
    
    /* Reset codec */
    codec_write_register(0x0000, 0x0000);
    system_delay_ms(10);
    
    /* Power management: enable core, output mixer, DAC */
    codec_write_register(WM8994_POWER_MANAGEMENT_1, 0x1003);  /* VMID, BIAS */
    codec_write_register(WM8994_POWER_MANAGEMENT_2, 0x0000);
    codec_write_register(WM8994_POWER_MANAGEMENT_3, 0x0000);
    
    system_delay_ms(100);
    
    /* Configure audio interface for I2S */
    codec_write_register(WM8994_AUDIO_INTERFACE_1, 0x0000);   /* I2S, 16-bit */
    codec_write_register(WM8994_AUDIO_INTERFACE_2, 0x4000);   /* 44.1kHz */
    
    /* Set volume */
    codec_set_volume(codec_state.volume);
    
    /* Output configuration */
    codec_write_register(WM8994_OUTPUT_MIXER_1, 0x0001);      /* DAC to output mixer */
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
