#include "main.h"

#include "../../../IMU/Inc/imu_madgwick_filter.h"
#include "../../../IMU/Inc/imu_mpu9250_drive.h"
#include "../../../Lib/ST7735/st7735.h"

#ifndef CALIBRATION_FEATURE_H
#define CALIBRATION_FEATURE_H

/** @brief Global flag indicating whether calibration has been completed */
extern volatile bool calibrated;

/**
 * @brief Base quaternion values stored after calibration with normal pose
 *
 * These reference quaternion components represent the sensor's orientation
 * when the device is in its calibrated "normal" resting position.
 * Used as the reference frame for subsequent orientation calculations.
 */
extern volatile float q0_base; /**< Base quaternion scalar component (w) */
extern volatile float q1_base; /**< Base quaternion x-axis component */
extern volatile float q2_base; /**< Base quaternion y-axis component */
extern volatile float q3_base; /**< Base quaternion z-axis component */

/**
 * @brief Quaternion data structure
 *
 * Represents a quaternion with scalar (w) and vector (x, y, z) components.
 * Used for 3D orientation representation to avoid gimbal lock.
 */
typedef struct {
    float w; /**< Scalar component */
    float x; /**< X-axis vector component */
    float y; /**< Y-axis vector component */
    float z; /**< Z-axis vector component */
} Quaternion;

/**
 * @brief Perform quaternion calibration by sampling normal pose data
 *
 * Collects multiple orientation samples while the device is held in a
 * predefined normal pose. The averaged result is stored in the global
 * q0_base through q3_base variables for later use as a reference frame.
 *
 * @note This function should be called once during initial setup while
 *       the device is stationary in its intended reference orientation.
 */
void calibrateQuaternion(void);

/**
 * @brief Compute the inverse quaternion for reference frame correction
 *
 * Performs three operations sequentially:
 * 1. Normalizes the base quaternion (q0_base to q3_base)
 * 2. Computes its inverse (conjugate for unit quaternion)
 * 3. Multiplies it with the current orientation quaternion
 *
 * This effectively transforms sensor readings from the global frame
 * into the calibrated reference frame.
 *
 * @return Quaternion The resulting orientation relative to the calibrated base pose
 */
Quaternion invQ(void);

#endif // CALIBRATION_FEATURE_H
