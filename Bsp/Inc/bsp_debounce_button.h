	#ifndef DEBOUNCE_BUTTON_H
	#define DEBOUNCE_BUTTON_H

	#include "main.h"
	#include <stdint.h>
	#include "stm32f4xx_hal.h"

	typedef struct{
		GPIO_TypeDef* port;
		uint16_t pin;
		uint32_t LastDebounceTime;    // The Last Time The Output Pin Was Toggled
		uint32_t DebounceDelay;       // The debounce Time; increase it if the output still flickers
		uint8_t BtnState;             // The Current Reading From The Input Pin
		uint8_t LastBtnState;         // The previous reading from The Input Pin
	} button_t;

	void button_init(button_t *btn, GPIO_TypeDef* port, uint16_t pin);
	uint8_t button_read(button_t *btn);
	#endif
