#include "ui2_2_lineset.h"
#include <stdio.h>
#include "keyboard.h"   // [NEW] 接入虚拟数字键盘 API

/* ===== 状态与对象 ===== */
static lv_obj_t *s_root = NULL;

static lv_obj_t *s_lbl_left  = NULL;
static lv_obj_t *s_lbl_right = NULL;
static lv_obj_t *s_cb_left   = NULL;
static lv_obj_t *s_cb_right  = NULL;
static lv_obj_t *s_btn_left  = NULL;
static lv_obj_t *s_btn_right = NULL;
static lv_obj_t *s_btn_exit  = NULL;

/* ===== 颜色与按钮样式 ===== */
#define COL_PANEL_BG 0xF5FAFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8
#define COL_TEXT     0xFFFFFF
#define COL_TITLE    0x333333

static void make_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa  (o, opa, 0);
    lv_obj_set_style_pad_all (o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static lv_obj_t* make_btn(int w, int h, const char *txt)
{
    lv_obj_t *b = lv_btn_create(s_root);
    // lv_obj_remove_style_all(b);   /* 按你当前项目习惯：保留主题，不清空全部样式 */
    lv_obj_set_size(b, w, h);

    lv_obj_set_style_bg_opa  (b, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x7A8CCF),   LV_PART_MAIN | LV_STATE_DISABLED);

    lv_obj_set_style_radius  (b, 14,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_shadow_width(b, 10, 0);
    lv_obj_set_style_shadow_opa (b, LV_OPA_20, 0);

    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt ? txt : "");
    lv_obj_set_style_text_color(lab, lv_color_hex(COL_TEXT), 0);
    lv_obj_center(lab);
    return b;
}

static void btn_set_text(lv_obj_t *btn, const char *txt){
    lv_obj_t *lab = lv_obj_get_child(btn, 0);
    if (lab) lv_label_set_text(lab, txt);
}
static void btn_set_center(lv_obj_t *btn, int cx, int cy){
    lv_obj_set_pos(btn, cx - lv_obj_get_width(btn)/2, cy - lv_obj_get_height(btn)/2);
}

/* ===== 布局默认（像素） ===== */
static int TITLE_Y = 56;
static int LEFT_TITLE_X   = 112, LEFT_CB_X  = 232;
static int RIGHT_TITLE_X  = 560, RIGHT_CB_X = 740;
static int COUNT_LEFT_CX  = 200, COUNT_RIGHT_CX = 600, COUNT_CY = 210;
static int EXIT_CX = 690, EXIT_CY = 332, EXIT_W = 190, EXIT_H = 60;
static int COUNT_W = 140, COUNT_H = 48;

/* 参数 */
static bool     s_left_en  = true;
static bool     s_right_en = true;
static uint16_t s_left_cnt = 6;
static uint16_t s_right_cnt= 6;
static uint16_t s_max_limit = 50;

/* 回调 */
static ui22_exit_cb_t  s_exit_cb = NULL;  static void *s_exit_ud = NULL;
static ui22_apply_cb_t s_apply_cb = NULL; static void *s_apply_ud = NULL;

/* 兼容：保留旧的外部键盘接口指针，但本文件已直接调用 keyboard.c */
static ui22_open_kb_fn s_open_kb  = NULL;

/* ===== 工具 ===== */
static void fmt_u16(char *buf, size_t n, uint16_t v){ snprintf(buf, n, "%u", (unsigned)v); }
static void apply_call(void){
    if (s_apply_cb) s_apply_cb(s_left_en, s_left_cnt, s_right_en, s_right_cnt, s_apply_ud);
}

/* ======= [NEW] 键盘 OK 桥接：把 keyboard 回调数值写回并刷新 ======= */
static void kb_left_ok(float v, void *ud)
{
    LV_UNUSED(ud);
    if (v < 0) v = 0;
    if (v > s_max_limit) v = s_max_limit;
    s_left_cnt = (uint16_t)v;
    char t[8]; fmt_u16(t, sizeof t, s_left_cnt);
    if (s_btn_left) btn_set_text(s_btn_left, t);
    apply_call();
}
static void kb_right_ok(float v, void *ud)
{
    LV_UNUSED(ud);
    if (v < 0) v = 0;
    if (v > s_max_limit) v = s_max_limit;
    s_right_cnt = (uint16_t)v;
    char t[8]; fmt_u16(t, sizeof t, s_right_cnt);
    if (s_btn_right) btn_set_text(s_btn_right, t);
    apply_call();
}

/* [NEW] keyboard.c 的回调签名桥接（text可忽略，float已解析好） */
static void kb_done_left_bridge (const char *text, float value, void *ud){ LV_UNUSED(text); kb_left_ok (value, ud); }
static void kb_done_right_bridge(const char *text, float value, void *ud){ LV_UNUSED(text); kb_right_ok(value, ud); }

/* ===== 事件 ===== */
static void on_cb_left(lv_event_t *e){
    if (e->code != LV_EVENT_VALUE_CHANGED) return;
    lv_state_t st = lv_obj_get_state(s_cb_left);
    s_left_en = (st & LV_STATE_CHECKED) != 0;
    apply_call();
}
static void on_cb_right(lv_event_t *e){
    if (e->code != LV_EVENT_VALUE_CHANGED) return;
    lv_state_t st = lv_obj_get_state(s_cb_right);
    s_right_en = (st & LV_STATE_CHECKED) != 0;
    apply_call();
}

/* [CHANGED] 点击“左数量”：改为使用 keyboard_show_digits_with_cb */
static void on_click_left_num(lv_event_t *e){
    LV_UNUSED(e);
    /* 仅数字；最大长度根据 s_max_limit 动态计算（1..5位常见够用） */
    int maxlen = 5;
    if (s_max_limit < 10) maxlen = 1;
    else if (s_max_limit < 100) maxlen = 2;
    else if (s_max_limit < 1000) maxlen = 3;
    else if (s_max_limit < 10000) maxlen = 4;

    keyboard_set_title("left count");          // [NEW]
    keyboard_set_max_len(maxlen);               // [NEW]
    char init[8]; fmt_u16(init, sizeof init, s_left_cnt);   // [NEW]
    keyboard_show_digits_with_cb(kb_done_left_bridge, NULL, init); // [NEW]
}

/* [CHANGED] 点击“右数量”：改为使用 keyboard_show_digits_with_cb */
static void on_click_right_num(lv_event_t *e){
    LV_UNUSED(e);
    int maxlen = 5;
    if (s_max_limit < 10) maxlen = 1;
    else if (s_max_limit < 100) maxlen = 2;
    else if (s_max_limit < 1000) maxlen = 3;
    else if (s_max_limit < 10000) maxlen = 4;

    keyboard_set_title("right count");         // [NEW]
    keyboard_set_max_len(maxlen);               // [NEW]
    char init[8]; fmt_u16(init, sizeof init, s_right_cnt);  // [NEW]
    keyboard_show_digits_with_cb(kb_done_right_bridge, NULL, init); // [NEW]
}

static void on_click_exit(lv_event_t *e){ LV_UNUSED(e); if (s_exit_cb) s_exit_cb(s_exit_ud); }

/* ===== 创建 UI ===== */
void ui22_lineset_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    make_plain(s_root, 800, 400, lv_color_hex(COL_PANEL_BG), LV_OPA_TRANSP);

    /* 左右标题 + 复选框 */
    s_lbl_left = lv_label_create(s_root);
    lv_obj_remove_style_all(s_lbl_left);
    lv_label_set_text(s_lbl_left, "left line");
    lv_obj_set_style_text_color(s_lbl_left, lv_color_hex(COL_TITLE), 0);
    lv_obj_align(s_lbl_left, LV_ALIGN_TOP_LEFT, LEFT_TITLE_X, TITLE_Y);

    s_cb_left = lv_checkbox_create(s_root);
    lv_obj_set_pos(s_cb_left, LEFT_CB_X, TITLE_Y);
    lv_checkbox_set_text(s_cb_left, ""); /* 不要文字 */
    if (s_left_en) lv_obj_add_state(s_cb_left, LV_STATE_CHECKED); else lv_obj_clear_state(s_cb_left, LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_cb_left, on_cb_left, LV_EVENT_VALUE_CHANGED, NULL);

    s_lbl_right = lv_label_create(s_root);
    lv_obj_remove_style_all(s_lbl_right);
    lv_label_set_text(s_lbl_right, "right line");
    lv_obj_set_style_text_color(s_lbl_right, lv_color_hex(COL_TITLE), 0);
    lv_obj_align(s_lbl_right, LV_ALIGN_TOP_LEFT, RIGHT_TITLE_X, TITLE_Y);

    s_cb_right = lv_checkbox_create(s_root);
    lv_obj_set_pos(s_cb_right, RIGHT_CB_X, TITLE_Y);
    lv_checkbox_set_text(s_cb_right, "");
    if (s_right_en) lv_obj_add_state(s_cb_right, LV_STATE_CHECKED); else lv_obj_clear_state(s_cb_right, LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_cb_right, on_cb_right, LV_EVENT_VALUE_CHANGED, NULL);

    /* 左右数量按钮 */
    char t[8];
    fmt_u16(t,sizeof t,s_left_cnt);
    s_btn_left = make_btn(COUNT_W, COUNT_H, t);
    btn_set_center(s_btn_left, COUNT_LEFT_CX, COUNT_CY);
    lv_obj_add_event_cb(s_btn_left, on_click_left_num, LV_EVENT_CLICKED, NULL);

    fmt_u16(t,sizeof t,s_right_cnt);
    s_btn_right = make_btn(COUNT_W, COUNT_H, t);
    btn_set_center(s_btn_right, COUNT_RIGHT_CX, COUNT_CY);
    lv_obj_add_event_cb(s_btn_right, on_click_right_num, LV_EVENT_CLICKED, NULL);

    /* 右下角退出 */
    s_btn_exit = make_btn(EXIT_W, EXIT_H, "exit");
    btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
    lv_obj_add_event_cb(s_btn_exit, on_click_exit, LV_EVENT_CLICKED, NULL);
}

void ui22_lineset_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }
    s_lbl_left = s_lbl_right = NULL;
    s_cb_left = s_cb_right = NULL;
    s_btn_left = s_btn_right = NULL;
    s_btn_exit = NULL;
}

/* ===== Root ===== */
lv_obj_t* ui22_root(void){ return s_root; }

/* ===== 回调绑定 ===== */
void ui22_set_exit_cb(ui22_exit_cb_t cb, void *user_data){ s_exit_cb = cb; s_exit_ud = user_data; }
void ui22_set_apply_cb(ui22_apply_cb_t cb, void *user_data){ s_apply_cb = cb; s_apply_ud = user_data; }

/* 兼容旧接口：仍然提供绑定函数，但本文件已直接调用 keyboard.c，不再依赖它 */
void ui22_bind_number_keyboard(ui22_open_kb_fn fn){ s_open_kb = fn; LV_UNUSED(s_open_kb); }  // [CHANGED] 兼容保留

/* ===== 参数存取 ===== */
void ui22_set_left_enabled(bool en){
    s_left_en = en;
    if (s_cb_left){
        if (en) lv_obj_add_state(s_cb_left, LV_STATE_CHECKED);
        else    lv_obj_clear_state(s_cb_left, LV_STATE_CHECKED);
    }
    apply_call();
}
bool ui22_get_left_enabled(void){ return s_left_en; }

void ui22_set_right_enabled(bool en){
    s_right_en = en;
    if (s_cb_right){
        if (en) lv_obj_add_state(s_cb_right, LV_STATE_CHECKED);
        else    lv_obj_clear_state(s_cb_right, LV_STATE_CHECKED);
    }
    apply_call();
}
bool ui22_get_right_enabled(void){ return s_right_en; }

void ui22_set_left_count(uint16_t n){
    if (n > s_max_limit) n = s_max_limit;
    s_left_cnt = n;
    if (s_btn_left){ char t[8]; fmt_u16(t,sizeof t,n); btn_set_text(s_btn_left,t); }
    apply_call();
}
uint16_t ui22_get_left_count(void){ return s_left_cnt; }

void ui22_set_right_count(uint16_t n){
    if (n > s_max_limit) n = s_max_limit;
    s_right_cnt = n;
    if (s_btn_right){ char t[8]; fmt_u16(t,sizeof t,n); btn_set_text(s_btn_right,t); }
    apply_call();
}
uint16_t ui22_get_right_count(void){ return s_right_cnt; }

void ui22_set_max_limit(uint16_t max_n){
    if (max_n == 0) max_n = 1;
    s_max_limit = max_n;
    if (s_left_cnt  > s_max_limit) ui22_set_left_count (s_max_limit);
    if (s_right_cnt > s_max_limit) ui22_set_right_count(s_max_limit);
}

/* ===== 布局接口 ===== */
void ui22_layout_reset_default(void){
    TITLE_Y = 56;
    LEFT_TITLE_X=112; LEFT_CB_X=232;
    RIGHT_TITLE_X=560; RIGHT_CB_X=740;
    COUNT_LEFT_CX=200; COUNT_RIGHT_CX=600; COUNT_CY=210;
    EXIT_CX=690; EXIT_CY=332; EXIT_W=190; EXIT_H=60;
    COUNT_W=140; COUNT_H=48;

    if (!s_root) return;
    /* 重新放置 */
    if (s_lbl_left)  lv_obj_align(s_lbl_left,  LV_ALIGN_TOP_LEFT, LEFT_TITLE_X, TITLE_Y);
    if (s_cb_left)   lv_obj_set_pos(s_cb_left, LEFT_CB_X, TITLE_Y);
    if (s_lbl_right) lv_obj_align(s_lbl_right, LV_ALIGN_TOP_LEFT, RIGHT_TITLE_X, TITLE_Y);
    if (s_cb_right)  lv_obj_set_pos(s_cb_right, RIGHT_CB_X, TITLE_Y);
    if (s_btn_left)  btn_set_center(s_btn_left,  COUNT_LEFT_CX,  COUNT_CY);
    if (s_btn_right) btn_set_center(s_btn_right, COUNT_RIGHT_CX, COUNT_CY);
    if (s_btn_exit)  btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
}
void ui22_set_titles_y(int y){
    TITLE_Y = y;
    if (s_lbl_left)  lv_obj_align(s_lbl_left,  LV_ALIGN_TOP_LEFT, LEFT_TITLE_X, TITLE_Y);
    if (s_cb_left)   lv_obj_set_pos(s_cb_left, LEFT_CB_X, TITLE_Y);
    if (s_lbl_right) lv_obj_align(s_lbl_right, LV_ALIGN_TOP_LEFT, RIGHT_TITLE_X, TITLE_Y);
    if (s_cb_right)  lv_obj_set_pos(s_cb_right, RIGHT_CB_X, TITLE_Y);
}
void ui22_set_left_title_x(int x_label, int x_cb){
    LEFT_TITLE_X = x_label; LEFT_CB_X = x_cb;
    if (s_lbl_left)  lv_obj_align(s_lbl_left,  LV_ALIGN_TOP_LEFT, LEFT_TITLE_X, TITLE_Y);
    if (s_cb_left)   lv_obj_set_pos(s_cb_left, LEFT_CB_X, TITLE_Y);
}
void ui22_set_right_title_x(int x_label, int x_cb){
    RIGHT_TITLE_X = x_label; RIGHT_CB_X = x_cb;
    if (s_lbl_right) lv_obj_align(s_lbl_right, LV_ALIGN_TOP_LEFT, RIGHT_TITLE_X, TITLE_Y);
    if (s_cb_right)  lv_obj_set_pos(s_cb_right, RIGHT_CB_X, TITLE_Y);
}
void ui22_set_count_btn_pos(int left_cx, int right_cx, int cy){
    COUNT_LEFT_CX = left_cx; COUNT_RIGHT_CX = right_cx; COUNT_CY = cy;
    if (s_btn_left)  btn_set_center(s_btn_left,  COUNT_LEFT_CX,  COUNT_CY);
    if (s_btn_right) btn_set_center(s_btn_right, COUNT_RIGHT_CX, COUNT_CY);
}
void ui22_set_exit_btn_pos_size(int cx, int cy, int w, int h){
    EXIT_CX=cx; EXIT_CY=cy; EXIT_W=w; EXIT_H=h;
    if (s_btn_exit){
        lv_obj_set_size(s_btn_exit, EXIT_W, EXIT_H);
        btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
    }
}
