/**
 * @file    error_classifier.c
 * @brief   Error classification implementation for exercise form detection.
 *
 * @details Analyzes real-time IMU data (roll, pitch, yaw from Madgwick filter)
 *          to detect specific form errors during exercise training.
 *          Uses configurable thresholds for pitch, roll, and tempo detection.
 *
 * @changelog
 *   v2.0 — Threshold tuning untuk deltoid lateral raise:
 *           - PITCH_PEAK_MIN     : 55 → 45 (tidak semua orang bisa 55° dengan benar)
 *           - ROLL_SHOULDER_THRESHOLD : 28 → 25 (lebih ketat deteksi shrug)
 *           - PITCH_RATE_MAX     : 130 → 110 (lebih ketat deteksi tempo cepat)
 *           - PITCH_ACTIVE_MIN   : 15 → 12 (deteksi mulai lebih awal)
 *           - Tambah guard dt untuk mencegah spike saat resume dari idle panjang
 *           - Tambah reset peak_pitch saat error tertentu ditemukan
 */

#include "tinyML/error_classifier.h"
#include <math.h>

/* =========================================================================
 * DETECTION THRESHOLDS
 *
 * Cara tuning:
 *   1. Kirim roll, pitch, yaw via UART saat gerakan normal
 *   2. Catat nilai peak saat form benar
 *   3. Catat nilai saat sengaja melakukan kesalahan (angkat bahu, terlalu cepat)
 *   4. Set threshold di antara nilai benar dan salah
 *
 * Untuk deltoid lateral raise dengan sensor di pergelangan tangan:
 *   - Pitch meningkat saat lengan diangkat ke samping
 *   - Roll meningkat saat bahu ikut terangkat (shrug)
 *   - Pitch rate tinggi = gerakan terlalu cepat
 * ========================================================================= */

/**
 * @brief ROM minimum — pitch minimum di puncak gerakan.
 *
 * [DIUBAH] 55.0 → 45.0
 * Alasan: 55° terlalu ketat untuk mayoritas pengguna, terutama pemula.
 *         45° masih mendeteksi ROM kurang tanpa terlalu banyak false positive.
 *         Jika sensor di pergelangan dan user bisa angkat 90°, bisa dinaikkan ke 50.
 */
#define PITCH_PEAK_MIN          45.0f

/**
 * @brief Shoulder shrug — batas maksimal roll sebelum dianggap angkat bahu.
 *
 * [DIUBAH] 28.0 → 25.0
 * Alasan: 28° terlalu longgar. Angkat bahu yang signifikan biasanya mulai
 *         terdeteksi di 20-22°. 25° memberikan margin tanpa false positive.
 *         Jika terlalu sering trigger saat gerakan normal, naikkan ke 27.
 */
#define ROLL_SHOULDER_THRESHOLD 25.0f

/**
 * @brief Tempo terlalu cepat — batas pitch rate (derajat/detik).
 *
 * [DIUBAH] 130.0 → 110.0
 * Alasan: Gerakan deltoid yang aman idealnya 2-3 detik naik.
 *         Jika range gerak 60°, maka 60/3 = 20°/s normal, 60/1 = 60°/s cepat.
 *         110°/s sudah termasuk sangat cepat untuk lateral raise.
 *         Jika terlalu sering trigger saat gerakan normal, naikkan ke 120.
 */
#define PITCH_RATE_MAX          110.0f

/**
 * @brief Pitch minimum untuk mengaktifkan deteksi.
 *
 * [DIUBAH] 15.0 → 12.0
 * Alasan: Deteksi dimulai lebih awal agar error di awal gerakan bisa tertangkap.
 *         Nilai 12° masih aman dari noise saat diam (biasanya < 5°).
 */
#define PITCH_ACTIVE_MIN        12.0f

/**
 * @brief Penurunan pitch dari peak untuk trigger evaluasi ROM.
 *
 * [TIDAK DIUBAH] Tetap 10°. Menandai fase turun setelah puncak gerakan.
 */
#define PITCH_DESCENT_TRIGGER   10.0f

/**
 * @brief Batas delta-time untuk mencegah spike pitch_rate.
 *
 * [BARU] Jika dt > 200ms (misal saat resume dari idle panjang atau lag),
 *        skip perhitungan pitch_rate untuk menghindari false ERR_TEMPO_FAST.
 */
#define DT_MAX_VALID            0.20f

/* =========================================================================
 * INTERNAL STATE VARIABLES
 * ========================================================================= */

static float    last_pitch  = 0.0f;
static uint32_t last_tick   = 0;
static float    peak_pitch  = 0.0f;
static uint8_t  in_motion   = 0;

/* =========================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * ========================================================================= */

/**
 * @brief Klasifikasi error gerakan saat ini.
 *
 * Prioritas deteksi (urutan dari paling berbahaya):
 *   1. Tempo terlalu cepat
 *   2. Bahu terangkat (shrug)
 *   3. ROM kurang
 *   4. Form umum salah (fallback)
 *
 * @return ErrorType_t Jenis error yang terdeteksi
 */
ErrorType_t classify_error(void)
{
    uint32_t now = HAL_GetTick();
    float dt = (now - last_tick) / 1000.0f;

    // Skip iterasi pertama atau dt tidak valid
    if (last_tick == 0 || dt <= 0.0f) {
        last_tick  = now;
        last_pitch = pitch;
        return ERR_NONE;
    }

    // [PERBAIKAN] Guard dt maksimum:
    // Jika dt terlalu besar (lag atau resume dari idle), jangan hitung pitch_rate
    // karena akan menghasilkan nilai spike yang memicu ERR_TEMPO_FAST palsu.
    // Hanya update timestamp dan pitch, tidak kembalikan error.
    if (dt > DT_MAX_VALID) {
        last_tick  = now;
        last_pitch = pitch;
        return ERR_NONE;
    }

    last_tick = now;

    float current_pitch = pitch;
    float current_roll  = fabsf(roll);

    // ── Hitung pitch rate ─────────────────────────────────────────────────
    float pitch_rate = fabsf(current_pitch - last_pitch) / dt;
    last_pitch = current_pitch;

    // ── Cek apakah user sedang bergerak (di atas threshold aktif) ────────
    if (fabsf(current_pitch) < PITCH_ACTIVE_MIN) {
        // Kembali ke posisi istirahat — reset state untuk repetisi berikutnya
        peak_pitch = 0.0f;
        in_motion  = 0;
        return ERR_NONE;
    }

    // Tandai sedang bergerak dan update peak
    in_motion = 1;
    if (current_pitch > peak_pitch) {
        peak_pitch = current_pitch;
    }

    // ================================================================
    // DETEKSI ERROR — urutan prioritas
    // ================================================================

    // Cek 1: Tempo terlalu cepat (prioritas tertinggi, paling berbahaya)
    if (pitch_rate > PITCH_RATE_MAX) {
        // Tidak reset peak_pitch agar evaluasi ROM tetap bisa terjadi
        return ERR_TEMPO_FAST;
    }

    // Cek 2: Bahu terangkat (shrug)
    if (current_roll > ROLL_SHOULDER_THRESHOLD) {
        return ERR_SHOULDER_UP;
    }

    // Cek 3: ROM kurang — evaluasi saat sudah melewati puncak
    // Trigger: pitch mulai turun lebih dari PITCH_DESCENT_TRIGGER dari peak
    if (in_motion && current_pitch < (peak_pitch - PITCH_DESCENT_TRIGGER)) {

        if (peak_pitch < PITCH_PEAK_MIN) {
            // Peak tidak mencapai minimum — ROM kurang
            peak_pitch = 0.0f;
            in_motion  = 0;
            return ERR_ROM_LOW;
        }

        // ROM cukup — reset untuk repetisi berikutnya
        peak_pitch = 0.0f;
        in_motion  = 0;
        // Tidak return error, lanjut ke fallback di bawah
    }

    // Cek 4: Fallback — form umum salah (masih dalam gerakan, tidak ada error spesifik)
    return ERR_FORM_GENERAL;
}

/* =========================================================================
 * ERROR STRING CONVERSION
 * ========================================================================= */

/**
 * @brief Konversi ErrorType_t ke string yang ditampilkan di LCD.
 *
 * @param[in] err Jenis error
 * @return const char* String deskripsi error
 */
const char* error_to_string(ErrorType_t err)
{
    switch (err) {
        case ERR_TEMPO_FAST:   return "Slow Down!";
        case ERR_SHOULDER_UP:  return "Lower Shoulder";
        case ERR_ROM_LOW:      return "Wider Motion";
        case ERR_FORM_GENERAL: return "Fix Your Form";
        default:               return "";
    }
}
