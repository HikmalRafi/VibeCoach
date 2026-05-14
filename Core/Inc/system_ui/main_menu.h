#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "main.h"
#include "lcd_screen/st7735.h"
#include "lcd_screen/fonts.h"
#include "fusionFilter_and_calibration/calibration_feature.h"
#include "debounce_button.h"
#include "usart.h"
#include <stdint.h>
#include <stddef.h>  // Untuk NULL

// =====================================================================
// DEKLARASI STRUKTUR DATA MENU
// =====================================================================

// Forward declaration untuk MenuPage_t agar bisa digunakan di dalam MenuItem_t
typedef struct MenuPage_t MenuPage_t;

// Struktur untuk satu baris pilihan menu
typedef struct {
    const char* text;           // Teks yang akan ditampilkan di layar
    void (*action)(void);       // Function pointer: fungsi yang dipanggil saat tombol OK ditekan
} MenuItem_t;

// Struktur untuk satu halaman menu penuh
struct MenuPage_t {
    const char* title;          // Judul halaman (ditampilkan di paling atas)
    const MenuItem_t* items;    // Array daftar pilihan menu di halaman ini
    uint8_t num_items;          // Jumlah pilihan yang ada di array
    const MenuPage_t* parent;   // Pointer ke halaman sebelumnya (untuk fitur "Kembali")
};

// =====================================================================
// DEKLARASI FUNGSI GLOBAL
// =====================================================================

/**
 * @brief Fungsi utama untuk menjalankan sistem menu.
 * Panggil fungsi ini di dalam loop while(1) pada main.c
 */
void training_menu(void);

// (Opsional) Jika kamu butuh memanggil fungsi ini dari luar, buka komennya:
// void goto_main_menu(void);
// void goto_deltoid_menu(void);
// void goto_pushup_menu(void);

#endif
