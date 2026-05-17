#include "../../../IMU/Inc/imu_calibration.h"

/** @brief Global flag: true if calibration baseline has been captured and is ready to use. */
volatile bool calibrated = false;

/**
 * @brief Baseline quaternion representing the user's "normal" pose.
 * @details These four values are the reference orientation captured during calibration.
 *          They are used by invQ() to subtract the normal pose from all subsequent
 *          sensor readings, making motion tracking relative instead of absolute.
 */
volatile float q0_base;
volatile float q1_base;
volatile float q2_base;
volatile float q3_base;

/**
 * @brief Timer handle for vibration motor PWM output.
 * @details TIM1 Channel 1 is configured for PWM and connected to the haptic motor driver.
 *          Writing to the compare register controls vibration intensity (0 = off).
 *          Declared extern — the actual definition is in the CubeMX-generated main file.
 */
extern TIM_HandleTypeDef htim1;

/**
 * @brief Fast inverse square root approximation (Quake III algorithm).
 * @details Computes 1/sqrt(x) roughly 4x faster than the standard library.
 *          Used here for quaternion normalisation (scaling to unit length).
 *          The "magic constant" 0x5F1F1412 provides a good initial guess,
 *          then one Newton-Raphson iteration refines the result.
 * @param x Input value (must be positive)
 * @return Approximated 1/sqrt(x)
 */
static float _invSqrt(float x)
{
    unsigned int i = 0x5F1F1412 - (*(unsigned int*)&x >> 1);
    float tmp = *(float*)&i;
    return tmp * (1.69000231f - 0.714158168f * x * tmp * tmp);
}

/** @brief PWM duty for countdown pulses. 700/999 ≈ 70% intensity. Increase if too weak. */
#define CALIB_VIB_TICK      700
/** @brief PWM duty for the completion pulse. 900/999 ≈ 90% intensity — hard to miss. */
#define CALIB_VIB_DONE      900

/**
 * @brief Non-blocking vibration state machine context.
 * @details Manages the ON/OFF timing for haptic pulses without using HAL_Delay(),
 *          which would block sensor sampling. Each pulse is 200ms ON then 200ms OFF.
 *          Call vib_arm() once to start, then vib_update() in a loop.
 */
typedef struct {
    uint8_t  pulses_total;   /**< How many pulses we want to emit (always 1 for our countdown). */
    uint8_t  pulses_sent;    /**< How many pulses have been completed so far. */
    bool     motor_on;       /**< Current motor state: true = vibrating, false = idle. */
    uint32_t last_tick;      /**< HAL_GetTick() timestamp of the last ON↔OFF transition. */
    bool     done;           /**< Set to true when all requested pulses have finished. */
} VibState;

/**
 * @brief Arm the vibration state machine for a new sequence.
 * @details Resets all counters and prepares to emit the requested number of pulses.
 *          Call this once before entering the loop that calls vib_update().
 * @param v      Pointer to the state machine instance
 * @param pulses Number of pulses to emit (pass 0 to skip vibration entirely)
 */
static void vib_arm(VibState *v, uint8_t pulses)
{
    v->pulses_total = pulses;
    v->pulses_sent  = 0;
    v->motor_on     = false;
    v->last_tick    = HAL_GetTick();
    v->done         = (pulses == 0);  // If 0 pulses requested, mark done immediately
}

/**
 * @brief Tick the vibration state machine (call every loop iteration).
 * @details Checks elapsed time and transitions between ON (motor running, 200ms)
 *          and OFF (motor idle, 200ms). Increments the sent-pulse counter each
 *          time a full ON→OFF cycle completes. Marks itself done when finished.
 *          This is non-blocking — it returns immediately so the caller can do
 *          other work (like reading IMU data at 100Hz).
 * @param v Pointer to the state machine instance
 */
static void vib_update(VibState *v)
{
    if (v->done) return;  // Nothing to do

    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - v->last_tick;

    if (!v->motor_on) {
        // Currently OFF — wait 200ms, then turn motor ON
        if (elapsed >= 200) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, CALIB_VIB_TICK);
            v->motor_on  = true;
            v->last_tick = now;
        }
    } else {
        // Currently ON — wait 200ms, then turn motor OFF
        if (elapsed >= 200) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            v->motor_on = false;
            v->last_tick = now;
            v->pulses_sent++;
            if (v->pulses_sent >= v->pulses_total) {
                v->done = true;  // All pulses delivered
            }
        }
    }
}

/**
 * @brief Run the full IMU calibration routine.
 * @details Guides the user to hold the device in their desired "normal" pose while
 *          the system captures and stores the baseline orientation. The procedure
 *          has three phases totalling ~5 seconds:
 *
 *          Phase 1 — Stabilisation (2s):
 *          Feeds 200 samples into the Madgwick filter so it converges from its
 *          initial state. These samples are not accumulated.
 *
 *          Phase 2 — Sampling + Countdown (3s):
 *          Displays "3", "2", "1" on screen with one vibration pulse per digit.
 *          Collects 300 quaternion samples at 100Hz and accumulates them.
 *
 *          Phase 3 — Baseline computation:
 *          Averages the 300 quaternions and normalises the result to unit length.
 *          Stores the baseline in q0_base..q3_base for later use by invQ().
 *
 *          A 600ms strong vibration signals completion.
 */
void calibrateQuaternion(void)
{
    calibrated = true;  // Set immediately — baseline will be ready after this function

    // Show initial prompt on the display
    ST7735_FillScreenFast(ST7735_BLACK);
    ST7735_DrawStringCenter("Ready", Font_16x26, ST7735_BLUE, ST7735_BLACK);

    /* ---------- PHASE 1: Stabilisation (200 × 10ms = 2s) ---------- */
    // Let the Madgwick filter settle before we start recording
    uint16_t stab_count = 0;
    uint32_t stab_tick  = HAL_GetTick();

    while (stab_count < 200) {
        if (HAL_GetTick() - stab_tick >= 10) {
            stab_tick = HAL_GetTick();
            imu_data_t imu = {0};
            if (imu_read_all(&imu) == 0) {
                // Feed sensor data into the filter (updates global q0..q3)
                MadgwickAHRSupdateIMU(
                    imu.gyro.x, imu.gyro.y, imu.gyro.z,
                    imu.accel.x, imu.accel.y, imu.accel.z
                );
            }
            stab_count++;
        }
    }

    /* ---------- PHASE 2: Sampling (300 × 10ms = 3s) + Countdown ---------- */
    // Record quaternions while showing "3" → "2" → "1" with vibration feedback
    uint16_t samples = 300;
    uint16_t counter = 0;
    float    sum_q0  = 0.0f, sum_q1 = 0.0f,
             sum_q2  = 0.0f, sum_q3 = 0.0f;  // Accumulators for averaging

    uint8_t current_second = 3;  // Start countdown at 3
    ST7735_FillScreenFast(ST7735_BLACK);
    ST7735_DrawCharCenter('3', Font_16x26, ST7735_YELLOW, ST7735_BLACK);

    VibState vib;
    vib_arm(&vib, 1);   // One vibration pulse for the "3" display
    uint32_t sample_tick = HAL_GetTick();

    while (counter < samples) {
        vib_update(&vib);  // Keep the vibration state machine ticking

        if (HAL_GetTick() - sample_tick >= 10) {
            sample_tick = HAL_GetTick();

            imu_data_t imu = {0};
            if (imu_read_all(&imu) == 0) {
                // Update filter and accumulate the resulting quaternion
                MadgwickAHRSupdateIMU(
                    imu.gyro.x, imu.gyro.y, imu.gyro.z,
                    imu.accel.x, imu.accel.y, imu.accel.z
                );
                sum_q0 += q0;
                sum_q1 += q1;
                sum_q2 += q2;
                sum_q3 += q3;
                counter++;
            }

            // Switch display every 100 samples (= 1 second at 100Hz)
            if (counter == 100 && current_second != 2) {
                current_second = 2;
                ST7735_FillScreenFast(ST7735_BLACK);
                ST7735_DrawCharCenter('2', Font_16x26, ST7735_YELLOW, ST7735_BLACK);
                vib_arm(&vib, 1);  // Pulse for "2"
            } else if (counter == 200 && current_second != 1) {
                current_second = 1;
                ST7735_FillScreenFast(ST7735_BLACK);
                ST7735_DrawCharCenter('1', Font_16x26, ST7735_YELLOW, ST7735_BLACK);
                vib_arm(&vib, 1);  // Pulse for "1"
            }
        }
    }

    // Safety: force motor off before the long completion pulse
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

    /* ---------- Completion: 600ms strong vibration (blocking) ---------- */
    // Blocking delay is fine here — sampling is already complete
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, CALIB_VIB_DONE);
    HAL_Delay(600);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

    /* ---------- PHASE 3: Average & normalise baseline quaternion ---------- */
    // Compute the mean orientation from all 300 samples
    q0_base = sum_q0 / (float)samples;
    q1_base = sum_q1 / (float)samples;
    q2_base = sum_q2 / (float)samples;
    q3_base = sum_q3 / (float)samples;

    // Normalise to unit length (a valid rotation quaternion must have ||q|| = 1)
    float recip = _invSqrt(
        q0_base*q0_base + q1_base*q1_base +
        q2_base*q2_base + q3_base*q3_base
    );

    if (recip > 0.0001f) {  // Guard against division by zero
        q0_base *= recip;
        q1_base *= recip;
        q2_base *= recip;
        q3_base *= recip;
    }
}

/**
 * @brief Get the corrected orientation relative to the calibrated baseline.
 * @details Reads the current IMU data, runs the Madgwick filter, then removes
 *          the baseline orientation so that the user's "normal" pose maps to
 *          the identity quaternion (no rotation). Mathematically:
 *
 *          q_result = conj(q_base) ⊗ q_current
 *
 *          This makes all subsequent motion tracking relative to how the user
 *          was holding the device during calibration.
 * @return Normalised quaternion representing the corrected orientation.
 *         Returns identity (1,0,0,0) if calibration hasn't been run or IMU read fails.
 */
Quaternion invQ(void)
{
    Quaternion quaternion = {1.0f, 0.0f, 0.0f, 0.0f};  // Identity fallback

    // Guard: don't try to correct if calibration hasn't been performed
    if (calibrated == false) {
        return quaternion;
    }

    // Read current sensor data
    imu_data_t imu = {0};
    if (imu_read_all(&imu) != 0) {
        return quaternion;  // I2C/SPI read error — return identity
    }

    // Update orientation filter with fresh sensor data
    MadgwickAHRSupdateIMU(
        imu.gyro.x, imu.gyro.y, imu.gyro.z,
        imu.accel.x, imu.accel.y, imu.accel.z
    );

    // Conjugate of the baseline quaternion (invert x,y,z, keep w)
    float q0i =  q0_base;   // w stays the same
    float q1i = -q1_base;   // x negated
    float q2i = -q2_base;   // y negated
    float q3i = -q3_base;   // z negated

    // Hamilton product: q_corrected = q_base_conjugate ⊗ q_current
    // This "subtracts" the baseline rotation from the current reading
    quaternion.w = q0i*q0 - q1i*q1 - q2i*q2 - q3i*q3;
    quaternion.x = q0i*q1 + q1i*q0 + q2i*q3 - q3i*q2;
    quaternion.y = q0i*q2 - q1i*q3 + q2i*q0 + q3i*q1;
    quaternion.z = q0i*q3 + q1i*q2 - q2i*q1 + q3i*q0;

    // Normalise the result quaternion to unit length
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
