/***********************
 * ui_left1.c（带注释）
 * 作用：
 *  - “左一界面”的两套子页 Panel A / Panel B
 *  - Panel A：左右各 5 行的“set/time”设置，带分页（右侧上下翻页 + 页码）
 *  - Panel B：start/end time + 复选框，同样按分页显示，但不出现右侧翻页控件
 *  - 右下角一个圆形按钮用来在 A/B 两个子页间切换
 ***********************/

#include "ui_left1.h"   // 对外声明（本模块的创建、清理、对外API等）
#include "ui_keypad.h"  // 九键输入法绑定（数值弹窗编辑）

#include <stdbool.h>
#include <stdio.h>

/* ====== 可扩展容量与分页配置 ====== */
/* 系统最多支持的“阀/行”数量（左右两列共用同一索引） */
#define MAX_VALVES     32
/* 每页显示的行数（A/B 两个子页都按这个分页显示） */
#define ROWS_PER_PAGE  5

/* ====== 阀门总数 & 分页状态 ====== */
/* 当前实际需要显示的行数（1..MAX_VALVES），可由外部通过 ui_left1_set_valve1_count 动态设置 */
static int  s_valve_count = 5;
/* 当前页号（0 起），0 表示最新/第一页 */
static int  s_page_idx    = 0;

/* 计算总页数：ceil(s_valve_count / ROWS_PER_PAGE) */
static inline int page_count(void) {
    return (s_valve_count + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
}
/* 当前页第一个数据索引（含） */
static inline int page_first_index(void) { return s_page_idx * ROWS_PER_PAGE; }
/* 当前页最后一个数据索引（不含），并裁剪到 s_valve_count 以内 */
static inline int page_last_index(void)  {
    int last = page_first_index() + ROWS_PER_PAGE;
    if(last > s_valve_count) last = s_valve_count;
    return last;
}

/* ====== 数据区（模型） ======
 * 左右两列各有 set（重量）和 time（秒）；Panel B 用 start/end（以秒计），
 * s_row_enable 表示该行是否勾选（启用）。
 */
static int  s_left_set[MAX_VALVES]     = {0};
static int  s_left_time_s[MAX_VALVES]  = { [0 ... MAX_VALVES-1] = 10 };
static int  s_right_set[MAX_VALVES]    = {0};
static int  s_right_time_s[MAX_VALVES] = { [0 ... MAX_VALVES-1] = 10 };
static int  s_start_sec[MAX_VALVES]    = {0};
static int  s_end_sec[MAX_VALVES]      = {0};
static bool s_row_enable[MAX_VALVES]   = {false};


/* ===== 对外数据读取：实现部分 ===== */

int ui_left1_get_valve1_count(void) {
    return s_valve_count;
}

const int* ui_left1_left_set_data(void)     { return s_left_set;     }
const int* ui_left1_left_time_data(void)    { return s_left_time_s;  }
const int* ui_left1_right_set_data(void)    { return s_right_set;    }
const int* ui_left1_right_time_data(void)   { return s_right_time_s; }
const int* ui_left1_start_time_data(void)   { return s_start_sec;    }
const int* ui_left1_end_time_data(void)     { return s_end_sec;      }
const bool* ui_left1_row_enable_data(void)  { return s_row_enable;   }

/* 复制当前页（或任意 first_index 起连续数据）的快照到调用者缓冲区，返回拷贝条数 */
int ui_left1_snapshot_pageA(int first_index, int* l_set, int* l_time,
                            int* r_set, int* r_time, int max_n)
{
    if(first_index < 0) first_index = 0;
    if(first_index >= s_valve_count) return 0;
    int n = s_valve_count - first_index;
    if(n > max_n) n = max_n;
    for(int i=0;i<n;i++){
        if(l_set)  l_set[i]  = s_left_set[first_index + i];
        if(l_time) l_time[i] = s_left_time_s[first_index + i];
        if(r_set)  r_set[i]  = s_right_set[first_index + i];
        if(r_time) r_time[i] = s_right_time_s[first_index + i];
    }
    return n;
}

int ui_left1_snapshot_pageB(int first_index, int* start_s, int* end_s,
                            bool* enabled, int max_n)
{
    if(first_index < 0) first_index = 0;
    if(first_index >= s_valve_count) return 0;
    int n = s_valve_count - first_index;
    if(n > max_n) n = max_n;
    for(int i=0;i<n;i++){
        if(start_s) start_s[i] = s_start_sec[first_index + i];
        if(end_s)   end_s[i]   = s_end_sec[first_index + i];
        if(enabled) enabled[i] = s_row_enable[first_index + i];
    }
    return n;
}

/* 与 ui_keypad 交互的“绑定条目”，每一行一个，负责把“变量 ↔ label 显示”绑定起来 */
static ui_bind_entry_t s_left_set_entries[MAX_VALVES];
static ui_bind_entry_t s_left_time_entries[MAX_VALVES];
static ui_bind_entry_t s_right_set_entries[MAX_VALVES];
static ui_bind_entry_t s_right_time_entries[MAX_VALVES];
static ui_bind_entry_t s_start_entries[MAX_VALVES];
static ui_bind_entry_t s_end_entries[MAX_VALVES];

/* 页面对象缓存 */
static lv_obj_t *s_panel_a     = NULL;   /* Panel A：重量设置 / 秒设置（左右两列） */
static lv_obj_t *s_panel_b     = NULL;   /* Panel B：start/end + 复选框 */
static lv_obj_t *s_switch_btn  = NULL;   /* 右下角 A/B 切换按钮（漂浮在顶层） */
static bool      s_show_panel_a = true;  /* 当前是否显示 Panel A（否则显示 Panel B） */

/* ====== 右侧翻页控件（仅 Panel A 里可见） ====== */
static lv_obj_t *s_page_ctrl   = NULL;   /* 包着页码 + 上/下按钮的竖直容器 */
static lv_obj_t *s_btn_up      = NULL;   /* 上一页按钮 */
static lv_obj_t *s_btn_down    = NULL;   /* 下一页按钮 */
static lv_obj_t *s_page_label  = NULL;   /* “page X/Y” 文本标签 */
static bool      s_page_label_on_top = true; /* 页码文本在翻页按钮上方（true）/下方（false） */

/* ========== 回调与基础 ========== */
/* 复选框回调：把 UI 状态写回 bool 变量 */
static void checkbox_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    bool *var = (bool *)lv_event_get_user_data(e);
    if(var) *var = lv_obj_has_state(cb, LV_STATE_CHECKED);
}

/**
 * @brief 生成一个“数值按钮”，点击后弹九键输入框。
 * @param parent    父对象
 * @param entry     对应的 ui_keypad 绑定条目（由本函数初始化）
 * @param bind_var  绑定的整型变量地址
 * @param suffix    后缀（如 "kg"、"s"），可为 NULL
 * @param is_time   是否时间模式（true=mm:ss 显示，var 以秒存）
 * @param max_digits 输入数字最大长度（时间模式一般用 4）
 * @return 返回按钮对象（其子 label 会自动创建）
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

    /* 绑定：把 label/var/suffix/is_time/max_digits 填入 entry，并刷新初始显示 */
    ui_keypad_entry_init(entry, label, bind_var, suffix, is_time, max_digits);
    /* 点击按钮打开数字键盘 */
    ui_keypad_bind_button(btn, entry);
    return btn;
}

/* ========== 页码/按钮：状态与文本刷新 ========== */
/* 刷新“page X/Y” 文本 */
static void update_page_label(void)
{
    if(!s_page_label) return;
    int pc = page_count();
    if(pc <= 0) pc = 1;
    char buf[32];
    snprintf(buf, sizeof(buf), "page %d/%d", s_page_idx + 1, pc);
    lv_label_set_text(s_page_label, buf);
}

/* 根据是否多页、当前页位置，更新上下翻页按钮的“禁用/启用”状态，以及整个翻页控件的显隐 */
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

/* 翻页核心：delta = -1 上一页；+1 下一页 */
static void page_change(int delta)
{
    int pc = page_count();
    if(pc <= 1) return;  /* 只有一页就不用翻 */

    int new_idx = s_page_idx + delta;
    if(new_idx < 0) new_idx = 0;
    if(new_idx >= pc) new_idx = pc - 1;
    if(new_idx == s_page_idx) return;

    s_page_idx = new_idx;

    /* 重建当前页 UI（A 页显示翻页控件，B 页没有） */
    /* 在本文件后面有对应的实现，这里提前声明原型以消除编译器告警 */
    extern void rebuild_panel_a_rows(void);
    extern void rebuild_panel_b_rows(void);
    rebuild_panel_a_rows();
    /* 如果 B 页也需要随页切换重建（当前逻辑按 A 页的分页），可以一并重建；不需要可省略 */
    rebuild_panel_b_rows();

    update_page_label();
    update_page_buttons_state();
}

/* 上/下翻页按钮事件 */
static void btn_up_event_cb(lv_event_t *e)   { LV_UNUSED(e); page_change(-1); }
static void btn_down_event_cb(lv_event_t *e) { LV_UNUSED(e); page_change(+1); }

/* 可选：面板接收键盘 ↑/↓ 键，用于物理按键翻页 */
static void key_event_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_UP)        page_change(-1);
    else if(key == LV_KEY_DOWN) page_change(+1);
}

/* ========== Panel A（重量设置）行重建 ========== */
/* 根据当前页，清空 Panel A 并重建该页的所有行 */
void rebuild_panel_a_rows(void)
{
    if(!s_panel_a) return;
    lv_obj_clean(s_panel_a);

    /* 顶部两段标题文本（左右列） */
    lv_obj_t *t1 = lv_label_create(s_panel_a);
    lv_label_set_text(t1, "left  set                              time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 85, 10);   /* 适当向左收一点 */

    lv_obj_t *t2 = lv_label_create(s_panel_a);
    lv_label_set_text(t2, "right set                             time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -100, 10);/* 同理向左收一点 */

    const int first = page_first_index();
    const int last  = page_last_index();
    const int row_cnt = (last > first) ? (last - first) : 0;

    /* 行距计算（按可用高度等分） */
    int top_margin = 50;
    int available_h = 370 - top_margin - 10;
    int row_space = row_cnt > 0 ? (available_h / row_cnt) : available_h;

    for(int i = 0; i < row_cnt; i++) {
        int idx = first + i;                    /* 实际数据行号（0..s_valve_count-1） */
        int y   = top_margin + i * row_space;   /* 本行的 Y 基准 */

        /* 左列：编号 + set(kg) + time(s) */
        lv_obj_t *idx_l = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx_l, "%d.", idx + 1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 40, y);

        lv_obj_t *btn_set_l = create_value_button(
            s_panel_a, &s_left_set_entries[idx],
            &s_left_set[idx], "kg", false, 3
        );
        lv_obj_align(btn_set_l, LV_ALIGN_TOP_LEFT, 60, y - 10);

        lv_obj_t *btn_time_l = create_value_button(
            s_panel_a, &s_left_time_entries[idx],
            &s_left_time_s[idx], "s", false, 3
        );
        lv_obj_align(btn_time_l, LV_ALIGN_TOP_LEFT, 220, y - 10);

        /* 右列：编号 + set(kg) + time(s) */
        lv_obj_t *idx_r = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx_r, "%d.", idx + 1);
        lv_obj_align(idx_r, LV_ALIGN_TOP_LEFT, 400, y);

        lv_obj_t *btn_set_r = create_value_button(
            s_panel_a, &s_right_set_entries[idx],
            &s_right_set[idx], "kg", false, 3
        );
        lv_obj_align(btn_set_r, LV_ALIGN_TOP_LEFT, 420, y - 10);

        lv_obj_t *btn_time_r = create_value_button(
            s_panel_a, &s_right_time_entries[idx],
            &s_right_time_s[idx], "s", false, 3
        );
        lv_obj_align(btn_time_r, LV_ALIGN_TOP_LEFT, 580, y - 10);
    }
}

/* ========== Panel B（上料/时间）行重建：不显示翻页控件 ========== */
/* 清空 Panel B，并按当前页重建 start/end + 复选框 行 */
void rebuild_panel_b_rows(void)
{
    if(!s_panel_b) return;
    lv_obj_clean(s_panel_b);

    /* 顶部标题（start time / end time） */
    lv_obj_t *t1 = lv_label_create(s_panel_b);
    lv_label_set_text(t1, "start time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 140, 10);

    lv_obj_t *t2 = lv_label_create(s_panel_b);
    lv_label_set_text(t2, "end time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -280, 10);

    const int first = page_first_index();
    const int last  = page_last_index();
    const int row_cnt = (last > first) ? (last - first) : 0;

    int top_margin = 50;
    int available_h = 400 - top_margin - 20;
    int row_space = row_cnt > 0 ? (available_h / row_cnt) : available_h;

    for(int i = 0; i < row_cnt; i++) {
        int idx = first + i;
        int y   = top_margin + i * row_space;

        /* 编号 */
        lv_obj_t *idx_l = lv_label_create(s_panel_b);
        lv_label_set_text_fmt(idx_l, "%d.", idx + 1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 100, y);

        /* start / end 两个时间按钮（mm:ss，变量以“秒”存） */
        lv_obj_t *btn_start = create_value_button(
            s_panel_b, &s_start_entries[idx],
            &s_start_sec[idx], NULL, true, 4
        );
        lv_obj_align(btn_start, LV_ALIGN_TOP_LEFT, 140, y - 10);

        lv_obj_t *btn_end = create_value_button(
            s_panel_b, &s_end_entries[idx],
            &s_end_sec[idx], NULL, true, 4
        );
        lv_obj_align(btn_end, LV_ALIGN_TOP_RIGHT, -280, y - 10);

        /* 复选框：是否启用该行 */
        lv_obj_t *cb = lv_checkbox_create(s_panel_b);
        lv_checkbox_set_text(cb, "");
        lv_obj_align(cb, LV_ALIGN_TOP_RIGHT, -140, y);
        lv_obj_add_event_cb(cb, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, &s_row_enable[idx]);
        if(s_row_enable[idx]) lv_obj_add_state(cb, LV_STATE_CHECKED);
    }

    /* Panel B 默认隐藏（入口处会控制显示/隐藏） */
    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
}

/* ====== 构建右侧翻页控件列（只在 Panel A 使用；容器内含：页码文本 + 上/下按钮） ====== */
static void build_page_ctrl(lv_obj_t *parent)
{
    if(s_page_ctrl) {
        lv_obj_del(s_page_ctrl);
        s_page_ctrl = NULL;
    }

    s_page_ctrl = lv_obj_create(parent);
    lv_obj_set_size(s_page_ctrl, 90, 220);          /* 右侧窄竖条 */
    lv_obj_align(s_page_ctrl, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_clear_flag(s_page_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    /* 页码文本（位置可在上/下两种样式间切换） */
    s_page_label = lv_label_create(s_page_ctrl);
    lv_label_set_text(s_page_label, "page 1/1");
    if(s_page_label_on_top) lv_obj_align(s_page_label, LV_ALIGN_TOP_MID, 0, 0);
    else                    lv_obj_align(s_page_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* 上翻按钮（圆形） */
    s_btn_up = lv_btn_create(s_page_ctrl);
    lv_obj_set_size(s_btn_up, 64, 64);
    lv_obj_set_style_radius(s_btn_up, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(s_btn_up, btn_up_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(s_btn_up, LV_ALIGN_CENTER, 0, s_page_label_on_top ? -40 : -20);

    lv_obj_t *icon_up = lv_label_create(s_btn_up);
    lv_label_set_text(icon_up, LV_SYMBOL_UP);
    lv_obj_center(icon_up);

    /* 下翻按钮（圆形） */
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

/* ====== A/B 界面切换（右下角圆形按钮） ======
 * 只切换显示标志，并保持“右侧翻页控件仅在 Panel A 可见”的策略。
 */
static void switch_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_show_panel_a = !s_show_panel_a;

    if(s_panel_a && s_panel_b) {
        if(s_show_panel_a) {
            lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
            if(s_page_ctrl) lv_obj_clear_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
            if(s_page_ctrl) lv_obj_add_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ====== 对外 API：设置总行数并重建界面（分页会自动调整） ====== */
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

/* 对外 API：设置“页码显示文本”是在（上方 true）还是（下方 false） */
void ui_left1_set_page_indicator1_top(bool on_top)
{
    s_page_label_on_top = on_top;
    if(s_page_label) {
        if(on_top) lv_obj_align(s_page_label, LV_ALIGN_TOP_MID, 0, 0);
        else       lv_obj_align(s_page_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

/* ====== 面板构建：A/B 两个子容器 ====== */
static void build_panel_a(lv_obj_t *parent)
{
    s_panel_a = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_a, 800, 400);              /* 你的中心区域尺寸 */
    lv_obj_align(s_panel_a, LV_ALIGN_TOP_MID, 0, 0);
    /* 为了能接收键盘事件（↑/↓），加入可聚焦与按键事件回调 */
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

/* ====== 生命周期：清理 / 创建 ====== */
/* 清理：关闭数字键盘、删除顶层按钮/右侧栏、重置状态；
 * 注意：不删除传入的父容器；外部在切换页面时会先 lv_obj_clean(center_container)。
 */
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

/**
 * @brief 创建“左一界面”的两个子页，并挂到给定的 center_container
 * @param center_container 主程序提供的中心显示区域（800x400）
 */
void ui_left1_create(lv_obj_t *center_container)
{
    if(!center_container) return;

    /* 构建两个子页 */
    build_panel_a(center_container);
    build_panel_b(center_container);

    /* 默认显示 Panel A，隐藏 Panel B */
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);

    /* 右下角 A/B 切换按钮（放到顶层，避免被底部导航遮盖） */
    if(s_switch_btn) lv_obj_del(s_switch_btn);
    s_switch_btn = lv_btn_create(lv_layer_top());
    lv_obj_set_size(s_switch_btn, 64, 64);
    lv_obj_set_style_radius(s_switch_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_switch_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(s_switch_btn, switch_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *icon = lv_label_create(s_switch_btn);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_center(icon);

    /* 右侧竖栏：页码 + 上下按钮（仅 Panel A 可见） */
    build_page_ctrl(center_container);
    if(!s_show_panel_a && s_page_ctrl) lv_obj_add_flag(s_page_ctrl, LV_OBJ_FLAG_HIDDEN);

    update_page_label();
    update_page_buttons_state();

    LV_LOG_USER("ui 1 create success");

#if LV_USE_GROUP
    /* 可选：加入默认 group，这样外接按键可用于翻页（↑/↓） */
    lv_group_t *g = lv_group_get_default();
    if(g) {
        lv_group_add_obj(g, s_panel_a);
        lv_group_add_obj(g, s_panel_b);
        lv_group_focus_obj(s_panel_a);
    }
#endif
}

/* ======（仅供本文件内部提前声明用） ======
void rebuild_panel_a_rows(void);
void rebuild_panel_b_rows(void);
*/
