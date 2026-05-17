#include "../../../IMU/Inc/imu_mpu9250_drive.h"

/** @brief MPU9250 default I2C slave address (AD0 pin LOW) */
#define SLAVE_ADDR   0x68

/** @brief Accelerometer sensitivity: LSB per g for ±2g range (default) */
#define ACCEL_SENS   16384.0f

/** @brief Gyroscope sensitivity: LSB per °/s for ±250dps range (default) */
#define GYRO_SENS    131.0f

/** @brief Conversion factor from degrees to radians (π/180) */
#define DEG_TO_RAD   0.0174533f

/**
 * @brief Read accelerometer and gyroscope data in a single I2C transaction (RECOMMENDED)
 *
 * Performs a burst read of 14 consecutive bytes starting from ACCEL_XOUT_H (0x3B).
 * This captures all 6 axes (accel x,y,z + gyro x,y,z) in one I2C transaction,
 * which is significantly more efficient than reading each sensor separately.
 *
 * MPU9250 register map (sequential order):
 *   0x3B ACCEL_XOUT_H   ← Start address
 *   0x3C ACCEL_XOUT_L
 *   0x3D ACCEL_YOUT_H
 *   0x3E ACCEL_YOUT_L
 *   0x3F ACCEL_ZOUT_H
 *   0x40 ACCEL_ZOUT_L
 *   0x41 TEMP_OUT_H     ← Read but ignored
 *   0x42 TEMP_OUT_L     ← Read but ignored
 *   0x43 GYRO_XOUT_H
 *   0x44 GYRO_XOUT_L
 *   0x45 GYRO_YOUT_H
 *   0x46 GYRO_YOUT_L
 *   0x47 GYRO_ZOUT_H
 *   0x48 GYRO_ZOUT_L
 *
 * Efficiency comparison:
 *   - This method:    1x Transmit + 1x Receive
 *   - Separate calls: 2x Transmit + 2x Receive (accel_data() + gyro_data())
 *   - Savings: 1 I2C transaction (approximately 0.2ms at 400kHz)
 *
 * @param[out] out Pointer to imu_data_t structure to store the readings
 * @return int 0 on success, -1 if out is NULL, or HAL error code on I2C failure
 */
int imu_read_all(imu_data_t *out)
{
    /** Guard: reject null pointer to prevent undefined behavior */
    if (out == NULL) return -1;

    /** Raw byte buffer for 14 registers: 6 accel + 2 temp + 6 gyro */
    uint8_t raw[14];

    /**
     * Burst read starting from ACCEL_XOUT_H (0x3B).
     * Reads all 14 bytes in a single I2C transaction.
     */
    int ret = stm32_i2c_read(SLAVE_ADDR, MPU9250_ACCEL_XOUT_H, 14, raw);

    /** Propagate I2C error to caller */
    if (ret != 0) return ret;

    /**
     * Parse accelerometer data (bytes 0–5)
     * Each axis: high byte << 8 | low byte
     */
    int16_t ax = (int16_t)((raw[0]  << 8) | raw[1]);  /**< ACCEL_XOUT [0x3B, 0x3C] */
    int16_t ay = (int16_t)((raw[2]  << 8) | raw[3]);  /**< ACCEL_YOUT [0x3D, 0x3E] */
    int16_t az = (int16_t)((raw[4]  << 8) | raw[5]);  /**< ACCEL_ZOUT [0x3F, 0x40] */
    /** raw[6] and raw[7] = TEMP_OUT_H/L → skipped (not needed) */

    /**
     * Parse gyroscope data (bytes 8–13)
     * Each axis: high byte << 8 | low byte
     */
    int16_t gx = (int16_t)((raw[8]  << 8) | raw[9]);   /**< GYRO_XOUT [0x43, 0x44] */
    int16_t gy = (int16_t)((raw[10] << 8) | raw[11]);  /**< GYRO_YOUT [0x45, 0x46] */
    int16_t gz = (int16_t)((raw[12] << 8) | raw[13]);  /**< GYRO_ZOUT [0x47, 0x48] */

    /**
     * Convert raw values to SI units
     * Accelerometer: raw / sensitivity = g (gravitational acceleration)
     */
    out->accel.x = (float)ax / ACCEL_SENS;
    out->accel.y = (float)ay / ACCEL_SENS;
    out->accel.z = (float)az / ACCEL_SENS;

    /**
     * Gyroscope: (raw / sensitivity) * DEG_TO_RAD = rad/s (angular velocity)
     * Step 1: raw / GYRO_SENS → degrees per second
     * Step 2: * DEG_TO_RAD → radians per second
     */
    out->gyro.x  = ((float)gx / GYRO_SENS) * DEG_TO_RAD;
    out->gyro.y  = ((float)gy / GYRO_SENS) * DEG_TO_RAD;
    out->gyro.z  = ((float)gz / GYRO_SENS) * DEG_TO_RAD;

    return 0;
}

/**
 * @brief Read accelerometer data only (legacy function)
 *
 * @deprecated This function is kept for backward compatibility only.
 *             For new code, use imu_read_all() which is more efficient
 *             as it reads both sensors in a single I2C transaction.
 *
 * Internally calls imu_read_all() and extracts only the accelerometer data.
 *
 * @return vec3 Accelerometer reading in g (x, y, z components)
 *         Returns {0, 0, 0} if I2C communication fails
 */
vec3 accel_data(void)
{
    imu_data_t imu;
    if (imu_read_all(&imu) != 0) {
        /** Return zero vector on read failure */
        vec3 zero = {0.0f, 0.0f, 0.0f};
        return zero;
    }
    return imu.accel;
}

/**
 * @brief Read gyroscope data only (legacy function)
 *
 * @deprecated This function is kept for backward compatibility only.
 *             For new code, use imu_read_all() which is more efficient
 *             as it reads both sensors in a single I2C transaction.
 *
 * Internally calls imu_read_all() and extracts only the gyroscope data.
 *
 * @return vec3 Gyroscope reading in rad/s (x, y, z components)
 *         Returns {0, 0, 0} if I2C communication fails
 */
vec3 gyro_data(void)
{
    imu_data_t imu;
    if (imu_read_all(&imu) != 0) {
        /** Return zero vector on read failure */
        vec3 zero = {0.0f, 0.0f, 0.0f};
        return zero;
    }
    return imu.gyro;
}
