/**
 * Display for agro drill LCD using Waveshare SKU 27078 7" inch ESP LCD
 * 
 * Optimized version with bug fixes and performance improvements:
 * - Fixed duplicate function definitions
 * - Improved memory management
 * - Better error handling
 * - Optimized UI updates
 * - Cleaner code structure
 */

#define LV_CONF_INCLUDE_SIMPLE  // place before lvgl.h
#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <esp_lcd_panel_ili9341.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lvgl_port.h>
#include <lvgl.h>

// Color definitions
#define COLOR_TEAL lv_color_make(0, 128, 128)
#define CUSTOM_COLOR_2 lv_color_make(255, 255, 100)

// Global variables for display management
esp_display_panel_handle_t board = nullptr;

// Function declarations
void backlight_slider_event_cb(lv_event_t *e);
void show_main_page();
void show_settings_page(lv_event_t *e = nullptr);
void show_debug_screen();
void alarm_ack_callback(lv_event_t *e);
void test_btn_cb(lv_event_t *e);
void start_warning_flash();
void stop_warning_flash();
void warning_timer_cb(lv_timer_t *timer);
void drill_box_anim_cb(void *obj, int32_t v);
void update_display_data();
void cleanup_ui_objects();

// UI Screen objects
lv_obj_t *main_scr = nullptr;
lv_obj_t *settings_scr = nullptr;
lv_obj_t *debug_screen = nullptr;

// Main screen UI objects
lv_obj_t *bin_left = nullptr;
lv_obj_t *lbl_bin_left_name = nullptr;
lv_obj_t *bin_right = nullptr;
lv_obj_t *lbl_bin_right_name = nullptr;
lv_obj_t *speed_label = nullptr;
lv_obj_t *rate_label = nullptr;
lv_obj_t *area_label = nullptr;
lv_obj_t *drill_box = nullptr;
lv_obj_t *drill_label = nullptr;
lv_obj_t *spinner = nullptr;
lv_obj_t *panel_speed = nullptr;
lv_obj_t *panel_rate = nullptr;
lv_obj_t *panel_area = nullptr;
lv_obj_t *settings_btn = nullptr;

// Settings screen UI objects
lv_obj_t *backlight_slider = nullptr;
lv_obj_t *alarm_btn = nullptr;

// Debug screen UI objects
lv_obj_t *debug_label = nullptr;
lv_obj_t *debug_label_1 = nullptr;
lv_obj_t *debug_label_2 = nullptr;
lv_obj_t *debug_label_3 = nullptr;
lv_obj_t *debug_label_4 = nullptr;

// Warning system
lv_obj_t *warn_label = nullptr;
lv_timer_t *warn_flash_timer = nullptr;

// Application state
bool test_mode_toggle_state = false;
unsigned long last_toggle_time = 0;
const unsigned long toggle_interval = 8000;  // 8 seconds
float currentSpeed = 0.0f;
float currentRate = 2.0f;
float totalArea = 0.0f;
bool ui_initialized = false;

// System configuration
const int MIN_BACKLIGHT_PERCENT = 10;
const int MAX_BACKLIGHT_PERCENT = 100;
const int DEFAULT_BACKLIGHT_PERCENT = 80;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting LCD Seeder Display...");

    // Initialize display panel
    esp_display_panel_config_t panel_config = {
        .width = 800,
        .height = 480,
        .data_width = 16,
        .pixel_format = ESP_LCD_COLOR_SPACE_RGB565,
    };

    board = esp_display_panel_new_ili9341(&panel_config, nullptr);
    if (!board) {
        Serial.println("ERROR: Failed to initialize display panel!");
        return;
    }

    // Initialize LVGL port
    lvgl_port_init(board);
    if (!lvgl_port_lock(-1)) {
        Serial.println("ERROR: Failed to acquire LVGL lock!");
        return;
    }

    Serial.println("Creating optimized UI...");
    create_main_screen();
    
    ui_initialized = true;
    lvgl_port_unlock();

    start_warning_flash();
    Serial.println("Setup complete!");
}

void loop() {
    if (!ui_initialized) {
        delay(100);
        return;
    }

    // Handle LVGL tasks
    lv_timer_handler();
    
    // Update display data periodically
    static unsigned long last_update = 0;
    if (millis() - last_update >= 100) {  // Update every 100ms
        update_display_data();
        last_update = millis();
    }

    // Test mode toggle
    if (millis() - last_toggle_time >= toggle_interval) {
        test_mode_toggle_state = !test_mode_toggle_state;
        last_toggle_time = millis();
        
        if (lvgl_port_lock(10)) {
            if (test_mode_toggle_state) {
                currentSpeed = 12.5f;
                currentRate = 150.0f;
                totalArea += 0.1f;
            } else {
                currentSpeed = 8.2f;
                currentRate = 120.0f;
                totalArea += 0.05f;
            }
            lvgl_port_unlock();
        }
    }

    delay(5);
}

void create_main_screen() {
    main_scr = lv_scr_act();
    lv_obj_set_style_bg_color(main_scr, lv_color_black(), 0);

    create_bin_level_bars();
    create_data_panels();
    create_drill_position_widget();
    create_spinner_widget();
    create_settings_button();
    create_test_button();
}

void create_bin_level_bars() {
    // Left bin
    bin_left = lv_bar_create(main_scr);
    lv_obj_set_size(bin_left, 80, 390);
    lv_obj_align(bin_left, LV_ALIGN_LEFT_MID, 30, 0);
    lv_obj_set_style_bg_color(bin_left, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bin_left, COLOR_TEAL, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_bar_set_value(bin_left, 75, LV_ANIM_OFF);

    lbl_bin_left_name = lv_label_create(main_scr);
    lv_label_set_text(lbl_bin_left_name, "LEFT BIN");
    lv_obj_set_style_text_color(lbl_bin_left_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_bin_left_name, &lv_font_montserrat_18, 0);
    lv_obj_align_to(lbl_bin_left_name, bin_left, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Right bin
    bin_right = lv_bar_create(main_scr);
    lv_obj_set_size(bin_right, 80, 390);
    lv_obj_align(bin_right, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_set_style_bg_color(bin_right, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bin_right, COLOR_TEAL, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_bar_set_value(bin_right, 45, LV_ANIM_OFF);

    lbl_bin_right_name = lv_label_create(main_scr);
    lv_label_set_text(lbl_bin_right_name, "RIGHT BIN");
    lv_obj_set_style_text_color(lbl_bin_right_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_bin_right_name, &lv_font_montserrat_18, 0);
    lv_obj_align_to(lbl_bin_right_name, bin_right, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void create_data_panels() {
    int current_y_offset = -150;
    int label_spacing = 110;
    int x_offset = -20;

    // Ground speed panel
    panel_speed = lv_obj_create(main_scr);
    lv_obj_set_size(panel_speed, 250, 100);
    lv_obj_set_style_bg_color(panel_speed, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_speed, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_speed, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(panel_speed, LV_ALIGN_CENTER, x_offset, current_y_offset);

    lv_obj_t *speed_title = lv_label_create(panel_speed);
    lv_label_set_text(speed_title, "GROUND SPEED");
    lv_obj_set_style_text_color(speed_title, lv_color_black(), 0);
    lv_obj_set_style_text_font(speed_title, &lv_font_montserrat_14, 0);
    lv_obj_align(speed_title, LV_ALIGN_TOP_MID, 0, 5);

    speed_label = lv_label_create(panel_speed);
    lv_label_set_text(speed_label, "0.0 km/h");
    lv_obj_set_style_text_color(speed_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_28, 0);
    lv_obj_center(speed_label);

    // Seeding rate panel
    current_y_offset += label_spacing;
    panel_rate = lv_obj_create(main_scr);
    lv_obj_set_size(panel_rate, 250, 100);
    lv_obj_set_style_bg_color(panel_rate, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_rate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_rate, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(panel_rate, LV_ALIGN_CENTER, x_offset, current_y_offset);

    lv_obj_t *rate_title = lv_label_create(panel_rate);
    lv_label_set_text(rate_title, "SEEDING RATE");
    lv_obj_set_style_text_color(rate_title, lv_color_black(), 0);
    lv_obj_set_style_text_font(rate_title, &lv_font_montserrat_14, 0);
    lv_obj_align(rate_title, LV_ALIGN_TOP_MID, 0, 5);

    rate_label = lv_label_create(panel_rate);
    lv_label_set_text(rate_label, "0 kg/ha");
    lv_obj_set_style_text_color(rate_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(rate_label, &lv_font_montserrat_30, 0);
    lv_obj_center(rate_label);

    // Total area panel
    current_y_offset += label_spacing;
    panel_area = lv_obj_create(main_scr);
    lv_obj_set_size(panel_area, 250, 100);
    lv_obj_set_style_bg_color(panel_area, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_area, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_area, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(panel_area, LV_ALIGN_CENTER, x_offset, current_y_offset);

    lv_obj_t *area_title = lv_label_create(panel_area);
    lv_label_set_text(area_title, "TOTAL AREA");
    lv_obj_set_style_text_color(area_title, lv_color_black(), 0);
    lv_obj_set_style_text_font(area_title, &lv_font_montserrat_14, 0);
    lv_obj_align(area_title, LV_ALIGN_TOP_MID, 0, 5);

    area_label = lv_label_create(panel_area);
    lv_label_set_text(area_label, "0.0 ha");
    lv_obj_set_style_text_color(area_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(area_label, &lv_font_montserrat_32, 0);
    lv_obj_center(area_label);
}

void create_drill_position_widget() {
    drill_box = lv_obj_create(main_scr);
    lv_obj_set_size(drill_box, 180, 110);
    lv_obj_set_style_pad_all(drill_box, 9, 0);
    lv_obj_set_style_bg_color(drill_box, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(drill_box, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(drill_box, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(drill_box, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(drill_box, LV_ALIGN_TOP_LEFT, 150, 20);

    drill_label = lv_label_create(drill_box);
    lv_label_set_text(drill_label, "DRILL\nPOSITION\nDOWN");
    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(drill_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(drill_label);
}

void create_spinner_widget() {
    spinner = lv_spinner_create(main_scr, 750, 40);
    lv_obj_set_size(spinner, 135, 135);
    lv_obj_align(spinner, LV_ALIGN_TOP_RIGHT, -200, 140);
    lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 19, LV_PART_MAIN);

    lv_obj_t *spinner_label = lv_label_create(spinner);
    lv_label_set_text(spinner_label, "CLUTCH\nENGAGED");
    lv_obj_set_style_text_align(spinner_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(spinner_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(spinner_label, &lv_font_montserrat_18, 0);
    lv_obj_center(spinner_label);
}

void create_settings_button() {
    settings_btn = lv_btn_create(main_scr);
    lv_obj_set_width(settings_btn, 160);
    lv_obj_set_height(settings_btn, 60);
    lv_obj_set_style_pad_all(settings_btn, 10, 0);
    lv_obj_set_style_border_width(settings_btn, 3, 0);
    lv_obj_set_style_border_color(settings_btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(settings_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(settings_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_text_color(settings_btn, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_COVER, 0);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -70, 20);

    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, "SETTINGS");
    lv_obj_set_style_text_font(settings_label, &lv_font_montserrat_18, 0);
    lv_obj_center(settings_label);

    lv_obj_add_event_cb(settings_btn, show_settings_page, LV_EVENT_CLICKED, nullptr);
}

void create_test_button() {
    lv_obj_t *test_btn = lv_btn_create(main_scr);
    lv_obj_set_size(test_btn, 120, 50);
    lv_obj_align(test_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    lv_obj_t *test_label = lv_label_create(test_btn);
    lv_label_set_text(test_label, "TEST TOUCH");
    lv_obj_center(test_label);
    
    lv_obj_add_event_cb(test_btn, test_btn_cb, LV_EVENT_CLICKED, nullptr);
}

void update_display_data() {
    if (!lvgl_port_lock(10)) return;

    static char buffer[32];

    // Update speed
    if (speed_label) {
        snprintf(buffer, sizeof(buffer), "%.1f km/h", currentSpeed);
        lv_label_set_text(speed_label, buffer);
    }

    // Update rate
    if (rate_label) {
        snprintf(buffer, sizeof(buffer), "%.0f kg/ha", currentRate);
        lv_label_set_text(rate_label, buffer);
    }

    // Update area
    if (area_label) {
        snprintf(buffer, sizeof(buffer), "%.1f ha", totalArea);
        lv_label_set_text(area_label, buffer);
    }

    lvgl_port_unlock();
}

void show_main_page() {
    if (main_scr && lvgl_port_lock(10)) {
        lv_scr_load(main_scr);
        lvgl_port_unlock();
    }
}

void show_settings_page(lv_event_t *e) {
    if (!lvgl_port_lock(10)) return;

    if (!settings_scr) {
        settings_scr = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(settings_scr, lv_color_black(), 0);

        // Title
        lv_obj_t *title = lv_label_create(settings_scr);
        lv_label_set_text(title, "Settings");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        // Alarm button
        alarm_btn = lv_btn_create(settings_scr);
        lv_obj_set_size(alarm_btn, 200, 80);
        lv_obj_set_style_bg_color(alarm_btn, CUSTOM_COLOR_2, 0);
        lv_obj_set_style_text_color(alarm_btn, lv_color_black(), 0);
        lv_obj_align(alarm_btn, LV_ALIGN_CENTER, 0, -50);

        lv_obj_t *alarm_label = lv_label_create(alarm_btn);
        lv_label_set_text(alarm_label, "PUSH TO\nCLEAR ALARM");
        lv_obj_set_style_text_align(alarm_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(alarm_label, &lv_font_montserrat_18, 0);
        lv_obj_center(alarm_label);
        lv_obj_add_event_cb(alarm_btn, alarm_ack_callback, LV_EVENT_CLICKED, nullptr);

        // Backlight slider
        lv_obj_t *slider_label = lv_label_create(settings_scr);
        lv_label_set_text(slider_label, "Backlight");
        lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);
        lv_obj_align(slider_label, LV_ALIGN_CENTER, 0, 50);

        backlight_slider = lv_slider_create(settings_scr);
        lv_obj_set_width(backlight_slider, 300);
        lv_slider_set_range(backlight_slider, MIN_BACKLIGHT_PERCENT, MAX_BACKLIGHT_PERCENT);
        lv_slider_set_value(backlight_slider, DEFAULT_BACKLIGHT_PERCENT, LV_ANIM_OFF);
        lv_obj_align_to(backlight_slider, slider_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(backlight_slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        // Debug label
        debug_label = lv_label_create(settings_scr);
        lv_label_set_text(debug_label, "Ready");
        lv_obj_set_style_text_color(debug_label, lv_color_white(), 0);
        lv_obj_align(debug_label, LV_ALIGN_CENTER, 0, 120);

        // Debug button
        lv_obj_t *debug_btn = lv_btn_create(settings_scr);
        lv_obj_set_size(debug_btn, 120, 50);
        lv_obj_align(debug_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_t *debug_btn_label = lv_label_create(debug_btn);
        lv_label_set_text(debug_btn_label, "DEBUG");
        lv_obj_center(debug_btn_label);
        lv_obj_add_event_cb(debug_btn, [](lv_event_t *e) { show_debug_screen(); }, LV_EVENT_CLICKED, nullptr);

        // Back button
        lv_obj_t *back_btn = lv_btn_create(settings_scr);
        lv_obj_set_size(back_btn, 120, 50);
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "BACK");
        lv_obj_center(back_label);
        lv_obj_add_event_cb(back_btn, [](lv_event_t *e) { show_main_page(); }, LV_EVENT_CLICKED, nullptr);
    }

    lv_scr_load(settings_scr);
    lvgl_port_unlock();
}

void show_debug_screen() {
    if (!lvgl_port_lock(10)) return;

    if (!debug_screen) {
        debug_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(debug_screen, lv_color_black(), 0);

        lv_obj_t *title = lv_label_create(debug_screen);
        lv_label_set_text(title, "Debug Information");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        debug_label_1 = lv_label_create(debug_screen);
        lv_label_set_text(debug_label_1, "Speed: 0.0 km/h");
        lv_obj_set_style_text_color(debug_label_1, lv_color_white(), 0);
        lv_obj_align(debug_label_1, LV_ALIGN_TOP_LEFT, 20, 80);

        debug_label_2 = lv_label_create(debug_screen);
        lv_label_set_text(debug_label_2, "Rate: 0 kg/ha");
        lv_obj_set_style_text_color(debug_label_2, lv_color_white(), 0);
        lv_obj_align(debug_label_2, LV_ALIGN_TOP_LEFT, 20, 110);

        debug_label_3 = lv_label_create(debug_screen);
        lv_label_set_text(debug_label_3, "Area: 0.0 ha");
        lv_obj_set_style_text_color(debug_label_3, lv_color_white(), 0);
        lv_obj_align(debug_label_3, LV_ALIGN_TOP_LEFT, 20, 140);

        debug_label_4 = lv_label_create(debug_screen);
        lv_label_set_text(debug_label_4, "System: OK");
        lv_obj_set_style_text_color(debug_label_4, lv_color_white(), 0);
        lv_obj_align(debug_label_4, LV_ALIGN_TOP_LEFT, 20, 170);

        lv_obj_t *back_btn = lv_btn_create(debug_screen);
        lv_obj_set_size(back_btn, 160, 60);
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "BACK TO SETTINGS");
        lv_obj_center(back_label);
        lv_obj_add_event_cb(back_btn, [](lv_event_t *e) { show_settings_page(); }, LV_EVENT_CLICKED, nullptr);
    }

    // Update debug information
    static char buffer[64];
    if (debug_label_1) {
        snprintf(buffer, sizeof(buffer), "Speed: %.1f km/h", currentSpeed);
        lv_label_set_text(debug_label_1, buffer);
    }
    if (debug_label_2) {
        snprintf(buffer, sizeof(buffer), "Rate: %.0f kg/ha", currentRate);
        lv_label_set_text(debug_label_2, buffer);
    }
    if (debug_label_3) {
        snprintf(buffer, sizeof(buffer), "Area: %.1f ha", totalArea);
        lv_label_set_text(debug_label_3, buffer);
    }
    if (debug_label_4) {
        snprintf(buffer, sizeof(buffer), "Free Heap: %d bytes", ESP.getFreeHeap());
        lv_label_set_text(debug_label_4, buffer);
    }

    lv_scr_load(debug_screen);
    lvgl_port_unlock();
}

void backlight_slider_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    int percent = lv_slider_get_value(lv_event_get_target(e));
    
    // Clamp to safe range
    if (percent < MIN_BACKLIGHT_PERCENT) {
        percent = MIN_BACKLIGHT_PERCENT;
        lv_slider_set_value(lv_event_get_target(e), percent, LV_ANIM_OFF);
    }

    // Update debug display
    if (debug_label) {
        static char buffer[32];
        snprintf(buffer, sizeof(buffer), "Backlight: %d%%", percent);
        lv_label_set_text(debug_label, buffer);
    }

    // Set hardware backlight
    if (board) {
        auto backlight = board->getBacklight();
        if (backlight) {
            int pwm_value = map(percent, 0, 100, 0, 255);
            backlight->setBrightness(pwm_value);
        }
    }
}

void alarm_ack_callback(lv_event_t *e) {
    Serial.println("Alarm acknowledged");
    stop_warning_flash();
    
    if (debug_label) {
        lv_label_set_text(debug_label, "Alarm cleared");
    }
}

void test_btn_cb(lv_event_t *e) {
    Serial.println("Test button pressed");
    
    if (debug_label) {
        lv_label_set_text(debug_label, "Touch test OK");
    }
}

void start_warning_flash() {
    if (!warn_label) {
        warn_label = lv_label_create(main_scr);
        lv_label_set_text(warn_label, "WARNING!");
        lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_text_font(warn_label, &lv_font_montserrat_42, 0);
        lv_obj_align(warn_label, LV_ALIGN_CENTER, -20, -60);
    }
    
    if (!warn_flash_timer) {
        warn_flash_timer = lv_timer_create(warning_timer_cb, 500, nullptr);
    }
}

void stop_warning_flash() {
    if (warn_flash_timer) {
        lv_timer_del(warn_flash_timer);
        warn_flash_timer = nullptr;
    }
    if (warn_label) {
        lv_obj_add_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void warning_timer_cb(lv_timer_t *timer) {
    if (warn_label) {
        if (lv_obj_has_flag(warn_label, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void drill_box_anim_cb(void *obj, int32_t v) {
    lv_obj_set_style_bg_opa((lv_obj_t *)obj, v, 0);
}

void cleanup_ui_objects() {
    // Clean up timers
    if (warn_flash_timer) {
        lv_timer_del(warn_flash_timer);
        warn_flash_timer = nullptr;
    }
    
    // Note: LVGL handles object cleanup when screens are deleted
    // Manual cleanup is generally not needed unless you're managing
    // objects outside of screen hierarchy
}