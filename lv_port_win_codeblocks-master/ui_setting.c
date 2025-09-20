#include "ui_setting.h"
#include <stdio.h>
#include <string.h>

/* ===================== 可调布局参数（配 800x480，中心区 800x400） ===================== */
#define MARGIN_X        24
#define MARGIN_Y        16
#define TILE_W          220
#define TILE_H          150
#define TILE_RADIUS     28
#define GAP_X           80
#define GAP_Y           70

#define BTN_W           140
#define BTN_H           44
#define ROW_H           56              /* 校准页每行高度（保证 7 行也能排下） */
#define COL_LEFT_X      80
#define COL_MID_X       420
#define COL_RIGHT_X     640

/* ===================== 内部状态 ===================== */
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
static lv_obj_t *s_stub_line = NULL;
static lv_obj_t *s_stub_manual = NULL;
static lv_obj_t *s_stub_alarm = NULL;
static lv_obj_t *s_stub_factory = NULL;
static lv_obj_t *s_stub_info = NULL;

static PageId s_page = PAGE_MENU;

/* 设备/参数模型（默认值随意给一些演示用） */
static CalibConfig g_cfg = {
    .point_count = 2,
    .max_weight_kg = 20,
    .zero_drift_kg = 0,
    .ck_type = 0,
    .setpoint_kg = {0,10,0,0}
};

/* ===================== 工具：禁滚动 ===================== */
static void no_scroll(lv_obj_t *o){
    if(!o) return;
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

/* ===================== 小型“数字键盘” ===================== */
/* 只输 0~9，OK 确认。绑定到某个 label + 目标 int*，并自动加单位后缀 */
typedef struct {
    lv_obj_t *target_label;
    int      *bind_var;
    const char *suffix;     /* 如 "kg" */
    uint8_t   max_digits;   /* 例如 3 表示最大 999 */
} KeypadBind;

static lv_obj_t *s_kp_mask = NULL, *s_kp_box=NULL, *s_kp_ta=NULL;
static KeypadBind s_kp = {0};

static void kp_close(void){
    if(s_kp_mask){
        lv_indev_reset(lv_indev_get_act(), NULL);
        lv_obj_del(s_kp_mask);
        s_kp_mask = s_kp_box = s_kp_ta = NULL;
    }
    memset(&s_kp, 0, sizeof(s_kp));
}

static void kp_btn_cb(lv_event_t *e){
    lv_obj_t *btnm = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btnm, lv_btnmatrix_get_selected_btn(btnm));
    if(!txt) return;

    if(strcmp(txt, "OK")==0){
        const char *in = lv_textarea_get_text(s_kp_ta);
        int v = 0;
        for(const char *p=in; *p; ++p){ if(*p>='0'&&*p<='9'){ v = v*10 + (*p-'0'); } }
        if(s_kp.bind_var) *s_kp.bind_var = v;
        if(s_kp.target_label){
            char buf[24];
            if(s_kp.suffix) lv_snprintf(buf,sizeof(buf),"%d%s",v,s_kp.suffix);
            else lv_snprintf(buf,sizeof(buf),"%d",v);
            lv_label_set_text(s_kp.target_label, buf);
        }
        kp_close();
        return;
    }else if(strcmp(txt, "<-")==0){
        lv_textarea_del_char(s_kp_ta);
    }else if(strcmp(txt, "C")==0){
        lv_textarea_set_text(s_kp_ta, "");
    }else{
        const char *cur = lv_textarea_get_text(s_kp_ta);
        if(strlen(cur) < s_kp.max_digits) lv_textarea_add_char(s_kp_ta, txt[0]);
    }
}

static void kp_open(lv_obj_t *target_label, int *bind_var, const char *suffix, uint8_t max_digits)
{
    kp_close();
    s_kp.target_label = target_label;
    s_kp.bind_var = bind_var;
    s_kp.suffix = suffix;
    s_kp.max_digits = max_digits;

    s_kp_mask = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_kp_mask);
    lv_obj_set_size(s_kp_mask, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_kp_mask, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_kp_mask, LV_OPA_60, 0);
    lv_obj_add_flag(s_kp_mask, LV_OBJ_FLAG_CLICKABLE);

    s_kp_box = lv_obj_create(s_kp_mask);
    lv_obj_set_size(s_kp_box, LV_HOR_RES*3/5, LV_VER_RES*3/5);
    lv_obj_center(s_kp_box);

    s_kp_ta = lv_textarea_create(s_kp_box);
    lv_obj_set_width(s_kp_ta, lv_pct(90));
    lv_obj_align(s_kp_ta, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(s_kp_ta, true);
    lv_textarea_set_max_length(s_kp_ta, max_digits);
    /* 预填当前值 */
    if(bind_var){
        char tmp[12]; lv_snprintf(tmp,sizeof(tmp),"%d", *bind_var);
        lv_textarea_set_text(s_kp_ta, tmp);
        lv_textarea_cursor_right(s_kp_ta);
    }

    static const char *map[]={
        "1","2","3","\n",
        "4","5","6","\n",
        "7","8","9","\n",
        "<-","0","C","OK",""
    };
    lv_obj_t *btnm = lv_btnmatrix_create(s_kp_box);
    lv_btnmatrix_set_map(btnm, map);
    lv_obj_set_size(btnm, lv_pct(90), lv_pct(70));
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btnm, kp_btn_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ===================== 通用：蓝色圆角大 tile 按钮 ===================== */
static lv_obj_t* make_tile(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, TILE_W, TILE_H);
    lv_obj_set_style_radius(btn, TILE_RADIUS, 0);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* ===================== 页面：主菜单 ===================== */
static void cb_show_calib(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_calibrate(); }
static void cb_show_line (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_line_setting(); }
static void cb_show_manual(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_manual(); }
static void cb_show_alarm (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_alarm_setting(); }
static void cb_show_factory(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_factory_reset(); }
static void cb_show_info   (lv_event_t *e){ LV_UNUSED(e); settings_ui_show_machine_info(); }

static void build_menu(lv_obj_t *parent)
{
    s_menu = lv_obj_create(parent);
    no_scroll(s_menu);
    lv_obj_set_size(s_menu, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_menu, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 3 x 2 网格（手动摆放更直观） */
    make_tile(s_menu, "calibrate",           cb_show_calib,  LV_ALIGN_TOP_LEFT,   MARGIN_X,               40);
    make_tile(s_menu, "line setting",        cb_show_line,   LV_ALIGN_TOP_MID,    0,                      40);
    make_tile(s_menu, "manual",              cb_show_manual, LV_ALIGN_TOP_RIGHT, -MARGIN_X,               40);

    make_tile(s_menu, "alarm setting",       cb_show_alarm,  LV_ALIGN_BOTTOM_LEFT, MARGIN_X,             -120);
    make_tile(s_menu, "factory data reset",  cb_show_factory, LV_ALIGN_BOTTOM_MID, 0,                    -120);
    make_tile(s_menu, "machine\ninformation",cb_show_info,   LV_ALIGN_BOTTOM_RIGHT,-MARGIN_X,            -120);
}

/* ===================== 页面：校准设置 ===================== */
static lv_obj_t *s_lbl_points = NULL;
static lv_obj_t *s_lbl_maxkg  = NULL;
static lv_obj_t *s_lbl_zero   = NULL;
static lv_obj_t *s_dd_ck      = NULL;
static lv_obj_t *s_right_panel = NULL;      /* 右侧行容器（动态重建） */
static lv_obj_t *s_btn_exit    = NULL;

static void rebuild_right_rows(void);       /* 前置声明 */

static void ev_points(lv_event_t *e){
    LV_UNUSED(e);
    kp_open(s_lbl_points, (int*)&g_cfg.point_count, NULL, 1); /* 1 位，允许 1~9(建议你自己限制到 <=4) */
}
static void ev_maxkg(lv_event_t *e){
    LV_UNUSED(e);
    kp_open(s_lbl_maxkg, &g_cfg.max_weight_kg, "kg", 3);
}
static void ev_zero(lv_event_t *e){
    LV_UNUSED(e);
    kp_open(s_lbl_zero, &g_cfg.zero_drift_kg, "kg", 3);
}
static void ev_ck_changed(lv_event_t *e){
    LV_UNUSED(e);
    g_cfg.ck_type = lv_dropdown_get_selected(s_dd_ck); /* 0 hens / 1 cocks */
}

static void ev_exit(lv_event_t *e){ LV_UNUSED(e); settings_ui_show_menu(); }

static lv_obj_t* make_value_btn(lv_obj_t *parent, const char *txt, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BTN_W, BTN_H);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return lab; /* 返回 label，便于后续刷新文本 */
}

static void refresh_left_labels(void)
{
    char buf[32];
    lv_snprintf(buf,sizeof(buf),"%d", g_cfg.point_count); lv_label_set_text(s_lbl_points, buf);
    lv_snprintf(buf,sizeof(buf),"%dkg", g_cfg.max_weight_kg); lv_label_set_text(s_lbl_maxkg, buf);
    lv_snprintf(buf,sizeof(buf),"%dkg", g_cfg.zero_drift_kg); lv_label_set_text(s_lbl_zero, buf);
    lv_dropdown_set_selected(s_dd_ck, g_cfg.ck_type);
}

static void ev_setpoint_btn(lv_event_t *e)
{
    /* 点击“设置重量”按钮：打开键盘绑定到对应 setpoint_kg[idx] */
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lab = lv_obj_get_child(btn, 0);
    kp_open(lab, &g_cfg.setpoint_kg[idx], "kg", 3);
}

static void ev_do_calibrate(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(settings_on_calibrate_start) settings_on_calibrate_start((uint8_t)idx, g_cfg.setpoint_kg[idx]);
}

/* 校准页右侧“setting weigh / calibrate point”动态重建 */
static void rebuild_right_rows(void)
{
    if(s_right_panel) lv_obj_del(s_right_panel);
    s_right_panel = lv_obj_create(s_calib);
    no_scroll(s_right_panel);
    lv_obj_set_size(s_right_panel, 320, lv_obj_get_height(s_calib)-MARGIN_Y*2);
    lv_obj_align(s_right_panel, LV_ALIGN_TOP_LEFT, COL_MID_X-20, MARGIN_Y);

    /* 标题 */
    lv_obj_t *t1 = lv_label_create(s_right_panel);
    lv_label_set_text(t1, "setting weigh");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 10, 0);

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
        lv_obj_add_event_cb(btn_set, ev_setpoint_btn, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        char buf[24]; lv_snprintf(buf,sizeof(buf),"%dkg", g_cfg.setpoint_kg[i]);
        lv_label_set_text(lv_label_create(btn_set), buf);

        /* 右边“pointX”按钮（挂在 s_calib，位置靠右列） */
        lv_obj_t *btn_point = lv_btn_create(s_calib);
        lv_obj_set_size(btn_point, BTN_W, BTN_H);
        lv_obj_align(btn_point, LV_ALIGN_TOP_LEFT, COL_RIGHT_X, y);
        lv_obj_add_event_cb(btn_point, ev_do_calibrate, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        char ptxt[24]; lv_snprintf(ptxt,sizeof(ptxt),"point%d", i+1);
        lv_label_set_text(lv_label_create(btn_point), ptxt);
    }
}

static void build_calibrate(lv_obj_t *parent)
{
    s_calib = lv_obj_create(parent);
    no_scroll(s_calib);
    lv_obj_set_size(s_calib, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_calib, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 左列标签与按钮 */
    lv_obj_t *l1 = lv_label_create(s_calib); lv_label_set_text(l1, "points num");
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40);
    s_lbl_points = make_value_btn(s_calib, "2", ev_points);
    lv_obj_align(lv_obj_get_parent(s_lbl_points), LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30);

    lv_obj_t *l2 = lv_label_create(s_calib); lv_label_set_text(l2, "max weigh");
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40+ROW_H*1);
    s_lbl_maxkg = make_value_btn(s_calib, "20kg", ev_maxkg);
    lv_obj_align(lv_obj_get_parent(s_lbl_maxkg), LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30+ROW_H*1);

    lv_obj_t *l3 = lv_label_create(s_calib); lv_label_set_text(l3, "zero defit");
    lv_obj_align(l3, LV_ALIGN_TOP_LEFT, COL_LEFT_X-60, 40+ROW_H*2);
    s_lbl_zero = make_value_btn(s_calib, "0kg", ev_zero);
    lv_obj_align(lv_obj_get_parent(s_lbl_zero), LV_ALIGN_TOP_LEFT, COL_LEFT_X, 30+ROW_H*2);

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

/* ===================== 占位子页（可替换为真实实现） ===================== */
static lv_obj_t* build_stub(lv_obj_t *parent, const char *title)
{
    lv_obj_t *box = lv_obj_create(parent);
    no_scroll(box);
    lv_obj_set_size(box, lv_obj_get_width(parent), lv_obj_get_height(parent));
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

/* ===================== 公共：创建/销毁 ===================== */
void settings_ui_create(lv_obj_t *parent)
{
    if(s_root) return;
    s_root = lv_obj_create(parent);
    no_scroll(s_root);
    lv_obj_set_size(s_root, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);

    build_menu(s_root);
    build_calibrate(s_root);
    s_stub_line    = build_stub(s_root, "line setting");
    s_stub_manual  = build_stub(s_root, "manual");
    s_stub_alarm   = build_stub(s_root, "alarm setting");
    s_stub_factory = build_stub(s_root, "factory data reset");
    s_stub_info    = build_stub(s_root, "machine information");

    /* 初始：只显示主菜单 */
    lv_obj_add_flag(s_calib,       LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_line,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_alarm,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_factory,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_info,   LV_OBJ_FLAG_HIDDEN);
    s_page = PAGE_MENU;
}

void settings_ui_destroy(void)
{
    kp_close();
    if(s_root){ lv_obj_del(s_root); s_root = NULL; }
    s_menu=s_calib=s_stub_line=s_stub_manual=s_stub_alarm=s_stub_factory=s_stub_info=NULL;
    s_page = PAGE_MENU;
}

/* ===================== 显示切换 ===================== */
static void show_only(PageId p)
{
    if(!s_root) return;
    /* 全部隐藏 */
    if(s_menu)         lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    if(s_calib)        lv_obj_add_flag(s_calib, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_line)    lv_obj_add_flag(s_stub_line, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_manual)  lv_obj_add_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_alarm)   lv_obj_add_flag(s_stub_alarm, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_factory) lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_info)    lv_obj_add_flag(s_stub_info, LV_OBJ_FLAG_HIDDEN);

    /* 显示对应 */
    switch(p){
    case PAGE_MENU:     lv_obj_clear_flag(s_menu, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_CALIBRATE:lv_obj_clear_flag(s_calib, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_LINE:     lv_obj_clear_flag(s_stub_line, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_MANUAL:   lv_obj_clear_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_ALARM:    lv_obj_clear_flag(s_stub_alarm, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_FACTORY:  lv_obj_clear_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN); break;
    case PAGE_INFO:     lv_obj_clear_flag(s_stub_info, LV_OBJ_FLAG_HIDDEN); break;
    default: break;
    }
    s_page = p;
}

void settings_ui_show_menu(void)      { show_only(PAGE_MENU); }
void settings_ui_show_calibrate(void) { show_only(PAGE_CALIBRATE); }
void settings_ui_show_line_setting(void){ show_only(PAGE_LINE); }
void settings_ui_show_manual(void)    { show_only(PAGE_MANUAL); }
void settings_ui_show_alarm_setting(void){ show_only(PAGE_ALARM); }
void settings_ui_show_factory_reset(void){ show_only(PAGE_FACTORY); }
void settings_ui_show_machine_info(void){ show_only(PAGE_INFO); }

/* ===================== 数据写入/读取 ===================== */
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
    /* 1.2 秒后自动消失 */
    lv_timer_t *t = lv_timer_create_basic();
    lv_timer_set_period(t, 1200);
    lv_timer_set_repeat_count(t, 1);
//    lv_timer_set_user_data(t, m);
  /*  lv_timer_set_cb(t, (lv_timer_cb_t)[](lv_timer_t *tm){
        lv_obj_t *mb = (lv_obj_t*)lv_timer_get_user_data(tm);
        if(mb) lv_obj_del(mb);
    });*/
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
    /* 这里仅演示：真实项目中你去触发称重传感器校准流程，完成后调用 notify_ok/fail */
    LV_LOG_USER("Calibrate start: point=%u target=%dkg", point_idx, target_kg);
    settings_calib_notify_ok(point_idx);
}
