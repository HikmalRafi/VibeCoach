/**
 * @file    main_menu.c
 * @brief   Sistem navigasi menu untuk perangkat latihan berbasis STM32.
 *
 * @details Modul ini mengelola seluruh tampilan dan interaksi menu pada LCD ST7735.
 *          Arsitektur menu menggunakan pointer ke struct (MenuPage_t) yang saling
 *          terhubung, sehingga mudah diperluas tanpa mengubah fungsi utama.
 *
 *          Konsep UI terinspirasi dari Flipper Zero:
 *          - Header bar : judul halaman + ikon baterai/status
 *          - Item menu  : ikon pixel + teks + panah navigasi
 *          - Status bar : hint tombol di bagian bawah layar
 *
 * @author  Tim Kamu
 * @version 2.0
 */

#include "../../App/Inc/app_menu.h"

/* =========================================================================
 * DEFINISI WARNA (RGB565)
 * Semua warna didefinisikan di sini agar mudah diganti tanpa cari satu-satu.
 * Format: 0xRRGGBB dikonversi ke RGB565 dengan macro ST7735_COLOR565(r,g,b)
 * ========================================================================= */

/** Warna latar belakang utama - biru gelap navy ala Flipper Zero */
#define COLOR_BG         ST7735_COLOR565(10,  20,  46)

/** Warna teks utama - putih terang */
#define COLOR_TEXT       ST7735_COLOR565(220, 220, 220)

/** Warna aksen - biru terang untuk ikon dan highlight */
#define COLOR_ACCENT     ST7735_COLOR565(74,  158, 255)

/** Warna item yang sedang dipilih (background inversi) */
#define COLOR_SELECT_BG  ST7735_COLOR565(230, 230, 230)

/** Warna teks saat item dipilih (harus kontras dengan SELECT_BG) */
#define COLOR_SELECT_FG  ST7735_COLOR565(10,  20,  46)

/** Warna header bar - biru lebih gelap dari BG */
#define COLOR_HDR_BG     ST7735_COLOR565(15,  52,  96)

/** Warna status bar - hampir hitam */
#define COLOR_SBAR_BG    ST7735_COLOR565(10,  22,  40)

/** Warna separator garis horizontal */
#define COLOR_SEP        ST7735_COLOR565(30,  58,  95)

/** Warna teks hint (sangat redup) */
#define COLOR_HINT       ST7735_COLOR565(60,  90,  130)

/** Warna hijau untuk status READY / baterai */
#define COLOR_GREEN      ST7735_COLOR565(74,  222, 128)

/** Warna merah untuk indikator STOP */
#define COLOR_RED        ST7735_COLOR565(239, 68,  68)

/* =========================================================================
 * KONSTANTA LAYOUT
 * Semua posisi elemen UI dihitung dari konstanta ini.
 * Jika ingin mengubah ukuran header, cukup ubah HDR_H saja.
 * ========================================================================= */

/** Tinggi header bar (baris paling atas) dalam pixel */
#define HDR_H       14

/** Tinggi status bar (baris paling bawah) dalam pixel */
#define SBAR_H      10

/** Posisi Y awal area konten menu (di bawah header) */
#define CONTENT_Y   (HDR_H + 1)

/** Tinggi setiap baris item menu dalam pixel */
#define ITEM_H      12

/** Lebar total layar (sesuai konfigurasi ST7735) */
#define LCD_W       ST7735_WIDTH

/** Tinggi total layar (sesuai konfigurasi ST7735) */
#define LCD_H       ST7735_HEIGHT

/* =========================================================================
 * VARIABEL STATE - TOMBOL & NAVIGASI
 * Dideklarasikan static agar tidak polusi namespace global.
 * ========================================================================= */

static button_t btn_atas;
static button_t btn_bawah;
static button_t btn_ok;

volatile uint8_t point       = 0;
static   uint8_t init_check  = 0;
volatile uint8_t training_is = 0;

uint8_t page_changed = 1;  // Flag: halaman baru perlu full render
static uint8_t last_cursor_pos = 0;  // Untuk deteksi perubahan cursor

volatile uint8_t cursor_pos        = 0;
const MenuPage_t* current_page = NULL;

/* Forward declaration halaman - didefinisikan di bawah */
extern const MenuPage_t page_main;
extern const MenuPage_t page_deltoid;
extern const MenuPage_t page_pushup;

/* =========================================================================
 * HELPER FUNCTIONS - KOMPONEN GAMBAR UI
 * Semua fungsi di seksi ini bersifat INTERNAL (static).
 * Pisahkan per komponen agar mudah dimodifikasi secara independen.
 * ========================================================================= */

/**
 * @brief Menggambar garis horizontal tipis sebagai separator.
 *
 * @param y Posisi Y garis dalam pixel.
 */
static void ui_draw_separator(uint16_t y)
{
    ST7735_FillRectangle(0, y, LCD_W, 1, COLOR_SEP);
}

/**
 * @brief Menggambar header bar di bagian paling atas layar.
 *
 * @details Header menampilkan:
 *          - Judul halaman (kiri)
 *          - Ikon baterai sederhana (kanan, 3 segmen)
 *
 * @param title String judul yang ditampilkan. Maksimal ~11 karakter
 *              agar tidak bertabrakan dengan ikon baterai.
 */
static void ui_draw_header(const char* title)
{
    /* Gambar background header */
    ST7735_FillRectangle(0, 0, LCD_W, HDR_H, COLOR_HDR_BG);

    /* Garis separator bawah header */
    ui_draw_separator(HDR_H);

    /* Tulis judul - gunakan Font_7x10 agar muat di HDR_H=14px */
    ST7735_WriteString(4, 2, title, Font_7x10, COLOR_TEXT, COLOR_HDR_BG);

    /* --- Ikon Baterai Dinamis --- */
    /* Posisi X: Pojok kanan dikurangi lebar baterai. Posisi Y: margin 3px dari atas */
    uint16_t bat_x = LCD_W - 18;
    uint16_t bat_y = 3;

    /* Panggil fungsi rendering baterai dari bsp_battery.c */
    bsp_battery_draw_icon(bat_x, bat_y, COLOR_HDR_BG);
}

/**
 * @brief Menggambar status bar di bagian paling bawah layar.
 *
 * @details Status bar menampilkan hint tombol kepada user.
 *          Desain: background gelap + teks hint kecil.
 *
 * @param hint     String hint yang ditampilkan di kiri.
 * @param dot_color Warna titik status di pojok kanan (gunakan COLOR_GREEN/RED/ACCENT).
 */
static void ui_draw_statusbar(const char* hint, uint16_t dot_color)
{
    uint16_t y = LCD_H - SBAR_H;

    /* Separator atas status bar */
    ui_draw_separator(y);

    /* Background status bar */
    ST7735_FillRectangle(0, y + 1, LCD_W, SBAR_H - 1, COLOR_SBAR_BG);

    /* Hint teks */
    ST7735_WriteString(4, y + 1, hint, Font_7x10, COLOR_HINT, COLOR_SBAR_BG);

    /* Titik status (indikator LED virtual) di pojok kanan */
    ST7735_FillRectangle(LCD_W - 8, y + 3, 5, 5, dot_color);
}

/**
 * @brief Menggambar satu baris item menu.
 *
 * @details Setiap item terdiri dari:
 *          - Kotak ikon kecil (10x10px) berisi 1 karakter
 *          - Teks nama item
 *          - Panah "›" di kanan jika item punya action
 *          - Efek inversi warna jika item sedang dipilih
 *
 * @param y          Posisi Y baris dalam pixel.
 * @param icon_char  Karakter tunggal untuk ikon (mis: '>', 'D', '+').
 * @param text       Teks nama item menu.
 * @param has_arrow  1 jika tampilkan panah navigasi "›", 0 jika tidak.
 * @param is_selected 1 jika item ini sedang dipilih cursor.
 */
static void ui_draw_menu_item(uint16_t y, char icon_char,
                               const char* text,
                               uint8_t has_arrow, uint8_t is_selected)
{
    /* Tentukan warna berdasarkan state terpilih atau tidak */
    uint16_t bg   = is_selected ? COLOR_SELECT_BG : COLOR_BG;
    uint16_t fg   = is_selected ? COLOR_SELECT_FG : COLOR_TEXT;
    uint16_t icon_fg = is_selected ? COLOR_SELECT_FG : COLOR_ACCENT;

    /* Background seluruh baris */
    ST7735_FillRectangle(0, y, LCD_W, ITEM_H, bg);

    /* --- Kotak Ikon --- */
    /* Border kotak ikon (2px dari kiri) */
    if (is_selected) {
        /* Saat dipilih: kotak solid dengan warna BG asli */
        ST7735_FillRectangle(2, y + 1, 10, 10, COLOR_BG);
        /* Tulis karakter ikon dengan warna BG */
        char icon_str[2] = {icon_char, '\0'};
        ST7735_WriteString(3, y + 1, icon_str, Font_7x10, COLOR_ACCENT, COLOR_BG);
    } else {
        /* Normal: outline kotak ikon */
        ST7735_FillRectangle(2,  y + 1, 10, 10, COLOR_BG);    /* area dalam */
        ST7735_FillRectangle(2,  y + 1, 10,  1, COLOR_ACCENT); /* atas */
        ST7735_FillRectangle(2,  y + 10, 10, 1, COLOR_ACCENT); /* bawah */
        ST7735_FillRectangle(2,  y + 1,  1, 10, COLOR_ACCENT); /* kiri */
        ST7735_FillRectangle(11, y + 1,  1, 10, COLOR_ACCENT); /* kanan */

        /* Tulis karakter ikon */
        char icon_str[2] = {icon_char, '\0'};
        ST7735_WriteString(4, y + 1, icon_str, Font_7x10, icon_fg, COLOR_BG);
    }

    /* --- Teks Item --- */
    /* Mulai X=15 (setelah ikon 10px + gap 3px) */
    ST7735_WriteString(15, y + 1, text, Font_7x10, fg, bg);

    /* --- Panah Navigasi (jika ada action) --- */
    if (has_arrow) {
        ST7735_WriteString(LCD_W - 9, y + 1, ">", Font_7x10, icon_fg, bg);
    }
}

/**
 * @brief Menggambar label seksi kecil (mis: "EXERCISE", "DEVICE").
 *
 * @details Label ini muncul di atas kelompok item sebagai kategori.
 *          Font sangat kecil, warna aksen redup.
 *
 * @param y     Posisi Y label.
 * @param label String label seksi.
 */
static void ui_draw_section_label(uint16_t y, const char* label)
{
    /* Area background tipis untuk label seksi */
    ST7735_FillRectangle(0, y, LCD_W, 9, COLOR_BG);
    ST7735_WriteString(4, y, label, Font_7x10, COLOR_HINT, COLOR_BG);
}

/* =========================================================================
 * FUNGSI AKSI NAVIGASI - Tidak diubah dari versi asli
 * ========================================================================= */

void goto_main_menu(void) {
    current_page = &page_main;
    cursor_pos   = 0;
    page_changed = 1;  // Tambahkan ini
}

void goto_deltoid_menu(void) {
    current_page = &page_deltoid;
    cursor_pos   = 0;
    page_changed = 1;  // Tambahkan ini
}

void goto_pushup_menu(void) {
    current_page = &page_pushup;
    cursor_pos   = 0;
    page_changed = 1;  // Tambahkan ini
}

void action_kembali(void) {
    if (current_page->parent != NULL) {
        current_page = current_page->parent;
        cursor_pos   = 0;
        page_changed = 1;  // Tambahkan ini
    }
}

/* =========================================================================
 * FUNGSI AKSI HARDWARE - Tidak diubah dari versi asli (hanya cosmetic)
 * ========================================================================= */

void action_mulai_deltoid(void)
{
    training_is = 1;

    /* Bersihkan layar sekali sebelum masuk loop training */
    ST7735_FillScreenFast(COLOR_BG);

    while (training_is == 1)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); /* off red LED  */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);   /* on  green LED */

        if (calibrated == false) {
            calibrateQuaternion();
        }

        Quaternion quarter = invQ();

        /* Kirim data ke Serial Monitor via UART */
        char buf[100];
        sprintf(buf, "%.2f,%.2f,%.2f,%.2f\r\n",
                quarter.w, quarter.x, quarter.y, quarter.z);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);

        /* --- Tampilan Training Aktif --- */
        /*
         * Di sini kita gambar layar training setiap loop.
         * Catatan: Gambar hanya area yang berubah agar tidak flicker.
         *          Untuk tampilan yg lebih smooth di masa depan, bisa pakai
         *          double buffer atau hanya update region tertentu saja.
         */

        /* Header khusus mode recording */
        ST7735_FillRectangle(0, 0, LCD_W, HDR_H, ST7735_COLOR565(15, 40, 16));
        ui_draw_separator(HDR_H);
        ST7735_WriteString(4, 2, "* RECORDING", Font_7x10,
                           COLOR_GREEN, ST7735_COLOR565(15, 40, 16));

        /* Data quaternion - tampilkan sebagai stats */
        uint16_t dy = CONTENT_Y + 4;

        /* Label W (komponen utama rotasi) */
        ST7735_FillRectangle(0, dy, LCD_W, 22, COLOR_BG);
        ST7735_WriteString(4, dy, "W", Font_7x10, COLOR_ACCENT, COLOR_BG);

        /* Nilai W dengan font besar */
        char val_buf[16];
        sprintf(val_buf, "%.3f", quarter.w);
        ST7735_WriteString(14, dy, val_buf, Font_11x18, COLOR_GREEN, COLOR_BG);
        dy += 24;

        /* Label XYZ dalam satu baris */
        ST7735_FillRectangle(0, dy, LCD_W, 12, COLOR_BG);
        ST7735_WriteString(4,  dy, "X", Font_7x10, COLOR_ACCENT, COLOR_BG);
        ST7735_WriteString(44, dy, "Y", Font_7x10, COLOR_ACCENT, COLOR_BG);
        ST7735_WriteString(84, dy, "Z", Font_7x10, COLOR_ACCENT, COLOR_BG);
        dy += 11;

        /* Nilai XYZ */
        ST7735_FillRectangle(0, dy, LCD_W, 10, COLOR_BG);
        char x_buf[10], y_buf[10], z_buf[10];
        sprintf(x_buf, "%.2f", quarter.x);
        sprintf(y_buf, "%.2f", quarter.y);
        sprintf(z_buf, "%.2f", quarter.z);
        ST7735_WriteString(2,  dy, x_buf, Font_7x10, COLOR_TEXT, COLOR_BG);
        ST7735_WriteString(42, dy, y_buf, Font_7x10, COLOR_TEXT, COLOR_BG);
        ST7735_WriteString(82, dy, z_buf, Font_7x10, COLOR_TEXT, COLOR_BG);
        dy += 14;

        /* Separator */
        ui_draw_separator(dy);
        dy += 3;

        /* Hint status bar bawah */
        ui_draw_statusbar("OK=STOP", COLOR_RED);

        HAL_Delay(10);

        /* Cek tombol OK untuk berhenti */
        if (button_read(&btn_ok) == 1) {
            training_is = 0;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); /* off green */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   /* on  red   */
            calibrated = false;
            ST7735_FillScreenFast(COLOR_BG);
        }
    }
}

/* =========================================================================
 * DATA MENU - Tidak diubah dari versi asli
 * ========================================================================= */

const MenuItem_t items_pushup[] = {
    {"MULAI PUSHUP", action_mulai_deltoid},
    {"KEMBALI",      action_kembali}
};
const MenuPage_t page_pushup = {
    "Push Up", items_pushup, 2, &page_main
};

const MenuItem_t items_deltoid[] = {
    {"MULAI",   action_mulai_deltoid},
    {"Setting", NULL},
    {"KEMBALI", action_kembali}
};
const MenuPage_t page_deltoid = {
    "Deltoid", items_deltoid, 3, &page_main
};

const MenuItem_t items_main[] = {
    {"Deltoid", goto_deltoid_menu},
    {"Push UP", goto_pushup_menu}
};
const MenuPage_t page_main = {
    "TRAINING", items_main, 2, NULL
};

/* =========================================================================
 * PETA IKON PER ITEM MENU
 *
 * Karena struct MenuItem_t tidak kita ubah (untuk menjaga kompatibilitas),
 * kita map ikon menggunakan array terpisah berdasarkan INDEX item.
 *
 * Cara extend: tambah karakter ikon sesuai urutan item di items_xxx[].
 * Gunakan karakter ASCII yang ada di Font_7x10 (lihat fonts.c).
 *
 * Karakter yang tersedia di Font_7x10 untuk ikon:
 *  '>' = play/mulai      '+' = tambah       '-' = minus
 *  '<' = kembali         '=' = setting      '*' = aktif/bintang
 *  '?' = bantuan         '!' = peringatan   '#' = menu
 * ========================================================================= */

/** Ikon untuk halaman main menu (urutan = urutan items_main[]) */
static const char icons_main[]    = {'D', 'P'};

/** Ikon untuk halaman deltoid menu */
static const char icons_deltoid[] = {'>', '=', '<'};

/** Ikon untuk halaman pushup menu */
static const char icons_pushup[]  = {'>', '<'};

/**
 * @brief Mendapatkan karakter ikon untuk item tertentu pada halaman saat ini.
 *
 * @details Fungsi ini mencocokkan halaman aktif dengan array ikon yang sesuai.
 *          Jika halaman tidak dikenal, karakter '>' digunakan sebagai fallback.
 *
 * @param index Index item menu (0-based).
 * @return Karakter ikon yang akan ditampilkan.
 */
static char get_item_icon(uint8_t index)
{
    if (current_page == &page_main    && index < sizeof(icons_main))
        return icons_main[index];

    if (current_page == &page_deltoid && index < sizeof(icons_deltoid))
        return icons_deltoid[index];

    if (current_page == &page_pushup  && index < sizeof(icons_pushup))
        return icons_pushup[index];

    return '>'; /* fallback default */
}

/* =========================================================================
 * FUNGSI UTAMA - TRAINING MENU
 * Ini adalah fungsi yang dipanggil dari while(1) di main.c.
 * Struktur utama SAMA dengan versi asli, hanya bagian render yang diperbarui.
 * ========================================================================= */

/**
 * @brief Fungsi utama sistem menu. Panggil di dalam while(1) pada main.c.
 *
 * @details Alur kerja:
 *          1. Inisialisasi tombol dan halaman awal (hanya 1x saat boot).
 *          2. Baca input tombol (atas/bawah/ok).
 *          3. Render seluruh UI (header, items, statusbar).
 *
 *          Anti-flicker: kursor lama dihapus dengan menimpa baris itu saja,
 *          bukan clear screen penuh. Hanya item yang berubah yang digambar ulang.
 */
void training_menu(void)
{
    /* --- INISIALISASI (hanya 1x saat boot) --- */
    if (init_check == 0) {
        button_init(&btn_atas,  GPIOB, GPIO_PIN_12);
        button_init(&btn_bawah, GPIOB, GPIO_PIN_13);
        button_init(&btn_ok,    GPIOB, GPIO_PIN_14);

        current_page = &page_main;
        init_check   = 1;
        page_changed = 1;  // Force full render pertama kali

        /* Clear screen dengan warna background tema */
        ST7735_FillScreenFast(COLOR_BG);
    }

    if (current_page == NULL) return;

    /* =========================================================
     * BAGIAN 1: BACA TOMBOL (DENGAN DEBOUNCE SEDERHANA)
     * ========================================================= */

    static uint8_t btn_atas_last = 0;
    static uint8_t btn_bawah_last = 0;
    static uint8_t btn_ok_last = 0;

    uint8_t btn_atas_now = button_read(&btn_atas);
    uint8_t btn_bawah_now = button_read(&btn_bawah);
    uint8_t btn_ok_now = button_read(&btn_ok);

    // Deteksi rising edge (transisi dari 0 ke 1)
    uint8_t btn_atas_pressed = (btn_atas_now == 1 && btn_atas_last == 0);
    uint8_t btn_bawah_pressed = (btn_bawah_now == 1 && btn_bawah_last == 0);
    uint8_t btn_ok_pressed = (btn_ok_now == 1 && btn_ok_last == 0);

    // Update last state
    btn_atas_last = btn_atas_now;
    btn_bawah_last = btn_bawah_now;
    btn_ok_last = btn_ok_now;

    if (btn_atas_pressed)
    {
        uint8_t old_pos = cursor_pos;

        if (cursor_pos > 0) {
            cursor_pos--;
        } else {
            cursor_pos = current_page->num_items - 1;
        }

        // Update hanya 2 baris yang berubah
        uint16_t old_y = CONTENT_Y + (old_pos  * ITEM_H);
        uint16_t new_y = CONTENT_Y + (cursor_pos * ITEM_H);

        ui_draw_menu_item(old_y,
                          get_item_icon(old_pos),
                          current_page->items[old_pos].text,
                          current_page->items[old_pos].action != NULL,
                          0);

        ui_draw_menu_item(new_y,
                          get_item_icon(cursor_pos),
                          current_page->items[cursor_pos].text,
                          current_page->items[cursor_pos].action != NULL,
                          1);
    }
    else if (btn_bawah_pressed)
    {
        uint8_t old_pos = cursor_pos;

        if (cursor_pos < current_page->num_items - 1) {
            cursor_pos++;
        } else {
            cursor_pos = 0;
        }

        uint16_t old_y = CONTENT_Y + (old_pos   * ITEM_H);
        uint16_t new_y = CONTENT_Y + (cursor_pos * ITEM_H);

        ui_draw_menu_item(old_y,
                          get_item_icon(old_pos),
                          current_page->items[old_pos].text,
                          current_page->items[old_pos].action != NULL,
                          0);

        ui_draw_menu_item(new_y,
                          get_item_icon(cursor_pos),
                          current_page->items[cursor_pos].text,
                          current_page->items[cursor_pos].action != NULL,
                          1);
    }
    else if (btn_ok_pressed)
    {
        if (current_page->items[cursor_pos].action != NULL) {
            current_page->items[cursor_pos].action();
            page_changed = 1;  // Force full render setelah navigasi
        }
    }

    /* =========================================================
     * BAGIAN 2: RENDER UI (HANYA JIKA DIPERLUKAN)
     * ========================================================= */

    // Render full hanya saat halaman berubah
    if (page_changed) {
        page_changed = 0;
        last_cursor_pos = cursor_pos;

        // Clear screen dengan background color (lebih cepat dari fill)
        ST7735_FillScreenFast(COLOR_BG);

        // Header bar
        ui_draw_header(current_page->title);

        // Semua item menu (hanya sekali saat halaman load)
        for (uint8_t i = 0; i < current_page->num_items; i++) {
            uint16_t item_y = CONTENT_Y + (i * ITEM_H);
            ui_draw_menu_item(
                item_y,
                get_item_icon(i),
                current_page->items[i].text,
                current_page->items[i].action != NULL,
                (cursor_pos == i)
            );
        }

        // Status bar
        ui_draw_statusbar("U/D:GERAK  OK:PILIH", COLOR_GREEN);
    }
}

