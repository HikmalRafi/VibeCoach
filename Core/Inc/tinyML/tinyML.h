#ifndef TINYML_H
#define TINYML_H

#include "main.h"
#include <stdarg.h>

// Enum ini harus terlihat oleh C dan C++
typedef enum {
    LEVEL_NONE = 0,
    LEVEL_KOREKSI,
    LEVEL_WASPADA,
    LEVEL_BAHAYA
} WrongIntensity;

#ifdef __cplusplus
extern "C" {
#endif

// Daftar fungsi yang akan dipanggil di main_menu.c
void add_sensor_data(void);
void update_vibration(WrongIntensity level);
WrongIntensity get_current_intensity(void);
float get_ema_score(void);

#ifdef __cplusplus
}
#endif

#endif
