#include <lcd_screen/lcd_screen.h>

void lcd_str(char* str, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y){
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString(str, Font, color);
	ssd1306_UpdateScreen();
}

void lcd_char(char ch, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y){
	ssd1306_SetCursor(x, y);
	ssd1306_WriteChar(ch, Font, color);
	ssd1306_UpdateScreen();
}
