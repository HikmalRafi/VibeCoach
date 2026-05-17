/**
 * @file    app_menu.c
 * @brief   Menu navigation system for STM32-based training device.
 * @details Handles page rendering, button navigation, and training session
 *          launching with a navy dark theme UI.
 */

#include "../../App/Inc/app_menu.h"
#include "../../Bsp/Inc/bsp_led_rgb.h"

/* =========================================================================
 * NAVY DARK THEME COLOR PALETTE (RGB565)
 * ========================================================================= */

/** @brief Main background colour (dark navy). */
#define COLOR_BG         ST7735_COLOR565(15,  23,  42)
/** @brief Card/surface colour (slightly lighter navy). */
#define COLOR_SURFACE    ST7735_COLOR565(22,  32,  55)
/** @brief Header bar background. */
#define COLOR_HDR_BG     ST7735_COLOR565(18,  28,  50)
/** @brief Status bar background. */
#define COLOR_SBAR_BG    ST7735_COLOR565(18,  28,  50)
/** @brief Selected item highlight background. */
#define COLOR_SELECT_BG  ST7735_COLOR565(30,  45,  75)

/** @brief Primary text colour (near white). */
#define COLOR_TEXT       ST7735_COLOR565(220, 228, 240)
/** @brief Secondary/subtitle text colour. */
#define COLOR_SUBTEXT    ST7735_COLOR565(140, 155, 180)
/** @brief Hint/disabled text colour. */
#define COLOR_HINT       ST7735_COLOR565(80,  95,  120)
/** @brief Selected item text colour (bright white). */
#define COLOR_SELECT_FG  ST7735_COLOR565(240, 245, 255)

/** @brief Accent colour (cyan/blue) — used for selection indicator and icons. */
#define COLOR_ACCENT     ST7735_COLOR565(56,  189, 248)
/** @brief Separator line colour. */
#define COLOR_SEP        ST7735_COLOR565(35,  50,  75)

/** @brief Success/positive indicator (green). */
#define COLOR_SUCCESS    ST7735_COLOR565(52,  211, 153)
/** @brief Caution/info indicator (blue). */
#define COLOR_CAUTION    ST7735_COLOR565(96,  165, 250)
/** @brief Warning indicator (yellow/amber). */
#define COLOR_WARNING    ST7735_COLOR565(251, 191, 36)
/** @brief Danger/error indicator (red). */
#define COLOR_DANGER     ST7735_COLOR565(248, 113, 113)

/* =========================================================================
 * LAYOUT CONSTANTS (in pixels)
 * ========================================================================= */

#define HDR_H       12    /**< Header bar height.                        */
#define CONTENT_Y   (HDR_H + 1) /**< Y-offset where menu items start.   */
#define ITEM_H      12    /**< Height of each menu item row.             */
#define LCD_W       ST7735_WIDTH   /**< Display width.                  */
#define LCD_H       ST7735_HEIGHT  /**< Display height.                 */
#define SBAR_H      9     /**< Status bar height (bottom).              */

/* =========================================================================
 * GLOBAL STATE VARIABLES
 * ========================================================================= */

static button_t btn_up;      /**< Button instance for UP navigation.     */
static button_t btn_down;    /**< Button instance for DOWN navigation.   */
static button_t btn_ok;      /**< Button instance for OK/Select action.  */

volatile uint8_t  point       = 0;   /**< (Reserved for future use).      */
static   uint8_t  init_check  = 0;   /**< First-run initialisation flag.  */
volatile uint8_t  training_is = 0;   /**< Flag: 1 = training session active. */

uint8_t page_changed    = 1;         /**< Flag: triggers full page redraw. */
static uint8_t last_cursor_pos = 0;  /**< Previous cursor position (for redraw optimisation). */

volatile uint8_t    cursor_pos   = 0;       /**< Currently selected item index. */
const MenuPage_t*   current_page = NULL;    /**< Pointer to the active menu page. */

/* Forward declarations — defined at the bottom of this file */
extern const MenuPage_t page_main;
extern const MenuPage_t page_deltoid;
extern const MenuPage_t page_pushup;

/* =========================================================================
 * UI DRAWING HELPERS
 * ========================================================================= */

/**
 * @brief Draw the header bar with title text.
 * @param title Text to display in the header.
 */
static void ui_draw_header(const char* title)
{
    ST7735_FillRectangle(0, 0, LCD_W, HDR_H, COLOR_HDR_BG);
    ST7735_WriteString(5, 2, title, Font_7x10, COLOR_SUBTEXT, COLOR_HDR_BG);
    bsp_battery_draw_icon(LCD_W - 18, 2, COLOR_HDR_BG);
}

/**
 * @brief Draw the bottom status bar with hint text and indicator dot.
 * @param hint     Help text (e.g., "UP/DOWN:MOVE  OK:SELECT").
 * @param dot_color Colour of the small indicator dot (far right).
 */
static void ui_draw_statusbar(const char* hint, uint16_t dot_color)
{
    uint16_t y = LCD_H - SBAR_H;
    ST7735_FillRectangle(0, y, LCD_W, SBAR_H, COLOR_SBAR_BG);
    // Draw 1-pixel separator line at the top of the status bar
    ST7735_FillRectangle(0, y, LCD_W, 1, COLOR_SEP);
    ST7735_WriteString(5, y + 1, hint, Font_7x10, COLOR_HINT, COLOR_SBAR_BG);
    // Small indicator dot (3x3 pixels) at the right edge
    ST7735_FillRectangle(LCD_W - 7, y + 3, 3, 3, dot_color);
}

/**
 * @brief Draw a single menu item row.
 * @param y           Y-coordinate of the item row.
 * @param icon_char   Single character to display as an icon.
 * @param text        Menu item label text.
 * @param has_arrow   1 = show ">" arrow (indicating submenu/action).
 * @param is_selected 1 = this item is currently highlighted.
 */
static void ui_draw_menu_item(uint16_t y, char icon_char,
                               const char* text,
                               uint8_t has_arrow, uint8_t is_selected)
{
    uint16_t bg = is_selected ? COLOR_SELECT_BG : COLOR_BG;
    uint16_t fg = is_selected ? COLOR_SELECT_FG : COLOR_TEXT;

    // Fill entire row with background colour
    ST7735_FillRectangle(0, y, LCD_W, ITEM_H, bg);

    // Draw accent bar (2px wide) on the left edge if selected
    if (is_selected) {
        ST7735_FillRectangle(0, y, 2, ITEM_H, COLOR_ACCENT);
    }

    // Draw icon character (single char in a 2-char string with null terminator)
    char icon_str[2] = {icon_char, '\0'};
    ST7735_WriteString(6, y + 1, icon_str, Font_7x10,
                       is_selected ? COLOR_ACCENT : COLOR_SUBTEXT, bg);

    // Draw menu item text
    ST7735_WriteString(20, y + 1, text, Font_7x10, fg, bg);

    // Draw right-arrow indicator if this item has an action/submenu
    if (has_arrow) {
        ST7735_WriteString(LCD_W - 10, y + 1, ">", Font_7x10, COLOR_HINT, bg);
    }
}

/* =========================================================================
 * NAVIGATION FUNCTIONS
 * ========================================================================= */

/** @brief Navigate to the main menu page. */
void goto_main_menu(void)    { current_page = &page_main;    cursor_pos = 0; page_changed = 1; }

/** @brief Navigate to the deltoid exercise submenu. */
void goto_deltoid_menu(void) { current_page = &page_deltoid; cursor_pos = 0; page_changed = 1; }

/** @brief Navigate to the push-up exercise submenu. */
void goto_pushup_menu(void)  { current_page = &page_pushup;  cursor_pos = 0; page_changed = 1; }

/**
 * @brief Go back to the parent menu page.
 * @details If the current page has a parent (i.e., it's a submenu),
 *          navigate to it. Otherwise this does nothing.
 */
void action_back(void) {
    if (current_page->parent != NULL) {
        current_page = current_page->parent;
        cursor_pos   = 0;
        page_changed = 1;
    }
}

/* =========================================================================
 * TRAINING ACTION
 * ========================================================================= */

/**
 * @brief Start a deltoid training session.
 * @details Resets the score, then enters a loop that continuously calls
 *          add_sensor_data() to process IMU data and run the classifier.
 *          The loop exits when the OK button is pressed, which sets
 *          training_is = 0 and marks the device as uncalibrated (so the
 *          next session will re-run calibration).
 *
 *          Note: add_sensor_data() internally handles:
 *          - Calibration (if not yet done)
 *          - Reading the inverse quaternion (invQ)
 *          - Buffering and classifying sensor data
 *          No need to call these separately.
 */
void action_start_deltoid(void)
{
    training_is = 1;

    // Reset score for a fresh training session
    extern void tinyml_reset_score(void);
    tinyml_reset_score();

    // Training loop — runs until OK button is pressed
    while (training_is == 1)
    {
        // This single call handles everything: calibration, IMU read,
        // quaternion correction, buffering, classification, and feedback.
        add_sensor_data();

        // Check for OK button press to exit training
        if (button_read(&btn_ok) == 1) {
            training_is = 0;          // Exit the training loop
            calibrated  = false;      // Force re-calibration on next session
        }
    }
}

/* =========================================================================
 * MENU PAGE DEFINITIONS
 * ========================================================================= */

/* --- Push-Up Submenu --- */
const MenuItem_t items_pushup[] = {
    {"START",   action_start_deltoid},  // Launch training session
    {"SETTING", NULL},                   // Placeholder — no action yet
    {"BACK",    action_back}             // Return to main menu
};
const MenuPage_t page_pushup = {
    "PUSH UP", items_pushup, 3, &page_main
};

/* --- Deltoid Submenu --- */
const MenuItem_t items_deltoid[] = {
    {"START",   action_start_deltoid},  // Launch training session
    {"SETTING", NULL},                   // Placeholder — no action yet
    {"BACK",    action_back}             // Return to main menu
};
const MenuPage_t page_deltoid = {
    "DELTOID", items_deltoid, 3, &page_main
};

/* --- Main Menu (top level) --- */
const MenuItem_t items_main[] = {
    {"Deltoid", goto_deltoid_menu},     // Navigate to deltoid submenu
    {"Push Up", goto_pushup_menu}       // Navigate to push-up submenu
};
const MenuPage_t page_main = {
    "TRAINING", items_main, 2, NULL     // NULL = no parent (top-level page)
};

/* =========================================================================
 * ICON MAPPING
 * ========================================================================= */

/** @brief Icons for main menu items: D = Deltoid, P = Push Up. */
static const char icons_main[]    = {'D', 'P'};
/** @brief Icons for deltoid submenu: > = action, * = setting, < = back. */
static const char icons_deltoid[] = {'>', '*', '<'};
/** @brief Icons for push-up submenu. */
static const char icons_pushup[]  = {'>', '*', '<'};

/**
 * @brief Get the icon character for a menu item based on current page.
 * @param index Item index within the current page.
 * @return Single character to display as the item's icon.
 */
static char get_item_icon(uint8_t index)
{
    if (current_page == &page_main    && index < sizeof(icons_main))    return icons_main[index];
    if (current_page == &page_deltoid && index < sizeof(icons_deltoid)) return icons_deltoid[index];
    if (current_page == &page_pushup  && index < sizeof(icons_pushup))  return icons_pushup[index];
    return '>';  // Default fallback icon
}

/* =========================================================================
 * MAIN MENU FUNCTION (call repeatedly from main loop)
 * ========================================================================= */

/**
 * @brief Main menu loop — handles rendering and button input.
 * @details Call this function repeatedly in the main while(1) loop.
 *          On first call, it initialises buttons and the display.
 *          It handles:
 *          - UP/DOWN button navigation with single-item redraw
 *          - OK button to trigger menu actions
 *          - Full page redraw when page_changed flag is set
 */
void training_menu(void)
{
    // --- First-run initialisation ---
    if (init_check == 0) {
        button_init(&btn_up,   GPIOB, GPIO_PIN_12);  // PB12 = UP
        button_init(&btn_down, GPIOB, GPIO_PIN_13);  // PB13 = DOWN
        button_init(&btn_ok,   GPIOB, GPIO_PIN_14);  // PB14 = OK
        current_page = &page_main;                    // Start at main menu
        init_check   = 1;
        page_changed = 1;                             // Force initial draw
        ST7735_FillScreenFast(COLOR_BG);              // Clear screen
    }

    if (current_page == NULL) return;  // Safety: no page loaded

    // --- Button state tracking (edge detection) ---
    static uint8_t btn_up_last   = 0;
    static uint8_t btn_down_last = 0;
    static uint8_t btn_ok_last   = 0;

    uint8_t btn_up_now   = button_read(&btn_up);
    uint8_t btn_down_now = button_read(&btn_down);
    uint8_t btn_ok_now   = button_read(&btn_ok);

    // Detect rising edges (press, not hold)
    uint8_t btn_up_pressed   = (btn_up_now   == 1 && btn_up_last   == 0);
    uint8_t btn_down_pressed = (btn_down_now == 1 && btn_down_last == 0);
    uint8_t btn_ok_pressed   = (btn_ok_now   == 1 && btn_ok_last   == 0);

    // Store current state for next iteration's edge detection
    btn_up_last   = btn_up_now;
    btn_down_last = btn_down_now;
    btn_ok_last   = btn_ok_now;

    // --- Button: UP ---
    if (btn_up_pressed) {
        uint8_t old_pos = cursor_pos;
        // Wrap around: if at first item, go to last item
        cursor_pos = (cursor_pos > 0) ? cursor_pos - 1 : current_page->num_items - 1;
        uint16_t old_y = CONTENT_Y + (old_pos    * ITEM_H);
        uint16_t new_y = CONTENT_Y + (cursor_pos * ITEM_H);
        // Redraw old item as unselected
        ui_draw_menu_item(old_y, get_item_icon(old_pos),
                          current_page->items[old_pos].text,
                          current_page->items[old_pos].action != NULL, 0);
        // Redraw new item as selected
        ui_draw_menu_item(new_y, get_item_icon(cursor_pos),
                          current_page->items[cursor_pos].text,
                          current_page->items[cursor_pos].action != NULL, 1);
    }
    // --- Button: DOWN ---
    else if (btn_down_pressed) {
        uint8_t old_pos = cursor_pos;
        // Wrap around: if at last item, go to first item
        cursor_pos = (cursor_pos < current_page->num_items - 1) ? cursor_pos + 1 : 0;
        uint16_t old_y = CONTENT_Y + (old_pos    * ITEM_H);
        uint16_t new_y = CONTENT_Y + (cursor_pos * ITEM_H);
        // Redraw old item as unselected
        ui_draw_menu_item(old_y, get_item_icon(old_pos),
                          current_page->items[old_pos].text,
                          current_page->items[old_pos].action != NULL, 0);
        // Redraw new item as selected
        ui_draw_menu_item(new_y, get_item_icon(cursor_pos),
                          current_page->items[cursor_pos].text,
                          current_page->items[cursor_pos].action != NULL, 1);
    }
    // --- Button: OK ---
    else if (btn_ok_pressed) {
        // Execute the action associated with the selected item (if any)
        if (current_page->items[cursor_pos].action != NULL) {
            current_page->items[cursor_pos].action();
            page_changed = 1;  // Trigger full redraw when we return
        }
    }

    // --- Full page redraw (only when page_changed flag is set) ---
    if (page_changed) {
        page_changed    = 0;
        last_cursor_pos = cursor_pos;
        ST7735_FillScreenFast(COLOR_BG);               // Clear screen
        ui_draw_header(current_page->title);            // Draw header

        // Draw all menu items
        for (uint8_t i = 0; i < current_page->num_items; i++) {
            uint16_t item_y = CONTENT_Y + (i * ITEM_H);
            ui_draw_menu_item(item_y, get_item_icon(i),
                              current_page->items[i].text,
                              current_page->items[i].action != NULL,
                              (cursor_pos == i));       // Highlight if selected
        }

        // Draw bottom status bar with navigation hints
        ui_draw_statusbar("UP/DOWN:MOVE  OK:SELECT", COLOR_SUCCESS);
    }
}
