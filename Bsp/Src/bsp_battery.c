/**
 * @file    bsp_battery.c
 * @brief   Pembacaan baterai LiPo via ADC STM32F411
 *
 * @details Fix v3.0 - Solusi battery indicator fluktuatif:
 *
 *          ROOT CAUSE fluktuasi:
 *          1. SPI/LCD aktif → arus switching → ripple di VDDA → ADC error
 *          2. Delay 1ms antar sample terlalu pendek (ADC belum settle)
 *          3. Tidak ada hysteresis → 1 count ADC noise = persentase loncat
 *          4. Tidak ada lapisan filter kedua (EMA)
 *
 *          FIX yang diterapkan:
 *          ┌─────────────────────────────────────────────────────────────┐
 *          │  ADC Raw  →  Averaging 32 sample (5ms delay per sample)    │
 *          │           →  EMA Filter (α=0.15, memori panjang)           │
 *          │           →  Hysteresis ±50mV sebelum update cache         │
 *          │           →  Hysteresis ±2% sebelum update persentase      │
 *          └─────────────────────────────────────────────────────────────┘
 *
 *          Efek: Display baterai stabil bahkan saat LCD/SPI sangat aktif.
 */

#include "../../Bsp/Inc/bsp_battery.h"
#include "../../../Lib/ST7735/st7735.h"

/* =========================================================================
 * KONFIGURASI INTERNAL
 * ========================================================================= */

/**
 * Jumlah sample ADC yang dirata-rata per pembacaan.
 * 32 sample → noise tereduksi ~5.6x dibanding 1 sample.
 */
#define BAT_AVG_SAMPLES         32

/**
 * Delay antar sample ADC dalam ms.
 * 5ms memberi waktu cukup untuk:
 *   - ADC input capacitor settle setelah MUX switch
 *   - Efek ripple SPI "menghilang" antar pembacaan
 */
#define BAT_SAMPLE_DELAY_MS     5

/**
 * Interval update pembacaan baterai dalam ms.
 * 5 detik cukup — level baterai tidak berubah drastis dalam hitungan detik.
 * Total waktu 1x baca = 32 × 5ms = 160ms → terjadi tiap 5000ms = aman.
 */
#define BSP_BAT_UPDATE_MS       5000

/**
 * Alpha untuk Exponential Moving Average (EMA).
 * Rumus: ema = α × new + (1-α) × ema_lama
 *
 * α = 0.15f → respon lambat tapi SANGAT stabil.
 *             Cocok untuk battery level yang tidak perlu cepat berubah.
 *
 * Panduan memilih α:
 *   0.05 - 0.15 : sangat stabil, lambat respon (ideal untuk battery)
 *   0.20 - 0.40 : seimbang
 *   0.50 - 1.00 : cepat respon, lebih berisik
 */
#define BAT_EMA_ALPHA           0.15f

/**
 * Hysteresis tegangan dalam Volt.
 * Voltage cache HANYA diupdate jika perubahan tegangan >= nilai ini.
 * 0.05V = 50mV → cukup untuk meredam noise ADC tanpa menunda deteksi nyata.
 */
#define BAT_HYST_VOLTAGE        0.05f

/**
 * Hysteresis persentase.
 * Persentase cache HANYA diupdate jika berubah >= nilai ini.
 * Mencegah display baterai berkedip antara mis. 74% ↔ 75%.
 */
#define BAT_HYST_PCT            2

/* =========================================================================
 * VARIABEL INTERNAL (CACHE + EMA STATE)
 * ========================================================================= */

/** Cache voltage terakhir yang di-display (setelah hysteresis) */
static float    bat_voltage_cached    = 0.0f;

/** Nilai EMA internal — tracking tegangan smooth, belum tentu di-display */
static float    bat_voltage_ema       = 0.0f;

/** Cache persentase terakhir yang di-display (setelah hysteresis) */
static uint8_t  bat_pct_cached        = 0;

/** Timestamp terakhir update ADC */
static uint32_t bat_last_update_tick  = 0;

/** Flag: sudah pernah dibaca minimal 1x */
static uint8_t  bat_initialized       = 0;

/** Flag: EMA sudah di-seed dengan nilai awal */
static uint8_t  bat_ema_seeded        = 0;

/* =========================================================================
 * VARIABEL EXTERNAL
 * ========================================================================= */

extern ADC_HandleTypeDef hadc1;

/* =========================================================================
 * HELPER: NILAI ABSOLUT FLOAT (tanpa import math.h)
 * ========================================================================= */

static inline float bat_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

/* =========================================================================
 * FUNGSI INTERNAL
 * ========================================================================= */

/**
 * @brief Baca ADC dengan averaging N sample.
 *
 * @details Menggunakan BAT_AVG_SAMPLES pembacaan dengan jeda BAT_SAMPLE_DELAY_MS
 *          antar sample. Jeda ini penting agar:
 *          1. Input capacitor ADC punya waktu settle setelah mux switch
 *          2. Efek burst SPI yang menyebabkan noise bisa "lewat" antar sample
 *
 * @return Nilai ADC rata-rata (0–4095), atau 0 jika semua sample gagal.
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

        /*
         * Jeda antar sample — JANGAN dikecilkan di bawah 2ms.
         * Noise dari SPI LCD bersifat burst, bukan kontinyu.
         * Dengan jeda 5ms, setiap sample punya kesempatan
         * dibaca saat bus relatif tenang.
         */
        HAL_Delay(BAT_SAMPLE_DELAY_MS);
    }

    if (count == 0) return 0;

    return sum / count;
}

/**
 * @brief Hitung persentase dari tegangan.
 *
 * @param voltage Tegangan dalam Volt.
 * @return Persentase 0–100.
 */
static uint8_t bat_voltage_to_pct(float voltage)
{
    if (voltage >= BAT_V_MAX) return 100;
    if (voltage <= BAT_V_MIN) return 0;

    float pct = ((voltage - BAT_V_MIN) / (BAT_V_MAX - BAT_V_MIN)) * 100.0f;
    return (uint8_t)pct;
}

/**
 * @brief Update cache jika sudah waktunya — dengan EMA + hysteresis.
 *
 * @details Flow update:
 *          1. Cek apakah interval BSP_BAT_UPDATE_MS sudah lewat
 *          2. Baca ADC dengan averaging
 *          3. Konversi ke tegangan aktual
 *          4. Terapkan EMA filter (smooth)
 *          5. Hysteresis tegangan: update cache hanya jika berubah >= BAT_HYST_VOLTAGE
 *          6. Hysteresis persentase: update display pct hanya jika berubah >= BAT_HYST_PCT
 */
static void bat_update_if_needed(void)
{
    uint32_t now = HAL_GetTick();

    /* Cek apakah perlu update */
    uint8_t need_update = 0;

    if (bat_initialized == 0)
    {
        need_update = 1;
    }
    else if ((now - bat_last_update_tick) >= BSP_BAT_UPDATE_MS)
    {
        need_update = 1;
    }

    if (!need_update) return;

    /* === Step 1: Baca ADC dengan averaging === */
    uint32_t adc_avg = bat_read_adc_averaged();

    /*
     * === Step 2: Konversi ke tegangan aktual ===
     *
     * Formula:
     *   v_pin = adc_count × (Vref / 4095)
     *   v_bat = v_pin × faktor_divider
     *
     * Voltage divider 10kΩ + 10kΩ → faktor = 2
     * (Vbat → 10k → pin_ADC → 10k → GND)
     *
     * Jika resistor kamu berbeda, ubah faktor perkalian:
     *   100k + 47k  → faktor = (100+47)/47 = 3.128f
     *   68k  + 33k  → faktor = (68+33)/33  = 3.061f
     */
    float v_raw = ((float)adc_avg * BAT_V_REF) / 4095.0f * 2.0f;

    /* Clamp range agar tidak keluar batas */
    if (v_raw > BAT_V_MAX) v_raw = BAT_V_MAX;
    if (v_raw < 0.0f)      v_raw = 0.0f;

    /* === Step 3: EMA filter ===
     *
     * Pertama kali: seed EMA dengan nilai raw langsung
     * agar tidak ada transisi panjang dari 0V ke tegangan aktual.
     */
    if (!bat_ema_seeded)
    {
        bat_voltage_ema = v_raw;
        bat_ema_seeded  = 1;
    }
    else
    {
        /* ema = α × raw + (1-α) × ema_lama */
        bat_voltage_ema = (BAT_EMA_ALPHA * v_raw)
                        + ((1.0f - BAT_EMA_ALPHA) * bat_voltage_ema);
    }

    /* === Step 4: Hysteresis tegangan ===
     *
     * Jangan update voltage cache jika EMA berubah < BAT_HYST_VOLTAGE.
     * Ini mencegah nilai voltage "bergetar" di display.
     */
    float volt_diff = bat_fabsf(bat_voltage_ema - bat_voltage_cached);

    if (volt_diff >= BAT_HYST_VOLTAGE || bat_initialized == 0)
    {
        bat_voltage_cached = bat_voltage_ema;

        /* === Step 5: Hysteresis persentase ===
         *
         * Hitung persentase baru, tapi update display
         * hanya jika berubah >= BAT_HYST_PCT persen.
         */
        uint8_t new_pct = bat_voltage_to_pct(bat_voltage_cached);

        uint8_t pct_diff = (new_pct > bat_pct_cached)
                         ? (new_pct - bat_pct_cached)
                         : (bat_pct_cached - new_pct);

        if (pct_diff >= BAT_HYST_PCT || bat_initialized == 0)
        {
            bat_pct_cached = new_pct;
        }
    }

    /* Update state */
    bat_last_update_tick = now;
    bat_initialized      = 1;
}

/* =========================================================================
 * FUNGSI PUBLIK
 * ========================================================================= */

float bsp_battery_get_voltage(void)
{
    bat_update_if_needed();
    return bat_voltage_cached;
}

uint8_t bsp_battery_get_percentage(void)
{
    bat_update_if_needed();
    return bat_pct_cached;
}

void bsp_battery_draw_icon(uint16_t x, uint16_t y, uint16_t bg_color)
{
    /*
     * Cukup panggil get_percentage() — update ADC hanya terjadi
     * jika memang sudah waktunya (interval 5 detik).
     * Aman dipanggil setiap frame render header.
     */
    uint8_t pct = bsp_battery_get_percentage();

    /* Warna outline selalu abu-abu terang */
    uint16_t color_outline = ST7735_COLOR565(200, 200, 200);
    uint16_t color_fill;

    /* Tentukan warna isi berdasarkan level */
    if (pct > 50)
    {
        color_fill = ST7735_COLOR565(74, 222, 128);   /* Hijau  > 50% */
    }
    else if (pct > 20)
    {
        color_fill = ST7735_COLOR565(255, 200, 0);    /* Kuning 20-50% */
    }
    else
    {
        color_fill = ST7735_COLOR565(239, 68, 68);    /* Merah  < 20% */
    }

    /* Body luar baterai (14x8 px) */
    ST7735_FillRectangle(x, y, 14, 8, color_outline);

    /* Area dalam dikosongkan dengan warna background */
    ST7735_FillRectangle(x + 1, y + 1, 12, 6, bg_color);

    /* Tonjolan kutub (+) di kanan */
    ST7735_FillRectangle(x + 14, y + 2, 2, 3, color_outline);

    /* Isi baterai — maks 10px di dalam outline */
    uint8_t fill_width = (pct * 10) / 100;

    /* Minimal 1px merah jika hampir habis tapi > 0% */
    if (fill_width == 0 && pct > 0)
    {
        fill_width = 1;
        color_fill = ST7735_COLOR565(239, 68, 68);
    }

    if (fill_width > 0)
    {
        ST7735_FillRectangle(x + 2, y + 2, fill_width, 4, color_fill);
    }
}
