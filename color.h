#ifndef COLOR_H
#define COLOR_H

#include "lvgl.h"

/* === Standard Colors === */
#define COLOR_WHITE      lv_color_make(0xFF, 0xFF, 0xFF)
#define COLOR_RED        lv_color_make(0xFF, 0x00, 0x00)
#define COLOR_GREEN      lv_color_make(0x47, 0xFC, 0x05)
#define COLOR_BLUE       lv_color_make(0x00, 0x00, 0xFF)
#define COLOR_SILVER     lv_color_make(0xC0, 0xC0, 0xC0)
#define COLOR_YELLOW     lv_color_make(0xFF, 0xFF, 0x00)
#define COLOR_ORANGE     lv_color_make(0xFF, 0xA5, 0x00)
#define COLOR_PURPLE     lv_color_make(0x80, 0x00, 0x80)
#define COLOR_NAVY       lv_color_make(0x00, 0x00, 0x80)
#define COLOR_LIME       lv_color_make(0x00, 0xFF, 0x00)
#define COLOR_TEAL       lv_color_make(0x00, 0x80, 0x80)
#define COLOR_AQUA       lv_color_make(0x00, 0xFF, 0xFF)
#define COLOR_FUCHSIA    lv_color_make(0xFF, 0x00, 0xFF)
#define COLOR_MAROON     lv_color_make(0x80, 0x00, 0x00)
#define COLOR_OLIVE      lv_color_make(0x80, 0x80, 0x00)

/* === Custom Colors ===
 * Define up to 10 custom colors for your project:
 * Replace the hex values with your desired RGB.
 */
#define CUSTOM_COLOR_1   lv_color_make(0x00, 0x00, 0x00)  // black
#define CUSTOM_COLOR_2   lv_color_make(0xD9, 0x00, 0x00)   // red
#define CUSTOM_COLOR_3   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_4   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_5   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_6   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_7   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_8   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_9   lv_color_make(0x00, 0x00, 0x00)
#define CUSTOM_COLOR_10  lv_color_make(0x00, 0x00, 0x00)

#endif /* COLOR_H */
