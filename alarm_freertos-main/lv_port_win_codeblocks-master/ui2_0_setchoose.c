#include "ui2_0_setchoose.h"
#include <stdio.h>

/* 颜色 */
#define COL_BG   0xF5FAFF
#define COL_BTN  0x4D73C9
#define COL_PR   0x395AA8
#define COL_TXT  0xFFFFFF

/* 状态 */
static lv_obj_t *s_root = NULL;
static lv_obj_t *s_btn[6] = {0};

static ui20_select_cb_t s_on_select = NULL;
static void *s_on_ud = NULL;

/* 布局（默认） */
static int s_cx1 = 160, s_cx2 = 400, s_cx3 = 640;  /* 三列中心 X */
static int s_cy1 = 140, s_cy2 = 240;               /* 两行中心 Y */
static int s_bw  = 180, s_bh  = 60;                /* 按钮宽高 */

/* 工具 */
static void make_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa  (o, opa, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}
static lv_obj_t* make_btn(const char *txt)
{
    lv_obj_t *b = lv_btn_create(s_root);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, s_bw, s_bh);

    /* 背景色：给常见状态都设上，避免某些状态回退为白色 */
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN),
                              LV_PART_MAIN | LV_STATE_DEFAULT | LV_STATE_FOCUSED | LV_STATE_HOVERED);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_PR),
                              LV_PART_MAIN | LV_STATE_PRESSED | LV_STATE_CHECKED);
    /* 如果会有禁用态，也可单独设： */
    // lv_obj_set_style_bg_color(b, lv_color_hex(0x7A8CCF),
    //                           LV_PART_MAIN | LV_STATE_DISABLED);

    lv_obj_set_style_bg_opa  (b, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (b, 14,           LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(b, lv_color_hex(COL_TXT), 0);
    lv_obj_set_style_shadow_width(b, 10, 0);
    lv_obj_set_style_shadow_opa (b, LV_OPA_20, 0);

    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt ? txt : "");
    lv_obj_center(lab);
    return b;
}
static void set_center(lv_obj_t *o, int cx, int cy)
{
    lv_obj_set_pos(o, cx - s_bw/2, cy - s_bh/2);
}

/* 事件：六个按钮统一回调 */
static void on_click(lv_event_t *e)
{
    uintptr_t idx = (uintptr_t)lv_event_get_user_data(e); /* 1..6 */
    if (s_on_select) s_on_select((uint8_t)idx, s_on_ud);
}

/* 放置六个按钮 */
static void place_all(void)
{
    int cx[3] = {s_cx1, s_cx2, s_cx3};
    int cy[2] = {s_cy1, s_cy2};
    for (int i=0;i<6;++i){
        if (!s_btn[i]) continue;
        lv_obj_set_size(s_btn[i], s_bw, s_bh);
        set_center(s_btn[i], cx[i%3], cy[i/3]);
    }
}

/* ===== 对外接口 ===== */
void ui20_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    make_plain(s_root, 800, 400, lv_color_hex(COL_BG), LV_OPA_TRANSP);

    /* 默认按钮文本 */
    const char *names[6] = {
        "Calibrate", "Line set", "Manual",
        "Option4",   "Option5",  "Option6"
    };

    for (int i=0;i<6;++i){
        s_btn[i] = make_btn(names[i]);
        lv_obj_add_event_cb(s_btn[i], on_click, LV_EVENT_CLICKED, (void*)(uintptr_t)(i+1));
    }
    place_all();
}
void ui20_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }
    for (int i=0;i<6;++i) s_btn[i]=NULL;
}
lv_obj_t* ui20_root(void){ return s_root; }

void ui20_set_select_cb(ui20_select_cb_t cb, void *user_data){ s_on_select=cb; s_on_ud=user_data; }

void ui20_set_button_text(uint8_t idx, const char *txt)
{
    if (idx<1 || idx>6) return;
    lv_obj_t *lab = lv_obj_get_child(s_btn[idx-1], 0);
    if (lab) lv_label_set_text(lab, txt?txt:"");
}
void ui20_set_button_enabled(uint8_t idx, bool en)
{
    if (idx<1 || idx>6) return;
    if (!s_btn[idx-1]) return;
    if (en) {
        lv_obj_clear_state(s_btn[idx-1], LV_STATE_DISABLED);
        lv_obj_clear_flag (s_btn[idx-1], LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_state(s_btn[idx-1], LV_STATE_DISABLED);
        // 或选择隐藏：lv_obj_add_flag(s_btn[idx-1], LV_OBJ_FLAG_HIDDEN);
    }
}

/* 布局 */
void ui20_layout_reset_default(void){ s_cx1=160; s_cx2=400; s_cx3=640; s_cy1=140; s_cy2=240; s_bw=180; s_bh=60; if (s_root) place_all(); }
void ui20_set_cols(int cx1, int cx2, int cx3){ s_cx1=cx1; s_cx2=cx2; s_cx3=cx3; if (s_root) place_all(); }
void ui20_set_rows(int cy1, int cy2){ s_cy1=cy1; s_cy2=cy2; if (s_root) place_all(); }
void ui20_set_button_size(int w, int h){ s_bw=w; s_bh=h; if (s_root) place_all(); }
