// alarm_buttons.h

#ifndef ALARM_BUTTONS_H
#define ALARM_BUTTONS_H

#include <lvgl.h> // Make sure LVGL is included
#include "color.h" // Assuming CUSTOM_COLOR_2 is here

// Define a structure to hold elements for one alarm button
typedef struct {
    lv_obj_t *btn;
    lv_obj_t *label;
    const char *name; // e.g., "SEED", "FERT", "OIL"
    // Add any other specific state for this alarm if needed
    // bool is_active;
    // int alarm_id;
} alarm_button_t;

// Declare a function to create a single alarm button
// It will take the parent object (e.g., a tab or the settings_scr itself),
// the text for the button, and a callback function for when it's clicked.
// We'll also pass a pointer to an alarm_button_t struct to store the created objects.
// Implementation is inline.
inline void create_alarm_button(lv_obj_t *parent, alarm_button_t *alarm_ui, const char *button_text, const char* name_text, lv_event_cb_t event_cb, void *user_data) {
    if (!parent || !alarm_ui) return;

    alarm_ui->name = name_text; // Store the name

    alarm_ui->btn = lv_btn_create(parent);
    lv_obj_set_size(alarm_ui->btn, 160, 100); // Adjusted height slightly
    lv_obj_set_style_pad_all(alarm_ui->btn, 10, 0);
    lv_obj_set_style_border_width(alarm_ui->btn, 3, 0); // Slightly thinner border
    lv_obj_set_style_border_color(alarm_ui->btn, lv_color_white(), 0);
    // Default button color
    lv_obj_set_style_bg_color(alarm_ui->btn, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
    // Uncomment and use if CUSTOM_COLOR_2 is defined and color.h is correctly included
    // lv_obj_set_style_bg_color(alarm_ui->btn, CUSTOM_COLOR_2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(alarm_ui->btn, lv_color_black(), 0); // Text color for label on button

    alarm_ui->label = lv_label_create(alarm_ui->btn);
    lv_label_set_text(alarm_ui->label, button_text); // e.g., "SEED\nALARM"
    lv_obj_set_style_text_align(alarm_ui->label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(alarm_ui->label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT); // Adjusted font
    lv_obj_center(alarm_ui->label);

    if (event_cb) {
        lv_obj_add_event_cb(alarm_ui->btn, event_cb, LV_EVENT_CLICKED, user_data);
    }
}

#endif // ALARM_BUTTONS_H
