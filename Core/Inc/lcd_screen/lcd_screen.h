#include <lcd_screen/ssd1306.h>
#include "main.h"

#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

void lcd_str(const char* str, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y);
void lcd_char(const char ch, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y);
void lcd_center_BigStr(const char* text);
void lcd_center_BigChar(const char ch);

#endif
