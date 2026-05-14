#include "debounce_button.h"

// Fungsi untuk inisialisasi tombol baru
void button_init(button_t *btn, GPIO_TypeDef* port, uint16_t pin) {
    btn->port = port;
    btn->pin = pin;
    btn->DebounceDelay = 20;
    btn->LastDebounceTime = 0;
    btn->LastBtnState = 1; // Asumsi Active Low (default belum ditekan = 1)
    btn->BtnState = 1;
}

// Fungsi untuk membaca dan melakukan debounce tombol
uint8_t button_read(button_t *btn) {
    uint8_t reading = HAL_GPIO_ReadPin(btn->port, btn->pin);
    uint32_t currentTick = HAL_GetTick();
    uint8_t result = 0; // 0 = tidak ada aksi, 1 = tombol baru saja ditekan

    if (reading != btn->LastBtnState) {
        btn->LastDebounceTime = currentTick;
    }

    if ((currentTick - btn->LastDebounceTime) > btn->DebounceDelay) {
        if (reading != btn->BtnState) {
            btn->BtnState = reading;

            // Jika tombol ditekan (Active Low = 0)
            if (btn->BtnState == 0) {
                result = 1; // Trigger!
            }
        }
    }
    btn->LastBtnState = reading;
    return result;
}
