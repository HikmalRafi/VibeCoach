/**
 * @file    battery.h
 * @brief   Monitoring baterai LiPo dengan ADC STM32
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* =========================================================================
 * KONFIGURASI HARDWARE
 * ========================================================================= */

/** Pin ADC untuk membaca tegangan baterai */
#define BATTERY_ADC_PIN        GPIO_PIN_0
#define BATTERY_ADC_PORT       GPIOA

/** ADC handle (sesuaikan dengan inisialisasi di main.c) */
extern ADC_HandleTypeDef hadc1;

/** Rasio voltage divider (2:1 dengan 2x 10kΩ) */
#define VOLTAGE_DIVIDER_RATIO  2.0f

/** Referensi tegangan ADC STM32 */
#define ADC_VREF               3.3f

/** ADC Resolution 12-bit = 4096 steps */
#define ADC_MAX_VALUE          4095.0f

/* =========================================================================
 * BATAS TEGANGAN BATERAI LIPO
 * ========================================================================= */

/** Tegangan maksimal LiPo (baru dicharge penuh) */
#define BATT_MAX_VOLTAGE       4.2f

/** Tegangan minimal LiPo (sebelum cutoff TP4056) */
#define BATT_MIN_VOLTAGE       3.3f

/** Tegangan nominal LiPo */
#define BATT_NOMINAL_VOLTAGE   3.7f

/* =========================================================================
 * FUNGSI API
 * ========================================================================= */

/**
 * @brief Inisialisasi ADC untuk monitoring baterai
 * @note  Panggil sekali saat startup di main()
 */
void battery_init(void);

/**
 * @brief Membaca tegangan baterai saat ini (dalam Volt)
 * @return Tegangan baterai dalam Volt (contoh: 3.85)
 */
float battery_read_voltage(void);

/**
 * @brief Menghitung persentase baterai (0-100%)
 * @return Persentase baterai
 */
uint8_t battery_get_percentage(void);

/**
 * @brief Mendapatkan level baterai untuk ikon (0-3)
 * @return 0=empty, 1=low, 2=medium, 3=full
 */
uint8_t battery_get_level(void);

#endif /* BATTERY_H */
