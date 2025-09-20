#include "ui_left1.h"
#include "ui_keypad.h"

#include <stdbool.h>
#include <stdio.h>

/* ====== 可扩展容量与分页配置 ====== */
#define MAX_VALVES     32
#define ROWS_PER_PAGE  5

/* ====== 阀门总数 & 分页状态 ====== */
static int  s_valve_count = 5;    /* 外部可调 */
static int  s_page_idx    = 0;    /* 0-based */

static inline int page_count(void) {
    return (s_valve_count + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
}
static inline int page_first_index(void) { return s_page_idx * ROWS_PER_PAGE; }
static inline int page_last_index(void)  {
    int last = page_first_index() + ROWS_PER_PAGE;
    if(last > s_valve_count) last = s_valve_count;
    return last;
}

/* ====== 数据区 ====== */
static int  s_left_set[MAX_VALVES]     = {0};
static int  s_left_time_s[MAX_VALVES]  = { [0 ... MAX_VALVES-1] = 10 };
static int  s_right_set[MAX_VALVES]    = {0};
static int  s_right_time_s[MAX_VALVES] = { [0 ... MAX_VALVES-1] = 10 };
static int  s_start_sec[MAX_VALVES]    = {0};
static int  s_end_sec[MAX_VALVES]      = {0};
static bool s_row_enable[MAX_VALVES]   = {false};

/* 输入绑定条目 */
static ui_bind_entry_t s_left_set_entries[MAX_VALVES];
static ui_bind_entry_t s_left_time_entries[MAX_VALVES];
static ui_bind_entry_t s_right_set_entries[MAX_VALVES];
static ui_bind_entry_t s_right_time_entries[MAX_VALVES];
static ui_bind_entry_t s_start_entries[MAX_VALVES];
static ui_bind_entry_t s_end_entries[MAX_VALVES];

/* 页面对象缓存 */
static lv_obj_t *s_panel_a     = NULL;   /* 重量设置界面（有页码与翻页按钮） */
static lv_obj_t *s_panel_b     = NULL;   /* 上料/时间配置界面（无页码与翻页按钮） */
static lv_obj_t *s_switch_btn  = NULL;   /* A/B切换按钮（沿用） */
static bool      s_show_panel_a = true;

/* ====== 新增：右侧翻页控件（只在 Panel A 显示） ====== */
static lv_obj_t *s_page_ctrl   = NULL;   /* 右侧竖直容器 */
static lv_obj_t *s_btn_up      = NULL;
static lv_obj_t *s_btn_down    = NULL;
static lv_obj_t *s_page_label  = NULL;   /* “page X/Y” */
static bool      s_page_label_on_top = true; /* 页码在按钮上方/下方 */

/* ========== 回调与基础 ========== */
static void checkbox_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    bool *var = (bool *)lv_event_get_user_data(e);
    if(var) *var = lv_obj_has_state(cb, LV_STATE_CHECKED);
}

/* 九键输入按钮封装 */
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

/* ========== 页码/按钮：状态与文本刷新 ========== */
static void update_page_label(void)
{
    if(!s_page_label) return;
    int pc = page_count();
    if(pc <= 0) pc = 1;
    char buf[32];
    snprintf(buf, sizeof(buf), "page %d/%d", s_page_idx + 1, pc);
    lv_label_set_text(s_page_label, buf);
}

static void update_page_buttons_state(void)
{
    bool multi = page_count() > 1;
    if(s_btn_up) {
        if(!multi || s_page_idx <= 0) lv_obj_add_state(s_btn_up, LV_STATE_DISABLED);
        else                          lv_obj_clear_state(s_btn_up, LV_STATE_DISABLED);
    }
    if(s_btn_down) {
        if(!multi || s_page_idx >= page_count() - 1) lv_obj_add_state(s_btn_down, LV_STATE_DISABLED);
        else                                         lv_obj_clear_state(s_btn_down, LV_STATE_DISABLED);
    }
    if(s_page_ctrl) {
        if(multi) lv_obj_clear_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);
        else      lv_obj_add_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 翻页核心 */
static void page_change(int delta)
{
    int pc = page_count();
    if(pc <= 1) return;

    int new_idx = s_page_idx + delta;
    if(new_idx < 0) new_idx = 0;
    if(new_idx >= pc) new_idx = pc - 1;
    if(new_idx == s_page_idx) return;

    s_page_idx = new_idx;
    /* 重建当前页UI */
    if(s_panel_a) {
        /* 仅重建Panel A内容（Panel B无分页控件） */
        /* 为了保持清晰，单独调用重建函数 */
    }
    /* 用专用重建函数刷新两个Panel的行 */
    /* 这两个函数在后面定义 */
    extern void rebuild_panel_a_rows(void);
    extern void rebuild_panel_b_rows(void);
    rebuild_panel_a_rows();
    rebuild_panel_b_rows(); /* 若B页用到分页数据，视需要可保留；否则可不重建 */

    update_page_label();
    update_page_buttons_state();
}

/* 按钮事件 */
static void btn_up_event_cb(lv_event_t *e)   { LV_UNUSED(e); page_change(-1); }
static void btn_down_event_cb(lv_event_t *e) { LV_UNUSED(e); page_change(+1); }

/* 键盘↑/↓也支持（可选） */
static void key_event_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_UP)      page_change(-1);
    else if(key == LV_KEY_DOWN) page_change(+1);
}

/* ========== Panel A（重量设置）行重建 ========== */
void rebuild_panel_a_rows(void)
{
    if(!s_panel_a) return;
    lv_obj_clean(s_panel_a);

    /* 标题（整体左移约20px） */
    lv_obj_t *t1 = lv_label_create(s_panel_a);
    lv_label_set_text(t1, "left  set                              time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 85, 10);   /* 原105 -> 85 */

    lv_obj_t *t2 = lv_label_create(s_panel_a);
    lv_label_set_text(t2, "right set                             time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -100, 10);/* 原-80 -> -100，使整体更靠左 */

    const int first = page_first_index();
    const int last  = page_last_index();
    const int row_cnt = (last > first) ? (last - first) : 0;

    int top_margin = 50;
    int available_h = 370 - top_margin - 10;
    int row_space = row_cnt > 0 ? (available_h / row_cnt) : available_h;

    for(int i = 0; i < row_cnt; i++) {
        int idx = first + i;
        int y   = top_margin + i * row_space;

        /* 左列（整体左移20px） */
        lv_obj_t *idx_l = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx_l, "%d.", idx + 1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 40, y);          /* 原60 -> 40 */

        lv_obj_t *btn_set_l = create_value_button(s_panel_a, &s_left_set_entries[idx],
                                                  &s_left_set[idx], "kg", false, 3);
        lv_obj_align(btn_set_l, LV_ALIGN_TOP_LEFT, 60, y - 10); /* 原80 -> 60 */

        lv_obj_t *btn_time_l = create_value_button(s_panel_a, &s_left_time_entries[idx],
                                                   &s_left_time_s[idx], "s", false, 3);
        lv_obj_align(btn_time_l, LV_ALIGN_TOP_LEFT, 220, y - 10);/* 原240 -> 220 */

        /* 右列（整体左移20px） */
        lv_obj_t *idx_r = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx_r, "%d.", idx + 1);
        lv_obj_align(idx_r, LV_ALIGN_TOP_LEFT, 400, y);         /* 原420 -> 400 */

        lv_obj_t *btn_set_r = create_value_button(s_panel_a, &s_right_set_entries[idx],
                                                  &s_right_set[idx], "kg", false, 3);
        lv_obj_align(btn_set_r, LV_ALIGN_TOP_LEFT, 420, y - 10);/* 原440 -> 420 */

        lv_obj_t *btn_time_r = create_value_button(s_panel_a, &s_right_time_entries[idx],
                                                   &s_right_time_s[idx], "s", false, 3);
        lv_obj_align(btn_time_r, LV_ALIGN_TOP_LEFT, 580, y - 10);/* 原600 -> 580 */
    }
}

/* ========== Panel B（上料/时间）行重建：不显示翻页控件 ========== */
void rebuild_panel_b_rows(void)
{
    if(!s_panel_b) return;
    lv_obj_clean(s_panel_b);

    lv_obj_t *t1 = lv_label_create(s_panel_b);
    lv_label_set_text(t1, "start time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 140, 10);   /* 左移20：原160 -> 140 */

    lv_obj_t *t2 = lv_label_create(s_panel_b);
    lv_label_set_text(t2, "end time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -280, 10); /* 左移20：原-260 -> -280 */

    const int first = page_first_index();
    const int last  = page_last_index();
    const int row_cnt = (last > first) ? (last - first) : 0;

    int top_margin = 50;
    int available_h = 400 - top_margin - 20;
    int row_space = row_cnt > 0 ? (available_h / row_cnt) : available_h;

    for(int i = 0; i < row_cnt; i++) {
        int idx = first + i;
        int y   = top_margin + i * row_space;

        lv_obj_t *idx_l = lv_label_create(s_panel_b);
        lv_label_set_text_fmt(idx_l, "%d.", idx + 1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 100, y);          /* 左移20：原120 -> 100 */

        lv_obj_t *btn_start = create_value_button(s_panel_b, &s_start_entries[idx],
                                                  &s_start_sec[idx], NULL, true, 4);
        lv_obj_align(btn_start, LV_ALIGN_TOP_LEFT, 140, y - 10); /* 左移20：原160 -> 140 */

        lv_obj_t *btn_end = create_value_button(s_panel_b, &s_end_entries[idx],
                                                &s_end_sec[idx], NULL, true, 4);
        lv_obj_align(btn_end, LV_ALIGN_TOP_RIGHT, -280, y - 10); /* 左移20：原-260 -> -280 */

        lv_obj_t *cb = lv_checkbox_create(s_panel_b);
        lv_checkbox_set_text(cb, "");
        lv_obj_align(cb, LV_ALIGN_TOP_RIGHT, -140, y);           /* 左移20：原-120 -> -140 */
        lv_obj_add_event_cb(cb, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, &s_row_enable[idx]);
        if(s_row_enable[idx]) lv_obj_add_state(cb, LV_STATE_CHECKED);
    }

    /* Panel B 默认隐藏（按原逻辑） */
    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
}

/* ====== 构建右侧翻页控件列（只在 Panel A 使用） ====== */
static void build_page_ctrl(lv_obj_t *parent)
{
    if(s_page_ctrl) {
        lv_obj_del(s_page_ctrl);
        s_page_ctrl = NULL;
    }

    s_page_ctrl = lv_obj_create(parent);
    lv_obj_set_size(s_page_ctrl, 90, 220);  /* 窄竖条 */
    lv_obj_align(s_page_ctrl, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_clear_flag(s_page_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    /* 页码 */
    s_page_label = lv_label_create(s_page_ctrl);
    lv_label_set_text(s_page_label, "page 1/1");
    if(s_page_label_on_top) lv_obj_align(s_page_label, LV_ALIGN_TOP_MID, 0, 0);
    else                    lv_obj_align(s_page_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* 上翻按钮 */
    s_btn_up = lv_btn_create(s_page_ctrl);
    lv_obj_set_size(s_btn_up, 64, 64);
    lv_obj_set_style_radius(s_btn_up, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(s_btn_up, btn_up_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(s_btn_up, LV_ALIGN_CENTER, 0, s_page_label_on_top ? -40 : -20);

    lv_obj_t *icon_up = lv_label_create(s_btn_up);
    lv_label_set_text(icon_up, LV_SYMBOL_UP);
    lv_obj_center(icon_up);

    /* 下翻按钮 */
    s_btn_down = lv_btn_create(s_page_ctrl);
    lv_obj_set_size(s_btn_down, 64, 64);
    lv_obj_set_style_radius(s_btn_down, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(s_btn_down, btn_down_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(s_btn_down, LV_ALIGN_CENTER, 0, s_page_label_on_top ? +40 : +20);

    lv_obj_t *icon_dn = lv_label_create(s_btn_down);
    lv_label_set_text(icon_dn, LV_SYMBOL_DOWN);
    lv_obj_center(icon_dn);

    update_page_label();
    update_page_buttons_state();
}

/* ====== A/B界面切换（保持“翻页控件仅在Panel A可见”） ====== */
static void switch_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_show_panel_a = !s_show_panel_a;

    if(s_panel_a && s_panel_b) {
        if(s_show_panel_a) {
            lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
            if(s_page_ctrl) lv_obj_clear_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN); /* 只在A显示 */
        } else {
            lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
            if(s_page_ctrl) lv_obj_add_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);  /* B隐藏 */
        }
    }
}

/* ====== 对外API ====== */
void ui_left1_set_valve1_count(int n)
{
    if(n < 1) n = 1;
    if(n > MAX_VALVES) n = MAX_VALVES;
    s_valve_count = n;

    if(s_page_idx >= page_count()) s_page_idx = page_count() - 1;
    if(s_page_idx < 0) s_page_idx = 0;

    rebuild_panel_a_rows();
    rebuild_panel_b_rows();
    update_page_label();
    update_page_buttons_state();
}

/* 页码在按钮上/下切换 */
void ui_left1_set_page_indicator1_top(bool on_top)
{
    s_page_label_on_top = on_top;
    if(s_page_label) {
        if(on_top) lv_obj_align(s_page_label, LV_ALIGN_TOP_MID, 0, 0);
        else       lv_obj_align(s_page_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

/* ====== 面板构建 ====== */
static void build_panel_a(lv_obj_t *parent)
{
    s_panel_a = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_a, 800, 400);
    lv_obj_align(s_panel_a, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(s_panel_a, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(s_panel_a, key_event_cb, LV_EVENT_KEY, NULL);

    rebuild_panel_a_rows();
}

static void build_panel_b(lv_obj_t *parent)
{
    s_panel_b = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_b, 800, 400);
    lv_obj_align(s_panel_b, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(s_panel_b, key_event_cb, LV_EVENT_KEY, NULL);

    rebuild_panel_b_rows();
}

/* ====== 生命周期 ====== */
void ui_left1_cleanup(void)
{
    ui_keypad_close();

    if(s_switch_btn) { lv_obj_del(s_switch_btn); s_switch_btn = NULL; }
    if(s_page_ctrl)  { lv_obj_del(s_page_ctrl);  s_page_ctrl  = NULL; }
    s_btn_up = s_btn_down = s_page_label = NULL;

    s_panel_a = NULL;
    s_panel_b = NULL;
    s_show_panel_a = true;
    s_page_idx = 0;
}

void ui_left1_create(lv_obj_t *center_container)
{
    if(!center_container) return;

    build_panel_a(center_container);
    build_panel_b(center_container);

    /* 默认显示Panel A，Panel B隐藏 */
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);

    /* 右下角A/B切换按钮（原逻辑保留） */
    if(s_switch_btn) lv_obj_del(s_switch_btn);
    s_switch_btn = lv_btn_create(lv_layer_top());
    lv_obj_set_size(s_switch_btn, 64, 64);
    lv_obj_set_style_radius(s_switch_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_switch_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(s_switch_btn, switch_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *icon = lv_label_create(s_switch_btn);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_center(icon);

    /* 右侧竖列：页码与上下按钮（仅Panel A可见） */
    build_page_ctrl(center_container);
    if(!s_show_panel_a && s_page_ctrl) lv_obj_add_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);

    update_page_label();
    update_page_buttons_state();

#if LV_USE_GROUP
    /* 可选：加入默认group以接收↑/↓键 */
    lv_group_t *g = lv_group_get_default();
    if(g) {
        lv_group_add_obj(g, s_panel_a);
        lv_group_add_obj(g, s_panel_b);
        lv_group_focus_obj(s_panel_a);
    }
#endif
}

/* ====== 对外声明（供本文件内部引用静态符号） ======
void rebuild_panel_a_rows(void);
void rebuild_panel_b_rows(void);
 */
