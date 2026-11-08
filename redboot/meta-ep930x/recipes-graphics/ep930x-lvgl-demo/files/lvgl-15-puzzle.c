/* 15 puzzle for the 320x240 VNC display: 15 numbered tiles and one gap on a
 * 4x4 board. Tap a tile next to the gap to slide it; put the tiles in order
 * 1..15 with the gap in the bottom-right corner. Port 5900, or -rfbport N. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vnc-display.h"

#define N      4
#define TILE   50
#define GAP    4
#define PITCH  (TILE + GAP)

static int board[N * N];          /* value at each position, 0 = the gap */
static lv_obj_t *tiles[N * N];    /* tile object by value 1..15 */
static lv_obj_t *moves_label, *status_label;
static int moves;

static int solved(void)
{
    for (int i = 0; i < N * N - 1; i++)
        if (board[i] != i + 1)
            return 0;
    return 1;
}

static int find(int value)
{
    for (int i = 0; i < N * N; i++)
        if (board[i] == value)
            return i;
    return -1;
}

static void show_state(void)
{
    int done = solved();

    for (int p = 0; p < N * N; p++) {
        int v = board[p];

        if (!v)
            continue;
        lv_obj_set_pos(tiles[v], (p % N) * PITCH, (p / N) * PITCH);
        lv_obj_set_style_bg_color(tiles[v],
                                  lv_color_hex(done ? 0x2E9E4F : 0x3A6EA5), 0);
    }
    lv_label_set_text_fmt(moves_label, "Moves\n%d", moves);
    if (done)
        lv_label_set_text_fmt(status_label, "Solved\nin %d!", moves);
    else
        lv_label_set_text(status_label, "");
}

static void swap_with_gap(int p, int gap)
{
    board[gap] = board[p];
    board[p] = 0;
}

static void tile_cb(lv_event_t *e)
{
    int value = (int)(intptr_t)lv_event_get_user_data(e);
    int p = find(value), gap = find(0);
    int dr = p / N - gap / N, dc = p % N - gap % N;

    if (dr < 0)
        dr = -dr;
    if (dc < 0)
        dc = -dc;
    if (dr + dc != 1 || solved())   /* only a tile next to the gap moves */
        return;
    swap_with_gap(p, gap);
    moves++;
    show_state();
}

/* Shuffle with random legal moves from the solved position: every result can
 * be solved (a random placement of the tiles could not, half of the time). */
static void shuffle(void)
{
    do {
        int gap = N * N - 1, prev = -1;

        for (int i = 0; i < N * N; i++)
            board[i] = (i + 1) % (N * N);
        for (int n = 0; n < 300; n++) {
            int cand[4], k = 0, pick;

            if (gap >= N)
                cand[k++] = gap - N;
            if (gap < N * (N - 1))
                cand[k++] = gap + N;
            if (gap % N > 0)
                cand[k++] = gap - 1;
            if (gap % N < N - 1)
                cand[k++] = gap + 1;
            do {
                pick = cand[rand() % k];
            } while (pick == prev);     /* do not undo the last move */
            swap_with_gap(pick, gap);
            prev = gap;
            gap = pick;
        }
    } while (solved());
    moves = 0;
}

static void new_cb(lv_event_t *e)
{
    (void)e;
    shuffle();
    show_state();
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *frame = lv_obj_create(scr);
    lv_obj_t *title = lv_label_create(scr);
    lv_obj_t *btn = lv_button_create(scr);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* the board: a plain container, no padding or border */
    lv_obj_set_size(frame, N * PITCH - GAP, N * PITCH - GAP);
    lv_obj_set_pos(frame, 10, 12);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    for (int v = 1; v < N * N; v++) {
        lv_obj_t *label;

        tiles[v] = lv_button_create(frame);
        lv_obj_set_size(tiles[v], TILE, TILE);
        lv_obj_set_style_radius(tiles[v], 6, 0);
        lv_obj_add_event_cb(tiles[v], tile_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)v);
        label = lv_label_create(tiles[v]);
        lv_label_set_text_fmt(label, "%d", v);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_center(label);
    }

    /* the panel on the right */
    lv_label_set_text(title, "15");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(title, 262, 12);

    moves_label = lv_label_create(scr);
    lv_obj_set_style_text_color(moves_label, lv_color_hex(0xA0A0A0), 0);
    lv_obj_set_pos(moves_label, 240, 50);

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x40FF80), 0);
    lv_obj_set_pos(status_label, 240, 100);

    lv_obj_set_size(btn, 72, 40);
    lv_obj_set_pos(btn, 240, 180);
    lv_obj_add_event_cb(btn, new_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn), "New");
    lv_obj_center(lv_obj_get_child(btn, 0));

    shuffle();
    show_state();
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));
    if (vnc_display_init(&argc, argv) != 0)
        return 1;
    build_ui();
    vnc_display_run();
    return 0;
}
