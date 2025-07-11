            /**
            * display for agro drill lcd using waveshare SKU 27078 7" inch esp lcd
            * 
            * Version with "Push to Clear Alarms" button and Backlight slider/label moved to the Settings screen.
            */

            #define LV_CONF_INCLUDE_SIMPLE  // place before lvgl.h
            #include <Arduino.h>
            #include <esp_display_panel.hpp>
            #include <lvgl.h>
            #include "lvgl_v8_port.h"
            #include "color.h"
            #include "esp_panel_board_custom_conf.h"

            using namespace esp_panel::drivers;
            using namespace esp_panel::board;

            #if LVGL_PORT_AVOID_TEARING_MODE
            #error "This example does not support the avoid tearing function. Please set `LVGL_PORT_AVOID_TEARING_MODE` to `0` in the `lvgl_v8_port.h` file."
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
            void alarm_ack_callback(lv_event_t *e);
            void test_btn_cb(lv_event_t *e);

            // Add a global label for debugging
            lv_obj_t *debug_label = nullptr;

            lv_obj_t *main_scr = nullptr;
            lv_obj_t *settings_scr = nullptr;
            lv_obj_t *settings_btn = nullptr;
            lv_obj_t *backlight_slider = nullptr;
            lv_obj_t *backlight_slider_value_label = NULL;

            lv_obj_t *alarm_btn = nullptr;

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

            bool test_mode_toggle_state = false;
            unsigned long last_toggle_time = 0;
            const unsigned long toggle_interval = 8000;  // 8 seconds

            float currentSpeed = 0;
            float currentRate = 2;
            float totalArea = 0;

            void drill_box_anim_cb(void *obj, int32_t v) {
            lv_obj_set_style_bg_opa((lv_obj_t *)obj, v, 0);  // Animate opacity
            }

            void test_btn_cb(lv_event_t *e) {
            if (debug_label) {
                lv_label_set_text(debug_label, "Test button pressed!");
            }
            }

            void show_debug_screen();
            void show_settings_page(lv_event_t *e = nullptr);  // Accept null for direct call

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
                static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
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
            lv_obj_set_style_bg_color(bin_left, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);
            lv_obj_set_style_bg_color(bin_left, COLOR_YELLOW, LV_PART_INDICATOR);

            lbl_bin_left_name = lv_label_create(main_scr);
            lv_label_set_text(lbl_bin_left_name, "SEED");
            lv_obj_set_style_text_color(lbl_bin_left_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_left_name, bin_left, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

            bin_right = lv_bar_create(main_scr);
            lv_obj_set_size(bin_right, 80, 390);
            lv_obj_align(bin_right, LV_ALIGN_LEFT_MID, 150, 0);
            lv_bar_set_range(bin_right, 0, 100);
            lv_bar_set_value(bin_right, 60, LV_ANIM_OFF);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bin_right, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(bin_right, COLOR_ORANGE, LV_PART_INDICATOR);

            lbl_bin_right_name = lv_label_create(main_scr);
            lv_label_set_text(lbl_bin_right_name, "FERTILIZER");
            lv_obj_set_style_text_color(lbl_bin_right_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_right_name, bin_right, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

            int current_y_offset = 140;
            int label_spacing = 110;
            int x_offset = -20;

            panel_speed = lv_obj_create(main_scr);
            lv_obj_set_size(panel_speed, 250, 100);
            lv_obj_set_style_bg_color(panel_speed, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_speed, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_speed, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_speed, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_speed, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_speed, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(panel_speed, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);

            speed_label = lv_label_create(panel_speed);
            lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 0.0);
            lv_obj_set_style_text_color(speed_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(speed_label);

            current_y_offset += label_spacing;
            panel_rate = lv_obj_create(main_scr);
            lv_obj_set_size(panel_rate, 250, 100);
            lv_obj_set_style_bg_color(panel_rate, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_rate, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_rate, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_rate, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_rate, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_rate, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(panel_rate, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);

            rate_label = lv_label_create(panel_rate);
            lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 0.0);
            lv_obj_set_style_text_color(rate_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(rate_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(rate_label);

            current_y_offset += label_spacing;
            panel_area = lv_obj_create(main_scr);
            lv_obj_set_size(panel_area, 250, 100);
            lv_obj_set_style_bg_color(panel_area, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_area, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_area, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_area, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_area, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_area, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(panel_area, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);

            area_label = lv_label_create(panel_area);
            lv_label_set_text_fmt(area_label, "Area: %.1f ha", 0.0);
            lv_obj_set_style_text_color(area_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(area_label, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(drill_label, "DRILL\nDOWN");
            lv_obj_set_style_text_align(drill_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(drill_label);

            // Spinner
            spinner = lv_spinner_create(main_scr, 750, 40);
            lv_obj_set_size(spinner, 135, 135);
            lv_obj_align(spinner, LV_ALIGN_CENTER, -20, -160);
            lv_obj_set_style_arc_color(spinner, lv_color_make(0x80, 0x00, 0x80), LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, 20, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, 19, LV_PART_MAIN);

            lv_obj_t *spinner_label = lv_label_create(spinner);
            lv_label_set_text(spinner_label, "CLUTCH\nENGAGED");
            lv_obj_set_style_text_align(spinner_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(spinner_label);
            lv_obj_set_style_text_color(spinner_label, lv_color_make(0xC0, 0xC0, 0xC0), 0);

            // --- Settings Button ---
            settings_btn = lv_btn_create(main_scr);
            lv_obj_set_width(settings_btn, 120);
            lv_obj_set_height(settings_btn, 90);
            lv_obj_set_style_pad_all(settings_btn, 10, 0);
            lv_obj_set_style_border_width(settings_btn, 5, 0);
            lv_obj_set_style_border_color(settings_btn, lv_color_white(), 0);
            lv_obj_set_style_border_opa(settings_btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(settings_btn, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(settings_btn, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(settings_btn, LV_OPA_COVER, 0);
            lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -70, 20);

            lv_obj_t *settings_label = lv_label_create(settings_btn);
            lv_label_set_text(settings_label, "SETTINGS");
            lv_obj_set_style_text_align(settings_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(settings_label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(settings_label, LV_ALIGN_CENTER, 0, 0);  // Center label inside button
            lv_obj_add_event_cb(settings_btn, show_settings_page, LV_EVENT_CLICKED, NULL);



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
            snprintf(speedLabel, sizeof(speedLabel), "Speed: %s km/h", speedBuf);
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
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0);
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
                    lv_obj_set_style_bg_color(drill_box, lv_palette_main(LV_PALETTE_RED), 0);
                    lv_label_set_text(drill_label, "DRILL\nUP");
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0);
                    lv_anim_del(drill_box, drill_box_anim_cb);
                    lv_obj_set_style_bg_opa(drill_box, LV_OPA_COVER, 0);
                }
                }
                lvgl_port_unlock();
            }
            }

            void alarm_ack_callback(lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            lv_obj_t *label = lv_obj_get_child(btn, 0);

            lv_label_set_text(label, "ALARM\nACKNOWLEDGED");
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), 0);
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btn, lv_color_white(), 0);

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
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALETTE_RED), 0);
                lv_obj_set_style_text_font(warn_label, &lv_font_montserrat_42, 0);
                lv_obj_align(warn_label, LV_ALIGN_CENTER, -20, -20);
            } else {
                lv_label_set_text(warn_label, "WARNING!");
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
            }
            if (!warn_flash_timer) {
                warn_flash_timer = lv_timer_create(flash_warn_label_cb, 500, warn_label);
            }
            }

            void stop_warning_flash() {
            if (warn_flash_timer) {
                lv_timer_del(warn_flash_timer);
                warn_flash_timer = nullptr;
            }
            if (warn_label) {
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALETTE_GREEN), 0);
                lv_label_set_text(warn_label, "SAFE");
            }
            }

            void show_main_page() {
            if (main_scr) {
                lv_scr_load(main_scr);
            }
            }


           void backlight_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int percent = lv_slider_get_value(slider);
    if (percent < 5) percent = 5;

    // Set backlight brightness (adjust if needed)
    int pwmValue = map(percent, 0, 100, 0, 80);
    if (board) {
        auto backlight = board->getBacklight();
        if (backlight) {
            backlight->setBrightness(pwmValue);
        }
    }
    //backlight->setBrightness(pwmValue);

    // Update value label text
    lv_label_set_text_fmt(backlight_slider_value_label, "%d", percent);

    // Track knob position (estimate)
    int slider_height = lv_obj_get_height(slider);
    lv_coord_t knob_y = lv_obj_get_y(slider) + ((100 - percent) * slider_height) / 100;
    lv_coord_t label_x = lv_obj_get_x(slider) + lv_obj_get_width(slider) + 10;
    lv_obj_set_pos(backlight_slider_value_label, label_x, knob_y);


    // Update live value label
    if (backlight_slider_value_label) {
        static char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", percent);
        lv_label_set_text(backlight_slider_value_label, buf);
    }
}


            // Modify show_settings_page to add button for debug screen
            void show_settings_page(lv_event_t *e) {
            if (!settings_scr) {
                settings_scr = lv_obj_create(NULL);
                lv_obj_t *label = lv_label_create(settings_scr);
                lv_obj_set_style_bg_color(settings_scr, lv_color_black(), 0);
                lv_obj_set_style_text_font(label, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(label, "Settings Page");
                lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40);
                lv_obj_set_style_text_color(label, lv_color_white(), 0);
                // "Push to Clear Alarm" button (existing)
                alarm_btn = lv_btn_create(settings_scr);
                lv_obj_set_width(alarm_btn, 120);
                lv_obj_set_height(alarm_btn, 100);
                lv_obj_set_style_pad_all(alarm_btn, 10, 0);
                lv_obj_set_style_border_width(alarm_btn, 5, 0);
                lv_obj_set_style_border_color(alarm_btn, lv_color_white(), 0);
                lv_obj_set_style_border_opa(alarm_btn, LV_OPA_COVER, 0);
                lv_obj_set_style_bg_color(alarm_btn, CUSTOM_COLOR_2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(alarm_btn, lv_color_black(), 0);
                lv_obj_set_style_bg_opa(alarm_btn, LV_OPA_COVER, 0);
                lv_obj_align(alarm_btn, LV_ALIGN_TOP_MID, 140, -120);

                lv_obj_t *alarm_label = lv_label_create(alarm_btn);
                lv_label_set_text(alarm_label, "PUSH TO\nCLEAR ALARM");
                lv_obj_set_style_text_align(alarm_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(alarm_label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_align(alarm_label, LV_ALIGN_CENTER, 0, 0);
                lv_obj_add_event_cb(alarm_btn, alarm_ack_callback, LV_EVENT_CLICKED, NULL);



                // Create the slider
                lv_obj_t *settings_scr = lv_obj_create(NULL);
    lv_scr_load(settings_scr);

    // Create label: "Backlight"
    lv_obj_t *slider_label = lv_label_create(settings_scr);
    lv_label_set_text(slider_label, "Backlight");
    lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);
    lv_obj_align(slider_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create backlight slider (vertical)
    backlight_slider = lv_slider_create(settings_scr);
    lv_slider_set_range(backlight_slider, 0, 100);
    lv_slider_set_value(backlight_slider, 100, LV_ANIM_OFF);
    lv_obj_set_size(backlight_slider, 30, 200); // narrow and tall
    lv_obj_align(backlight_slider, LV_ALIGN_BOTTOM_MID, -280, -120);

    // Live value label next to knob
    backlight_slider_value_label = lv_label_create(settings_scr);
    lv_label_set_text(backlight_slider_value_label, "100");
    lv_obj_set_style_text_color(backlight_slider_value_label, lv_color_white(), 0);

    // Attach slider event
    lv_obj_add_event_cb(backlight_slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Trigger callback manually to place label initially
    //backlight_slider_event_cb(lv_event_create(LV_EVENT_VALUE_CHANGED, backlight_slider));
    backlight_slider_event_cb(nullptr);
                /*
                    // Button to show Debug Screen
                    lv_obj_t *debug_btn = lv_btn_create(settings_scr);
                    lv_obj_set_width(debug_btn, 160);
                    lv_obj_set_height(debug_btn, 60);
                    lv_obj_align(debug_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
                    lv_obj_t *debug_btn_label = lv_label_create(debug_btn);
                    lv_label_set_text(debug_btn_label, "SHOW DEBUG");
                    lv_obj_center(debug_btn_label);
                    lv_obj_add_event_cb(debug_btn, [](lv_event_t *e) {
                        show_debug_screen();
                    }, LV_EVENT_CLICKED, NULL);
            */
                // Back button to return to main screen
                lv_obj_t *back_btn = lv_btn_create(settings_scr);
                lv_obj_set_width(back_btn, 120);
                lv_obj_set_height(back_btn, 60);
                lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
                lv_obj_t *back_label = lv_label_create(back_btn);
                lv_label_set_text(back_label, "BACK\nTO MAIN");
                lv_obj_center(back_label);
                lv_obj_add_event_cb(
                back_btn, [](lv_event_t *e) {
                    show_main_page();
                },
                LV_EVENT_CLICKED, NULL);

                // Optionally, assign debug_label for backward compatibility
                debug_label = debug_label_1;
            }
            lv_scr_load(settings_scr);
            }