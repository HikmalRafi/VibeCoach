#ifndef APP_SPLASH_H
#define APP_SPLASH_H

/**
 * @file    app_splash.h
 * @brief   Splash screen module for boot and wake-from-STANDBY events.
 *
 * @details This module handles the initial display sequences:
 *          - Normal boot animation : logo + device name + loading bar
 *          - Wake animation        : shorter sequence, goes directly to menu
 *
 * @author  Your Team
 * @version 1.0
 */

#include "main.h"
#include <stdint.h>

/* =========================================================================
 * SPLASH SCREEN CONFIGURATION
 * ========================================================================= */

/** @brief Device name displayed on the splash screen */
#define SPLASH_DEVICE_NAME      "AI-Motion"

/** @brief Subtitle / tagline displayed below the device name */
#define SPLASH_SUBTITLE         "Correction v1.0"

/** @brief Normal splash screen duration (ms) - before entering menu */
#define SPLASH_DURATION_MS      2500

/** @brief Wake splash screen duration (ms) - shorter than normal boot */
#define SPLASH_WAKE_DURATION_MS 1200

/* =========================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================= */

/**
 * @brief Display the splash screen during initial boot.
 *
 * @details Shows:
 *          - Theme-colored background
 *          - Device name (large, centered)
 *          - Small subtitle text
 *          - Animated loading bar
 *          - Duration: SPLASH_DURATION_MS
 *
 * @note This function blocks for the duration of the splash screen.
 *       Called once during system startup before entering the main menu.
 */
void app_splash_show_boot(void);

/**
 * @brief Display a short splash screen when waking from STANDBY mode.
 *
 * @details A more compact version of the boot splash.
 *          Duration: SPLASH_WAKE_DURATION_MS
 *
 * @note This provides visual feedback that the device is resuming
 *       from low-power state without the full boot sequence.
 */
void app_splash_show_wake(void);

/**
 * @brief Display a shutdown animation before entering STANDBY mode.
 *
 * @details Shows "Shutting down..." text with a shrinking progress bar.
 *          Called from bsp_power_on_shutdown_callback().
 *
 * @note This provides visual confirmation that the device is
 *       powering down safely.
 */
void app_splash_show_shutdown(void);

#endif /* APP_SPLASH_H */
