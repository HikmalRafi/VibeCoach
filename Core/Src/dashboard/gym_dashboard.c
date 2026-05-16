/**
 * @file gym_dashboard.c
 * @brief Implementasi Professional Gym Assistant Dashboard
 *
 * DESIGN PHILOSOPHY:
 * - Flipper Zero style: Clean borders, monospace fonts, icon-based indicators
 * - Real-time: Render partial updates (tidak clear screen penuh)
 * - Informative: Multi-indicator system untuk berbagai jenis kesalahan
 * - Non-blocking: Semua fungsi return immediately, tidak ada while loop
 *
 * LAYOUT LAYAR (128x160 pixels):
 * ┌─────────────────────────────┐  y=0
 * │  🏋 GYM ASSISTANT          │  y=0-17  (Header)
 * ├─────────────────────────────┤  y=18
 * │  STATUS: ● PERFECT         │  y=20-29 (Status bar)
 * ├─────────────────────────────┤  y=32
 * │  ACCURACY  ████████░░ 85%  │  y=35-45 (Progress bars)
 * │  TEMPO     ██████░░░░ 72%  │  y=48-58
 * │  ROM       ████████░░ 88%  │  y=61-71
 * ├─────────────────────────────┤  y=74
 * │  ⚠ WARNINGS (if any)      │  y=77-87 (Warning area)
 * │  ⚠ Wrong Pose Detected!   │
 * ├─────────────────────────────┤  y=90
 * │  REPS: 012  |  ⏱ 01:23    │  y=93-103 (Stats footer)
 * └─────────────────────────────┘  y=106-128
 */

#include "dashboard/gym_dashboard.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================
 * PRIVATE CONSTANTS - UI Layout & Colors
 * ======================================================================== */

// Layout coordinates (calculated for 128x160 display)
#define HEADER_Y        0
#define HEADER_HEIGHT   18
#define STATUS_Y        20
#define PROGRESS_START_Y 35
#define PROGRESS_SPACING 13
#define WARNING_Y       77
#define FOOTER_Y        95
#define FOOTER_HEIGHT   15

// Warna Flipper Zero style (muted, professional)
#define COLOR_BG        0x0000  // Pure black background
#define COLOR_HEADER_BG 0x18E3  // Dark navy
#define COLOR_BORDER    0x7BEF  // Grey-blue border
#define COLOR_WHITE     0xFFFF
#define COLOR_GREEN     0x07E0  // Perfect/Success
#define COLOR_YELLOW    0xFFE0  // Warning
#define COLOR_RED       0xF800  // Danger
#define COLOR_CYAN      0x07FF  // Info
#define COLOR_ORANGE    0xFD20  // Attention
#define COLOR_GREY      0x8410  // Inactive

// Progress bar colors
#define COLOR_BAR_BG    0x39E7  // Dark grey bar background
#define COLOR_BAR_ACC   0x07E0  // Green bar (Accuracy)
#define COLOR_BAR_TEMPO 0x07FF  // Cyan bar (Tempo)
#define COLOR_BAR_ROM   0xFD20  // Orange bar (Range of Motion)

// Warning blink timing (ms)
#define WARNING_BLINK_MS 500

/* ========================================================================
 * PRIVATE VARIABLES
 * ======================================================================== */

static bool initialized = false;
static MovementStatus last_status = MOVE_IDLE;
static uint32_t last_render_tick = 0;
static uint32_t warning_toggle_tick = 0;
static bool warning_visible = true;

// Saved coordinates untuk partial update optimization
static uint8_t last_rep_count = 255;  // Force initial render

/* ========================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ======================================================================== */

static void draw_header(void);
static void draw_borders(void);
static void draw_status_bar(MovementStatus status);
static void draw_progress_bar(uint8_t y, const char* label, float value, uint16_t color);
static void draw_warnings(MovementStatus status);
static void draw_footer(uint8_t reps, uint32_t elapsed_seconds);
static const char* get_status_icon(MovementStatus status);
static uint16_t get_status_color(MovementStatus status);
static const char* get_status_text(MovementStatus status);
static void draw_char_at(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg);

/* ========================================================================
 * PUBLIC FUNCTIONS
 * ======================================================================== */

void GymDashboard_Init(void) {
    ST7735_FillScreenFast(COLOR_BG);
    draw_borders();
    draw_header();
    initialized = true;
    last_render_tick = HAL_GetTick();
}

void GymDashboard_Render(MovementStatus status,
                         float accuracy_score,
                         float tempo_score,
                         float range_of_motion,
                         uint8_t rep_count) {

    if (!initialized) {
        GymDashboard_Init();
    }

    // Safety clamp values
    if (accuracy_score > 1.0f) accuracy_score = 1.0f;
    if (accuracy_score < 0.0f) accuracy_score = 0.0f;
    if (tempo_score > 1.0f) tempo_score = 1.0f;
    if (tempo_score < 0.0f) tempo_score = 0.0f;
    if (range_of_motion > 1.0f) range_of_motion = 1.0f;
    if (range_of_motion < 0.0f) range_of_motion = 0.0f;

    // Calculate elapsed time (dummy - bisa diganti timer real)
    static uint32_t start_time = 0;
    if (start_time == 0) start_time = HAL_GetTick();
    uint32_t elapsed = (HAL_GetTick() - start_time) / 1000;

    // === RENDER UI (partial updates untuk performance) ===

    // Status bar - selalu update jika berubah
    if (status != last_status) {
        draw_status_bar(status);
        last_status = status;
    }

    // Progress bars - update setiap 100ms untuk animasi smooth
    draw_progress_bar(PROGRESS_START_Y, "ACC", accuracy_score, COLOR_BAR_ACC);
    draw_progress_bar(PROGRESS_START_Y + PROGRESS_SPACING, "TMP", tempo_score, COLOR_BAR_TEMPO);
    draw_progress_bar(PROGRESS_START_Y + (PROGRESS_SPACING * 2), "ROM", range_of_motion, COLOR_BAR_ROM);

    // Warnings - blinking effect for attention
    if (status == MOVE_WRONG_POSE || status == MOVE_WRONG_TEMPO || status == MOVE_EXTREME) {
        if (HAL_GetTick() - warning_toggle_tick > WARNING_BLINK_MS) {
            warning_visible = !warning_visible;
            warning_toggle_tick = HAL_GetTick();
            draw_warnings(status);
        }
    } else {
        if (warning_visible != true) {
            warning_visible = true;
            draw_warnings(status);  // Clear warnings
        }
    }

    // Footer - update hanya jika rep count berubah
    if (rep_count != last_rep_count) {
        draw_footer(rep_count, elapsed);
        last_rep_count = rep_count;
    }

    last_render_tick = HAL_GetTick();
}

void GymDashboard_UpdateFeedback(void) {
    // Get current vibration level based on status
    VibrationLevel vibe;

    switch (last_status) {
        case MOVE_PERFECT:
            vibe = VIBE_OFF;
            break;
        case MOVE_GOOD:
            vibe = VIBE_OFF;
            break;
        case MOVE_WRONG_POSE:
            vibe = VIBE_WARNING;
            break;
        case MOVE_WRONG_TEMPO:
            vibe = VIBE_GENTLE;  // Gentle reminder for tempo
            break;
        case MOVE_EXTREME:
            vibe = VIBE_DANGER;
            break;
        case MOVE_IDLE:
        default:
            vibe = VIBE_OFF;
            break;
    }

    // Kontrol motor getar via PWM Timer
    // NOTE: Sesuaikan dengan channel timer Anda
    extern TIM_HandleTypeDef htim1;
    switch (vibe) {
        case VIBE_OFF:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            break;
        case VIBE_GENTLE:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 300);  // 30% duty
            break;
        case VIBE_WARNING:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 600);  // 60% duty
            break;
        case VIBE_DANGER:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 999);  // 100% duty
            break;
    }
}

MovementStatus GymDashboard_GetLastStatus(void) {
    return last_status;
}

/* ========================================================================
 * PRIVATE FUNCTIONS - UI Drawing
 * ======================================================================== */

static void draw_header(void) {
    // Background header
    ST7735_FillRectangle(0, HEADER_Y, ST7735_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);

    // Title dengan Font 11x18
    ST7735_WriteString(4, 2, "GYM ASST", Font_11x18, COLOR_CYAN, COLOR_HEADER_BG);

    // Decorative line
    ST7735_FillRectangle(0, HEADER_HEIGHT-1, ST7735_WIDTH, 1, COLOR_BORDER);
}

static void draw_borders(void) {
    // Border tipis di sekeliling layar (Flipper Zero style)
    // Top border
    ST7735_FillRectangle(0, 0, ST7735_WIDTH, 1, COLOR_BORDER);
    // Bottom border
    ST7735_FillRectangle(0, ST7735_HEIGHT-1, ST7735_WIDTH, 1, COLOR_BORDER);
    // Left border
    ST7735_FillRectangle(0, 0, 1, ST7735_HEIGHT, COLOR_BORDER);
    // Right border
    ST7735_FillRectangle(ST7735_WIDTH-1, 0, 1, ST7735_HEIGHT, COLOR_BORDER);
}

static void draw_status_bar(MovementStatus status) {
    // Clear area
    ST7735_FillRectangle(2, STATUS_Y, ST7735_WIDTH-4, 10, COLOR_BG);

    // Draw icon dan text
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s %s",
             get_status_icon(status),
             get_status_text(status));

    ST7735_WriteString(4, STATUS_Y, buffer, Font_7x10,
                       get_status_color(status), COLOR_BG);

    // Separator line
    ST7735_FillRectangle(2, STATUS_Y + 11, ST7735_WIDTH-4, 1, COLOR_BORDER);
}

static void draw_progress_bar(uint8_t y, const char* label, float value, uint16_t color) {
    char buffer[16];
    uint8_t bar_width = 80;  // Pixel width of bar
    uint8_t bar_x = 30;      // X start position
    uint8_t filled = (uint8_t)(value * bar_width);

    // Clear area (hanya area yang diperlukan - partial update)
    ST7735_FillRectangle(2, y, ST7735_WIDTH-4, 12, COLOR_BG);

    // Label
    ST7735_WriteString(4, y, label, Font_7x10, COLOR_WHITE, COLOR_BG);

    // Bar background
    ST7735_FillRectangle(bar_x, y+2, bar_width, 7, COLOR_BAR_BG);

    // Bar filled
    if (filled > 0) {
        ST7735_FillRectangle(bar_x, y+2, filled, 7, color);
    }

    // Percentage text
    snprintf(buffer, sizeof(buffer), "%3d%%", (int)(value * 100));
    ST7735_WriteString(bar_x + bar_width + 4, y, buffer, Font_7x10, color, COLOR_BG);
}

static void draw_warnings(MovementStatus status) {
    // Clear warning area
    ST7735_FillRectangle(2, WARNING_Y, ST7735_WIDTH-4, 14, COLOR_BG);

    // Separator line
    ST7735_FillRectangle(2, WARNING_Y, ST7735_WIDTH-4, 1, COLOR_BORDER);

    if (!warning_visible && status != MOVE_PERFECT && status != MOVE_GOOD) {
        return;  // Blink off phase
    }

    char warning_text[32];
    uint16_t warn_color = COLOR_YELLOW;

    // Tentukan pesan warning spesifik
    switch (status) {
        case MOVE_WRONG_POSE:
            snprintf(warning_text, sizeof(warning_text), "!! WRONG POSE - Check Form !!");
            warn_color = COLOR_RED;
            break;
        case MOVE_WRONG_TEMPO:
            snprintf(warning_text, sizeof(warning_text), "!! TEMPO - Too Fast/Slow !!");
            warn_color = COLOR_YELLOW;
            break;
        case MOVE_EXTREME:
            snprintf(warning_text, sizeof(warning_text), "!! DANGER - Reduce Range !!");
            warn_color = COLOR_ORANGE;
            break;
        case MOVE_PERFECT:
            snprintf(warning_text, sizeof(warning_text), "  Perfect Form! Keep it up!");
            warn_color = COLOR_GREEN;
            break;
        case MOVE_GOOD:
            snprintf(warning_text, sizeof(warning_text), "  Good movement");
            warn_color = COLOR_CYAN;
            break;
        default:
            return;
    }

    ST7735_WriteString(4, WARNING_Y + 1, warning_text, Font_7x10, warn_color, COLOR_BG);
}

static void draw_footer(uint8_t reps, uint32_t elapsed_seconds) {
    // Clear footer area
    ST7735_FillRectangle(2, FOOTER_Y, ST7735_WIDTH-4, FOOTER_HEIGHT, COLOR_BG);

    // Separator line
    ST7735_FillRectangle(2, FOOTER_Y, ST7735_WIDTH-4, 1, COLOR_BORDER);

    // Repetition counter
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "REPS: %03d", reps);
    ST7735_WriteString(4, FOOTER_Y + 2, buffer, Font_11x18, COLOR_CYAN, COLOR_BG);

    // Timer
    uint8_t minutes = elapsed_seconds / 60;
    uint8_t seconds = elapsed_seconds % 60;
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    ST7735_WriteString(80, FOOTER_Y + 2, buffer, Font_11x18, COLOR_WHITE, COLOR_BG);
}

/* ========================================================================
 * HELPER FUNCTIONS - Icons & Status Display
 * ======================================================================== */

static const char* get_status_icon(MovementStatus status) {
    // Unicode-inspired ASCII icons for Flipper Zero style
    switch (status) {
        case MOVE_PERFECT:     return "[*]";  // Star for perfect
        case MOVE_GOOD:        return "[+]";  // Plus for good
        case MOVE_WRONG_POSE:  return "[!]";  // Exclamation for wrong
        case MOVE_WRONG_TEMPO: return "[~]";  // Wave for tempo
        case MOVE_EXTREME:     return "[X]";  // X for danger
        case MOVE_IDLE:        return "[ ]";  // Empty for idle
        default:               return "[?]";
    }
}

static uint16_t get_status_color(MovementStatus status) {
    switch (status) {
        case MOVE_PERFECT:     return COLOR_GREEN;
        case MOVE_GOOD:        return COLOR_CYAN;
        case MOVE_WRONG_POSE:  return COLOR_RED;
        case MOVE_WRONG_TEMPO: return COLOR_YELLOW;
        case MOVE_EXTREME:     return COLOR_ORANGE;
        case MOVE_IDLE:        return COLOR_GREY;
        default:               return COLOR_WHITE;
    }
}

static const char* get_status_text(MovementStatus status) {
    switch (status) {
        case MOVE_PERFECT:     return "PERFECT";
        case MOVE_GOOD:        return "GOOD";
        case MOVE_WRONG_POSE:  return "WRONG POSE";
        case MOVE_WRONG_TEMPO: return "WRONG TEMPO";
        case MOVE_EXTREME:     return "DANGER";
        case MOVE_IDLE:        return "READY";
        default:               return "UNKNOWN";
    }
}

static void draw_char_at(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg) {
    ST7735_DrawChar(x, y, c, Font_7x10, color, bg);
}
