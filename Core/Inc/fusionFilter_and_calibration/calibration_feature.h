#include <fusionFilter_and_calibration/Madgwick_filter.h>
#include <imu_sensor/MPU9250_raw_data.h>
#include <lcd_screen/lcd_screen.h>
#include <lcd_screen/ssd1306.h>
#include "main.h"

#ifndef CALIBRATION_FEATURE_H
#define CALIBRATION_FEATURE_H

extern volatile bool calibrated; //calibration status
//quaternion base variabel data after calibrate with normal pose
extern volatile float q0_base;
extern volatile float q1_base;
extern volatile float q2_base;
extern volatile float q3_base;

typedef struct{
	float w;
	float x;
	float y;
	float z;
} Quaternion;

void calibrateQuaternion(void); //take normal pose data samples
Quaternion invQ(void); //normalise, inverse, and multiply quaternions data

#endif
