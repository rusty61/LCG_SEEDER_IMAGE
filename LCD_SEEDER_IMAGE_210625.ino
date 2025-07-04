            /**
            * display for agro drill lcd using waveshare SKU 27078 7" inch esp lcd
            */
            #define LV_CONF_INCLUDE_SIMPLE  // place befor lvgl.h
            #include <Arduino.h>
            #include <esp_display_panel.hpp>
            #include <lvgl.h>
            #include "lvgl_v8_port.h"
            #include "color.h"


            using namespace esp_panel::drivers;
            using namespace esp_panel::board;

            #if LVGL_PORT_AVOID_TEARING_MODE
            #error "This example does not support the avoid tearing function. Please set `LVGL_PORT_AVOID_TEARING_MODE` to `0` in the `lvgl_v8_port.h` file."
            #endif



            // Buffers to hold formatted strings
            char speedBuf[16], rateBuf[16], areaBuf[16];

            

            /* Bin level bars */
            static lv_obj_t *bin_left = NULL;
            static lv_obj_t *lbl_bin_left_name = NULL;
            static lv_obj_t *bin_right = NULL;
            static lv_obj_t *lbl_bin_right_name = NULL;


            /* Data labels */
            static lv_obj_t *speed_label = NULL;
            static lv_obj_t *rate_label = NULL;
            static lv_obj_t *area_label = NULL;

            /* Alarm and status */
            lv_obj_t *alarm_btn;
            static lv_obj_t *alarm_box = NULL;
            /*  dril postion wiget  */
            static lv_obj_t *drill_box = NULL;
            static lv_obj_t *drill_label = NULL;
            void drill_box_anim_cb(void *obj, int32_t v) {
            lv_obj_set_style_bg_opa((lv_obj_t *)obj, v, 0);  // Animate opacity
            }

            /* Spinner arc */
           /* Spinner arc */
            static lv_obj_t *spinner;
            static lv_style_t style_arc;
            static lv_anim_t anim;

            static lv_obj_t *panel_speed = NULL;
            static lv_obj_t *panel_rate = NULL;
            static lv_obj_t *panel_area = NULL;

            lv_obj_t *warn_label = NULL;
            lv_timer_t *warn_flash_timer = NULL;


            static bool test_mode_toggle_state = false;  // General toggle for values
            static unsigned long last_toggle_time = 0;
            const unsigned long toggle_interval = 8000;  // 8 seconds

                float currentSpeed = 0;
                float currentRate = 2;
                float totalArea = 0;

            void setup() {
            Serial.begin(115200);

            Serial.println("Initializing board");
            Board *board = new Board();
            board->init();
            #if LVGL_PORT_AVOID_TEARING_MODE
            auto lcd = board->getLCD();
            // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
            lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
            #if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
            auto lcd_bus = lcd->getBus();
            /**
                * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
                * "bounce buffer" functionality to enhance the RGB data bandwidth.
                * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
                */
            if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
                static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
            }
            #endif
            #endif
            assert(board->begin());

            Serial.println("Initializing LVGL");
            lvgl_port_init(board->getLCD(), board->getTouch());

            Serial.println("Creating UI");
            /* Lock the mutex due to the LVGL APIs are not thread-safe */
            lvgl_port_lock(-1);


            lv_obj_t *scr = lv_scr_act();
            lv_obj_set_style_bg_color(scr, lv_color_black(), 0);



            // Bin level bars
            //bin_left = lv_bar_create(scr); // Declaration moved to global
            bin_left = lv_bar_create(scr);
            lv_obj_set_size(bin_left, 80, 390);
            lv_obj_align(bin_left, LV_ALIGN_LEFT_MID, 30, 0);
            lv_bar_set_range(bin_left, 0, 100);
            lv_bar_set_value(bin_left, 75, LV_ANIM_OFF);
            lv_obj_set_style_radius(bin_left, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bin_left, 0, LV_PART_INDICATOR);
            // Main part (background of the bar)
            lv_obj_set_style_bg_color(bin_left, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);  // Dark Grey
            // Indicator part (the value/progress part of the bar)
            lv_obj_set_style_bg_color(bin_left, COLOR_YELLOW, LV_PART_INDICATOR);

            // Create and position label for left bin
            lbl_bin_left_name = lv_label_create(scr);
            lv_label_set_text(lbl_bin_left_name, "SEED");
            lv_obj_set_style_text_color(lbl_bin_left_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_left_name, bin_left, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);  // 5px spacing below the bar



            //bin_right = lv_bar_create(scr); // Declaration moved to global
            bin_right = lv_bar_create(scr);
            lv_obj_set_size(bin_right, 80, 390);
            lv_obj_align(bin_right, LV_ALIGN_LEFT_MID, 150, 0);
            lv_bar_set_range(bin_right, 0, 100);
            lv_bar_set_value(bin_right, 60, LV_ANIM_OFF);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bin_right, 0, LV_PART_INDICATOR);
            // Main part (background of the bar)
            lv_obj_set_style_bg_color(bin_right, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);  // Dark Grey
            // Indicator part (the value/progress part of the bar)
            lv_obj_set_style_bg_color(bin_right, COLOR_ORANGE, LV_PART_INDICATOR);

            // Create and position label for right bin
            lbl_bin_right_name = lv_label_create(scr);
            lv_label_set_text(lbl_bin_right_name, "FERTILIZER");
            lv_obj_set_style_text_color(lbl_bin_right_name, COLOR_SILVER, 0);
            lv_obj_align_to(lbl_bin_right_name, bin_right, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);  // 5px spacing below the bar

            int current_y_offset = 140;
            int label_spacing = 110;  // Estimated height + spacing for each label box. Adjust if needed.
            int x_offset = -20;        //  ( X ,Y ) =  X = left - or right +   Y = up - or down +

            // Ground speed
            // Create panel_speed (x_offset and current_y_offset are already defined)
            panel_speed = lv_obj_create(scr);
            lv_obj_set_size(panel_speed, 250, 100);  // Width 300, height wraps content DEPTH IS THE SECOND NUMBER
            lv_obj_set_style_bg_color(panel_speed, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_speed, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_speed, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_speed, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_speed, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_speed, 8, LV_PART_MAIN | LV_STATE_DEFAULT);  // Panel padding
            lv_obj_align(panel_speed, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);  // Align the panel

            // Modify speed_label
            speed_label = lv_label_create(panel_speed);                   // Create label as child of panel_speed
            lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 0.0);  // Keep this

            // New text styling for speed_label:
            lv_obj_set_style_text_color(speed_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(speed_label);  // Center label within the panel

            // Seeding rate
            current_y_offset += label_spacing;  // Keep this line from the previous layout logic
            panel_rate = lv_obj_create(scr);
            lv_obj_set_size(panel_rate, 250, 100);
            lv_obj_set_style_bg_color(panel_rate, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_rate, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_rate, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_rate, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_rate, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_rate, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(panel_rate, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);  // Align the panel

            // Modify rate_label
            rate_label = lv_label_create(panel_rate);                   // Create label as child of panel_rate
            lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 0.0);  // Keep its text format

            // New text styling for rate_label:
            lv_obj_set_style_text_color(rate_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(rate_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(rate_label);  // Center label within the panel

            // Total area sown
            current_y_offset += label_spacing;  // Keep this line
            panel_area = lv_obj_create(scr);
            lv_obj_set_size(panel_area, 250, 100);
            lv_obj_set_style_bg_color(panel_area, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(panel_area, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(panel_area, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(panel_area, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(panel_area, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(panel_area, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(panel_area, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset);  // Align the panel

            // Modify area_label
            area_label = lv_label_create(panel_area);                 // Create label as child of panel_area
            lv_label_set_text_fmt(area_label, "Area: %.1f ha", 0.0);  // Keep its text format

            // New text styling for area_label:
            lv_obj_set_style_text_color(area_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(area_label, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(area_label);  // Center label within the panel

            // Create a box/container (object)
            drill_box = lv_obj_create(scr);
            lv_obj_set_size(drill_box, 180, 110);
            lv_obj_set_style_pad_all(drill_box, 9, 0);
            lv_obj_set_style_border_width(drill_box, 3, 0);
            lv_obj_set_style_border_color(drill_box, lv_color_white(), 0);
            lv_obj_set_style_border_opa(drill_box, LV_OPA_COVER, 0);
            // Don't set bg here; the loop will control it                   // LV_ALIGN_CENTER  LV_ALIGN_RIGHT_MID  LV_ALIGN_LEFT_MID
                                                                              //  ( X ,Y ) =  X = left - or right +   Y = up - or down +
            lv_obj_set_style_bg_opa(drill_box, LV_OPA_COVER, 0);
            lv_obj_align(drill_box, LV_ALIGN_CENTER, -20, 140);

            drill_label = lv_label_create(drill_box);
            lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(drill_label, "DRILL\nDOWN");  // Initial text
           lv_obj_set_style_text_align(drill_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(drill_label);




            // Alarm indicator
            //alarm_box = lv_label_create(scr); // Declaration moved to global
            //current_y_offset += label_spacing;
            alarm_btn = lv_btn_create(scr);
            lv_obj_set_width(alarm_btn, 160);
            lv_obj_set_height(alarm_btn, 90);
            lv_obj_set_style_pad_all(alarm_btn, 10, 0);
            lv_obj_set_style_border_width(alarm_btn, 3, 0);
            lv_obj_set_style_border_color(alarm_btn, lv_color_white(), 0);
            lv_obj_set_style_border_opa(alarm_btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(alarm_btn,CUSTOM_COLOR_2, LV_PART_MAIN | LV_STATE_DEFAULT);  // Red background  CUSTOM_COLOR_2
            lv_obj_set_style_text_color(alarm_btn, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(alarm_btn, LV_OPA_COVER, 0);
            lv_obj_align(alarm_btn, LV_ALIGN_TOP_RIGHT, -70, 20);

            // Create label inside button
            lv_obj_t *alarm_label = lv_label_create(alarm_btn);
            
            lv_label_set_text(alarm_label, "PUSH TO\nCLEAR ALARM");
            lv_obj_set_style_text_align(alarm_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_set_style_text_font(alarm_label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
         

            
// Attach event callback
lv_obj_add_event_cb(alarm_btn, alarm_ack_callback, LV_EVENT_CLICKED, NULL);


       spinner = lv_spinner_create(scr, 750, 40); // arc length 60, spin time 1000ms
                lv_obj_set_size(spinner, 135, 135);
                lv_obj_align(spinner, LV_ALIGN_CENTER, -20, -160); // LV_ALIGN_CENTER  LV_ALIGN_RIGHT_MID  LV_ALIGN_LEFT_MID
                                                                //  ( X ,Y ) =  X = left - or right +   Y = up - or down +
                // Style the spinner to have a green arc
                // For LVGL v8, styling is done via parts and states.
                // We want to style the LV_PART_INDICATOR which is the arc of the spinner.
                lv_obj_set_style_arc_color(spinner, lv_color_make(0x80, 0x00, 0x80), LV_PART_INDICATOR); // add this lv_color_make
                lv_obj_set_style_arc_width(spinner, 20, LV_PART_INDICATOR); // Make the arc a bit thicker for visibility
                // Make the base/background track arc wide (e.g., 30px)
                 lv_obj_set_style_arc_width(spinner, 19, LV_PART_MAIN);
            /* Add a label to the center of the spinner */
                lv_obj_t * spinner_label = lv_label_create(spinner);
                lv_label_set_text(spinner_label, "CLUTCH\nENGAGED");  //engaged
                 lv_obj_set_style_text_align(spinner_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(spinner_label);
                lv_obj_set_style_text_color(spinner_label, lv_color_make(0xC0, 0xC0, 0xC0), 0); // Set text color to white


                lvgl_port_unlock();
                //return; // Explicit return statement
                start_warning_flash();
            }


            void loop() {
            // LVGL task handler
            lv_timer_handler();  // Should be called periodically
            delay(5);            // Small delay for LVGL task handling
// Format float to string: dtostrf(value, width, precision, buffer)
            dtostrf(currentSpeed, 4, 1, speedBuf);  // e.g., 42.5 -> "42.5"
            dtostrf(currentRate, 4, 1, rateBuf);    // e.g., 3.1  -> "3.1"
            dtostrf(totalArea, 5, 1, areaBuf);    // e.g., 12.7 -> "12.7"

            char speedLabel[32], rateLabel[32], areaLabel[32];
            snprintf(speedLabel, sizeof(speedLabel), "Speed: %s km/h", speedBuf);
            snprintf(rateLabel, sizeof(rateLabel), "Rate: %s ha/h", rateBuf);
            snprintf(areaLabel, sizeof(areaLabel), "Area: %s ha", areaBuf);
            unsigned long current_time = millis();
            if (current_time - last_toggle_time >= toggle_interval) {
                last_toggle_time = current_time;
                test_mode_toggle_state = !test_mode_toggle_state;  // Flip the state

                lvgl_port_lock(-1);  // Lock LVGL access

                if (test_mode_toggle_state) {
                currentSpeed = 8.7;
                 currentRate = 25.5;
                totalArea = 85.2;
                
                // State 1: Set to sample values
                if (bin_left) lv_bar_set_value(bin_left, 100, LV_ANIM_OFF);
                if (bin_right) lv_bar_set_value(bin_right, 100, LV_ANIM_OFF);
                if (speed_label) lv_label_set_text(speed_label, speedLabel);
                if (rate_label) lv_label_set_text(rate_label, rateLabel);
                if (area_label) lv_label_set_text(area_label, areaLabel);
                // Update drill indicator for DOWN state (GREEN)
                if (drill_box) {
                    lv_obj_set_style_bg_color(drill_box, COLOR_GREEN, 0);

                    lv_label_set_text(drill_label, "DRILL\nDOWN");
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0);  // white text on green, or change
                    lv_anim_t a;
                    lv_anim_init(&a);
                    lv_anim_set_var(&a, drill_box);
                    lv_anim_set_exec_cb(&a, drill_box_anim_cb);
                    lv_anim_set_time(&a, 2000);                             // duration of one fade in/out (ms)
                    lv_anim_set_values(&a, LV_OPA_40, LV_OPA_COVER);        // from 30% to 100% opaque
                    lv_anim_set_playback_time(&a, 1000);                    // fade out time
                    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);  // loop forever
                    lv_anim_start(&a);
                }
                if (alarm_box) {
                    lv_label_set_text(alarm_box, "ALARM!");
                    // Text color is now fixed to white in setup, background makes it an alarm
                }


                } else {
                 currentSpeed = 11.3;
                 currentRate = 2.85;
               totalArea = 15.2;
                // State 0: Set to zero or 'off' values
                if (bin_left) lv_bar_set_value(bin_left, 25, LV_ANIM_OFF);
                if (bin_right) lv_bar_set_value(bin_right, 15, LV_ANIM_OFF);
                if (speed_label) lv_label_set_text(speed_label, speedLabel);
                if (rate_label) lv_label_set_text(rate_label, rateLabel);
                if (area_label) lv_label_set_text(area_label, areaLabel);
                // Update drill indicator for UP state (RED)
                if (drill_box) {
                    lv_obj_set_style_bg_color(drill_box, lv_palette_main(LV_PALETTE_RED), 0);
                    lv_label_set_text(drill_label, "DRILL\nUP");
                    lv_obj_set_style_text_font(drill_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(drill_label, lv_color_black(), 0);  // black text on red
                    lv_anim_del(drill_box, drill_box_anim_cb);
                    lv_obj_set_style_bg_opa(drill_box, LV_OPA_COVER, 0);  // reset to fully opaque
                }
                if (alarm_box) {
                    lv_label_set_text(alarm_box, "OFF");  // Clear alarm
                    // Text color is now fixed to white in setup
                }
                }

                lvgl_port_unlock();                     // Unlock LVGL access
                Serial.println("Widget data toggled");  // For debugging
                delay(5000);
                start_warning_flash();
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

            // Add alarm-clearing logic here
            stop_warning_flash();  // <--- This line stops the
            }


            void flash_warn_label_cb(lv_timer_t *timer) {
            lv_obj_t *label = (lv_obj_t *)timer->user_data;
            if (lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);  // Show
            } else {
                lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);  // Hide
            }
            }


            void start_warning_flash() {
            if (!warn_label) {
                warn_label = lv_label_create(lv_scr_act());
                lv_label_set_text(warn_label, "WARNING!");
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALETTE_RED), 0);
                lv_obj_set_style_text_font(warn_label, &lv_font_montserrat_42, 0);  // Larger font (optional)
                lv_obj_align(warn_label, LV_ALIGN_CENTER, -20, -10);
            } else {
                lv_label_set_text(warn_label, "WARNING!");
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);
            }
            if (!warn_flash_timer) {
                warn_flash_timer = lv_timer_create(flash_warn_label_cb, 500, warn_label);  // 500ms
            }
            }


            void stop_warning_flash() {
            if (warn_flash_timer) {
                lv_timer_del(warn_flash_timer);
                warn_flash_timer = NULL;
            }
            if (warn_label) {
                lv_obj_clear_flag(warn_label, LV_OBJ_FLAG_HIDDEN);                              // Ensure visible
                lv_obj_set_style_text_color(warn_label, lv_palette_main(LV_PALETTE_GREEN), 0);  // Set to green or whatever color
                lv_label_set_text(warn_label, "SAFE");                                          // Or clear text: lv_label_set_text(warn_label, "");
            }
            }
