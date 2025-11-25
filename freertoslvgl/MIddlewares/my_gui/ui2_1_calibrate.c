#include "ui2_1_calibrate.h"
#include <stdio.h>
#include "keyboard.h"     // [NEW] 直接接入虚拟数字键盘 API
#include <stdint.h>  /* 为了 uintptr_t */
/* ================== 固定颜色 ================== */
#define COL_PANEL_BG 0xF5FAFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8
#define COL_TEXT     0xFFFFFF
#define COL_TITLE    0x333333

/* ================== 默认布局（像素） ==================
   以 800×400 的父容器为参考，采用绝对定位并禁滚动。 */
static int TITLE_Y = 22;          /* 三列标题 Y */
static int ROW_Y0  = 92;          /* 第 1 行的 Y */
static int ROW_GAP = 70;          /* 行间距 */

static int LEFT_LABEL_X = 36;     /* 左列标签 X */
static int LEFT_BTN_X   = 72;     /* 左列按钮 X */

static int MID_TITLE_CX = 400;    /* 中列标题中心 X */
static int MID_BTN_CX   = 360;    /* 中列按钮中心 X */

static int RIGHT_TITLE_CX = 580;  /* 右列标题中心 X */
static int RIGHT_BTN_CX   = 620;  /* 右列按钮中心 X */

static int BTN_W = 136;           /* 小按钮 w */
static int BTN_H = 44;            /* 小按钮 h */
static int EXIT_W = 124;          /* 退出按钮 w */
static int EXIT_H = 60;           /* 退出按钮 h */
static int EXIT_CX = 632;         /* 退出按钮中心 X */
static int EXIT_CY = 314;         /* 退出按钮中心 Y */

/* ================== 模块状态 ================== */
static lv_obj_t *s_root = NULL;   /* 800×400 透明根容器 */

static lv_obj_t *s_lbl_left_title = NULL;
static lv_obj_t *s_lbl_mid_title  = NULL;
static lv_obj_t *s_lbl_rgt_title  = NULL;

/* 左列：3 数值 + 下拉 */
static lv_obj_t *s_btn_points  = NULL;
static lv_obj_t *s_btn_max     = NULL;
static lv_obj_t *s_btn_empty   = NULL;
static lv_obj_t *s_dd_bird     = NULL;

/* 中列：3 个“设置权重” */
static lv_obj_t *s_btn_set[3]  = {0};

/* 右列：3 个“校准 pointX” */
static lv_obj_t *s_btn_cal[3]  = {0};

/* 退出按钮 */
static lv_obj_t *s_btn_exit    = NULL;

/* 数据：左列 */
static uint8_t     s_points = 3;                   /* 1..3 */
static float       s_max_kg = 20.0f;
static float       s_empty_kg = 0.0f;
static ui21_bird_t s_bird = UI21_BIRD_HEN;

/* 数据：中列（对应每个 point 的“设置权重”） */
static float s_set_kg[3] = {0.0f, 10.0f, 0.0f};

/* 回调（对外） */
static ui21_exit_cb_t  s_exit_cb  = NULL; static void *s_exit_ud  = NULL;
static ui21_calib_cb_t s_calib_cb = NULL; static void *s_calib_ud = NULL;

/* 外部数字键盘绑定（旧接口保留字段，但本版已内建 keyboard 调用） */
static ui21_open_kb_fn s_open_kb = NULL;

/* ================== 样式对象（更稳，避免主题覆盖） ================== */
static lv_style_t st_btn_base, st_btn_pr, st_btn_dis;
static bool st_inited = false;

static void apply_points_visibility(void);


static void ensure_btn_styles(void)
{
    if (st_inited) return;
    st_inited = true;

    lv_style_init(&st_btn_base);
    lv_style_set_bg_color(&st_btn_base, lv_color_hex(COL_BTN));
    lv_style_set_bg_opa  (&st_btn_base, LV_OPA_COVER);
    lv_style_set_radius  (&st_btn_base, 14);
    lv_style_set_border_width(&st_btn_base, 0);
    lv_style_set_shadow_width(&st_btn_base, 10);
    lv_style_set_shadow_opa  (&st_btn_base, LV_OPA_20);

    lv_style_init(&st_btn_pr);
    lv_style_set_bg_color(&st_btn_pr, lv_color_hex(COL_BTN_PR));
    lv_style_set_bg_opa  (&st_btn_pr, LV_OPA_COVER);

    lv_style_init(&st_btn_dis);
    lv_style_set_bg_color(&st_btn_dis, lv_color_hex(0x7A8CCF));
    lv_style_set_bg_opa  (&st_btn_dis, LV_OPA_COVER);
}

/* ================== 工具 ================== */
static void make_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa(o,  opa, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static lv_obj_t* make_btn(int w, int h, const char *txt)
{
    ensure_btn_styles();

    lv_obj_t *b = lv_btn_create(s_root);
    // lv_obj_remove_style_all(b);      /* [保留注释] 你当前注释掉后可见主题；本版沿用你的做法 */
    lv_obj_set_size(b, w, h);

    /* 用样式对象 + selector 绑定到所有常见状态，防止回到主题灰白 */
    lv_obj_add_style(b, &st_btn_base, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(b, &st_btn_base, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(b, &st_btn_base, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_add_style(b, &st_btn_pr,   LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(b, &st_btn_pr,   LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_style(b, &st_btn_dis,  LV_PART_MAIN | LV_STATE_DISABLED);

    /* 文字颜色放在 label 上更保险 */
    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt ? txt : "");
    lv_obj_set_style_text_color(lab, lv_color_hex(COL_TEXT), 0);
    lv_obj_center(lab);

    /* 防止误处于禁用/编辑状态 */
    lv_obj_clear_state(b, LV_STATE_DISABLED | LV_STATE_EDITED);
    return b;
}

static void btn_set_text(lv_obj_t *btn, const char *txt)
{
    lv_obj_t *lab = lv_obj_get_child(btn, 0);
    if (lab) lv_label_set_text(lab, txt);
}
static void btn_set_center(lv_obj_t *btn, int cx, int cy)
{
    lv_obj_set_pos(btn, cx - lv_obj_get_width(btn)/2, cy - lv_obj_get_height(btn)/2);
}

static void label_title_at(lv_obj_t **out, const char *txt, int cx, int y)
{
    lv_obj_t *l = lv_label_create(s_root);
    lv_obj_remove_style_all(l);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TITLE), 0);
#if LV_FONT_MONTSERRAT_18
    lv_obj_set_style_text_font(l, &lv_font_montserrat_18, 0);
#endif
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, cx - lv_obj_get_width(l)/2, y);
    *out = l;
}

/* 格式化 */
static void fmt_int(char *buf, size_t n, int v){ snprintf(buf, n, "%d", v); }
static void fmt_kg (char *buf, size_t n, float v){ snprintf(buf, n, "%.0fkg", v); }
static void fmt_kg1(char *buf, size_t n, float v){ snprintf(buf, n, "%.1fkg", v); }


/* ====== [ADD] 键盘数值写回：内部处理函数（配合桥接回调使用） ====== */


static void kb_points_ok(float v, void *ud)
{
    LV_UNUSED(ud);
    if (v < 1) v = 1;
    if (v > 3) v = 3;
    s_points = (uint8_t)v;

    char t[16];
    fmt_int(t, sizeof t, s_points);
    if (s_btn_points) btn_set_text(s_btn_points, t);

    apply_points_visibility();
}

static void kb_max_ok(float v, void *ud)
{
    LV_UNUSED(ud);
    if (v < 0) v = 0;
    s_max_kg = v;

    char t[16];
    fmt_kg(t, sizeof t, s_max_kg);      /* 显示成 “20kg” */
    if (s_btn_max) btn_set_text(s_btn_max, t);
}

static void kb_empty_ok(float v, void *ud)
{
    LV_UNUSED(ud);
    if (v < 0) v = 0;
    s_empty_kg = v;

    char t[16];
    fmt_kg(t, sizeof t, s_empty_kg);
    if (s_btn_empty) btn_set_text(s_btn_empty, t);
}

static void kb_set_i_ok(float v, void *ud)
{
    uintptr_t idx = (uintptr_t)ud;
    if (idx >= 3) return;

    s_set_kg[idx] = v;
    char t[16];
    fmt_kg1(t, sizeof t, v);            /* 显示成 “10.0kg” */
    if (s_btn_set[idx]) btn_set_text(s_btn_set[idx], t);
}

/* ================== 键盘 OK 桥接回调（调用原有的 kb_*_ok） ================== */
// [NEW] 这几段把 keyboard.c 的回调结果桥接到你现有的 float 处理逻辑
static void kb_done_points_bridge(const char *text, float value, void *ud){ LV_UNUSED(text); kb_points_ok(value, ud); }
static void kb_done_max_bridge   (const char *text, float value, void *ud){ LV_UNUSED(text); kb_max_ok   (value, ud); }
static void kb_done_empty_bridge (const char *text, float value, void *ud){ LV_UNUSED(text); kb_empty_ok (value, ud); }
static void kb_done_set_i_bridge (const char *text, float value, void *ud){ LV_UNUSED(text); kb_set_i_ok (value, ud); }

/* 显隐 3 行（根据 s_points） */
static void apply_points_visibility(void)
{
    for (int i = 0; i < 3; ++i) {
        bool show = (i < s_points);
        if (s_btn_set[i]){
            if (show) lv_obj_clear_flag(s_btn_set[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (s_btn_set[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (s_btn_cal[i]){
            if (show) lv_obj_clear_flag(s_btn_cal[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (s_btn_cal[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ================== 点击事件：改成直接弹 keyboard ================== */
/* [CHANGED] 下面 6 个 on_click_* 事件，原来调用 s_open_kb(...)，现在统一用 keyboard_*. */
static void on_click_points(lv_event_t *e)
{
    /* points n：只要整数 1..3 → digits-only，长度 1~2（保险设 2） */
    keyboard_set_title("points n");          // [NEW]
    keyboard_set_max_len(2);                  // [NEW]
    char init[8]; fmt_int(init, sizeof init, s_points);  // [NEW]
    keyboard_show_digits_with_cb(kb_done_points_bridge, NULL, init);  // [NEW]
}

static void on_click_max(lv_event_t *e)
{
    /* max weight：带小数（比如 1 位） */
    keyboard_set_title("max weight(kg)");    // [NEW]
    keyboard_set_max_len(8);                  // [NEW]
    char init[16]; fmt_kg(init, sizeof init, s_max_kg);  // 如 "20kg"，也可只给数字
    /* 若你只想初值显示数字：snprintf(init, sizeof init, "%.1f", s_max_kg); */
    keyboard_show_with_cb(kb_done_max_bridge, NULL, init);  // [NEW]
}

static void on_click_empty(lv_event_t *e)
{
    keyboard_set_title("zero def(kg)");      // [NEW]
    keyboard_set_max_len(8);                  // [NEW]
    char init[16]; fmt_kg(init, sizeof init, s_empty_kg);
    keyboard_show_with_cb(kb_done_empty_bridge, NULL, init); // [NEW]
}

static void on_click_set0(lv_event_t *e)
{
    keyboard_set_title("setting weight(kg)");  // [NEW]
    keyboard_set_max_len(8);                   // [NEW]
    char init[16]; snprintf(init, sizeof init, "%.1f", s_set_kg[0]);  // [NEW]
    keyboard_show_with_cb(kb_done_set_i_bridge, (void*)(uintptr_t)0, init); // [NEW]
}
static void on_click_set1(lv_event_t *e)
{
    keyboard_set_title("setting weight(kg)");  // [NEW]
    keyboard_set_max_len(8);                   // [NEW]
    char init[16]; snprintf(init, sizeof init, "%.1f", s_set_kg[1]);  // [NEW]
    keyboard_show_with_cb(kb_done_set_i_bridge, (void*)(uintptr_t)1, init); // [NEW]
}
static void on_click_set2(lv_event_t *e)
{
    keyboard_set_title("setting weight(kg)");  // [NEW]
    keyboard_set_max_len(8);                   // [NEW]
    char init[16]; snprintf(init, sizeof init, "%.1f", s_set_kg[2]);  // [NEW]
    keyboard_show_with_cb(kb_done_set_i_bridge, (void*)(uintptr_t)2, init); // [NEW]
}

/* 校准按钮 */
static void on_click_cal0(lv_event_t *e){ LV_UNUSED(e); if (s_calib_cb) s_calib_cb(0, s_calib_ud); }
static void on_click_cal1(lv_event_t *e){ LV_UNUSED(e); if (s_calib_cb) s_calib_cb(1, s_calib_ud); }
static void on_click_cal2(lv_event_t *e){ LV_UNUSED(e); if (s_calib_cb) s_calib_cb(2, s_calib_ud); }

/* 退出 */
static void on_click_exit(lv_event_t *e){ LV_UNUSED(e); if (s_exit_cb) s_exit_cb(s_exit_ud); }

/* ================== 创建/销毁 ================== */
void ui21_calib_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    /* 根容器：占满 800×400，浅色背景，禁滚动，零内边距 */
    s_root = lv_obj_create(parent_800x400);

    // lv_obj_remove_style_all(s_root);               // [保留注释] 你的版本注释掉，保持生效外观
    lv_obj_set_size(s_root, 800, 400);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0xF2F6FB), 0);
    lv_obj_set_style_bg_opa  (s_root, LV_OPA_COVER, 0);

    lv_obj_set_style_pad_all   (s_root, 0, 0);
    lv_obj_set_style_pad_row   (s_root, 0, 0);
    lv_obj_set_style_pad_column(s_root, 0, 0);
    lv_obj_set_style_border_width (s_root, 0, 0);
    lv_obj_set_style_outline_width(s_root, 0, 0);

    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_root, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_root, LV_SCROLLBAR_MODE_OFF);

    /* 三列标题 */
    label_title_at(&s_lbl_left_title, "left settings", 160, TITLE_Y);
    label_title_at(&s_lbl_mid_title,  "setting weigh", MID_TITLE_CX, TITLE_Y);
    label_title_at(&s_lbl_rgt_title,  "calibrate point", RIGHT_TITLE_CX, TITLE_Y);

    /* 左列标签 */
    const char *cap[4] = {"points n", "max weight", "zero def", "types of"};
    for (int i=0;i<4;++i){
        lv_obj_t *lab = lv_label_create(s_root);
        lv_obj_remove_style_all(lab);
        lv_label_set_text(lab, cap[i]);
        lv_obj_set_style_text_color(lab, lv_color_hex(COL_TITLE), 0);
        lv_obj_align(lab, LV_ALIGN_TOP_LEFT, LEFT_LABEL_X, ROW_Y0 + i*ROW_GAP);
    }

    /* 左列按钮：points / max / empty */
    char tmp[16];
    fmt_int(tmp,sizeof tmp,s_points);
    s_btn_points = make_btn(BTN_W,BTN_H,tmp);
    btn_set_center(s_btn_points, LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 0*ROW_GAP);
    lv_obj_add_event_cb(s_btn_points, on_click_points, LV_EVENT_CLICKED, NULL);

    fmt_kg(tmp,sizeof tmp,s_max_kg);
    s_btn_max = make_btn(BTN_W,BTN_H,tmp);
    btn_set_center(s_btn_max, LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 1*ROW_GAP);
    lv_obj_add_event_cb(s_btn_max, on_click_max, LV_EVENT_CLICKED, NULL);

    fmt_kg(tmp,sizeof tmp,s_empty_kg);
    s_btn_empty = make_btn(BTN_W,BTN_H,tmp);
    btn_set_center(s_btn_empty, LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 2*ROW_GAP);
    lv_obj_add_event_cb(s_btn_empty, on_click_empty, LV_EVENT_CLICKED, NULL);

    /* 左列下拉：鸡只类型 */
    s_dd_bird = lv_dropdown_create(s_root);
    lv_dropdown_set_options(s_dd_bird, "hens\nroosters");
    lv_dropdown_set_selected(s_dd_bird, (s_bird==UI21_BIRD_HEN)?0:1);
    lv_obj_align(s_dd_bird, LV_ALIGN_TOP_LEFT, LEFT_BTN_X, ROW_Y0 + 3*ROW_GAP);
    lv_obj_set_style_pad_hor(s_dd_bird, 12, 0);
    lv_obj_set_style_radius  (s_dd_bird, 12, 0);

    /* 中列：3 个“设置权重”按钮 */
    for (int i=0;i<3;++i){
        char t[16]; fmt_kg1(t,sizeof t,s_set_kg[i]);
        s_btn_set[i] = make_btn(BTN_W, BTN_H, t);
        btn_set_center(s_btn_set[i], MID_BTN_CX, ROW_Y0 + BTN_H/2 + i*ROW_GAP);
    }
    lv_obj_add_event_cb(s_btn_set[0], on_click_set0, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_set[1], on_click_set1, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_set[2], on_click_set2, LV_EVENT_CLICKED, NULL);

    /* 右列：3 个“校准”按钮 */
    s_btn_cal[0] = make_btn(BTN_W, BTN_H, "point1");
    s_btn_cal[1] = make_btn(BTN_W, BTN_H, "point2");
    s_btn_cal[2] = make_btn(BTN_W, BTN_H, "point3");
    for (int i=0;i<3;++i){
        btn_set_center(s_btn_cal[i], RIGHT_BTN_CX, ROW_Y0 + BTN_H/2 + i*ROW_GAP);
    }
    lv_obj_add_event_cb(s_btn_cal[0], on_click_cal0, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_cal[1], on_click_cal1, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_cal[2], on_click_cal2, LV_EVENT_CLICKED, NULL);

    /* 右下角退出 */
    s_btn_exit = make_btn(EXIT_W, EXIT_H, "exit");
    btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
    lv_obj_add_event_cb(s_btn_exit, on_click_exit, LV_EVENT_CLICKED, NULL);

    apply_points_visibility();
}

void ui21_calib_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }
    s_lbl_left_title = s_lbl_mid_title = s_lbl_rgt_title = NULL;
    s_btn_points = s_btn_max = s_btn_empty = NULL;
    s_dd_bird = NULL;
    for (int i=0;i<3;++i) s_btn_set[i]=s_btn_cal[i]=NULL;
    s_btn_exit = NULL;
}

/* ================== 回调/绑定 ================== */
void ui21_set_exit_cb(ui21_exit_cb_t cb, void *user_data){ s_exit_cb = cb; s_exit_ud = user_data; }
void ui21_set_calib_cb(ui21_calib_cb_t cb, void *user_data){ s_calib_cb = cb; s_calib_ud = user_data; }
void ui21_bind_number_keyboard(ui21_open_kb_fn fn){ s_open_kb = fn; }  /* [保留旧接口] 兼容你的历史代码 */

/* ================== Getter / Setter ================== */
void    ui21_set_points(uint8_t n){ if (n<1) n=1; if (n>3) n=3; s_points=n; if (s_btn_points){char t[16];fmt_int(t,sizeof t,n);btn_set_text(s_btn_points,t);} apply_points_visibility(); }
uint8_t ui21_get_points(void){ return s_points; }

void  ui21_set_max_weight(float kg){ if (kg<0) kg=0; s_max_kg=kg; if (s_btn_max){char t[16];fmt_kg(t,sizeof t,kg);btn_set_text(s_btn_max,t);} }
float ui21_get_max_weight(void){ return s_max_kg; }

void  ui21_set_empty_weight(float kg){ if (kg<0) kg=0; s_empty_kg=kg; if (s_btn_empty){char t[16];fmt_kg(t,sizeof t,kg);btn_set_text(s_btn_empty,t);} }
float ui21_get_empty_weight(void){ return s_empty_kg; }

void        ui21_set_bird(ui21_bird_t t){ s_bird=t; if (s_dd_bird) lv_dropdown_set_selected(s_dd_bird,(t==UI21_BIRD_HEN)?0:1); }
ui21_bird_t ui21_get_bird(void){ if (s_dd_bird){ int sel = lv_dropdown_get_selected(s_dd_bird); return (sel==0)?UI21_BIRD_HEN:UI21_BIRD_ROOSTER; } return s_bird; }

void  ui21_set_point_weight(uint8_t idx, float kg){ if (idx>=3) return; s_set_kg[idx]=kg; if (s_btn_set[idx]){char t[16];fmt_kg1(t,sizeof t,kg);btn_set_text(s_btn_set[idx],t);} }
float ui21_get_point_weight(uint8_t idx){ return (idx<3)? s_set_kg[idx] : 0.0f; }

/* ================== 可选：布局调整 ================== */
void ui21_set_layout_defaults(void)
{
    TITLE_Y=22; ROW_Y0=92; ROW_GAP=70;
    LEFT_LABEL_X=56; LEFT_BTN_X=112;
    MID_TITLE_CX=400; MID_BTN_CX=360;
    RIGHT_TITLE_CX=620; RIGHT_BTN_CX=620;
    BTN_W=136; BTN_H=44; EXIT_W=184; EXIT_H=60; EXIT_CX=692; EXIT_CY=344;

    if (!s_root) return;
    /* 标题 */
    if (s_lbl_left_title) lv_obj_align(s_lbl_left_title, LV_ALIGN_TOP_LEFT, 160 - lv_obj_get_width(s_lbl_left_title)/2, TITLE_Y);
    if (s_lbl_mid_title)  lv_obj_align(s_lbl_mid_title,  LV_ALIGN_TOP_LEFT, MID_TITLE_CX - lv_obj_get_width(s_lbl_mid_title)/2,  TITLE_Y);
    if (s_lbl_rgt_title)  lv_obj_align(s_lbl_rgt_title,  LV_ALIGN_TOP_LEFT, RIGHT_TITLE_CX - lv_obj_get_width(s_lbl_rgt_title)/2, TITLE_Y);
    /* 左列按钮/下拉 */
    if (s_btn_points) btn_set_center(s_btn_points, LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 0*ROW_GAP);
    if (s_btn_max)    btn_set_center(s_btn_max,    LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 1*ROW_GAP);
    if (s_btn_empty)  btn_set_center(s_btn_empty,  LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 2*ROW_GAP);
    if (s_dd_bird)    lv_obj_set_pos(s_dd_bird, LEFT_BTN_X, ROW_Y0 + 3*ROW_GAP);
    /* 中列 */
    for (int i=0;i<3;++i) if (s_btn_set[i]) btn_set_center(s_btn_set[i], MID_BTN_CX, ROW_Y0 + BTN_H/2 + i*ROW_GAP);
    /* 右列 */
    for (int i=0;i<3;++i) if (s_btn_cal[i]) btn_set_center(s_btn_cal[i], RIGHT_BTN_CX, ROW_Y0 + BTN_H/2 + i*ROW_GAP);
    /* 退出 */
    if (s_btn_exit) btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
}
void ui21_set_titles_y(int y){ TITLE_Y=y; if (s_lbl_left_title) lv_obj_align(s_lbl_left_title, LV_ALIGN_TOP_LEFT, 160 - lv_obj_get_width(s_lbl_left_title)/2, TITLE_Y); if (s_lbl_mid_title) lv_obj_align(s_lbl_mid_title, LV_ALIGN_TOP_LEFT, MID_TITLE_CX - lv_obj_get_width(s_lbl_mid_title)/2, TITLE_Y); if (s_lbl_rgt_title) lv_obj_align(s_lbl_rgt_title, LV_ALIGN_TOP_LEFT, RIGHT_TITLE_CX - lv_obj_get_width(s_lbl_rgt_title)/2, TITLE_Y); }
void ui21_set_rows_y(int first_y, int row_gap)
{
    ROW_Y0 = first_y; ROW_GAP = row_gap;
    if (!s_root) return;
    if (s_btn_points) btn_set_center(s_btn_points, LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 0*ROW_GAP);
    if (s_btn_max)    btn_set_center(s_btn_max,    LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 1*ROW_GAP);
    if (s_btn_empty)  btn_set_center(s_btn_empty,  LEFT_BTN_X + BTN_W/2, ROW_Y0 + BTN_H/2 + 2*ROW_GAP);
    if (s_dd_bird)    lv_obj_set_pos(s_dd_bird, LEFT_BTN_X, ROW_Y0 + 3*ROW_GAP);
    for (int i=0;i<3;++i){
        if (s_btn_set[i]) btn_set_center(s_btn_set[i], MID_BTN_CX,   ROW_Y0 + BTN_H/2 + i*ROW_GAP);
        if (s_btn_cal[i]) btn_set_center(s_btn_cal[i], RIGHT_BTN_CX, ROW_Y0 + BTN_H/2 + i*ROW_GAP);
    }
    if (s_btn_exit) btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
}
