/**
 * Bare Metal I2S Implementation - STM32F407
 * Direct register access for I2S3 with DMA support
 * 
 * I2S3 is connected to SPI3 peripheral
 * Uses DMA1 stream 5 for transmit
 * 
 * Configuration:
 * - Master mode
 * - 16-bit data
 * - Stereo mode
 * - Slave transmit (data goes to codec)
 * - DMA for continuous streaming
 */

#include "i2s.h"
#include "gpio.h"
#include "system.h"
#include "stm32f407xx.h"

/* I2S3 DMA status */
static volatile uint32_t i2s_dma_complete_flag = 0;

/**
 * DMA1 Stream 5 interrupt handler (I2S3 TX complete)
 */
void DMA1_Stream5_IRQHandler(void) {
    if (DMA1->HISR & DMA_HISR_TCIF5) {
        i2s_dma_complete_flag = 1;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF5;  /* Clear flag */
    }
}

/**
 * Calculate I2S prescaler and lin prescaler for given sample rate
 * F407 APB1 = 42MHz, APB2 = 84MHz
 * I2S3 is on APB1 (SPI3)
 * 
 * I2S Clock = PCLK / (prescaler * 2) if lin_prescaler = 0
 * For 44.1kHz, 16-bit, stereo: bit_clock = 44.1 * 32 * 2 = 2.8224 MHz
 */
static void i2s_calculate_prescaler(i2s_sample_rate_t sample_rate, 
                                    uint8_t* prescaler, uint8_t* lin_pres) {
    uint32_t pclk = 42000000;  /* APB1 for SPI3 */
    uint32_t bit_rate = sample_rate * 32 * 2;  /* 16-bit, stereo, 2 channels */
    
    /* Calculate prescaler */
    uint32_t div = pclk / (2 * bit_rate);
    
    if (div <= 1) {
        *prescaler = 1;
        *lin_pres = 0;
    } else if (div <= 255) {
        *prescaler = div;
        *lin_pres = 0;
    } else {
        *prescaler = 255;
        *lin_pres = (div + 254) / 255;  /* Round up */
    }
}

/**
 * Initialize I2S3 for audio streaming
 * 
 * Configuration:
 * - Master mode transmit
 * - 16-bit data
 * - Stereo
 * - DMA enabled
 */
void i2s_init(i2s_sample_rate_t sample_rate) {
    uint8_t prescaler, lin_pres;
    
    /* Enable SPI3 (I2S3) clock on APB1 */
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    
    /* Enable DMA1 clock on AHB */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    
    /* Configure I2S3 pins */
    /* PC7 (MCLK), PC10 (CK), PC12 (SD) - AF6 */
    gpio_init_port(GPIO_PORT_C);
    gpio_config(GPIO_PORT_C, 7, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    gpio_config(GPIO_PORT_C, 10, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    gpio_config(GPIO_PORT_C, 12, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    gpio_config_alt_func(GPIO_PORT_C, 7, 6);
    gpio_config_alt_func(GPIO_PORT_C, 10, 6);
    gpio_config_alt_func(GPIO_PORT_C, 12, 6);
    
    /* PA4 (WS) - AF6 */
    gpio_init_port(GPIO_PORT_A);
    gpio_config(GPIO_PORT_A, 4, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    gpio_config_alt_func(GPIO_PORT_A, 4, 6);
    
    /* Calculate prescaler for sample rate */
    i2s_calculate_prescaler(sample_rate, &prescaler, &lin_pres);
    
    /* Configure SPI3 as I2S master transmitter */
    SPI3->I2SCFGR = 0;
    SPI3->I2SPR = 0;
    
    /* I2SCFGR settings */
    uint32_t i2scfgr = 0;
    i2scfgr |= SPI_I2SCFGR_I2SMOD;           /* I2S mode (not SPI) */
    i2scfgr |= SPI_I2SCFGR_I2SCFG_1;         /* Master transmit */
    i2scfgr |= SPI_I2SCFGR_PCMSYNC;          /* PCM frame synchronization */
    i2scfgr |= (0 << SPI_I2SCFGR_DATLEN_Pos); /* 16-bit data */
    i2scfgr |= (0 << SPI_I2SCFGR_CHLEN_Pos);  /* 16-bit channel length */
    i2scfgr |= SPI_I2SCFGR_CKPOL;            /* Clock polarity */
    
    SPI3->I2SCFGR = i2scfgr;
    
    /* I2SPR settings (prescaler) */
    uint32_t i2spr = 0;
    i2spr |= (prescaler << SPI_I2SPR_I2SDIV_Pos);
    i2spr |= (lin_pres << SPI_I2SPR_ODD_Pos);
    i2spr |= SPI_I2SPR_MCKOE;                 /* Enable master clock output */
    
    SPI3->I2SPR = i2spr;
    
    /* Configure DMA1 Stream 5 for SPI3 TX */
    /* Stream 5, Channel 0 is SPI3_TX */
    DMA1_Stream5->CR = 0;  /* Reset */
    
    uint32_t dma_cr = 0;
    dma_cr |= (0 << DMA_SxCR_CHSEL_Pos);      /* Channel 0 for SPI3 */
    dma_cr |= (1 << DMA_SxCR_PL_Pos);         /* Medium priority */
    dma_cr |= (1 << DMA_SxCR_MSIZE_Pos);      /* Memory size 16-bit */
    dma_cr |= (1 << DMA_SxCR_PSIZE_Pos);      /* Peripheral size 16-bit */
    dma_cr |= DMA_SxCR_MINC;                  /* Memory increment */
    dma_cr |= DMA_SxCR_DIR_0;                 /* Memory to peripheral */
    dma_cr |= DMA_SxCR_TCIE;                  /* Transfer complete interrupt */
    
    DMA1_Stream5->CR = dma_cr;
    
    /* Set DMA peripheral address (SPI3 data register) */
    DMA1_Stream5->PAR = (uint32_t)&(SPI3->DR);
    
    /* Enable DMA interrupt */
    NVIC_SetPriority(DMA1_Stream5_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    
    /* Enable I2S peripheral */
    SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
    
    i2s_dma_complete_flag = 0;
}

/**
 * Start I2S streaming via DMA
 * 
 * Parameters:
 * - buffer: 16-bit audio samples (stereo interleaved: L R L R ...)
 * - samples: number of samples to transfer (not bytes)
 */
void i2s_start_dma(const int16_t* buffer, uint32_t samples) {
    if (!buffer || samples == 0) return;
    
    /* Disable DMA stream first */
    DMA1_Stream5->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream5->CR & DMA_SxCR_EN);  /* Wait for disable */
    
    /* Clear all flags for stream 5 */
    DMA1->HIFCR |= (DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5 | 
                    DMA_HIFCR_CTCIF5);
    
    /* Set memory address and number of items to transfer */
    DMA1_Stream5->M0AR = (uint32_t)buffer;
    DMA1_Stream5->NDTR = samples;
    
    /* Enable DMA stream */
    DMA1_Stream5->CR |= DMA_SxCR_EN;
    
    /* Clear complete flag */
    i2s_dma_complete_flag = 0;
}

/**
 * Stop I2S streaming
 */
void i2s_stop(void) {
    /* Disable I2S */
    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    
    /* Disable DMA */
    DMA1_Stream5->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream5->CR & DMA_SxCR_EN);
}

/**
 * Check if DMA transfer is complete
 */
uint8_t i2s_dma_complete(void) {
    return i2s_dma_complete_flag;
}
