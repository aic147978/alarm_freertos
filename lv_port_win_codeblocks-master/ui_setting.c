#include "ui_setting.h"
#include "ui_keypad.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>
#include "ui_left1.h"
/* ===================== 布局参数（针对中心区 ~800x400） ===================== */
/* 整个页面内容与四周的留白（内边距） */
#define MARGIN_X        8
#define MARGIN_Y        8

/* 菜单页大按钮尺寸 */
#define TILE_W          160
#define TILE_H          120
#define TILE_RADIUS     28

/* 校准页数值按钮尺寸 */
#define BTN_W           100
#define BTN_H           36

/* 校准页列/行坐标 */
#define ROW_H           72
#define COL_LEFT_X      80
#define COL_MID_X       320
#define COL_RIGHT_X     560

/* ===================== 内部状态/指针 ===================== */
typedef enum {
    PAGE_MENU = 0,
    PAGE_CALIBRATE,
    PAGE_LINE,
    PAGE_MANUAL,
    PAGE_ALARM,
    PAGE_FACTORY,
    PAGE_INFO,
} PageId;

static lv_obj_t *s_root = NULL;
static lv_obj_t *s_menu = NULL;
static lv_obj_t *s_calib = NULL;
static lv_obj_t *s_line  = NULL;   /* ← 真实的 line setting 页容器（非占位） */
static lv_obj_t *s_manual = NULL;  /* ← 新增：手动模式页容器 */
static lv_obj_t *s_stub_alarm = NULL;
static lv_obj_t *s_stub_factory = NULL;
static lv_obj_t *s_stub_info = NULL;
static PageId    s_page = PAGE_MENU;

/* ====== 校准页数据模型（默认值演示用） ====== */
static CalibConfig g_cfg = {
    .point_count   = 3,
    .max_weight_kg = 20,
    .zero_drift_kg = 0,
    .ck_type       = 0,
    .setpoint_kg   = {0, 10, 0, 0}
};

/* 去掉容器的面板外观 */
static void strip_panel_look(lv_obj_t *o){
    if(!o) return;
    lv_obj_set_style_bg_opa      (o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0,            LV_PART_MAIN);
    lv_obj_set_style_outline_width(o,0,            LV_PART_MAIN);
    lv_obj_set_style_shadow_width(o, 0,            LV_PART_MAIN);
    lv_obj_set_style_radius      (o, 0,            LV_PART_MAIN);
    lv_obj_set_style_pad_all     (o, 0,            LV_PART_MAIN);
}

/* === 菜单大按钮样式 === */
static lv_style_t st_tile, st_tile_label;
static bool st_inited = false;
static void ensure_styles(void){
    if(st_inited) return;
    st_inited = true;

    lv_style_init(&st_tile);
    lv_style_set_bg_color(&st_tile, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&st_tile, LV_OPA_COVER);
    lv_style_set_radius(&st_tile, TILE_RADIUS);
    lv_style_set_border_width(&st_tile, 0);
    lv_style_set_shadow_width(&st_tile, 10);
    lv_style_set_shadow_opa(&st_tile, LV_OPA_40);
    lv_style_set_pad_all(&st_tile, 8);

    lv_style_init(&st_tile_label);
    lv_style_set_text_color(&st_tile_label, lv_color_white());
    lv_style_set_text_align(&st_tile_label, LV_TEXT_ALIGN_CENTER);
}

/* 禁滚动 */
static void no_scroll(lv_obj_t *o){
    if(!o) return;
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

/* 菜单大按钮工厂 */
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

/* ===================== 菜单页 ===================== */
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
    lv_obj_set_size(s_menu, LV_PCT(100), LV_PCT(135));
    lv_obj_align(s_menu, LV_ALIGN_TOP_LEFT, 0, -20);
    lv_obj_set_style_bg_opa(s_menu, LV_OPA_TRANSP, 0);

    make_tile(s_menu, "calibrate",            cb_show_calib,   LV_ALIGN_TOP_LEFT,     MARGIN_X, 40);
    make_tile(s_menu, "line setting",         cb_show_line,    LV_ALIGN_TOP_MID,      0,        40);
    make_tile(s_menu, "manual",               cb_show_manual,  LV_ALIGN_TOP_RIGHT,   -MARGIN_X, 40);
    make_tile(s_menu, "alarm setting",        cb_show_alarm,   LV_ALIGN_BOTTOM_LEFT,  MARGIN_X,  -120);
    make_tile(s_menu, "factory data reset",   cb_show_factory, LV_ALIGN_BOTTOM_MID,   0,         -120);
    make_tile(s_menu, "machine\ninformation", cb_show_info,    LV_ALIGN_BOTTOM_RIGHT,-MARGIN_X,  -120);
}

/* ===================== 校准页（保持你之前逻辑） ===================== */
static lv_obj_t *s_lbl_points = NULL;
static lv_obj_t *s_lbl_maxkg  = NULL;
static lv_obj_t *s_lbl_zero   = NULL;
static lv_obj_t *s_dd_ck      = NULL;
static lv_obj_t *s_right_panel = NULL;
static lv_obj_t *s_btn_exit    = NULL;

static ui_bind_entry_t s_entry_points = {0};
static ui_bind_entry_t s_entry_maxkg  = {0};
static ui_bind_entry_t s_entry_zero   = {0};
static ui_bind_entry_t s_entry_setpoints[4] = {0};

static void ev_ck_changed(lv_event_t *e){
    LV_UNUSED(e);
    g_cfg.ck_type = lv_dropdown_get_selected(s_dd_ck);
}
static void ev_exit(lv_event_t *e){
    LV_UNUSED(e); settings_ui_show_menu();
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
static void refresh_left_labels(void){
    ui_keypad_refresh_entry(&s_entry_points);
    ui_keypad_refresh_entry(&s_entry_maxkg);
    ui_keypad_refresh_entry(&s_entry_zero);
    lv_dropdown_set_selected(s_dd_ck, g_cfg.ck_type);
}
static void ev_do_calibrate(lv_event_t *e){
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(settings_on_calibrate_start) settings_on_calibrate_start((uint8_t)idx, g_cfg.setpoint_kg[idx]);
}
static void rebuild_right_rows(void)
{
    if(s_right_panel) lv_obj_del(s_right_panel);

    s_right_panel = lv_obj_create(s_calib);
    no_scroll(s_right_panel);
    lv_obj_set_size(s_right_panel, 320, LV_PCT(100));
    lv_obj_align(s_right_panel, LV_ALIGN_TOP_LEFT, COL_MID_X-20, 0);
    lv_obj_set_style_pad_top(s_right_panel, MARGIN_Y, 0);
    lv_obj_set_style_pad_bottom(s_right_panel, MARGIN_Y, 0);

    lv_obj_t *t1 = lv_label_create(s_right_panel);
    lv_label_set_text(t1, "setting weigh");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 10, 0);

    lv_obj_t *t2 = lv_label_create(s_calib);
    lv_label_set_text(t2, "calibrate point");
    lv_obj_align(t2, LV_ALIGN_TOP_LEFT, COL_RIGHT_X-20, MARGIN_Y);

    uint8_t n = g_cfg.point_count; if(n<1) n=1; if(n>4) n=4;
    for(uint8_t i=0;i<n;i++){
        int y = MARGIN_Y + 40 + i*ROW_H;

        lv_obj_t *btn_set = lv_btn_create(s_right_panel);
        lv_obj_set_size(btn_set, BTN_W, BTN_H);
        lv_obj_align(btn_set, LV_ALIGN_TOP_LEFT, 10, 40 + i*ROW_H);
        lv_obj_t *label = lv_label_create(btn_set);
        ui_keypad_entry_init(&s_entry_setpoints[i], label, &g_cfg.setpoint_kg[i], "kg", false, 3);
        ui_keypad_bind_button(btn_set, &s_entry_setpoints[i]);

        lv_obj_t *btn_point = lv_btn_create(s_calib);
        lv_obj_set_size(btn_point, BTN_W, BTN_H);
        lv_obj_align(btn_point, LV_ALIGN_TOP_LEFT, COL_RIGHT_X, y);
        lv_obj_add_event_cb(btn_point, ev_do_calibrate, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        char ptxt[24]; lv_snprintf(ptxt,sizeof(ptxt),"point%d", i+1);
        lv_label_set_text(lv_label_create(btn_point), ptxt);
    }
    for(uint8_t i=n;i<4;i++) s_entry_setpoints[i].label = NULL;
}

static void build_calibrate(lv_obj_t *parent)
{
    s_calib = lv_obj_create(parent);
    no_scroll(s_calib);
    lv_obj_set_size(s_calib, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_calib, LV_ALIGN_TOP_LEFT, 0, 0);

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

    s_btn_exit = lv_btn_create(s_calib);
    lv_obj_set_size(s_btn_exit, 180, 56);
    lv_obj_align(s_btn_exit, LV_ALIGN_BOTTOM_RIGHT, -24, -16);
    lv_obj_add_event_cb(s_btn_exit, ev_exit, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_exit), "exit");

    refresh_left_labels();
    rebuild_right_rows();
}

/* ===================== line setting 页（外部实现） ===================== */
/* 你自己的 line-setting 构建函数；若函数名不同，请改这里 */
extern lv_obj_t* build_line_setting(lv_obj_t *parent);

/* ===== line-setting 的运行态（从你的实现里导出）。用它决定手动页阀门数量 =====
 * 如果你的变量名不同，请：
 *  1）把下方4个 extern 改成你的对外符号，或
 *  2）删掉 extern，换成你的 getter 函数调用。
 */
extern bool s_line_enable_left;   // 【填写处】若你不是这个变量名，请改
extern bool s_line_enable_right;  // 【填写处】
extern int  s_line_count_left;    // 【填写处】
extern int  s_line_count_right;   // 【填写处*/

/* ===================== 手动模式页 ===================== */
#ifndef MANUAL_MAX_VALVES
#define MANUAL_MAX_VALVES  16
#endif
#define MANUAL_TOP_Y1      40
#define MANUAL_TOP_Y2     120
#define MANUAL_LANE_H      20
#define MANUAL_SILO_X      60
#define MANUAL_SILO_Y      40
#define MANUAL_SILO_W     190
#define MANUAL_SILO_H     190
#define MANUAL_RAIL_X0   (MANUAL_SILO_X + MANUAL_SILO_W + 20)
#define MANUAL_RAIL_X1   (760)
#define MANUAL_VALVE_W     56
#define MANUAL_VALVE_H     68
#define MANUAL_VALVE_GAP   38
#define MANUAL_PANEL_Y0   240
#define MANUAL_COL_L      220
#define MANUAL_COL_R      620

/* 顶部图形/阀门对象缓存 */
static lv_obj_t *s_silo = NULL;
static lv_obj_t *s_rail1 = NULL;
static lv_obj_t *s_rail2 = NULL;
static lv_obj_t *s_valve_btn[2][MANUAL_MAX_VALVES];
static uint8_t   s_valve_cnt[2] = {0,0};
static int8_t    s_sel_line = -1;
static int8_t    s_sel_idx  = -1;

/* weigh/time 绑定到“手动副本” */
static ui_bind_entry_t s_entry_w = {0};
static ui_bind_entry_t s_entry_t = {0};

/* 手动页的“影子变量”（不会影响自动模式） */
static int s_manual_left_weigh [MANUAL_MAX_VALVES]  = {0};
static int s_manual_left_time_s[MANUAL_MAX_VALVES]  = {10};
static int s_manual_right_weigh[MANUAL_MAX_VALVES]  = {0};
static int s_manual_right_time_s[MANUAL_MAX_VALVES] = {10};

/* 下方控件与样式 */
static lv_obj_t *s_btn_w = NULL;
static lv_obj_t *s_btn_t = NULL;
static lv_obj_t *s_btn_start = NULL;
static lv_obj_t *s_btn_stop  = NULL;
static lv_obj_t *s_btn_exit_m = NULL;
static bool      s_manual_running = false;
static lv_style_t st_valve_norm, st_valve_sel, st_rail, st_silo2;
static bool      st_manual_inited = false;

static void manual_ensure_styles(void){
    if(st_manual_inited) return;
    st_manual_inited = true;

    lv_style_init(&st_silo2);
    lv_style_set_bg_color(&st_silo2, lv_palette_darken(LV_PALETTE_BLUE, 2));
    lv_style_set_bg_opa(&st_silo2, LV_OPA_COVER);
    lv_style_set_radius(&st_silo2, 6);

    lv_style_init(&st_rail);
    lv_style_set_bg_color(&st_rail, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&st_rail, LV_OPA_COVER);
    lv_style_set_radius(&st_rail, 4);

    lv_style_init(&st_valve_norm);
    lv_style_set_bg_color(&st_valve_norm, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&st_valve_norm, LV_OPA_COVER);
    lv_style_set_radius(&st_valve_norm, 6);
    lv_style_set_shadow_width(&st_valve_norm, 10);
    lv_style_set_shadow_opa(&st_valve_norm, LV_OPA_40);

    lv_style_init(&st_valve_sel);
    lv_style_set_bg_color(&st_valve_sel, lv_palette_lighten(LV_PALETTE_INDIGO, 1));
    lv_style_set_bg_opa(&st_valve_sel, LV_OPA_COVER);
    lv_style_set_radius(&st_valve_sel, 6);
    lv_style_set_border_width(&st_valve_sel, 3);
    lv_style_set_border_color(&st_valve_sel, lv_palette_main(LV_PALETTE_YELLOW));
}

static void manual_mark_selected(lv_obj_t *btn, bool sel){
    if(!btn) return;
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, MANUAL_VALVE_W, MANUAL_VALVE_H);
    lv_obj_add_style(btn, sel ? &st_valve_sel : &st_valve_norm, 0);
}

/* 互斥选择 */
static void manual_select(int line, int idx){
    if(line<0 || line>1) return;
    if(idx<0) return;

    if(s_sel_line >= 0 && s_sel_idx >= 0){
        lv_obj_t *old = s_valve_btn[s_sel_line][s_sel_idx];
        if(old) manual_mark_selected(old, false);
    }
    s_sel_line = (int8_t)line;
    s_sel_idx  = (int8_t)idx;

    lv_obj_t *now = s_valve_btn[line][idx];
    if(now) manual_mark_selected(now, true);

    if(line==0){
        s_entry_w.var = &s_manual_left_weigh[idx];
        s_entry_t.var = &s_manual_left_time_s[idx];
    }else{
        s_entry_w.var = &s_manual_right_weigh[idx];
        s_entry_t.var = &s_manual_right_time_s[idx];
    }
    ui_keypad_refresh_entry(&s_entry_w);
    ui_keypad_refresh_entry(&s_entry_t);
}

static void valve_event_cb(lv_event_t *e){
    uintptr_t packed = (uintptr_t)lv_event_get_user_data(e);
    int line = (int)((packed >> 16) & 0xFFFF);
    int idx  = (int)( packed        & 0xFFFF);
    manual_select(line, idx);
}

static void manual_build_line(lv_obj_t *parent, int line, lv_coord_t center_y, int valve_count)
{
    if(valve_count < 0) valve_count = 0;
    if(valve_count > MANUAL_MAX_VALVES) valve_count = MANUAL_MAX_VALVES;
    s_valve_cnt[line] = (uint8_t)valve_count;

    lv_obj_t *rail = lv_obj_create(parent);
    strip_panel_look(rail);
    lv_obj_set_size(rail, MANUAL_RAIL_X1 - MANUAL_RAIL_X0, MANUAL_LANE_H);
    lv_obj_align(rail, LV_ALIGN_TOP_LEFT, MANUAL_RAIL_X0, center_y - MANUAL_LANE_H/2);
    lv_obj_add_style(rail, &st_rail, 0);
    if(line==0) s_rail1 = rail; else s_rail2 = rail;

    if(valve_count <= 0){
        for(int i=0;i<MANUAL_MAX_VALVES;i++) s_valve_btn[line][i] = NULL;
        return;
    }

    const int total_w = valve_count * MANUAL_VALVE_W + (valve_count-1) * MANUAL_VALVE_GAP;
    const int x_start = MANUAL_RAIL_X0 + ((MANUAL_RAIL_X1 - MANUAL_RAIL_X0) - total_w)/2;

    for(int i=0;i<valve_count;i++){
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, MANUAL_VALVE_W, MANUAL_VALVE_H);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT,
                     x_start + i*(MANUAL_VALVE_W + MANUAL_VALVE_GAP),
                     center_y - MANUAL_VALVE_H/2);
        manual_mark_selected(btn, false);
        lv_obj_add_event_cb(btn, valve_event_cb, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)(((uint32_t)line<<16) | (uint32_t)i));
        s_valve_btn[line][i] = btn;
    }
    for(int i=valve_count;i<MANUAL_MAX_VALVES;i++) s_valve_btn[line][i] = NULL;
}

/* Start/Suspend/Stop/Exit */
static void manual_start_event(lv_event_t *e){
    LV_UNUSED(e);
    if(s_sel_line<0 || s_sel_idx<0) return;

    s_manual_running = !s_manual_running;
    lv_obj_t *lab = lv_obj_get_child(s_btn_start, 0);
    if(lab) lv_label_set_text(lab, s_manual_running ? "suspend" : "start");

    int kg = (s_sel_line==0) ? s_manual_left_weigh[s_sel_idx]  : s_manual_right_weigh[s_sel_idx];
    int ts = (s_sel_line==0) ? s_manual_left_time_s[s_sel_idx] : s_manual_right_time_s[s_sel_idx];
    manual_on_start(s_manual_running, (uint8_t)s_sel_line, (uint8_t)s_sel_idx, kg, ts);
}
static void manual_stop_event(lv_event_t *e){
    LV_UNUSED(e);
    s_manual_running = false;
    lv_obj_t *lab = lv_obj_get_child(s_btn_start, 0);
    if(lab) lv_label_set_text(lab, "start");
    manual_on_stop();
}
static void manual_exit_event(lv_event_t *e){
    LV_UNUSED(e);
    settings_ui_show_menu();
}

static lv_obj_t* manual_make_value_btn(lv_obj_t *parent, ui_bind_entry_t *entry,
                                       const char *title, lv_coord_t x_title, lv_coord_t x_btn)
{
    lv_obj_t *t = lv_label_create(parent);
    lv_label_set_text(t, title);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, x_title, MANUAL_PANEL_Y0 - 24);

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BTN_W*1.6, BTN_H*1.4);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_btn, MANUAL_PANEL_Y0 + 8);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, "--");
    lv_obj_center(lab);

    entry->label = lab;
    entry->var   = NULL;                       /* 选中阀门后才指向具体元素 */
    entry->suffix= (title[0]=='s' && title[8]=='w') ? "kg" : "s";
    entry->is_time = (entry->suffix[0]=='s');
    entry->max_digits = entry->is_time ? 3 : 3;

    ui_keypad_refresh_entry(entry);
    ui_keypad_bind_button(btn, entry);
    return btn;
}

static void build_manual(lv_obj_t *parent)
{
    manual_ensure_styles();

    s_manual = lv_obj_create(parent);
    no_scroll(s_manual);
    strip_panel_look(s_manual);
    lv_obj_set_size(s_manual, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_manual, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 左侧料塔 */
    s_silo = lv_obj_create(s_manual);
    strip_panel_look(s_silo);
    lv_obj_set_size(s_silo, MANUAL_SILO_W, MANUAL_SILO_H);
    lv_obj_align(s_silo, LV_ALIGN_TOP_LEFT, MANUAL_SILO_X, MANUAL_SILO_Y);
    lv_obj_add_style(s_silo, &st_silo2, 0);

    /* 两行阀门：数量来自 line-setting 页 */
    int nL = (/*左线启用?*/ s_line_enable_left  ? s_line_count_left  : 0);   // 【填写处】
    int nR = (/*右线启用?*/ s_line_enable_right ? s_line_count_right : 0);   // 【填写处】
    manual_build_line(s_manual, 0, MANUAL_TOP_Y1, nL);
    manual_build_line(s_manual, 1, MANUAL_TOP_Y2, nR);

    /* weigh/time 数值（绑定在 entry_w/entry_t 上，选阀门后会指向对应副本） */
    s_btn_w = manual_make_value_btn(s_manual, &s_entry_w, "setting weigh", MANUAL_COL_L-40, MANUAL_COL_L-80);
    s_btn_t = manual_make_value_btn(s_manual, &s_entry_t, "setting time",  MANUAL_COL_R-20, MANUAL_COL_R-80);

    /* 控制按钮 */
    s_btn_start = lv_btn_create(s_manual);
    lv_obj_set_size(s_btn_start, 200, 64);
    lv_obj_align(s_btn_start, LV_ALIGN_TOP_LEFT, 320-100, MANUAL_PANEL_Y0 + 96);
    lv_obj_add_event_cb(s_btn_start, manual_start_event, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_start), "start");

    s_btn_stop = lv_btn_create(s_manual);
    lv_obj_set_size(s_btn_stop, 200, 64);
    lv_obj_align(s_btn_stop, LV_ALIGN_TOP_LEFT, 560-100, MANUAL_PANEL_Y0 + 96);
    lv_obj_add_event_cb(s_btn_stop, manual_stop_event, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_stop), "stop");

    s_btn_exit_m = lv_btn_create(s_manual);
    lv_obj_set_size(s_btn_exit_m, 240, 64);
    lv_obj_align(s_btn_exit_m, LV_ALIGN_TOP_LEFT, 900-240, MANUAL_PANEL_Y0 + 96);
    lv_obj_add_event_cb(s_btn_exit_m, manual_exit_event, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_exit_m), "exit");

    /* 初始默认选第一个可用阀门 */
    if(nL>0)      manual_select(0, 0);
    else if(nR>0) manual_select(1, 0);
}

static void destroy_manual(void){
    s_manual_running = false;
    s_sel_line = s_sel_idx = -1;
    if(s_manual){ lv_obj_del(s_manual); s_manual = NULL; }
}

/* ===== 手动页把“自动模式”值同步为本页初值（供外部调用，可选） ===== */
void settings_manual_sync_from_auto(const int *left_w, const int *left_t,
                                    const int *right_w, const int *right_t,
                                    int n_left, int n_right)
{
    if(n_left  > MANUAL_MAX_VALVES)  n_left  = MANUAL_MAX_VALVES;
    if(n_right > MANUAL_MAX_VALVES)  n_right = MANUAL_MAX_VALVES;

    for(int i=0;i<n_left;i++){
        if(left_w) s_manual_left_weigh[i]  = left_w[i];   // 【填写处】把你的自动模式数组地址传进来
        if(left_t) s_manual_left_time_s[i] = left_t[i];   // 【填写处】
    }
    for(int i=0;i<n_right;i++){
        if(right_w) s_manual_right_weigh[i]  = right_w[i];   // 【填写处】
        if(right_t) s_manual_right_time_s[i] = right_t[i];   // 【填写处】
    }
}

/* 业务回调弱符号（外部实现可覆盖） */
__attribute__((weak))
void manual_on_start(bool resume, uint8_t line, uint8_t valve_idx, int kg, int time_s){
    LV_LOG_USER("[manual] start/resume=%d line=%u valve=%u target=%dkg %ds",
                resume?1:0, line, valve_idx, kg, time_s);
}
__attribute__((weak))
void manual_on_stop(void){
    LV_LOG_USER("[manual] stop");
}

/* ===================== 占位页（报警/出厂/信息） ===================== */
static lv_obj_t* build_stub(lv_obj_t *parent, const char *title)
{
    lv_obj_t *box = lv_obj_create(parent);
    no_scroll(box);
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

/* ===================== 主入口/销毁 ===================== */
void settings_ui_create(lv_obj_t *parent)
{
    if(s_root) return;

    s_root = lv_obj_create(parent);
    no_scroll(s_root);
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);
    strip_panel_look(s_root);

    build_menu(s_root);
    build_calibrate(s_root);
    s_line = build_line_setting(s_root);      /* ← 你的 line-setting 真实实现 */
    build_manual(s_root);                     /* ← 新增：手动页 */
    s_stub_alarm   = build_stub(s_root, "alarm setting");
    s_stub_factory = build_stub(s_root, "factory data reset");
    s_stub_info    = build_stub(s_root, "machine information");

    /* 初始显示菜单，隐藏其它 */
    lv_obj_add_flag(s_calib,       LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_line,        LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_manual,      LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_alarm,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_factory,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_info,   LV_OBJ_FLAG_HIDDEN);
    s_page = PAGE_MENU;
}

void settings_ui_destroy(void)
{
    ui_keypad_close();
    destroy_manual();                /* ← 新增：清理手动页 */

    if(s_root){ lv_obj_del(s_root); s_root = NULL; }
    s_menu=s_calib=s_line=s_stub_alarm=s_stub_factory=s_stub_info=NULL;

    s_lbl_points = s_lbl_maxkg = s_lbl_zero = NULL;
    s_dd_ck = NULL;
    s_right_panel = NULL;
    s_btn_exit = NULL;
    memset(&s_entry_points, 0, sizeof(s_entry_points));
    memset(&s_entry_maxkg,  0, sizeof(s_entry_maxkg));
    memset(&s_entry_zero,   0, sizeof(s_entry_zero));
    memset(s_entry_setpoints, 0, sizeof(s_entry_setpoints));
    s_page = PAGE_MENU;
}

/* ===================== 切页 ===================== */
static void show_only(PageId p)
{
    if(!s_root) return;

    if(s_menu)         lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    if(s_calib)        lv_obj_add_flag(s_calib, LV_OBJ_FLAG_HIDDEN);
    if(s_line)         lv_obj_add_flag(s_line,  LV_OBJ_FLAG_HIDDEN);
    if(s_manual)       lv_obj_add_flag(s_manual,LV_OBJ_FLAG_HIDDEN);
    if(s_stub_alarm)   lv_obj_add_flag(s_stub_alarm, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_factory) lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_info)    lv_obj_add_flag(s_stub_info, LV_OBJ_FLAG_HIDDEN);

    switch(p){
    case PAGE_MENU:      lv_obj_clear_flag(s_menu,   LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_menu);   break;
    case PAGE_CALIBRATE: lv_obj_clear_flag(s_calib,  LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_calib);  break;
    case PAGE_LINE:      lv_obj_clear_flag(s_line,   LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_line);   break;
    case PAGE_MANUAL:    lv_obj_clear_flag(s_manual, LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_manual); break;
    case PAGE_ALARM:     lv_obj_clear_flag(s_stub_alarm,   LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_stub_alarm);   break;
    case PAGE_FACTORY:   lv_obj_clear_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_stub_factory); break;
    case PAGE_INFO:      lv_obj_clear_flag(s_stub_info,    LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(s_stub_info);    break;
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

/* ===================== 校准页对外（保持不变） ===================== */
static void refresh_if_visible(void){
    if(s_page==PAGE_CALIBRATE){
        refresh_left_labels();
        rebuild_right_rows();
    }
}
void settings_calib_set_points(uint8_t n){ if(n<1) n=1; if(n>4) n=4; g_cfg.point_count = n; refresh_if_visible(); }
void settings_calib_set_max_kg(int kg){ g_cfg.max_weight_kg = kg; refresh_if_visible(); }
void settings_calib_set_zero_kg(int kg){ g_cfg.zero_drift_kg = kg; refresh_if_visible(); }
void settings_calib_set_ck_type(uint8_t type01){ g_cfg.ck_type = (type01?1:0); refresh_if_visible(); }
void settings_calib_set_setpoint(uint8_t idx, int kg){ if(idx<4){ g_cfg.setpoint_kg[idx] = kg; refresh_if_visible(); } }
const CalibConfig* settings_calib_get(void){ return &g_cfg; }

/* 校准业务弱符号 */
__attribute__((weak))
void settings_on_calibrate_start(uint8_t point_idx, int target_kg){
    LV_LOG_USER("Calibrate start: point=%u target=%dkg", point_idx, target_kg);
    /* 你在别处实现真正流程，完成后可弹提示等 */
}
