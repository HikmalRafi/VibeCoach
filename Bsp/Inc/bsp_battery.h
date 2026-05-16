/**
 * @file    bsp_battery.h
 * @brief   BSP pembacaan baterai LiPo via ADC STM32F411
 *
 * @details Modul ini menyediakan fungsi untuk:
 *          - Membaca tegangan baterai LiPo melalui ADC dengan averaging
 *          - Menghitung persentase kapasitas baterai
 *          - Menggambar ikon baterai pada LCD ST7735
 *
 *          Sistem cache internal memastikan pembacaan ADC hanya
 *          dilakukan setiap BSP_BAT_UPDATE_MS ms, sehingga tidak
 *          mengganggu performa render LCD/SPI.
 *
 * @note    Asumsi rangkaian voltage divider: 10k + 10k (faktor 2x).
 *          Sesuaikan BAT_V_REF jika referensi ADC berbeda.
 */

#ifndef BSP_BATTERY_H
#define BSP_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * INCLUDES
 * ========================================================================= */

#include "stdint.h"

/* =========================================================================
 * KONFIGURASI TEGANGAN BATERAI
 * ========================================================================= */

/**
 * @brief Tegangan referensi ADC (Vref+) dalam Volt.
 *
 * @note  Sesuaikan dengan tegangan Vref+ pada board STM32 kamu.
 *        Umumnya 3.3V untuk STM32F411.
 */
#define BAT_V_REF       3.3f

/**
 * @brief Tegangan penuh baterai LiPo 1-cell (100%) dalam Volt.
 *
 * @note  Baterai LiPo 1-cell nominal penuh = 4.2V.
 */
#define BAT_V_MAX       4.2f

/**
 * @brief Tegangan minimum baterai LiPo 1-cell (0%) dalam Volt.
 *
 * @note  Baterai LiPo 1-cell cutoff = 3.0V.
 *        Di bawah nilai ini dianggap kosong (0%).
 */
#define BAT_V_MIN       3.0f

/* =========================================================================
 * FUNGSI PUBLIK
 * ========================================================================= */

/**
 * @brief  Dapatkan tegangan baterai saat ini dalam Volt.
 *
 * @details Mengembalikan nilai dari cache internal.
 *          Cache diperbarui secara otomatis setiap BSP_BAT_UPDATE_MS ms,
 *          atau saat pertama kali fungsi ini dipanggil.
 *
 * @return float  Tegangan baterai dalam Volt (misal: 3.85f).
 *                Rentang: 0.0f hingga BAT_V_MAX.
 */
float bsp_battery_get_voltage(void);

/**
 * @brief  Dapatkan persentase kapasitas baterai saat ini.
 *
 * @details Mengembalikan nilai dari cache internal.
 *          Persentase dihitung linear antara BAT_V_MIN (0%) dan BAT_V_MAX (100%).
 *          Cache diperbarui secara otomatis setiap BSP_BAT_UPDATE_MS ms,
 *          atau saat pertama kali fungsi ini dipanggil.
 *
 * @return uint8_t  Persentase baterai (0 - 100).
 */
uint8_t bsp_battery_get_percentage(void);

/**
 * @brief  Gambar ikon baterai pada LCD ST7735.
 *
 * @details Menggambar ikon baterai berukuran 16x8 px (termasuk tonjolan kutub +).
 *          Warna isi ikon menyesuaikan level baterai:
 *          - > 50% : Hijau
 *          - 20–50%: Kuning
 *          - < 20% : Merah
 *
 *          Aman dipanggil setiap frame render karena menggunakan sistem cache
 *          internal; ADC hanya dibaca ulang saat interval sudah terlewati.
 *
 * @param  x         Koordinat X pojok kiri atas ikon pada LCD (pixel).
 * @param  y         Koordinat Y pojok kiri atas ikon pada LCD (pixel).
 * @param  bg_color  Warna latar belakang layar (format RGB565),
 *                   digunakan untuk mengosongkan area dalam outline baterai.
 */
void bsp_battery_draw_icon(uint16_t x, uint16_t y, uint16_t bg_color);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BATTERY_H */
