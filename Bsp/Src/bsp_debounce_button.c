/**
 * @file    bsp_debounce_button.c
 * @brief   Button debounce implementation using time-based state machine.
 *
 * @details Implements a classic debounce algorithm that requires the button
 *          state to remain stable for DebounceDelay milliseconds before
 *          accepting it as a valid state change. This eliminates false
 *          triggers caused by mechanical switch bounce.
 *
 *          The algorithm detects a button press on the falling edge only
 *          (active low: 1 → 0 transition), returning 1 for a single
 *          iteration when a new press is confirmed.
 */

#include "../../Bsp/Inc/bsp_debounce_button.h"

/**
 * @brief Initialize a new button debounce structure
 *
 * Configures the GPIO port/pin, sets default debounce parameters,
 * and initializes the button state assuming an active-low configuration
 * (default unpressed state = HIGH/1).
 *
 * @param[out] btn  Pointer to the button_t structure to initialize
 * @param[in]  port GPIO port of the button (e.g., GPIOB)
 * @param[in]  pin  GPIO pin number (e.g., GPIO_PIN_12)
 *
 * @note Default DebounceDelay is set to 20ms.
 *       Increase this value if the button output still flickers.
 */
void button_init(button_t *btn, GPIO_TypeDef* port, uint16_t pin) {
    btn->port = port;
    btn->pin = pin;
    btn->DebounceDelay = 20;                        /**< Default debounce: 20ms */
    btn->LastDebounceTime = 0;                      /**< Reset debounce timer */
    btn->LastBtnState = 1;                          /**< Assume active low (default unpressed = 1) */
    btn->BtnState = 1;                              /**< Initial stable state: unpressed */
}

/**
 * @brief Read and debounce a button, detecting new press events
 *
 * Implements a time-based debounce state machine:
 * 1. Reads the raw GPIO pin state
 * 2. If the reading differs from the last reading, resets the debounce timer
 * 3. If the reading has been stable for DebounceDelay ms:
 *    - Updates the stable button state
 *    - Detects falling edge (active low: 1 → 0) as a button press
 *
 * @param[in,out] btn Pointer to the button_t structure
 * @return uint8_t 1 if a new button press was detected (on falling edge),
 *                 0 if no action (button released, held, or still bouncing)
 *
 * @note This function should be called frequently (every main loop iteration)
 *       for accurate debounce timing. The returned press event is only active
 *       for one call — subsequent calls will return 0 until the button is
 *       released and pressed again.
 */
uint8_t button_read(button_t *btn) {
    /** Read the current raw GPIO pin state */
    uint8_t reading = HAL_GPIO_ReadPin(btn->port, btn->pin);
    uint32_t currentTick = HAL_GetTick();
    uint8_t result = 0; /**< Default: no action detected */

    /**
     * If the reading has changed since last time,
     * reset the debounce timer to start counting stability.
     */
    if (reading != btn->LastBtnState) {
        btn->LastDebounceTime = currentTick;
    }

    /**
     * Check if the reading has been stable longer than the debounce delay.
     * Only then is the state considered valid.
     */
    if ((currentTick - btn->LastDebounceTime) > btn->DebounceDelay) {
        /**
         * If the stable reading differs from the current known state,
         * the button has genuinely changed state.
         */
        if (reading != btn->BtnState) {
            btn->BtnState = reading;

            /**
             * Detect button press: active low configuration.
             * A transition to 0 (LOW) means the button was pressed.
             * Returns 1 to indicate a trigger event on this call only.
             */
            if (btn->BtnState == 0) {
                result = 1; /**< Trigger! New button press detected */
            }
        }
    }

    /** Store the current raw reading for comparison on the next call */
    btn->LastBtnState = reading;
    return result;
}
