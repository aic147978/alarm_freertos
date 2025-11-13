#include "keyboard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ===== 风格与尺寸 ===== */
#define COL_MASK_BG      0x000000
#define COL_PANEL_BG     0xF7FAF7
#define COL_BTN_NORM     0xD8DADF
#define COL_BTN_PRESS    0xC3C6CC
#define COL_TEXT         0x2B2F36

#define PANEL_W          440
#define PANEL_H          260
#define PANEL_PAD_T      8     /* 面板上边距 —— 调小就更贴顶 */
#define PANEL_PAD_B      12
#define PANEL_PAD_L      12
#define PANEL_PAD_R      12

#define GAP_H            10    /* 键之间的水平/竖直间距 */
#define GAP_V            10

#define TA_H             40    /* 输入框高度 */
#define TA_RADIUS        12

#define BTN_MIN_H        36
#define BTN_RADIUS       14

/* ===== 内部状态 ===== */
static lv_obj_t *s_overlay = NULL;      /* 全屏遮罩 */
static lv_obj_t *s_panel   = NULL;      /* 居中面板 */
static lv_obj_t *s_ta      = NULL;      /* 输入框 */

static lv_obj_t *s_parent  = NULL;      /* 父对象（可为 lv_layer_top） */
static lv_obj_t *s_target_ta = NULL;    /* 模式1：写入目标 textarea */
static kb_submit_cb_t s_submit_cb = NULL; /* 模式2：回调返回 */
static void *s_submit_ud = NULL;

static uint16_t s_max_len = 12;
static bool s_created = false;

/* ===== 按钮样式 ===== */
static void style_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_NORM), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PRESS), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius(btn, BTN_RADIUS, LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_hex(COL_TEXT), LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_min_height(btn, BTN_MIN_H, 0);
}

/* ===== 键码与事件 ===== */
typedef enum {
    K_1,K_2,K_3, K_4,K_5,K_6, K_7,K_8,K_9, K_BACK, K_0, K_CLR, K_OK
} kcode_t;

static void key_ev(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;

    kcode_t kc = (kcode_t)(uintptr_t)lv_event_get_user_data(e);
    const char *cur = lv_textarea_get_text(s_ta);
    char buf[64]; size_t len = cur? strlen(cur) : 0;

    switch (kc)
    {
        case K_0: case K_1: case K_2: case K_3:
        case K_4: case K_5: case K_6: case K_7: case K_8: case K_9:
        {
            if (len >= s_max_len) break;
            char ch = '0';
            if (kc==K_1) ch='1'; else if (kc==K_2) ch='2'; else if (kc==K_3) ch='3';
            else if (kc==K_4) ch='4'; else if (kc==K_5) ch='5'; else if (kc==K_6) ch='6';
            else if (kc==K_7) ch='7'; else if (kc==K_8) ch='8'; else if (kc==K_9) ch='9';
            snprintf(buf, sizeof(buf), "%s%c", cur?cur:"", ch);
            lv_textarea_set_text(s_ta, buf);
            break;
        }
        case K_BACK:
            if (len>0) { memcpy(buf, cur, len-1); buf[len-1]=0; lv_textarea_set_text(s_ta, buf); }
            break;
        case K_CLR:
            lv_textarea_set_text(s_ta, "");
            break;
        case K_OK:
        {
            const char *txt = lv_textarea_get_text(s_ta);
            float v = 0.f; if (txt && txt[0]) v = (float)strtod(txt, NULL);
            if (s_target_ta) lv_textarea_set_text(s_target_ta, txt?txt:"");
            if (s_submit_cb) s_submit_cb(txt?txt:"", v, s_submit_ud);
            keyboard_hide();
            break;
        }
        default: break;
    }
}

/* 创建一个按键并放进网格单元 */
static lv_obj_t* make_key(lv_obj_t *parent, const char *txt, kcode_t code,
                          int col, int col_span, int row, int row_span)
{
    lv_obj_t *btn = lv_btn_create(parent);
    style_btn(btn);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);

    lv_obj_set_grid_cell(btn,
        LV_GRID_ALIGN_STRETCH, col, col_span,
        LV_GRID_ALIGN_STRETCH, row, row_span);

    lv_obj_add_event_cb(btn, key_ev, LV_EVENT_CLICKED, (void*)(uintptr_t)code);
    return btn;
}

/* ===== 构建 UI（懒创建） ===== */
static void ensure_created(void)
{
    if (s_created) return;
    if (!s_parent) s_parent = lv_layer_top();

    /* 遮罩层 */
    s_overlay = lv_obj_create(s_parent);
    lv_obj_remove_style_all(s_overlay);
    lv_obj_set_size(s_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(COL_MASK_BG), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_40, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);

    /* 面板：无标题行，直接三行——输入框 / 数字3×3 / 底排四键 */
    s_panel = lv_obj_create(s_overlay);
    lv_obj_set_size(s_panel, PANEL_W, PANEL_H);
    lv_obj_center(s_panel);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(COL_PANEL_BG), 0);
    lv_obj_set_style_radius(s_panel, 14, 0);
    lv_obj_set_style_pad_top   (s_panel, PANEL_PAD_T, 0);  /* 顶部间距（很关键） */
    lv_obj_set_style_pad_bottom(s_panel, PANEL_PAD_B, 0);
    lv_obj_set_style_pad_left  (s_panel, PANEL_PAD_L, 0);
    lv_obj_set_style_pad_right (s_panel, PANEL_PAD_R, 0);
    lv_obj_set_style_shadow_width(s_panel, 18, 0);
    lv_obj_set_style_shadow_opa (s_panel, LV_OPA_20, 0);
    lv_obj_clear_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(s_panel, GAP_V, 0);          /* 行间距（输入框与键区之间） */

    static lv_coord_t pcols[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t prows[] = {
        LV_GRID_CONTENT,      /* row 0: 输入框 */
        LV_GRID_FR(1),        /* row 1: 数字 3×3 */
        LV_GRID_CONTENT,      /* row 2: 底排四键 */
        LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_layout(s_panel, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(s_panel, pcols, prows);

    /* 行0：输入框（直接放面板，不再套一层容器，减少顶部空隙） */
    s_ta = lv_textarea_create(s_panel);
    lv_obj_set_height(s_ta, TA_H);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_cursor_click_pos(s_ta, false);
    lv_obj_set_style_radius(s_ta, TA_RADIUS, 0);
    lv_obj_set_grid_cell(s_ta,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_START,   0, 1);

    /* 行1：数字 3×3 容器 */
    lv_obj_t *grid = lv_obj_create(s_panel);
    lv_obj_remove_style_all(grid);
    lv_obj_set_style_pad_column(grid, GAP_H, 0);
    lv_obj_set_style_pad_row   (grid, GAP_V, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    static lv_coord_t c3[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t r3[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, c3, r3);
    lv_obj_set_grid_cell(grid,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_STRETCH, 1, 1);

    /* 1 2 3 / 4 5 6 / 7 8 9 */
    make_key(grid, "1", K_1, 0,1, 0,1);
    make_key(grid, "2", K_2, 1,1, 0,1);
    make_key(grid, "3", K_3, 2,1, 0,1);
    make_key(grid, "4", K_4, 0,1, 1,1);
    make_key(grid, "5", K_5, 1,1, 1,1);
    make_key(grid, "6", K_6, 2,1, 1,1);
    make_key(grid, "7", K_7, 0,1, 2,1);
    make_key(grid, "8", K_8, 1,1, 2,1);
    make_key(grid, "9", K_9, 2,1, 2,1);

    /* 行2：底排 <- 0 C OK */
    lv_obj_t *row = lv_obj_create(s_panel);
    lv_obj_remove_style_all(row);
    lv_obj_set_style_pad_column(row, GAP_H, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    static lv_coord_t c4[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t r1[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_layout(row, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(row, c4, r1);
    lv_obj_set_grid_cell(row,
        LV_GRID_ALIGN_STRETCH, 0, 1,
        LV_GRID_ALIGN_START,   2, 1);

    make_key(row, "<-", K_BACK, 0,1, 0,1);
    make_key(row, "0",  K_0,    1,1, 0,1);
    make_key(row, "C",  K_CLR,  2,1, 0,1);
    make_key(row, "OK", K_OK,   3,1, 0,1);

    s_created = true;
}

/* ===== 对外接口 ===== */
void keyboard_init(lv_obj_t *parent)
{
    s_parent = parent;
    ensure_created();
}

void keyboard_show_for_ta(lv_obj_t *target_ta, const char *initial_text)
{
    ensure_created();
    s_target_ta = target_ta;
    s_submit_cb = NULL; s_submit_ud = NULL;

    lv_textarea_set_text(s_ta, initial_text ? initial_text : "");
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
}

void keyboard_show_with_cb(kb_submit_cb_t cb, void *user_data, const char *initial_text)
{
    ensure_created();
    s_target_ta = NULL; s_submit_cb = cb; s_submit_ud = user_data;

    lv_textarea_set_text(s_ta, initial_text ? initial_text : "");
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
}

void keyboard_hide(void)
{
    if (!s_created) return;
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    s_target_ta = NULL;
    s_submit_cb = NULL;
    s_submit_ud = NULL;
}

bool keyboard_is_open(void)
{
    if (!s_created) return false;
    return !lv_obj_has_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

void keyboard_set_max_len(uint16_t max_len)
{
    s_max_len = (max_len == 0) ? 1 : max_len;
}

void keyboard_set_title(const char *title) { (void)title; } /* 已无标题行 */
