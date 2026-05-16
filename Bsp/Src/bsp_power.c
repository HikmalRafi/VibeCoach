/**
 * @file    bsp_power.c
 * @brief   Power toggle ON/OFF via single press PB10 (pull-up, Active LOW)
 *
 * @details Pakai library button_t yang sudah ada — tidak ada modifikasi apapun
 *          pada bsp_debounce_button. Cukup button_init() + button_read().
 *
 *          Flow:
 *            while(1)
 *              └─ bsp_power_poll()
 *                    └─ button_read(&btn_power) == 1  →  press terdeteksi
 *                          └─ bsp_power_shutdown()
 *                                ├─ on_shutdown_callback()  : LCD off, LED off
 *                                ├─ tunggu tombol dilepas
 *                                ├─ HAL_PWR_EnterSTOPMode() : tidur ~100uA
 *                                │       (tekan tombol → falling EXTI → wake)
 *                                ├─ SystemClock_Config()    : reinit HSE/PLL
 *                                ├─ tunggu tombol dilepas
 *                                ├─ on_wakeup_callback()    : LCD on, splash
 *                                └─ return ke while(1)
 */

#include "../../Bsp/Inc/bsp_power.h"

extern void SystemClock_Config(void);

/* =========================================================================
 * STATE INTERNAL
 * ========================================================================= */

static button_t btn_power;       /* Instance button untuk tombol power       */
static uint8_t  pwr_wakeup_flag; /* Flag: baru saja wake dari STOP mode      */

/* =========================================================================
 * INIT
 * ========================================================================= */

void bsp_power_init(void)
{
    /*
     * Pakai button_init() biasa — Active LOW, sama persis dengan
     * tombol navigasi lainnya. Pull-up membuat idle = HIGH,
     * saat ditekan → LOW → button_read() return 1.
     */
    button_init(&btn_power, PWR_BTN_PORT, PWR_BTN_PIN);

    pwr_wakeup_flag = 0;
}

/* =========================================================================
 * POLL — panggil di dalam while(1) setiap loop iteration
 * ========================================================================= */

void bsp_power_poll(void)
{
    /*
     * button_read() sudah handle:
     *   - Debounce 20ms
     *   - Edge detection (return 1 hanya sekali saat pertama ditekan)
     *   - Active LOW (trigger saat BtnState == 0)
     *
     * Tidak perlu tambah logika apapun di sini.
     */
    if (button_read(&btn_power) == 1)
    {
        bsp_power_shutdown();

        /*
         * Setelah wake dari STOP mode, bsp_power_shutdown() return ke sini.
         * Re-init btn_power agar state bersih — siap detect press berikutnya.
         */
        button_init(&btn_power, PWR_BTN_PORT, PWR_BTN_PIN);
    }
}

/* =========================================================================
 * SHUTDOWN → STOP MODE → WAKE
 * ========================================================================= */

void bsp_power_shutdown(void)
{
    /* ── Step 1: Callback shutdown ──────────────────────────────────────────
     * Matikan LCD, LED, tampilkan animasi shutdown.
     * Didefinisikan di app_menu_power.c (override weak default).
     * ──────────────────────────────────────────────────────────────────── */
    bsp_power_on_shutdown_callback();

    /* ── Step 2: Tunggu tombol DILEPAS ──────────────────────────────────────
     * Wajib sebelum masuk STOP mode.
     * Jika PB10 masih LOW saat EnterSTOPMode dipanggil,
     * EXTI langsung fire → MCU tidak sempat tidur sama sekali.
     * ──────────────────────────────────────────────────────────────────── */
    uint32_t t = HAL_GetTick();
    while (PWR_BTN_IS_PRESSED())
    {
        if ((HAL_GetTick() - t) > 5000) break; /* safety: max tunggu 5 detik */
        HAL_Delay(10);
    }
    HAL_Delay(200); /* debounce setelah tombol dilepas */

    /* ── Step 3: Masuk STOP mode ─────────────────────────────────────────────
     * MCU tidur di sini dengan konsumsi ~100uA.
     *
     * MCU akan wake saat ada falling edge di PB10:
     *   Tombol ditekan → PB10 HIGH→LOW → EXTI falling → wake
     *
     * Syarat CubeMX (wajib, tanpa ini MCU tidak bisa wake):
     *   PB10 Mode : External Interrupt - Falling edge trigger
     *   Pull      : Pull-up (atau external pull-up)
     *   NVIC      : EXTI line[15:10] interrupts → centang Enabled
     * ──────────────────────────────────────────────────────────────────── */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* ═══════════════════════════════════════════════════════════════════════
     *  EKSEKUSI LANJUT DI SINI SETELAH WAKE
     *  STOP mode tidak reset MCU — program lanjut dari titik ini.
     * ═══════════════════════════════════════════════════════════════════════ */

    /* ── Step 4: Reinit system clock ────────────────────────────────────────
     * STOP mode mematikan HSE dan PLL → MCU jalan di HSI ~16MHz.
     * Tanpa reinit:
     *   - HAL_Delay(1000) terasa jadi ~6 detik
     *   - LCD timing salah → display corrupt
     *   - UART baud rate meleset → data kacau
     * ──────────────────────────────────────────────────────────────────── */
    SystemClock_Config();

    /* ── Step 5: Set flag post-wake ─────────────────────────────────────── */
    pwr_wakeup_flag = 1;

    /* ── Step 6: Tunggu tombol dilepas setelah wake ──────────────────────────
     * User mungkin masih menahan tombol saat MCU baru bangun.
     * Jika tidak ditunggu → button_read() di poll langsung detect press lagi
     * → langsung shutdown lagi tanpa sempat nyala.
     * ──────────────────────────────────────────────────────────────────── */
    t = HAL_GetTick();
    while (PWR_BTN_IS_PRESSED())
    {
        if ((HAL_GetTick() - t) > 5000) break;
        HAL_Delay(10);
    }
    HAL_Delay(200); /* debounce setelah dilepas */

    /* ── Step 7: Reset flag ─────────────────────────────────────────────── */
    pwr_wakeup_flag = 0;

    /* ── Step 8: Callback wakeup ─────────────────────────────────────────────
     * Reinit SPI, LCD, nyalakan LED, tampilkan splash wake.
     * ──────────────────────────────────────────────────────────────────── */
    bsp_power_on_wakeup_callback();

    /* Return ke bsp_power_poll() → return ke while(1) → device ON kembali */
}

/* =========================================================================
 * GETTER FLAG WAKEUP
 * ========================================================================= */

uint8_t bsp_power_is_wakeup(void)
{
    return pwr_wakeup_flag;
}

/* =========================================================================
 * DEFAULT CALLBACK — weak, override di app_menu_power.c
 * ========================================================================= */

__attribute__((weak)) void bsp_power_on_shutdown_callback(void) { }
__attribute__((weak)) void bsp_power_on_wakeup_callback(void)   { }
