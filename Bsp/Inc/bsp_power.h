/**
 * @file    bsp_power.h
 * @brief   Power toggle ON/OFF via PB10 — pull-up, Active LOW
 *
 * @details Hardware:
 *            3.3V ─── [pull-up] ─── PB10 ─── [tombol] ─── GND
 *
 *          Kondisi pin:
 *            Tombol tidak ditekan  →  PB10 = HIGH (1)  ditarik VCC
 *            Tombol ditekan        →  PB10 = LOW  (0)  tersambung GND
 *
 *          Ini Active LOW — persis sama dengan btn_atas/btn_bawah/btn_ok.
 *          Library button_read() langsung bisa dipakai tanpa modifikasi.
 *
 *          CubeMX PB10 (wajib):
 *            GPIO Mode         : External Interrupt - Falling edge
 *            GPIO Pull-up/down : Pull-up  (atau pakai external pull-up)
 *            NVIC EXTI15_10    : Enabled
 */

#ifndef BSP_POWER_H
#define BSP_POWER_H

#include "main.h"
#include "../../Bsp/Inc/bsp_debounce_button.h"
#include <stdint.h>

/* Pin power button */
#define PWR_BTN_PORT   GPIOB
#define PWR_BTN_PIN    GPIO_PIN_1

/* Untuk cek state pin secara langsung (dipakai di shutdown saat tunggu lepas) */
#define PWR_BTN_IS_PRESSED()   (HAL_GPIO_ReadPin(PWR_BTN_PORT, PWR_BTN_PIN) == GPIO_PIN_RESET)

extern void SystemClock_Config(void);

void    bsp_power_init(void);
void    bsp_power_poll(void);
void    bsp_power_shutdown(void);
uint8_t bsp_power_is_wakeup(void);
void    bsp_power_on_shutdown_callback(void);
void    bsp_power_on_wakeup_callback(void);

#endif /* BSP_POWER_H */
