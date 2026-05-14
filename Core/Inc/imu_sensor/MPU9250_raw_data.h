#ifndef MPU9250_RAW_DATA_H
#define MPU9250_RAW_DATA_H

#include <imu_sensor/MPU9250_RegisterMap.h>
#include <imu_sensor/stm32_mpu9250_i2c.h>
#include "main.h"

// =====================================================================
// Struct data 3 sumbu (float, satuan SI)
// =====================================================================
typedef struct {
    float x;
    float y;
    float z;
} vec3;

// =====================================================================
// Struct gabungan hasil baca 1x transaksi (lebih efisien)
// Accel: satuan g
// Gyro:  satuan rad/s
// =====================================================================
typedef struct {
    vec3 accel;  // g
    vec3 gyro;   // rad/s
} imu_data_t;

// =====================================================================
// PUBLIC API
// =====================================================================

// Baca accel + gyro sekaligus dalam 1 transaksi I2C (DISARANKAN)
// Return 0 jika sukses, non-zero jika gagal
int imu_read_all(imu_data_t *out);

// Baca accel saja (tetap ada untuk kompatibilitas)
vec3 accel_data(void);

// Baca gyro saja (tetap ada untuk kompatibilitas)
vec3 gyro_data(void);

#endif // MPU9250_RAW_DATA_H
