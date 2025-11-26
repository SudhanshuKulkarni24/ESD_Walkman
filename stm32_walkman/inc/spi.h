/**
 * Bare Metal SPI Driver - STM32F407
 * Direct register access for SPI1, SPI2, SPI3, SPI4, SPI5
 */

#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>

/* SPI Bus selection */
typedef enum {
    SPI_BUS_1 = 1,
    SPI_BUS_2 = 2,
    SPI_BUS_3 = 3,
    SPI_BUS_4 = 4,
    SPI_BUS_5 = 5
} spi_bus_t;

/* SPI Clock polarity */
typedef enum {
    SPI_CPOL_LOW = 0,
    SPI_CPOL_HIGH = 1
} spi_cpol_t;

/* SPI Clock phase */
typedef enum {
    SPI_CPHA_1EDGE = 0,
    SPI_CPHA_2EDGE = 1
} spi_cpha_t;

/* SPI Data size */
typedef enum {
    SPI_DATASIZE_8BIT = 0,
    SPI_DATASIZE_16BIT = 1
} spi_datasize_t;

/* SPI Baud rate prescaler */
typedef enum {
    SPI_PRESCALER_2 = 0,    /* fpclk/2 */
    SPI_PRESCALER_4 = 1,    /* fpclk/4 */
    SPI_PRESCALER_8 = 2,    /* fpclk/8 */
    SPI_PRESCALER_16 = 3,   /* fpclk/16 */
    SPI_PRESCALER_32 = 4,   /* fpclk/32 */
    SPI_PRESCALER_64 = 5,   /* fpclk/64 */
    SPI_PRESCALER_128 = 6,  /* fpclk/128 */
    SPI_PRESCALER_256 = 7   /* fpclk/256 */
} spi_prescaler_t;

/* Initialize SPI bus */
void spi_init(spi_bus_t bus, spi_datasize_t datasize, spi_prescaler_t prescaler,
              spi_cpol_t cpol, spi_cpha_t cpha);

/* Send data via SPI (blocking) */
void spi_write(spi_bus_t bus, const uint8_t* data, uint32_t len);

/* Read data via SPI (blocking) */
void spi_read(spi_bus_t bus, uint8_t* data, uint32_t len);

/* Send and receive simultaneously */
void spi_transfer(spi_bus_t bus, const uint8_t* tx, uint8_t* rx, uint32_t len);

/* Send single byte */
void spi_write_byte(spi_bus_t bus, uint8_t byte);

/* Receive single byte */
uint8_t spi_read_byte(spi_bus_t bus);

/* Check if SPI is busy */
uint8_t spi_is_busy(spi_bus_t bus);

#endif /* __SPI_H__ */
