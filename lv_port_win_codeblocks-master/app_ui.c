#include "lvgl.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "weightset.h"   /* 1_1 */
#include "timeset.h"     /* 1_2 */
#include "keyboard.h"    /* 若你有全局键盘模块，可留用；没有也不影响本文件编译 */

#define UI_W            800
#define UI_H            480
#define UI_CONTENT_H    400             /* 上半部分内容区高度 */
#define UI_NAV_H        (UI_H-UI_CONTENT_H) /* 底栏高度（降低的高度） */

#define COL_NAV_BG      0xC7D3EF        /* 底栏淡蓝色 */
#define COL_BTN         0x4D73C9        /* 导航按钮蓝色 */
#define COL_BTN_PR      0x395AA8
#define COL_TEXT        0xFFFFFF

/* ============ 视图枚举与状态 ============ */
typedef enum {
    VIEW_NONE = 0,
    VIEW_1 = 1,
    VIEW_2 = 2,
    VIEW_3 = 3,
    VIEW_4 = 4,
    VIEW_5 = 5
} view_id_t;

/* 子界面1的内部模式：1_1/1_2 */
typedef enum {
    V1_MODE_WEIGHT = 0,   /* 1_1 -> weightset */
    V1_MODE_TIME   = 1    /* 1_2 -> timeset   */
} v1_mode_t;

/* 根对象 */
static lv_obj_t *s_root = NULL;         /* 整个页面根（通常是 screen 或父容器） */
static lv_obj_t *s_content = NULL;      /* 800x400 内容区容器 */
static lv_obj_t *s_nav = NULL;          /* 底部导航栏容器 */

/* 导航按钮句柄（用于高亮/选中） */
static lv_obj_t *s_btn1 = NULL;
static lv_obj_t *s_btn2 = NULL;
static lv_obj_t *s_btn3 = NULL; /* 中间圆形（Home, 子界面3） */
static lv_obj_t *s_btn4 = NULL;
static lv_obj_t *s_btn5 = NULL;

/* 当前显示的子界面与子界面1的模式 */
static view_id_t  s_current = VIEW_NONE;
static v1_mode_t  s_v1_mode = V1_MODE_WEIGHT;

/* ================== 工具：样式与控件 ================== */
static void style_nav_rect_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN), LV_PART_MAIN | LV_STATE_DEFAULT);
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

/* 创建底部矩形按钮（左二/右二） */
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

/* 创建中间圆形 Home（逻辑上是子界面3） */
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

/* ================== 子界面内容的装卸 ================== */
static void clear_content(void)
{
    /* 若当前是子界面1的某个模式，先让其模块销毁 UI（数据保留） */
    if (s_current == VIEW_1) {
        if (s_v1_mode == V1_MODE_WEIGHT) {
            weightset_destroy();
        } else {
            timeset_destroy();
        }
    }
    /* 其他子界面的 UI（如果你后续实现）也在此集中销毁 */

    /* 清空内容容器里的所有子对象 */
    if (s_content) {
        lv_obj_clean(s_content);
    }
}

/* --- 子界面1：两个模式之间互相切换 --- */
static void v1_switch_to_timeset(void *ud);  /* 前置声明 */
static void v1_switch_to_weight(void *ud);

static void v1_load_weight(void)
{
    clear_content();
    s_v1_mode = V1_MODE_WEIGHT;
    weightset_create(s_content);
    /* 挂上 1_1 → 1_2 切换回调 */
    weightset_set_toggle_cb(v1_switch_to_timeset, NULL);

    /* 这里可按需设置布局（可不设，使用模块默认）
       例如：weightset_set_title_y(10); ... */
}

static void v1_load_timeset(void)
{
    clear_content();
    s_v1_mode = V1_MODE_TIME;
    timeset_create(s_content);
    /* 挂上 1_2 → 1_1 切换回调 */
    timeset_set_toggle_cb(v1_switch_to_weight, NULL);

    /* 按需设置布局（可不设）
       例如：timeset_set_title_y(20); ... */
}

static void do_v1_switch_to_timeset(void *ud) { LV_UNUSED(ud); if (s_current==VIEW_1) v1_load_timeset(); }
static void do_v1_switch_to_weight(void *ud) { LV_UNUSED(ud); if (s_current==VIEW_1) v1_load_weight(); }

static void v1_switch_to_timeset(void *ud) { LV_UNUSED(ud); lv_async_call(do_v1_switch_to_timeset, NULL); }
static void v1_switch_to_weight(void *ud) { LV_UNUSED(ud); lv_async_call(do_v1_switch_to_weight, NULL); }

/* --- 其他子界面简单占位（可按需替换为真实内容） --- */
static void make_placeholder(const char *txt)
{
    clear_content();
    lv_obj_t *label = lv_label_create(s_content);
    lv_label_set_text(label, txt);
    lv_obj_center(label);
}

/* ================== 导航切换 ================== */
static void set_nav_checked(view_id_t id)
{
    /* 先全部取消选中 */
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

static void switch_view(view_id_t id)
{
    if (s_current == id) return;

    s_current = id;
    set_nav_checked(id);

    switch (id) {
        case VIEW_1:
            v1_load_weight();  /* 进入子界面1默认先载入 1_1（weightset） */
            break;
        case VIEW_2:
            make_placeholder("View 2");
            break;
        case VIEW_3:
            make_placeholder("View 3 (Home)");
            break;
        case VIEW_4:
            make_placeholder("View 4");
            break;
        case VIEW_5:
            make_placeholder("View 5");
            break;
        default: break;
    }
}

/* 底部按钮事件 */
static void nav_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    view_id_t id = (view_id_t)(uintptr_t)lv_event_get_user_data(e);
    LV_UNUSED(btn);
    if (e->code == LV_EVENT_CLICKED) {
        switch_view(id);
    }
}

/* ================== 导航栏创建 ================== */
static void create_navbar(void)
{
    /* 底部导航容器 */
    s_nav = lv_obj_create(s_root);
    lv_obj_set_size(s_nav, UI_W, UI_NAV_H);
    lv_obj_align(s_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_nav, lv_color_hex(COL_NAV_BG), 0);
    lv_obj_set_style_bg_opa(s_nav, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_nav, LV_OBJ_FLAG_SCROLLABLE);

    /* 四个矩形按钮尺寸与位置 */
    const int btn_w = 170;
    const int btn_h = UI_NAV_H - 18;     /* 上下留 9px 透气 */
    const int gap_x = 14;

    /* 左侧两个矩形按钮：1、2 */
    s_btn1 = make_nav_rect(s_nav, "1", btn_w, btn_h);
    lv_obj_align(s_btn1, LV_ALIGN_LEFT_MID, gap_x, 0);
    lv_obj_add_event_cb(s_btn1, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_1);

    s_btn2 = make_nav_rect(s_nav, "2", btn_w, btn_h);
    lv_obj_align_to(s_btn2, s_btn1, LV_ALIGN_OUT_RIGHT_MID, gap_x, 0);
    lv_obj_add_event_cb(s_btn2, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_2);

    /* 右侧两个矩形按钮：4、5 */
    s_btn5 = make_nav_rect(s_nav, "5", btn_w, btn_h);
    lv_obj_align(s_btn5, LV_ALIGN_RIGHT_MID, -gap_x, 0);
    lv_obj_add_event_cb(s_btn5, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_5);

    s_btn4 = make_nav_rect(s_nav, "4", btn_w, btn_h);
    lv_obj_align_to(s_btn4, s_btn5, LV_ALIGN_OUT_LEFT_MID, -gap_x, 0);
    lv_obj_add_event_cb(s_btn4, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_4);

    /* 中间圆形 Home（子界面3）——半嵌到底栏中 */
    const int home_d = 72;
    s_btn3 = make_nav_home(s_root, home_d);
    lv_obj_align(s_btn3, LV_ALIGN_BOTTOM_MID, 0, -UI_NAV_H/2);
    lv_obj_add_event_cb(s_btn3, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)VIEW_3);
}

/* ================== 对外：创建与默认显示 ================== */
void app_ui_create(lv_obj_t *parent_screen_or_container)
{
    if (s_root) return;

    /* 根容器 */
    s_root = lv_obj_create(parent_screen_or_container);
    lv_obj_set_size(s_root, UI_W, UI_H);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_root, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_root, LV_SCROLLBAR_MODE_OFF);

    /* 内容容器（800×400） */
    s_content = lv_obj_create(s_root);
    lv_obj_set_size(s_content, UI_W, UI_CONTENT_H);
    lv_obj_align(s_content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_content, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(s_content, LV_OPA_TRANSP, 0);   /* 透明背景 */

    /* 底部导航栏 */
    create_navbar();

    /* 默认显示：子界面3（Home） */
    switch_view(VIEW_3);
}

/* 如果你需要提供一个销毁接口 */
void app_ui_destroy(void)
{
    if (!s_root) return;
    clear_content();
    lv_obj_del(s_root);
    s_root = s_content = s_nav = NULL;
    s_btn1 = s_btn2 = s_btn3 = s_btn4 = s_btn5 = NULL;
    s_current = VIEW_NONE;
    s_v1_mode = V1_MODE_WEIGHT;
}

/* 可选：外部供调用的“显示某个子界面”接口 */
void app_ui_show(view_id_t id)
{
    switch_view(id);
}

/* ========== 你如果需要 app_ui.h，可参考下面最小头文件 ==========
#ifndef APP_UI_H
#define APP_UI_H
#ifdef __cplusplus
extern "C" {
#endif
void app_ui_create(lv_obj_t *parent);
void app_ui_destroy(void);
#ifdef __cplusplus
}
#endif
#endif // APP_UI_H
============================================================= */
