#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f4xx_hal.h"

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 32
#define SSD1306_PAGES  (SSD1306_HEIGHT / 8)

HAL_StatusTypeDef ssd1306_init(I2C_HandleTypeDef *hi2c);
void              ssd1306_clear(void);
/* 2x scale: each glyph is 12 columns wide and 2 pages (16 rows) tall. */
void              ssd1306_draw_text(uint8_t page, uint8_t col, const char *s);
HAL_StatusTypeDef ssd1306_flush(void);
HAL_StatusTypeDef ssd1306_display_on(void);
HAL_StatusTypeDef ssd1306_display_off(void);

#endif
