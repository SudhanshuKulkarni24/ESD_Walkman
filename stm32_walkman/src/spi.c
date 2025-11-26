/**
 * Bare Metal SPI Implementation - STM32F407
 * Direct register access to SPI peripherals
 * 
 * SPI1: APB2 (84MHz) - used for LCD
 * SPI2: APB1 (42MHz) - used for SD card via SPI (not used if using SDIO)
 * SPI3: APB1 (42MHz)
 * SPI4: APB2 (84MHz)
 * SPI5: APB2 (84MHz) - used for LCD on F407 Discovery
 */

#include "spi.h"
#include "gpio.h"
#include "stm32f407xx.h"

/* SPI base addresses */
static SPI_TypeDef* const spi_bases[6] = {NULL, SPI1, SPI2, SPI3, SPI4, SPI5};

/**
 * Initialize SPI bus with given parameters
 * 
 * This configures the SPI peripheral and associated GPIO pins
 * 
 * SPI1 pins (APB2):
 * - PB3: SCK, PB4: MISO, PB5: MOSI (AF5)
 * - PA4: NSS (software managed for LCD chip select)
 * 
 * SPI5 pins (APB2):
 * - PF7: SCK, PF8: MISO, PF9: MOSI (AF5)
 * - PF6: NSS (for LCD - software managed)
 */
void spi_init(spi_bus_t bus, spi_datasize_t datasize, spi_prescaler_t prescaler,
              spi_cpol_t cpol, spi_cpha_t cpha) {
    if (bus < 1 || bus > 5) return;
    
    SPI_TypeDef* spi = spi_bases[bus];
    
    /* Enable SPI clock and configure GPIO pins */
    if (bus == SPI_BUS_1) {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        
        /* Configure PB3 (SCK), PB4 (MISO), PB5 (MOSI) as alternate function AF5 */
        gpio_init_port(GPIO_PORT_B);
        gpio_config(GPIO_PORT_B, 3, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config(GPIO_PORT_B, 4, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config(GPIO_PORT_B, 5, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config_alt_func(GPIO_PORT_B, 3, 5);
        gpio_config_alt_func(GPIO_PORT_B, 4, 5);
        gpio_config_alt_func(GPIO_PORT_B, 5, 5);
        
        /* Configure PA4 (NSS) as GPIO output for manual control */
        gpio_config(GPIO_PORT_A, 4, GPIO_MODE_OUTPUT, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_set(GPIO_PORT_A, 4);
        
    } else if (bus == SPI_BUS_5) {
        RCC->APB2ENR |= RCC_APB2ENR_SPI5EN;
        
        /* Configure PF7 (SCK), PF8 (MISO), PF9 (MOSI) as alternate function AF5 */
        gpio_init_port(GPIO_PORT_F);
        gpio_config(GPIO_PORT_F, 7, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config(GPIO_PORT_F, 8, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config(GPIO_PORT_F, 9, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_config_alt_func(GPIO_PORT_F, 7, 5);
        gpio_config_alt_func(GPIO_PORT_F, 8, 5);
        gpio_config_alt_func(GPIO_PORT_F, 9, 5);
        
        /* Configure PF6 (NSS) as GPIO output for manual control */
        gpio_config(GPIO_PORT_F, 6, GPIO_MODE_OUTPUT, GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
        gpio_set(GPIO_PORT_F, 6);
        
    } else {
        /* SPI2, SPI3, SPI4 not configured here yet */
        return;
    }
    
    /* Reset SPI peripheral */
    spi->CR1 = 0;
    
    /* Configure SPI */
    uint32_t cr1 = 0;
    
    /* Set clock divider (prescaler) */
    cr1 |= (prescaler << SPI_CR1_BR_Pos);
    
    /* Master mode */
    cr1 |= SPI_CR1_MSTR;
    
    /* Clock polarity and phase */
    if (cpol == SPI_CPOL_HIGH) {
        cr1 |= SPI_CR1_CPOL;
    }
    if (cpha == SPI_CPHA_2EDGE) {
        cr1 |= SPI_CR1_CPHA;
    }
    
    /* Data size */
    if (datasize == SPI_DATASIZE_8BIT) {
        cr1 &= ~SPI_CR1_DFF;  /* 8-bit */
    } else {
        cr1 |= SPI_CR1_DFF;   /* 16-bit */
    }
    
    /* Software slave management (NSS via GPIO) */
    cr1 |= SPI_CR1_SSM;
    cr1 |= SPI_CR1_SSI;
    
    /* MSB first */
    cr1 &= ~SPI_CR1_LSBFIRST;
    
    spi->CR1 = cr1;
    
    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;
}

/**
 * Get SPI peripheral by bus number
 */
static SPI_TypeDef* spi_get_periph(spi_bus_t bus) {
    if (bus < 1 || bus > 5) return NULL;
    return spi_bases[bus];
}

/**
 * Check if SPI is busy (BSY flag in SR)
 */
uint8_t spi_is_busy(spi_bus_t bus) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return 0;
    return (spi->SR & SPI_SR_BSY) ? 1 : 0;
}

/**
 * Wait for SPI to be ready for transmission
 */
static void spi_wait_txe(spi_bus_t bus) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    while (!(spi->SR & SPI_SR_TXE));
}

/**
 * Wait for SPI to have data ready
 */
static void spi_wait_rxne(spi_bus_t bus) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    while (!(spi->SR & SPI_SR_RXNE));
}

/**
 * Wait for SPI to finish (not busy)
 */
static void spi_wait_busy(spi_bus_t bus) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    while (spi->SR & SPI_SR_BSY);
}

/**
 * Send single byte via SPI
 */
void spi_write_byte(spi_bus_t bus, uint8_t byte) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    
    spi_wait_txe(bus);
    *((__IO uint8_t*)&spi->DR) = byte;
    spi_wait_busy(bus);
    
    /* Read dummy byte to clear RXNE flag */
    (void)spi->DR;
}

/**
 * Receive single byte via SPI
 */
uint8_t spi_read_byte(spi_bus_t bus) {
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return 0;
    
    spi_wait_txe(bus);
    *((__IO uint8_t*)&spi->DR) = 0xFF;  /* Send dummy byte */
    spi_wait_rxne(bus);
    
    return *((__IO uint8_t*)&spi->DR);
}

/**
 * Send data buffer via SPI (blocking)
 */
void spi_write(spi_bus_t bus, const uint8_t* data, uint32_t len) {
    if (!data || len == 0) return;
    
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    
    for (uint32_t i = 0; i < len; i++) {
        spi_wait_txe(bus);
        *((__IO uint8_t*)&spi->DR) = data[i];
    }
    
    spi_wait_busy(bus);
    
    /* Read dummy bytes to clear RXNE flags */
    while (spi->SR & SPI_SR_RXNE) {
        (void)spi->DR;
    }
}

/**
 * Read data buffer via SPI (blocking)
 * Sends dummy bytes and reads response
 */
void spi_read(spi_bus_t bus, uint8_t* data, uint32_t len) {
    if (!data || len == 0) return;
    
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    
    for (uint32_t i = 0; i < len; i++) {
        spi_wait_txe(bus);
        *((__IO uint8_t*)&spi->DR) = 0xFF;  /* Send dummy byte */
        spi_wait_rxne(bus);
        data[i] = *((__IO uint8_t*)&spi->DR);
    }
    
    spi_wait_busy(bus);
}

/**
 * Send and receive simultaneously via SPI
 */
void spi_transfer(spi_bus_t bus, const uint8_t* tx, uint8_t* rx, uint32_t len) {
    if (len == 0) return;
    
    SPI_TypeDef* spi = spi_get_periph(bus);
    if (!spi) return;
    
    for (uint32_t i = 0; i < len; i++) {
        spi_wait_txe(bus);
        uint8_t byte = (tx) ? tx[i] : 0xFF;
        *((__IO uint8_t*)&spi->DR) = byte;
        spi_wait_rxne(bus);
        byte = *((__IO uint8_t*)&spi->DR);
        if (rx) rx[i] = byte;
    }
    
    spi_wait_busy(bus);
}
