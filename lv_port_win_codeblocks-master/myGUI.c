#include "myGUI.h"
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* ===================== 屏幕与布局常量 ===================== */
#define SCREEN_WIDTH                800
#define SCREEN_HEIGHT               480
#define SCREEN_MARGIN               10
#define TOP_PANEL_OFFSET            60
#define LEFT_PANEL_WIDTH            200
#define RIGHT_PANEL_WIDTH           180
#define BOTTOM_PANEL_HEIGHT         120

#define OUTPUT_CASE_COUNT           11
#define INPUT_CASE_COUNT            6
#define MAX_CASE_COUNT              OUTPUT_CASE_COUNT

#define MODULE_COUNT                6
#define MAX_OPTION_COUNT            40
#define MODULE_GRID_MAX_COL         6

/* ===================== 类型定义 ===================== */
typedef enum {
    CASE_TYPE_OUTPUT = 0,
    CASE_TYPE_INPUT,
    CASE_TYPE_TOTAL
} case_type_t;

typedef enum {
    MODULE_TEMP = 0,
    MODULE_ANALOG_OUTPUT,
    MODULE_RS485,
    MODULE_DELAY,
    MODULE_ANALOG_INPUT,
    MODULE_BOOL_INPUT,
    MODULE_TYPE_TOTAL = MODULE_COUNT
} module_type_t;

typedef struct {
    const char *name;
    uint32_t option_count;
} module_info_t;

/* ===================== 全局静态变量 ===================== */
static const module_info_t g_module_infos[MODULE_COUNT] = {
    { "temp", 10 },
    { "analog output", 8 },
    { "RS485", 3 },
    { "DELAY", 40 },
    { "analog input", 8 },
    { "bool input", 5 }
};

static const char *g_case_type_labels[CASE_TYPE_TOTAL] = { "output", "input" };
static const uint8_t g_case_type_counts[CASE_TYPE_TOTAL] = { OUTPUT_CASE_COUNT, INPUT_CASE_COUNT };

static lv_obj_t *g_center_container = NULL;             /* 中间按钮显示区域 */
static lv_obj_t *g_case_lists[CASE_TYPE_TOTAL] = { NULL }; /* 左侧小选项容器 */
static lv_obj_t *g_case_buttons[CASE_TYPE_TOTAL][MAX_CASE_COUNT] = { NULL };
static lv_obj_t *g_case_status_labels[CASE_TYPE_TOTAL][MAX_CASE_COUNT] = { NULL };
static lv_obj_t *g_case_toggle_btns[CASE_TYPE_TOTAL] = { NULL };
static lv_obj_t *g_module_btns[MODULE_COUNT] = { NULL };

static case_type_t g_active_case_type = CASE_TYPE_OUTPUT;
static uint8_t g_active_case_index = 0;
static module_type_t g_active_module = MODULE_TEMP;

static uint8_t g_option_states[CASE_TYPE_TOTAL][MAX_CASE_COUNT][MODULE_COUNT][MAX_OPTION_COUNT] = { 0 };

/* ===================== 工具函数声明 ===================== */
static void refresh_center_buttons(void);
static void update_case_type_display(void);
static void update_case_selection(void);
static void update_module_highlight(void);
static void set_case_button_visual(lv_obj_t *btn, bool selected);
static void set_toggle_button_visual(lv_obj_t *btn, bool selected);
static void set_module_button_visual(lv_obj_t *btn, bool selected);
static void set_option_button_visual(lv_obj_t *btn, bool selected);

/* ===================== 事件回调 ===================== */
static void exit_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Exit button clicked, return to previous page");
    /* 根据实际项目可在此切换回上级界面 */
}

static void case_toggle_event_cb(lv_event_t *e)
{
    case_type_t type = (case_type_t)(uintptr_t)lv_event_get_user_data(e);
    if(type >= CASE_TYPE_TOTAL) return;

    if(g_active_case_type != type) {
        g_active_case_type = type;
        g_active_case_index = 0;
        /* 根据类型选择默认功能模块 */
        if(type == CASE_TYPE_OUTPUT) {
            g_active_module = MODULE_TEMP;
        } else {
            g_active_module = MODULE_ANALOG_INPUT;
        }
        LV_LOG_USER("Switch to %s list", g_case_type_labels[type]);
        update_case_type_display();
        update_module_highlight();
        refresh_center_buttons();
    }
}

static void case_button_event_cb(lv_event_t *e)
{
    uint32_t index = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    uint32_t max_count = g_case_type_counts[g_active_case_type];
    if(index >= max_count) return;

    g_active_case_index = (uint8_t)index;
    LV_LOG_USER("Select %s case%u", g_case_type_labels[g_active_case_type], (unsigned int)(index + 1));
    update_case_selection();
    refresh_center_buttons();
}

static void module_button_event_cb(lv_event_t *e)
{
    module_type_t module = (module_type_t)(uintptr_t)lv_event_get_user_data(e);
    if(module >= MODULE_TYPE_TOTAL) return;

    g_active_module = module;
    LV_LOG_USER("Enter module %s", g_module_infos[module].name);
    update_module_highlight();
    refresh_center_buttons();
}

static void option_button_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint32_t option_index = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(option_index >= MAX_OPTION_COUNT) return;

    uint8_t *state = &g_option_states[g_active_case_type][g_active_case_index][g_active_module][option_index];
    *state = *state ? 0 : 1;
    set_option_button_visual(btn, *state != 0);

    LV_LOG_USER("Toggle %s case%u %s option%u -> %s",
                g_case_type_labels[g_active_case_type],
                (unsigned int)(g_active_case_index + 1),
                g_module_infos[g_active_module].name,
                (unsigned int)(option_index + 1),
                *state ? "ON" : "OFF");
}

/* ===================== 样式更新函数 ===================== */
static void set_label_text_color_for_children(lv_obj_t *parent, lv_color_t color)
{
    if(parent == NULL) return;
    lv_obj_t *child = lv_obj_get_child(parent, 0);
    while(child) {
        lv_obj_set_style_text_color(child, color, LV_PART_MAIN);
        child = lv_obj_get_next(child);
    }
}

static void set_case_button_visual(lv_obj_t *btn, bool selected)
{
    if(btn == NULL) return;
    lv_color_t bg_color = selected ? lv_palette_main(LV_PALETTE_BLUE) : lv_palette_lighten(LV_PALETTE_GREY, 4);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
    set_label_text_color_for_children(btn, selected ? lv_color_white() : lv_color_black());
}

static void set_toggle_button_visual(lv_obj_t *btn, bool selected)
{
    if(btn == NULL) return;
    lv_color_t bg_color = selected ? lv_palette_main(LV_PALETTE_BLUE) : lv_palette_lighten(LV_PALETTE_GREY, 3);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
    set_label_text_color_for_children(btn, selected ? lv_color_white() : lv_color_black());
}

static void set_module_button_visual(lv_obj_t *btn, bool selected)
{
    if(btn == NULL) return;
    lv_color_t bg_color = selected ? lv_palette_main(LV_PALETTE_BLUE) : lv_palette_lighten(LV_PALETTE_GREY, 2);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    set_label_text_color_for_children(btn, selected ? lv_color_white() : lv_color_black());
}

static void set_option_button_visual(lv_obj_t *btn, bool selected)
{
    if(btn == NULL) return;
    if(selected) {
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        set_label_text_color_for_children(btn, lv_color_white());
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_palette_lighten(LV_PALETTE_GREY, 2), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        set_label_text_color_for_children(btn, lv_color_black());
    }
}

/* ===================== UI 更新函数 ===================== */
static void update_case_type_display(void)
{
    for(uint8_t i = 0; i < CASE_TYPE_TOTAL; i++) {
        if(g_case_toggle_btns[i]) {
            set_toggle_button_visual(g_case_toggle_btns[i], g_active_case_type == (case_type_t)i);
        }
        if(g_case_lists[i]) {
            if(i == g_active_case_type) {
                lv_obj_clear_flag(g_case_lists[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(g_case_lists[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    update_case_selection();
}

static void update_case_selection(void)
{
    for(uint8_t type = 0; type < CASE_TYPE_TOTAL; type++) {
        uint8_t max_count = g_case_type_counts[type];
        for(uint8_t i = 0; i < max_count; i++) {
            bool selected = (type == g_active_case_type) && (i == g_active_case_index);
            set_case_button_visual(g_case_buttons[type][i], selected);
        }
    }
}

static void update_module_highlight(void)
{
    for(uint8_t i = 0; i < MODULE_COUNT; i++) {
        set_module_button_visual(g_module_btns[i], g_active_module == (module_type_t)i);
    }
}

static void refresh_center_buttons(void)
{
    if(g_center_container == NULL) return;

    lv_obj_clean(g_center_container);

    /* 标题显示当前选择 */
    char title[64];
    lv_snprintf(title, sizeof(title), "%s - %s case%u",
                g_module_infos[g_active_module].name,
                g_case_type_labels[g_active_case_type],
                (unsigned int)(g_active_case_index + 1));

    lv_obj_t *title_label = lv_label_create(g_center_container);
    lv_label_set_text(title_label, title);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 10, 10);

    uint32_t btn_cnt = g_module_infos[g_active_module].option_count;
    if(btn_cnt == 0) return;

    lv_obj_t *grid_cont = lv_obj_create(g_center_container);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid_cont, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(grid_cont, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_column(grid_cont, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_cont, 0, LV_PART_MAIN);
    lv_obj_set_size(grid_cont, LV_PCT(100), lv_obj_get_height(g_center_container) - 50);
    lv_obj_align(grid_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_layout(grid_cont, LV_LAYOUT_GRID);
    lv_obj_clear_flag(grid_cont, LV_OBJ_FLAG_SCROLLABLE);

    uint32_t col_cnt = 1;
    if(btn_cnt >= 24) {
        col_cnt = 6;
    } else if(btn_cnt >= 16) {
        col_cnt = 5;
    } else if(btn_cnt >= 9) {
        col_cnt = 4;
    } else if(btn_cnt >= 5) {
        col_cnt = 3;
    } else {
        col_cnt = btn_cnt;
    }
    if(col_cnt == 0) col_cnt = 1;
    uint32_t row_cnt = (btn_cnt + col_cnt - 1) / col_cnt;

    static lv_coord_t col_dsc[MODULE_GRID_MAX_COL + 1];
    static lv_coord_t row_dsc[MAX_OPTION_COUNT + 1];
    for(uint32_t i = 0; i < MODULE_GRID_MAX_COL + 1; i++) {
        col_dsc[i] = LV_GRID_TEMPLATE_LAST;
    }
    for(uint32_t i = 0; i < MAX_OPTION_COUNT + 1; i++) {
        row_dsc[i] = LV_GRID_TEMPLATE_LAST;
    }
    for(uint32_t i = 0; i < col_cnt; i++) {
        col_dsc[i] = LV_GRID_FR(1);
    }
    col_dsc[col_cnt] = LV_GRID_TEMPLATE_LAST;
    for(uint32_t i = 0; i < row_cnt; i++) {
        row_dsc[i] = LV_GRID_FR(1);
    }
    row_dsc[row_cnt] = LV_GRID_TEMPLATE_LAST;

    lv_obj_set_grid_dsc_array(grid_cont, col_dsc, row_dsc);

    for(uint32_t i = 0; i < btn_cnt; i++) {
        lv_obj_t *btn = lv_btn_create(grid_cont);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_0, LV_PART_MAIN);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, option_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_set_grid_cell(btn,
                             LV_GRID_ALIGN_STRETCH, i % col_cnt, 1,
                             LV_GRID_ALIGN_STRETCH, i / col_cnt, 1);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%u", (unsigned int)(i + 1));
        lv_obj_center(label);

        bool selected = g_option_states[g_active_case_type][g_active_case_index][g_active_module][i] != 0;
        set_option_button_visual(btn, selected);
    }
}

/* ===================== 左侧小选项构建 ===================== */
static void create_case_buttons(lv_obj_t *parent, case_type_t type)
{
    if(parent == NULL) return;
    uint8_t count = g_case_type_counts[type];

    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(parent, 6, LV_PART_MAIN);

    for(uint8_t i = 0; i < count; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, 40);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_left(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_right(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_top(btn, 5, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(btn, 5, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_0, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, case_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *name_label = lv_label_create(btn);
        lv_label_set_text_fmt(name_label, "case%u", (unsigned int)(i + 1));
        lv_obj_align(name_label, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *status_label = lv_label_create(btn);
        lv_label_set_text(status_label, "00/00");
        lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, 0, 0);

        g_case_buttons[type][i] = btn;
        g_case_status_labels[type][i] = status_label;
    }
}

/* ===================== 功能模块按钮构建 ===================== */
static lv_obj_t *create_module_button(lv_obj_t *parent, module_type_t module, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, module_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)module);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, g_module_infos[module].name);
    lv_obj_center(label);

    g_module_btns[module] = btn;
    return btn;
}

/* ===================== 主界面构建 ===================== */
void my_GUI(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(scr, SCREEN_WIDTH, SCREEN_HEIGHT);

    /* 左上角退出按钮 */
    lv_obj_t *exit_btn = lv_btn_create(scr);
    lv_obj_set_size(exit_btn, 100, 40);
    lv_obj_align(exit_btn, LV_ALIGN_TOP_LEFT, SCREEN_MARGIN, SCREEN_MARGIN);
    lv_obj_set_style_radius(exit_btn, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(exit_btn, LV_OPA_0, LV_PART_MAIN);
    lv_obj_add_event_cb(exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, LV_SYMBOL_LEFT " 退出");
    lv_obj_center(exit_label);

    /* 左侧边栏 */
    lv_obj_t *left_panel = lv_obj_create(scr);
    lv_obj_set_size(left_panel, LEFT_PANEL_WIDTH, SCREEN_HEIGHT - TOP_PANEL_OFFSET - SCREEN_MARGIN);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, SCREEN_MARGIN, TOP_PANEL_OFFSET);
    lv_obj_set_style_bg_color(left_panel, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_style_border_width(left_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(left_panel, 10, LV_PART_MAIN);

    /* output / input 切换按钮 */
    g_case_toggle_btns[CASE_TYPE_OUTPUT] = lv_btn_create(left_panel);
    lv_obj_set_size(g_case_toggle_btns[CASE_TYPE_OUTPUT], 80, 36);
    lv_obj_align(g_case_toggle_btns[CASE_TYPE_OUTPUT], LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_radius(g_case_toggle_btns[CASE_TYPE_OUTPUT], 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_case_toggle_btns[CASE_TYPE_OUTPUT], LV_OPA_0, LV_PART_MAIN);
    lv_obj_add_event_cb(g_case_toggle_btns[CASE_TYPE_OUTPUT], case_toggle_event_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)CASE_TYPE_OUTPUT);
    lv_obj_t *output_label = lv_label_create(g_case_toggle_btns[CASE_TYPE_OUTPUT]);
    lv_label_set_text(output_label, "output");
    lv_obj_center(output_label);

    g_case_toggle_btns[CASE_TYPE_INPUT] = lv_btn_create(left_panel);
    lv_obj_set_size(g_case_toggle_btns[CASE_TYPE_INPUT], 80, 36);
    lv_obj_align(g_case_toggle_btns[CASE_TYPE_INPUT], LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_radius(g_case_toggle_btns[CASE_TYPE_INPUT], 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_case_toggle_btns[CASE_TYPE_INPUT], LV_OPA_0, LV_PART_MAIN);
    lv_obj_add_event_cb(g_case_toggle_btns[CASE_TYPE_INPUT], case_toggle_event_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)CASE_TYPE_INPUT);
    lv_obj_t *input_label = lv_label_create(g_case_toggle_btns[CASE_TYPE_INPUT]);
    lv_label_set_text(input_label, "input");
    lv_obj_center(input_label);

    /* output / input 小选项列表 */
    g_case_lists[CASE_TYPE_OUTPUT] = lv_obj_create(left_panel);
    lv_obj_set_size(g_case_lists[CASE_TYPE_OUTPUT], LV_PCT(100),
                    lv_obj_get_height(left_panel) - 50);
    lv_obj_align(g_case_lists[CASE_TYPE_OUTPUT], LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_bg_color(g_case_lists[CASE_TYPE_OUTPUT], lv_palette_lighten(LV_PALETTE_GREY, 5), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_case_lists[CASE_TYPE_OUTPUT], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_case_lists[CASE_TYPE_OUTPUT], 0, LV_PART_MAIN);
    create_case_buttons(g_case_lists[CASE_TYPE_OUTPUT], CASE_TYPE_OUTPUT);

    g_case_lists[CASE_TYPE_INPUT] = lv_obj_create(left_panel);
    lv_obj_set_size(g_case_lists[CASE_TYPE_INPUT], LV_PCT(100),
                    lv_obj_get_height(left_panel) - 50);
    lv_obj_align(g_case_lists[CASE_TYPE_INPUT], LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_bg_color(g_case_lists[CASE_TYPE_INPUT], lv_palette_lighten(LV_PALETTE_GREY, 5), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_case_lists[CASE_TYPE_INPUT], 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_case_lists[CASE_TYPE_INPUT], 0, LV_PART_MAIN);
    create_case_buttons(g_case_lists[CASE_TYPE_INPUT], CASE_TYPE_INPUT);

    /* 中间显示区域 */
    lv_coord_t center_width = SCREEN_WIDTH - (SCREEN_MARGIN * 3) - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH;
    lv_coord_t center_height = SCREEN_HEIGHT - BOTTOM_PANEL_HEIGHT - SCREEN_MARGIN * 2;
    g_center_container = lv_obj_create(scr);
    lv_obj_set_size(g_center_container, center_width, center_height);
    lv_obj_align(g_center_container, LV_ALIGN_TOP_LEFT,
                 LEFT_PANEL_WIDTH + SCREEN_MARGIN * 2, SCREEN_MARGIN);
    lv_obj_set_style_bg_color(g_center_container, lv_palette_lighten(LV_PALETTE_GREY, 5), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_center_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_center_container, 10, LV_PART_MAIN);

    /* 右侧功能模块容器 */
    lv_obj_t *right_module_panel = lv_obj_create(scr);
    lv_obj_set_size(right_module_panel, RIGHT_PANEL_WIDTH,
                    SCREEN_HEIGHT - BOTTOM_PANEL_HEIGHT - SCREEN_MARGIN * 2);
    lv_obj_align(right_module_panel, LV_ALIGN_TOP_RIGHT, -SCREEN_MARGIN, SCREEN_MARGIN);
    lv_obj_set_style_bg_color(right_module_panel, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);
    lv_obj_set_style_border_width(right_module_panel, 0, LV_PART_MAIN);
    lv_obj_set_layout(right_module_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_module_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_module_panel,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(right_module_panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(right_module_panel, 12, LV_PART_MAIN);

    /* 底部功能模块容器 */
    lv_obj_t *bottom_module_panel = lv_obj_create(scr);
    lv_obj_set_size(bottom_module_panel, center_width,
                    BOTTOM_PANEL_HEIGHT - SCREEN_MARGIN);
    lv_obj_align(bottom_module_panel, LV_ALIGN_BOTTOM_LEFT,
                 LEFT_PANEL_WIDTH + SCREEN_MARGIN * 2, -SCREEN_MARGIN);
    lv_obj_set_style_bg_color(bottom_module_panel, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom_module_panel, 0, LV_PART_MAIN);
    lv_obj_set_layout(bottom_module_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_module_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_module_panel,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(bottom_module_panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(bottom_module_panel, 12, LV_PART_MAIN);

    /* 创建功能模块按钮：下方四个 */
    lv_coord_t bottom_btn_width = (lv_obj_get_width(bottom_module_panel) - 12 * 3) / 4;
    if(bottom_btn_width < 80) bottom_btn_width = 80;
    for(uint8_t i = MODULE_TEMP; i <= MODULE_DELAY; i++) {
        create_module_button(bottom_module_panel, (module_type_t)i, bottom_btn_width, 60);
    }

    /* 右侧两个模块按钮 */
    lv_coord_t right_btn_height = 60;
    create_module_button(right_module_panel, MODULE_ANALOG_INPUT, LV_PCT(100), right_btn_height);
    create_module_button(right_module_panel, MODULE_BOOL_INPUT, LV_PCT(100), right_btn_height);

    /* 初始化显示状态 */
    update_case_type_display();
    update_module_highlight();
    refresh_center_buttons();
}
