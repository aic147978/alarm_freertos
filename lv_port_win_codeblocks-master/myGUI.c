
#include "myGUI.h"
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

lv_obj_t *g_center_container = NULL;


extern void ui_left1_create(lv_obj_t *center_container);   // 左一按钮对应的界面
static void create_ui(void);                             // 你的主界面加载函数
extern lv_obj_t *g_center_container;                        // 中央容器

static int g_left_set[5]      = { 0, 0, 10, 0, 0 };   // kg
static int g_left_time_s[5]   = {10,10,10,10,10};   // 秒
static int g_right_set[5]     = { 0, 0, 0, 0, 0 };
static int g_right_time_s[5]  = {10,10,10,10,10};

static int g_start_sec[5]     = {0};                // mm:ss 存成“秒”
static int g_end_sec[5]       = {0};
static bool g_row_enable[5]   = {false};            // 复选框是否打勾


typedef struct {
    lv_obj_t *label;     // 界面上的 label
    int      *var;       // 绑定的变量地址
    const char *suffix;  // 单位 (如 "kg" / "s")
    bool is_time;        // 是否是时间
} bind_entry_t;

static bool s_block_click = false;

static void unblock_cb(lv_timer_t *t){
    s_block_click = false;
    lv_timer_del(t);
}
/* 可选：如果“左一界面”在顶层建了悬浮控件（如切换圆钮/弹窗），
   在它的实现里提供这个清理钩子；没有就忽略。 */
__attribute__((weak)) void ui_left1_cleanup(void) {}

/* 自定义映射：哪个底栏按钮进左一界面、哪个回主界面 */
#define NAV_IDX_FIRST_PAGE   0
#define NAV_IDX_GO_HOME      5

static void set_label_from_var(bind_entry_t *e)
{
    if(!e || !e->label || !e->var) return;
    char buf[16];

    if(e->is_time) {
        int mm = (*e->var) / 60;
        int ss = (*e->var) % 60;
        lv_snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
    } else {
        if(e->suffix) lv_snprintf(buf, sizeof(buf), "%d%s", *e->var, e->suffix);
        else          lv_snprintf(buf, sizeof(buf), "%d", *e->var);
    }
    lv_label_set_text(e->label, buf);
}

static void eat_all_cb(lv_event_t *e){
    // 吞掉所有事件，防止继续传给下层
//    lv_event_stop_bubbling(e);
//    lv_event_stop_processing(e);
}

static void nav_btn_event_cb(lv_event_t * e)
{
    uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
    LV_LOG_USER("nav btn %lu clicked", (unsigned long)idx);
    if(!g_center_container) return;

    /* 离开当前页面前的清理（如删除顶层悬浮控件、关闭弹窗等） */
    ui_left1_cleanup();

    switch(idx) {
    case NAV_IDX_FIRST_PAGE:      /* 左边第一个按钮 → 进入对应界面 */
        lv_obj_clean(g_center_container);                 // 清空中央区域
        ui_left1_create(g_center_container);              // 构建左一界面
        break;

    case NAV_IDX_GO_HOME:         /* 回到主界面 */
        lv_obj_clean(g_center_container);
       create_ui();
        break;

    default:                      /* 其它按钮：占位（以后再接入） */
    {
        lv_obj_clean(g_center_container);
        lv_obj_t *hint = lv_label_create(g_center_container);
        lv_label_set_text_fmt(hint, "Page %d (TBD)", (int)idx + 1);
        lv_obj_center(hint);
        break;
    }
    }
}


static void cb_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    bool *var = (bool*)lv_event_get_user_data(e);
    if(var) *var = lv_obj_has_state(cb, LV_STATE_CHECKED);
}

/* 回调：按钮点击 */


extern void ui_left1_create(lv_obj_t *center_container);


static void menu_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG_USER("Home button clicked");
}

/* 创建界面 */
void create_ui(void)
{
    lv_obj_t * scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        if(!g_center_container) {
        g_center_container = lv_obj_create(scr);
        lv_obj_set_size(g_center_container, 800, 400);   // 你需要调整合适的大小
        lv_obj_align(g_center_container, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(g_center_container, LV_OBJ_FLAG_SCROLLABLE);
    }
    /* 底部导航栏背景 */
    lv_obj_t * nav_bar = lv_obj_create(scr);
    lv_obj_set_size(nav_bar, 800, 80);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar,
                              lv_palette_lighten(LV_PALETTE_BLUE, 3),
                              LV_PART_MAIN);
    lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建 4 个矩形按钮 */
    for (int i = 0; i < 4; i++) {
        lv_obj_t * btn = lv_btn_create(nav_bar);
        lv_obj_set_size(btn, 120, 50);

        /* 两个放左边、两个放右边 */
        if(i < 2) {
            lv_obj_align(btn, LV_ALIGN_LEFT_MID, 20 + i * 140, 0);
        } else {
            lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -20 - (i - 2) * 140, 0);
        }

        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)i);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "Btn%d", i + 1);
        lv_obj_center(label);
    }

    /* 中央圆形 Home 按钮 */
    lv_obj_t * home_btn = lv_btn_create(scr);
    lv_obj_set_size(home_btn, 80, 80);
    lv_obj_set_style_radius(home_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(home_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(home_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * home_label = lv_label_create(home_btn);
    lv_label_set_text(home_label, LV_SYMBOL_HOME);  // LVGL 内置 Home 图标
    lv_obj_center(home_label);
}

/* ---------- 前置：九宫格键盘上下文 ---------- */
typedef struct {
    lv_obj_t *target_label;       // 被编辑 label（可选）
    bool      is_time_mmss;
    const char *suffix;
    uint8_t   max_digits;

    bind_entry_t *entry;          // ⭐ 绑定对象（变量+label）
} keypad_ctx_t;

static lv_obj_t *s_panel_a = NULL;     // 子界面 A（图1）
static lv_obj_t *s_panel_b = NULL;     // 子界面 B（图2）
static bool      s_show_a  = true;     // 当前显示 A 还是 B
static lv_obj_t *s_switch_btn = NULL;  // 右下角圆形切换按钮
static lv_obj_t *s_modal = NULL;       // 键盘蒙层
static lv_obj_t *s_kb_box = NULL;      // 键盘盒
static lv_obj_t *s_ta = NULL;          // 键盘输入框
static keypad_ctx_t s_kp = {0};        // 键盘上下文（当前编辑目标）

/* ---------- 工具：将纯数字格式化为 mm:ss ---------- */
static void fmt_mmss(const char *digits, char *out, size_t out_sz)
{
    // 取最后 4 位，不足左侧补 0
    char buf[5] = "0000";
    size_t n = strlen(digits);
    if(n > 4) digits += (n - 4), n = 4;
    memcpy(buf + (4 - n), digits, n);
    int mm = (buf[0]-'0')*10 + (buf[1]-'0');
    int ss = (buf[2]-'0')*10 + (buf[3]-'0');
    if(ss > 59) ss = 59;   // 简单约束
    lv_snprintf(out, out_sz, "%02d:%02d", mm, ss);
}

/* ---------- 键盘弹窗：关闭 ---------- */
static void keypad_close(void)
{
    if(s_modal) {
        /* reset active input device so the originating button won't re-trigger */
        lv_indev_reset(lv_indev_get_act(), NULL);
        lv_obj_del(s_modal);

        s_kb_box = NULL;
        s_ta = NULL;
    }
  //  memset(&s_kp, 0, sizeof(s_kp));
    s_block_click = true;
    lv_timer_create(unblock_cb, 300, NULL);
 s_modal = NULL;
}

/* ---------- 键盘按钮事件 ---------- */
static void kb_btnm_event_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btnm, lv_btnmatrix_get_selected_btn(btnm));
    if(!txt) return;

        LV_LOG_USER("Key pressed: %s", txt);   // ← 打印按下的键

if(strcmp(txt, "OK") == 0) {
    const char *in = lv_textarea_get_text(s_ta);
    LV_LOG_USER("Input confirmed: %s", in);

    bind_entry_t *entry = s_kp.entry;
    if(entry) {
        if(entry->is_time) {
            char buf4[5] = "0000";
            size_t n = strlen(in);
            if(n > 4) { in += (n - 4); n = 4; }
            memcpy(buf4 + (4 - n), in, n);
            int mm = (buf4[0]-'0')*10 + (buf4[1]-'0');
            int ss = (buf4[2]-'0')*10 + (buf4[3]-'0');
            if(ss > 59) ss = 59;
            if(entry->var) *entry->var = mm * 60 + ss;
        } else {
            int val = atoi(in);
            if(entry->var) *entry->var = val;
        }

        // ⭐ 刷新 label
        set_label_from_var(entry);

        // ⭐ 打印确认：哪一个 label 被更新了
        LV_LOG_USER("Update label=%p var=%p value=%d",
                    (void*)entry->label,
                    (void*)entry->var,
                    entry->var ? *entry->var : -1);

        // ⭐ 强制重绘
        lv_obj_invalidate(entry->label);
    }

    keypad_close();
    return;
}else if(strcmp(txt, "<-") == 0) {
            LV_LOG_USER("Backspace");
        lv_textarea_del_char(s_ta);
    } else if(strcmp(txt, "C") == 0) {
                LV_LOG_USER("Clear input");
        lv_textarea_set_text(s_ta, "");
    } else {
        // 限制输入最大位数（仅对纯数字位计数）
        const char *cur = lv_textarea_get_text(s_ta);
        size_t cur_digits = strlen(cur);
        if(s_kp.is_time_mmss) {
            if(cur_digits < s_kp.max_digits && txt[0] >= '0' && txt[0] <= '9')
                lv_textarea_add_char(s_ta, txt[0]);
        } else {
            if(cur_digits < s_kp.max_digits && txt[0] >= '0' && txt[0] <= '9')
                lv_textarea_add_char(s_ta, txt[0]);
        }
    }
}

/* ---------- 打开九宫格键盘（数字/时间通用） ---------- */
static void keypad_open(lv_obj_t *target_label, bool is_time_mmss, const char *suffix, uint8_t max_digits)
{

    // keypad_open 里预填之前加
LV_LOG_USER("Open keypad: entry=%p label=%p var=%p is_time=%d",
            (void*)s_kp.entry,
            s_kp.entry ? (void*)s_kp.entry->label : NULL,
            s_kp.entry ? (void*)s_kp.entry->var : NULL,
            (int)is_time_mmss);


    keypad_close();
    s_kp.target_label = target_label;
    s_kp.is_time_mmss = is_time_mmss;
    s_kp.suffix       = suffix;
    s_kp.max_digits   = max_digits;

    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_50, 0);
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_modal, eat_all_cb, LV_EVENT_ALL, NULL); // ⭐

    s_kb_box = lv_obj_create(s_modal);
    lv_obj_set_size(s_kb_box, LV_HOR_RES * 3 / 5, LV_VER_RES * 3 / 5);
    lv_obj_center(s_kb_box);

    s_ta = lv_textarea_create(s_kb_box);
    lv_obj_set_width(s_ta, lv_pct(90));
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_password_mode(s_ta, false);
    lv_textarea_set_max_length(s_ta, max_digits);

    // ⭐ 用当前变量预填
    if(s_kp.entry && s_kp.entry->var){
        char pre[8] = {0};
        if(is_time_mmss){
            int mm = (*s_kp.entry->var) / 60;
            int ss = (*s_kp.entry->var) % 60;
            // 预填为 mmss（避免 “:” 影响纯数字键入）
            lv_snprintf(pre, sizeof(pre), "%02d%02d", mm, ss);
        }else{
            lv_snprintf(pre, sizeof(pre), "%d", *s_kp.entry->var);
        }
        lv_textarea_set_text(s_ta, pre);
        lv_textarea_cursor_right(s_ta);
    }

    static const char *map[] = {
        "1","2","3","\n",
        "4","5","6","\n",
        "7","8","9","\n",
        "<-","0","C","OK",""
    };
    lv_obj_t *btnm = lv_btnmatrix_create(s_kb_box);
    lv_btnmatrix_set_map(btnm, map);
    lv_obj_set_size(btnm, lv_pct(90), lv_pct(70));
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btnm, kb_btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ---------- 数值框点击：三种包装回调 ---------- */
static void num_kg_event_cb(lv_event_t *e){
    if(s_block_click) return;
    bind_entry_t *entry = (bind_entry_t*)lv_event_get_user_data(e);
    s_kp.entry = entry;
    keypad_open(entry->label, false, entry->suffix, 3);
}

static void num_s_event_cb (lv_event_t *e){
    if(s_block_click) return;
    bind_entry_t *entry = (bind_entry_t*)lv_event_get_user_data(e);
    s_kp.entry = entry;
    keypad_open(entry->label, false, entry->suffix, 3);
}

static void time_event_cb  (lv_event_t *e){
    if(s_block_click) return;
    bind_entry_t *entry = (bind_entry_t*)lv_event_get_user_data(e);
    s_kp.entry = entry;
    keypad_open(entry->label, true,  NULL, 4);
}

/* 创建“已绑定变量”的按钮，并返回中间的 label */
static lv_obj_t* make_bound_value_btn(lv_obj_t *parent,
                                      int *bind_var,
                                      const char *suffix,
                                      bool is_time,
                                      lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 40);

    lv_obj_t *lab = lv_label_create(btn);
    lv_obj_center(lab);

    // 分配并记录绑定
    bind_entry_t *entry = lv_mem_alloc(sizeof(bind_entry_t));
    entry->label  = lab;
    entry->var    = bind_var;
    entry->suffix = suffix;
    entry->is_time = is_time;

    // 首次按变量值渲染
    set_label_from_var(entry);

    // 点击时把 entry 传给事件回调
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, entry);
    return lab;
}

/* ---------- 子界面 A：左右各 5 行（每行：编号、set、time） ---------- */
static void build_panel_a(lv_obj_t *parent)
{
    s_panel_a = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_a, 800, 400);
    lv_obj_align(s_panel_a, LV_ALIGN_TOP_MID, 0, 0);

    // 标题
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

    // 左列
    for(int r=0; r<row_cnt; r++){
        int y = top_margin + r * row_space;

        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r+1);
        lv_obj_align(idx, LV_ALIGN_TOP_LEFT, 60, y);

        // 绑定 g_left_set[r] 和 g_left_time_s[r]
        lv_obj_t *lab_set  = make_bound_value_btn(s_panel_a, &g_left_set[r],     "kg", false, num_kg_event_cb);
        lv_obj_align(lab_set->parent, LV_ALIGN_TOP_LEFT,  80,  y - 10);

        lv_obj_t *lab_time = make_bound_value_btn(s_panel_a, &g_left_time_s[r],  "s",  false, num_s_event_cb);
        lv_obj_align(lab_time->parent, LV_ALIGN_TOP_LEFT, 240, y - 10);
    }

    // 右列
    for(int r=0; r<row_cnt; r++){
        int y = top_margin + r * row_space;

        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r+1);
        lv_obj_align(idx, LV_ALIGN_TOP_LEFT, 420, y);

        lv_obj_t *lab_set  = make_bound_value_btn(s_panel_a, &g_right_set[r],    "kg", false, num_kg_event_cb);
        lv_obj_align(lab_set->parent, LV_ALIGN_TOP_LEFT,  440, y - 10);

        lv_obj_t *lab_time = make_bound_value_btn(s_panel_a, &g_right_time_s[r], "s",  false, num_s_event_cb);
        lv_obj_align(lab_time->parent, LV_ALIGN_TOP_LEFT, 600, y - 10);
    }
}


/* ---------- 子界面 B：左右各 5 行（start time / end time + 复选框） ---------- */
static void build_panel_b(lv_obj_t *parent)
{
    s_panel_b = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_b, 800, 400);
    lv_obj_align(s_panel_b, LV_ALIGN_TOP_MID, 0, 0);

    // 标题
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

    for(int r=0; r<row_cnt; r++){
        int y = top_margin + r * row_space;

        lv_obj_t *idx_l = lv_label_create(s_panel_b);
        lv_label_set_text_fmt(idx_l, "%d.", r+1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 120, y);

        // 绑定时间（以秒为单位）
        lv_obj_t *lab_start = make_bound_value_btn(s_panel_b, &g_start_sec[r], NULL, true, time_event_cb);
        lv_obj_align(lab_start->parent, LV_ALIGN_TOP_LEFT, 160, y - 10);

        lv_obj_t *lab_end   = make_bound_value_btn(s_panel_b, &g_end_sec[r],   NULL, true, time_event_cb);
        lv_obj_align(lab_end->parent, LV_ALIGN_TOP_RIGHT, -260, y - 10);

        // 复选框 → 直接把 bool* 传作 user_data
        lv_obj_t *cb = lv_checkbox_create(s_panel_b);
        lv_checkbox_set_text(cb, "");
        lv_obj_align(cb, LV_ALIGN_TOP_RIGHT, -120, y);
        lv_obj_add_event_cb(cb, cb_event_cb, LV_EVENT_VALUE_CHANGED, &g_row_enable[r]);
        if(g_row_enable[r]) lv_obj_add_state(cb, LV_STATE_CHECKED);
    }

    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
}

/* ---------- 右下角圆形“切换页面”按钮 ---------- */
static void switch_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_show_a = !s_show_a;
    if(s_show_a){
        lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (s_panel_b, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (s_panel_a, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ========== 对外入口：创建这两页并放入你给的“中间显示区域” ========== */
void ui_left1_create(lv_obj_t *center_container)
{
    // 两个子界面
    build_panel_a(center_container);
    build_panel_b(center_container);

    // 右下角圆形切换按钮（放到屏幕最上层以避免被底栏遮挡）
    s_switch_btn = lv_btn_create(lv_layer_top());
    lv_obj_set_size(s_switch_btn, 64, 64);
    lv_obj_set_style_radius(s_switch_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_switch_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(s_switch_btn, switch_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ic = lv_label_create(s_switch_btn);
    lv_label_set_text(ic, LV_SYMBOL_REFRESH);
    lv_obj_center(ic);
}

void my_GUI(void)
{

create_ui();


}












