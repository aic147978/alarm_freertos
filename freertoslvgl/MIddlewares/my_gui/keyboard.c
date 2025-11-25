#include "keyboard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ========== 内部状态（单例） ========== */
static lv_obj_t *s_overlay = NULL;     /* 全屏遮罩层（lv_layer_top 子对象） */
static lv_obj_t *s_panel   = NULL;     /* 键盘面板容器 */
static lv_obj_t *s_title   = NULL;     /* 标题 */
static lv_obj_t *s_disp    = NULL;     /* 显示当前输入的标签 */
static keyboard_done_cb_t s_cb = NULL;
static void *s_cb_ud = NULL;

static bool s_digits_only = false;     /* true: 只允许 0-9；false: 允许 0-9 和 '.' */
static uint8_t s_max_len  = 12;        /* 最大输入长度（<= 31） */
static char s_title_text[24] = "INPUT";
static char s_buf[32] = {0};           /* 输入缓冲 */

/* 面板尺寸（可调） */
static lv_coord_t s_panel_w = 360;
static lv_coord_t s_panel_h = 260;

/* 颜色/样式 */
#define COL_OVERLAY  0x000000  /* 遮罩黑 */
#define COL_PANEL_BG 0xFFFFFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8
#define COL_TEXT     0xFFFFFF

/* ========== 小工具 ========== */
static inline void kb_set_text(const char *t)
{
    if (!t) t = "";
    strncpy(s_buf, t, sizeof(s_buf)-1);
    s_buf[sizeof(s_buf)-1] = '\0';
    if (s_disp) lv_label_set_text(s_disp, s_buf);
}

static inline void kb_clear(void){ kb_set_text(""); }

static inline void kb_push_char(char c)
{
    size_t n = strlen(s_buf);
    if (n >= s_max_len || n >= sizeof(s_buf)-1) return;
    s_buf[n] = c; s_buf[n+1] = '\0';
    if (s_disp) lv_label_set_text(s_disp, s_buf);
}

static inline void kb_pop_char(void)
{
    size_t n = strlen(s_buf);
    if (n == 0) return;
    s_buf[n-1] = '\0';
    if (s_disp) lv_label_set_text(s_disp, s_buf);
}

/* ========== 事件回调 ========== */
static void on_key_clicked(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0)); /* 按钮里唯一 label */

    if (strcmp(txt, "Del") == 0) { kb_pop_char(); return; }
    if (strcmp(txt, "Clr") == 0) { kb_clear();   return; }
    if (strcmp(txt, "OK")  == 0) {
        if (s_cb) {
            /* 解析 float 值（时间页可以忽略这个 value） */
            char *endp = NULL;
            float v = strtof(s_buf, &endp);
            s_cb(s_buf, v, s_cb_ud);
        }
        keyboard_hide();
        return;
    }

    /* 普通字符键 */
    if (txt && txt[0]) {
        char c = txt[0];
        if (isdigit((unsigned char)c)) {
            kb_push_char(c);
            return;
        }
        if (!s_digits_only && c=='.') {
            /* 只允许一个小数点 */
            if (strchr(s_buf, '.') == NULL) kb_push_char('.');
            return;
        }
    }
}

/* 遮罩点击：点击遮罩不关闭（可按需改成关闭） */
static void on_overlay_clicked(lv_event_t *e) { LV_UNUSED(e); }

/* ========== 构建 UI（懒创建） ========== */
static void style_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (btn, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (btn, 12,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_hex(COL_TEXT), LV_PART_MAIN | LV_STATE_ANY);
}

static lv_obj_t* make_button(lv_obj_t *parent, const char *txt, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    style_btn(btn);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, on_key_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_center(label);
    return btn;
}

static void ensure_built(void)
{
    if (s_overlay) return;

    /* 遮罩 */
    s_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_overlay, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(COL_OVERLAY), 0);
    lv_obj_set_style_bg_opa(s_overlay, 96, 0);                 /* 半透明 */
    lv_obj_add_event_cb(s_overlay, on_overlay_clicked, LV_EVENT_CLICKED, NULL);
    /* 禁滚动 */
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_overlay, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_overlay, LV_SCROLLBAR_MODE_OFF);

    /* 面板 */
    s_panel = lv_obj_create(s_overlay);
    lv_obj_set_size(s_panel, s_panel_w, s_panel_h);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(COL_PANEL_BG), 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_panel, 16, 0);
    lv_obj_align(s_panel, LV_ALIGN_CENTER, 0, 0);
    /* 禁滚动 */
    lv_obj_clear_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_panel, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_panel, LV_SCROLLBAR_MODE_OFF);

    /* 标题 + 显示区 */
    s_title = lv_label_create(s_panel);
    lv_label_set_text(s_title, s_title_text);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 10);

    s_disp = lv_label_create(s_panel);
    lv_label_set_text(s_disp, "");
    lv_obj_align(s_disp, LV_ALIGN_TOP_MID, 0, 46);

    /* 键区布局：4 列 × 4 行；OK 与其他键相同尺寸 */
    const lv_coord_t margin_x = 14, margin_y = 14;
    const lv_coord_t grid_w = s_panel_w - margin_x*2;
    const lv_coord_t grid_h = s_panel_h - 100 - margin_y;  /* 上方标题 + 显示占 ~100px */

    const int COLS = 4, ROWS = 4;
    lv_coord_t cell_w = (grid_w - (COLS-1)*10) / COLS;
    lv_coord_t cell_h = (grid_h - (ROWS-1)*10) / ROWS;
    lv_coord_t origin_x = margin_x;
    lv_coord_t origin_y = 100;

    /* 行 1: 7 8 9 Del */
    make_button(s_panel,"7",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*0, cell_w, cell_h);
    make_button(s_panel,"8",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*0, cell_w, cell_h);
    make_button(s_panel,"9",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*0, cell_w, cell_h);
    make_button(s_panel,"Del", origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*0, cell_w, cell_h);

    /* 行 2: 4 5 6 Clr */
    make_button(s_panel,"4",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*1, cell_w, cell_h);
    make_button(s_panel,"5",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*1, cell_w, cell_h);
    make_button(s_panel,"6",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*1, cell_w, cell_h);
    make_button(s_panel,"Clr", origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*1, cell_w, cell_h);

    /* 行 3: 1 2 3 OK */
    make_button(s_panel,"1",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*2, cell_w, cell_h);
    make_button(s_panel,"2",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*2, cell_w, cell_h);
    make_button(s_panel,"3",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*2, cell_w, cell_h);
    make_button(s_panel,"OK",  origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*2, cell_w, cell_h);

    /* 行 4: . 0 （留空） （留空）*/
    make_button(s_panel,".",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*3, cell_w, cell_h);
    make_button(s_panel,"0",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*3, cell_w, cell_h);
    /* 留空两格不放按钮，避免拥挤，OK 已在上面一行 */
}

/* ========== 对外接口 ========== */
void keyboard_show_with_cb(keyboard_done_cb_t cb, void *user_data, const char *initial_text)
{
    s_digits_only = false;
    s_cb = cb; s_cb_ud = user_data;
    ensure_built();
    kb_set_text(initial_text);
    if (s_title) lv_label_set_text(s_title, s_title_text);

    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
}

void keyboard_show_digits_with_cb(keyboard_done_cb_t cb, void *user_data, const char *initial_text)
{
    s_digits_only = true;
    s_cb = cb; s_cb_ud = user_data;
    ensure_built();
    kb_set_text(initial_text);
    if (s_title) lv_label_set_text(s_title, s_title_text);

    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
}

void keyboard_hide(void)
{
    if (!s_overlay) return;
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

bool keyboard_is_visible(void)
{
    return s_overlay && !lv_obj_has_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

void keyboard_set_max_len(uint8_t max_len)
{
    if (max_len == 0) max_len = 1;
    if (max_len > sizeof(s_buf)-1) max_len = sizeof(s_buf)-1;
    s_max_len = max_len;
}

void keyboard_set_title(const char *title)
{
    if (!title) title = "INPUT";
    strncpy(s_title_text, title, sizeof(s_title_text)-1);
    s_title_text[sizeof(s_title_text)-1] = '\0';
    if (s_title) lv_label_set_text(s_title, s_title_text);
}

void keyboard_set_panel_size(lv_coord_t w, lv_coord_t h)
{
    if (w < 200) w = 200;
    if (h < 180) h = 180;
    s_panel_w = w; s_panel_h = h;

    if (s_panel) {
        lv_obj_set_size(s_panel, s_panel_w, s_panel_h);
        lv_obj_align(s_panel, LV_ALIGN_CENTER, 0, 0);

        /* 重新布局按钮（简单做法：销毁重建键区；为稳妥这里直接重建组件） */
        lv_obj_clean(s_panel);
        /* 标题与显示 */
        s_title = lv_label_create(s_panel);
        lv_label_set_text(s_title, s_title_text);
        lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 10);

        s_disp = lv_label_create(s_panel);
        lv_label_set_text(s_disp, s_buf);
        lv_obj_align(s_disp, LV_ALIGN_TOP_MID, 0, 46);

        /* 重新创建按钮网格 */
        const lv_coord_t margin_x = 14, margin_y = 14;
        const lv_coord_t grid_w = s_panel_w - margin_x*2;
        const lv_coord_t grid_h = s_panel_h - 100 - margin_y;
        const int COLS = 4, ROWS = 4;
        lv_coord_t cell_w = (grid_w - (COLS-1)*10) / COLS;
        lv_coord_t cell_h = (grid_h - (ROWS-1)*10) / ROWS;
        lv_coord_t origin_x = margin_x;
        lv_coord_t origin_y = 100;

        make_button(s_panel,"7",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*0, cell_w, cell_h);
        make_button(s_panel,"8",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*0, cell_w, cell_h);
        make_button(s_panel,"9",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*0, cell_w, cell_h);
        make_button(s_panel,"Del", origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*0, cell_w, cell_h);

        make_button(s_panel,"4",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*1, cell_w, cell_h);
        make_button(s_panel,"5",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*1, cell_w, cell_h);
        make_button(s_panel,"6",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*1, cell_w, cell_h);
        make_button(s_panel,"Clr", origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*1, cell_w, cell_h);

        make_button(s_panel,"1",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*2, cell_w, cell_h);
        make_button(s_panel,"2",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*2, cell_w, cell_h);
        make_button(s_panel,"3",   origin_x + (cell_w+10)*2, origin_y + (cell_h+10)*2, cell_w, cell_h);
        make_button(s_panel,"OK",  origin_x + (cell_w+10)*3, origin_y + (cell_h+10)*2, cell_w, cell_h);

        make_button(s_panel,".",   origin_x + (cell_w+10)*0, origin_y + (cell_h+10)*3, cell_w, cell_h);
        make_button(s_panel,"0",   origin_x + (cell_w+10)*1, origin_y + (cell_h+10)*3, cell_w, cell_h);
    }
}
