#include "ui_setting.h"
#include "ui_keypad.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>
/*
 ===== Debug helpers =====
static void log_obj(const char *tag, lv_obj_t *o){
    if(!o){ LV_LOG_USER("%s: (null)", tag); return; }
    LV_LOG_USER("%s: obj=%p pos=(%d,%d) size=%dx%d hidden=%d children=%u",
        tag, o,
        (int)lv_obj_get_x(o), (int)lv_obj_get_y(o),
        (int)lv_obj_get_width(o), (int)lv_obj_get_height(o),
        lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN),
        (unsigned)lv_obj_get_child_cnt(o));
}

static void log_children(const char *tag, lv_obj_t *parent){
    uint32_t n = lv_obj_get_child_cnt(parent);
    LV_LOG_USER("%s: children=%u", tag, (unsigned)n);
    for(uint32_t i=0;i<n;i++){
        lv_obj_t *ch = lv_obj_get_child(parent, i);
        LV_LOG_USER("  [%u] %p hidden=%d size=%dx%d",
            (unsigned)i, ch,
            lv_obj_has_flag(ch, LV_OBJ_FLAG_HIDDEN),
            (int)lv_obj_get_width(ch), (int)lv_obj_get_height(ch));
    }
}*/

/* ===================== 布局参数（针对中心区 ~800x400） ===================== */

/* 整个页面内容与四周的留白（内边距）。影响表格/右侧面板、菜单等整体离边缘的距离 */
#define MARGIN_X        8      // 左右边距（像素）
#define MARGIN_Y        8      // 上下边距（像素）

/* “主菜单页”里每个大瓷砖按钮（calibrate/line setting/...）的尺寸与圆角 */
#define TILE_W          220     // 大按钮的宽
#define TILE_H          120     // 大按钮的高
#define TILE_RADIUS     28      // 大按钮圆角半径，仅视觉效果

/* 预留的瓷砖按钮间距（目前大按钮是用 align 定位，这两个可能没实际用到；
   如果以后改为网格/自动排布，可用它们作为水平/垂直间距） */
#define GAP_X           80      // 大按钮之间的水平间距
#define GAP_Y           70      // 大按钮之间的垂直间距

/* 校准页等处的小按钮（数值框、point1/point2）的尺寸 */
#define BTN_W           100     // 小按钮宽
#define BTN_H           36      // 小按钮高

/* 校准页中每一“行”的竖向步进：决定行与行之间的间距
   例如 points/max weigh/zero drift/… 或右侧的“setting weigh + pointX”成对行。
   想让一页最多容纳 7 行，就把它调得小些；想更疏松，就调大些。*/
#define ROW_H           72      // 行高（步进）。影响控件纵向排布节距

/* 这三个是“绝对 X 坐标”，用于把左/中/右三列的控件放到指定列上
   ——相对于当前页面容器（如 s_calib）的左上角 (0,0)。*/
#define COL_LEFT_X      80      // 左列的起始 X（放左侧标签+数值按钮）
#define COL_MID_X       320     // 中列的起始 X（右侧面板的左边缘）
#define COL_RIGHT_X     560     // 右列的起始 X（右侧 point1/point2 按钮列）

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

/* 设备/参数模型（默认值演示用） */
static CalibConfig g_cfg = {
    .point_count   = 3,
    .max_weight_kg = 20,
    .zero_drift_kg = 0,
    .ck_type       = 0,
    .setpoint_kg   = {0, 10, 0, 0}
};


/* 去掉容器的面板外观：透明背景、无边框/阴影/描边、无内边距 */
static void strip_panel_look(lv_obj_t *o){
    if(!o) return;
    lv_obj_set_style_bg_opa      (o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0,            LV_PART_MAIN);
    lv_obj_set_style_outline_width(o,0,            LV_PART_MAIN);
    lv_obj_set_style_shadow_width(o, 0,            LV_PART_MAIN);
    lv_obj_set_style_radius      (o, 0,            LV_PART_MAIN);
    lv_obj_set_style_pad_all     (o, 0,            LV_PART_MAIN);
}
/* === 大按钮样式：蓝底白字/圆角/阴影 === */
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

/* ===================== 工具：禁滚动 ===================== */
static void no_scroll(lv_obj_t *o){
    if(!o) return;
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

/* ===================== 通用：蓝色圆角大 tile 按钮 ===================== */
static lv_obj_t* make_tile(lv_obj_t *parent, const char *txt,
                           lv_event_cb_t cb, lv_align_t align,
                           lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    ensure_styles();
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 120);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_style(btn, &st_tile, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
    lv_obj_add_style(lab, &st_tile_label, 0);

    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    LV_LOG_USER("tile '%s' created: btn=%p label=%p", txt, btn, lab);
    return btn;
}

/* ===================== 页面：主菜单 ===================== */
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
    /* ★ 用百分比占满父容器，避免 0×0 */
    lv_obj_set_size(s_menu, LV_PCT(100), LV_PCT(135));
    lv_obj_align(s_menu, LV_ALIGN_TOP_LEFT, 0, -20);
    lv_obj_set_style_bg_opa(s_menu, LV_OPA_TRANSP, 0);

    LV_LOG_USER("build_menu: s_menu=%p size=%dx%d",
        s_menu, (int)lv_obj_get_width(s_menu), (int)lv_obj_get_height(s_menu));

    /* 六个大按钮 */
    make_tile(s_menu, "calibrate",            cb_show_calib,   LV_ALIGN_TOP_LEFT,     MARGIN_X, 40);
    make_tile(s_menu, "line setting",         cb_show_line,    LV_ALIGN_TOP_MID,      0,        40);
    make_tile(s_menu, "manual",               cb_show_manual,  LV_ALIGN_TOP_RIGHT,   -MARGIN_X, 40);

    make_tile(s_menu, "alarm setting",        cb_show_alarm,   LV_ALIGN_BOTTOM_LEFT,  MARGIN_X,  -120);
    make_tile(s_menu, "factory data reset",   cb_show_factory, LV_ALIGN_BOTTOM_MID,   0,         -120);
    make_tile(s_menu, "machine\ninformation", cb_show_info,    LV_ALIGN_BOTTOM_RIGHT,-MARGIN_X,  -120);
}

/* ===================== 页面：校准设置 ===================== */
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

static void rebuild_right_rows(void);

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

/* 校准页右侧“setting weigh / calibrate point”动态重建 */
/* 右侧区域：统一放在一个容器里，用“列-行”布局 */
/* 右侧区域：统一放在一个容器里，用“列-行”布局 */
static void rebuild_right_rows(void)
{
    if(s_right_panel) lv_obj_del(s_right_panel);

    s_right_panel = lv_obj_create(s_calib);
    no_scroll(s_right_panel);
    /* ★ 用百分比高度，避免在布局前读到 0 高 */
    lv_obj_set_size(s_right_panel, 320, LV_PCT(100));
    lv_obj_align(s_right_panel, LV_ALIGN_TOP_LEFT, COL_MID_X-20, 0);
    /* 用内边距形成上下边距效果 */
    lv_obj_set_style_pad_top(s_right_panel, MARGIN_Y, 0);
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
    /* ★ 占满父容器，避免 0×0 */
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

/* ===================== 占位子页（可替换为真实实现） ===================== */
static lv_obj_t* build_stub(lv_obj_t *parent, const char *title)
{
    lv_obj_t *box = lv_obj_create(parent);
    no_scroll(box);
    /* ★ 占满父容器 */
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

/* ===================== 公共：创建/销毁 ===================== */
void settings_ui_create(lv_obj_t *parent)
{
    if(s_root) return;

    LV_LOG_USER("settings_ui_create: parent=%p size=%dx%d",
                parent, (int)lv_obj_get_width(parent), (int)lv_obj_get_height(parent));

    s_root = lv_obj_create(parent);
    no_scroll(s_root);
    /* ★ 占满父容器，避免 0×0 */
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);

    strip_panel_look(s_root);

    LV_LOG_USER("settings_ui_create: s_root=%p size=%dx%d",
                s_root, (int)lv_obj_get_width(s_root), (int)lv_obj_get_height(s_root));

    build_menu(s_root);
    build_calibrate(s_root);
    s_stub_line    = build_stub(s_root, "line setting");
    s_stub_manual  = build_stub(s_root, "manual");
    s_stub_alarm   = build_stub(s_root, "alarm setting");
    s_stub_factory = build_stub(s_root, "factory data reset");
    s_stub_info    = build_stub(s_root, "machine information");

    /* 初始：只显示主菜单 */
    lv_obj_add_flag(s_calib,        LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_line,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_manual,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_alarm,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_stub_info,    LV_OBJ_FLAG_HIDDEN);
    s_page = PAGE_MENU;

//    log_obj("settings_ui_create: root", s_root);
  //  log_children("settings_ui_create: root children", s_root);
}

void settings_ui_destroy(void)
{
    ui_keypad_close();
    if(s_root){ lv_obj_del(s_root); s_root = NULL; }
    s_menu=s_calib=s_stub_line=s_stub_manual=s_stub_alarm=s_stub_factory=s_stub_info=NULL;
    s_page = PAGE_MENU;
}

/* ===================== 显示切换 ===================== */
static void show_only(PageId p)
{
    LV_LOG_USER("show_only(%d)", p);
    if(!s_root) return;

    /* 全部隐藏 */
    if(s_menu)         lv_obj_add_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
    if(s_calib)        lv_obj_add_flag(s_calib, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_line)    lv_obj_add_flag(s_stub_line, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_manual)  lv_obj_add_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_alarm)   lv_obj_add_flag(s_stub_alarm, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_factory) lv_obj_add_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
    if(s_stub_info)    lv_obj_add_flag(s_stub_info, LV_OBJ_FLAG_HIDDEN);

    /* 显示对应，并尽量前置，避免层叠遮挡 */
    switch(p){
    case PAGE_MENU:
        lv_obj_clear_flag(s_menu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_menu);
        break;
    case PAGE_CALIBRATE:
        lv_obj_clear_flag(s_calib, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_calib);
        break;
    case PAGE_LINE:
        lv_obj_clear_flag(s_stub_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_stub_line);
        break;
    case PAGE_MANUAL:
        lv_obj_clear_flag(s_stub_manual, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_stub_manual);
        break;
    case PAGE_ALARM:
        lv_obj_clear_flag(s_stub_alarm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_stub_alarm);
        break;
    case PAGE_FACTORY:
        lv_obj_clear_flag(s_stub_factory, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_stub_factory);
        break;
    case PAGE_INFO:
        lv_obj_clear_flag(s_stub_info, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_stub_info);
        break;
    default: break;
    }
    s_page = p;

    /* 打印当前 root 子项状态，排查可见性问题 */
//    log_children("show_only: root children", s_root);
}

void settings_ui_show_menu(void)          { show_only(PAGE_MENU); }
void settings_ui_show_calibrate(void)     { show_only(PAGE_CALIBRATE); }
void settings_ui_show_line_setting(void)  { show_only(PAGE_LINE); }
void settings_ui_show_manual(void)        { show_only(PAGE_MANUAL); }
void settings_ui_show_alarm_setting(void) { show_only(PAGE_ALARM); }
void settings_ui_show_factory_reset(void) { show_only(PAGE_FACTORY); }
void settings_ui_show_machine_info(void)  { show_only(PAGE_INFO); }

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
    /* 此处可加回调删除 msgbox；简化略 */
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
