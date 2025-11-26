/**
 * Bare Metal I2C Driver - STM32F407
 * Direct register access for I2C1, I2C2, I2C3
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>

/* I2C Bus selection */
typedef enum {
    I2C_BUS_1 = 1,
    I2C_BUS_2 = 2,
    I2C_BUS_3 = 3
} i2c_bus_t;

/* Initialize I2C bus */
void i2c_init(i2c_bus_t bus, uint32_t clock_speed);

/* I2C write operation (address + data) */
int i2c_write(i2c_bus_t bus, uint8_t addr, const uint8_t* data, uint32_t len);

/* I2C read operation (address + data) */
int i2c_read(i2c_bus_t bus, uint8_t addr, uint8_t* data, uint32_t len);

/* I2C write then read (write register address, read register value) */
int i2c_write_read(i2c_bus_t bus, uint8_t addr, uint8_t reg, uint8_t* data, uint32_t len);

/* Check if I2C is busy */
uint8_t i2c_is_busy(i2c_bus_t bus);

#endif /* __I2C_H__ */
