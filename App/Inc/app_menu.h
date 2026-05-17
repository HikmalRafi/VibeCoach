/**
 * @file    app_menu.h
 * @brief   Main header for the AI-Motion Correction menu system.
 *
 * @details Contains menu struct declarations and navigation functions.
 *          Version 2.1 adds power management integration.
 *          Version 3.1 adds real-time form monitoring display.
 */

#ifndef APP_MENU_H
#define APP_MENU_H

#include "main.h"
#include "../../../Lib/ST7735/st7735.h"
#include "../../../Lib/ST7735/st7735_fonts.h"
#include "../../../IMU/Inc/imu_calibration.h"
#include "../../Bsp/Inc/bsp_debounce_button.h"
#include "../../Bsp/Inc/bsp_power.h"
#include "../../App/Inc/app_splash.h"
#include "../../Bsp/Inc/bsp_battery.h"
#include "../../Bsp/Inc/bsp_led_rgb.h"
#include "../../Core/Inc/tinyML/tinyML.h"
#include "usart.h"
#include <stdint.h>
#include <stddef.h>

/* =========================================================================
 * MENU DATA STRUCTURE DECLARATIONS
 * ========================================================================= */

typedef struct MenuPage_t MenuPage_t;

typedef struct {
    const char* text;
    void (*action)(void);
} MenuItem_t;

struct MenuPage_t {
    const char* title;
    const MenuItem_t* items;
    uint8_t num_items;
    const MenuPage_t* parent;
};

/* =========================================================================
 * FORM STATUS DISPLAY TYPES
 * ========================================================================= */

/**
 * @brief Form status states for real-time monitoring display
 */
typedef enum {
    FORM_STATUS_IDLE = 0,
    FORM_STATUS_CORRECT,
    FORM_STATUS_WRONG
} FormStatus_t;

/* =========================================================================
 * GLOBAL FUNCTION DECLARATIONS
 * ========================================================================= */

void training_menu(void);
void goto_main_menu(void);
void goto_deltoid_menu(void);
void goto_pushup_menu(void);
void action_back(void);
void action_start_deltoid(void);

#endif /* APP_MENU_H */
