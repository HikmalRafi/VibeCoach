/**
 * @file    app_splash.c
 * @brief   Implementasi splash screen untuk AI-Motion Correction device.
 *
 * @details Semua fungsi di sini bersifat BLOCKING (pakai HAL_Delay).
 *          Dipanggil hanya 1x saat boot/wake, bukan di dalam loop.
 */

#include "../../App/Inc/app_splash.h"
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"

/* =========================================================================
 * WARNA LOKAL (konsisten dengan app_menu.c)
 * ========================================================================= */
#define SP_COLOR_BG       ST7735_COLOR565(10,  20,  46)
#define SP_COLOR_TEXT     ST7735_COLOR565(220, 220, 220)
#define SP_COLOR_ACCENT   ST7735_COLOR565(74,  158, 255)
#define SP_COLOR_GREEN    ST7735_COLOR565(74,  222, 128)
#define SP_COLOR_DIM      ST7735_COLOR565(40,  70,  110)
#define SP_COLOR_HDR      ST7735_COLOR565(15,  52,  96)

/* =========================================================================
 * HELPER INTERNAL
 * ========================================================================= */

/**
 * @brief Gambar loading bar dengan progress 0-100%.
 *
 * @param x       Posisi X kiri bar
 * @param y       Posisi Y bar
 * @param width   Lebar total bar dalam pixel
 * @param height  Tinggi bar dalam pixel
 * @param pct     Persentase isi (0-100)
 */
static void splash_draw_progress(uint16_t x, uint16_t y,
                                  uint16_t width, uint16_t height,
                                  uint8_t pct)
{
    /* Outline bar */
    ST7735_FillRectangle(x, y, width, height, SP_COLOR_DIM);

    /* Inner fill berdasarkan persentase */
    uint16_t fill_w = (uint16_t)((width - 2) * pct / 100);
    if (fill_w > 0)
    {
        ST7735_FillRectangle(x + 1, y + 1, fill_w, height - 2, SP_COLOR_ACCENT);
    }
}

/**
 * @brief Animasikan loading bar dari pct_start ke pct_end.
 *
 * @param x, y, w, h  Posisi dan ukuran bar
 * @param pct_start   Persentase awal
 * @param pct_end     Persentase akhir
 * @param duration_ms Total durasi animasi dalam milidetik
 */
static void splash_animate_progress(uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h,
                                     uint8_t pct_start, uint8_t pct_end,
                                     uint32_t duration_ms)
{
    uint8_t  range   = pct_end - pct_start;
    uint32_t steps   = range;          /* 1 step per persen */
    uint32_t delay   = duration_ms / steps;

    for (uint8_t p = pct_start; p <= pct_end; p++)
    {
        splash_draw_progress(x, y, w, h, p);
        HAL_Delay(delay);
    }
}

/* =========================================================================
 * IMPLEMENTASI FUNGSI PUBLIK
 * ========================================================================= */

void app_splash_show_boot(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /* --- Bersihkan layar --- */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /* --- Garis dekoratif atas (aksen warna) --- */
    ST7735_FillRectangle(0, 0, lcd_w, 3, SP_COLOR_ACCENT);

    /* --- Logo area: blok warna di tengah atas --- */
    uint16_t logo_y = 12;
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y, 40, 30, SP_COLOR_HDR);
    /* Border tipis logo */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y,      40,  1, SP_COLOR_ACCENT); /* top    */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y + 29, 40,  1, SP_COLOR_ACCENT); /* bottom */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y,       1, 30, SP_COLOR_ACCENT); /* left   */
    ST7735_FillRectangle(lcd_w / 2 + 19, logo_y,       1, 30, SP_COLOR_ACCENT); /* right  */

    /* Teks "AI" di tengah kotak logo */
    ST7735_WriteString(lcd_w / 2 - 12, logo_y + 10, "AI", Font_11x18,
                       SP_COLOR_GREEN, SP_COLOR_HDR);

    HAL_Delay(200); /* Jeda singkat setelah logo muncul */

    /* --- Nama alat --- */
    uint16_t name_y = logo_y + 36;
    ST7735_WriteString(4, name_y, SPLASH_DEVICE_NAME, Font_11x18,
                       SP_COLOR_TEXT, SP_COLOR_BG);

    /* --- Subtitle --- */
    uint16_t sub_y = name_y + 22;
    ST7735_WriteString(4, sub_y, SPLASH_SUBTITLE, Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /* --- Separator tipis --- */
    uint16_t sep_y = sub_y + 14;
    ST7735_FillRectangle(4, sep_y, lcd_w - 8, 1, SP_COLOR_DIM);

    HAL_Delay(300); /* Jeda sebelum loading bar muncul */

    /* --- Loading bar animasi --- */
    uint16_t bar_x = 10;
    uint16_t bar_y = sep_y + 6;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 6;

    /* Gambar bar kosong dulu */
    splash_draw_progress(bar_x, bar_y, bar_w, bar_h, 0);

    /* Teks "Loading..." di bawah bar */
    ST7735_WriteString(bar_x, bar_y + 10, "Starting...", Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /* Animasi loading bar 0% -> 100% */
    splash_animate_progress(bar_x, bar_y, bar_w, bar_h,
                             0, 100, SPLASH_DURATION_MS);

    /* --- Garis dekoratif bawah --- */
    ST7735_FillRectangle(0, lcd_h - 3, lcd_w, 3, SP_COLOR_ACCENT);

    HAL_Delay(200); /* Jeda kecil sebelum masuk menu */

    /* Clear layar sebelum menu digambar */
    ST7735_FillScreenFast(SP_COLOR_BG);
}

void app_splash_show_wake(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /* --- Bersihkan layar --- */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /* --- Garis dekoratif atas --- */
    ST7735_FillRectangle(0, 0, lcd_w, 3, SP_COLOR_GREEN);

    /* --- Teks selamat datang singkat --- */
    uint16_t center_y = lcd_h / 2 - 15;
    ST7735_WriteString(4, center_y, SPLASH_DEVICE_NAME, Font_11x18,
                       SP_COLOR_TEXT, SP_COLOR_BG);

    ST7735_WriteString(4, center_y + 22, "Welcome back!", Font_7x10,
                       SP_COLOR_GREEN, SP_COLOR_BG);

    /* --- Loading bar singkat --- */
    uint16_t bar_x = 10;
    uint16_t bar_y = center_y + 40;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 5;

    splash_animate_progress(bar_x, bar_y, bar_w, bar_h,
                             0, 100, SPLASH_WAKE_DURATION_MS);

    /* Clear layar sebelum menu digambar */
    ST7735_FillScreenFast(SP_COLOR_BG);
}

void app_splash_show_shutdown(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /* --- Background gelap untuk efek shutdown --- */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /* --- Teks shutdown di tengah --- */
    uint16_t center_y = lcd_h / 2 - 15;
    ST7735_WriteString(4, center_y, "Shutting down", Font_7x10,
                       ST7735_COLOR565(239, 68, 68), SP_COLOR_BG);

    ST7735_WriteString(4, center_y + 14, "Please wait...", Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /* --- Progress bar mengecil (efek shutdown) --- */
    uint16_t bar_x = 10;
    uint16_t bar_y = center_y + 34;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 5;

    /* Animasi bar dari 100% -> 0% */
    for (int8_t p = 100; p >= 0; p -= 2)
    {
        splash_draw_progress(bar_x, bar_y, bar_w, bar_h, (uint8_t)p);
        HAL_Delay(10);
    }

    /* Layar hitam total sebelum STANDBY */
    HAL_Delay(200);
    ST7735_FillScreenFast(0x0000);
}
