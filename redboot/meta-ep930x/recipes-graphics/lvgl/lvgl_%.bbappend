# LVGL for a board with no framebuffer: only the core library, no DRM, fbdev
# or SDL drivers. The demo draws into memory and serves it over VNC.
PACKAGECONFIG = ""

# RGB565, the format the demo hands to the VNC server
LVGL_CONFIG_LV_COLOR_DEPTH = "16"
LVGL_CONFIG_LV_MEM_SIZE = "(128 * 1024U)"
