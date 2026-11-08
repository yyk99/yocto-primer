/* LVGL demo for a board without a display: LVGL renders into memory and
 * LibVNCServer shows the result. Connect a VNC viewer to port 5900 (the server
 * listens on the loopback address only; use an ssh tunnel). */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <rfb/rfb.h>
#include <lvgl/lvgl.h>

#define WIDTH  320
#define HEIGHT 240

static rfbScreenInfoPtr screen;
static int ptr_x, ptr_y, ptr_down, ptr_latched;

/* LVGL's millisecond clock, registered with lv_tick_set_cb() */
static uint32_t custom_tick_get(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    int w = lv_area_get_width(area);
    uint16_t *src = (uint16_t *)px;
    uint16_t *dst = (uint16_t *)screen->frameBuffer;

    for (int y = area->y1; y <= area->y2; y++) {
        memcpy(dst + y * WIDTH + area->x1, src, w * 2);
        src += w;
    }
    rfbMarkRectAsModified(screen, area->x1, area->y1, area->x2 + 1, area->y2 + 1);
    lv_display_flush_ready(disp);
}

static void ptr_event(int mask, int x, int y, rfbClientPtr cl)
{
    /* Act like a touch panel: only the left button counts as a touch, and the
     * position changes only while touching (and at the release), so moving the
     * mouse with no button pressed does nothing. */
    if ((mask & 1) || ptr_down) {
        ptr_x = x;
        ptr_y = y;
    }
    ptr_down = mask & 1;
    if (ptr_down)
        ptr_latched = 1;
    rfbDefaultPtrAddEvent(mask, x, y, cl);
}

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    data->point.x = ptr_x;
    data->point.y = ptr_y;
    /* A quick click presses and releases between two reads (every 30 ms);
     * report every press at least once. */
    if (ptr_down || ptr_latched)
        data->state = LV_INDEV_STATE_PRESSED;
    else
        data->state = LV_INDEV_STATE_RELEASED;
    ptr_latched = 0;
}

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
    static uint8_t draw_buf[WIDTH * 24 * 2];   /* 24 rows of RGB565 */
    lv_display_t *disp;
    lv_indev_t *indev;

    screen = rfbGetScreen(&argc, argv, WIDTH, HEIGHT, 5, 3, 2);
    if (!screen)
        return 1;
    screen->desktopName = "ep930x";
    screen->frameBuffer = calloc(WIDTH * HEIGHT, 2);
    screen->alwaysShared = TRUE;
    screen->listenInterface = htonl(INADDR_LOOPBACK);
    screen->ptrAddEvent = ptr_event;
    /* RGB565, little endian */
    screen->serverFormat.bitsPerPixel = 16;
    screen->serverFormat.depth = 16;
    screen->serverFormat.bigEndian = 0;
    screen->serverFormat.redMax = 31;
    screen->serverFormat.greenMax = 63;
    screen->serverFormat.blueMax = 31;
    screen->serverFormat.redShift = 11;
    screen->serverFormat.greenShift = 5;
    screen->serverFormat.blueShift = 0;
    rfbInitServer(screen);

    lv_init();
    lv_tick_set_cb(custom_tick_get);
    disp = lv_display_create(WIDTH, HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);

    build_ui();

    while (rfbIsActive(screen)) {
        uint32_t ms = lv_timer_handler();

        if (ms > 30)
            ms = 30;
        rfbProcessEvents(screen, ms * 1000);
    }
    return 0;
}
