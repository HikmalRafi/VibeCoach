#include "device&module_tester/device_tester.h"

button_t btn_start;

void button_tes(void){
	button_init(&btn_start, GPIOB, GPIO_PIN_2);

	if(button_read(&btn_start) == 1){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);	//off red led
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);	//on green led

	    /*char tes[100];
        sprintf(tes, "OK\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)tes, strlen(tes), HAL_MAX_DELAY);
        HAL_Delay(500);*/

	} else if (button_read(&btn_start) != 1){
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); //off green led
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);	//on red led
	}
}
