#ifndef ERROR_CLASSIFIER_H
#define ERROR_CLASSIFIER_H

/**
 * @file    error_classifier.h
 * @brief   Error classification module for exercise form detection.
 *
 * @details Defines error types that can be detected during training
 *          and provides functions to classify and convert errors to
 *          human-readable strings for display.
 */

#include "main.h"

#include "../../../IMU/Inc/imu_madgwick_filter.h"  /**< For roll, pitch, yaw access */

/* =========================================================================
 * ERROR TYPE ENUMERATION
 * ========================================================================= */

/**
 * @brief Types of exercise form errors that can be detected
 *
 * Each error type represents a specific form deviation that
 * the classifier can identify during training sessions.
 */
typedef enum {
    ERR_NONE = 0,        /**< Correct movement, no error detected */
    ERR_TEMPO_FAST,      /**< Movement performed too quickly */
    ERR_SHOULDER_UP,     /**< Shoulder shrug detected (shoulder elevation) */
    ERR_ROM_LOW,         /**< Range of motion insufficient, not reaching peak */
    ERR_FORM_GENERAL,    /**< Incorrect form but no specific category matched */
} ErrorType_t;

/* =========================================================================
 * PUBLIC FUNCTION DECLARATIONS
 * ========================================================================= */

/**
 * @brief Classify the current exercise movement for form errors
 *
 * Analyzes the current sensor data (roll, pitch, yaw from Madgwick filter)
 * and determines if any form errors are present.
 *
 * @return ErrorType_t The detected error type, or ERR_NONE if form is correct
 *
 * @note This function should be called after the Madgwick filter has been
 *       updated with fresh sensor data.
 */
ErrorType_t classify_error(void);

/**
 * @brief Convert an error type to a human-readable string
 *
 * Maps each ErrorType_t enum value to its corresponding display string
 * for showing on the LCD screen during training.
 *
 * @param[in] err The error type to convert
 * @return const char* Pointer to a null-terminated string describing the error.
 *                     Returns "None" for ERR_NONE, specific message for others.
 */
const char* error_to_string(ErrorType_t err);

#endif /* ERROR_CLASSIFIER_H */
