/**
 * @file    tinyML.h
 * @brief   Public API for the AI motion correction (Gym Form Detector) module.
 * @details This header exposes the functions needed to integrate the tinyML
 *          subsystem into the main application. The module handles:
 *          - IMU calibration and orientation correction
 *          - Sensor data buffering
 *          - Edge Impulse classifier invocation
 *          - Score-based haptic + LED feedback
 */

#ifndef TINYML_H
#define TINYML_H

#include "usart.h"
#include "main.h"
#include <stdarg.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * UART Logging
 * -------------------------------------------------------------------------- */

/**
 * @brief Print formatted string over UART (used by Edge Impulse SDK).
 * @details Wraps vprint() with variable arguments. The Edge Impulse SDK
 *          calls this function internally for debug/diagnostic output.
 * @param format printf-style format string
 * @param ...    Variable arguments matching the format string
 */
void ei_printf(const char *format, ...);

/* --------------------------------------------------------------------------
 * C-Linkage Functions (callable from both C and C++ code)
 * -------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Core UART transmit function (called by ei_printf).
 * @details Formats a string using vsprintf and transmits it via UART1.
 *          Separated from ei_printf to allow direct C-linkage access.
 * @param fmt  Format string
 * @param argp Variable argument list (va_list)
 */
void vprint(const char *fmt, va_list argp);

/**
 * @brief Process one sensor sample — the main entry point for the ML pipeline.
 * @details Call this function periodically (~every 10ms) from the main loop.
 *          It handles the full pipeline:
 *          1. Runs IMU calibration on first call (blocks ~5 seconds)
 *          2. Reads corrected quaternion via invQ()
 *          3. Buffers quaternion components until a full frame is ready
 *          4. Runs the Edge Impulse classifier on the complete frame
 *          5. Updates the error score based on classification result
 *          6. Applies vibration + RGB LED feedback
 *
 *          During a training session, this is the only function that needs
 *          to be called — calibration, quaternion correction, and feedback
 *          are all handled internally.
 */
void add_sensor_data(void);

/**
 * @brief Reset all tinyML state for a fresh training session.
 * @details Zeros the error score, clears the sensor buffer, stops any
 *          active vibration, and turns off the RGB LED. Call this when
 *          starting a new exercise so the user begins with a clean slate.
 */
void tinyml_reset_score(void);

#ifdef __cplusplus
}
#endif

#endif /* TINYML_H */
