#ifndef APP_SPLASH_H
#define APP_SPLASH_H

/**
 * @file    app_splash.h
 * @brief   Splash screen saat boot / wake dari STANDBY.
 *
 * @details Modul ini menangani tampilan awal:
 *          - Animasi boot normal  : logo + nama alat + loading bar
 *          - Animasi wake         : lebih singkat, langsung ke menu
 *
 * @author  Tim Kamu
 * @version 1.0
 */

#include "main.h"
#include <stdint.h>

/* =========================================================================
 * KONFIGURASI SPLASH SCREEN
 * ========================================================================= */

/** Nama alat yang ditampilkan di splash screen */
#define SPLASH_DEVICE_NAME      "AI-Motion"

/** Subtitle / tagline di bawah nama alat */
#define SPLASH_SUBTITLE         "Correction v1.0"

/** Durasi splash screen normal (ms) - sebelum masuk menu */
#define SPLASH_DURATION_MS      2500

/** Durasi splash screen wake (ms) - lebih singkat */
#define SPLASH_WAKE_DURATION_MS 1200

/* =========================================================================
 * DEKLARASI FUNGSI
 * ========================================================================= */

/**
 * @brief Tampilkan splash screen saat boot pertama kali.
 *
 * @details Menampilkan:
 *          - Background berwarna tema
 *          - Nama alat (besar, di tengah)
 *          - Subtitle kecil
 *          - Loading bar animasi
 *          - Durasi: SPLASH_DURATION_MS
 */
void app_splash_show_boot(void);

/**
 * @brief Tampilkan splash screen singkat saat wake dari STANDBY.
 *
 * @details Versi lebih ringkas dari boot splash.
 *          Durasi: SPLASH_WAKE_DURATION_MS
 */
void app_splash_show_wake(void);

/**
 * @brief Tampilkan animasi shutdown sebelum masuk STANDBY.
 *
 * @details Menampilkan teks "Shutting down..." + progress bar mengecil.
 *          Dipanggil dari bsp_power_on_shutdown_callback().
 */
void app_splash_show_shutdown(void);

#endif /* APP_SPLASH_H */
