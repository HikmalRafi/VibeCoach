/**
 * @file    bsp_led_rgb.h
 * @brief   BSP module for RGB LED control via PWM (TIM channels).
 *
 * @details Provides a simple API for controlling a common-cathode RGB LED
 *          using three timer channels. Supports predefined colors,
 *          custom intensity control, and training-aware color updates
 *          based on exercise form intensity levels.
 *
 *          Training session LED behavior:
 *          - Not in training         : LED OFF
 *          - LEVEL_NONE (correct)    : GREEN   (good form, keep going)
 *          - LEVEL_CORRECTION        : BLUE    (caution - minor correction)
 *          - LEVEL_WARNING           : MAGENTA (warning - moderate deviation)
 *          - LEVEL_DANGER            : RED     (danger - severe form error)
 */

#ifndef BSP_LED_RGB_H
#define BSP_LED_RGB_H

#include "main.h"
#include "../../Core/Inc/tinyML/tinyML.h"  /**< For WrongIntensity enum */

/**
 * @brief Available LED colors enumeration
 *
 * Defines the predefined color options for the RGB LED.
 * Colors are created by mixing red, green, and blue PWM channels.
 */
typedef enum {
    LED_COLOR_OFF = 0,   /**< LED off (all channels 0) */
    LED_COLOR_RED,       /**< Red only */
    LED_COLOR_GREEN,     /**< Green only */
    LED_COLOR_BLUE,      /**< Blue only */
    LED_COLOR_MAGENTA,   /**< Red + Blue */
    LED_COLOR_CYAN,      /**< Green + Blue */
    LED_COLOR_YELLOW,    /**< Red + Green */
    LED_COLOR_WHITE      /**< Red + Green + Blue */
} LED_Color_t;

void LED_RGB_Init(TIM_HandleTypeDef *htim_red, uint32_t ch_red,
                  TIM_HandleTypeDef *htim_green, uint32_t ch_green,
                  TIM_HandleTypeDef *htim_blue, uint32_t ch_blue);
void LED_RGB_SetColor(LED_Color_t color);
void LED_RGB_SetIntensity(uint8_t intensity);
void LED_RGB_Off(void);
LED_Color_t LED_RGB_GetCurrentColor(void);

#endif /* BSP_LED_RGB_H */
