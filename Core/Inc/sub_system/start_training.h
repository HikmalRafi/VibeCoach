#include <fusionFilter_and_calibration/calibration_feature.h>
#include <imu_sensor/MPU9250_raw_data.h>
#include "main.h"
#include "usart.h"
#include "debounce_button.h"

#ifndef START_TRAINING_H
#define START_TRAINING_H

extern volatile uint8_t point;

void training_menu(void);
void training_tinyML(void);
void deltoid_menu(void);

#endif
