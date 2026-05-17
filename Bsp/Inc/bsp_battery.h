/**
 * @file    bsp_battery.h
 * @brief   BSP module for LiPo battery reading via STM32F411 ADC
 *
 * @details This module provides functions for:
 *          - Reading LiPo battery voltage via ADC with averaging
 *          - Calculating battery capacity percentage
 *          - Drawing battery icon on ST7735 LCD
 *
 *          An internal caching system ensures ADC is only read every
 *          BSP_BAT_UPDATE_MS ms, preventing interference with LCD/SPI performance.
 *
 * @note    Circuit assumption:
 *            Vbat ── 10kΩ ── PA0 ── 10kΩ ── GND
 *          STM32 GND MUST be connected to battery GND / TP4056 B-.
 *
 * USAGE IN main.c:
 * @code
 *   // After all peripheral init, before while(1)
 *   bsp_battery_force_init();   // pre-warm cache, avoid blocking in the loop
 *
 *   while (1) {
 *       // Safe to call every frame
 *       bsp_battery_draw_icon(x, y, bg_color);
 *
 *       // For hardware connection debugging
 *       if (!bsp_battery_is_adc_ok()) {
 *           // ADC failed — check PA0 wiring and GND
 *       }
 *   }
 * @endcode
 */

#ifndef BSP_BATTERY_H
#define BSP_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/* =========================================================================
 * BATTERY VOLTAGE CONFIGURATION
 * ========================================================================= */

/**
 * @brief ADC reference voltage (Vref+) in Volts.
 *
 * Adjust to match the Vref+ voltage on your STM32 board.
 * Typically 3.3V for STM32F411.
 */
#define BAT_V_REF       3.3f

/**
 * @brief Full charge voltage of 1-cell LiPo battery (100%) in Volts.
 *
 * 1-cell LiPo nominal full charge = 4.2V.
 */
#define BAT_V_MAX       4.2f

/**
 * @brief Minimum voltage of 1-cell LiPo battery (0%) in Volts.
 *
 * 1-cell LiPo cutoff = 3.0V. Below this value is considered empty.
 */
#define BAT_V_MIN       3.0f

/* =========================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================= */

/**
 * @brief  Pre-warm the battery cache. Call ONCE in main.c before while(1).
 *
 * @details Forces the first ADC reading to occur here,
 *          rather than during the first get_voltage/get_percentage call
 *          inside the render loop.
 *
 *          Without this, the 160ms blocking delay (32 samples × 5ms) can cause
 *          LCD flicker or truncate the splash screen animation.
 */
void bsp_battery_force_init(void);

/**
 * @brief  Get the current battery voltage in Volts.
 *
 * @details Returns the value from the internal cache.
 *          Cache is automatically refreshed every BSP_BAT_UPDATE_MS ms.
 *          Non-blocking after bsp_battery_force_init() has been called.
 *
 * @return float  Battery voltage in Volts (e.g.: 3.85f).
 *                Range: 0.0f to BAT_V_MAX.
 */
float bsp_battery_get_voltage(void);

/**
 * @brief  Get the current battery capacity percentage.
 *
 * @details Returns the value from the internal cache.
 *          Percentage is calculated linearly between BAT_V_MIN (0%) and BAT_V_MAX (100%).
 *          Non-blocking after bsp_battery_force_init() has been called.
 *
 * @return uint8_t  Battery percentage (0–100).
 */
uint8_t bsp_battery_get_percentage(void);

/**
 * @brief  Check if the ADC was successfully read in the last update.
 *
 * @details Use for hardware connection debugging.
 *          If always returns 0, check:
 *            - PA0 is connected to the voltage divider midpoint (10k+10k)
 *            - STM32 GND is connected to battery GND / TP4056 B-
 *            - CubeMX: ADC1_IN0 active, PA0 mode = Analog
 *            - CubeMX: hadc1 channel = ADC_CHANNEL_0
 *
 * @return uint8_t  1 if ADC is OK, 0 if all samples failed.
 */
uint8_t bsp_battery_is_adc_ok(void);

/**
 * @brief  Draw a battery icon on the ST7735 LCD.
 *
 * @details Draws a 16×8 px battery icon (including positive terminal bump).
 *          Fill color adjusts according to battery level:
 *            - > 50% : Green
 *            - 20–50%: Yellow
 *            - < 20% : Red
 *
 *          Safe to call every render frame.
 *          ADC is only re-read when the update interval has elapsed.
 *
 * @param  x         X coordinate of the icon's top-left corner (pixels).
 * @param  y         Y coordinate of the icon's top-left corner (pixels).
 * @param  bg_color  Screen background color (RGB565),
 *                   used to clear the area inside the battery outline.
 */
void bsp_battery_draw_icon(uint16_t x, uint16_t y, uint16_t bg_color);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BATTERY_H */
