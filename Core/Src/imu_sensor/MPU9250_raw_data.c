#include <imu_sensor/MPU9250_raw_data.h>

#define slave_addr 0x68
#define ACCEL_REG MPU9250_ACCEL_XOUT_H
#define GYRO_REG  MPU9250_GYRO_XOUT_H
#define address_mag 0x0C

vec3 accel_data(){
	vec3 accel;

	uint8_t rawData[6];

	stm32_i2c_read(slave_addr, ACCEL_REG, 6, rawData);

	  int16_t ax = (rawData[0] << 8) | rawData[1];
	  int16_t ay = (rawData[2] << 8) | rawData[3];
	  int16_t az = (rawData[4] << 8) | rawData[5];

	  accel.x = ax / 16384.0;
	  accel.y = ay / 16384.0;
	  accel.z = az / 16384.0;

	  return accel;
}

vec3 gyro_data(){

	vec3 gyro;

	  uint8_t rawDataGyro[6];

	  stm32_i2c_read(slave_addr, GYRO_REG, 6, rawDataGyro);

	  int16_t gx = (rawDataGyro[0] << 8) | rawDataGyro[1];
	  int16_t gy = (rawDataGyro[2] << 8) | rawDataGyro[3];
	  int16_t gz = (rawDataGyro[4] << 8) | rawDataGyro[5];

	    gyro.x = (gx / 131.0) * 0.0174533;
	    gyro.y = (gy / 131.0) * 0.0174533;
	    gyro.z = (gz / 131.0) * 0.0174533;

	  return gyro;
}



