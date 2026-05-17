/**
 * @file    debounce_button.h
 * @brief   Button debounce module using a simple state machine.
 *
 * @details Provides a reusable button debounce implementation that
 *          eliminates mechanical switch bounce noise. Uses a time-based
 *          approach where the button state is only considered stable
 *          after the signal has remained unchanged for DebounceDelay ms.
 *
 *          Based on the classic Arduino debounce algorithm, adapted
 *          for STM32 HAL with struct-based encapsulation for multiple
 *          button instances.
 */

#ifndef DEBOUNCE_BUTTON_H
#define DEBOUNCE_BUTTON_H

#include "main.h"
#include <stdint.h>
#include "stm32f4xx_hal.h"

/**
 * @brief Structure representing a debounced button instance
 *
 * Encapsulates all state required for debouncing a single
 * GPIO-based push button. Multiple instances can be created
 * for different buttons without conflicts.
 *
 * @field port             GPIO port of the button (e.g., GPIOB)
 * @field pin              GPIO pin number (e.g., GPIO_PIN_12)
 * @field LastDebounceTime Timestamp of the last state change (ms)
 * @field DebounceDelay    Debounce time in ms; increase if output still flickers
 * @field BtnState         Current debounced (stable) button state
 * @field LastBtnState     Previous raw reading from the input pin
 */
typedef struct {
    GPIO_TypeDef* port;             /**< GPIO port of the button */
    uint16_t pin;                   /**< GPIO pin number */
    uint32_t LastDebounceTime;      /**< Last time the output pin was toggled */
    uint32_t DebounceDelay;         /**< Debounce time; increase if output still flickers */
    uint8_t BtnState;               /**< Current debounced reading from the input pin */
    uint8_t LastBtnState;           /**< Previous raw reading from the input pin */
} button_t;

/**
 * @brief Initialize a button debounce structure
 *
 * Sets up the button struct with the specified GPIO port and pin,
 * reads the initial state, and configures the debounce delay.
 *
 * @param[out] btn  Pointer to the button_t structure to initialize
 * @param[in]  port GPIO port of the button (e.g., GPIOB)
 * @param[in]  pin  GPIO pin number (e.g., GPIO_PIN_12)
 *
 * @note Call once for each button before using button_read().
 *       Default debounce delay is set to ~50ms which works well
 *       for most mechanical switches.
 */
void button_init(button_t *btn, GPIO_TypeDef* port, uint16_t pin);

/**
 * @brief Read the debounced state of a button
 *
 * Returns the stable (debounced) button state. This function
 * should be called frequently (e.g., every loop iteration) for
 * proper debounce timing.
 *
 * @param[in,out] btn Pointer to the button_t structure
 * @return uint8_t 1 if button is pressed (HIGH), 0 if released (LOW)
 *
 * @note This function handles the internal debounce logic.
 *       The returned value is the stable state, not the raw GPIO reading.
 */
uint8_t button_read(button_t *btn);

#endif /* DEBOUNCE_BUTTON_H */
