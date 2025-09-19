#include "ui_left1.h"
#include "ui_keypad.h"

#include <stdbool.h>

/** 左右两列设定值、时间、起止时间等运行参数。 */
static int  s_left_set[5]     = {0, 0, 10, 0, 0};
static int  s_left_time_s[5]  = {10, 10, 10, 10, 10};
static int  s_right_set[5]    = {0, 0, 0, 0, 0};
static int  s_right_time_s[5] = {10, 10, 10, 10, 10};
static int  s_start_sec[5]    = {0};
static int  s_end_sec[5]      = {0};
static bool s_row_enable[5]   = {false};

/** 与数值按钮绑定的描述条目。 */
static ui_bind_entry_t s_left_set_entries[5];
static ui_bind_entry_t s_left_time_entries[5];
static ui_bind_entry_t s_right_set_entries[5];
static ui_bind_entry_t s_right_time_entries[5];
static ui_bind_entry_t s_start_entries[5];
static ui_bind_entry_t s_end_entries[5];

/** 页面使用到的对象缓存。 */
static lv_obj_t *s_panel_a = NULL;
static lv_obj_t *s_panel_b = NULL;
static lv_obj_t *s_switch_btn = NULL;
static bool      s_show_panel_a = true;

/**
 * @brief 复选框状态变化回调，直接同步到数组。
 */
static void checkbox_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    bool *var = (bool *)lv_event_get_user_data(e);
    if(var) {
        *var = lv_obj_has_state(cb, LV_STATE_CHECKED);
    }
}

/**
 * @brief 构造一个绑定九键输入的按钮。
 */
static lv_obj_t *create_value_button(lv_obj_t *parent,
                                     ui_bind_entry_t *entry,
                                     int *bind_var,
                                     const char *suffix,
                                     bool is_time,
                                     uint8_t max_digits)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 40);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_center(label);

    ui_keypad_entry_init(entry, label, bind_var, suffix, is_time, max_digits);
    ui_keypad_bind_button(btn, entry);

    return btn;
}

/**
 * @brief 构建左侧“Set/Time”双列表格。
 */
static void build_panel_a(lv_obj_t *parent)
{
    s_panel_a = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_a, 800, 400);
    lv_obj_align(s_panel_a, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *t1 = lv_label_create(s_panel_a);
    lv_label_set_text(t1, "left  set                              time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 105, 10);

    lv_obj_t *t2 = lv_label_create(s_panel_a);
    lv_label_set_text(t2, "right set                             time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -80, 10);

    int row_cnt = 5;
    int top_margin = 50;
    int available_h = 370 - top_margin - 10;
    int row_space = available_h / row_cnt;

    for(int r = 0; r < row_cnt; r++) {
        int y = top_margin + r * row_space;

        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r + 1);
        lv_obj_align(idx, LV_ALIGN_TOP_LEFT, 60, y);

        lv_obj_t *btn_set = create_value_button(s_panel_a, &s_left_set_entries[r],
                                                &s_left_set[r], "kg", false, 3);
        lv_obj_align(btn_set, LV_ALIGN_TOP_LEFT, 80, y - 10);

        lv_obj_t *btn_time = create_value_button(s_panel_a, &s_left_time_entries[r],
                                                 &s_left_time_s[r], "s", false, 3);
        lv_obj_align(btn_time, LV_ALIGN_TOP_LEFT, 240, y - 10);
    }

    for(int r = 0; r < row_cnt; r++) {
        int y = top_margin + r * row_space;

        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r + 1);
        lv_obj_align(idx, LV_ALIGN_TOP_LEFT, 420, y);

        lv_obj_t *btn_set = create_value_button(s_panel_a, &s_right_set_entries[r],
                                                &s_right_set[r], "kg", false, 3);
        lv_obj_align(btn_set, LV_ALIGN_TOP_LEFT, 440, y - 10);

        lv_obj_t *btn_time = create_value_button(s_panel_a, &s_right_time_entries[r],
                                                 &s_right_time_s[r], "s", false, 3);
        lv_obj_align(btn_time, LV_ALIGN_TOP_LEFT, 600, y - 10);
    }
}

/**
 * @brief 构建右侧“开始/结束时间 + 复选框”表格。
 */
static void build_panel_b(lv_obj_t *parent)
{
    s_panel_b = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_b, 800, 400);
    lv_obj_align(s_panel_b, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *t1 = lv_label_create(s_panel_b);
    lv_label_set_text(t1, "start time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 160, 10);

    lv_obj_t *t2 = lv_label_create(s_panel_b);
    lv_label_set_text(t2, "end time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -260, 10);

    int row_cnt = 5;
    int top_margin = 50;
    int available_h = 400 - top_margin - 20;
    int row_space = available_h / row_cnt;

    for(int r = 0; r < row_cnt; r++) {
        int y = top_margin + r * row_space;

        lv_obj_t *idx_l = lv_label_create(s_panel_b);
        lv_label_set_text_fmt(idx_l, "%d.", r + 1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 120, y);

        lv_obj_t *btn_start = create_value_button(s_panel_b, &s_start_entries[r],
                                                  &s_start_sec[r], NULL, true, 4);
        lv_obj_align(btn_start, LV_ALIGN_TOP_LEFT, 160, y - 10);

        lv_obj_t *btn_end = create_value_button(s_panel_b, &s_end_entries[r],
                                                &s_end_sec[r], NULL, true, 4);
        lv_obj_align(btn_end, LV_ALIGN_TOP_RIGHT, -260, y - 10);

        lv_obj_t *cb = lv_checkbox_create(s_panel_b);
        lv_checkbox_set_text(cb, "");
        lv_obj_align(cb, LV_ALIGN_TOP_RIGHT, -120, y);
        lv_obj_add_event_cb(cb, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, &s_row_enable[r]);
        if(s_row_enable[r]) {
            lv_obj_add_state(cb, LV_STATE_CHECKED);
        }
    }

    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 切换按钮的事件，切换显示 PanelA/PanelB。
 */
static void switch_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_show_panel_a = !s_show_panel_a;

    if(s_panel_a && s_panel_b) {
        if(s_show_panel_a) {
            lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_left1_cleanup(void)
{
    ui_keypad_close();

    if(s_switch_btn) {
        lv_obj_del(s_switch_btn);
        s_switch_btn = NULL;
    }

    s_panel_a = NULL;
    s_panel_b = NULL;
    s_show_panel_a = true;
}

void ui_left1_create(lv_obj_t *center_container)
{
    if(!center_container) {
        return;
    }

    build_panel_a(center_container);
    build_panel_b(center_container);

    if(s_switch_btn) {
        lv_obj_del(s_switch_btn);
    }

    s_switch_btn = lv_btn_create(lv_layer_top());
    lv_obj_set_size(s_switch_btn, 64, 64);
    lv_obj_set_style_radius(s_switch_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_switch_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(s_switch_btn, switch_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_label_create(s_switch_btn);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_center(icon);
}
