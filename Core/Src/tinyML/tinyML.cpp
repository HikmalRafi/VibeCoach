#include <tinyML/tinyML.h>
#include "usart.h"
#include "main.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

extern "C"{
#include <fusionFilter_and_calibration/calibration_feature.h>
#include <imu_sensor/MPU9250_raw_data.h>
}

float sensor_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
int counter = 0;

void vprint(const char *fmt, va_list argp)
{
    char string[200];
    if(0 < vsprintf(string, fmt, argp)) // build string
    {
        HAL_UART_Transmit(&huart1, (uint8_t*)string, strlen(string), 0xffffff); // send message via UART
    }
}

void ei_printf(const char *format, ...) {
    va_list myargs;
    va_start(myargs, format);
    vprint(format, myargs);
    va_end(myargs);
}

void add_sensor_data() {
	if(calibrated == false){
		  calibrateQuaternion();
	}

	Quaternion quarter = invQ();

    if (counter < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    	sensor_buffer[counter++] = quarter.w;
    	sensor_buffer[counter++] = quarter.x;
    	sensor_buffer[counter++] = quarter.y;
    	sensor_buffer[counter++] = quarter.z;
    }

    // 4. Jika buffer sudah penuh, saatnya AI bekerja!
        if (counter >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {

            // Bungkus buffer ke format yang dimengerti Edge Impulse
            signal_t signal;
            numpy::signal_from_buffer(sensor_buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

            // Jalankan Prediksi
            ei_impulse_result_t result = { 0 };
            run_classifier(&signal, &result, false);

            // Kirim hasil ke UART untuk kamu lihat
            ei_printf("Prediksi: %s, Nilai: %.2f\n",
                      result.classification[0].label,
                      result.classification[0].value);

            // 5. RESET counter ke 0 agar bisa mengisi data baru lagi (looping)
            counter = 0;
        }
   HAL_Delay(10);
}
