
#include "myGUI.h"
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>

lv_obj_t *g_center_container = NULL;


extern void ui_left1_create(lv_obj_t *center_container);   // 左一按钮对应的界面
static void create_ui(void);                             // 你的主界面加载函数
extern lv_obj_t *g_center_container;                        // 中央容器

/* 可选：如果“左一界面”在顶层建了悬浮控件（如切换圆钮/弹窗），
   在它的实现里提供这个清理钩子；没有就忽略。 */
__attribute__((weak)) void ui_left1_cleanup(void) {}

/* 自定义映射：哪个底栏按钮进左一界面、哪个回主界面 */
#define NAV_IDX_FIRST_PAGE   0
#define NAV_IDX_GO_HOME      5


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
    lv_obj_t *target_label;       // 被编辑的显示标签
    bool      is_time_mmss;       // true=格式 mm:ss, false=普通数字
    const char *suffix;           // 单位：如 "kg" / "s" / ""（时间不用）
    uint8_t   max_digits;         // 允许输入的纯数字位数（时间通常 4）
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
    if(s_modal) { lv_obj_del(s_modal); s_modal = NULL; s_kb_box = NULL; s_ta = NULL; }
    memset(&s_kp, 0, sizeof(s_kp));
}

/* ---------- 键盘按钮事件 ---------- */
static void kb_btnm_event_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btnm, lv_btnmatrix_get_selected_btn(btnm));
    if(!txt) return;

    if(strcmp(txt, "OK") == 0) {
        // 读取输入并写回目标标签
        const char *in = lv_textarea_get_text(s_ta);
        char out[16] = {0};
        if(s_kp.is_time_mmss) {
            fmt_mmss(in, out, sizeof(out));
        } else {
            // 限长
            char digits[10] = {0};
            size_t len = LV_MIN(strlen(in), s_kp.max_digits);
            memcpy(digits, in, len);
            if(s_kp.suffix) lv_snprintf(out, sizeof(out), "%s%s", digits, s_kp.suffix);
            else            lv_snprintf(out, sizeof(out), "%s",   digits);
        }
        if(s_kp.target_label) lv_label_set_text(s_kp.target_label, out);
        keypad_close();
    } else if(strcmp(txt, "<-") == 0) {
        lv_textarea_del_char(s_ta);
    } else if(strcmp(txt, "C") == 0) {
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
    keypad_close(); // 先保证干净
    s_kp.target_label = target_label;
    s_kp.is_time_mmss = is_time_mmss;
    s_kp.suffix       = suffix;
    s_kp.max_digits   = max_digits;

    s_modal = lv_obj_create(lv_layer_top());              // 覆盖层
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_50, 0);

    s_kb_box = lv_obj_create(s_modal);                    // 居中白框
    lv_obj_set_size(s_kb_box, LV_HOR_RES * 3 / 5, LV_VER_RES * 3 / 5);
    lv_obj_center(s_kb_box);

    s_ta = lv_textarea_create(s_kb_box);                  // 输入框
    lv_obj_set_width(s_ta, lv_pct(90));
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_password_mode(s_ta, false);
    lv_textarea_set_max_length(s_ta, max_digits);

    // 九宫格键盘（自定义 btnmatrix）
    static const char *map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "<-", "0", "C", "OK", ""
    };
    lv_obj_t *btnm = lv_btnmatrix_create(s_kb_box);
    lv_btnmatrix_set_map(btnm, map);
    lv_obj_set_size(btnm, lv_pct(90), lv_pct(70));
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btnm, kb_btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ---------- 数值框点击：三种包装回调 ---------- */
static void num_kg_event_cb(lv_event_t *e){ keypad_open((lv_obj_t*)lv_event_get_user_data(e), false, "kg", 3); }
static void num_s_event_cb (lv_event_t *e){ keypad_open((lv_obj_t*)lv_event_get_user_data(e), false, "s",  3); }
static void time_event_cb  (lv_event_t *e){ keypad_open((lv_obj_t*)lv_event_get_user_data(e), true,  NULL, 4); }

/* ---------- 工具：创建“蓝色数值按钮”并返回中间的 label ---------- */
static lv_obj_t* make_value_btn(lv_obj_t *parent, const char *init, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 48);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, init);
    lv_obj_center(lab);
    // 点击后弹键盘，回调的 user_data 传 label 指针
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, lab);
    return lab; // 方便调用处保存/后续更新
}

/* ---------- 子界面 A：左右各 5 行（每行：编号、set、time） ---------- */
static void build_panel_a(lv_obj_t *parent)
{
    s_panel_a = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_a, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_a, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_panel_a, LV_ALIGN_TOP_MID, 0, 0);

    // 两列标题
    lv_obj_t *t1 = lv_label_create(s_panel_a); lv_label_set_text(t1, "left  set   time");
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 80, 40);
    lv_obj_t *t2 = lv_label_create(s_panel_a); lv_label_set_text(t2, "right set   time");
    lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -320, 40);

    // 左列 5 行
    for(int r=0; r<5; r++){
        // 行号
        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r+1);
        lv_obj_align(idx, LV_ALIGN_TOP_LEFT, 40, 100 + r*80);

        // set（kg）
        lv_obj_t *lab_set = make_value_btn(s_panel_a, "0kg", num_kg_event_cb);
        lv_obj_align(lab_set->parent, LV_ALIGN_TOP_LEFT, 120, 88 + r*80);

        // time（s）
        lv_obj_t *lab_time = make_value_btn(s_panel_a, "10s", num_s_event_cb);
        lv_obj_align(lab_time->parent, LV_ALIGN_TOP_LEFT, 320, 88 + r*80);
    }

    // 右列 5 行
    for(int r=0; r<5; r++){
        lv_obj_t *idx = lv_label_create(s_panel_a);
        lv_label_set_text_fmt(idx, "%d.", r+1);
        lv_obj_align(idx, LV_ALIGN_TOP_RIGHT, -480, 100 + r*80);

        lv_obj_t *lab_set = make_value_btn(s_panel_a, "0kg", num_kg_event_cb);
        lv_obj_align(lab_set->parent, LV_ALIGN_TOP_RIGHT, -360, 88 + r*80);

        lv_obj_t *lab_time = make_value_btn(s_panel_a, "10s", num_s_event_cb);
        lv_obj_align(lab_time->parent, LV_ALIGN_TOP_RIGHT, -160, 88 + r*80);
    }
}

/* ---------- 复选框点击：空/对号（lv_checkbox 自带） ---------- */
static void cb_event_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target(e);
    LV_LOG_USER("row %d checked=%d", (int)(uintptr_t)lv_event_get_user_data(e),
                lv_obj_has_state(cb, LV_STATE_CHECKED));
}

/* ---------- 子界面 B：左右各 5 行（start time / end time + 复选框） ---------- */
static void build_panel_b(lv_obj_t *parent)
{
    s_panel_b = lv_obj_create(parent);
    lv_obj_clear_flag(s_panel_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_panel_b, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_panel_b, LV_ALIGN_TOP_MID, 0, 0);

    // 标题
    lv_obj_t *t1 = lv_label_create(s_panel_b); lv_label_set_text(t1, "start time"); lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 160, 40);
    lv_obj_t *t2 = lv_label_create(s_panel_b); lv_label_set_text(t2, "end time");   lv_obj_align(t2, LV_ALIGN_TOP_RIGHT, -260, 40);

    for(int r=0; r<5; r++){
        // 行号（左侧）
        lv_obj_t *idx_l = lv_label_create(s_panel_b);
        lv_label_set_text_fmt(idx_l, "%d.", r+1);
        lv_obj_align(idx_l, LV_ALIGN_TOP_LEFT, 120, 100 + r*80);

        // 左列时间
        lv_obj_t *lab_start = make_value_btn(s_panel_b, "00:00", time_event_cb);
        lv_obj_align(lab_start->parent, LV_ALIGN_TOP_LEFT, 160, 88 + r*80);

        // 右列时间
        lv_obj_t *lab_end   = make_value_btn(s_panel_b, "00:00", time_event_cb);
        lv_obj_align(lab_end->parent, LV_ALIGN_TOP_RIGHT, -260, 88 + r*80);

        // 复选框
        lv_obj_t *cb = lv_checkbox_create(s_panel_b);
        lv_checkbox_set_text(cb, "");                  // 只显示框
        lv_obj_align(cb, LV_ALIGN_TOP_RIGHT, -120, 98 + r*80);
        lv_obj_add_event_cb(cb, cb_event_cb, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)(r+1));
    }

    lv_obj_add_flag(s_panel_b, LV_OBJ_FLAG_HIDDEN); // 初始隐藏 B，显示 A
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












