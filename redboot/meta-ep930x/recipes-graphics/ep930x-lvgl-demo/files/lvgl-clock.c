/* Digital clock for the 320x240 VNC display: HH:MM in big monospaced digits
 * and the date below, in the board's local time (TZ, UTC by default). Like
 * the SuziePi clock app, but drawn into memory and served over VNC (see
 * vnc-display.c). Port 5900, or -rfbport N when another app uses it. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "vnc-display.h"

#define TIME_FONT_PX 96 /* "HH:MM" is about 3 em wide: 96 px -> ~290 px of 320 */

static lv_obj_t *time_label;
static lv_obj_t *date_label;

extern const uint8_t digits_font_ttf[];
extern const size_t digits_font_ttf_len;

static void update_cb(lv_timer_t *timer)
{
    static int last_min = -1;
    time_t now = time(NULL);
    struct tm tm;
    char buf[48];

    (void)timer;
    localtime_r(&now, &tm);
    if (tm.tm_min == last_min)
        return;
    last_min = tm.tm_min;

    strftime(buf, sizeof(buf), "%H:%M", &tm);
    lv_label_set_text(time_label, buf);
    strftime(buf, sizeof(buf), "%a, %b %d %Y", &tm);
    lv_label_set_text(date_label, buf);
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_font_t *time_font;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    time_label = lv_label_create(scr);
    time_font = lv_tiny_ttf_create_data(digits_font_ttf, digits_font_ttf_len, TIME_FONT_PX);
    lv_label_set_text(time_label, "--:--");
    if (time_font)
        lv_obj_set_style_text_font(time_label, time_font, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x40FF80), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10);

    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xA0A0A0), 0);
    lv_obj_align(date_label, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_timer_create(update_cb, 500, NULL);
    update_cb(NULL);
}

int main(int argc, char **argv)
{
    if (vnc_display_init(&argc, argv) != 0)
        return 1;
    build_ui();
    vnc_display_run();
    return 0;
}
