#include "vnc-display.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <rfb/rfb.h>

static rfbScreenInfoPtr screen;
static int ptr_x, ptr_y, ptr_down, ptr_latched;

/* LVGL's millisecond clock, registered with lv_tick_set_cb() */
static uint32_t tick_get(void)
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
        memcpy(dst + y * VNC_WIDTH + area->x1, src, w * 2);
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

int vnc_display_init(int *argc, char **argv)
{
    static uint8_t draw_buf[VNC_WIDTH * 24 * 2];   /* 24 rows of RGB565 */
    lv_display_t *disp;
    lv_indev_t *indev;

    screen = rfbGetScreen(argc, argv, VNC_WIDTH, VNC_HEIGHT, 5, 3, 2);
    if (!screen)
        return -1;
    screen->desktopName = "ep930x";
    screen->frameBuffer = calloc(VNC_WIDTH * VNC_HEIGHT, 2);
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
    lv_tick_set_cb(tick_get);
    disp = lv_display_create(VNC_WIDTH, VNC_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);
    return 0;
}

void vnc_display_run(void)
{
    while (rfbIsActive(screen)) {
        uint32_t ms = lv_timer_handler();

        if (ms > 30)
            ms = 30;
        rfbProcessEvents(screen, ms * 1000);
    }
}
