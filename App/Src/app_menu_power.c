/**
 * @file    app_menu_power.c
 * @brief   Override callback power untuk app_menu (shutdown + wakeup)
 */

#include "../../App/Inc/app_menu.h"
#include "../../Bsp/Inc/bsp_power.h"
#include "../../App/Inc/app_splash.h"
#include "../../../Lib/ST7735/st7735.h"

#define COLOR_BG  ST7735_COLOR565(10, 20, 46)
#define LCD_BL_PORT   GPIOB
#define LCD_BL_PIN    GPIO_PIN_10

extern const MenuPage_t* current_page;
extern const MenuPage_t  page_main;
extern volatile uint8_t training_is;
extern volatile uint8_t cursor_pos;
extern uint8_t page_changed;

void bsp_power_on_shutdown_callback(void)
{
    training_is = 0;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

    app_splash_show_shutdown();

    /* Tidurkan IMU */
    uint8_t sleep_cmd = 0x40;
    stm32_i2c_write(0x68, MPU9250_PWR_MGMT_1, 1, &sleep_cmd);
    HAL_Delay(10);

    /* Matikan backlight */
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET);
}

void bsp_power_on_wakeup_callback(void)
{
    extern void MX_SPI1_Init(void);
    MX_SPI1_Init();
    ST7735_Init();

    /* Nyalakan backlight */
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET);

    uint8_t wake_cmd = 0x00;
    stm32_i2c_write(0x68, MPU9250_PWR_MGMT_1, 1, &wake_cmd);
    HAL_Delay(100);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    /* Reset ke main menu */
    current_page = &page_main;
    cursor_pos   = 0;
    page_changed = 1;

    app_splash_show_wake();
}
