/**
 * @file ui_enhancement.c
 * @brief Implementasi UI Enhancement
 */

#include "system_ui/ui_enhancement.h"
#include <stdlib.h>

// =====================================================================
// STATIC VARIABLES
// =====================================================================
static uint32_t toast_end_time = 0;
static char toast_message[32] = {0};
static uint32_t last_pulse_time = 0;
static uint8_t pulse_phase = 0;

// =====================================================================
// IMPLEMENTASI
// =====================================================================

void UI_DrawStatusBar(uint8_t battery_percent, const char* time_str) {
    // Background status bar
    ST7735_FillRectangle(0, 0, ST7735_WIDTH, 12, UI_COLOR_BG_DARK);

    // Icon baterai sederhana (ASCII)
    ST7735_WriteString(2, 2, "[", Font_7x10, UI_COLOR_ACCENT, UI_COLOR_BG_DARK);

    // Bar baterai (simulasi persen)
    uint8_t bar_count = (battery_percent > 100) ? 8 : (battery_percent / 12);
    char battery_bar[12] = "";
    for (uint8_t i = 0; i < bar_count; i++) {
        strcat(battery_bar, "|");
    }
    ST7735_WriteString(4, 2, battery_bar, Font_7x10,
                       (battery_percent > 20) ? UI_COLOR_SUCCESS : UI_COLOR_WARNING,
                       UI_COLOR_BG_DARK);
    ST7735_WriteString(4 + (bar_count * 4), 2, "]", Font_7x10, UI_COLOR_ACCENT, UI_COLOR_BG_DARK);

    // Waktu (kanan)
    uint16_t time_x = ST7735_WIDTH - (strlen(time_str) * 7) - 2;
    ST7735_WriteString(time_x, 2, time_str, Font_7x10, UI_COLOR_ACCENT, UI_COLOR_BG_DARK);
}

void UI_DrawHeader(const char* title, char icon_char) {
    // Background header (tebal 18px, warna card)
    ST7735_FillRectangle(0, 14, ST7735_WIDTH, 18, UI_COLOR_CARD);

    // Border bawah
    ST7735_FillRectangle(0, 31, ST7735_WIDTH, 1, UI_COLOR_ACCENT);

    // Icon (lingkaran kecil + karakter)
    ST7735_DrawPixel(8, 20, UI_COLOR_ACCENT);
    ST7735_DrawPixel(8, 21, UI_COLOR_ACCENT);
    ST7735_DrawPixel(8, 22, UI_COLOR_ACCENT);
    ST7735_DrawPixel(7, 21, UI_COLOR_ACCENT);
    ST7735_DrawPixel(9, 21, UI_COLOR_ACCENT);

    // Karakter icon di tengah lingkaran
    char icon_str[2] = {icon_char, '\0'};
    ST7735_WriteString(6, 19, icon_str, Font_7x10, UI_COLOR_ACCENT, UI_COLOR_CARD);

    // Title (indent 20px dari kiri)
    ST7735_WriteString(20, 18, title, Font_11x18, UI_COLOR_ACCENT, UI_COLOR_CARD);
}

void UI_DrawMenuItem(uint16_t x, uint16_t y, const char* text, uint8_t is_selected) {
    uint16_t bg_color = is_selected ? UI_COLOR_ACCENT : UI_COLOR_BG_DARK;
    uint16_t text_color = is_selected ? UI_COLOR_BG_DARK : UI_COLOR_ACCENT;

    // Card background
    ST7735_FillRectangle(x, y, ST7735_WIDTH - (x*2), 14, bg_color);

    // Border tipis jika selected
    if (is_selected) {
        ST7735_FillRectangle(x, y, ST7735_WIDTH - (x*2), 1, UI_COLOR_SUCCESS);
        ST7735_FillRectangle(x, y + 13, ST7735_WIDTH - (x*2), 1, UI_COLOR_SUCCESS);
    }

    // Icon panah kanan jika selected
    if (is_selected) {
        ST7735_WriteString(x + 4, y + 2, ">", Font_7x10, text_color, bg_color);
        ST7735_WriteString(x + 8, y + 2, text, Font_7x10, text_color, bg_color);
    } else {
        ST7735_WriteString(x + 4, y + 2, text, Font_7x10, text_color, bg_color);
    }
}

void UI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percent, uint16_t color) {
    if (percent > 100) percent = 100;

    uint16_t fill_width = (width * percent) / 100;

    // Background (abu-abu gelap)
    ST7735_FillRectangle(x, y, width, height, UI_COLOR_CARD);

    // Fill
    if (fill_width > 0) {
        ST7735_FillRectangle(x, y, fill_width, height, color);
    }

    // Border
    ST7735_FillRectangle(x, y, width, 1, UI_COLOR_BORDER);
    ST7735_FillRectangle(x, y + height - 1, width, 1, UI_COLOR_BORDER);
    ST7735_FillRectangle(x, y, 1, height, UI_COLOR_BORDER);
    ST7735_FillRectangle(x + width - 1, y, 1, height, UI_COLOR_BORDER);
}

void UI_DrawTrainingScreen(
    const char* exercise_name,
    uint16_t rep_current,
    uint16_t rep_target,
    uint8_t confidence,
    float quat_w, float quat_x, float quat_y, float quat_z,
    FeedbackLevel feedback_level
) {
    // ===== BACKGROUND COLOR BERUBAH BERDASARKAN FEEDBACK =====
    uint16_t bg_color;
    switch (feedback_level) {
        case FEEDBACK_DANGER:
            bg_color = ST7735_COLOR565(40, 0, 0);  // Merah gelap
            break;
        case FEEDBACK_WARNING:
            bg_color = ST7735_COLOR565(40, 30, 0); // Oranye gelap
            break;
        case FEEDBACK_SUCCESS:
            bg_color = ST7735_COLOR565(0, 30, 0);  // Hijau gelap
            break;
        default:
            bg_color = UI_COLOR_BG_DARK;
            break;
    }
    ST7735_FillScreenFast(bg_color);

    // ===== HEADER =====
    UI_DrawHeader(exercise_name, (exercise_name[0] == 'D') ? 'D' : 'P');

    // ===== BADGE "RECORDING" (kiri) & TIMER (kanan) =====
    ST7735_WriteString(4, 36, "RECORDING", Font_7x10, UI_COLOR_ERROR, bg_color);
    // Timer (simulasi - nanti bisa diganti actual timer)
    ST7735_WriteString(80, 36, "00:32", Font_7x10, UI_COLOR_ACCENT, bg_color);

    // ===== QUATERNION DISPLAY (3 angka) =====
    char quat_buf[32];
    sprintf(quat_buf, "X:%.2f Y:%.2f Z:%.2f", quat_x, quat_y, quat_z);
    ST7735_WriteString(4, 50, quat_buf, Font_7x10, UI_COLOR_ACCENT, bg_color);

    // ===== CARD: REPS (di kiri) =====
    ST7735_FillRectangle(4, 65, 55, 35, UI_COLOR_CARD);
    ST7735_WriteString(8, 70, "REPS", Font_7x10, UI_COLOR_ACCENT, UI_COLOR_CARD);
    sprintf(quat_buf, "%d/%d", rep_current, rep_target);
    ST7735_WriteString(8, 82, quat_buf, Font_11x18, UI_COLOR_SUCCESS, UI_COLOR_CARD);

    // ===== CARD: CONFIDENCE (di kanan) =====
    ST7735_FillRectangle(68, 65, 55, 35, UI_COLOR_CARD);
    ST7735_WriteString(72, 70, "CONF", Font_7x10, UI_COLOR_ACCENT, UI_COLOR_CARD);
    sprintf(quat_buf, "%d%%", confidence);
    ST7735_WriteString(72, 82, quat_buf, Font_11x18, UI_COLOR_ACCENT, UI_COLOR_CARD);

    // ===== PROGRESS BAR (lebar penuh) =====
    UI_DrawProgressBar(4, 108, ST7735_WIDTH - 8, 8, confidence,
                       (confidence > 70) ? UI_COLOR_SUCCESS :
                       (confidence > 40) ? UI_COLOR_WARNING : UI_COLOR_ERROR);

    // ===== INSTRUKSI BAWAH =====
    if (feedback_level == FEEDBACK_DANGER) {
        ST7735_WriteString(4, 125, "> WRONG POSE! SLOW DOWN <", Font_7x10, UI_COLOR_ERROR, bg_color);
    } else if (feedback_level == FEEDBACK_WARNING) {
        ST7735_WriteString(4, 125, "> CHECK YOUR TEMPO <", Font_7x10, UI_COLOR_WARNING, bg_color);
    } else if (feedback_level == FEEDBACK_SUCCESS) {
        ST7735_WriteString(4, 125, "> GOOD FORM! KEEP GOING <", Font_7x10, UI_COLOR_SUCCESS, bg_color);
    }

    ST7735_WriteString(4, 140, "STOP TO EXIT", Font_7x10, UI_COLOR_ACCENT, bg_color);
}

void UI_ShowToast(const char* message, uint32_t duration_ms) {
    strncpy(toast_message, message, 31);
    toast_message[31] = '\0';
    toast_end_time = HAL_GetTick() + duration_ms;

    // Gambar toast (persegi panjang di tengah layar)
    uint16_t text_width = strlen(message) * 7;
    uint16_t box_x = (ST7735_WIDTH - text_width - 16) / 2;
    uint16_t box_y = 60;

    ST7735_FillRectangle(box_x - 2, box_y - 2, text_width + 20, 20, UI_COLOR_CARD);
    ST7735_FillRectangle(box_x - 1, box_y - 1, text_width + 18, 18, UI_COLOR_ACCENT);
    ST7735_WriteString(box_x, box_y, message, Font_7x10, UI_COLOR_BG_DARK, UI_COLOR_ACCENT);
}

void UI_PulseLine(uint16_t target_y, uint16_t color) {
    uint32_t now = HAL_GetTick();
    if (now - last_pulse_time > 100) {
        last_pulse_time = now;
        pulse_phase = !pulse_phase;

        if (pulse_phase) {
            ST7735_FillRectangle(0, target_y, ST7735_WIDTH, 2, color);
        } else {
            ST7735_FillRectangle(0, target_y, ST7735_WIDTH, 2, UI_COLOR_BG_DARK);
        }
    }
}
