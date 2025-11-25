/* app_ui.c ― 顶层 UI 总控（含 ui2_* 子菜单与跳转）
 * 底部按钮：左2 + 中圆(Home=子界面3) + 右2
 * 上电默认显示子界面3
 */

#include "app_ui.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ui1_1_weightset.h"     /* 1_1 */
#include "ui1_2_timeset.h"       /* 1_2 */
#include "ui4_1_weighthistory.h" /* 4_1 */
#include "ui4_2_alarmhistory.h"  /* 4_2 */

/* ―― 子界面2 家族 ―― */
#include "ui2_0_setchoose.h"     /* 2_0 菜单 */
#include "ui2_1_calibrate.h"     /* 2_1 校准 */
#include "ui2_2_lineset.h"       /* 2_2 行数设置 */
#include "ui2_3_manual.h"        /* 2_3 手动控制 */
#include "ui2_4_alarmset.h"      /* [NEW] 2_4 报警阈值设置 */

#include "keyboard.h"            /* 可选：keyboard_hide() */

#include "control_bridge.h"    // 写控制 targetWeight[] 以及其他参数

#include "control_bridge.h"
#include "lvgl.h"

/* ===================== 固定画面尺寸 ===================== */
#define UI_W            800
#define UI_H            480
#define UI_CONTENT_H    400
#define UI_NAV_H        80

/* ===================== 底栏样式 ===================== */
#define COL_NAV_BG      0xC7D3EF
#define COL_BTN    0x2D5CC0  /* 普通(默认) */
#define COL_BTN_PR 0x1E3F8F  /* 按下/选中(更深) */
#define COL_TEXT        0xFFFFFF

#define NAV_BTN_W       170
#define NAV_BTN_H       62
#define NAV_GAP_X       14

#define NAV_HOME_D      72
#define NAV_HOME_OFSY   (-40)

/* ===================== 内部页状态枚举 ===================== */
typedef enum { V1_MODE_WEIGHT=0, V1_MODE_TIME=1 } v1_mode_t;       /* 1_1 / 1_2 */
typedef enum { V4_MODE_HISTORY=0, V4_MODE_ALARM=1 } v4_mode_t;     /* 4_1 / 4_2 */
typedef enum {
    V2_PAGE_MENU = 0,  /* 2_0 */
    V2_PAGE_CALIB,     /* 2_1 */
    V2_PAGE_LINESET,   /* 2_2 */
    V2_PAGE_MANUAL,    /* 2_3 */
    V2_PAGE_ALARMSET   /* [NEW] 2_4 */
} v2_page_t;

/* ===================== 顶层对象与状态 ===================== */
static lv_obj_t *s_root    = NULL;
static lv_obj_t *s_content = NULL;
static lv_obj_t *s_nav     = NULL;

static lv_obj_t *s_btn1 = NULL, *s_btn2 = NULL, *s_btn3 = NULL, *s_btn4 = NULL, *s_btn5 = NULL;

static view_id_t s_current = VIEW_NONE;
static v1_mode_t s_v1_mode = V1_MODE_WEIGHT;
static v4_mode_t s_v4_mode = V4_MODE_HISTORY;
static v2_page_t s_v2_page = V2_PAGE_MENU;

/* ===================== 工具函数 ===================== */
static void make_panel_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa  (o, opa, 0);

    /* 关键：把所有 padding/outline 都置 0，且禁滚动 */
    lv_obj_set_style_pad_all   (o, 0, 0);
    lv_obj_set_style_pad_row   (o, 0, 0);
    lv_obj_set_style_pad_column(o, 0, 0);
    lv_obj_set_style_border_width (o, 0, 0);
    lv_obj_set_style_outline_width(o, 0, 0);

    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static void style_nav_rect_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius(btn, 14, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_hex(COL_TEXT), 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
}

static void style_nav_home_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(btn, 18, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    lv_obj_set_style_shadow_spread(btn, 2, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
}

static lv_obj_t* make_nav_rect(lv_obj_t *parent, const char *txt, int w, int h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    style_nav_rect_btn(btn);
    lv_obj_set_size(btn, w, h);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_center(label);
    return btn;
}
static lv_obj_t* make_nav_home(lv_obj_t *parent, int diameter)
{
    lv_obj_t *btn = lv_btn_create(parent);
    style_nav_home_btn(btn);
    lv_obj_set_size(btn, diameter, diameter);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME);
    lv_obj_center(label);
    return btn;
}

/* ===================== View1：1_1 / 1_2 ===================== */
static void v1_unload_current(void)
{
    if (s_v1_mode == V1_MODE_WEIGHT) ui11_weightset_destroy();
    else                             ui12_timeset_destroy();
    keyboard_hide();
}
static void v1_load_weight(void);
static void v1_load_timeset(void);
static void v1_go_weight_cb(void *ud){ LV_UNUSED(ud); v1_load_weight(); }
static void v1_go_timeset_cb(void *ud){ LV_UNUSED(ud); v1_load_timeset(); }
static void apply_weightset_to_control(void);

static void v1_load_weight(void)
{
    v1_unload_current();
    s_v1_mode = V1_MODE_WEIGHT;
    ui11_weightset_create(s_content);
    ui11_weightset_set_toggle_cb(v1_go_timeset_cb, NULL);
}
static void v1_load_timeset(void)
{
    v1_unload_current();
    s_v1_mode = V1_MODE_TIME;
    ui12_timeset_create(s_content);
    ui12_timeset_set_toggle_cb(v1_go_weight_cb, NULL);
}

/* ===================== View4：4_1 / 4_2 ===================== */
static void v4_unload_current(void)
{
    if (s_v4_mode == V4_MODE_HISTORY) ui41_weighthistory_destroy();
    else                              ui42_alarmhistory_destroy();
}
static void v4_load_41(void);
static void v4_load_42(void);
static void v4_go_41_cb(void *ud){ LV_UNUSED(ud); v4_load_41(); }
static void v4_go_42_cb(void *ud){ LV_UNUSED(ud); v4_load_42(); }

static void v4_load_41(void)
{
    v4_unload_current();
    s_v4_mode = V4_MODE_HISTORY;
    ui41_weighthistory_create(s_content);
    ui41_set_toggle_cb(v4_go_42_cb, NULL);

    /* 示例数据 */
    ui41_set_row_count(12);
    ui41_clear_all();
    ui41_set_record(0, "2025-11-13 10:21:03", 12.5f, 87.5f);
    ui41_set_record(1, "2025-11-13 10:45:22",  8.0f, 79.5f);
    for (int i = 2; i < 12; ++i) {
        char ts[32];
        snprintf(ts, sizeof(ts), "2025-11-13 11:%02d:%02d", 10+i, 5+i);
        ui41_set_record((uint16_t)i, ts, 6.5f + 0.3f*i, 60.0f - 0.8f*i);
    }
}
static void v4_load_42(void)
{
    v4_unload_current();
    s_v4_mode = V4_MODE_ALARM;
    ui42_alarmhistory_create(s_content);
    ui42_set_toggle_cb(v4_go_41_cb, NULL);

    /* 示例数据 */
    ui42_set_row_count(9);
    ui42_clear_all();
    ui42_set_record(0, "Overheat",    "2025-11-13 10:32:10");
    ui42_set_record(1, "Door Open",   "2025-11-13 10:45:07");
    ui42_set_record(2, "Fan Failure", "2025-11-13 11:05:21");
    for (int i = 3; i < 9; ++i) {
        char ts[32], tp[48];
        snprintf(ts, sizeof(ts), "2025-11-13 11:%02d:%02d", 10+i, 7+i);
        snprintf(tp, sizeof(tp), "Sensor %d offline", i);
        ui42_set_record((uint16_t)i, tp, ts);
    }
}

/* ===================== View2：2_0 / 2_1 / 2_2 / 2_3 / 2_4 ===================== */

/* 前置声明（注意：这里只声明，不实现，避免重复定义） */
static void v2_on_menu_select(uint8_t idx, void *ud);
static void v2_exit_to_menu(void *ud);                 /* ← 这里只是声明！ */
static void v2_enter_manual(void *ud);
static void v2_lineset_apply(bool L_en, uint16_t L_cnt, bool R_en, uint16_t R_cnt, void *ud);
static void v2_on_open(ui23_side_t side, uint16_t idx, void *ud);
static void v2_on_close(ui23_side_t side, uint16_t idx, void *ud);
static void v2_load_24(void);                          /* [NEW] 2_4 载入函数前置声明 */


/* 销毁当前 2_* 页面 */
static void v2_unload_current(void)
{
    switch (s_v2_page) {
    case V2_PAGE_MENU:    ui20_destroy(); break;
    case V2_PAGE_CALIB:   ui21_calib_destroy(); break;
    case V2_PAGE_LINESET: ui22_lineset_destroy(); break;
    case V2_PAGE_MANUAL:  ui23_manual_destroy(); break;
    case V2_PAGE_ALARMSET:ui24_alarmset_destroy(); break;  /* [NEW] 补充 2_4 销毁 */
    default: break;
    }
}

/* 载入 2_0 */
static void v2_load_20(void)
{
    v2_unload_current();
    s_v2_page = V2_PAGE_MENU;

    ui20_create(s_content);
    ui20_set_button_text(1, "Calibrate");  /* 2_1 */
    ui20_set_button_text(2, "Line set");   /* 2_2 */
    ui20_set_button_text(3, "Manual");     /* 2_3 */
    ui20_set_button_text(4, "Alarm set");  /* [CHANGED] 原“Option4”→“Alarm set”(2_4) */
    ui20_set_button_text(5, "Option5");
    ui20_set_button_text(6, "Option6");

    ui20_set_select_cb(v2_on_menu_select, NULL);
}

/* 载入 2_1 */
static void v2_load_21(void)
{
    v2_unload_current();
    s_v2_page = V2_PAGE_CALIB;

    ui21_calib_create(s_content);
    ui21_set_exit_cb(v2_exit_to_menu, NULL);

    /* 示例参数 */
    ui21_set_points(3);
    ui21_set_max_weight(25.0f);
    ui21_set_empty_weight(1.5f);
    ui21_set_point_weight(0, 0.0f);
    ui21_set_point_weight(1, 10.0f);
    ui21_set_point_weight(2, 20.0f);
}

/* 载入 2_2 */
static void v2_load_22(void)
{
    v2_unload_current();
    s_v2_page = V2_PAGE_LINESET;

    ui22_lineset_create(s_content);
    ui22_set_exit_cb(v2_exit_to_menu, NULL);
    ui22_set_apply_cb(v2_lineset_apply, NULL);

    /* 示例：默认左右启用，各 6 行 */
    ui22_set_left_enabled(true);
    ui22_set_right_enabled(true);
    ui22_set_left_count(6);
    ui22_set_right_count(6);
}

/* 载入 2_3 */
static void v2_load_23(void)
{
    v2_unload_current();
    s_v2_page = V2_PAGE_MANUAL;

    ui23_manual_create(s_content);
    ui23_set_exit_cb(v2_exit_to_menu, NULL);
    ui23_set_enter_manual_cb(v2_enter_manual, NULL);
    ui23_set_open_cb(v2_on_open, NULL);
    ui23_set_close_cb(v2_on_close, NULL);

    /* 从 2_2 当前设置同步两排按钮 */
    ui23_apply_lineset(ui22_get_left_enabled(), ui22_get_left_count(),
                       ui22_get_right_enabled(), ui22_get_right_count());
}

/* 载入 2_4（报警阈值设置） */
static void v2_load_24(void)  /* [NEW] */
{
    v2_unload_current();
    s_v2_page = V2_PAGE_ALARMSET;

    ui24_alarmset_create(s_content);
    ui24_set_exit_cb(v2_exit_to_menu, NULL);

    /* 示例：默认阈值与勾选项 */
    ui24_set_full_percent(115);
    ui24_set_empty_percent(10);
    ui24_set_timeout_sec(30);
    ui24_set_check_all(false);
    /* 也可以定点启用某些勾选，如：
       ui24_set_check(0, true);
       ui24_set_check(5, true);
    */
}

/* 2_0 点击跳转 */
static void v2_on_menu_select(uint8_t idx, void *ud)
{
    LV_UNUSED(ud);
    switch (idx) {
    case 1: v2_load_21(); break;
    case 2: v2_load_22(); break;
    case 3: v2_load_23(); break;
    case 4: v2_load_24(); break;   /* [NEW] 选第 4 项进入 2_4 */
    case 5: /* Option5 */ break;
    case 6: /* Option6 */ break;
    default: break;
    }
}

/* 统一返回 2_0 */
static void v2_exit_to_menu(void *ud)
{
    LV_UNUSED(ud);
    v2_load_20();
}

/* 进入 2_3 时通知：停止自动模式等 */
static void v2_enter_manual(void *ud)
{
    LV_UNUSED(ud);
    /* stop_auto(); // 你自己的逻辑 */
}

/* 2_2 参数变化时联动 2_3 */
static void v2_lineset_apply(bool L_en, uint16_t L_cnt, bool R_en, uint16_t R_cnt, void *ud)
{
    LV_UNUSED(ud);
    if (s_v2_page == V2_PAGE_MANUAL) {
        ui23_apply_lineset(L_en, L_cnt, R_en, R_cnt);
    }
}

/* 2_3 的 Open/Close 回调（纯 C 函数） */
static void v2_on_open(ui23_side_t side, uint16_t idx, void *ud)
{
    LV_UNUSED(ud);
    /* relay_open(side == UI23_SIDE_LEFT, idx); */
}
static void v2_on_close(ui23_side_t side, uint16_t idx, void *ud)
{
    LV_UNUSED(ud);
    /* relay_close(side == UI23_SIDE_LEFT, idx); */
}

/* ===================== 切换与卸载 ===================== */
static void clear_content_of(view_id_t which)
{
    bool deleted_by_module = false;

    if (which == VIEW_1) {
        v1_unload_current();
        keyboard_hide();
        deleted_by_module = true;
    } else if (which == VIEW_4) {
        v4_unload_current();
        deleted_by_module = true;
    } else if (which == VIEW_2) {
        v2_unload_current();
        deleted_by_module = true;
    }

    if (!deleted_by_module && s_content) {
        lv_obj_clean(s_content);
    }
}

static void make_placeholder(const char *txt)
{
    lv_obj_t *label = lv_label_create(s_content);
    lv_label_set_text(label, txt);
    lv_obj_center(label);
}

static void set_nav_checked(view_id_t id)
{
    if (s_btn1) lv_obj_clear_state(s_btn1, LV_STATE_CHECKED);
    if (s_btn2) lv_obj_clear_state(s_btn2, LV_STATE_CHECKED);
    if (s_btn3) lv_obj_clear_state(s_btn3, LV_STATE_CHECKED);
    if (s_btn4) lv_obj_clear_state(s_btn4, LV_STATE_CHECKED);
    if (s_btn5) lv_obj_clear_state(s_btn5, LV_STATE_CHECKED);

    switch (id) {
        case VIEW_1: if (s_btn1) lv_obj_add_state(s_btn1, LV_STATE_CHECKED); break;
        case VIEW_2: if (s_btn2) lv_obj_add_state(s_btn2, LV_STATE_CHECKED); break;
        case VIEW_3: if (s_btn3) lv_obj_add_state(s_btn3, LV_STATE_CHECKED); break;
        case VIEW_4: if (s_btn4) lv_obj_add_state(s_btn4, LV_STATE_CHECKED); break;
        case VIEW_5: if (s_btn5) lv_obj_add_state(s_btn5, LV_STATE_CHECKED); break;
        default: break;
    }
}

static void nav_btn_event_cb(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;
    view_id_t id = (view_id_t)(uintptr_t)lv_event_get_user_data(e);

    /* 切换视图 */
    if (s_current == id) return;
    view_id_t prev = s_current;
    clear_content_of(prev);
    s_current = id;
    set_nav_checked(id);

    switch (id) {
        case VIEW_1:
            if (s_v1_mode == V1_MODE_WEIGHT) v1_load_weight();
            else                              v1_load_timeset();
            break;
        case VIEW_2:
            v2_load_20();  /* 进入 2_0 菜单 */
            break;
        case VIEW_3:
            make_placeholder("View 3 (Home)");
            break;
        case VIEW_4:
            if (s_v4_mode == V4_MODE_HISTORY) v4_load_41();
            else                               v4_load_42();
            break;
        case VIEW_5:
            make_placeholder("View 5");
            break;
        default: break;
    }
}

/* ===================== 底部导航栏 ===================== */
static void create_navbar(void)
{
    s_nav = lv_obj_create(s_root);
    make_panel_plain(s_nav, UI_W, UI_NAV_H, lv_color_hex(COL_NAV_BG), LV_OPA_COVER);
    lv_obj_align(s_nav, LV_ALIGN_BOTTOM_MID, 0, 0);

    s_btn1 = make_nav_rect(s_nav, "1", NAV_BTN_W, NAV_BTN_H);
    lv_obj_align(s_btn1, LV_ALIGN_LEFT_MID, NAV_GAP_X, 0);
    lv_obj_add_event_cb(s_btn1, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_1);

    s_btn2 = make_nav_rect(s_nav, "2", NAV_BTN_W, NAV_BTN_H);
    lv_obj_align_to(s_btn2, s_btn1, LV_ALIGN_OUT_RIGHT_MID, NAV_GAP_X, 0);
    lv_obj_add_event_cb(s_btn2, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_2);

    s_btn5 = make_nav_rect(s_nav, "5", NAV_BTN_W, NAV_BTN_H);
    lv_obj_align(s_btn5, LV_ALIGN_RIGHT_MID, -NAV_GAP_X, 0);
    lv_obj_add_event_cb(s_btn5, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_5);

    s_btn4 = make_nav_rect(s_nav, "4", NAV_BTN_W, NAV_BTN_H);
    lv_obj_align_to(s_btn4, s_btn5, LV_ALIGN_OUT_LEFT_MID, -NAV_GAP_X, 0);
    lv_obj_add_event_cb(s_btn4, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_4);

    s_btn3 = make_nav_home(s_root, NAV_HOME_D);
    lv_obj_align(s_btn3, LV_ALIGN_BOTTOM_MID, 0, NAV_HOME_OFSY);
    lv_obj_add_event_cb(s_btn3, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_3);
}


static lv_obj_t *s_confirm_box = NULL;

/* 点击弹窗按钮事件 */
static void confirm_event_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    const char *btn_txt = lv_msgbox_get_active_btn_text(mbox);

    /* 关闭弹窗 */
    lv_obj_del(mbox);
    s_confirm_box = NULL;

    if(btn_txt && strcmp(btn_txt, "保存")==0){
        /* 选择保存：写入控制层后再切页面 */
        apply_weightset_to_control();
        /* 这里做你的切换，比如去 1_2： */
        // v1_load_1_2();
    }else{
        /* 不保存：直接切换 */
        // v1_load_1_2();
    }
}

/* 右下角切换按钮的回调：弹出确认框 */
static void on_ui11_switch_clicked(void *ud)
{
    (void)ud;
    if(s_confirm_box) return;

    static const char *btns[] = {"保存", "不保存", ""};

    s_confirm_box = lv_msgbox_create(NULL, "是否保存设置？",
                                     "保存后会写入重量控制参数", btns, false);
    lv_obj_set_width(s_confirm_box, 320);
    lv_obj_center(s_confirm_box);
    lv_obj_add_event_cb(s_confirm_box, confirm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* 在进入 1_1 后把右下角按钮的回调改成 on_ui11_switch_clicked */
static void load_ui11(lv_obj_t *parent_800x400)
{
    ui11_weightset_create(parent_800x400);

    /* 方案 B：把 1_1 的右下角切换按钮设置成“先弹窗” */
    ui11_weightset_set_toggle_cb(on_ui11_switch_clicked, NULL);

    /* 如果你同时想要 A 方案的“销毁即保存”，就不要注册上面的弹窗；
       二者通常二选一即可。 */
}

/* ===================== 对外接口 ===================== */
void app_ui_create(lv_obj_t *parent)
{
    if (s_root) return;
    if (!parent) parent = lv_scr_act();

    s_root = lv_obj_create(parent);
    make_panel_plain(s_root, UI_W, UI_H, lv_color_black(), LV_OPA_TRANSP);

    s_content = lv_obj_create(s_root);
    make_panel_plain(s_content, UI_W, UI_CONTENT_H, lv_color_black(), LV_OPA_TRANSP);
    lv_obj_align(s_content, LV_ALIGN_TOP_MID, 0, 0);

    create_navbar();

    s_current = VIEW_NONE;
    /* 上电默认进子界面3 */
    nav_btn_event_cb(&(lv_event_t){ .code = LV_EVENT_CLICKED, .user_data = (void*)(uintptr_t)VIEW_3 });
}

void app_ui_destroy(void)
{
    if (!s_root) return;

    clear_content_of(s_current);
    lv_obj_del(s_root);

    s_root = s_content = s_nav = NULL;
    s_btn1 = s_btn2 = s_btn3 = s_btn4 = s_btn5 = NULL;
    s_current = VIEW_NONE;
    s_v1_mode = V1_MODE_WEIGHT;
    s_v4_mode = V4_MODE_HISTORY;
    s_v2_page = V2_PAGE_MENU;
}

void app_ui_show(view_id_t id)
{
    if (!s_root) return;
    /* 模拟点击 */
    nav_btn_event_cb(&(lv_event_t){ .code = LV_EVENT_CLICKED, .user_data = (void*)(uintptr_t)id });
}


static void apply_weightset_to_control(void)          //更新写入数据到控制逻辑
{
    /* 左右各最多 7 路：左 0..6，右 7..13 */
    uint16_t rows = ui11_weightset_get_row_count();
    if(rows > 7) rows = 7;

    for(uint16_t r = 0; r < rows; ++r) {
        float lw = ui11_weightset_get_left_weight(r);   // kg，来自 1_1
        float rw = ui11_weightset_get_right_weight(r);  // kg

        ctrl_set_target_weight_left (r, lw);            // kg→g 后写 targetWeight[r]
        ctrl_set_target_weight_right(r, rw);            // kg→g 后写 targetWeight[7+r]
    }
}

