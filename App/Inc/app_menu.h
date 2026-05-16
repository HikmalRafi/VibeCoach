#ifndef APP_MENU_H
#define APP_MENU_H

/**
 * @file    app_menu.h
 * @brief   Header utama sistem menu AI-Motion Correction.
 *
 * @details Berisi deklarasi struct menu dan fungsi-fungsi navigasi.
 *          Versi 2.1 menambahkan integrasi power management.
 */

#include "main.h"
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"
#include "../../../IMU/Inc/imu_calibration.h"
#include "../../Bsp/Inc/bsp_debounce_button.h"
#include "../../Bsp/Inc/bsp_power.h"       /* Power management */
#include "../../App/Inc/app_splash.h"      /* Splash screen    */
#include "../../Bsp/Inc/bsp_battery.h"
#include "usart.h"
#include <stdint.h>
#include <stddef.h>

/* =========================================================================
 * DEKLARASI STRUKTUR DATA MENU
 * ========================================================================= */

/* Forward declaration agar bisa digunakan di dalam MenuItem_t */
typedef struct MenuPage_t MenuPage_t;

/* Struktur untuk satu baris pilihan menu */
typedef struct {
    const char* text;
    void (*action)(void);
} MenuItem_t;

/* Struktur untuk satu halaman menu penuh */
struct MenuPage_t {
    const char* title;
    const MenuItem_t* items;
    uint8_t num_items;
    const MenuPage_t* parent;
};

/* =========================================================================
 * DEKLARASI FUNGSI GLOBAL
 * ========================================================================= */

/**
 * @brief Fungsi utama sistem menu. Panggil di dalam while(1) pada main.c.
 */
void training_menu(void);

#endif /* APP_MENU_H */
