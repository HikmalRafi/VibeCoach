#include <imu_sensor/MPU9250_RegisterMap.h>
#include <imu_sensor/stm32_mpu9250_i2c.h>
#include "main.h"

#ifndef MPU9250_RAW_DATA_H
#define MPU9250_RAW_DATA_H

typedef struct{
	float x;
	float y;
	float z;
} vec3;

vec3 accel_data();
vec3 gyro_data();

#endif
