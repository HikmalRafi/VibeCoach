#include <fusionFilter_and_calibration/calibration_feature.h>

volatile bool calibrated = false; //calibration status
//quaternion base variabel data after calibrate with normal pose
volatile float q0_base;
volatile float q1_base;
volatile float q2_base;
volatile float q3_base;

void calibrateQuaternion(void){
	calibrated = true; //save calibration status

	ssd1306_Fill(Black);
	lcd_center_BigStr("Ready");
	//perform sensor data stabilization
	for(int j=0; j<200; j++) {
	        vec3 accel = accel_data();
	        vec3 gyro = gyro_data();
	        MadgwickAHRSupdateIMU(gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);
	        HAL_Delay(10);
	 }

	uint16_t samples = 300; //sampel normal pose for calibrate
	uint16_t counter = 0;
	uint8_t second = 3 + '0';

	float sum_q0 = 0, sum_q1 = 0, sum_q2 = 0, sum_q3 = 0;
	//take normal pose data samples
	while(counter < samples) {
		vec3 accel = accel_data();	//set accel data from MPU9250_raw_data.c
		vec3 gyro = gyro_data();		//set gyro data from MPU9250_raw_data.c

		MadgwickAHRSupdateIMU(gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z); //input raw data from imu sensor to madgwick filter	for filtering fusion

		sum_q0 += q0;
		sum_q1 += q1;
		sum_q2 += q2;
		sum_q3 += q3;

		counter++;

		if(counter % 100 == 0){
			ssd1306_Fill(Black);
			lcd_center_BigChar(second);
		    second--;
		}
	    HAL_Delay(10);
	}

	//save data to base variable
	q0_base = sum_q0 / samples;
	q1_base = sum_q1 / samples;
	q2_base = sum_q2 / samples;
	q3_base = sum_q3 / samples;

	//normalise quaternion baseline
	float norm = sqrt(q0_base*q0_base + q1_base*q1_base + q2_base*q2_base + q3_base*q3_base);
	if(norm > 0.0001f){
		q0_base /= norm;
		q1_base /= norm;
		q2_base /= norm;
		q3_base /= norm;
	}
}

Quaternion invQ(){
	//check calibration status
	Quaternion quaternion;
	if(calibrated == true){
		vec3 accel = accel_data();	//set accel data from MPU9250_raw_data.c
		vec3 gyro = gyro_data();		//set gyro data from MPU9250_raw_data.c

		MadgwickAHRSupdateIMU(gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z); //input raw data from imu sensor to madgwick filter	for filtering fusion

		//inverse quaternion baseline data
		float q0i = q0_base;
		float q1i = -q1_base;
		float q2i = -q2_base;
		float q3i = -q3_base;

		//multiply quaternions
		quaternion.w = q0i*q0 - q1i*q1 - q2i*q2 - q3i*q3;
		quaternion.x = q0i*q1 + q1i*q0 + q2i*q3 - q3i*q2;
		quaternion.y = q0i*q2 - q1i*q3 + q2i*q0 + q3i*q1;
		quaternion.z = q0i*q3 + q1i*q2 - q2i*q1 + q3i*q0;

		//normalise final quaternions data
		float norm_rel = sqrt(
		quaternion.w*quaternion.w +
		quaternion.x*quaternion.x +
		quaternion.y*quaternion.y +
		quaternion.z*quaternion.z);

		if(norm_rel > 0.0001f){
			quaternion.w /= norm_rel;
			quaternion.x /= norm_rel;
			quaternion.y /= norm_rel;
			quaternion.z /= norm_rel;
		}
	}else{
		return quaternion;
	}
	return quaternion;
}
