#ifndef MPU9250_RAW_DATA_H
#define MPU9250_RAW_DATA_H

#include "main.h"
#include "../../../IMU/Inc/imu_mpu9250_regmap.h"
#include "../../../IMU/Inc/imu_i2c_hal.h"

/**
 * @brief 3-axis data structure (float, SI units)
 *
 * Represents a vector with x, y, z components in SI units.
 * Used for accelerometer (g) and gyroscope (rad/s) measurements.
 */
typedef struct {
    float x; /**< X-axis component */
    float y; /**< Y-axis component */
    float z; /**< Z-axis component */
} vec3;

/**
 * @brief Combined IMU reading structure from a single transaction
 *
 * Holds both accelerometer and gyroscope data obtained in one I2C burst read.
 * This approach is more efficient than reading each sensor separately.
 *
 * @field accel Accelerometer data in g (gravitational acceleration)
 * @field gyro  Gyroscope data in rad/s (angular velocity)
 */
typedef struct {
    vec3 accel; /**< Accelerometer reading in g */
    vec3 gyro;  /**< Gyroscope reading in rad/s */
} imu_data_t;

// =====================================================================
// PUBLIC API
// =====================================================================

/**
 * @brief Read accelerometer and gyroscope data in a single I2C transaction (RECOMMENDED)
 *
 * Performs a burst read to fetch all 6 axes (accel x,y,z + gyro x,y,z) at once.
 * This method minimizes I2C bus overhead and ensures time-coherent sensor data.
 *
 * @param[out] out Pointer to imu_data_t structure to store the readings
 * @return int 0 on success, non-zero if I2C communication fails
 */
int imu_read_all(imu_data_t *out);

/**
 * @brief Read accelerometer data only (kept for backward compatibility)
 *
 * @deprecated Consider using imu_read_all() for better efficiency and data coherence.
 *
 * @return vec3 Accelerometer reading in g (x, y, z components)
 */
vec3 accel_data(void);

/**
 * @brief Read gyroscope data only (kept for backward compatibility)
 *
 * @deprecated Consider using imu_read_all() for better efficiency and data coherence.
 *
 * @return vec3 Gyroscope reading in rad/s (x, y, z components)
 */
vec3 gyro_data(void);

#endif // MPU9250_RAW_DATA_H
