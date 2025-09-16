#include "myGUI.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdint.h>

/* 中央内容容器，用于切换主界面和子界面 */
static lv_obj_t *g_center_container = NULL;

/* 子界面按钮事件回调，后续扩展 */
static void sub_btn_event_cb(lv_event_t *e)
{
    uintptr_t id = (uintptr_t)lv_event_get_user_data(e);
    LV_LOG_USER("Option %lu clicked", (unsigned long)id);
    /* 预留功能实现位置 */
}

/* 加载子界面：title 为标题，btn_cnt 为按钮数量 */
static void load_sub_ui(const char *title, uint32_t btn_cnt)
{
    if(g_center_container == NULL) return;

    /* 清空中央容器内容 */
    lv_obj_clean(g_center_container);

    /* 按钮网格区域 */
    lv_obj_t *btn_cont = lv_obj_create(g_center_container);
    lv_obj_set_size(btn_cont,
                    lv_obj_get_width(g_center_container) - 20,
                    lv_obj_get_height(g_center_container) - 80);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_layout(btn_cont, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_row(btn_cont, 10, 0);    /* 行间距 */
    lv_obj_set_style_pad_column(btn_cont, 10, 0); /* 列间距 */
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);

    /* 顶部标题栏 */
    lv_obj_t *header = lv_obj_create(g_center_container);
    lv_obj_set_size(header, lv_obj_get_width(g_center_container), 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(header);
    lv_label_set_text(title_label, title);
    lv_obj_center(title_label);
    lv_obj_move_foreground(header);

    /* 固定 4 列 × 3 行 */
    static const lv_coord_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    static const lv_coord_t row_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(btn_cont, col_dsc, row_dsc);

    /* 创建子界面按钮 */
    for(uint32_t i = 0; i < btn_cnt; i++) {
        lv_obj_t *btn = lv_btn_create(btn_cont);
        lv_obj_set_size(btn, 150, 60); /* 固定大小，保证一致 */
        lv_obj_add_event_cb(btn, sub_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(i + 1));

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "选项%d", i + 1);
        lv_obj_center(label);

        /* 放到网格里 */
        lv_obj_set_grid_cell(btn,
                             LV_GRID_ALIGN_CENTER, i % 4, 1,
                             LV_GRID_ALIGN_CENTER, i / 4, 1);
    }
}

/* 加载主界面 */
static void load_main_ui(void)
{
    if(g_center_container == NULL) return;
    lv_obj_clean(g_center_container);

    /* Top-right time label */
    lv_obj_t *time_label = lv_label_create(g_center_container);
    lv_label_set_text(time_label, "time:00/00/00");
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 5);

    /* Left panel with labels */
    lv_obj_t *left_panel = lv_obj_create(g_center_container);
    lv_obj_set_size(left_panel, 160, 340);
    lv_obj_set_style_bg_color(left_panel,
                              lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 10, -20);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *temp_label = lv_label_create(left_panel);
    lv_label_set_text(temp_label, "temp:");
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *day_label = lv_label_create(left_panel);
    lv_label_set_text(day_label, "day:");
    lv_obj_align(day_label, LV_ALIGN_TOP_LEFT, 10, 40);

    /* Center container (for future information) */
    lv_obj_t *center_container = lv_obj_create(g_center_container);
    lv_obj_set_size(center_container, 580, 300);
    lv_obj_align(center_container, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_clear_flag(center_container, LV_OBJ_FLAG_SCROLLABLE);
}

/* 底部导航栏按钮事件回调 */
static void nav_btn_event_cb(lv_event_t *e)
{
    uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
    LV_LOG_USER("Navigation button %lu clicked", (unsigned long)idx);

    switch(idx) {
    case 0:
        load_sub_ui("界面1", 6);
        break;
    case 1:
        load_sub_ui("界面2", 12);
        break;
    case 2:
        load_sub_ui("界面3", 8);
        break;
    default:
        break;
    }
}

/* 中央菜单按钮事件回调，返回主界面 */
static void menu_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Main menu button clicked");
    load_main_ui();
}

/* ======== 主界面创建 ======== */
void my_GUI(void)
{
    /* 获取当前屏幕并禁止滚动 */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);


    /* 子界面容器 */
    g_center_container = lv_obj_create(scr);
    lv_obj_set_size(g_center_container, 800, 400);
    lv_obj_align(g_center_container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(g_center_container, LV_OBJ_FLAG_SCROLLABLE);

    /* 底部导航栏 */
    lv_obj_t *nav_bar = lv_obj_create(scr);
    lv_obj_set_size(nav_bar, 800, 80);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);
    lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 六个导航按钮文字 */
    const char *btn_texts[6] = { "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6" };
    for (int i = 0; i < 6; i++) {
        /* 创建导航按钮 */
        lv_obj_t *btn = lv_btn_create(nav_bar);
        lv_obj_set_size(btn, 80, 60);
        if (i < 3) {
            /* 左侧三个按钮 */
            lv_obj_align(btn, LV_ALIGN_LEFT_MID, 10 + i * 110, 0);
        } else {
            /* 右侧三个按钮 */
            lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -10 - (i - 3) * 110, 0);
        }
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, btn_texts[i]);
        lv_obj_center(label);
    }

    /* 中间圆形菜单按钮 */
    lv_obj_t *menu_btn = lv_btn_create(scr);
    lv_obj_set_size(menu_btn, 80, 80);
    lv_obj_set_style_radius(menu_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(menu_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(menu_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *menu_label = lv_label_create(menu_btn);
    lv_label_set_text(menu_label, "Menu");
    lv_obj_center(menu_label);

    /* 加载默认主界面 */
    load_main_ui();
}
