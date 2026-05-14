#ifndef TINYML_H
#define TINYML_H

#include "usart.h"
#include "main.h"
#include <stdarg.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "tinyML/error_classifier.h"

#include "fusionFilter_and_calibration/Madgwick_filter.h"

typedef enum {
    LEVEL_NONE = 0,
    LEVEL_KOREKSI,
    LEVEL_WASPADA,
    LEVEL_BAHAYA
} WrongIntensity;

void vprint(const char *fmt, va_list argp);
void add_sensor_data(void);
void update_vibration(WrongIntensity level);

// Getter untuk display
WrongIntensity get_current_intensity(void);
float          get_ema_score(void);

#endif
