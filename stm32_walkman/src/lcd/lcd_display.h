/**
 * LCD Display Driver Header
 */

#ifndef __LCD_DISPLAY_H
#define __LCD_DISPLAY_H

#include <stdint.h>
#include "player.h"

/* LCD Dimensions */
#define LCD_WIDTH 240
#define LCD_HEIGHT 320

/* LCD Control Pins (adjust based on your STM32 board) */
#define LCD_GPIO_PORT GPIOA
#define LCD_CS_PIN    GPIO_PIN_4   // Chip Select
#define LCD_DC_PIN    GPIO_PIN_5   // Data/Command
#define LCD_RST_PIN   GPIO_PIN_6   // Reset

/* RGB565 Color Definitions */
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_DARK_GREEN  0x0320   // Spotify-like dark green
#define COLOR_LIGHT_GREEN 0x07FF
#define COLOR_GRAY        0x8410
#define COLOR_DARK_GRAY   0x4208
#define COLOR_YELLOW      0xFFE0

typedef enum {
    LCD_OK = 0,
    LCD_ERROR = 1
} lcd_status_t;

typedef struct {
    uint8_t initialized;
    uint16_t width;
    uint16_t height;
} lcd_state_t;

/* Initialization */
int lcd_init(void);
void lcd_reset(void);

/* Low-level drawing */
void lcd_write_cmd(uint8_t cmd);
void lcd_write_data(uint8_t data);
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

/* High-level display */
void lcd_display_song_info(const char* title, const char* artist, 
                           uint32_t duration_sec, uint32_t position_sec);
void lcd_display_status(const char* status_text);
void lcd_draw_text(uint16_t x, uint16_t y, const char* text, 
                   uint16_t fg_color, uint16_t bg_color, uint8_t size);
void lcd_draw_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     const char* label, uint16_t bg_color, uint16_t fg_color);
void lcd_update(const player_t* player, uint32_t position);
void lcd_display_volume(uint8_t volume);

#endif /* __LCD_DISPLAY_H */
