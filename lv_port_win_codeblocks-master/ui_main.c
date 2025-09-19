#include "ui_main.h"
#include "ui_left1.h"

#include <stdint.h>

/** 主内容区域的容器指针。 */
static lv_obj_t *s_center_container = NULL;

/** 底部导航栏按钮索引定义。 */
#define NAV_IDX_FIRST_PAGE 0

/**
 * @brief 底部导航按钮事件。
 */
static void nav_btn_event_cb(lv_event_t *e)
{
    uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
    if(!s_center_container) {
        return;
    }

    ui_left1_cleanup();

    switch(idx) {
    case NAV_IDX_FIRST_PAGE:
        lv_obj_clean(s_center_container);
        ui_left1_create(s_center_container);
        break;

    default:
        lv_obj_clean(s_center_container);
        lv_obj_t *hint = lv_label_create(s_center_container);
        lv_label_set_text_fmt(hint, "Page %d (TBD)", (int)idx + 1);
        lv_obj_center(hint);
        break;
    }
}

/**
 * @brief Home 按钮点击事件，目前仅作调试输出。
 */
static void menu_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);

    if(!s_center_container) {
        return;
    }

    ui_left1_cleanup();
    lv_obj_clean(s_center_container);
}

void ui_main_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    if(!s_center_container) {
        s_center_container = lv_obj_create(scr);
        lv_obj_set_size(s_center_container, 800, 400);
        lv_obj_align(s_center_container, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(s_center_container, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t *nav_bar = lv_obj_create(scr);
    lv_obj_set_size(nav_bar, 800, 80);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_palette_lighten(LV_PALETTE_BLUE, 3), LV_PART_MAIN);
    lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);

    for(int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(nav_bar);
        lv_obj_set_size(btn, 120, 50);

        if(i < 2) {
            lv_obj_align(btn, LV_ALIGN_LEFT_MID, 20 + i * 140, 0);
        } else {
            lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -20 - (i - 2) * 140, 0);
        }

        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "Btn%d", i + 1);
        lv_obj_center(label);
    }

    lv_obj_t *home_btn = lv_btn_create(scr);
    lv_obj_set_size(home_btn, 80, 80);
    lv_obj_set_style_radius(home_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(home_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(home_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *home_label = lv_label_create(home_btn);
    lv_label_set_text(home_label, LV_SYMBOL_HOME);
    lv_obj_center(home_label);
}
