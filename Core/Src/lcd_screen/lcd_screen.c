#include <lcd_screen/lcd_screen.h>

void lcd_str(const char* str, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y){
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString(str, Font, color);
	ssd1306_UpdateScreen();
}

void lcd_char(char ch, FontDef Font, SSD1306_COLOR color, uint8_t x, uint8_t y){
	ssd1306_SetCursor(x, y);
	ssd1306_WriteChar(ch, Font, color);
	ssd1306_UpdateScreen();
}

void lcd_center_BigStr(const char* text){
	// 1. Definisikan ukuran layar dan font
	    uint8_t screen_width = 128;
	    uint8_t screen_height = 64;
	    uint8_t font_width = 16;  // Sesuaikan jika kamu pakai Font_7x10
	    uint8_t font_height = 26; // Sesuaikan jika kamu pakai Font_7x10

	    // 2. Hitung panjang teks (jumlah karakter)
	    uint8_t text_length = strlen(text);

	    // 3. Hitung total lebar pixel yang dipakai teks
	    uint8_t total_text_width = text_length * font_width;

	    // 4. Masukkan ke rumus (Sisa ruang dibagi 2)
	    // Gunakan pengecekan agar tidak minus jika teks kepanjangan
	    uint8_t x_pos = 0;
	    if (screen_width > total_text_width) {
	        x_pos = (screen_width - total_text_width) / 2;
	    }

	    uint8_t y_pos = (screen_height - font_height) / 2;

	    // 5. Tampilkan ke layar
	    lcd_str(text, Font_16x26, White, x_pos, y_pos);
}

void lcd_center_BigChar(const char ch){
	// 1. Definisikan ukuran layar dan font
	    uint8_t screen_width = 128;
	    uint8_t screen_height = 64;
	    uint8_t font_width = 16;  // Sesuaikan jika kamu pakai Font_7x10
	    uint8_t font_height = 26; // Sesuaikan jika kamu pakai Font_7x10

	    // 2. Hitung panjang teks (jumlah karakter)
	    uint8_t text_length = 1;

	    // 3. Hitung total lebar pixel yang dipakai teks
	    uint8_t total_text_width = text_length * font_width;

	    // 4. Masukkan ke rumus (Sisa ruang dibagi 2)
	    // Gunakan pengecekan agar tidak minus jika teks kepanjangan
	    uint8_t x_pos = 0;
	    if (screen_width > total_text_width) {
	        x_pos = (screen_width - total_text_width) / 2;
	    }

	    uint8_t y_pos = (screen_height - font_height) / 2;

	    // 5. Tampilkan ke layar
	    lcd_char(ch, Font_16x26, White, x_pos, y_pos);
}
