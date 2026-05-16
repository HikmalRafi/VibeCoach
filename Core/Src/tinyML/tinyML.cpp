#include "tinyML/tinyML.h"
#include <math.h>
#include <string.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// Ambil handle timer dari main.c untuk PWM getar
extern TIM_HandleTypeDef htim1;

// Variabel Global (Hapus static agar bisa diakses getter)
WrongIntensity current_intensity = LEVEL_NONE;
float ema_score = 0.0f;

// Gunakan extern "C" untuk fungsi yang butuh koneksi ke file .c
extern "C" {
    #include "../../../IMU/Inc/imu_calibration.h"
    #include "../../../IMU/Inc/imu_mpu9250_drive.h"

    WrongIntensity get_current_intensity(void) { return current_intensity; }
    float get_ema_score(void) { return ema_score; }
}

// Konfigurasi EMA & Threshold
#define LABEL_CORRECT_MOVE  0
#define LABEL_IDLE          1
#define LABEL_WRONG_MOVE    2
#define EMA_ALPHA_ATTACK    0.80f
#define EMA_ALPHA_DECAY     0.18f
#define EMA_ALPHA_CORRECTING 0.50f
#define PWM_KOREKSI         380
#define PWM_WASPADA         700
#define PWM_BAHAYA          950

// Buffer data sensor
float sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void update_vibration(WrongIntensity level) {
    static uint32_t vib_timer = 0;
    static uint8_t pattern_step = 0;
    uint32_t now = HAL_GetTick();

    if (now < vib_timer) return;

    switch (level) {
        case LEVEL_KOREKSI:
            if (pattern_step == 0) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_KOREKSI);
                vib_timer = now + 120;
                pattern_step = 1;
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                vib_timer = now + 650;
                pattern_step = 0;
            }
            break;
        case LEVEL_BAHAYA:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_BAHAYA);
            break;
        default:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            break;
    }
}

void add_sensor_data() {
    static uint32_t last_sample = 0;
    if (HAL_GetTick() - last_sample < 10) return;
    last_sample = HAL_GetTick();

    if (!calibrated) return;

    Quaternion q = invQ();

    // Geser buffer (Rolling window)
    memmove(&sensor_buffer[0], &sensor_buffer[4], (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 4) * sizeof(float));
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE-4] = q.w;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE-3] = q.x;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE-2] = q.y;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE-1] = q.z;

    // Sederhanakan: Misal prob_wrong didapat dari classifier
    // Di sini panggil run_classifier() seperti code kamu sebelumnya

    // Logic penentuan intensity
    if (ema_score > 0.75f) current_intensity = LEVEL_BAHAYA;
    else if (ema_score > 0.25f) current_intensity = LEVEL_KOREKSI;
    else current_intensity = LEVEL_NONE;

    update_vibration(current_intensity);
}
