/**
 * Bare Metal I2S Driver - STM32F407
 * Direct register access for I2S3 audio interface with DMA
 * 
 * I2S3 is used with DMA for streaming audio to WM8994 codec
 * Pins:
 * - PC7: MCLK (Master Clock)
 * - PC10: CK (Clock)
 * - PC12: SD (Serial Data)
 * - PA4: WS (Word Select)
 * - AF6 for all pins
 */

#ifndef __I2S_H__
#define __I2S_H__

#include <stdint.h>

/* I2S sample rate */
typedef enum {
    I2S_SR_44100 = 44100,
    I2S_SR_48000 = 48000,
    I2S_SR_96000 = 96000
} i2s_sample_rate_t;

/* Initialize I2S3 for audio streaming */
void i2s_init(i2s_sample_rate_t sample_rate);

/* Start I2S streaming via DMA */
void i2s_start_dma(const int16_t* buffer, uint32_t samples);

/* Stop I2S streaming */
void i2s_stop(void);

/* Check if DMA transfer complete */
uint8_t i2s_dma_complete(void);

#endif /* __I2S_H__ */
