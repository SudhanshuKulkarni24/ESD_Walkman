/**
 * ILI9341 LCD Display Driver for STM32
 * 240x320 TFT Display with SPI Interface
 * Shows current song and playback controls
 */

#include "lcd_display.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* LCD SPI handle */
SPI_HandleTypeDef hspi1;

/* Display buffer and state */
static lcd_state_t lcd_state = {0};

/* Font data (simple 5x7 bitmap font) */
static const uint8_t font_5x7[256][5] = {
    /* Space: 0x20 */
    {0x00, 0x00, 0x00, 0x00, 0x00},
    /* More font data would go here - simplified for demo */
};

/**
 * Initialize LCD display
 */
int lcd_init(void) {
    // Initialize SPI for LCD communication
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        return LCD_ERROR;
    }
    
    // Initialize GPIO for LCD control pins (CS, DC, RST)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LCD_CS_PIN | LCD_DC_PIN | LCD_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_GPIO_PORT, &GPIO_InitStruct);
    
    // Initialize display
    lcd_reset();
    lcd_write_cmd(0x01);  // Software reset
    HAL_Delay(150);
    
    lcd_write_cmd(0x28);  // Display OFF
    lcd_write_cmd(0x11);  // Sleep OUT
    HAL_Delay(150);
    
    lcd_write_cmd(0x29);  // Display ON
    
    // Clear display
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    lcd_state.initialized = 1;
    
    return LCD_OK;
}

/**
 * Reset LCD display
 */
void lcd_reset(void) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(150);
}

/**
 * Write command byte to LCD
 */
void lcd_write_cmd(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_RESET);  // DC = 0 for command
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_RESET);  // CS = 0
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_SET);    // CS = 1
}

/**
 * Write data byte to LCD
 */
void lcd_write_data(uint8_t data) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_SET);    // DC = 1 for data
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_RESET);  // CS = 0
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_SET);    // CS = 1
}

/**
 * Set LCD window/region for drawing
 */
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address set
    lcd_write_cmd(0x2A);
    lcd_write_data(x0 >> 8);
    lcd_write_data(x0 & 0xFF);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFF);
    
    // Row address set
    lcd_write_cmd(0x2B);
    lcd_write_data(y0 >> 8);
    lcd_write_data(y0 & 0xFF);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFF);
    
    // Write to RAM
    lcd_write_cmd(0x2C);
}

/**
 * Fill rectangular area with color
 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;
    
    if (x_end >= LCD_WIDTH) x_end = LCD_WIDTH - 1;
    if (y_end >= LCD_HEIGHT) y_end = LCD_HEIGHT - 1;
    
    lcd_set_window(x, y, x_end, y_end);
    
    uint32_t pixels = w * h;
    uint8_t color_h = color >> 8;
    uint8_t color_l = color & 0xFF;
    
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_SET);    // DC = 1 for data
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_RESET);  // CS = 0
    
    for (uint32_t i = 0; i < pixels; i++) {
        HAL_SPI_Transmit(&hspi1, &color_h, 1, HAL_MAX_DELAY);
        HAL_SPI_Transmit(&hspi1, &color_l, 1, HAL_MAX_DELAY);
    }
    
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_SET);    // CS = 1
}

/**
 * Draw pixel at (x, y) with color
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    lcd_set_window(x, y, x, y);
    
    uint8_t color_h = color >> 8;
    uint8_t color_l = color & 0xFF;
    
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_SET);    // DC = 1 for data
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_RESET);  // CS = 0
    HAL_SPI_Transmit(&hspi1, &color_h, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &color_l, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_CS_PIN, GPIO_PIN_SET);    // CS = 1
}

/**
 * Draw horizontal line
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    lcd_fill_rect(x, y, length, 1, color);
}

/**
 * Draw vertical line
 */
void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    lcd_fill_rect(x, y, 1, length, color);
}

/**
 * Display current song info
 * Shows: Song title, Artist, Duration, Current position
 */
void lcd_display_song_info(const char* title, const char* artist, 
                           uint32_t duration_sec, uint32_t position_sec) {
    // Clear display
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    
    // Header bar (dark green)
    lcd_fill_rect(0, 0, LCD_WIDTH, 40, COLOR_DARK_GREEN);
    
    // Title (white text on dark green)
    lcd_draw_text(10, 12, "NOW PLAYING", COLOR_WHITE, COLOR_DARK_GREEN, 1);
    
    // Song title (large, white)
    lcd_draw_text(10, 50, title, COLOR_WHITE, COLOR_BLACK, 2);
    
    // Artist (gray)
    lcd_draw_text(10, 100, artist, COLOR_GRAY, COLOR_BLACK, 1);
    
    // Progress bar background
    lcd_fill_rect(10, 150, LCD_WIDTH - 20, 20, COLOR_DARK_GRAY);
    
    // Progress bar fill (green)
    if (duration_sec > 0) {
        uint16_t progress_width = ((position_sec * (LCD_WIDTH - 20)) / duration_sec);
        if (progress_width > LCD_WIDTH - 20) progress_width = LCD_WIDTH - 20;
        lcd_fill_rect(10, 150, progress_width, 20, COLOR_GREEN);
    }
    
    // Time info (MM:SS format)
    char time_str[32];
    uint32_t mins = position_sec / 60;
    uint32_t secs = position_sec % 60;
    uint32_t total_mins = duration_sec / 60;
    uint32_t total_secs = duration_sec % 60;
    
    sprintf(time_str, "%02d:%02d / %02d:%02d", mins, secs, total_mins, total_secs);
    lcd_draw_text(10, 180, time_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    // Control buttons area
    lcd_draw_button(20, 240, 60, 40, "◀", COLOR_GRAY, COLOR_WHITE);      // Previous
    lcd_draw_button(110, 240, 60, 40, "▶", COLOR_GREEN, COLOR_BLACK);    // Play/Pause
    lcd_draw_button(200, 240, 60, 40, "▶▶", COLOR_GRAY, COLOR_WHITE);    // Next
}

/**
 * Display playback status
 */
void lcd_display_status(const char* status_text) {
    // Clear bottom area
    lcd_fill_rect(0, 220, LCD_WIDTH, LCD_HEIGHT - 220, COLOR_BLACK);
    
    // Draw status
    lcd_draw_text(10, 230, status_text, COLOR_WHITE, COLOR_BLACK, 1);
}

/**
 * Draw text on LCD (simplified - uses simple character positioning)
 */
void lcd_draw_text(uint16_t x, uint16_t y, const char* text, 
                   uint16_t fg_color, uint16_t bg_color, uint8_t size) {
    if (text == NULL) return;
    
    // Simple text drawing - character by character
    // Each character is approximately 5 pixels wide (size 1)
    uint16_t px = x;
    uint16_t py = y;
    
    for (const char* p = text; *p; p++) {
        // Draw character box as placeholder
        // In real implementation, would use bitmap font
        if (*p == ' ') {
            px += 5 * size;
        } else {
            // Draw simple character background
            lcd_fill_rect(px, py, 5 * size, 8 * size, bg_color);
            
            // TODO: Draw character bitmap
            // For now, just advance position
            px += 5 * size;
        }
    }
}

/**
 * Draw button on LCD
 */
void lcd_draw_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     const char* label, uint16_t bg_color, uint16_t fg_color) {
    // Draw button background
    lcd_fill_rect(x, y, w, h, bg_color);
    
    // Draw button border
    lcd_draw_hline(x, y, w, COLOR_WHITE);
    lcd_draw_hline(x, y + h - 1, w, COLOR_DARK_GRAY);
    lcd_draw_vline(x, y, h, COLOR_WHITE);
    lcd_draw_vline(x + w - 1, y, h, COLOR_DARK_GRAY);
    
    // TODO: Draw label
}

/**
 * Update display with player state
 */
void lcd_update(const player_t* player, uint32_t position) {
    if (!lcd_state.initialized) return;
    
    // Update song info if changed
    if (player->is_playing) {
        lcd_display_song_info(player->current_file, "Unknown Artist", 
                             180, position);  // Assume 3 min song
    } else if (player->is_paused) {
        lcd_display_status("PAUSED");
    } else {
        lcd_display_status("STOPPED");
    }
}

/**
 * Display volume level as bar
 */
void lcd_display_volume(uint8_t volume) {
    // Draw volume bar at top
    uint16_t bar_width = (volume * (LCD_WIDTH - 20)) / 100;
    lcd_fill_rect(10, 5, LCD_WIDTH - 20, 10, COLOR_DARK_GRAY);
    if (bar_width > 0) {
        lcd_fill_rect(10, 5, bar_width, 10, COLOR_GREEN);
    }
}
