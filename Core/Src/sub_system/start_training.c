#include "sub_system/start_training.h"

// Objek tombol
static button_t btn_atas;
static button_t btn_bawah;
static button_t btn_ok;

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
    ssd1306_Fill(Black);
}

void goto_deltoid_menu(void) {
    current_page = &page_deltoid;
    cursor_pos = 0;
    ssd1306_Fill(Black);
}

void goto_pushup_menu(void) {
    current_page = &page_pushup;
    cursor_pos = 0;
    ssd1306_Fill(Black);
}

// Aksi ajaib untuk kembali ke menu sebelumnya (parent)
void action_kembali(void) {
    if (current_page->parent != NULL) {
        current_page = current_page->parent;
        cursor_pos = 0;
        ssd1306_Fill(Black);
    }
}

// --- Aksi Eksekusi Program (Hardware) ---
void action_mulai_deltoid(void) {
    // Di sinilah logika TinyML dan MPU9250 kamu diletakkan nantinya
	training_is = 1;
	while(training_is == 1){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);	//off red led
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);	//on green led



		if(button_read(&btn_ok) == 1){
			training_is = 0;
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); //off green led
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);	//on red led
			ssd1306_Fill(Black);
			calibrated = false;
		}
	}
    // Kamu mungkin butuh state mesin khusus (misal: is_training = true)
    // agar loop utama tidak menggambar menu lagi saat training berjalan.
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
    }

    // Keamanan: Jika entah kenapa halaman kosong, jangan lakukan apa-apa
    if (current_page == NULL) return;

    // --- BAGIAN 1: BACA TOMBOL ---
    if(button_read(&btn_atas) == 1){
        if(cursor_pos > 0) {
            cursor_pos--;
        } else {
            cursor_pos = current_page->num_items - 1; // Fitur "Wrap around" (dari paling atas lompat ke bawah)
        }
        ssd1306_Fill(Black);
    }
    else if(button_read(&btn_bawah) == 1){
        if(cursor_pos < current_page->num_items - 1) {
            cursor_pos++;
        } else {
            cursor_pos = 0; // Fitur "Wrap around" (dari paling bawah lompat ke atas)
        }
        ssd1306_Fill(Black);
    }
    else if(button_read(&btn_ok) == 1){
        // Panggil fungsi yang tersambung pada pilihan kursor saat ini
        if(current_page->items[cursor_pos].action != NULL){
            current_page->items[cursor_pos].action();
        }
    }

    // --- BAGIAN 2: GAMBAR KE OLED ---
    // Gambar Judul di paling atas (Baris ke-0)
    lcd_str(current_page->title, Font_11x18, White, 0, 0);

    // Looping untuk menggambar teks pilihan
    for(uint8_t i = 0; i < current_page->num_items; i++){

        // Gambar kursor '>' jika i sama dengan posisi kursor
        if(cursor_pos == i){
            lcd_str(">", Font_7x10, White, 0, 18 + (i * 10));
        }

        // Gambar teks menunya
        ssd1306_SetCursor(14, 18 + (i * 10));
        ssd1306_WriteString(current_page->items[i].text, Font_7x10, White);
    }

    // Kirim data buffer ke layar
    ssd1306_UpdateScreen();
}



//=========================================================================================================================

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

