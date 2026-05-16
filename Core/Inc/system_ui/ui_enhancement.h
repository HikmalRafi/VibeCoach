/**
 * @file ui_enhancement.h
 * @brief UI Enhancement untuk tampilan profesional ala Flipper Zero
 * @author Your Team
 * @version 1.0
 *
 * @note Fungsi-fungsi ini TIDAK mengubah struktur menu existing,
 *       hanya menambah elemen visual di atasnya.
 */

#ifndef UI_ENHANCEMENT_H
#define UI_ENHANCEMENT_H

#include <stdint.h>
#include <string.h>
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"

// =====================================================================
// KONSTANTA WARNA (RGB565) - Warna khas Flipper Zero
// =====================================================================
#define UI_COLOR_BG_DARK     0x0841      // Hitam kebiruan (#08212B)
#define UI_COLOR_ACCENT      0xFAA0      // Orange Flipper (#FFA500)
#define UI_COLOR_SUCCESS     0x07E0      // Hijau terang
#define UI_COLOR_WARNING     0xFFE0      // Kuning
#define UI_COLOR_ERROR       0xF800      // Merah
#define UI_COLOR_CARD        0x2108      // Abu-abu gelap untuk card
#define UI_COLOR_BORDER      0x4A69      // Biru tua untuk border

// =====================================================================
// ENUM FEEDBACK LEVEL
// =====================================================================
typedef enum {
    FEEDBACK_NONE = 0,
    FEEDBACK_WARNING,       // Peringanan ringan (sedikit melenceng)
    FEEDBACK_DANGER,        // Bahaya (salah total)
    FEEDBACK_SUCCESS        // Gerakan benar
} FeedbackLevel;

// =====================================================================
// FUNGSI UI - Status Bar & Header
// =====================================================================

/**
 * @brief Gambar status bar seperti Flipper (baterei + waktu)
 * @param battery_percent Persen baterai (0-100)
 * @param time_str String waktu format "12:34"
 */
void UI_DrawStatusBar(uint8_t battery_percent, const char* time_str);

/**
 * @brief Gambar header halaman dengan icon
 * @param title Judul halaman
 * @param icon_char Karakter icon (gunakan ASCII: 'M','D','P', dll)
 */
void UI_DrawHeader(const char* title, char icon_char);

/**
 * @brief Gambar card selection (menu item dengan border)
 * @param x, y Posisi
 * @param text Teks menu
 * @param is_selected Apakah item ini sedang dipilih cursor
 */
void UI_DrawMenuItem(uint16_t x, uint16_t y, const char* text, uint8_t is_selected);

// =====================================================================
// FUNGSI TRAINING MODE - Visual Feedback
// =====================================================================

/**
 * @brief Tampilan layar training (seperti screenshot yang Anda kirim)
 * @param exercise_name Nama exercise ("DELTOID" / "PUSH UP")
 * @param rep_current Rep saat ini
 * @param rep_target Target rep
 * @param confidence Confidence score (0-100)
 * @param quat_w,x,y,z Nilai quaternion
 * @param feedback_level Level feedback (warna layar berubah)
 */
void UI_DrawTrainingScreen(
    const char* exercise_name,
    uint16_t rep_current,
    uint16_t rep_target,
    uint8_t confidence,
    float quat_w, float quat_x, float quat_y, float quat_z,
    FeedbackLevel feedback_level
);

/**
 * @brief Gambar progress bar (untuk reps atau confidence)
 * @param x, y Posisi kiri atas
 * @param width, height Ukuran progress bar
 * @param percent Nilai 0-100
 * @param color Warna bar (gunakan UI_COLOR_SUCCESS/WARNING/ERROR)
 */
void UI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percent, uint16_t color);

/**
 * @brief Tampilkan notifikasi besar di tengah layar (animasi singkat)
 * @param message Teks notifikasi
 * @param duration_ms Durasi tampil (ms), 0 = sampai next redraw
 */
void UI_ShowToast(const char* message, uint32_t duration_ms);

// =====================================================================
// FUNGSI UTILITY - Animasi
// =====================================================================

/**
 * @brief Efek "pulse" untuk attention (misal saat gerakan salah)
 * @param target_y Baris target (0-15)
 * @param color Warna pulse
 */
void UI_PulseLine(uint16_t target_y, uint16_t color);

#endif // UI_ENHANCEMENT_H
