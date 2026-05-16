/**
 * @file    app_menu_power.c
 * @brief   Override callback power untuk app_menu (shutdown + wakeup)
 */

#include "../../App/Inc/app_menu.h"
#include "../../Bsp/Inc/bsp_power.h"
#include "../../App/Inc/app_splash.h"
#include "../../../Lib/ST7735/st7735.h"

#define COLOR_BG  ST7735_COLOR565(10, 20, 46)

extern volatile uint8_t training_is;
extern volatile uint8_t cursor_pos;
extern uint8_t page_changed;

void bsp_power_on_shutdown_callback(void)
{
    training_is = 0;

    /* Matikan LED */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

    /* Animasi shutdown + layar hitam */
    app_splash_show_shutdown();

    /* Matikan backlight PWM */
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

void bsp_power_on_wakeup_callback(void)
{
    extern void MX_SPI1_Init(void);
    MX_SPI1_Init();
    ST7735_Init();

    /* Nyalakan backlight PWM kembali */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    /* LED merah = idle */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    app_splash_show_wake();

    cursor_pos   = 0;
    page_changed = 1;
}
