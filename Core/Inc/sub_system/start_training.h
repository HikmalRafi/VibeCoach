#include <fusionFilter_and_calibration/calibration_feature.h>
#include <imu_sensor/MPU9250_raw_data.h>
#include <tinyML/tinyML.h>
#include "main.h"
#include "usart.h"
#include "debounce_button.h"

#ifndef START_TRAINING_H
#define START_TRAINING_H

// Forward declaration agar Struct bisa saling mengenali
struct MenuPage_s;



typedef void (*MenuAction_t)(void);

typedef struct{
	const char* text;
	MenuAction_t action;
}MenuItem_t;

typedef struct MenuPage_s{
	const char* title;
	const MenuItem_t* items;
	uint8_t num_items;
	const struct MenuPage_s* parent;
}MenuPage_t;

void training_menu(void);

#endif
