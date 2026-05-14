#include "system_ui/main_menu.h"

// Definisi warna dasar (RGB565) jika belum ada di st7735.h
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF

static button_t btn_atas;
static button_t btn_bawah;
static button_t btn_ok;

volatile uint8_t point = 0;
static uint8_t init_check = 0;
volatile uint8_t training_is = 0;

// State Navigasi
volatile uint8_t cursor_pos = 0;              // Posisi panah '>' saat ini
static const MenuPage_t* current_page = NULL; // Menyimpan halaman apa yang sedang buka

// Kita deklarasikan nama halamannya dulu agar fungsi aksi di bawah bisa melihatnya
extern const MenuPage_t page_main;
extern const MenuPage_t page_deltoid;
extern const MenuPage_t page_pushup;

// =====================================================================
// FUNGSI AKSI (Yang terjadi saat "OK" ditekan)
// =====================================================================

// --- Aksi Pindah Halaman Menu ---
void goto_main_menu(void) {
    current_page = &page_main;
    cursor_pos = 0;
    ST7735_FillScreenFast(COLOR_BLACK);
}

void goto_deltoid_menu(void) {
    current_page = &page_deltoid;
    cursor_pos = 0;
    ST7735_FillScreenFast(COLOR_BLACK);
}

void goto_pushup_menu(void) {
    current_page = &page_pushup;
    cursor_pos = 0;
    ST7735_FillScreenFast(COLOR_BLACK);
}

// Aksi ajaib untuk kembali ke menu sebelumnya (parent)
void action_kembali(void) {
    if (current_page->parent != NULL) {
        current_page = current_page->parent;
        cursor_pos = 0;
        ST7735_FillScreenFast(COLOR_BLACK);
    }
}

// --- Aksi Eksekusi Program (Hardware) ---
void action_mulai_deltoid(void) {
    // Di sinilah logika TinyML dan MPU9250 kamu diletakkan nantinya
    training_is = 1;
    while(training_is == 1){
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);    //off red led
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);      //on green led

        if(calibrated == false){
              calibrateQuaternion();
        }

        Quaternion quarter = invQ();

        //print to serial monitor with UART
        char buf[100];
        sprintf(buf, "%.2f,%.2f,%.2f,%.2f\r\n", quarter.w, quarter.x, quarter.y, quarter.z);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
        //print to serial monitor with UART

        HAL_Delay(10); //set 5ms delay, because i want to set sampleFreq in 200.0f and want to read detail data

        if(button_read(&btn_ok) == 1){
            training_is = 0;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); //off green led
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   //on red led
            ST7735_FillScreenFast(COLOR_BLACK);
            calibrated = false;
        }
    }
}

// =====================================================================
// DATA MENU (Disimpan di Memori Flash / ROM)
// =====================================================================

// --- Halaman: Push Up ---
const MenuItem_t items_pushup[] = {
    {"MULAI PUSHUP", action_mulai_deltoid}, // Sementara pakai fungsi yang sama
    {"KEMBALI", action_kembali}
};
const MenuPage_t page_pushup = {
    "Push Up",           // Judul
    items_pushup,        // Daftar Pilihan
    2,                   // Jumlah Pilihan
    &page_main           // Parent (Jika kembali, lari ke page_main)
};

// --- Halaman: Deltoid ---
const MenuItem_t items_deltoid[] = {
    {"MULAI", action_mulai_deltoid},
    {"Setting", NULL},              // NULL berarti tombol OK tidak berefek apa-apa
    {"KEMBALI", action_kembali}
};
const MenuPage_t page_deltoid = {
    "Deltoid",
    items_deltoid,
    3,
    &page_main           // Parent
};

// --- Halaman: Main Menu ---
const MenuItem_t items_main[] = {
    {"Deltoid", goto_deltoid_menu},
    {"Push UP", goto_pushup_menu}
};
const MenuPage_t page_main = {
    "Training",
    items_main,
    2,
    NULL                 // Ini halaman paling awal, jadi tidak ada parent
};

// =====================================================================
// FUNGSI UTAMA (Sistem Core)
// =====================================================================

void training_menu(void){
    // Inisialisasi tombol dan atur halaman awal hanya 1x saat boot
    if(init_check == 0){
        button_init(&btn_atas, GPIOB, GPIO_PIN_12);
        button_init(&btn_bawah, GPIOA, GPIO_PIN_4);
        button_init(&btn_ok, GPIOB, GPIO_PIN_2);

        current_page = &page_main; // Set halaman pertama
        init_check = 1;
        ST7735_FillScreenFast(COLOR_BLACK); // Clear screen pertama kali
    }

    // Keamanan: Jika entah kenapa halaman kosong, jangan lakukan apa-apa
    if (current_page == NULL) return;

    // --- BAGIAN 1: BACA TOMBOL ---
    if(button_read(&btn_atas) == 1){
        // Timpa kursor lama dengan spasi (warna hitam) sebelum pindah
        ST7735_WriteString(0, 18 + (cursor_pos * 10), " ", Font_7x10, COLOR_WHITE, COLOR_BLACK);

        if(cursor_pos > 0) {
            cursor_pos--;
        } else {
            cursor_pos = current_page->num_items - 1; // Fitur "Wrap around"
        }
        // Kita tidak memakai FillScreenFast di sini agar tidak berkedip (flicker)
    }
    else if(button_read(&btn_bawah) == 1){
        // Timpa kursor lama dengan spasi (warna hitam) sebelum pindah
        ST7735_WriteString(0, 18 + (cursor_pos * 10), " ", Font_7x10, COLOR_WHITE, COLOR_BLACK);

        if(cursor_pos < current_page->num_items - 1) {
            cursor_pos++;
        } else {
            cursor_pos = 0; // Fitur "Wrap around"
        }
        // Kita tidak memakai FillScreenFast di sini agar tidak berkedip (flicker)
    }
    else if(button_read(&btn_ok) == 1){
        // Panggil fungsi yang tersambung pada pilihan kursor saat ini
        if(current_page->items[cursor_pos].action != NULL){
            current_page->items[cursor_pos].action();
        }
    }

    // --- BAGIAN 2: GAMBAR KE TFT ---
    // Gambar Judul di paling atas (Baris ke-0)
    // Parameter: X, Y, Text, Font, Foreground, Background
    ST7735_WriteString(0, 0, current_page->title, Font_11x18, COLOR_WHITE, COLOR_BLACK);

    // Looping untuk menggambar teks pilihan
    for(uint8_t i = 0; i < current_page->num_items; i++){

        // Gambar kursor '>' jika i sama dengan posisi kursor
        if(cursor_pos == i){
            ST7735_WriteString(0, 18 + (i * 10), ">", Font_7x10, COLOR_WHITE, COLOR_BLACK);
        }

        // Gambar teks menunya (Mulai di X=14)
        ST7735_WriteString(14, 18 + (i * 10), current_page->items[i].text, Font_7x10, COLOR_WHITE, COLOR_BLACK);
    }
}
