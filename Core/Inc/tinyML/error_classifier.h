#ifndef ERROR_CLASSIFIER_H
#define ERROR_CLASSIFIER_H

#include "main.h"

#include "../../../IMU/Inc/imu_madgwick_filter.h"  // untuk roll, pitch, yaw

// =====================================================================
// Jenis kesalahan yang bisa dideteksi
// =====================================================================
typedef enum {
    ERR_NONE = 0,      // Gerakan benar
    ERR_TEMPO_CEPAT,   // Gerakan terlalu cepat
    ERR_BAHU_NAIK,     // Bahu terangkat (shoulder shrug)
    ERR_ROM_KURANG,    // Range of motion kurang, tidak sampai puncak
    ERR_FORM_UMUM,     // Salah tapi tidak masuk kategori spesifik
} ErrorType_t;

// =====================================================================
// PUBLIC
// =====================================================================
ErrorType_t classify_error(void);
const char* error_to_string(ErrorType_t err);

#endif
