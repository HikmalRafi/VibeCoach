#include <fusionFilter_and_calibration/calibration_feature.h>

volatile bool calibrated = false;
volatile float q0_base;
volatile float q1_base;
volatile float q2_base;
volatile float q3_base;

// =====================================================================
// invSqrt lokal untuk normalisasi — konsisten dengan Madgwick
// =====================================================================
static float _invSqrt(float x)
{
    unsigned int i = 0x5F1F1412 - (*(unsigned int*)&x >> 1);
    float tmp = *(float*)&i;
    return tmp * (1.69000231f - 0.714158168f * x * tmp * tmp);
}

// =====================================================================
// calibrateQuaternion()
//
// FIX utama:
//   1. HAL_Delay() diganti HAL_GetTick() — CPU tidak terkunci,
//      bisa ngecek kondisi darurat kalau perlu
//   2. accel_data() + gyro_data() terpisah → imu_read_all() 1x baca
//   3. sqrt() → _invSqrt() untuk normalisasi quaternion baseline
// =====================================================================
void calibrateQuaternion(void)
{
    calibrated = true;

    ST7735_FillScreenFast(ST7735_BLACK);
    ST7735_DrawStringCenter("Ready", Font_16x26, ST7735_BLUE, ST7735_BLACK);

    // --- FASE 1: Stabilisasi sensor (200 sampel × 10ms = 2 detik) ---
    // FIX: pakai tick-based timing, bukan HAL_Delay blocking
    uint16_t stab_count = 0;
    uint32_t stab_tick  = HAL_GetTick();

    while (stab_count < 200) {
        if (HAL_GetTick() - stab_tick >= 10) {
            stab_tick = HAL_GetTick();

            // FIX: 1x baca sensor untuk accel + gyro sekaligus
            imu_data_t imu = {0};
            if (imu_read_all(&imu) == 0) {
                MadgwickAHRSupdateIMU(
                    imu.gyro.x,  imu.gyro.y,  imu.gyro.z,
                    imu.accel.x, imu.accel.y, imu.accel.z
                );
            }
            stab_count++;
        }
    }

    // --- FASE 2: Sampling pose normal (300 sampel × 10ms = 3 detik) ---
    uint16_t samples = 300;
    uint16_t counter = 0;
    float sum_q0 = 0, sum_q1 = 0, sum_q2 = 0, sum_q3 = 0;

    // Tampil countdown awal
    uint8_t second_char = '3';
    ST7735_FillScreenFast(ST7735_BLACK);
    ST7735_DrawCharCenter(second_char, Font_16x26, ST7735_YELLOW, ST7735_BLACK);

    uint32_t sample_tick = HAL_GetTick();

    while (counter < samples) {
        if (HAL_GetTick() - sample_tick >= 10) {
            sample_tick = HAL_GetTick();

            // FIX: 1x baca sensor untuk accel + gyro sekaligus
            imu_data_t imu = {0};
            if (imu_read_all(&imu) == 0) {
                MadgwickAHRSupdateIMU(
                    imu.gyro.x,  imu.gyro.y,  imu.gyro.z,
                    imu.accel.x, imu.accel.y, imu.accel.z
                );

                sum_q0 += q0;
                sum_q1 += q1;
                sum_q2 += q2;
                sum_q3 += q3;
                counter++;
            }

            // Update display countdown setiap 100 sampel
            if (counter == 100) {
                second_char = '2';
                ST7735_FillScreenFast(ST7735_BLACK);
                ST7735_DrawCharCenter(second_char, Font_16x26, ST7735_YELLOW, ST7735_BLACK);
            } else if (counter == 200) {
                second_char = '1';
                ST7735_FillScreenFast(ST7735_BLACK);
                ST7735_DrawCharCenter(second_char, Font_16x26, ST7735_YELLOW, ST7735_BLACK);
            }
        }
    }

    // --- Hitung rata-rata quaternion baseline ---
    q0_base = sum_q0 / samples;
    q1_base = sum_q1 / samples;
    q2_base = sum_q2 / samples;
    q3_base = sum_q3 / samples;

    // FIX: Normalisasi pakai _invSqrt(), konsisten dengan Madgwick
    float recip = _invSqrt(
        q0_base*q0_base + q1_base*q1_base +
        q2_base*q2_base + q3_base*q3_base
    );
    if (recip > 0.0001f) {
        q0_base *= recip;
        q1_base *= recip;
        q2_base *= recip;
        q3_base *= recip;
    }
}

// =====================================================================
// invQ() — Quaternion relatif terhadap pose baseline
//
// FIX utama:
//   1. accel_data() + gyro_data() terpisah → imu_read_all() 1x baca
//   2. sqrt() → _invSqrt() untuk normalisasi akhir
//   3. Return quaternion identity (bukan garbage) kalau belum kalibrasi
// =====================================================================
Quaternion invQ(void)
{
    Quaternion quaternion = {1.0f, 0.0f, 0.0f, 0.0f}; // FIX: init identity, bukan garbage

    if (calibrated == false) {
        return quaternion; // Kembalikan identity, bukan data acak
    }

    // FIX: 1x baca sensor untuk accel + gyro sekaligus
    imu_data_t imu = {0};
    if (imu_read_all(&imu) != 0) {
        return quaternion; // Gagal baca sensor, kembalikan identity
    }

    MadgwickAHRSupdateIMU(
        imu.gyro.x,  imu.gyro.y,  imu.gyro.z,
        imu.accel.x, imu.accel.y, imu.accel.z
    );

    // Inverse quaternion baseline (konjugat, karena unit quaternion)
    float q0i =  q0_base;
    float q1i = -q1_base;
    float q2i = -q2_base;
    float q3i = -q3_base;

    // Quaternion relatif = baseline_inv × current
    quaternion.w = q0i*q0 - q1i*q1 - q2i*q2 - q3i*q3;
    quaternion.x = q0i*q1 + q1i*q0 + q2i*q3 - q3i*q2;
    quaternion.y = q0i*q2 - q1i*q3 + q2i*q0 + q3i*q1;
    quaternion.z = q0i*q3 + q1i*q2 - q2i*q1 + q3i*q0;

    // FIX: Normalisasi pakai _invSqrt()
    float recip = _invSqrt(
        quaternion.w*quaternion.w + quaternion.x*quaternion.x +
        quaternion.y*quaternion.y + quaternion.z*quaternion.z
    );
    if (recip > 0.0001f) {
        quaternion.w *= recip;
        quaternion.x *= recip;
        quaternion.y *= recip;
        quaternion.z *= recip;
    }

    return quaternion;
}
