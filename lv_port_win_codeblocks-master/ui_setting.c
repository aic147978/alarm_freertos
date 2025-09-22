#include "ui_setting.h"
#include "ui_keypad.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>

/* ===================== 布局参数（针对中心区 ~800x400） ===================== */

/* 整个页面内容与四周的留白（内边距） */
#define MARGIN_X        8
#define MARGIN_Y        8

/* 主菜单页大按钮（tile）的外观与尺寸 */
#define TILE_W          160
#define TILE_H          120
#define TILE_RADIUS     28
#define GAP_X           80
#define GAP_Y           70

/* 常规小按钮（数值框、point1/point2 等）尺寸 */
#define BTN_W           100
#define BTN_H           36

/* 不同设置页中的“行距”（控制纵向密度） */
#define ROW_H           72

/* 三列的“绝对 X 坐标”（相对本页根容器左上角） */
#define COL_LEFT_X      80
#define COL_MID_X       320
#define COL_RIGHT_X     560

/* ===================== 内部状态（页面枚举与根节点们） ===================== */
typedef enum {
    PAGE_MENU = 0,
    PAGE_CALIBRATE,
    PAGE_LINE,
    PAGE_MANUAL,
    PAGE_ALARM,
    PAGE_FACTORY,
    PAGE_INFO,
} PageId;

static lv_obj_t *s_root = NULL;         /* 本模块根容器（挂在你传入的 parent 上） */
static lv_obj_t *s_menu = NULL;
static lv_obj_t *s_calib = NULL;
static lv_obj_t *s_line  = NULL;        /* ← 实体化的 line setting 子页 */
static lv_obj_t *s_stub_manual  = NULL;
static lv_obj_t *s_stub_alarm   = NULL;
static lv_obj_t *s_stub_factory = NULL;
static lv_obj_t *s_stub_info    = NULL;

static PageId s_page = PAGE_MENU;

/* ===================== 校准页：模型与对象 ===================== */
static CalibConfig g_cfg = {
    .point_count   = 3,
    .max_weight_kg = 20,
    .zero_drift_kg = 0,
    .ck_type       = 0,
    .setpoint_kg   = {0, 10, 0, 0}
};

static lv_obj_t *s_lbl_points = NULL;
static lv_obj_t *s_lbl_maxkg  = NULL;
static lv_obj_t *s_lbl_zero   = NULL;
static lv_obj_t *s_dd_ck      = NULL;
static lv_obj_t *s_right_panel = NULL;      /* 右侧行容器（动态重建） */
static lv_obj_t *s_btn_exit    = NULL;

static ui_bind_entry_t s_entry_points = {0};
static ui_bind_entry_t s_entry_maxkg  = {0};
static ui_bind_entry_t s_entry_zero   = {0};
static ui_bind_entry_t s_entry_setpoints[4] = {0};

/* ===================== Line setting 子页：对象 & 模型 ===================== */
static lv_obj_t *s_line_lbl_left  = NULL;
static lv_obj_t *s_line_lbl_right = NULL;
static lv_obj_t *s_line_cb_left   = NULL;
static lv_obj_t *s_line_cb_right  = NULL;

static lv_obj_t *s_line_btn_left  = NULL;
static lv_obj_t *s_line_btn_right = NULL;
static lv_obj_t *s_line_lab_left  = NULL;
static lv_obj_t *s_line_lab_right = NULL;
static lv_obj_t *s_line_btn_exit  = NULL;

static ui_bind_entry_t s_line_entry_left  = {0};
static ui_bind_entry_t s_line_entry_right = {0};

static bool s_line_left_enabled  = false;
static bool s_line_right_enabled = true;
static int  s_line_left_count    = 6;
static int  s_line_right_count   = 6;

#define LINE_COUNTER_DIGITS  2
#define LINE_BTN_Y           (30 + ROW_H * 2)

/* ===================== 样式与通用工具 ===================== */

static void strip_panel_look(lv_obj_t *o){
    if(!o) return;
    lv_obj_set_style_bg_opa       (o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width (o, 0,            LV_PART_MAIN);
    lv_obj_set_style_outline_width(o, 0,            LV_PART_MAIN);
    lv_obj_set_style_shadow_width (o, 0,            LV_PART_MAIN);
    lv_obj_set_style_radius       (o, 0,            LV_PART_MAIN);
    lv_obj_set_style_pad_all      (o, 0,            LV_PART_MAIN);
}

static lv_style_t st_tile, st_tile_label;
static bool st_inited = false;

static void ensure_styles(void){
    if(st_inited) return;
    st_inited = true;

    lv_style_init(&st_tile);
    lv_style_set_bg_color (&st_tile, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa   (&st_tile, LV_OPA_COVER);
    lv_style_set_radius   (&st_tile, TILE_RADIUS);
    lv_style_set_border_width(&st_tile, 0);
    lv_style_set_shadow_width(&st_tile, 10);
    lv_style_set_shadow_opa  (&st_tile, LV_OPA_40);
    lv_style_set_pad_all  (&st_tile, 8);

    lv_style_init(&st_tile_label);
    lv_style_set_text_color (&st_tile_label, lv_color_white());
    lv_style_set_text_align (&st_tile_label, LV_TEXT_ALIGN_CENTER);
}

static void no_scroll(lv_obj_t *o){
    if(!o) return;
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

/* 大按钮（tile） */
static lv_obj_t* make_tile(lv_obj_t *parent, const char *txt,
                           lv_event_cb_t cb, lv_align_t align,
                           lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    ensure_styles();
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, TILE_W, TILE_H);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_style(btn, &st_tile, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    lv_obj_add_style(lab, &st_tile_label, 0);

    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* ===================== 主菜单页 ===================== */

static void cb_show_calib (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_calibrate(); }
static void cb_show_line  (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_line_setting(); }
static void cb_show_manual(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_manual(); }
static void cb_show_alarm (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_alarm_setting(); }
static void cb_show_factory(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_factory_reset(); }
static void cb_show_info  (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_machine_info(); }

static void build_menu(lv_obj_t *parent)
{
    s_menu = lv_obj_create(parent);
    no_scroll(s_menu);
    strip_panel_look(s_menu);
    lv_obj_set_size(s_menu, LV_PCT(100), LV_PCT(135));
    lv_obj_align(s_menu, LV_ALIGN_TOP_LEFT, 0, -20);

    /* 六个大按钮 */
    make_tile(s_menu, "calibrate",            cb_show_calib,   LV_ALIGN_TOP_LEFT,     MARGIN_X, 40);
    make_tile(s_menu, "line setting",         cb_show_line,    LV_ALIGN_TOP_MID,      0,        40);
    make_tile(s_menu, "manual",               cb_show_manual,  LV_ALIGN_TOP_RIGHT,   -MARGIN_X, 40);

    make_tile(s_menu, "alarm setting",        cb_show_alarm,   LV_ALIGN_BOTTOM_LEFT,  MARGIN_X,  -120);
    make_tile(s_menu, "factory data reset",   cb_show_factory, LV_ALIGN_BOTTOM_MID,   0,         -120);
    make_tile(s_menu, "machine\ninformation", cb_show_info,    LV_ALIGN_BOTTOM_RIGHT,-MARGIN_X,  -120);
}

/* ===================== 校准页 ===================== */

static void rebuild_right_rows(void); /* 前置声明 */

static void ev_ck_changed(lv_event_t *e){
    LV_UNUSED(e);
    g_cfg.ck_type = lv_dropdown_get_selected(s_dd_ck); /* 0 hens / 1 cocks */
}

static void ev_exit(lv_event_t *e){
    LV_UNUSED(e);
    settings_ui_show_menu();
}

static lv_obj_t* make_value_btn(lv_obj_t *parent, const char *txt, lv_obj_t **label_out)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BTN_W, BTN_H);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    if(label_out) *label_out = lab;
    return btn;
}

static void refresh_left_labels(void)
{
    ui_keypad_refresh_entry(&s_entry_points);
    ui_keypad_refresh_entry(&s_entry_maxkg);
    ui_keypad_refresh_entry(&s_entry_zero);
    lv_dropdown_set_selected(s_dd_ck, g_cfg.ck_type);
}

static void ev_do_calibrate(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(settings_on_calibrate_start) settings_on_calibrate_start((uint8_t)idx, g_cfg.setpoint_kg[idx]);
}

/* 右侧“setting weigh / calibrate point”动态重建 */
static void rebuild_right_rows(void)
{
    if(s_right_panel) lv_obj_del(s_right_panel);

    s_right_panel = lv_obj_create(s_calib);
    no_scroll(s_right_panel);
    strip_panel_look(s_right_panel);
    lv_obj_set_size(s_right_panel, 320, LV_PCT(100));
    lv_obj_align(s_right_panel, LV_ALIGN_TOP_LEFT, COL_MID_X-20, 0);
    lv_obj_set_style_pad_top   (s_right_panel, MARGIN_Y, 0);
    lv_obj_set_style_pad_bottom(s_right_panel, MARGIN_Y, 0);

    /* 标题（左半） */
    lv_obj_t *t1 = lv_label_create(s_right_panel);
    lv_label_set_text(t1, "setting weigh");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 10, 0);

    /* 标题（右列） */
    lv_obj_t *t2 = lv_label_create(s_calib);
    lv_label_set_text(t2, "calibrate point");
    lv_obj_align(t2, LV_ALIGN_TOP_LEFT, COL_RIGHT_X-20, MARGIN_Y);

    /* 行：设定重量按钮 + 对应 pointX 按钮 */
    uint8_t n = g_cfg.point_count; if(n<1) n=1; if(n>4) n=4;

    for(uint8_t i=0;i<n;i++){
        int y = MARGIN_Y + 40 + i*ROW_H;

        /* 左边设置重量（在 right_panel 里） */
        lv_obj_t *btn_set = lv_btn_create(s_right_panel);
        lv_obj_set_size(btn_set, BTN_W, BTN_H);
        lv_obj_align(btn_set, LV_ALIGN_TOP_LEFT, 10, 40 + i*ROW_H);
        lv_obj_t *label = lv_label_create(btn_set);
        ui_keypad_entry_init(&s_entry_setpoints[i], label, &g_cfg.setpoint_kg[i], "kg", false, 3);
        ui_keypad_bind_button(btn_set, &s_entry_setpoints[i]);

        /* 右边“pointX”按钮（挂在 s_calib，位置靠右列） */
        lv_obj_t *btn_point = lv_btn_create(s_calib);
        lv_obj_set_size(btn_point, BTN_W, BTN_H);
        lv_obj_align(btn_point, LV_ALIGN_TOP_LEFT, COL_RIGHT_X, y);
        lv_obj_add_event_cb(btn_point, ev_do_calibrate, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        char ptxt[24]; lv_snprintf(ptxt,sizeof(ptxt),"point%d", i+1);
        lv_label_set_text(lv_label_create(btn_point), ptxt);
    }

    for(uint8_t i=n; i<4; ++i){
        s_entry_setpoints[i].label = NULL;
    }
}

static void build_calibrate(lv_obj_t *parent)
{
    s_calib = lv_obj_create(parent);
    no_scroll(s_calib);
    strip_panel_look(s_calib);
    lv_obj_set_size(s_calib, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_calib, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 左列标签与按钮 */
    lv_obj_t *l1 = lv_label_create(s_calib); lv_label_set_text(l1, "points num");
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40);
    lv_obj_t *btn_points = make_value_btn(s_calib, "2", &s_lbl_points);
    lv_obj_align(btn_points, LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30);
    ui_keypad_entry_init(&s_entry_points, s_lbl_points, (int*)&g_cfg.point_count, NULL, false, 1);
    ui_keypad_bind_button(btn_points, &s_entry_points);

    lv_obj_t *l2 = lv_label_create(s_calib); lv_label_set_text(l2, "max weigh");
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40+ROW_H*1);
    lv_obj_t *btn_max = make_value_btn(s_calib, "20kg", &s_lbl_maxkg);
    lv_obj_align(btn_max, LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30+ROW_H*1);
    ui_keypad_entry_init(&s_entry_maxkg, s_lbl_maxkg, &g_cfg.max_weight_kg, "kg", false, 3);
    ui_keypad_bind_button(btn_max, &s_entry_maxkg);

    lv_obj_t *l3 = lv_label_create(s_calib); lv_label_set_text(l3, "zero defit");
    lv_obj_align(l3, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40+ROW_H*2);
    lv_obj_t *btn_zero = make_value_btn(s_calib, "0kg", &s_lbl_zero);
    lv_obj_align(btn_zero, LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30+ROW_H*2);
    ui_keypad_entry_init(&s_entry_zero, s_lbl_zero, &g_cfg.zero_drift_kg, "kg", false, 3);
    ui_keypad_bind_button(btn_zero, &s_entry_zero);

    lv_obj_t *l4 = lv_label_create(s_calib); lv_label_set_text(l4, "types of ck");
    lv_obj_align(l4, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40+ROW_H*3);
    s_dd_ck = lv_dropdown_create(s_calib);
    lv_dropdown_set_options_static(s_dd_ck, "hens\ncocks");
    lv_obj_set_size(s_dd_ck, BTN_W, BTN_H);
    lv_obj_align(s_dd_ck, LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30+ROW_H*3);
    lv_obj_add_event_cb(s_dd_ck, ev_ck_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* 右下角 Exit */
    s_btn_exit = lv_btn_create(s_calib);
    lv_obj_set_size(s_btn_exit, 180, 56);
    lv_obj_align(s_btn_exit, LV_ALIGN_BOTTOM_RIGHT, -24, -16);
    lv_obj_add_event_cb(s_btn_exit, ev_exit, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_exit), "exit");

    refresh_left_labels();
    rebuild_right_rows();
}

/* ===================== Line setting 子页 ===================== */

static void line_set_btn_enabled(lv_obj_t *btn, bool en){
    if(!btn) return;
    if(en) {
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn, LV_OPA_COVER, 0);
    } else {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn, LV_OPA_40, 0);
    }
}

static void line_refresh(void)
{
    ui_keypad_refresh_entry(&s_line_entry_left);
    ui_keypad_refresh_entry(&s_line_entry_right);

    if(s_line_cb_left) {
        if(s_line_left_enabled)  lv_obj_add_state(s_line_cb_left, LV_STATE_CHECKED);
        else                      lv_obj_clear_state(s_line_cb_left, LV_STATE_CHECKED);
    }
    if(s_line_cb_right) {
        if(s_line_right_enabled) lv_obj_add_state(s_line_cb_right, LV_STATE_CHECKED);
        else                      lv_obj_clear_state(s_line_cb_right, LV_STATE_CHECKED);
    }

    line_set_btn_enabled(s_line_btn_left,  s_line_left_enabled);
    line_set_btn_enabled(s_line_btn_right, s_line_right_enabled);
}

static void line_ev_cb_changed(lv_event_t *e){
    lv_obj_t *cb = lv_event_get_target(e);
    bool chk = lv_obj_has_state(cb, LV_STATE_CHECKED);
    if(cb == s_line_cb_left)  s_line_left_enabled  = chk;
    if(cb == s_line_cb_right) s_line_right_enabled = chk;
    line_refresh();
}

static void line_ev_exit(lv_event_t *e){
    LV_UNUSED(e);
    settings_ui_show_menu();
}

static void build_line(lv_obj_t *parent)
{
    s_line = lv_obj_create(parent);
    no_scroll(s_line);
    strip_panel_look(s_line);
    lv_obj_set_size(s_line, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_line, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 标题 + 复选框（左） */
    s_line_lbl_left = lv_label_create(s_line);
    lv_label_set_text(s_line_lbl_left, "left line");
    lv_obj_align(s_line_lbl_left, LV_ALIGN_TOP_LEFT, COL_LEFT_X, 40);

    s_line_cb_left = lv_checkbox_create(s_line);
    lv_checkbox_set_text(s_line_cb_left, "");
    lv_obj_align(s_line_cb_left, LV_ALIGN_TOP_LEFT, COL_LEFT_X + 120, 38);
    lv_obj_add_event_cb(s_line_cb_left, line_ev_cb_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* 标题 + 复选框（右） */
    s_line_lbl_right = lv_label_create(s_line);
    lv_label_set_text(s_line_lbl_right, "right line");
    lv_obj_align(s_line_lbl_right, LV_ALIGN_TOP_LEFT, COL_RIGHT_X, 40);

    s_line_cb_right = lv_checkbox_create(s_line);
    lv_checkbox_set_text(s_line_cb_right, "");
    lv_obj_align(s_line_cb_right, LV_ALIGN_TOP_LEFT, COL_RIGHT_X + 120, 38);
    lv_obj_add_event_cb(s_line_cb_right, line_ev_cb_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* 数量按钮（左） */
    s_line_btn_left = lv_btn_create(s_line);
    lv_obj_set_size(s_line_btn_left, BTN_W + 40, BTN_H + 8);
    lv_obj_align(s_line_btn_left, LV_ALIGN_TOP_LEFT, COL_LEFT_X + 20, LINE_BTN_Y);
    s_line_lab_left = lv_label_create(s_line_btn_left);
    lv_obj_center(s_line_lab_left);
    ui_keypad_entry_init(&s_line_entry_left, s_line_lab_left, &s_line_left_count, NULL, false, LINE_COUNTER_DIGITS);
    ui_keypad_bind_button(s_line_btn_left, &s_line_entry_left);

    /* 数量按钮（右） */
    s_line_btn_right = lv_btn_create(s_line);
    lv_obj_set_size(s_line_btn_right, BTN_W + 40, BTN_H + 8);
    lv_obj_align(s_line_btn_right, LV_ALIGN_TOP_LEFT, COL_RIGHT_X + 20, LINE_BTN_Y);
    s_line_lab_right = lv_label_create(s_line_btn_right);
    lv_obj_center(s_line_lab_right);
    ui_keypad_entry_init(&s_line_entry_right, s_line_lab_right, &s_line_right_count, NULL, false, LINE_COUNTER_DIGITS);
    ui_keypad_bind_button(s_line_btn_right, &s_line_entry_right);

    /* 右下角 exit */
    s_line_btn_exit = lv_btn_create(s_line);
    lv_obj_set_size(s_line_btn_exit, 220, 64);
    lv_obj_align(s_line_btn_exit, LV_ALIGN_BOTTOM_RIGHT, -24, -16);
    lv_obj_add_event_cb(s_line_btn_exit, line_ev_exit, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_line_btn_exit), "exit");

    line_refresh();
}

/* ===================== 其它占位页（保持不变） ===================== */

static lv_obj_t* build_stub(lv_obj_t *parent, const char *title)
{
    lv_obj_t *box = lv_obj_create(parent);
    no_scroll(box);
    strip_panel_look(box);
    lv_obj_set_size(box, LV_PCT(100), LV_PCT(100));
    lv_obj_align(box, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lab = lv_label_create(box);
    lv_label_set_text_fmt(lab, "%s\n(TBD)", title);
    lv_obj_align(lab, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *back = lv_btn_create(box);
    lv_obj_set_size(back, 140, 48);
    lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -24, -16);
    lv_obj_add_event_cb(back, ev_exit, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(back), "back");
    return box;
}

/* ===================== 创建 / 销毁 ===================== */

void settings_ui_create(lv_obj_t *parent)
{
    if(s_root) return;

    s_root = lv_obj_create(parent);
    no_scroll(s_root);
    strip_panel_look(s_root);
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);

    build_menu(s_root);
    build_calibrate(s_root);
    build_line(s_root);                      /* ← 用真实的 line 子页 */
    s_stub_manual  = build_stub(s_root, "manual");
    s_stub_alarm   = build_stub(s_root, "alarm setting");
    s_stub_factory = build_stub(s_root, "factory data reset");
    s_stub_info    = build_stub(s_root, "machine information");

    /* 初始：只显示主菜单 */
    lv_obj_add_flag(s_calib,        LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_line,         LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_manual,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_alarm,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_info,    LV_OBJ_FLAG_HIDDEN);
    s_page = PAGE_MENU;
}

void settings_ui_destroy(void)
{
    ui_keypad_close();

    if(s_root){ lv_obj_del(s_root); s_root = NULL; }

    /* 根下属页面指针 */
    s_menu = s_calib = s_line = s_stub_manual = s_stub_alarm = s_stub_factory = s_stub_info = NULL;

    /* 校准页对象指针与绑定清理 */
    s_lbl_points = s_lbl_maxkg = s_lbl_zero = NULL;
    s_dd_ck = NULL;
    s_right_panel = NULL;
    s_btn_exit = NULL;
    memset(&s_entry_points, 0, sizeof(s_entry_points));
    memset(&s_entry_maxkg,  0, sizeof(s_entry_maxkg));
    memset(&s_entry_zero,   0, sizeof(s_entry_zero));
    memset(s_entry_setpoints, 0, sizeof(s_entry_setpoints));

    /* line 子页对象指针与绑定清理（关键） */
    s_line_lbl_left = s_line_lbl_right = NULL;
    s_line_cb_left = s_line_cb_right = NULL;
    s_line_btn_left = s_line_btn_right = NULL;
    s_line_lab_left = s_line_lab_right = NULL;
    s_line_btn_exit = NULL;
    memset(&s_line_entry_left,  0, sizeof(s_line_entry_left));
    memset(&s_line_entry_right, 0, sizeof(s_line_entry_right));

    s_page = PAGE_MENU;
}

/* ===================== 显示切换 ===================== */

static void show_only(PageId p)
{
    if(!s_root) return;

    /* 全部隐藏 */
    if(s_menu)         lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    if(s_calib)        lv_obj_add_flag(s_calib, LV_OBJ_FLAG_HIDDEN);
    if(s_line)         lv_obj_add_flag(s_line,  LV_OBJ_FLAG_HIDDEN);
    if(s_stub_manual)  lv_obj_add_flag(s_stub_manual,  LV_OBJ_FLAG_HIDDEN);
    if(s_stub_alarm)   lv_obj_add_flag(s_stub_alarm,   LV_OBJ_FLAG_HIDDEN);
    if(s_stub_factory) lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_info)    lv_obj_add_flag(s_stub_info,    LV_OBJ_FLAG_HIDDEN);

    /* 显示对应页 */
    switch(p){
    case PAGE_MENU:     lv_obj_clear_flag(s_menu, LV_OBJ_FLAG_HIDDEN);         lv_obj_move_foreground(s_menu);         break;
    case PAGE_CALIBRATE:lv_obj_clear_flag(s_calib, LV_OBJ_FLAG_HIDDEN);        lv_obj_move_foreground(s_calib);        break;
    case PAGE_LINE:     lv_obj_clear_flag(s_line,  LV_OBJ_FLAG_HIDDEN);        lv_obj_move_foreground(s_line);         break;
    case PAGE_MANUAL:   lv_obj_clear_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN);  lv_obj_move_foreground(s_stub_manual);  break;
    case PAGE_ALARM:    lv_obj_clear_flag(s_stub_alarm,  LV_OBJ_FLAG_HIDDEN);  lv_obj_move_foreground(s_stub_alarm);   break;
    case PAGE_FACTORY:  lv_obj_clear_flag(s_stub_factory,LV_OBJ_FLAG_HIDDEN);  lv_obj_move_foreground(s_stub_factory); break;
    case PAGE_INFO:     lv_obj_clear_flag(s_stub_info,   LV_OBJ_FLAG_HIDDEN);  lv_obj_move_foreground(s_stub_info);    break;
    default: break;
    }
    s_page = p;
}

void settings_ui_show_menu(void)          { show_only(PAGE_MENU); }
void settings_ui_show_calibrate(void)     { show_only(PAGE_CALIBRATE); }
void settings_ui_show_line_setting(void)  { show_only(PAGE_LINE); }
void settings_ui_show_manual(void)        { show_only(PAGE_MANUAL); }
void settings_ui_show_alarm_setting(void) { show_only(PAGE_ALARM); }
void settings_ui_show_factory_reset(void) { show_only(PAGE_FACTORY); }
void settings_ui_show_machine_info(void)  { show_only(PAGE_INFO); }

/* ===================== 数据写入/读取（校准页） ===================== */

static void refresh_if_visible(void)
{
    if(s_page==PAGE_CALIBRATE){
        refresh_left_labels();
        rebuild_right_rows();
    }
}

void settings_calib_set_points(uint8_t n){
    if(n<1) n=1; if(n>4) n=4;
    g_cfg.point_count = n;
    refresh_if_visible();
}
void settings_calib_set_max_kg(int kg){ g_cfg.max_weight_kg = kg; refresh_if_visible(); }
void settings_calib_set_zero_kg(int kg){ g_cfg.zero_drift_kg = kg; refresh_if_visible(); }
void settings_calib_set_ck_type(uint8_t type01){ g_cfg.ck_type = (type01?1:0); refresh_if_visible(); }
void settings_calib_set_setpoint(uint8_t idx, int kg){
    if(idx<4){ g_cfg.setpoint_kg[idx] = kg; refresh_if_visible(); }
}
const CalibConfig* settings_calib_get(void){ return &g_cfg; }

/* ===================== 校准过程提示（供外部回调后调用） ===================== */

static void toast(const char *txt)
{
    if(!s_root) return;
    lv_obj_t *m = lv_msgbox_create(lv_layer_top(), NULL, txt, NULL, false);
    lv_obj_center(m);
    lv_timer_t *t = lv_timer_create_basic();
    lv_timer_set_period(t, 1200);
    lv_timer_set_repeat_count(t, 1);
}

void settings_calib_notify_ok(uint8_t point_idx)
{
    char buf[64]; lv_snprintf(buf,sizeof(buf),"Calibrate point%d OK", point_idx+1);
    toast(buf);
}
void settings_calib_notify_fail(uint8_t point_idx, const char *reason)
{
    char buf[96]; lv_snprintf(buf,sizeof(buf),"Calibrate point%d FAIL\n%s", point_idx+1, reason?reason:"");
    toast(buf);
}

/* ===================== 业务钩子默认实现（弱符号） ===================== */
__attribute__((weak))
void settings_on_calibrate_start(uint8_t point_idx, int target_kg){
    LV_LOG_USER("Calibrate start: point=%u target=%dkg", point_idx, target_kg);
    settings_calib_notify_ok(point_idx);
}
