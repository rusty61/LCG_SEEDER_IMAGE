            /**
            * display for agro drill lcd using waveshare SKU 27078 7" inch esp l
cd
            *
            * Version with "Push to Clear Alarms" button and Backlight slider/la
bel moved to the Settings screen.
            */

            #define LV_CONF_INCLUDE_SIMPLE  // place before lvgl.h
            #include <Arduino.h>
            #include <esp_display_panel.hpp>
            #include <lvgl.h>
            #include "lvgl_v8_port.h"
            #include "color.h"
            #include "esp_panel_board_custom_conf.h"
            #include "alarm_buttons.h" // Added for new alarm buttons


            using namespace esp_panel::drivers;
            using namespace esp_panel::board;

            #if LVGL_PORT_AVOID_TEARING_MODE
            #error "This example does not support the avoid tearing function. Pl
ease set `LVGL_PORT_AVOID_TEARING_MODE` to `0` in the `lvgl_v8_port.h` file."
            #endif

            Board *board = nullptr;  // Global variable

            // Buffers to hold formatted strings
            char speedBuf[16], rateBuf[16], areaBuf[16];


            #define BACKLIGHT_CHANNEL 0
            #define BACKLIGHT_FREQ 5000
            #define BACKLIGHT_RES 8  // 8-bit: brightness from 0 to 255


            // Forward declarations for LVGL callback functions
            void backlight_slider_event_cb(lv_event_t *e);
            void show_main_page();
            void show_settings_page(lv_event_t *e);
            void alarm_ack_callback(lv_event_t *e); // For the original SEED ALARM button
            void generic_alarm_button_cb(lv_event_t *e); // For the 5 new alarm buttons
            void test_btn_cb(lv_event_t *e);

            // Add a global label for debugging
            lv_obj_t *debug_label = nullptr;

            lv_obj_t *main_scr = nullptr;
            lv_obj_t *settings_scr = nullptr;
            lv_obj_t *settings_btn = nullptr;
            lv_obj_t *backlight_slider = nullptr;
            lv_obj_t *backlight_slider_value_label = NULL;
            lv_obj_t *backlight_title_label = nullptr; // Added global for backlight title

            // lv_obj_t *alarm_btn = nullptr; // This was the old single SEED ALARM button, now managed by alarm_button_uis or a specific instance

            // Globals for the new alarm button system
            alarm_button_t alarm_button_uis[5]; // Holds UI elements for 5 buttons
            const char* alarm_names[5] = {"SEED", "FERTILIZER", "OIL PRESS", "HYD LEVEL", "SHAFT SPD"};
            const char* alarm_button_texts[5] = {"SEED\nALARM", "FERT\nALARM", "OIL\nALARM", "HYD\nALARM", "SHAFT\nALARM"};


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

            lv_obj_t *warn_label = nullptr;
            lv_timer_t *warn_flash_timer = nullptr;

            // --- Add global debug label pointers and screen ---
            lv_obj_t *debug_screen = nullptr;
            //lv_obj_t *debug_label = nullptr; // For legacy compatibility
            lv_obj_t *debug_label_1 = nullptr;
            lv_obj_t *debug_label_2 = nullptr;
            lv_obj_t *debug_label_3 = nullptr;
            lv_obj_t *debug_label_4 = nullptr;

             // --- Signal Strength Indicator Globals ---
             #define MAX_SIGNAL_BARS 5
             lv_obj_t *signal_bars[MAX_SIGNAL_BARS]; // Array to hold the bar ob
jects for settings screen
             lv_obj_t *signal_container_settings;   // Container for the signal
bars on settings screen
              // --- End Signal Strength Indicator Globals ---

            bool test_mode_toggle_state = false;
            unsigned long last_toggle_time = 0;
            const unsigned long toggle_interval = 8000;  // 8 seconds

            float currentSpeed = 0;
            float currentRate = 2;
            float totalArea = 0;
             static int current_test_signal_level = 0;

            void drill_box_anim_cb(void *obj, int32_t v) {
            lv_obj_set_style_bg_opa((lv_obj_t *)obj, v, 0);  // Animate opacity
            }

            void test_btn_cb(lv_event_t *e) {
            if (debug_label) {
                lv_label_set_text(debug_label, "Test button pressed!");
            }
            }

            void show_debug_screen();
            void show_settings_page(lv_event_t *e = nullptr);  // Accept null fo
r direct call

            void setup() {
            Serial.begin(115200);


            Serial.println("Initializing board");
            board = new Board();
            board->init();
            #if LVGL_PORT_AVOID_TEARING_MODE
            auto lcd = board->getLCD();
            lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
            #if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
            auto lcd_bus = lcd->getBus();
            if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
                static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->
getFrameWidth() * 10);
            }
            #endif
            #endif
            assert(board->begin());

            Serial.println("Initializing LVGL");
            lvgl_port_init(board->getLCD(), board->getTouch());

            Serial.println("Creating UI");
            lvgl_port_lock(-1);

            // MAIN SCREEN
            main_scr = lv_scr_act();
            lv_obj_set_style_bg_color(main_scr, lv_color_black(), 0);

            bin_left = lv_bar_create(main_scr);
            lv_obj_set_size(bin_left, 80, 390);
            lv_obj_align(bin_left, LV_ALIGN_LEFT_MID, 30, 0);
            lv_bar_set_range(bin_left, 0, 100);
            lv_bar_set_value(bin_left, 75, LV_ANIM_OFF);
            lv_obj_set_style_radius(bin_left, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bin_left, 0, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bin_left, lv_palette_lighten(LV_PALETTE_GR
EY, 4), LV_PART_MAIN);
            lv_obj_set_style_bg_color(bin_left, COLOR_YELLOW, LV_PART_INDICATOR)
;

            lbl_bin_left_name = lv_label_create(main_scr);
            lv_label_set_text(lbl_bin_left_name, "SEED");
            lv_obj_set_style_text_color(lbl_bin_left_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_left_name, bin_left, LV_ALIGN_OUT_BOTTOM_MID
, 0, 5);

            bin_right = lv_bar_create(main_scr);
            lv_obj_set_size(bin_right, 80, 390);
            lv_obj_align(bin_right, LV_ALIGN_LEFT_MID, 150, 0);
            lv_bar_set_range(bin_right, 0, 100);
            lv_bar_set_value(bin_right, 60, LV_ANIM_OFF);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bin_right, lv_palette_lighten(LV_PALETTE_G
REY, 3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(bin_right, COLOR_ORANGE, LV_PART_INDICATOR
);

            lbl_bin_right_name = lv_label_create(main_scr);
            lv_label_set_text(lbl_bin_right_name, "FERTILIZER");
            lv_obj_set_style_text_color(lbl_bin_right_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_right_name, bin_right, LV_ALIGN_OUT_BOTTOM_M
ID, 0, 5);

            int current_y_offset = 140;
            int label_spacing = 110;
            int x_offset = -20;

            panel_speed = lv_obj_create(main_scr);
            lv_obj_set_size(panel_speed, 250, 100);
            lv_obj_set_style_bg_color(panel_speed, COLOR_TEAL, LV_PART_MAIN | LV
_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_speed, LV_OPA_COVER, LV_PART_MAIN | LV
_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_speed, 5, LV_PART_MAIN | LV_STAT
E_DEFAULT);
            lv_obj_set_style_border_color(panel_speed, lv_color_white(), LV_PART
_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_speed, 6, LV_PART_MAIN | LV_STATE_DEFA
ULT);
            lv_obj_set_style_pad_all(panel_speed, 8, LV_PART_MAIN | LV_STATE_DEF
AULT);
            lv_obj_align(panel_speed, LV_ALIGN_TOP_RIGHT, x_offset, current_y_of
fset);

            speed_label = lv_label_create(panel_speed);
            lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 0.0);
            lv_obj_set_style_text_color(speed_label, COLOR_YELLOW, LV_PART_MAIN
| LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_28, LV_P
ART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(speed_label);

            current_y_offset += label_spacing;
            panel_rate = lv_obj_create(main_scr);
            lv_obj_set_size(panel_rate, 250, 100);
            lv_obj_set_style_bg_color(panel_rate, COLOR_TEAL, LV_PART_MAIN | LV_
STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_rate, LV_OPA_COVER, LV_PART_MAIN | LV_
STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_rate, 5, LV_PART_MAIN | LV_STATE
_DEFAULT);
            lv_obj_set_style_border_color(panel_rate, lv_color_white(), LV_PART_
MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_rate, 6, LV_PART_MAIN | LV_STATE_DEFAU
LT);
            lv_obj_set_style_pad_all(panel_rate, 8, LV_PART_MAIN | LV_STATE_DEFA
ULT);
            lv_obj_align(panel_rate, LV_ALIGN_TOP_RIGHT, x_offset, current_y_off
set);

            rate_label = lv_label_create(panel_rate);
            lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 0.0);
            lv_obj_set_style_text_color(rate_label, COLOR_YELLOW, LV_PART_MAIN |
 LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(rate_label, &lv_font_montserrat_30, LV_PA
RT_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(rate_label);

            current_y_offset += label_spacing;
            panel_area = lv_obj_create(main_scr);
            lv_obj_set_size(panel_area, 250, 100);
            lv_obj_set_style_bg_color(panel_area, COLOR_TEAL, LV_PART_MAIN | LV_
STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_area, LV_OPA_COVER, LV_PART_MAIN | LV_
STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_area, 5, LV_PART_MAIN | LV_STATE
_DEFAULT);
            lv_obj_set_style_border_color(panel_area, lv_color_white(), LV_PART_
MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_area, 6, LV_PART_MAIN | LV_STATE_DEFAU
LT);
            lv_obj_set_style_pad_all(panel_area, 8, LV_PART_MAIN | LV_STATE_DEFA
ULT);
            lv_obj_align(panel_area, LV_ALIGN_TOP_RIGHT, x_offset, current_y_off
set);

            area_label = lv_label_create(panel_area);
            lv_label_set_text_fmt(area_label, "Area: %.1f ha", 0.0);
            lv_obj_set_style_text_color(area_label, COLOR_YELLOW, LV_PART_MAIN |
 LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(area_label, &lv_font_montserrat_32, LV_PA
RT_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(area_label);

            drill_box = lv_obj_create(main_scr);
            lv_obj_set_size(drill_box, 180, 110);
            lv_obj_set_style_pad_all(drill_box, 9, 0);
            lv_obj_set_style_border_width(drill_box, 3, 0);
            lv_obj_set_style_border_color(drill_box, lv_color_white(), 0);
            lv_obj_set_style_border_opa(drill_box, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_opa(drill_box, LV_OPA_COVER, 0);
            lv_obj_align(drill_box, LV_ALIGN_CENTER, -20, 140);

            drill_label = lv_label_create(drill_box);
            lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_26, LV_P
ART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(drill_label, "DRILL\nDOWN");
            lv_obj_set_style_text_align(drill_label, LV_TEXT_ALIGN_CENTER, LV_PA
RT_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(drill_label);

            // Spinner
            spinner = lv_spinner_create(main_scr, 750, 40);
            lv_obj_set_size(spinner, 135, 135);
            lv_obj_align(spinner, LV_ALIGN_CENTER, -20, -160);
            lv_obj_set_style_arc_color(spinner, lv_color_make(0x80, 0x00, 0x80),
 LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, 20, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, 19, LV_PART_MAIN);

            lv_obj_t *spinner_label = lv_label_create(spinner);
            lv_label_set_text(spinner_label, "CLUTCH\nENGAGED");
            lv_obj_set_style_text_align(spinner_label, LV_TEXT_ALIGN_CENTER, LV_
PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(spinner_label);
            lv_obj_set_style_text_color(spinner_label, lv_color_make(0xC0, 0xC0,
 0xC0), 0);

            // --- Settings Button ---
            settings_btn = lv_btn_create(main_scr);
            lv_obj_set_width(settings_btn, 120);
            lv_obj_set_height(settings_btn, 90);
            lv_obj_set_style_pad_all(settings_btn, 10, 0);
            lv_obj_set_style_border_width(settings_btn, 5, 0);
            lv_obj_set_style_border_color(settings_btn, lv_color_white(), 0);
            lv_obj_set_style_border_opa(settings_btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(settings_btn, lv_palette_main(LV_PALETTE_Y
ELLOW), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(settings_btn, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(settings_btn, LV_OPA_COVER, 0);
            lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -70, 20);

            lv_obj_t *settings_label_btn = lv_label_create(settings_btn); // Renamed to avoid conflict
            lv_label_set_text(settings_label_btn, "SETTINGS");
            lv_obj_set_style_text_align(settings_label_btn, LV_TEXT_ALIGN_CENTER, LV
_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(settings_label_btn, &lv_font_montserrat_18, L
V_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(settings_label_btn, LV_ALIGN_CENTER, 0, 0);  // Center labe
l inside button
            lv_obj_add_event_cb(settings_btn, show_settings_page, LV_EVENT_CLICK
ED, NULL);



            lvgl_port_unlock();

            start_warning_flash();

            Serial.println("setup() complete");
            }

            void loop() {
            lv_timer_handler();
            delay(5);

            dtostrf(currentSpeed, 4, 1, speedBuf);
            dtostrf(currentRate, 4, 1, rateBuf);
            dtostrf(totalArea, 5, 1, areaBuf);

            char speedLabel[32], rateLabel[32], areaLabel[32];
            snprintf(speedLabel, sizeof(speedLabel), "Speed: %s km/h", speedBuf)
;
            snprintf(rateLabel, sizeof(rateLabel), "Rate: %s ha/h", rateBuf);
            snprintf(areaLabel, sizeof(areaLabel), "Area: %s ha", areaBuf);

            unsigned long current_time = millis();
            if (current_time - last_toggle_time >= toggle_interval) {
                last_toggle_time = current_time;
                test_mode_toggle_state = !test_mode_toggle_state;

                lvgl_port_lock(-1);

                if (test_mode_toggle_state) {
                currentSpeed = 8.7;
                currentRate = 25.5;
                totalArea = 85.2;
                if (bin_left) lv_bar_set_value(bin_left, 100, LV_ANIM_OFF);
                if (bin_right) lv_bar_set_value(bin_right, 100, LV_ANIM_OFF);
                if (speed_label) lv_label_set_text(speed_label, speedLabel);
                if (rate_label) lv_label_set_text(rate_label, rateLabel);
                if (area_label) lv_label_set_text(area_label, areaLabel);
                if (drill_box && drill_label) {
                    lv_obj_set_style_bg_color(drill_box, COLOR_GREEN, 0);
                    lv_label_set_text(drill_label, "DRILL\nDOWN");
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_
26, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0
);
                    lv_anim_t a;
                    lv_anim_init(&a);
                    lv_anim_set_var(&a, drill_box);
                    lv_anim_set_exec_cb(&a, drill_box_anim_cb);
                    lv_anim_set_time(&a, 2000);
                    lv_anim_set_values(&a, LV_OPA_40, LV_OPA_COVER);
                    lv_anim_set_playback_time(&a, 1000);
                    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
                    lv_anim_start(&a);
                }
                } else {
                currentSpeed = 11.3;
                currentRate = 2.85;
                totalArea = 15.2;
                if (bin_left) lv_bar_set_value(bin_left, 25, LV_ANIM_OFF);
                if (bin_right) lv_bar_set_value(bin_right, 15, LV_ANIM_OFF);
                if (speed_label) lv_label_set_text(speed_label, speedLabel);
                if (rate_label) lv_label_set_text(rate_label, rateLabel);
                if (area_label) lv_label_set_text(area_label, areaLabel);
                if (drill_box && drill_label) {
                    lv_obj_set_style_bg_color(drill_box, lv_palette_main(LV_PALE
TTE_RED), 0);
                    lv_label_set_text(drill_label, "DRILL\nUP");
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_
30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0
);
                    lv_anim_del(drill_box, drill_box_anim_cb);
                    lv_obj_set_style_bg_opa(drill_box, LV_OPA_COVER, 0);
                }
                }
                 // --- Update Signal Strength Display ---
        // We'll make the signal strength cycle each time this block runs.
        // MAX_SIGNAL_BARS is 5, so levels are 0, 1, 2, 3, 4, 5.
        // (MAX_SIGNAL_BARS + 1) gives the number of states (0 to 5 inclusive is
 6 states).

        if (lv_scr_act() == settings_scr) { // Only update if settings screen is
 active
            // To prevent calling update_signal_strength_display_settings with a NULL signal_container_settings
            // before settings_scr is fully initialized, ensure signal_container_settings is not NULL.
            if (signal_container_settings) {
                 update_signal_strength_display_settings(current_test_signal_level);
                 Serial.printf("Loop Test: Setting signal strength to %d\n", current_test_signal_level);
            }
        }

        current_test_signal_level++;
        if (current_test_signal_level > MAX_SIGNAL_BARS) { // If it goes beyond
5 (0-5 are valid levels)
            current_test_signal_level = 0; // Reset to 0
        }
        // --- End Update Signal Strength Display ---
                lvgl_port_unlock();

            }
            }

// Generic callback for the new alarm buttons
void generic_alarm_button_cb(lv_event_t *e) {
    alarm_button_t *alarm_ui = (alarm_button_t *)lv_event_get_user_data(e);
    if (alarm_ui) {
        Serial.printf("Alarm button '%s' clicked!\n", alarm_ui->name);
        // Example: Change appearance to an acknowledged state
        lv_obj_set_style_bg_color(alarm_ui->btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text_fmt(alarm_ui->label, "%s\nACK'D", alarm_ui->name);
        // stop_warning_flash(); // Or a more specific action based on the alarm
    }
}

            void alarm_ack_callback(lv_event_t *e) { // Retained for the original SEED alarm button if used directly
            lv_obj_t *btn = lv_event_get_target(e);
            lv_obj_t *label = lv_obj_get_child(btn, 0);

            lv_label_set_text(label, "SEED\nACKNOWLEDGED"); // Specific to SEED
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), 0)
;
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_26, LV_PART_MAIN
 | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN
| LV_STATE_DEFAULT);
            // lv_obj_set_style_text_color(btn, lv_color_black(), 0); // Already black from creation
            lv_obj_set_style_text_color(btn, lv_color_white(), 0); // Changed to white for acknowledged state


            stop_warning_flash();
            }

            void flash_warn_label_cb(lv_timer_t *timer) {
            lv_obj_t *label = (lv_obj_t *)timer->user_data;
            if (lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
            }
            }

            void start_warning_flash() {
            if (!warn_label) {
                warn_label = lv_label_create(lv_scr_act());
                lv_label_set_text(warn_label, "WARNING!");
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALET
TE_RED), 0);
                lv_obj_set_style_text_font(warn_label, &lv_font_montserrat_42, 0
);
                lv_obj_align(warn_label, LV_ALIGN_CENTER, -20, -20);
            } else {
                lv_label_set_text(warn_label, "WARNING!");
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
            }
            if (!warn_flash_timer) {
                warn_flash_timer = lv_timer_create(flash_warn_label_cb, 500, war
n_label);
            }
            }

            void stop_warning_flash() {
            if (warn_flash_timer) {
                lv_timer_del(warn_flash_timer);
                warn_flash_timer = nullptr;
            }
            if (warn_label) {
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALET
TE_GREEN), 0);
                lv_label_set_text(warn_label, "SAFE");
            }
            }

            void show_main_page() {
            if (main_scr) {
                lv_scr_load(main_scr);
            }
            }


// Function 1: backlight_slider_event_cb (Modified with lv_obj_align_to)
void backlight_slider_event_cb(lv_event_t *e) {
    // Serial.println("DEBUG: backlight_slider_event_cb CALLED!"); // Keep for debugging if needed

    lv_obj_t *slider = lv_event_get_target(e);
    if (!slider) {
        // Serial.println("DEBUG: Slider event CB - slider is NULL!");
        return;
    }

    int percent_val = lv_slider_get_value(slider);
    int display_percent = percent_val;

    int pwm_percent = percent_val;
    if (pwm_percent < 5) pwm_percent = 5; // Ensure minimum 5% for PWM

    int pwmValue = map(pwm_percent, 0, 100, 0, 80); // Map 0-100% to 0-80 PWM for backlight
    if (board) {
        auto backlight = board->getBacklight();
        if (backlight) {
            backlight->setBrightness(pwmValue);
        }
    }

    if (backlight_slider_value_label) {
        static char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", display_percent);
        lv_label_set_text(backlight_slider_value_label, buf);

        // Only try to align if the slider and label are on the same screen and part of the same layout context
        if (lv_obj_get_screen(slider) == lv_obj_get_screen(backlight_slider_value_label)) {
            lv_coord_t slider_height = lv_obj_get_height(slider);
            lv_coord_t label_height = lv_obj_get_height(backlight_slider_value_label);
            lv_coord_t y_offset_on_slider = (lv_coord_t)(((100.0f - display_percent) / 100.0f) * slider_height);
            lv_coord_t final_y_offset_for_align = y_offset_on_slider - (label_height / 2);

            if (final_y_offset_for_align < 0) final_y_offset_for_align = 0;
            if (final_y_offset_for_align + label_height > slider_height) final_y_offset_for_align = slider_height - label_height;

            // Align the label relative to the slider.
            // The X offset (40) places it to the right. The Y offset aligns it vertically with the knob.
            lv_obj_align_to(backlight_slider_value_label, slider, LV_ALIGN_OUT_RIGHT_TOP, 30, final_y_offset_for_align);
        }
        // Serial.printf("DEBUG: Slider Event: percent=%d, using lv_obj_align_to with x_offset=30, y_offset=%d\n",display_percent, (int)final_y_offset_for_align);
    } else {
        // Serial.println("DEBUG: Slider event CB - backlight_slider_value_label is NULL!");
    }
}

   // Function 2: show_settings_page
void show_settings_page(lv_event_t *e) {
    if (!settings_scr) {
        settings_scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(settings_scr, lv_color_black(), 0);

        // --- Create a Tabview ---
        lv_obj_t *tabview = lv_tabview_create(settings_scr, LV_DIR_TOP, 50);
        lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));

        lv_obj_t *tab_alarms = lv_tabview_add_tab(tabview, "Alarms");
        lv_obj_t *tab_general = lv_tabview_add_tab(tabview, "General");

        // --- Populate "Alarms" Tab ---
        lv_obj_set_flex_flow(tab_alarms, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(tab_alarms, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(tab_alarms, 15, 0);
        lv_obj_set_style_pad_gap(tab_alarms, 15, 0);

        // Create the 5 Alarm Buttons
        for (int i = 0; i < 5; i++) {
            create_alarm_button(tab_alarms, &alarm_button_uis[i], alarm_button_texts[i], alarm_names[i], generic_alarm_button_cb, &alarm_button_uis[i]);
            if (strcmp(alarm_names[i], "SEED") == 0) {
                 lv_obj_set_style_bg_color(alarm_button_uis[i].btn, CUSTOM_COLOR_2, 0); // SEED alarm specific color
                 // The original code had an alarm_btn for SEED alarm with alarm_ack_callback.
                 // We'll use generic_alarm_button_cb for all, and it can be customized.
                 // If specific behavior for SEED ACK is still needed beyond generic, it can be added in generic_alarm_button_cb by checking name.
            }
        }

        // --- Populate "General" Tab ---
        // This tab will hold backlight and signal strength. Using flex for layout.
        lv_obj_set_flex_flow(tab_general, LV_FLEX_FLOW_COLUMN); // Main flow is column
        lv_obj_set_flex_align(tab_general, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER); // Center items horizontally
        lv_obj_set_style_pad_all(tab_general, 10, 0);
        lv_obj_set_style_pad_gap(tab_general, 20, 0);


        // Container for Backlight controls (Title, Slider, Value)
        // This container helps group them and allows the slider's value label to be aligned relative to the slider itself.
        lv_obj_t* backlight_controls_container = lv_obj_create(tab_general);
        lv_obj_remove_style_all(backlight_controls_container); // Remove default container padding/styles
        lv_obj_set_width(backlight_controls_container, lv_pct(80)); // Take up significant width
        lv_obj_set_height(backlight_controls_container, LV_SIZE_CONTENT); // Adjust height to content
        lv_obj_set_style_pad_bottom(backlight_controls_container, 20, 0); // Add some padding at the bottom of this group
        // No flex on this container itself, its children will be manually positioned or use their own layouts.
        // Alignment of this container within tab_general is handled by tab_general's flex settings.

        backlight_title_label = lv_label_create(backlight_controls_container);
        lv_label_set_text(backlight_title_label, "DISPLAY BRIGHTNESS");
        lv_obj_set_style_text_font(backlight_title_label, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(backlight_title_label, lv_color_white(), 0);
        lv_obj_align(backlight_title_label, LV_ALIGN_TOP_CENTER, 0, 5); // Centered at the top of its container

        backlight_slider = lv_slider_create(backlight_controls_container);
        lv_slider_set_range(backlight_slider, 0, 100);
        lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
        lv_obj_set_size(backlight_slider, 60, 250); // Width, Height
        lv_obj_align(backlight_slider, LV_ALIGN_CENTER, -30, 40); // Align slider below title, slightly left to make space for label

        // Styles from fetched code
        lv_obj_set_style_bg_color(backlight_slider, lv_palette_lighten(LV_PALETTE_GREY, 1), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(backlight_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(backlight_slider, lv_color_make(0x00, 0x00, 0x80), LV_PART_KNOB);
        lv_obj_set_style_border_color(backlight_slider, lv_palette_darken(LV_PALETTE_BLUE, 9), LV_PART_KNOB);
        lv_obj_set_style_border_width(backlight_slider, 5, LV_PART_KNOB);
        lv_obj_set_style_shadow_width(backlight_slider, 8, LV_PART_KNOB);
        lv_obj_set_style_shadow_opa(backlight_slider, LV_OPA_50, LV_PART_KNOB);

        backlight_slider_value_label = lv_label_create(backlight_controls_container); // Parent to the same container
        lv_obj_set_style_text_color(backlight_slider_value_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(backlight_slider_value_label, &lv_font_montserrat_24, 0);
        // Initial text & position will be set by the event callback

        lv_obj_add_event_cb(backlight_slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_event_send(backlight_slider, LV_EVENT_VALUE_CHANGED, NULL); // Trigger initial positioning


        // Signal Strength Indicator (Moved to General Tab)
        // Create a container for the signal strength to group its label and bars
        lv_obj_t* signal_group_container = lv_obj_create(tab_general);
        lv_obj_remove_style_all(signal_group_container);
        lv_obj_set_width(signal_group_container, LV_SIZE_CONTENT);
        lv_obj_set_height(signal_group_container, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(signal_group_container, LV_FLEX_FLOW_COLUMN); // Stack title and bars vertically
        lv_obj_set_flex_align(signal_group_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(signal_group_container, 5, 0);
        // Alignment of signal_group_container within tab_general is handled by tab_general's flex settings.

        lv_obj_t *signal_title_label = lv_label_create(signal_group_container);
        lv_label_set_text(signal_title_label, "ESP-NOW Signal");
        lv_obj_set_style_text_color(signal_title_label, lv_palette_main(LV_PALETTE_YELLOW), 0);
        lv_obj_set_style_text_font(signal_title_label, &lv_font_montserrat_18, 0);

        signal_container_settings = lv_obj_create(signal_group_container); // This is the flex container for the bars themselves
        lv_obj_remove_style_all(signal_container_settings);
        lv_obj_set_size(signal_container_settings, LV_SIZE_CONTENT, LV_SIZE_CONTENT); // Adjust to content
        lv_obj_set_flex_flow(signal_container_settings, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(signal_container_settings, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER); // Align bars at their bottom
        lv_obj_set_style_pad_gap(signal_container_settings, 3, 0); // Reduced gap between bars

        lv_coord_t bar_width = 10;
        lv_coord_t base_bar_height = 15;
        lv_coord_t height_increment = 5;

        for (int i = 0; i < MAX_SIGNAL_BARS; i++) {
            signal_bars[i] = lv_obj_create(signal_container_settings);
            lv_obj_remove_style_all(signal_bars[i]);
            lv_obj_set_size(signal_bars[i], bar_width, base_bar_height + (i * height_increment));
            lv_obj_set_style_bg_color(signal_bars[i], lv_color_hex(0x666666), 0); // Default inactive color
            lv_obj_set_style_bg_opa(signal_bars[i], LV_OPA_COVER, 0);
            lv_obj_set_style_radius(signal_bars[i], 2, 0);
        }
        // Initial update of signal strength display might be needed if current_test_signal_level has a non-zero default
        update_signal_strength_display_settings(current_test_signal_level);


        // --- Back Button (Outside Tabview, on settings_scr itself) ---
        lv_obj_t *back_btn = lv_btn_create(settings_scr); // Parent is settings_scr
        lv_obj_set_size(back_btn, 120, 60); // Adjusted size
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -15); // Positioned at bottom-left
        lv_obj_t *back_button_label = lv_label_create(back_btn);
        lv_label_set_text(back_button_label, "BACK");
        lv_obj_set_style_text_font(back_button_label, &lv_font_montserrat_20, 0);
        lv_obj_center(back_button_label);
        lv_obj_add_event_cb(back_btn, [](lv_event_t * evt) {
            show_main_page();
        }, LV_EVENT_CLICKED, NULL);
    }
    lv_scr_load(settings_scr);
}



// --- Function to Update Signal Strength Display on Settings Screen ---
void update_signal_strength_display_settings(int level) { // level = 0 (no signa
l) to 5 (full signal)
    // Ensure level is within bounds
    if (level < 0) level = 0;
    if (level > MAX_SIGNAL_BARS) level = MAX_SIGNAL_BARS;

    // Serial.printf("Updating signal display to level: %d", level); // Optional
: for debugging

    for (int i = 0; i < MAX_SIGNAL_BARS; i++) {
        if (signal_bars[i]) { // Check if bar object exists
            if (i < level) {
                // Active bar color (e.g., white or a shade of green)
                lv_obj_set_style_bg_color(signal_bars[i],lv_palette_main(LV_PALE
TTE_YELLOW), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                // Inactive bar color (e.g., a darker grey)
                lv_obj_set_style_bg_color(signal_bars[i], lv_color_hex(0x666666)
, 0);
            }
        }
    }
}
// --- End Update Signal Strength Function ---