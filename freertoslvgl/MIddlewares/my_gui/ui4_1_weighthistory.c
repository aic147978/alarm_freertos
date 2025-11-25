#include "ui4_1_weighthistory.h"
#include <stdio.h>
#include <string.h>

/* ===== 常量与颜色 ===== */
#define MAX_ROWS     200
#define PAGE_ROWS    5
#define TS_LEN       32
#define COL_PANEL_BG 0xF2F6FB
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8
#define COL_TEXT     0xFFFFFF

/* ===== 布局（表格版默认值，可通过接口修改） ===== */
static int g_frame_x = 24,  g_frame_y = 12,  g_frame_w = 752, g_frame_h = 376; /* 带边框的框 */
static int g_col_w0 = 80,   g_col_w1 = 240, g_col_w2 = 180, g_col_w3 = 180;    /* 4 列宽 */
static int g_row_pad = 10;                                                     /* 行内上下留白 */
static int PAGER_UP_CX = 760, PAGER_UP_CY = 180, PAGER_DN_CX = 760, PAGER_DN_CY = 242, PAGER_D = 48;
static int SWITCH_CX = 744, SWITCH_CY = 378, SWITCH_W = 96, SWITCH_H = 40;

/* ===== 模块状态 ===== */
static lv_obj_t *s_root  = NULL;  /* 800×400 */
static lv_obj_t *s_frame = NULL;  /* 内框（带边框的白背景） */
static lv_obj_t *s_table = NULL;  /* lv_table */

static lv_obj_t *s_btn_up = NULL, *s_btn_dn = NULL, *s_btn_switch = NULL;

static uint16_t s_rows_total = 0;
static uint16_t s_page = 0;

static char  s_ts [MAX_ROWS][TS_LEN];
static float s_dlv[MAX_ROWS];
static float s_rem[MAX_ROWS];

static ui41_toggle_cb_t s_toggle_cb = NULL;
static void *s_toggle_ud = NULL;

/* ===== 工具：统一去主题/去滚动的容器 ===== */
static void make_panel_plain(lv_obj_t *o, lv_coord_t w, lv_coord_t h, lv_color_t bg, lv_opa_t opa)
{
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_align(o, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa(o,  opa, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

/* ===== 按钮样式 ===== */
static void style_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (btn, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (btn, 12,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_hex(COL_TEXT), LV_PART_MAIN | LV_STATE_ANY);
}

/* ===== 表格绘制事件：把第 0 行当作表头（浅底、加粗） =====
 * 旧版 LVGL：dsc->id 是“单元格索引”，需用列数还原 row/col
 */
static void tbl_draw_event(lv_event_t *e)
{
    lv_obj_t *tbl = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);

    if (dsc->part == LV_PART_ITEMS) {
        uint16_t col_cnt = lv_table_get_col_cnt(tbl);
        uint16_t row = dsc->id / col_cnt;     /* 关键：旧版没有 dsc->id.row，只能这么算 */
        /* uint16_t col = dsc->id % col_cnt;  // 如需列信息，可用 */

        if (row == 0) { /* 表头行 */
            dsc->rect_dsc->bg_opa  = LV_OPA_20;
            dsc->rect_dsc->bg_color = lv_palette_lighten(LV_PALETTE_GREY, 3);
#if LV_FONT_MONTSERRAT_16
            dsc->label_dsc->font = &lv_font_montserrat_16;
#endif
            dsc->label_dsc->color = lv_color_black();
        }
    }
}

/* ===== 刷新表格内容 ===== */
static void refresh_table(void)
{
    if (!s_table) return;

    /* 表头 */
    lv_table_set_cell_value(s_table, 0, 0, "serial");
    lv_table_set_cell_value(s_table, 0, 1, "time");
    lv_table_set_cell_value(s_table, 0, 2, "delivered (kg)");
    lv_table_set_cell_value(s_table, 0, 3, "remain (kg)");

    uint16_t start = s_page * PAGE_ROWS;

    for (uint16_t v = 0; v < PAGE_ROWS; ++v) {
        uint16_t idx = start + v;
        uint16_t tr = v + 1; /* 表格的可视行：跳过表头 */

        if (idx < s_rows_total) {
            static char no[12], t[48], dlv[24], rem[24];
            snprintf(no,  sizeof(no),  "%u", idx + 1);
            snprintf(t,   sizeof(t),   "%s", s_ts[idx][0] ? s_ts[idx] : "--");
            snprintf(dlv, sizeof(dlv), "%.2f", s_dlv[idx]);
            snprintf(rem, sizeof(rem), "%.2f", s_rem[idx]);

            lv_table_set_cell_value(s_table, tr, 0, no);
            lv_table_set_cell_value(s_table, tr, 1, t);
            lv_table_set_cell_value(s_table, tr, 2, dlv);
            lv_table_set_cell_value(s_table, tr, 3, rem);
        } else {
            lv_table_set_cell_value(s_table, tr, 0, "");
            lv_table_set_cell_value(s_table, tr, 1, "");
            lv_table_set_cell_value(s_table, tr, 2, "");
            lv_table_set_cell_value(s_table, tr, 3, "");
        }
    }
}

/* ===== 事件：翻页 / 切换 ===== */
static void on_pager_up(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;
    if (s_page > 0) { --s_page; refresh_table(); }
}
static void on_pager_dn(lv_event_t *e)
{
    if (e->code != LV_EVENT_CLICKED) return;
    uint16_t pc = ui41_get_page_count();
    if (s_page + 1 < pc) { ++s_page; refresh_table(); }
}
static void on_switch_42(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_toggle_cb) s_toggle_cb(s_toggle_ud);
}

/* ===== 创建UI ===== */
void ui41_weighthistory_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    /* 根：800×400，透明、禁滚动 */
    s_root = lv_obj_create(parent_800x400);
    make_panel_plain(s_root, 800, 400, lv_color_hex(COL_PANEL_BG), LV_OPA_TRANSP);

    /* 内框（带边框的容器） */
    s_frame = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_frame);
    lv_obj_set_size(s_frame, g_frame_w, g_frame_h);
    lv_obj_set_pos  (s_frame, g_frame_x, g_frame_y);
    lv_obj_set_style_bg_color  (s_frame, lv_color_white(), 0);
    lv_obj_set_style_bg_opa    (s_frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius    (s_frame, 12, 0);
    lv_obj_set_style_border_width(s_frame, 2, 0);
    lv_obj_set_style_border_color(s_frame, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_pad_all   (s_frame, 10, 0);
    lv_obj_clear_flag(s_frame, LV_OBJ_FLAG_SCROLLABLE);

    /* 表格 */
    s_table = lv_table_create(s_frame);
    lv_obj_set_size(s_table, g_frame_w - 20, g_frame_h - 20);
    lv_obj_center(s_table);
    lv_table_set_col_cnt(s_table, 4);
    lv_table_set_row_cnt(s_table, PAGE_ROWS + 1);
    lv_table_set_col_width(s_table, 0, g_col_w0);
    lv_table_set_col_width(s_table, 1, g_col_w1);
    lv_table_set_col_width(s_table, 2, g_col_w2);
    lv_table_set_col_width(s_table, 3, g_col_w3);

    /* 网格线 + 行内留白 + 文本全居中（旧版没有 lv_table_set_cell_align） */
    lv_obj_set_style_border_width(s_table, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_table, lv_palette_main(LV_PALETTE_GREY), LV_PART_ITEMS);
    lv_obj_set_style_border_opa  (s_table, LV_OPA_60, LV_PART_ITEMS);
    lv_obj_set_style_pad_ver     (s_table, g_row_pad, LV_PART_ITEMS);
    lv_obj_set_style_text_align  (s_table, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS);

    lv_obj_add_event_cb(s_table, tbl_draw_event, LV_EVENT_DRAW_PART_BEGIN, NULL);

    /* 翻页按钮 */
    s_btn_up = lv_btn_create(s_root);  style_btn(s_btn_up);
    lv_obj_set_size(s_btn_up, PAGER_D, PAGER_D);
    lv_obj_set_style_radius(s_btn_up, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(s_btn_up, PAGER_UP_CX - PAGER_D/2, PAGER_UP_CY - PAGER_D/2);
    lv_obj_add_event_cb(s_btn_up, on_pager_up, LV_EVENT_CLICKED, NULL);
    lv_obj_t *u = lv_label_create(s_btn_up); lv_label_set_text(u, LV_SYMBOL_UP); lv_obj_center(u);

    s_btn_dn = lv_btn_create(s_root);  style_btn(s_btn_dn);
    lv_obj_set_size(s_btn_dn, PAGER_D, PAGER_D);
    lv_obj_set_style_radius(s_btn_dn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(s_btn_dn, PAGER_DN_CX - PAGER_D/2, PAGER_DN_CY - PAGER_D/2);
    lv_obj_add_event_cb(s_btn_dn, on_pager_dn, LV_EVENT_CLICKED, NULL);
    lv_obj_t *d = lv_label_create(s_btn_dn); lv_label_set_text(d, LV_SYMBOL_DOWN); lv_obj_center(d);

    /* 右下角切换到 4_2 */
    s_btn_switch = lv_btn_create(s_root); style_btn(s_btn_switch);
    lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
    lv_obj_set_pos(s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    lv_obj_add_event_cb(s_btn_switch, on_switch_42, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(s_btn_switch); lv_label_set_text(lb, "4_2"); lv_obj_center(lb);

    refresh_table();
}

void ui41_weighthistory_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }
    s_frame = s_table = NULL;
    s_btn_up = s_btn_dn = s_btn_switch = NULL;
}

/* ===== 行数/分页 ===== */
void ui41_set_row_count(uint16_t rows)
{
    if (rows < 1) rows = 1;
    if (rows > MAX_ROWS) rows = MAX_ROWS;
    s_rows_total = rows;
    uint16_t pc = ui41_get_page_count();
    if (pc == 0) s_page = 0;
    else if (s_page >= pc) s_page = pc - 1;
    refresh_table();
}
uint16_t ui41_get_row_count(void){ return s_rows_total; }
uint16_t ui41_get_page(void){ return s_page; }
uint16_t ui41_get_page_count(void){ return (s_rows_total + PAGE_ROWS - 1) / PAGE_ROWS; }
void ui41_set_page(uint16_t page){ uint16_t pc=ui41_get_page_count(); if (pc==0) page=0; else if (page>=pc) page=pc-1; s_page=page; refresh_table(); }
void ui41_next_page(void){ uint16_t pc=ui41_get_page_count(); if (s_page+1<pc){ ++s_page; refresh_table(); } }
void ui41_prev_page(void){ if (s_page>0){ --s_page; refresh_table(); } }

/* ===== 数据访问 ===== */
void ui41_set_record(uint16_t row, const char *timestamp, float delivered_kg, float remain_kg)
{
    if (row >= MAX_ROWS) return;
    if (timestamp) {
        strncpy(s_ts[row], timestamp, TS_LEN-1);
        s_ts[row][TS_LEN-1] = '\0';
    } else s_ts[row][0] = '\0';
    s_dlv[row] = delivered_kg;
    s_rem[row] = remain_kg;
    if (row >= s_rows_total) s_rows_total = row + 1;
    refresh_table();
}
void ui41_get_record(uint16_t row, const char **out_timestamp, float *out_delivered_kg, float *out_remain_kg)
{
    if (out_timestamp)    *out_timestamp    = (row<MAX_ROWS)? s_ts[row] : "";
    if (out_delivered_kg) *out_delivered_kg = (row<MAX_ROWS)? s_dlv[row] : 0.0f;
    if (out_remain_kg)    *out_remain_kg    = (row<MAX_ROWS)? s_rem[row] : 0.0f;
}
uint16_t ui41_push_record(const char *timestamp, float delivered_kg, float remain_kg)
{
    if (s_rows_total >= MAX_ROWS) return MAX_ROWS;
    ui41_set_record(s_rows_total, timestamp, delivered_kg, remain_kg);
    return (uint16_t)(s_rows_total - 1);
}
void ui41_clear_all(void)
{
    for (uint16_t i=0;i<MAX_ROWS;++i){ s_ts[i][0]='\0'; s_dlv[i]=0.0f; s_rem[i]=0.0f; }
    s_rows_total = 0; s_page = 0; refresh_table();
}

/* ===== 其他 ===== */
void      ui41_set_toggle_cb(ui41_toggle_cb_t cb, void *user_data){ s_toggle_cb = cb; s_toggle_ud = user_data; }
lv_obj_t* ui41_root(void){ return s_root; }

/* =====（可选）布局调节 ===== */
void ui41_tbl_set_frame(int x, int y, int w, int h)
{
    g_frame_x=x; g_frame_y=y; g_frame_w=w; g_frame_h=h;
    if (s_frame){
        lv_obj_set_pos (s_frame, g_frame_x, g_frame_y);
        lv_obj_set_size(s_frame, g_frame_w, g_frame_h);
    }
    if (s_table){
        lv_obj_set_size(s_table, g_frame_w - 20, g_frame_h - 20);
        lv_obj_center(s_table);
    }
}
void ui41_tbl_set_col_width(int c0, int c1, int c2, int c3)
{
    g_col_w0=c0; g_col_w1=c1; g_col_w2=c2; g_col_w3=c3;
    if (s_table){
        lv_table_set_col_width(s_table, 0, g_col_w0);
        lv_table_set_col_width(s_table, 1, g_col_w1);
        lv_table_set_col_width(s_table, 2, g_col_w2);
        lv_table_set_col_width(s_table, 3, g_col_w3);
    }
}
void ui41_tbl_set_row_pad(int ver_pad)
{
    g_row_pad = ver_pad;
    if (s_table) lv_obj_set_style_pad_ver(s_table, g_row_pad, LV_PART_ITEMS);
}
void ui41_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter)
{
    PAGER_UP_CX=up_cx; PAGER_UP_CY=up_cy; PAGER_DN_CX=dn_cx; PAGER_DN_CY=dn_cy; PAGER_D=diameter;
    if (s_btn_up){ lv_obj_set_size(s_btn_up, PAGER_D, PAGER_D); lv_obj_set_pos(s_btn_up, PAGER_UP_CX-PAGER_D/2, PAGER_UP_CY-PAGER_D/2); }
    if (s_btn_dn){ lv_obj_set_size(s_btn_dn, PAGER_D, PAGER_D); lv_obj_set_pos(s_btn_dn, PAGER_DN_CX-PAGER_D/2, PAGER_DN_CY-PAGER_D/2); }
}
void ui41_set_switch_pos_size(int cx, int cy, int w, int h)
{
    SWITCH_CX=cx; SWITCH_CY=cy; SWITCH_W=w; SWITCH_H=h;
    if (s_btn_switch){
        lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
        lv_obj_set_pos (s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    }
}
