#include "tinyML/error_classifier.h"
#include <math.h>

// =====================================================================
// THRESHOLD — tuning di sini kalau kurang pas di badan kamu
//
// Cara tuning:
//   1. Pakai UART print roll, pitch, yaw saat gerak normal
//   2. Catat nilai puncak saat gerakan benar
//   3. Catat nilai saat sengaja salah (bahu naik, terlalu cepat, dll)
//   4. Set threshold di antara keduanya
// =====================================================================

// Lateral Raise — tangan naik ke samping → pitch naik
#define PITCH_PEAK_MIN      55.0f   // derajat minimum saat puncak gerakan
                                    // kalau tidak tercapai = ROM kurang

// Bahu naik — biasanya roll keluar dari range normal
#define ROLL_BAHU_THRESHOLD 28.0f   // derajat, lebih dari ini = bahu naik

// Tempo — seberapa cepat pitch boleh berubah
#define PITCH_RATE_MAX      130.0f  // derajat/detik, lebih dari ini = terlalu cepat

// Minimum pitch untuk mulai deteksi (hindari false positive saat idle)
#define PITCH_ACTIVE_MIN    15.0f

// =====================================================================
// State internal
// =====================================================================
static float   last_pitch     = 0.0f;
static uint32_t last_tick     = 0;
static float   peak_pitch     = 0.0f;     // pitch tertinggi dalam 1 rep
static uint8_t rep_in_motion  = 0;        // 1 = sedang gerak naik

// =====================================================================
// classify_error()
//
// Dipanggil setelah model bilang SALAH (current_intensity > LEVEL_NONE)
// Menggunakan roll, pitch, yaw dari computeAngles() yang sudah ada
// di Madgwick kamu
//
// Return: jenis kesalahan yang paling mungkin
// =====================================================================
ErrorType_t classify_error(void)
{
    // Hitung dt
    uint32_t now = HAL_GetTick();
    float dt = (now - last_tick) / 1000.0f;
    if (last_tick == 0 || dt <= 0.0f || dt > 0.5f) {
        // Skip iterasi pertama atau kalau dt tidak wajar
        last_tick  = now;
        last_pitch = pitch;
        return ERR_NONE;
    }
    last_tick = now;

    // Ambil nilai angle saat ini dari variabel global Madgwick
    // (roll, pitch, yaw sudah dihitung di computeAngles())
    float current_pitch = pitch;
    float current_roll  = fabsf(roll);

    // --- Deteksi tempo ---
    float pitch_rate = fabsf(current_pitch - last_pitch) / dt;
    last_pitch = current_pitch;

    // Hanya deteksi saat user sedang aktif bergerak
    if (fabsf(current_pitch) < PITCH_ACTIVE_MIN) {
        peak_pitch    = 0.0f;   // reset peak saat kembali ke posisi awal
        rep_in_motion = 0;
        return ERR_NONE;
    }

    // Track peak pitch dalam 1 rep
    rep_in_motion = 1;
    if (current_pitch > peak_pitch) {
        peak_pitch = current_pitch;
    }

    // ----------------------------------------------------------------
    // PRIORITAS DETEKSI:
    // 1. Tempo (paling bahaya, cek duluan)
    // 2. Bahu naik
    // 3. ROM kurang (cek saat sudah melewati puncak)
    // 4. Form umum (fallback)
    // ----------------------------------------------------------------

    if (pitch_rate > PITCH_RATE_MAX) {
        return ERR_TEMPO_CEPAT;
    }

    if (current_roll > ROLL_BAHU_THRESHOLD) {
        return ERR_BAHU_NAIK;
    }

    // Cek ROM — evaluasi saat pitch mulai turun lagi (sudah lewat puncak)
    if (rep_in_motion && current_pitch < (peak_pitch - 10.0f)) {
        if (peak_pitch < PITCH_PEAK_MIN) {
            peak_pitch    = 0.0f;
            rep_in_motion = 0;
            return ERR_ROM_KURANG;
        }
        peak_pitch    = 0.0f;
        rep_in_motion = 0;
    }

    return ERR_FORM_UMUM;
}

// =====================================================================
// String untuk ditampilkan ke OLED
// =====================================================================
const char* error_to_string(ErrorType_t err)
{
    switch (err) {
        case ERR_TEMPO_CEPAT: return "Lebih Lambat!";
        case ERR_BAHU_NAIK:   return "Turunkan Bahu";
        case ERR_ROM_KURANG:  return "Gerak Lebih Lebar";
        case ERR_FORM_UMUM:   return "Perbaiki Form";
        default:              return "";
    }
}
