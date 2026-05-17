/**
 * @file    bsp_led_rgb.c
 * @brief   RGB LED control implementation via PWM timer channels.
 *
 * @details Provides PWM-based control for a common-cathode RGB LED
 *          using three timer channels. Supports predefined colors
 *          with full brightness (PWM duty = 999/1000) and a global
 *          intensity control that scales all channels proportionally.
 *
 *          Training session LED behavior:
 *          - IDLE (no training)    : LED OFF
 *          - LEVEL_NONE (correct)  : GREEN   (good form, keep going)
 *          - LEVEL_CORRECTION      : BLUE    (caution - minor correction)
 *          - LEVEL_WARNING         : MAGENTA (warning - moderate deviation)
 *          - LEVEL_DANGER          : RED     (danger - severe form error)
 */

#include "../../../Bsp/Inc/bsp_led_rgb.h"
#include "../../Core/Inc/tinyML/tinyML.h"  /**< For WrongIntensity enum */

/** @brief Timer handle for the red LED PWM channel */
static TIM_HandleTypeDef *tim_red;
/** @brief Timer handle for the green LED PWM channel */
static TIM_HandleTypeDef *tim_green;
/** @brief Timer handle for the blue LED PWM channel */
static TIM_HandleTypeDef *tim_blue;

/** @brief Timer channel number for red LED */
static uint32_t channel_red;
/** @brief Timer channel number for green LED */
static uint32_t channel_green;
/** @brief Timer channel number for blue LED */
static uint32_t channel_blue;

/** @brief Current LED color state */
static LED_Color_t current_color = LED_COLOR_OFF;

/** @brief Training mode flag (extern from app_menu.c) */
extern volatile uint8_t training_is;

/**
 * @brief Initialize the RGB LED with PWM timer channels
 *
 * Stores the timer handles and channel numbers for later use,
 * then turns off all channels to ensure a known initial state.
 *
 * @param[in] htim_red   Timer handle for the red LED channel
 * @param[in] ch_red     Timer channel number for red LED
 * @param[in] htim_green Timer handle for the green LED channel
 * @param[in] ch_green   Timer channel number for green LED
 * @param[in] htim_blue  Timer handle for the blue LED channel
 * @param[in] ch_blue    Timer channel number for blue LED
 */
void LED_RGB_Init(TIM_HandleTypeDef *htim_red, uint32_t ch_red,
                  TIM_HandleTypeDef *htim_green, uint32_t ch_green,
                  TIM_HandleTypeDef *htim_blue, uint32_t ch_blue)
{
    /** Store timer handles for later use */
    tim_red = htim_red;
    tim_green = htim_green;
    tim_blue = htim_blue;

    /** Store channel numbers for later use */
    channel_red = ch_red;
    channel_green = ch_green;
    channel_blue = ch_blue;

    /** Ensure LED starts in off state */
    LED_RGB_Off();
}

/**
 * @brief Set the RGB LED to a predefined color
 *
 * Configures the PWM duty cycles for all three channels
 * to produce the requested color. Uses PWM value 999 (out of 1000)
 * for full brightness on active channels.
 *
 * Color mixing:
 * - OFF     : All channels 0
 * - RED     : Red channel only
 * - GREEN   : Green channel only
 * - BLUE    : Blue channel only
 * - MAGENTA : Red + Blue
 *
 * @param[in] color Desired color from LED_Color_t enum
 *
 * @note PWM period is assumed to be 1000 (ARR = 999).
 *       Adjust PWM values if using a different timer period.
 */
void LED_RGB_SetColor(LED_Color_t color)
{
    current_color = color;

    switch(color) {
        case LED_COLOR_OFF:
            /** All channels off */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 0);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 0);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 0);
            break;
        case LED_COLOR_RED:
            /** Red channel: full, Green/Blue: off */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 999);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 0);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 0);
            break;
        case LED_COLOR_GREEN:
            /** Green channel: full, Red/Blue: off */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 0);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 999);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 0);
            break;
        case LED_COLOR_BLUE:
            /** Blue channel: full, Red/Green: off */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 0);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 0);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 999);
            break;
        case LED_COLOR_MAGENTA:
            /** Red + Blue = Magenta (warning indicator) */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 999);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 0);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 999);
            break;
        default:
            /** Unknown color → turn off */
            __HAL_TIM_SET_COMPARE(tim_red, channel_red, 0);
            __HAL_TIM_SET_COMPARE(tim_green, channel_green, 0);
            __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, 0);
            break;
    }
}

/**
 * @brief Turn off the RGB LED immediately
 *
 * Convenience function that delegates to LED_RGB_SetColor(LED_COLOR_OFF)
 * to set all PWM channels to 0% duty cycle.
 */
void LED_RGB_Off(void)
{
    LED_RGB_SetColor(LED_COLOR_OFF);
}

/**
 * @brief Set the global intensity/brightness of the RGB LED
 *
 * Applies a uniform scaling factor to all three PWM channels.
 * The intensity is mapped from 0-255 range to 0-999 PWM duty cycle.
 *
 * @param[in] intensity Brightness level (0 = off, 255 = full brightness)
 *
 * @note This function sets ALL channels to the same PWM value,
 *       which produces white light at the specified intensity.
 *       For per-channel intensity control with color preservation,
 *       use LED_RGB_SetColor() instead.
 */
void LED_RGB_SetIntensity(uint8_t intensity)
{
    /**
     * Map 0-255 intensity to 0-999 PWM duty cycle.
     * Formula: pwm_val = intensity / 255 * 999
     */
    uint16_t pwm_val = (uint16_t)((float)intensity / 255.0f * 999.0f);

    /** Apply the same PWM value to all three channels */
    __HAL_TIM_SET_COMPARE(tim_red, channel_red, pwm_val);
    __HAL_TIM_SET_COMPARE(tim_green, channel_green, pwm_val);
    __HAL_TIM_SET_COMPARE(tim_blue, channel_blue, pwm_val);
}

/**
 * @brief Update RGB LED based on current training intensity level
 *
 * Maps the exercise intensity level to LED colors:
 * - Not training (training_is == 0) : LED OFF
 * - LEVEL_NONE    (correct form)    : GREEN   (good form, keep going)
 * - LEVEL_CORRECTION (caution)      : BLUE    (minor correction needed)
 * - LEVEL_WARNING (warning)         : MAGENTA (moderate form deviation)
 * - LEVEL_DANGER  (danger)          : RED     (severe form error)
 *
 * Color progression logic:
 * GREEN → BLUE → MAGENTA → RED follows a natural cool-to-warm
 * progression that intuitively signals increasing urgency.
 *
 * This function should be called periodically during training sessions,
 * typically inside the display update section (10 Hz is sufficient).
 *
 * @param[in] lvl Current intensity level from the classifier
 *
 * @note Only updates LED when the color actually changes to avoid
 *       unnecessary PWM register writes.
 */

/**
 * @brief Get the current LED color
 * @return LED_Color_t Current color of the RGB LED
 */
LED_Color_t LED_RGB_GetCurrentColor(void)
{
    return current_color;
}
