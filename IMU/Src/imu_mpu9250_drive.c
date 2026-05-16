#include "../../../IMU/Inc/imu_mpu9250_drive.h"

#define SLAVE_ADDR   0x68
#define ACCEL_SENS   16384.0f  // LSB/g  untuk ±2g (default)
#define GYRO_SENS    131.0f    // LSB/(°/s) untuk ±250dps (default)
#define DEG_TO_RAD   0.0174533f

// =====================================================================
// UTAMA: Baca accel + gyro sekaligus — 1 transaksi I2C, 14 byte
//
// Register map MPU9250 (berurutan):
//   0x3B ACCEL_XOUT_H
//   0x3C ACCEL_XOUT_L
//   0x3D ACCEL_YOUT_H
//   0x3E ACCEL_YOUT_L
//   0x3F ACCEL_ZOUT_H
//   0x40 ACCEL_ZOUT_L
//   0x41 TEMP_OUT_H   ← dibaca tapi diabaikan
//   0x42 TEMP_OUT_L   ← dibaca tapi diabaikan
//   0x43 GYRO_XOUT_H
//   0x44 GYRO_XOUT_L
//   0x45 GYRO_YOUT_H
//   0x46 GYRO_YOUT_L
//   0x47 GYRO_ZOUT_H
//   0x48 GYRO_ZOUT_L
//
// Dengan cara ini: 1x Transmit + 1x Receive → hemat 1 transaksi I2C
// dibanding accel_data() + gyro_data() terpisah (2x Tx + 2x Rx)
// =====================================================================
int imu_read_all(imu_data_t *out)
{
    if (out == NULL) return -1;

    uint8_t raw[14];
    int ret = stm32_i2c_read(SLAVE_ADDR, MPU9250_ACCEL_XOUT_H, 14, raw);

    if (ret != 0) return ret; // Propagate error ke caller

    // Parse Accel (byte 0–5)
    int16_t ax = (int16_t)((raw[0]  << 8) | raw[1]);
    int16_t ay = (int16_t)((raw[2]  << 8) | raw[3]);
    int16_t az = (int16_t)((raw[4]  << 8) | raw[5]);
    // raw[6] dan raw[7] = TEMP → skip

    // Parse Gyro (byte 8–13)
    int16_t gx = (int16_t)((raw[8]  << 8) | raw[9]);
    int16_t gy = (int16_t)((raw[10] << 8) | raw[11]);
    int16_t gz = (int16_t)((raw[12] << 8) | raw[13]);

    // Konversi ke satuan SI
    out->accel.x = (float)ax / ACCEL_SENS;   // g
    out->accel.y = (float)ay / ACCEL_SENS;
    out->accel.z = (float)az / ACCEL_SENS;

    out->gyro.x  = ((float)gx / GYRO_SENS) * DEG_TO_RAD;  // rad/s
    out->gyro.y  = ((float)gy / GYRO_SENS) * DEG_TO_RAD;
    out->gyro.z  = ((float)gz / GYRO_SENS) * DEG_TO_RAD;

    return 0;
}

// =====================================================================
// LEGACY: Tetap ada agar tidak break kode lama
// Gunakan imu_read_all() untuk kode baru
// =====================================================================
vec3 accel_data(void)
{
    imu_data_t imu;
    if (imu_read_all(&imu) != 0) {
        vec3 zero = {0.0f, 0.0f, 0.0f};
        return zero;
    }
    return imu.accel;
}

vec3 gyro_data(void)
{
    imu_data_t imu;
    if (imu_read_all(&imu) != 0) {
        vec3 zero = {0.0f, 0.0f, 0.0f};
        return zero;
    }
    return imu.gyro;
}
