/**
 * @file    bsp_battery.c
 * @brief   LiPo battery reading via STM32F411 ADC
 *
 * @details Fix v4.0 - Improvements from v3.0:
 *
 *          BUGS FIXED:
 *          ┌─────────────────────────────────────────────────────────────────┐
 *          │  1. Silent ADC failure → returns UINT32_MAX as sentinel         │
 *          │     If all samples fail, cache is NOT updated                   │
 *          │                                                                 │
 *          │  2. Clarity bug: explicit is_first_run flag                     │
 *          │     Replaces bat_initialized == 0 which was prone to            │
 *          │     misinterpretation                                           │
 *          │                                                                 │
 *          │  3. Blocking 160ms on first call                                │
 *          │     Solution: call bsp_battery_force_init() in main.c           │
 *          │     before while(1) to pre-warm the cache                       │
 *          │                                                                 │
 *          │  4. Added bsp_battery_is_adc_ok() for debugging                 │
 *          └─────────────────────────────────────────────────────────────────┘
 *
 *          READING PIPELINE (unchanged):
 *          ┌─────────────────────────────────────────────────────────────────┐
 *          │  ADC Raw  →  Averaging 32 samples (5ms delay per sample)       │
 *          │           →  EMA Filter (α=0.15, long memory)                  │
 *          │           →  Hysteresis ±50mV before voltage cache update      │
 *          │           →  Hysteresis ±2% before percentage cache update     │
 *          └─────────────────────────────────────────────────────────────────┘
 */

#include "../../Bsp/Inc/bsp_battery.h"
#include "../../../Lib/ST7735/st7735.h"

/* =========================================================================
 * INTERNAL CONFIGURATION
 * ========================================================================= */

/** @brief Number of ADC samples to average. 32 samples → noise reduced ~5.6x. */
#define BAT_AVG_SAMPLES         32

/**
 * @brief Delay between ADC samples (ms).
 *
 * 5ms allows ADC settling time + SPI ripple effects to "pass" between readings.
 * DO NOT reduce below 2ms.
 */
#define BAT_SAMPLE_DELAY_MS     5

/**
 * @brief Battery reading update interval (ms).
 *
 * Total 1 read cycle = 32 × 5ms = 160ms → occurs every 5000ms.
 */
#define BSP_BAT_UPDATE_MS       5000

/**
 * @brief Alpha for EMA (Exponential Moving Average).
 *
 * α = 0.15 → slow response, very stable. Ideal for battery indicators.
 */
#define BAT_EMA_ALPHA           0.15f

/** @brief Voltage hysteresis (Volts). Cache only updates if delta >= this value. */
#define BAT_HYST_VOLTAGE        0.05f

/** @brief Percentage hysteresis. Cache only updates if delta >= this value. */
#define BAT_HYST_PCT            2

/**
 * @brief Sentinel value for ADC error.
 *
 * If bat_read_adc_averaged() returns this value, ALL samples failed.
 * Caller must skip the update and maintain the existing cache.
 */
#define BAT_ADC_ERROR           UINT32_MAX

/* =========================================================================
 * INTERNAL VARIABLES
 * ========================================================================= */

/** @brief Last valid cached voltage (after hysteresis) */
static float    bat_voltage_cached    = 0.0f;

/** @brief Internal EMA value — smoothed voltage before hysteresis */
static float    bat_voltage_ema       = 0.0f;

/** @brief Last valid cached percentage (after hysteresis) */
static uint8_t  bat_pct_cached        = 0;

/** @brief Timestamp of last ADC update */
static uint32_t bat_last_update_tick  = 0;

/** @brief Flag: at least 1 successful reading has been completed */
static uint8_t  bat_initialized       = 0;

/** @brief Flag: EMA has been seeded with an initial value */
static uint8_t  bat_ema_seeded        = 0;

/**
 * @brief Last ADC status flag.
 *
 * 1 = ADC OK, 0 = all samples failed (check hardware connection).
 * Can be read via bsp_battery_is_adc_ok() for debugging.
 */
static uint8_t  bat_adc_ok            = 0;

/* =========================================================================
 * EXTERNAL VARIABLES
 * ========================================================================= */

/** @brief ADC handle for ADC1 peripheral */
extern ADC_HandleTypeDef hadc1;

/* =========================================================================
 * HELPER FUNCTIONS
 * ========================================================================= */

/**
 * @brief Inline absolute value for float (avoids linking libm).
 * @param[in] x Input value
 * @return float Absolute value of x
 */
static inline float bat_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

/* =========================================================================
 * INTERNAL FUNCTIONS
 * ========================================================================= */

/**
 * @brief Read ADC with averaging of N samples.
 *
 * Takes multiple ADC readings and averages them to reduce noise.
 * LCD SPI noise is bursty, not continuous — the 5ms inter-sample delay
 * gives each sample a chance to be read during bus quiet periods.
 *
 * @return Averaged ADC value (0–4095),
 *         or BAT_ADC_ERROR (UINT32_MAX) if ALL samples failed.
 *
 * @note  Caller MUST check return value before using the result.
 *        Do not update cache if return value is BAT_ADC_ERROR.
 */
static uint32_t bat_read_adc_averaged(void)
{
    uint32_t sum   = 0;
    uint8_t  count = 0;

    for (uint8_t i = 0; i < BAT_AVG_SAMPLES; i++)
    {
        HAL_ADC_Start(&hadc1);

        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK)
        {
            sum += HAL_ADC_GetValue(&hadc1);
            count++;
        }

        HAL_ADC_Stop(&hadc1);

        /**
         * Inter-sample delay — DO NOT reduce below 2ms.
         * LCD SPI noise is bursty, not continuous.
         * With 5ms, each sample has a chance to be read during bus quiet periods.
         */
        HAL_Delay(BAT_SAMPLE_DELAY_MS);
    }

    /** FIX: Return sentinel if all samples failed, not 0 */
    if (count == 0)
    {
        bat_adc_ok = 0;
        return BAT_ADC_ERROR;
    }

    bat_adc_ok = 1;
    return sum / count;
}

/**
 * @brief Convert voltage to battery percentage.
 *
 * Linear mapping between BAT_V_MIN (0%) and BAT_V_MAX (100%).
 *
 * @param[in] voltage Battery voltage in Volts.
 * @return uint8_t Percentage 0–100.
 */
static uint8_t bat_voltage_to_pct(float voltage)
{
    if (voltage >= BAT_V_MAX) return 100;
    if (voltage <= BAT_V_MIN) return 0;

    float pct = ((voltage - BAT_V_MIN) / (BAT_V_MAX - BAT_V_MIN)) * 100.0f;
    return (uint8_t)pct;
}

/**
 * @brief Update cache if the update interval has elapsed — with EMA + hysteresis.
 *
 * @details Flow:
 *          1. Check BSP_BAT_UPDATE_MS interval
 *          2. Read ADC with averaging
 *          3. SKIP if ADC error (all samples failed) — maintain existing cache
 *          4. Convert to actual voltage
 *          5. Apply EMA filter
 *          6. Voltage hysteresis → update voltage cache
 *          7. Percentage hysteresis → update percentage cache
 */
static void bat_update_if_needed(void)
{
    uint32_t now = HAL_GetTick();

    /** Determine if update is needed */
    uint8_t is_first_run = (bat_initialized == 0);
    uint8_t need_update  = is_first_run
                         || ((now - bat_last_update_tick) >= BSP_BAT_UPDATE_MS);

    if (!need_update) return;

    /** === Step 1: Read ADC with averaging === */
    uint32_t adc_avg = bat_read_adc_averaged();

    /**
     * FIX: Skip update if ADC error.
     * Cache is maintained → display does not change to 0 on ADC failure.
     * Use bsp_battery_is_adc_ok() to detect this condition.
     *
     * Timestamp is still updated to avoid retrying every loop,
     * but bat_initialized is NOT set → retry will occur on next call
     * if this was the first run.
     */
    if (adc_avg == BAT_ADC_ERROR)
    {
        bat_last_update_tick = now;
        return;
    }

    /**
     * === Step 2: Convert to actual voltage ===
     *
     * Formula:
     *   v_pin = adc_count × (Vref / 4095)
     *   v_bat = v_pin × divider_factor
     *
     * Voltage divider 10kΩ + 10kΩ → factor = 2
     * (Vbat → 10k → PA0 → 10k → GND)
     *
     * Change factor if using different resistors:
     *   100k + 47k  → (100+47)/47 = 3.128f
     *   68k  + 33k  → (68+33)/33  = 3.061f
     */
    float v_raw = ((float)adc_avg * BAT_V_REF) / 4095.0f * 2.0f;

    /** Clamp to valid range */
    if (v_raw > BAT_V_MAX) v_raw = BAT_V_MAX;
    if (v_raw < 0.0f)      v_raw = 0.0f;

    /**
     * === Step 3: EMA filter ===
     * Seed with raw value on first reading to avoid transitioning from 0V.
     */
    if (!bat_ema_seeded)
    {
        bat_voltage_ema = v_raw;
        bat_ema_seeded  = 1;
    }
    else
    {
        /** ema = α × raw + (1-α) × previous_ema */
        bat_voltage_ema = (BAT_EMA_ALPHA * v_raw)
                        + ((1.0f - BAT_EMA_ALPHA) * bat_voltage_ema);
    }

    /**
     * === Step 4: Voltage hysteresis ===
     *
     * FIX: Use is_first_run (instead of bat_initialized == 0 inline)
     * for clarity — both are equivalent but this is easier to read.
     */
    float volt_diff = bat_fabsf(bat_voltage_ema - bat_voltage_cached);

    if (volt_diff >= BAT_HYST_VOLTAGE || is_first_run)
    {
        bat_voltage_cached = bat_voltage_ema;

        /**
         * === Step 5: Percentage hysteresis ===
         * Update display percentage only if change >= BAT_HYST_PCT.
         */
        uint8_t new_pct = bat_voltage_to_pct(bat_voltage_cached);

        uint8_t pct_diff = (new_pct > bat_pct_cached)
                         ? (new_pct - bat_pct_cached)
                         : (bat_pct_cached - new_pct);

        if (pct_diff >= BAT_HYST_PCT || is_first_run)
        {
            bat_pct_cached = new_pct;
        }
    }

    /** Update state — set initialized AFTER all calculations complete */
    bat_last_update_tick = now;
    bat_initialized      = 1;
}

/* =========================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================= */

/**
 * @brief Pre-warm the battery cache. Call ONCE in main.c before while(1).
 *
 * @details Without this, the 160ms blocking delay (32 samples × 5ms) occurs
 *          when this function is first called inside the render loop,
 *          which can cause LCD flicker or truncate the splash screen.
 *
 *          Usage example in main.c:
 *          @code
 *          // After all peripheral init, before while(1)
 *          bsp_battery_force_init();
 *
 *          while (1) {
 *              // bsp_battery_get_voltage() here is already non-blocking
 *          }
 *          @endcode
 */
void bsp_battery_force_init(void)
{
    /** Reset state to force bat_update_if_needed() to run */
    bat_initialized = 0;
    bat_update_if_needed();
}

/**
 * @brief Get the current battery voltage in Volts.
 *
 * @details Returns the value from the internal cache.
 *          Cache is automatically refreshed every BSP_BAT_UPDATE_MS ms.
 *          Non-blocking after bsp_battery_force_init() has been called.
 *
 * @return float Battery voltage in Volts (e.g.: 3.85f).
 *                Range: 0.0f to BAT_V_MAX.
 */
float bsp_battery_get_voltage(void)
{
    bat_update_if_needed();
    return bat_voltage_cached;
}

/**
 * @brief Get the current battery capacity percentage.
 *
 * @details Returns the value from the internal cache.
 *          Percentage is calculated linearly between BAT_V_MIN (0%) and BAT_V_MAX (100%).
 *          Non-blocking after bsp_battery_force_init() has been called.
 *
 * @return uint8_t Battery percentage (0–100).
 */
uint8_t bsp_battery_get_percentage(void)
{
    bat_update_if_needed();
    return bat_pct_cached;
}

/**
 * @brief Check if the ADC was successfully read in the last update.
 *
 * @return uint8_t 1 if ADC is OK, 0 if all samples failed.
 *
 * @note  Useful for hardware connection debugging.
 *        If always returns 0, check:
 *          - PA0 is connected to the voltage divider midpoint
 *          - STM32 GND is connected to battery GND / TP4056 B-
 *          - CubeMX: ADC1_IN0 active, PA0 mode = Analog
 */
uint8_t bsp_battery_is_adc_ok(void)
{
    return bat_adc_ok;
}

/**
 * @brief Draw a battery icon on the ST7735 LCD.
 *
 * @details Draws a 16×8 px battery icon (including positive terminal bump).
 *          Fill color adjusts according to battery level:
 *            - > 50% : Green
 *            - 20–50%: Yellow
 *            - < 20% : Red
 *
 *          Safe to call every render frame.
 *          ADC is only re-read when the update interval has elapsed.
 *
 * @param[in] x         X coordinate of the icon's top-left corner (pixels).
 * @param[in] y         Y coordinate of the icon's top-left corner (pixels).
 * @param[in] bg_color  Screen background color (RGB565),
 *                      used to clear the area inside the battery outline.
 */
void bsp_battery_draw_icon(uint16_t x, uint16_t y, uint16_t bg_color)
{
    /**
     * Call get_percentage() — ADC update only occurs if interval has elapsed.
     * Safe to call every render frame.
     */
    uint8_t pct = bsp_battery_get_percentage();

    uint16_t color_outline = ST7735_COLOR565(200, 200, 200);
    uint16_t color_fill;

    /** Color coding based on battery level */
    if (pct > 50)
        color_fill = ST7735_COLOR565(74, 222, 128);   /**< Green  > 50% */
    else if (pct > 20)
        color_fill = ST7735_COLOR565(255, 200, 0);    /**< Yellow 20–50% */
    else
        color_fill = ST7735_COLOR565(239, 68, 68);    /**< Red    < 20% */

    /** Battery body outline (14×8 px) */
    ST7735_FillRectangle(x, y, 14, 8, color_outline);

    /** Clear interior area with background color */
    ST7735_FillRectangle(x + 1, y + 1, 12, 6, bg_color);

    /** Positive terminal bump on the right side */
    ST7735_FillRectangle(x + 14, y + 2, 2, 3, color_outline);

    /** Battery fill — max 10px width */
    uint8_t fill_width = (pct * 10) / 100;

    /** Minimum 1px red if almost empty but still > 0% */
    if (fill_width == 0 && pct > 0)
    {
        fill_width = 1;
        color_fill = ST7735_COLOR565(239, 68, 68); /**< Red for critical level */
    }

    if (fill_width > 0)
    {
        ST7735_FillRectangle(x + 2, y + 2, fill_width, 4, color_fill);
    }
}
