# Bare Metal Drivers - Quick Reference

## System & Timing

```c
#include "system.h"

system_init();              // Initialize clock to 168MHz, enable SysTick
system_get_tick();          // Returns milliseconds since startup
system_delay_ms(100);       // Block for 100ms
system_delay_us(50);        // Block for 50µs
```

## GPIO Control

```c
#include "gpio.h"

/* Initialize port */
gpio_init_port(GPIO_PORT_A);

/* Configure pin */
gpio_config(GPIO_PORT_D, 13, GPIO_MODE_OUTPUT, 
            GPIO_OUTPUT_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

/* Set/Clear/Toggle */
gpio_set(GPIO_PORT_D, 13);      // Set HIGH
gpio_clear(GPIO_PORT_D, 13);    // Set LOW  
gpio_toggle(GPIO_PORT_D, 13);   // Toggle

/* Read input */
uint8_t val = gpio_read(GPIO_PORT_A, 0);  // Returns 0 or 1
gpio_write(GPIO_PORT_A, 0, 1);            // Write 0 or 1

/* Alternate function (for SPI, I2C, I2S) */
gpio_config_alt_func(GPIO_PORT_B, 3, 5);  // PB3 = AF5

/* External interrupt */
gpio_config_interrupt(GPIO_PORT_A, 0, GPIO_INT_FALLING);
gpio_exti_clear(0);  // Clear pending flag in ISR
```

## SPI Bus

```c
#include "spi.h"

/* Initialize SPI5 at 42MHz (84MHz/2) */
spi_init(SPI_BUS_5, SPI_DATASIZE_8BIT, SPI_PRESCALER_2,
         SPI_CPOL_LOW, SPI_CPHA_1EDGE);

/* Single byte */
spi_write_byte(SPI_BUS_5, 0xA0);
uint8_t data = spi_read_byte(SPI_BUS_5);

/* Multiple bytes */
uint8_t tx[10] = {...};
uint8_t rx[10];
spi_write(SPI_BUS_5, tx, 10);
spi_read(SPI_BUS_5, rx, 10);
spi_transfer(SPI_BUS_5, tx, rx, 10);
```

## I2C Bus

```c
#include "i2c.h"

/* Initialize I2C1 at 400kHz */
i2c_init(I2C_BUS_1, 400000);

/* Write data to slave */
uint8_t data[3] = {0xA0, 0x12, 0x34};
i2c_write(I2C_BUS_1, 0x1A, data, 3);

/* Read data from slave */
uint8_t rx[2];
i2c_read(I2C_BUS_1, 0x1A, rx, 2);

/* Write then read (register read) */
uint8_t reg_value[2];
i2c_write_read(I2C_BUS_1, 0x1A, 0x00, reg_value, 2);
```

## I2S Audio

```c
#include "i2s.h"

/* Initialize I2S3 at 44.1kHz */
i2s_init(I2S_SR_44100);

/* Start DMA streaming */
int16_t audio_buffer[44100];  // 1 second of 44.1kHz stereo
i2s_start_dma(audio_buffer, 44100);

/* Wait for completion */
while (!i2s_dma_complete()) {
    /* Optional: refill buffer or do other work */
}

/* Stop */
i2s_stop();
```

## Button Handling

```c
#include "buttons.h"
#include "gpio.h"

void button_callback(button_event_t event) {
    if (event == BUTTON_PRESSED) {
        /* Handle press */
    } else if (event == BUTTON_RELEASED) {
        /* Handle release */
    }
}

int main(void) {
    buttons_init();
    buttons_register_callback(BTN_PLAY_PAUSE, button_callback);
    
    while (1) {
        buttons_poll();  /* Call regularly in main loop */
    }
}
```

## LCD Display

```c
#include "lcd_display.h"

lcd_init();
lcd_fill_rect(0, 0, 240, 320, COLOR_BLACK);
lcd_draw_text(10, 50, "Hello", COLOR_WHITE, COLOR_BLACK, 2);
```

## Complete Example

```c
#include "system.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include "i2s.h"
#include "buttons.h"

int main(void) {
    /* Initialize system clock to 168MHz */
    system_init();
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    /* Initialize GPIO ports */
    gpio_init_port(GPIO_PORT_A);
    gpio_init_port(GPIO_PORT_D);
    gpio_init_port(GPIO_PORT_F);
    
    /* Initialize peripherals */
    spi_init(SPI_BUS_5, SPI_DATASIZE_8BIT, SPI_PRESCALER_2,
             SPI_CPOL_LOW, SPI_CPHA_1EDGE);
    i2c_init(I2C_BUS_1, 400000);
    i2s_init(I2S_SR_44100);
    
    /* Initialize buttons */
    buttons_init();
    buttons_register_callback(BTN_PLAY_PAUSE, on_play_pressed);
    
    /* Main loop */
    while (1) {
        buttons_poll();
        system_delay_ms(10);
    }
}
```

## Interrupt Handlers

```c
/* EXTI0 - PA0 button */
void EXTI0_IRQHandler(void) {
    gpio_exti_clear(0);
    /* Handle button press */
}

/* EXTI15-10 - PD13-15 buttons */
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (1 << 13)) {
        gpio_exti_clear(13);
        /* Handle PD13 */
    }
    if (EXTI->PR & (1 << 14)) {
        gpio_exti_clear(14);
        /* Handle PD14 */
    }
    if (EXTI->PR & (1 << 15)) {
        gpio_exti_clear(15);
        /* Handle PD15 */
    }
}

/* I2S3 DMA Transfer Complete */
void DMA1_Stream5_IRQHandler(void) {
    if (DMA1->HISR & DMA_HISR_TCIF5) {
        i2s_dma_complete_flag = 1;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF5;
    }
}
```

## GPIO Port Enum

```c
GPIO_PORT_A = 0    /* GPIOA */
GPIO_PORT_B = 1    /* GPIOB */
GPIO_PORT_C = 2    /* GPIOC */
GPIO_PORT_D = 3    /* GPIOD */
GPIO_PORT_E = 4    /* GPIOE */
GPIO_PORT_F = 5    /* GPIOF */
GPIO_PORT_G = 6    /* GPIOG */
GPIO_PORT_H = 7    /* GPIOH */
GPIO_PORT_I = 8    /* GPIOI */
```

## GPIO Mode Enum

```c
GPIO_MODE_INPUT        /* Input mode */
GPIO_MODE_OUTPUT       /* Output (push-pull or open-drain) */
GPIO_MODE_ALT_FUNC     /* Alternate function (SPI, I2C, etc) */
GPIO_MODE_ANALOG       /* Analog mode (ADC) */
```

## SPI Bus Enum

```c
SPI_BUS_1  /* SPI1 - APB2 (84MHz) */
SPI_BUS_2  /* SPI2 - APB1 (42MHz) */
SPI_BUS_3  /* SPI3 - APB1 (42MHz) */
SPI_BUS_4  /* SPI4 - APB2 (84MHz) */
SPI_BUS_5  /* SPI5 - APB2 (84MHz) */
```

## SPI Prescaler Enum

```c
SPI_PRESCALER_2      /* fpclk/2 */
SPI_PRESCALER_4      /* fpclk/4 */
SPI_PRESCALER_8      /* fpclk/8 */
SPI_PRESCALER_16     /* fpclk/16 */
SPI_PRESCALER_32     /* fpclk/32 */
SPI_PRESCALER_64     /* fpclk/64 */
SPI_PRESCALER_128    /* fpclk/128 */
SPI_PRESCALER_256    /* fpclk/256 */
```

## I2C Bus Enum

```c
I2C_BUS_1  /* I2C1 - APB1, PB6 (SCL) / PB7 (SDA) */
I2C_BUS_2  /* I2C2 - APB1 */
I2C_BUS_3  /* I2C3 - APB1 */
```

## I2S Sample Rate Enum

```c
I2S_SR_44100  /* 44.1 kHz */
I2S_SR_48000  /* 48 kHz */
I2S_SR_96000  /* 96 kHz */
```

## Button Enum

```c
BTN_PREVIOUS   /* PD13 */
BTN_PLAY_PAUSE /* PD14 */
BTN_NEXT       /* PD15 */
BTN_VOL_UP     /* PA0 */
BTN_VOL_DOWN   /* PD0 */
BTN_SHUFFLE    /* PD1 */
BTN_LOOP       /* PD2 */
```

---

**Ready to use!** Copy-paste examples and modify for your needs. 📋
