/**
 * @file    app_splash.c
 * @brief   Splash screen implementation for AI-Motion Correction device.
 *
 * @details All functions in this module are BLOCKING (using HAL_Delay).
 *          Called only once during boot/wake, not inside the main loop.
 */

#include "../../App/Inc/app_splash.h"
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"

/* =========================================================================
 * LOCAL COLORS (consistent with app_menu.c)
 * ========================================================================= */
#define SP_COLOR_BG       ST7735_COLOR565(10,  20,  46)   /**< Dark navy blue background */
#define SP_COLOR_TEXT     ST7735_COLOR565(220, 220, 220)  /**< Light gray text */
#define SP_COLOR_ACCENT   ST7735_COLOR565(74,  158, 255)  /**< Blue accent */
#define SP_COLOR_GREEN    ST7735_COLOR565(74,  222, 128)  /**< Green (safe/active) */
#define SP_COLOR_DIM      ST7735_COLOR565(40,  70,  110)  /**< Dimmed color for secondary elements */
#define SP_COLOR_HDR      ST7735_COLOR565(15,  52,  96)   /**< Header/dark panel color */

/* =========================================================================
 * INTERNAL HELPER FUNCTIONS
 * ========================================================================= */

/**
 * @brief Draw a loading bar with 0-100% progress.
 *
 * Renders a progress bar with a dimmed outline and an accent-colored fill
 * proportional to the given percentage.
 *
 * @param[in] x       Left X position of the bar
 * @param[in] y       Top Y position of the bar
 * @param[in] width   Total bar width in pixels
 * @param[in] height  Bar height in pixels
 * @param[in] pct     Fill percentage (0-100)
 */
static void splash_draw_progress(uint16_t x, uint16_t y,
                                  uint16_t width, uint16_t height,
                                  uint8_t pct)
{
    /** Draw bar outline (dimmed border) */
    ST7735_FillRectangle(x, y, width, height, SP_COLOR_DIM);

    /** Calculate and draw inner fill based on percentage */
    uint16_t fill_w = (uint16_t)((width - 2) * pct / 100);
    if (fill_w > 0)
    {
        ST7735_FillRectangle(x + 1, y + 1, fill_w, height - 2, SP_COLOR_ACCENT);
    }
}

/**
 * @brief Animate the loading bar from pct_start to pct_end.
 *
 * Smoothly fills or empties the progress bar over a specified duration.
 * Updates at 1 step per percentage point.
 *
 * @param[in] x, y, w, h  Bar position and dimensions
 * @param[in] pct_start   Starting percentage
 * @param[in] pct_end     Ending percentage
 * @param[in] duration_ms Total animation duration in milliseconds
 */
static void splash_animate_progress(uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h,
                                     uint8_t pct_start, uint8_t pct_end,
                                     uint32_t duration_ms)
{
    uint8_t  range   = pct_end - pct_start;
    uint32_t steps   = range;          /**< 1 step per percentage point */
    uint32_t delay   = duration_ms / steps;

    for (uint8_t p = pct_start; p <= pct_end; p++)
    {
        splash_draw_progress(x, y, w, h, p);
        HAL_Delay(delay);
    }
}

/* =========================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * ========================================================================= */

/**
 * @brief Display the full boot splash screen sequence.
 *
 * Shows a complete startup animation including:
 * - Accent-colored top decorative line
 * - "AI" logo box in the upper center area
 * - Device name (SPLASH_DEVICE_NAME)
 * - Subtitle/tagline (SPLASH_SUBTITLE)
 * - Thin separator line
 * - Animated loading bar (0% → 100%)
 * - Accent-colored bottom decorative line
 *
 * Total duration: SPLASH_DURATION_MS (default 2500ms)
 *
 * @note This function blocks for the entire splash duration.
 *       Call once at system startup before entering the main menu loop.
 */
void app_splash_show_boot(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /** Clear screen to background color */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /** Top decorative line (accent color) */
    ST7735_FillRectangle(0, 0, lcd_w, 3, SP_COLOR_ACCENT);

    /** Logo area: colored block in upper center */
    uint16_t logo_y = 12;
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y, 40, 30, SP_COLOR_HDR);
    /** Logo thin border */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y,      40,  1, SP_COLOR_ACCENT); /**< Top border */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y + 29, 40,  1, SP_COLOR_ACCENT); /**< Bottom border */
    ST7735_FillRectangle(lcd_w / 2 - 20, logo_y,       1, 30, SP_COLOR_ACCENT); /**< Left border */
    ST7735_FillRectangle(lcd_w / 2 + 19, logo_y,       1, 30, SP_COLOR_ACCENT); /**< Right border */

    /** "AI" text centered inside the logo box */
    ST7735_WriteString(lcd_w / 2 - 12, logo_y + 10, "AI", Font_11x18,
                       SP_COLOR_GREEN, SP_COLOR_HDR);

    HAL_Delay(200); /**< Brief pause after logo appears */

    /** Device name display */
    uint16_t name_y = logo_y + 36;
    ST7735_WriteString(4, name_y, SPLASH_DEVICE_NAME, Font_11x18,
                       SP_COLOR_TEXT, SP_COLOR_BG);

    /** Subtitle / tagline display */
    uint16_t sub_y = name_y + 22;
    ST7735_WriteString(4, sub_y, SPLASH_SUBTITLE, Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /** Thin separator line */
    uint16_t sep_y = sub_y + 14;
    ST7735_FillRectangle(4, sep_y, lcd_w - 8, 1, SP_COLOR_DIM);

    HAL_Delay(300); /**< Brief pause before loading bar appears */

    /** Loading bar animation setup */
    uint16_t bar_x = 10;
    uint16_t bar_y = sep_y + 6;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 6;

    /** Draw empty bar frame first */
    splash_draw_progress(bar_x, bar_y, bar_w, bar_h, 0);

    /** "Starting..." text below the bar */
    ST7735_WriteString(bar_x, bar_y + 10, "Starting...", Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /** Animate loading bar from 0% to 100% */
    splash_animate_progress(bar_x, bar_y, bar_w, bar_h,
                             0, 100, SPLASH_DURATION_MS);

    /** Bottom decorative line (accent color) */
    ST7735_FillRectangle(0, lcd_h - 3, lcd_w, 3, SP_COLOR_ACCENT);

    HAL_Delay(200); /**< Brief pause before entering menu */

    /** Clear screen before menu rendering */
    ST7735_FillScreenFast(SP_COLOR_BG);
}

/**
 * @brief Display a short wake-from-STANDBY splash screen.
 *
 * Shows a compact wake animation including:
 * - Green top decorative line (indicates successful wake)
 * - Device name centered
 * - "Welcome back!" greeting
 * - Short animated loading bar (0% → 100%)
 *
 * Total duration: SPLASH_WAKE_DURATION_MS (default 1200ms)
 *
 * @note This function blocks for the entire splash duration.
 *       Called once when waking from STANDBY mode.
 */
void app_splash_show_wake(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /** Clear screen to background color */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /** Top decorative line (green = wake indicator) */
    ST7735_FillRectangle(0, 0, lcd_w, 3, SP_COLOR_GREEN);

    /** Welcome text centered vertically */
    uint16_t center_y = lcd_h / 2 - 15;
    ST7735_WriteString(4, center_y, SPLASH_DEVICE_NAME, Font_11x18,
                       SP_COLOR_TEXT, SP_COLOR_BG);

    ST7735_WriteString(4, center_y + 22, "Welcome back!", Font_7x10,
                       SP_COLOR_GREEN, SP_COLOR_BG);

    /** Short loading bar animation */
    uint16_t bar_x = 10;
    uint16_t bar_y = center_y + 40;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 5;

    splash_animate_progress(bar_x, bar_y, bar_w, bar_h,
                             0, 100, SPLASH_WAKE_DURATION_MS);

    /** Clear screen before menu rendering */
    ST7735_FillScreenFast(SP_COLOR_BG);
}

/**
 * @brief Display a shutdown animation before entering STANDBY mode.
 *
 * Shows a power-off sequence including:
 * - Dark background for shutdown effect
 * - "Shutting down" text in red (centered)
 * - "Please wait..." subtitle in dimmed color
 * - Shrinking progress bar animation (100% → 0%)
 * - Final black screen before STANDBY entry
 *
 * @note This function blocks for the entire animation duration.
 *       Called from bsp_power_on_shutdown_callback() before STANDBY entry.
 */
void app_splash_show_shutdown(void)
{
    uint16_t lcd_w = ST7735_WIDTH;
    uint16_t lcd_h = ST7735_HEIGHT;

    /** Dark background for shutdown effect */
    ST7735_FillScreenFast(SP_COLOR_BG);

    /** Shutdown text centered on screen */
    uint16_t center_y = lcd_h / 2 - 15;
    ST7735_WriteString(4, center_y, "Shutting down", Font_7x10,
                       ST7735_COLOR565(239, 68, 68), SP_COLOR_BG); /**< Red text for shutdown */

    ST7735_WriteString(4, center_y + 14, "Please wait...", Font_7x10,
                       SP_COLOR_DIM, SP_COLOR_BG);

    /** Shrinking progress bar (reverse animation for shutdown effect) */
    uint16_t bar_x = 10;
    uint16_t bar_y = center_y + 34;
    uint16_t bar_w = lcd_w - 20;
    uint16_t bar_h = 5;

    /** Animate bar from 100% down to 0% */
    for (int8_t p = 100; p >= 0; p -= 2)
    {
        splash_draw_progress(bar_x, bar_y, bar_w, bar_h, (uint8_t)p);
        HAL_Delay(10);
    }

    /** Complete black screen before STANDBY mode entry */
    HAL_Delay(200);
    ST7735_FillScreenFast(0x0000);
}
