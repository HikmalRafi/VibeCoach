/**
 * @file gym_dashboard.h
 * @brief Professional Gym Assistant Dashboard UI
 * @author Your Team Name
 * @date 2026
 *
 * Fitur:
 * - Real-time movement visualization dengan progress bar animasi
 * - Multi-warning system: Wrong Pose, Wrong Tempo, Extreme Position
 * - Flipper Zero inspired UI design (clean, monospace, border-based)
 * - Triple feedback: Visual (TFT) + Haptic (Vibration) + Optional Audio (Buzzer)
 */

#ifndef GYM_DASHBOARD_H
#define GYM_DASHBOARD_H

#include "main.h"
#include <stdbool.h>
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"

/* ========================================================================
 * ENUM & STRUCT DEFINITIONS
 * ======================================================================== */

/** @brief Status gerakan user */
typedef enum {
    MOVE_PERFECT = 0,   ///< Gerakan sempurna
    MOVE_GOOD,          ///< Gerakan baik (minor deviation)
    MOVE_WRONG_POSE,    ///< Pose/jalur gerakan salah
    MOVE_WRONG_TEMPO,   ///< Kecepatan gerakan tidak sesuai
    MOVE_EXTREME,       ///< Posisi terlalu ekstrim (resiko cedera)
    MOVE_IDLE           ///< Tidak bergerak
} MovementStatus;

/** @brief Level bahaya untuk vibration motor */
typedef enum {
    VIBE_OFF = 0,       ///< Motor mati
    VIBE_GENTLE,        ///< Getaran halus (koreksi minor)
    VIBE_WARNING,       ///< Getaran sedang (warning)
    VIBE_DANGER         ///< Getaran kuat (bahaya cedera)
} VibrationLevel;

/* ========================================================================
 * PUBLIC FUNCTIONS - Aman dipanggil dari main_menu.c
 * ======================================================================== */

/**
 * @brief Inisialisasi dashboard gym
 * Harus dipanggil SEKALI sebelum menggunakan fungsi lain
 */
void GymDashboard_Init(void);

/**
 * @brief Render dashboard gym real-time
 *
 * @param status Status gerakan saat ini
 * @param accuracy_score Skor akurasi (0.0 - 1.0)
 * @param tempo_score Skor tempo (0.0 - 1.0)
 * @param range_of_motion Persentase ROM (0.0 - 1.0)
 * @param rep_count Jumlah repetisi yang sudah dilakukan
 *
 * @note Panggil fungsi ini setiap 100-200ms untuk animasi smooth
 */
void GymDashboard_Render(MovementStatus status,
                         float accuracy_score,
                         float tempo_score,
                         float range_of_motion,
                         uint8_t rep_count);

/**
 * @brief Update feedback multi-modal berdasarkan status
 * Secara otomatis mengatur vibration motor dan visual warning
 */
void GymDashboard_UpdateFeedback(void);

/**
 * @brief Simpan status terakhir untuk recovery
 */
MovementStatus GymDashboard_GetLastStatus(void);

#endif /* GYM_DASHBOARD_H */
