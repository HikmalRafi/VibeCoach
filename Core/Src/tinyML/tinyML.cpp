#include <tinyML/tinyML.h>
ErrorType_t current_error = ERR_NONE;

extern "C" {
    #include <fusionFilter_and_calibration/calibration_feature.h>
    #include <imu_sensor/MPU9250_raw_data.h>
}

// ================================================================
// KONFIGURASI
// ================================================================

#define LABEL_CORRECT_MOVE  0
#define LABEL_IDLE          1
#define LABEL_WRONG_MOVE    2

#define AI_INTERVAL_TICKS   10

#define EMA_ALPHA_ATTACK    0.80f
#define EMA_ALPHA_DECAY     0.18f
#define EMA_ALPHA_CORRECTING 0.50f  // Decay lebih cepat saat user sedang koreksi arah

#define CONFIDENCE_MIN      0.60f

#define THRESHOLD_KOREKSI   0.25f
#define THRESHOLD_WASPADA   0.50f
#define THRESHOLD_BAHAYA    0.75f

// Tuning feel getaran — naikkan semua PWM biar lebih "berasa"
#define PWM_KOREKSI         380     // Naik dari 300
#define PWM_WASPADA         700     // Naik dari 620
#define PWM_BAHAYA          950     // Tetap max

// Delta quaternion threshold untuk deteksi "sedang koreksi"
// Semakin kecil = lebih sensitif deteksi gerak koreksi
#define CORRECTION_DELTA_THRESHOLD  0.015f

// ================================================================
// VIBRATION ENGINE — Pola diperbarui agar lebih "berasa"
// ================================================================

void update_vibration(WrongIntensity level) {
    static uint32_t       vib_timer    = 0;
    static uint8_t        pattern_step = 0;
    static WrongIntensity last_level   = LEVEL_NONE;

    uint32_t now = HAL_GetTick();

    if (level != last_level) {
        pattern_step = 0;
        vib_timer    = now;
        last_level   = level;

        if (level == LEVEL_NONE) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            return;
        }
    }

    if (now < vib_timer) return;

    switch (level) {

        // KOREKSI: Satu denyut pendek, jeda panjang
        // Feel: "tik" ... "tik" ... "tik"
        case LEVEL_KOREKSI:
            if (pattern_step == 0) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_KOREKSI);
                vib_timer = now + 120;  // Sedikit lebih panjang agar terasa
                pattern_step = 1;
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                vib_timer = now + 650;
                pattern_step = 0;
            }
            break;

        // WASPADA: Triple pulse, lebih tegas dari sebelumnya
        // Feel: "tit-tit-tit" ... "tit-tit-tit"
        case LEVEL_WASPADA:
            switch (pattern_step) {
                case 0: case 2: case 4:
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_WASPADA);
                    vib_timer = now + 110;
                    pattern_step++;
                    break;
                case 1: case 3:
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                    vib_timer = now + 70;
                    pattern_step++;
                    break;
                case 5:
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                    vib_timer = now + 400;
                    pattern_step = 0;
                    break;
            }
            break;

        // BAHAYA: Buzz panjang, jeda sangat pendek
        // Feel: getar terus tidak berhenti
        case LEVEL_BAHAYA:
            if (pattern_step == 0) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_BAHAYA);
                vib_timer = now + 350;
                pattern_step = 1;
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
                vib_timer = now + 60;
                pattern_step = 0;
            }
            break;

        default:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            break;
    }
}

// ================================================================
// DIRECTION-AWARE EMA
// Kunci fix masalah getar saat turun dari posisi terlalu tinggi
// ================================================================

static float ema_score = 0.0f;

// Deteksi apakah user sedang "koreksi arah" dari posisi salah
// Caranya: hitung delta quaternion antara frame sekarang dan sebelumnya
// Kalau prob_wrong turun DAN quaternion bergerak signifikan
// → user aktif koreksi → decay lebih cepat

static bool is_user_correcting(float prob_wrong,
                                float prev_prob_wrong,
                                Quaternion q_now,
                                Quaternion q_prev) {
    // Cek prob_wrong sedang turun
    bool prob_falling = (prev_prob_wrong - prob_wrong) > 0.15f;

    // Cek quaternion bergerak (user aktif bergerak, bukan diam)
    float dw = q_now.w - q_prev.w;
    float dx = q_now.x - q_prev.x;
    float dy = q_now.y - q_prev.y;
    float dz = q_now.z - q_prev.z;
    float delta_mag = sqrtf(dw*dw + dx*dx + dy*dy + dz*dz);
    bool  is_moving = delta_mag > CORRECTION_DELTA_THRESHOLD;

    // User koreksi = prob wrong turun + aktif bergerak
    return (prob_falling && is_moving);
}

static float ema_update(float new_sample, bool correcting) {
    float alpha;
    if (new_sample > ema_score) {
        alpha = EMA_ALPHA_ATTACK;       // Naik cepat saat salah
    } else if (correcting) {
        alpha = EMA_ALPHA_CORRECTING;   // Turun lebih cepat saat koreksi
    } else {
        alpha = EMA_ALPHA_DECAY;        // Turun pelan saat biasa
    }

    ema_score = alpha * new_sample + (1.0f - alpha) * ema_score;
    if (ema_score < 0.05f) ema_score = 0.0f;
    return ema_score;
}

// ================================================================
// MAIN LOOP
// ================================================================

float sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void add_sensor_data() {
    static uint32_t       last_sample_tick   = 0;
    static int            ai_tick            = 0;
    static float          prev_wrong_prob    = 0.0f;
    static uint8_t        falling_edge_count = 0;
    static Quaternion     q_prev             = {1.0f, 0.0f, 0.0f, 0.0f};
    static WrongIntensity current_intensity  = LEVEL_NONE;

    uint32_t now = HAL_GetTick();
    if (now - last_sample_tick < 10) return;
    last_sample_tick = now;

    if (!calibrated) {
        calibrateQuaternion();
        return;
    }

    Quaternion q = invQ();

    memmove(&sensor_buffer[0],
            &sensor_buffer[4],
            (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 4) * sizeof(float));
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 4] = q.w;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 3] = q.x;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 2] = q.y;
    sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 1] = q.z;

    if (++ai_tick >= AI_INTERVAL_TICKS) {
        ai_tick = 0;

        signal_t signal;
        numpy::signal_from_buffer(sensor_buffer,
                                  EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
                                  &signal);
        ei_impulse_result_t result = {0};
        run_classifier(&signal, &result, false);

        float prob_correct = result.classification[LABEL_CORRECT_MOVE].value;
        float prob_idle    = result.classification[LABEL_IDLE].value;
        float prob_wrong   = result.classification[LABEL_WRONG_MOVE].value;

        float max_val = prob_correct;
        int   max_idx = LABEL_CORRECT_MOVE;
        if (prob_idle  > max_val) { max_val = prob_idle;  max_idx = LABEL_IDLE; }
        if (prob_wrong > max_val) { max_val = prob_wrong; max_idx = LABEL_WRONG_MOVE; }

        // Scoring
        float raw_score;
        if (max_val >= CONFIDENCE_MIN) {
            raw_score = (max_idx == LABEL_WRONG_MOVE) ? prob_wrong : 0.0f;
        } else {
            raw_score = prob_wrong * 0.75f;
        }

        // Falling edge — fix ekor getaran keras
        float delta = prev_wrong_prob - prob_wrong;
        if (delta > 0.30f) {
            falling_edge_count++;
            if (falling_edge_count >= 2) {
                ema_score *= 0.3f;
                falling_edge_count = 0;
            }
        } else {
            falling_edge_count = 0;
        }

        // Deteksi apakah user sedang aktif koreksi arah
        bool correcting = is_user_correcting(prob_wrong, prev_wrong_prob, q, q_prev);

        prev_wrong_prob = prob_wrong;
        q_prev          = q;

        float smoothed = ema_update(raw_score, correcting);

        if      (smoothed >= THRESHOLD_BAHAYA)  current_intensity = LEVEL_BAHAYA;
        else if (smoothed >= THRESHOLD_WASPADA) current_intensity = LEVEL_WASPADA;
        else if (smoothed >= THRESHOLD_KOREKSI) current_intensity = LEVEL_KOREKSI;
        else                                    current_intensity = LEVEL_NONE;
    }

    update_vibration(current_intensity);
    computeAngles(); // hitung roll, pitch, yaw dari quaternion saat ini

    if (current_intensity > LEVEL_NONE) {
        current_error = classify_error();
    } else {
        current_error = ERR_NONE;
    }

}

// Getter agar start_training.c bisa akses data internal
static WrongIntensity current_intensity = LEVEL_NONE; // pastikan ini tidak static local

WrongIntensity get_current_intensity(void) {
    return current_intensity;
}

float get_ema_score(void) {
    return ema_score;
}
