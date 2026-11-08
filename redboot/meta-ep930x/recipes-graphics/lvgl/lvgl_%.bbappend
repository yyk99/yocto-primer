# LVGL for a board with no framebuffer: only the core library, no DRM, fbdev
# or SDL drivers. The demo draws into memory and serves it over VNC.
PACKAGECONFIG = ""

# RGB565, the format the demo hands to the VNC server
LVGL_CONFIG_LV_COLOR_DEPTH = "16"
# Room for the clock's 96 px font (about 11 cached glyphs) next to the widgets
LVGL_CONFIG_LV_MEM_SIZE = "(256 * 1024U)"

# The clock draws its digits from an embedded TrueType font (tiny_ttf), and
# its date in Montserrat 20; lv-conf.inc has no switches for these.
do_configure:append() {
    sed -r -e "s|^([[:space:]]*#define LV_USE_TINY_TTF[[:space:]]).*|\1 1|" \
           -e "s|^([[:space:]]*#define LV_FONT_MONTSERRAT_20[[:space:]]).*|\1 1|" \
           -i "${S}/lv_conf.h"
    grep -q "define LV_USE_TINY_TTF *1" "${S}/lv_conf.h" || bbfatal "LV_USE_TINY_TTF was not enabled in lv_conf.h"
}
