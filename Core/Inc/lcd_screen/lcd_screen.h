#include <lcd_screen/ssd1306.h>
#include "main.h"

#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

void lcd_str(char* str, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y);
void lcd_char(char ch, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y);


#endif
