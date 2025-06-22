/**
 * Detailed usage of the example can be found in the [README.md](./README.md) file
 */
#define LV_CONF_INCLUDE_SIMPLE      // place befor lvgl.h
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
LV_IMG_DECLARE(img_drill_down);
LV_IMG_DECLARE(img_drill_up);


/* Bin level bars */
static lv_obj_t * bin_left= NULL;
static lv_obj_t * lbl_bin_left_name= NULL;
static lv_obj_t * bin_right= NULL;
static lv_obj_t * lbl_bin_right_name= NULL;


/* Data labels */
static lv_obj_t * speed_label= NULL;
static lv_obj_t * rate_label= NULL;
static lv_obj_t * area_label= NULL;

/* Alarm and status */
static lv_obj_t * alarm_box= NULL;


static lv_obj_t * drill_indicator_img = NULL; // Renamed and will replace the drill_status label

/* Spinner arc */
static lv_obj_t * spinner;
static lv_style_t style_arc;
static lv_anim_t anim;

static lv_obj_t *panel_speed = NULL;
static lv_obj_t *panel_rate = NULL;
static lv_obj_t *panel_area = NULL;
 


static bool test_mode_toggle_state = false; // General toggle for values
static unsigned long last_toggle_time = 0;
const unsigned long toggle_interval = 2000; // 2 seconds



void setup()
{
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
    lv_obj_set_style_bg_color(bin_left, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN); // Dark Grey
    // Indicator part (the value/progress part of the bar)
    lv_obj_set_style_bg_color(bin_left, COLOR_YELLOW, LV_PART_INDICATOR);

    // Create and position label for left bin
    lbl_bin_left_name = lv_label_create(scr);
    lv_label_set_text(lbl_bin_left_name, "SEED");
    lv_obj_set_style_text_color(lbl_bin_left_name, COLOR_SILVER, 0);
    lv_obj_align_to(lbl_bin_left_name, bin_left, LV_ALIGN_OUT_BOTTOM_MID, 0, 5); // 5px spacing below the bar



    //bin_right = lv_bar_create(scr); // Declaration moved to global
    bin_right = lv_bar_create(scr);
    lv_obj_set_size(bin_right, 80, 390);
    lv_obj_align(bin_right, LV_ALIGN_LEFT_MID, 150, 0);
    lv_bar_set_range(bin_right, 0, 100);
    lv_bar_set_value(bin_right, 60, LV_ANIM_OFF);
    lv_obj_set_style_radius(bin_right, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bin_right, 0, LV_PART_INDICATOR);
    // Main part (background of the bar)
    lv_obj_set_style_bg_color(bin_right, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN); // Dark Grey
    // Indicator part (the value/progress part of the bar)
    lv_obj_set_style_bg_color(bin_right, COLOR_ORANGE, LV_PART_INDICATOR);

    // Create and position label for right bin
    lbl_bin_right_name = lv_label_create(scr);
    lv_label_set_text(lbl_bin_right_name, "FERTILIZER");
    lv_obj_set_style_text_color(lbl_bin_right_name, COLOR_SILVER, 0);
    lv_obj_align_to(lbl_bin_right_name, bin_right, LV_ALIGN_OUT_BOTTOM_MID, 0, 5); // 5px spacing below the bar

   int current_y_offset = 50;
    int label_spacing = 150; // Estimated height + spacing for each label box. Adjust if needed.
    int x_offset = -30;

      // Ground speed
    // Create panel_speed (x_offset and current_y_offset are already defined)
    panel_speed = lv_obj_create(scr);
    lv_obj_set_size(panel_speed, 250, 100); // Width 300, height wraps content DEPTH IS THE SECOND NUMBER 
    lv_obj_set_style_bg_color(panel_speed, COLOR_AQUA, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel_speed, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_speed, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel_speed, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_speed, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_speed, 8, LV_PART_MAIN | LV_STATE_DEFAULT); // Panel padding
    lv_obj_align(panel_speed, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset); // Align the panel

    // Modify speed_label
    speed_label = lv_label_create(panel_speed); // Create label as child of panel_speed
    lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 0.0); // Keep this

    // New text styling for speed_label:
    lv_obj_set_style_text_color(speed_label, CUSTOM_COLOR_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(speed_label); // Center label within the panel

    // Seeding rate
    current_y_offset += label_spacing; // Keep this line from the previous layout logic
    panel_rate = lv_obj_create(scr);
    lv_obj_set_size(panel_rate,250, 100);
    lv_obj_set_style_bg_color(panel_rate, COLOR_GREEN, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel_rate, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_rate, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel_rate, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_rate, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_rate, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(panel_rate, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset); // Align the panel

    // Modify rate_label
    rate_label = lv_label_create(panel_rate); // Create label as child of panel_rate
    lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 0.0); // Keep its text format

    // New text styling for rate_label:
    lv_obj_set_style_text_color(rate_label, COLOR_NAVY , LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(rate_label, &lv_font_montserrat_34, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(rate_label); // Center label within the panel

    // Total area sown
    current_y_offset += label_spacing; // Keep this line
    panel_area = lv_obj_create(scr);
    lv_obj_set_size(panel_area, 250, 100);
    lv_obj_set_style_bg_color(panel_area, COLOR_TEAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel_area, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_area, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel_area, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_area, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_area, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(panel_area, LV_ALIGN_TOP_RIGHT, x_offset, current_y_offset); // Align the panel

    // Modify area_label
    area_label = lv_label_create(panel_area); // Create label as child of panel_area
    lv_label_set_text_fmt(area_label, "Area: %.1f ha", 0.0); // Keep its text format

    // New text styling for area_label:
    lv_obj_set_style_text_color(area_label, COLOR_YELLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(area_label, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(area_label); // Center label within the panel


  // Drill Indicator Image
    // current_y_offset += label_spacing; // This offset logic might need re-evaluation if other elements depended on drill_status label's height.
                                        // For now, we remove the label and place the image.
    drill_indicator_img = lv_img_create(scr);                   // create the image object using the global variable
    lv_img_set_src(drill_indicator_img, &img_drill_up);         // Start with drill UP
    lv_obj_set_width(drill_indicator_img, 140);                 // Set width (as original)
    lv_obj_set_height(drill_indicator_img, LV_SIZE_CONTENT);    // Set height (as original)
    lv_obj_align(drill_indicator_img, LV_ALIGN_CENTER, 0, 100); // Position it (original image position)

    // Style for drill UP (RED indicator) - Attempting recolor first
    lv_obj_set_style_img_recolor(drill_indicator_img, COLOR_RED, 0);
    lv_obj_set_style_img_recolor_opa(drill_indicator_img, LV_OPA_COVER, 0);
    // Fallback: if images are not monochrome, recolor won't work as expected.
    // In that case, one would use:
    // lv_obj_set_style_bg_color(drill_indicator_img, COLOR_RED, 0);
    // lv_obj_set_style_bg_opa(drill_indicator_img, LV_OPA_COVER, 0);
    // Ensure image has transparency for bg color to be visible as a 'tint' or border.

    

     // Alarm indicator
    //alarm_box = lv_label_create(scr); // Declaration moved to global
    current_y_offset += label_spacing;
    alarm_box = lv_label_create(scr);
    //lv_obj_set_style_text_color(alarm_box, lv_palette_main(LV_PALETTE_RED), 0); // This line is managed by the new setup below
    lv_label_set_text(alarm_box, "PUSH"); // Initial text
    lv_obj_set_width(alarm_box, 200);
    lv_obj_set_height(alarm_box, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(alarm_box, 5, 0);
    lv_obj_set_style_border_width(alarm_box, 1, 0);
    lv_obj_set_style_border_color(alarm_box, lv_color_white(), 0);           // White border
    lv_obj_set_style_border_opa(alarm_box, LV_OPA_COVER, 0);
    // New styling for alarm_box background and text
    lv_obj_set_style_bg_color(alarm_box, lv_palette_main(LV_PALETTE_RED), 3); // Red background
    lv_obj_set_style_text_color(alarm_box, lv_color_black(), 0);             // White text
    lv_obj_set_style_bg_opa(alarm_box, LV_OPA_COVER, 0); // Ensure background is opaque
    lv_obj_align(alarm_box, LV_ALIGN_CENTER, 0, 0);


    // Create and style the spinner
    spinner = lv_spinner_create(scr, 800, 60); // arc length 60, spin time 1000ms
    lv_obj_set_size(spinner, 120, 120);
    lv_obj_align(spinner, LV_ALIGN_CENTER, -10, -140); // LV_ALIGN_CENTER  LV_ALIGN_RIGHT_MID  LV_ALIGN_LEFT_MID
                                                    //  ( X ,Y ) =  X = left - or right +   Y = up - or down +
    // Style the spinner to have a green arc
    // For LVGL v8, styling is done via parts and states.
    // We want to style the LV_PART_INDICATOR which is the arc of the spinner.
    lv_obj_set_style_arc_color(spinner, lv_color_make(0x5A, 0xD0, 0x2F), LV_PART_INDICATOR); // add this lv_color_make
    lv_obj_set_style_arc_width(spinner, 18, LV_PART_INDICATOR); // Make the arc a bit thicker for visibility
   /* Add a label to the center of the spinner */
    lv_obj_t * spinner_label = lv_label_create(spinner);
    lv_label_set_text(spinner_label, "ENGAGED");  //engaged
    lv_obj_center(spinner_label);
    lv_obj_set_style_text_color(spinner_label, lv_color_white(), 0); // Set text color to white


    lvgl_port_unlock();
     //return; // Explicit return statement
}


void loop()
{
 // LVGL task handler
    lv_timer_handler(); // Should be called periodically
    delay(5); // Small delay for LVGL task handling

    unsigned long current_time = millis();
    if (current_time - last_toggle_time >= toggle_interval) {
        last_toggle_time = current_time;
        test_mode_toggle_state = !test_mode_toggle_state; // Flip the state

        lvgl_port_lock(-1); // Lock LVGL access

        if (test_mode_toggle_state) {
            // State 1: Set to sample values
            if (bin_left) lv_bar_set_value(bin_left, 100, LV_ANIM_OFF);
            if (bin_right) lv_bar_set_value(bin_right, 100, LV_ANIM_OFF);
            if (speed_label) lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 42.5);
            if (rate_label) lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 3.1);
            if (area_label) lv_label_set_text_fmt(area_label, "Area: %.1f ha", 12.7);
            // Update drill indicator for DOWN state (GREEN)
            if (drill_indicator_img) {
                lv_img_set_src(drill_indicator_img, &img_drill_down);
                lv_obj_set_style_img_recolor(drill_indicator_img, COLOR_GREEN, 0);
                lv_obj_set_style_img_recolor_opa(drill_indicator_img, LV_OPA_COVER, 0);
                // Fallback: lv_obj_set_style_bg_color(drill_indicator_img, COLOR_GREEN, 0);
            }
            if (alarm_box) {
                lv_label_set_text(alarm_box, "ALARM!");
                // Text color is now fixed to white in setup, background makes it an alarm
            }
        } else {
            // State 0: Set to zero or 'off' values
            if (bin_left) lv_bar_set_value(bin_left, 25, LV_ANIM_OFF);
            if (bin_right) lv_bar_set_value(bin_right, 15, LV_ANIM_OFF);
            if (speed_label) lv_label_set_text_fmt(speed_label, "Speed: %.1f km/h", 0.0);
            if (rate_label) lv_label_set_text_fmt(rate_label, "Rate: %.1f ha/h", 0.0);
            if (area_label) lv_label_set_text_fmt(area_label, "Area: %.1f ha", 0.0);
            // Update drill indicator for UP state (RED)
            if (drill_indicator_img) {
                lv_img_set_src(drill_indicator_img, &img_drill_up);
                lv_obj_set_style_img_recolor(drill_indicator_img, COLOR_RED, 0);
                lv_obj_set_style_img_recolor_opa(drill_indicator_img, LV_OPA_COVER, 0);
                // Fallback: lv_obj_set_style_bg_color(drill_indicator_img, COLOR_RED, 0);
            }
            if (alarm_box) {
                lv_label_set_text(alarm_box, ""); // Clear alarm
                // Text color is now fixed to white in setup
                }
        }

        lvgl_port_unlock(); // Unlock LVGL access
        Serial.println("Widget data toggled"); // For debugging
    }
}
