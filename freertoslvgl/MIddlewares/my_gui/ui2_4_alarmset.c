#include "ui2_4_alarmset.h"
#include "keyboard.h"   /* 用于弹出数字键盘 */
#include <stdio.h>

/* ===== 固定色与按钮风格 ===== */
#define COL_PANEL_BG 0xF5FAFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8
#define COL_TXT      0xFFFFFF
#define COL_TITLE    0x333333

/* ===== 根/对象 ===== */
static lv_obj_t *s_root = NULL;

/* 左列：标题 + 三行标签 + 三个输入按钮 + 两个单位标签(%) + 一个单位(s) */
static lv_obj_t *s_lbl_left_title = NULL;
static lv_obj_t *s_lab_full = NULL;
static lv_obj_t *s_lab_empty = NULL;
static lv_obj_t *s_lab_timeout = NULL;
static lv_obj_t *s_btn_full = NULL;
static lv_obj_t *s_btn_empty = NULL;
static lv_obj_t *s_btn_timeout = NULL;
static lv_obj_t *s_unit_full = NULL;
static lv_obj_t *s_unit_empty = NULL;
static lv_obj_t *s_unit_timeout = NULL;

/* 右两列：10 个 checkbox（0..4 在左列，5..9 在右列）+ 标题 */
static lv_obj_t *s_lbl_right_title = NULL;
static lv_obj_t *s_checks[10] = {0};

/* 退出按钮 */
static lv_obj_t *s_btn_exit = NULL;

/* ===== 数据 ===== */
static uint16_t s_full_pct  = 115;   /* 满料报警阈值%（默认 115%） */
static uint16_t s_empty_pct = 10;    /* 空料报警阈值%  （默认 10%） */
static uint32_t s_timeout_s = 30;    /* 超时报警秒数   （默认 30s） */
static bool     s_check[10] = {0};   /* 10 个勾选项    （默认全关） */

/* 退出回调 */
static ui24_exit_cb_t s_exit_cb = NULL;
static void          *s_exit_ud = NULL;

/* ===== 布局参数（像素） ===== */
static int TITLE_LEFT_Y   = 28;       /* 左列标题 Y */
static int LABEL_X        = 58;       /* 左列行标签 X */
static int INPUT_BTN_X    = 182;      /* 左列输入按钮 X */
static int ROW_FIRST_Y    = 88;       /* 左列第 1 行 Y */
static int ROW_GAP_Y      = 66;       /* 左列行距 */
static int INPUT_W        = 140;      /* 左列输入按钮宽 */
static int INPUT_H        = 44;       /* 左列输入按钮高 */
static int UNIT_GAP_X     = 6;        /* 单位标与按钮右缘间距 */

static int CHECK_LEFT_X   = 420;      /* 右边两列：左列 X（索引 0..4） */
static int CHECK_RIGHT_X  = 600;      /* 右边两列：右列 X（索引 5..9） */
static int CHECK_TOP_Y    = 88;       /* 勾选首行 Y */
static int CHECK_V_GAP    = 50;       /* 勾选行间距 */

static int EXIT_CX        = 692;      /* 退出按钮中心 X */
static int EXIT_CY        = 344;      /* 退出按钮中心 Y */
static int EXIT_W         = 184;      /* 退出按钮宽 */
static int EXIT_H         = 60;       /* 退出按钮高 */

/* ===== 工具：简样式/按钮工厂/文本 ===== */
static void make_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa  (o, opa, 0);
    lv_obj_set_style_pad_all (o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static lv_obj_t* make_btn(int w, int h, const char *txt)
{
    lv_obj_t *b = lv_btn_create(s_root);
    /* 保留主题（不 remove_all），但显式设置常见状态的颜色与圆角 */
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_opa  (b, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_radius  (b, 14,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_shadow_width(b, 10, 0);
    lv_obj_set_style_shadow_opa (b, LV_OPA_20, 0);

    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt ? txt : "");
    lv_obj_set_style_text_color(lab, lv_color_hex(COL_TXT), 0);
    lv_obj_center(lab);
    return b;
}

static void btn_set_text(lv_obj_t *btn, const char *txt)
{
    lv_obj_t *lab = lv_obj_get_child(btn, 0);
    if (lab) lv_label_set_text(lab, txt);
}
static void btn_set_center(lv_obj_t *btn, int cx, int cy)
{
    lv_obj_set_pos(btn, cx - lv_obj_get_width(btn)/2, cy - lv_obj_get_height(btn)/2);
}

static lv_obj_t* make_label_at(int x, int y, const char *txt, lv_color_t color)
{
    lv_obj_t *l = lv_label_create(s_root);
    lv_obj_remove_style_all(l);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, x, y);
    return l;
}

/* ===== 显示文本格式化 ===== */
static void fmt_pct_text(char *buf, size_t n, uint16_t pct){ snprintf(buf, n, "%u", (unsigned)pct); }   /* 显示在按钮上只放数字 */
static void fmt_sec_text(char *buf, size_t n, uint32_t s) { snprintf(buf, n, "%u", (unsigned)s); }

/* ===== 键盘 OK → 数据写回（桥接） ===== */
static void kb_full_ok  (const char *txt, float v, void *ud)
{
    LV_UNUSED(txt); LV_UNUSED(ud);
    if (v < 0) v = 0;
    if (v > 10000) v = 10000;   /* 极限上限，按需修改 */
    s_full_pct = (uint16_t)v;
    char b[16]; fmt_pct_text(b, sizeof b, s_full_pct);
    if (s_btn_full) btn_set_text(s_btn_full, b);
}
static void kb_empty_ok (const char *txt, float v, void *ud)
{
    LV_UNUSED(txt); LV_UNUSED(ud);
    if (v < 0) v = 0;
    if (v > 10000) v = 10000;
    s_empty_pct = (uint16_t)v;
    char b[16]; fmt_pct_text(b, sizeof b, s_empty_pct);
    if (s_btn_empty) btn_set_text(s_btn_empty, b);
}
static void kb_timeout_ok(const char *txt, float v, void *ud)
{
    LV_UNUSED(txt); LV_UNUSED(ud);
    if (v < 0) v = 0;
    if (v > 86400) v = 86400;   /* 上限一天 */
    s_timeout_s = (uint32_t)v;
    char b[16]; fmt_sec_text(b, sizeof b, s_timeout_s);
    if (s_btn_timeout) btn_set_text(s_btn_timeout, b);
}

/* ===== 事件 ===== */
static void on_click_full(lv_event_t *e)
{
    /* 百分比，只允许数字，不带小数点；长度按 5 位兜底 */
    keyboard_set_title("满料报警(%)");
    keyboard_set_max_len(5);
    char init[16]; fmt_pct_text(init, sizeof init, s_full_pct);
    keyboard_show_digits_with_cb(kb_full_ok, NULL, init);
}
static void on_click_empty(lv_event_t *e)
{
    keyboard_set_title("空料报警(%)");
    keyboard_set_max_len(5);
    char init[16]; fmt_pct_text(init, sizeof init, s_empty_pct);
    keyboard_show_digits_with_cb(kb_empty_ok, NULL, init);
}
static void on_click_timeout(lv_event_t *e)
{
    keyboard_set_title("超时报警(秒)");
    keyboard_set_max_len(6);
    char init[16]; fmt_sec_text(init, sizeof init, s_timeout_s);
    keyboard_show_digits_with_cb(kb_timeout_ok, NULL, init);
}
static void on_click_exit(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_exit_cb) s_exit_cb(s_exit_ud);
}

/* ===== 创建/销毁 ===== */
void ui24_alarmset_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    make_plain(s_root, 800, 400, lv_color_hex(COL_PANEL_BG), LV_OPA_TRANSP);

    /* 左列：标题与三行 */
    s_lbl_left_title = make_label_at(120, TITLE_LEFT_Y, "alarm thresholds", lv_color_hex(COL_TITLE));

    /* 行 1：满料报警（按钮 + % 单位） */
    s_lab_full  = make_label_at(LABEL_X, ROW_FIRST_Y + 0*ROW_GAP_Y, "满料报警", lv_color_hex(COL_TITLE));
    char t[16];
    fmt_pct_text(t, sizeof t, s_full_pct);
    s_btn_full  = make_btn(INPUT_W, INPUT_H, t);
    lv_obj_set_pos(s_btn_full, INPUT_BTN_X, ROW_FIRST_Y + 0*ROW_GAP_Y);
    lv_obj_add_event_cb(s_btn_full, on_click_full, LV_EVENT_CLICKED, NULL);
    s_unit_full = make_label_at(INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 0*ROW_GAP_Y + (INPUT_H-18)/2, "%", lv_color_hex(COL_TITLE));

    /* 行 2：空料报警 */
    s_lab_empty = make_label_at(LABEL_X, ROW_FIRST_Y + 1*ROW_GAP_Y, "空料报警", lv_color_hex(COL_TITLE));
    fmt_pct_text(t, sizeof t, s_empty_pct);
    s_btn_empty = make_btn(INPUT_W, INPUT_H, t);
    lv_obj_set_pos(s_btn_empty, INPUT_BTN_X, ROW_FIRST_Y + 1*ROW_GAP_Y);
    lv_obj_add_event_cb(s_btn_empty, on_click_empty, LV_EVENT_CLICKED, NULL);
    s_unit_empty = make_label_at(INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 1*ROW_GAP_Y + (INPUT_H-18)/2, "%", lv_color_hex(COL_TITLE));

    /* 行 3：超时报警（秒） */
    s_lab_timeout = make_label_at(LABEL_X, ROW_FIRST_Y + 2*ROW_GAP_Y, "超时报警", lv_color_hex(COL_TITLE));
    fmt_sec_text(t, sizeof t, s_timeout_s);
    s_btn_timeout = make_btn(INPUT_W, INPUT_H, t);
    lv_obj_set_pos(s_btn_timeout, INPUT_BTN_X, ROW_FIRST_Y + 2*ROW_GAP_Y);
    lv_obj_add_event_cb(s_btn_timeout, on_click_timeout, LV_EVENT_CLICKED, NULL);
    s_unit_timeout = make_label_at(INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 2*ROW_GAP_Y + (INPUT_H-18)/2, "s", lv_color_hex(COL_TITLE));

    /* 右两列：10 个勾选项（0..4 左列，5..9 右列） */
    s_lbl_right_title = make_label_at(520, TITLE_LEFT_Y, "enable matrix", lv_color_hex(COL_TITLE));

    for (int i = 0; i < 10; ++i) {
        s_checks[i] = lv_checkbox_create(s_root);
        lv_checkbox_set_text(s_checks[i], "");  /* 不显示文字，只要方框 */
        int col = (i < 5) ? 0 : 1;
        int row = (i < 5) ? i : (i - 5);
        int x   = (col == 0) ? CHECK_LEFT_X : CHECK_RIGHT_X;
        int y   = CHECK_TOP_Y + row * CHECK_V_GAP;
        lv_obj_set_pos(s_checks[i], x, y);

        if (s_check[i]) lv_obj_add_state(s_checks[i], LV_STATE_CHECKED);
        else            lv_obj_clear_state(s_checks[i], LV_STATE_CHECKED);
    }

    /* 右下角退出 */
    s_btn_exit = make_btn(EXIT_W, EXIT_H, "exit");
    btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
    lv_obj_add_event_cb(s_btn_exit, on_click_exit, LV_EVENT_CLICKED, NULL);
}

void ui24_alarmset_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }

    s_lbl_left_title = s_lab_full = s_lab_empty = s_lab_timeout = NULL;
    s_btn_full = s_btn_empty = s_btn_timeout = NULL;
    s_unit_full = s_unit_empty = s_unit_timeout = NULL;
    s_lbl_right_title = NULL;
    for (int i=0;i<10;++i) s_checks[i] = NULL;
    s_btn_exit = NULL;
}

/* ===== Root 与回调 ===== */
lv_obj_t* ui24_root(void){ return s_root; }
void ui24_set_exit_cb(ui24_exit_cb_t cb, void *user_data){ s_exit_cb = cb; s_exit_ud = user_data; }

/* ===== 参数存取 ===== */
void     ui24_set_full_percent(uint16_t pct){
    s_full_pct = pct;
    if (s_btn_full){ char b[16]; fmt_pct_text(b, sizeof b, s_full_pct); btn_set_text(s_btn_full, b); }
}
uint16_t ui24_get_full_percent(void){ return s_full_pct; }

void     ui24_set_empty_percent(uint16_t pct){
    s_empty_pct = pct;
    if (s_btn_empty){ char b[16]; fmt_pct_text(b, sizeof b, s_empty_pct); btn_set_text(s_btn_empty, b); }
}
uint16_t ui24_get_empty_percent(void){ return s_empty_pct; }

void     ui24_set_timeout_sec(uint32_t sec){
    s_timeout_s = sec;
    if (s_btn_timeout){ char b[16]; fmt_sec_text(b, sizeof b, s_timeout_s); btn_set_text(s_btn_timeout, b); }
}
uint32_t ui24_get_timeout_sec(void){ return s_timeout_s; }

void ui24_set_check(uint8_t idx, bool enable){
    if (idx >= 10) return;
    s_check[idx] = enable;
    if (s_checks[idx]){
        if (enable) lv_obj_add_state(s_checks[idx], LV_STATE_CHECKED);
        else        lv_obj_clear_state(s_checks[idx], LV_STATE_CHECKED);
    }
}
bool ui24_get_check(uint8_t idx){
    if (idx >= 10) return false;
    return s_check[idx];
}
void ui24_set_check_all(bool enable){
    for (uint8_t i=0;i<10;++i) ui24_set_check(i, enable);
}

/* ===== 布局微调 ===== */
void ui24_set_titles_y(int y){
    TITLE_LEFT_Y = y;
    if (s_lbl_left_title)  lv_obj_align(s_lbl_left_title,  LV_ALIGN_TOP_LEFT, 120, TITLE_LEFT_Y);
    if (s_lbl_right_title) lv_obj_align(s_lbl_right_title, LV_ALIGN_TOP_LEFT, 520, TITLE_LEFT_Y);
}
void ui24_set_left_cols_x(int lab_x, int btn_x){
    LABEL_X = lab_x; INPUT_BTN_X = btn_x;
    if (!s_root) return;
    if (s_lab_full)   lv_obj_set_pos(s_lab_full,  LABEL_X, ROW_FIRST_Y + 0*ROW_GAP_Y);
    if (s_lab_empty)  lv_obj_set_pos(s_lab_empty, LABEL_X, ROW_FIRST_Y + 1*ROW_GAP_Y);
    if (s_lab_timeout)lv_obj_set_pos(s_lab_timeout, LABEL_X, ROW_FIRST_Y + 2*ROW_GAP_Y);

    if (s_btn_full)   lv_obj_set_pos(s_btn_full,  INPUT_BTN_X, ROW_FIRST_Y + 0*ROW_GAP_Y);
    if (s_btn_empty)  lv_obj_set_pos(s_btn_empty, INPUT_BTN_X, ROW_FIRST_Y + 1*ROW_GAP_Y);
    if (s_btn_timeout)lv_obj_set_pos(s_btn_timeout, INPUT_BTN_X, ROW_FIRST_Y + 2*ROW_GAP_Y);

    if (s_unit_full)  lv_obj_set_pos(s_unit_full,  INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 0*ROW_GAP_Y + (INPUT_H-18)/2);
    if (s_unit_empty) lv_obj_set_pos(s_unit_empty, INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 1*ROW_GAP_Y + (INPUT_H-18)/2);
    if (s_unit_timeout)lv_obj_set_pos(s_unit_timeout, INPUT_BTN_X + INPUT_W + UNIT_GAP_X, ROW_FIRST_Y + 2*ROW_GAP_Y + (INPUT_H-18)/2);
}
void ui24_set_rows_y(int first_y, int row_gap){
    ROW_FIRST_Y = first_y; ROW_GAP_Y = row_gap;
    ui24_set_left_cols_x(LABEL_X, INPUT_BTN_X);  /* 复用上面的更新 */
}
void ui24_set_checks_xy(int left_x, int right_x, int top_y, int v_gap){
    CHECK_LEFT_X = left_x; CHECK_RIGHT_X = right_x; CHECK_TOP_Y = top_y; CHECK_V_GAP = v_gap;
    if (!s_root) return;
    for (int i=0;i<10;++i){
        if (!s_checks[i]) continue;
        int col = (i < 5) ? 0 : 1;
        int row = (i < 5) ? i : (i - 5);
        int x   = (col == 0) ? CHECK_LEFT_X : CHECK_RIGHT_X;
        int y   = CHECK_TOP_Y + row * CHECK_V_GAP;
        lv_obj_set_pos(s_checks[i], x, y);
    }
}
void ui24_set_exit_btn_pos_size(int cx, int cy, int w, int h){
    EXIT_CX = cx; EXIT_CY = cy; EXIT_W = w; EXIT_H = h;
    if (s_btn_exit){
        lv_obj_set_size(s_btn_exit, EXIT_W, EXIT_H);
        btn_set_center(s_btn_exit, EXIT_CX, EXIT_CY);
    }
}
