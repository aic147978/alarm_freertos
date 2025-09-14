#include "myGUI.h"
#include "lvgl/lvgl.h"
#include <stdio.h>

/* ======== Event callbacks ======== */
static void nav_btn_event_cb(lv_event_t * e)
{
    uint32_t idx = (uint32_t)lv_event_get_user_data(e);
    LV_LOG_USER("Navigation button %d clicked", idx);
    /* TODO: switch to corresponding interface */
}

static void menu_btn_event_cb(lv_event_t * e)
{
    LV_LOG_USER("Main menu button clicked");
    /* TODO: open main menu */
}

/* ======== Main UI creation ======== */
void my_GUI(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Top-right time label */
    lv_obj_t *time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "time:00/00/00");
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    /* Left panel with labels */
    lv_obj_t *left_panel = lv_obj_create(scr);
    lv_obj_set_size(left_panel, 160, 360);
    lv_obj_set_style_bg_color(left_panel, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 10, -20);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *temp_label = lv_label_create(left_panel);
    lv_label_set_text(temp_label, "temp:");
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *day_label = lv_label_create(left_panel);
    lv_label_set_text(day_label, "day:");
    lv_obj_align(day_label, LV_ALIGN_TOP_LEFT, 10, 40);

    /* Center container (for future information) */
    lv_obj_t *center_container = lv_obj_create(scr);
    lv_obj_set_size(center_container, 600, 360);
    lv_obj_align(center_container, LV_ALIGN_RIGHT_MID, -10, -20);
    lv_obj_clear_flag(center_container, LV_OBJ_FLAG_SCROLLABLE);

    /* Bottom navigation bar */
    lv_obj_t *nav_bar = lv_obj_create(scr);
    lv_obj_set_size(nav_bar, 800, 80);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);
    lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);

    const char *btn_texts[6] = { "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6" };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(nav_bar);
        lv_obj_set_size(btn, 80, 60);
        if (i < 3) {
            lv_obj_align(btn, LV_ALIGN_LEFT_MID, 10 + i * 110, 0);
        } else {
            lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -10 - (i - 3) * 110, 0);
        }
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, btn_texts[i]);
        lv_obj_center(label);
    }

    /* Central circular main menu button */
    lv_obj_t *menu_btn = lv_btn_create(scr);
    lv_obj_set_size(menu_btn, 80, 80);
    lv_obj_set_style_radius(menu_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(menu_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(menu_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *menu_label = lv_label_create(menu_btn);
    lv_label_set_text(menu_label, "Menu");
    lv_obj_center(menu_label);
}

