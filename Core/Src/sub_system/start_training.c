#include "sub_system/start_training.h"

//init button name
static button_t btn_atas;
static button_t btn_bawah;
static button_t btn_ok;

volatile uint8_t point = 0;
static uint8_t init_check = 0;
/*void training_tinyML(void){
	  uint8_t btn1 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);

	  // madgwick quaternion data
	  if(count == 1){
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);	//off red led
			  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);	//on green led

			  if(calibrated == false){
				  calibrateQuaternion();
			  }

			  Quaternion quarter = invQ();

			  //print to serial monitor with UART
			  lcd_str("GERAK!!", Font_11x18, White, 0, 0);
		      char buf[100];
	          sprintf(buf, "%.2f,%.2f,%.2f,%.2f\r\n", quarter.w, quarter.x, quarter.y, quarter.z);
	          HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
	          //print to serial monitor with UART

	          HAL_Delay(10); //set 5ms delay, because i want to set sampleFreq in 200.0f and want to read detail data
	  } else if(count == 0){
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); //off green led
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);	//on red led
		  calibrated = false;
	  }
}*/

void deltoid_menu(void){
	if(button_read(&btn_ok) == 1 && point == 1){

		if(button_read(&btn_atas) == 1){
			point = 0;
			ssd1306_Fill(Black);
		}else if(button_read(&btn_bawah) == 1){
			point = 1;
			ssd1306_Fill(Black);
		}


		char* training_list[2] = {"MULAI", "Setting"};

		lcd_str("Deltoid", Font_11x18, White, 0, 0);
		for(uint8_t i = 0; i < sizeof(training_list) / sizeof(training_list[0]); i++){

			if(point == i){
				lcd_str(">", Font_7x10, White, 0, 18 + (i * 10));
			}

			ssd1306_SetCursor(14, 18 + (i * 10));
			ssd1306_WriteString(training_list[i], Font_7x10, White);
		}
	}
}


void training_menu(void){
	if(init_check == 0){
		button_init(&btn_atas, GPIOB, GPIO_PIN_12);
		button_init(&btn_bawah, GPIOA, GPIO_PIN_4);
		button_init(&btn_ok, GPIOB, GPIO_PIN_2);
		init_check = 1;
	}

	char* training_list[2] = {"Deltoid", "Push UP"};

	if(button_read(&btn_atas) == 1){
		point = 0;
		ssd1306_Fill(Black);
	}else if(button_read(&btn_bawah) == 1){
		point = 1;
		ssd1306_Fill(Black);
	}

	lcd_str("Training", Font_11x18, White, 0, 0);
	for(uint8_t i = 0; i < sizeof(training_list) / sizeof(training_list[0]); i++){

		if(point == i){
			lcd_str(">", Font_7x10, White, 0, 18 + (i * 10));
		}

		ssd1306_SetCursor(14, 18 + (i * 10));
		ssd1306_WriteString(training_list[i], Font_7x10, White);
	}

	ssd1306_UpdateScreen();
}
