/**
 * Bare Metal I2C Implementation - STM32F407
 * Direct register access to I2C peripherals
 * 
 * I2C1: APB1 (42MHz) - used for codec (WM8994)
 * I2C2: APB1 (42MHz)
 * I2C3: APB1 (42MHz)
 * 
 * Pins:
 * I2C1: PB6 (SCL), PB7 (SDA) - AF4
 */

#include "i2c.h"
#include "gpio.h"
#include "system.h"
#include "stm32f407xx.h"

/* I2C base addresses */
static I2C_TypeDef* const i2c_bases[4] = {NULL, I2C1, I2C2, I2C3};

/* I2C timeout in milliseconds */
#define I2C_TIMEOUT_MS 1000

/**
 * Calculate I2C CCR value for given clock speed
 * F407 APB1 clock is 42 MHz
 * For standard I2C (100kHz): CCR = 42MHz / (2 * 100kHz) = 210
 * For fast I2C (400kHz): CCR = 42MHz / (3 * 400kHz) = 35 (with DUTY=1)
 */
static uint16_t i2c_calculate_ccr(uint32_t clock_speed) {
    uint32_t pclk = 42000000;  /* APB1 = 42MHz for I2C */
    
    if (clock_speed <= 100000) {
        /* Standard mode: CCR = Fpclk / (2 * f_i2c) */
        return pclk / (2 * clock_speed);
    } else {
        /* Fast mode: CCR = Fpclk / (3 * f_i2c) with DUTY=1 */
        return pclk / (3 * clock_speed);
    }
}

/**
 * Calculate TRISE value for given clock speed
 * Standard mode: TRISE = (1000ns / Tpclk) + 1
 * Fast mode: TRISE = (300ns / Tpclk) + 1
 */
static uint16_t i2c_calculate_trise(uint32_t clock_speed) {
    uint32_t pclk = 42000000;  /* APB1 = 42MHz for I2C */
    
    if (clock_speed <= 100000) {
        /* Standard mode: max rise time = 1000ns */
        return ((1000 * pclk) / 1000000000) + 1;
    } else {
        /* Fast mode: max rise time = 300ns */
        return ((300 * pclk) / 1000000000) + 1;
    }
}

/**
 * Initialize I2C bus
 * 
 * Configures I2C peripheral and associated GPIO pins
 */
void i2c_init(i2c_bus_t bus, uint32_t clock_speed) {
    if (bus < 1 || bus > 3) return;
    
    I2C_TypeDef* i2c = i2c_bases[bus];
    
    if (bus == I2C_BUS_1) {
        /* Enable I2C1 clock */
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        
        /* Configure PB6 (SCL), PB7 (SDA) as open-drain alternate function AF4 */
        gpio_init_port(GPIO_PORT_B);
        gpio_config(GPIO_PORT_B, 6, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_OD, GPIO_SPEED_HIGH, GPIO_PULL_UP);
        gpio_config(GPIO_PORT_B, 7, GPIO_MODE_ALT_FUNC, GPIO_OUTPUT_OD, GPIO_SPEED_HIGH, GPIO_PULL_UP);
        gpio_config_alt_func(GPIO_PORT_B, 6, 4);
        gpio_config_alt_func(GPIO_PORT_B, 7, 4);
    }
    
    /* Disable I2C peripheral */
    i2c->CR1 &= ~I2C_CR1_PE;
    
    /* Configure I2C clock */
    uint16_t ccr = i2c_calculate_ccr(clock_speed);
    uint16_t trise = i2c_calculate_trise(clock_speed);
    
    /* Set CCR value */
    i2c->CCR &= ~0xFFF;
    i2c->CCR |= ccr;
    
    /* Set TRISE value */
    i2c->TRISE = trise;
    
    /* Enable I2C, ACK generation, generate START */
    i2c->CR1 |= I2C_CR1_PE;
    i2c->CR1 |= I2C_CR1_ACK;
    i2c->CR1 |= I2C_CR1_ENGC;  /* General call */
}

/**
 * Wait for I2C event
 * Returns 1 if event occurred, 0 if timeout
 */
static uint8_t i2c_wait_event(i2c_bus_t bus, uint32_t event_flag) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return 0;
    
    uint32_t timeout = 0;
    uint32_t timeout_max = I2C_TIMEOUT_MS * 1000;  /* Approximate */
    
    while (!(i2c->SR1 & event_flag) && timeout < timeout_max) {
        timeout++;
    }
    
    return (i2c->SR1 & event_flag) ? 1 : 0;
}

/**
 * I2C start condition
 */
static void i2c_start(i2c_bus_t bus) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return;
    
    i2c->CR1 |= I2C_CR1_START;
    i2c_wait_event(bus, I2C_SR1_SB);  /* Wait for START to complete */
}

/**
 * I2C stop condition
 */
static void i2c_stop(i2c_bus_t bus) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return;
    
    i2c->CR1 |= I2C_CR1_STOP;
}

/**
 * I2C send address byte
 * Bit 0 = read/write (0 for write, 1 for read)
 */
static void i2c_send_address(i2c_bus_t bus, uint8_t addr) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return;
    
    i2c->DR = addr;
    i2c_wait_event(bus, I2C_SR1_ADDR);  /* Wait for address sent */
    
    /* Clear ADDR flag by reading SR2 */
    (void)i2c->SR2;
}

/**
 * I2C write byte
 */
static void i2c_write_byte(i2c_bus_t bus, uint8_t byte) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return;
    
    i2c_wait_event(bus, I2C_SR1_TXE);  /* Wait for TX ready */
    i2c->DR = byte;
}

/**
 * I2C read byte
 */
static uint8_t i2c_read_byte(i2c_bus_t bus, uint8_t ack) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return 0;
    
    if (ack) {
        i2c->CR1 |= I2C_CR1_ACK;
    } else {
        i2c->CR1 &= ~I2C_CR1_ACK;
    }
    
    i2c_wait_event(bus, I2C_SR1_RXNE);  /* Wait for RX ready */
    return i2c->DR;
}

/**
 * Check if I2C is busy (generates START automatically if needed)
 */
uint8_t i2c_is_busy(i2c_bus_t bus) {
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return 0;
    
    return (i2c->SR2 & I2C_SR2_BUSY) ? 1 : 0;
}

/**
 * I2C write operation
 * addr: 7-bit slave address (will be shifted left by 1)
 * data: pointer to data buffer
 * len: number of bytes to write
 */
int i2c_write(i2c_bus_t bus, uint8_t addr, const uint8_t* data, uint32_t len) {
    if (!data || len == 0 || bus < 1 || bus > 3) return -1;
    
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return -1;
    
    /* Generate START condition */
    i2c_start(bus);
    
    /* Send address byte (write mode = addr << 1 | 0) */
    i2c_send_address(bus, (addr << 1) | 0);
    
    /* Write data bytes */
    for (uint32_t i = 0; i < len; i++) {
        i2c_write_byte(bus, data[i]);
    }
    
    /* Wait for last byte transmitted */
    i2c_wait_event(bus, I2C_SR1_BTF);
    
    /* Generate STOP condition */
    i2c_stop(bus);
    
    return 0;
}

/**
 * I2C read operation
 * addr: 7-bit slave address
 * data: pointer to receive buffer
 * len: number of bytes to read
 */
int i2c_read(i2c_bus_t bus, uint8_t addr, uint8_t* data, uint32_t len) {
    if (!data || len == 0 || bus < 1 || bus > 3) return -1;
    
    I2C_TypeDef* i2c = i2c_bases[bus];
    if (!i2c) return -1;
    
    /* Generate START condition */
    i2c_start(bus);
    
    /* Send address byte (read mode = addr << 1 | 1) */
    i2c_send_address(bus, (addr << 1) | 1);
    
    /* Read data bytes */
    for (uint32_t i = 0; i < len; i++) {
        if (i == (len - 1)) {
            /* Last byte - send NACK */
            data[i] = i2c_read_byte(bus, 0);
        } else {
            data[i] = i2c_read_byte(bus, 1);
        }
    }
    
    /* Generate STOP condition */
    i2c_stop(bus);
    
    return 0;
}

/**
 * I2C write then read operation
 * Useful for reading register values
 * addr: 7-bit slave address
 * reg: register address to write
 * data: pointer to read buffer
 * len: number of bytes to read
 */
int i2c_write_read(i2c_bus_t bus, uint8_t addr, uint8_t reg, uint8_t* data, uint32_t len) {
    if (!data || len == 0 || bus < 1 || bus > 3) return -1;
    
    uint8_t reg_addr = reg;
    
    /* Write register address */
    if (i2c_write(bus, addr, &reg_addr, 1) != 0) {
        return -1;
    }
    
    /* Small delay between write and read */
    system_delay_us(10);
    
    /* Read register value */
    if (i2c_read(bus, addr, data, len) != 0) {
        return -1;
    }
    
    return 0;
}
