/* LVGL demo for a board without a display: widgets drawn by LVGL into memory
 * and shown over VNC (see vnc-display.c). Connect a VNC viewer to port 5900 (the
 * server listens on the loopback address only; use an ssh tunnel). */
#include <stdio.h>
#include <string.h>
#include "vnc-display.h"

static lv_obj_t *count_label, *slider_label, *load_label;

static void button_cb(lv_event_t *e)
{
    static int count;

    lv_label_set_text_fmt(count_label, "Clicked %d times", ++count);
}

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    lv_label_set_text_fmt(slider_label, "Slider: %d", (int)lv_slider_get_value(slider));
}

static void load_timer_cb(lv_timer_t *t)
{
    char buf[64] = "";
    FILE *f = fopen("/proc/loadavg", "r");

    if (f) {
        if (!fgets(buf, sizeof(buf), f))
            buf[0] = 0;
        fclose(f);
    }
    buf[strcspn(buf, "\n")] = 0;
    lv_label_set_text_fmt(load_label, "Load: %s", buf);
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *title = lv_label_create(scr);
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_t *slider = lv_slider_create(scr);

    lv_label_set_text(title, "EP9302 + LVGL over VNC");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_add_event_cb(btn, button_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn), "Click me");

    count_label = lv_label_create(scr);
    lv_label_set_text(count_label, "Clicked 0 times");
    lv_obj_align(count_label, LV_ALIGN_TOP_MID, 0, 95);

    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    slider_label = lv_label_create(scr);
    lv_label_set_text(slider_label, "Slider: 0");
    lv_obj_align(slider_label, LV_ALIGN_TOP_MID, 0, 160);

    load_label = lv_label_create(scr);
    lv_obj_align(load_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    load_timer_cb(NULL);
    lv_timer_create(load_timer_cb, 2000, NULL);
}

int main(int argc, char **argv)
{
    if (vnc_display_init(&argc, argv) != 0)
        return 1;
    build_ui();
    vnc_display_run();
    return 0;
}
