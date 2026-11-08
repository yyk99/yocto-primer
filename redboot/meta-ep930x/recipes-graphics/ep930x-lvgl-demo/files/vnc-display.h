/* Shared by the LVGL apps: a 320x240 RGB565 display that LVGL draws into
 * memory and LibVNCServer serves on the loopback address (port 5900, or
 * -rfbport N). The mouse acts as a touch panel. */
#ifndef VNC_DISPLAY_H
#define VNC_DISPLAY_H

#include <lvgl/lvgl.h>

#define VNC_WIDTH  320
#define VNC_HEIGHT 240

/* Start the VNC server and LVGL; returns 0 on success. Call before building
 * the UI. argv may contain LibVNCServer options such as -rfbport. */
int vnc_display_init(int *argc, char **argv);

/* Run the LVGL and VNC event loop until the server stops. */
void vnc_display_run(void);

#endif
