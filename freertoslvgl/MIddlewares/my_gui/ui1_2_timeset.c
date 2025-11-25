#include "ui1_2_timeset.h"
#include "keyboard.h"   /* 需提供 keyboard_show_with_cb(...) */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* atoi */

/* ================== 位置与尺寸（像素） ================== */
/* —— 标题行纵向位置 —— */
static lv_coord_t TITLE_Y = 20;

/* —— 左/右五行上下基线 —— */
static lv_coord_t L_ROWS_TOP_Y    = 64;   /* 左：第一行基线 Y */
static lv_coord_t L_ROWS_BOTTOM_Y = 360;  /* 左：第五行基线 Y */
static lv_coord_t R_ROWS_TOP_Y    = 64;   /* 右：第一行基线 Y */
static lv_coord_t R_ROWS_BOTTOM_Y = 360;  /* 右：第五行基线 Y */

/* —— 左/右三列 X（序号 / start / end） —— */
static lv_coord_t L_IDX_X   =  32;
static lv_coord_t L_START_X =  76;
static lv_coord_t L_END_X   = 242;

static lv_coord_t R_IDX_X   = 420;
static lv_coord_t R_START_X = 464;
static lv_coord_t R_END_X   = 630;

/* —— 复选框列 X —— */
static lv_coord_t CB_X      = 750;

/* —— 时间按钮尺寸 —— */
static lv_coord_t CELL_W = 150;
static lv_coord_t CELL_H = 46;

/* —— 右侧翻页按钮（中心点绝对定位） —— */
static int PAGER_UP_CX = 760;
static int PAGER_UP_CY = 180;
static int PAGER_DN_CX = 760;
static int PAGER_DN_CY = 242;
static int PAGER_D     = 48;

/* —— 右下角“切换 1_1”按钮 —— */
static int SWITCH_CX = 744;
static int SWITCH_CY = 378;
static int SWITCH_W  = 96;
static int SWITCH_H  = 40;

/* —— 颜色 —— */
#define COL_PANEL_BG 0xFFFFFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8

/* —— 数据容量与分页 —— */
#define MAX_ROWS   64
#define PAGE_ROWS  5

/* ================== 模块状态 ================== */
static lv_obj_t *s_root = NULL;   /* 根容器(800×400) */
static lv_obj_t *s_title_ls = NULL, *s_title_le = NULL;  /* 左：start/end 标题 */
static lv_obj_t *s_title_rs = NULL, *s_title_re = NULL;  /* 右：start/end 标题 */
static lv_obj_t *s_btn_up = NULL, *s_btn_dn = NULL;      /* 翻页 */
static lv_obj_t *s_btn_switch = NULL;                    /* 右下角切换 */

static uint16_t s_rows_total = 5;
static uint16_t s_page = 0;

/* —— 数据（destroy 不清空；再次 create 会保留） —— */
static uint16_t s_LS[MAX_ROWS] = {0};  /* 左 start  分钟数 */
static uint16_t s_LE[MAX_ROWS] = {0};  /* 左 end    分钟数 */
static uint16_t s_RS[MAX_ROWS] = {0};  /* 右 start  分钟数 */
static uint16_t s_RE[MAX_ROWS] = {0};  /* 右 end    分钟数 */
static uint8_t  s_EN[MAX_ROWS] = {0};  /* 复选框 启用 */

static ui12_timeset_toggle_cb_t s_toggle_cb = NULL;
static void *s_toggle_ud = NULL;

/* 每行控件缓存（便于翻页刷新） */
typedef struct {
    lv_obj_t *numL;
    lv_obj_t *btn_LS; lv_obj_t *lbl_LS;
    lv_obj_t *btn_LE; lv_obj_t *lbl_LE;

    lv_obj_t *numR;
    lv_obj_t *btn_RS; lv_obj_t *lbl_RS;
    lv_obj_t *btn_RE; lv_obj_t *lbl_RE;

    lv_obj_t *cb_en;  /* 启用复选框（右侧） */
} row_widgets_t;
static row_widgets_t s_rows[PAGE_ROWS];

/* ================== 工具：格式化/解析 ================== */
static void fmt_hhmm(char *out5, uint16_t min_of_day)
{
    if (min_of_day > 1439) min_of_day = 1439;
    uint16_t h = min_of_day / 60;
    uint16_t m = min_of_day % 60;
    sprintf(out5, "%02u:%02u", (unsigned)h, (unsigned)m);
}

/* 支持：空(→0)、2位(→HH:00)、3位(→H:MM)、4位(→HH:MM)；超界自动钳位 */
static uint16_t parse_hhmm_digits(const char *digits)
{
    if (!digits || !digits[0]) return 0;
    int len = (int)strlen(digits);
    int h=0, m=0;
    if (len <= 2) { h = atoi(digits); m = 0; }
    else if (len == 3){ h = digits[0]-'0'; m = atoi(digits+1); }
    else { /* >=4 取前两位小时、后两位分钟 */
        char hh[3]={digits[0], digits[1], 0};
        char mm[3]={digits[len-2], digits[len-1], 0};
        h = atoi(hh); m = atoi(mm);
    }
    if (h<0) h=0; if (h>23) h=23;
    if (m<0) m=0; if (m>59) m=59;
    return (uint16_t)(h*60 + m);
}

/* ================== 样式与小工具 ================== */
static void style_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN),   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR),LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (btn, LV_OPA_COVER,            LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (btn, 12,                      LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_white(),      LV_PART_MAIN | LV_STATE_ANY);
}

static inline lv_coord_t row_y_left (int v){
    return L_ROWS_TOP_Y + (lv_coord_t)((L_ROWS_BOTTOM_Y - L_ROWS_TOP_Y) * v / 4.0f);
}
static inline lv_coord_t row_y_right(int v){
    return R_ROWS_TOP_Y + (lv_coord_t)((R_ROWS_BOTTOM_Y - R_ROWS_TOP_Y) * v / 4.0f);
}

/* ================== 交互：键盘与复选框 ================== */
typedef enum { C_LS=0, C_LE=1, C_RS=2, C_RE=3 } cell_t;

static void kb_done(const char *text, float val_unused, void *ud)
{
    LV_UNUSED(val_unused);
    uint16_t packed = (uint16_t)(uintptr_t)ud;
    uint16_t vis = (packed >> 8) & 0xFF;
    cell_t   ck  = (cell_t)(packed & 0xFF);
    uint16_t idx = s_page*PAGE_ROWS + vis;
    if(idx >= s_rows_total) return;

    uint16_t mm = parse_hhmm_digits(text);
    char t[6]; fmt_hhmm(t, mm);

    switch(ck){
        case C_LS: s_LS[idx]=mm; lv_label_set_text(s_rows[vis].lbl_LS, t); break;
        case C_LE: s_LE[idx]=mm; lv_label_set_text(s_rows[vis].lbl_LE, t); break;
        case C_RS: s_RS[idx]=mm; lv_label_set_text(s_rows[vis].lbl_RS, t); break;
        case C_RE: s_RE[idx]=mm; lv_label_set_text(s_rows[vis].lbl_RE, t); break;
    }
}

static void cell_click_cb(lv_event_t *e)
{
    if(e->code != LV_EVENT_CLICKED) return;
    lv_obj_t *target = lv_event_get_target(e);
    for(int v=0; v<PAGE_ROWS; ++v){
        if(target==s_rows[v].btn_LS){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|C_LS),""); return; }
        if(target==s_rows[v].btn_LE){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|C_LE),""); return; }
        if(target==s_rows[v].btn_RS){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|C_RS),""); return; }
        if(target==s_rows[v].btn_RE){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|C_RE),""); return; }
    }
}

static void cb_event(lv_event_t *e)
{
    if(e->code != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *cb = lv_event_get_target(e);
    for(int v=0; v<PAGE_ROWS; ++v){
        if(cb == s_rows[v].cb_en){
            uint16_t idx = s_page*PAGE_ROWS + v;
            if(idx < s_rows_total) s_EN[idx] = lv_obj_has_state(cb, LV_STATE_CHECKED) ? 1 : 0;
            break;
        }
    }
}

/* ================== 刷新当前页 ================== */
static void rebuild_rows(void)
{
    uint16_t start = s_page*PAGE_ROWS;

    for(int v=0; v<PAGE_ROWS; ++v){
        row_widgets_t *rw = &s_rows[v];

        lv_coord_t yL = row_y_left(v);
        lv_coord_t yR = row_y_right(v);

        uint16_t idx = start + v;
        bool valid = idx < s_rows_total;

        /* 左侧：序号/两个时间 */
        lv_obj_set_pos(rw->numL,   L_IDX_X,   yL);
        lv_obj_set_pos(rw->btn_LS, L_START_X, yL-16);
        lv_obj_set_pos(rw->btn_LE, L_END_X,   yL-16);

        /* 右侧：序号/两个时间/复选框 */
        lv_obj_set_pos(rw->numR,   R_IDX_X,   yR);
        lv_obj_set_pos(rw->btn_RS, R_START_X, yR-16);
        lv_obj_set_pos(rw->btn_RE, R_END_X,   yR-16);
        lv_obj_set_pos(rw->cb_en,  CB_X,      yR-8);

        lv_obj_set_size(rw->btn_LS, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_LE, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_RS, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_RE, CELL_W, CELL_H);

        if(valid){
            char no[8], t[6];
            snprintf(no, sizeof(no), "%u.", idx+1);

            lv_label_set_text(rw->numL, no);
            lv_label_set_text(rw->numR, no);

            fmt_hhmm(t, s_LS[idx]); lv_label_set_text(rw->lbl_LS, t);
            fmt_hhmm(t, s_LE[idx]); lv_label_set_text(rw->lbl_LE, t);
            fmt_hhmm(t, s_RS[idx]); lv_label_set_text(rw->lbl_RS, t);
            fmt_hhmm(t, s_RE[idx]); lv_label_set_text(rw->lbl_RE, t);

            if (s_EN[idx]) lv_obj_add_state(rw->cb_en, LV_STATE_CHECKED);
            else           lv_obj_clear_state(rw->cb_en, LV_STATE_CHECKED);

            lv_obj_clear_flag(rw->numL,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_LS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_LE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->numR,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_RS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_RE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->cb_en, LV_OBJ_FLAG_HIDDEN);
        }else{
            lv_obj_add_flag(rw->numL,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_LS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_LE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->numR,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_RS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_RE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->cb_en, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ================== 翻页与切换 ================== */
static void pager_up_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page>0){ --s_page; rebuild_rows(); } }
static void pager_dn_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page+1<ui12_timeset_get_page_count()){ ++s_page; rebuild_rows(); } }
static void switch_cb  (lv_event_t *e){ LV_UNUSED(e); if(s_toggle_cb) s_toggle_cb(s_toggle_ud); }

/* ================== 构建一次性控件 ================== */
static void build_titles(void)
{
    /* 左：start / end */
    s_title_ls = lv_label_create(s_root);
    lv_label_set_text(s_title_ls, "start time");
    lv_obj_set_pos(s_title_ls, L_START_X + (CELL_W/2 - 36), TITLE_Y);

    s_title_le = lv_label_create(s_root);
    lv_label_set_text(s_title_le, "end time");
    lv_obj_set_pos(s_title_le, L_END_X   + (CELL_W/2 - 30), TITLE_Y);

    /* 右：start / end */
    s_title_rs = lv_label_create(s_root);
    lv_label_set_text(s_title_rs, "start time");
    lv_obj_set_pos(s_title_rs, R_START_X + (CELL_W/2 - 36), TITLE_Y);

    s_title_re = lv_label_create(s_root);
    lv_label_set_text(s_title_re, "end time");
    lv_obj_set_pos(s_title_re, R_END_X   + (CELL_W/2 - 30), TITLE_Y);
}

static void build_one_row(int v)
{
    row_widgets_t *rw = &s_rows[v];

    rw->numL = lv_label_create(s_root);

    rw->btn_LS = lv_btn_create(s_root); style_btn(rw->btn_LS);
    rw->lbl_LS = lv_label_create(rw->btn_LS); lv_obj_center(rw->lbl_LS);
    lv_obj_add_event_cb(rw->btn_LS, cell_click_cb, LV_EVENT_CLICKED, NULL);

    rw->btn_LE = lv_btn_create(s_root); style_btn(rw->btn_LE);
    rw->lbl_LE = lv_label_create(rw->btn_LE); lv_obj_center(rw->lbl_LE);
    lv_obj_add_event_cb(rw->btn_LE, cell_click_cb, LV_EVENT_CLICKED, NULL);

    rw->numR = lv_label_create(s_root);

    rw->btn_RS = lv_btn_create(s_root); style_btn(rw->btn_RS);
    rw->lbl_RS = lv_label_create(rw->btn_RS); lv_obj_center(rw->lbl_RS);
    lv_obj_add_event_cb(rw->btn_RS, cell_click_cb, LV_EVENT_CLICKED, NULL);

    rw->btn_RE = lv_btn_create(s_root); style_btn(rw->btn_RE);
    rw->lbl_RE = lv_label_create(rw->btn_RE); lv_obj_center(rw->lbl_RE);
    lv_obj_add_event_cb(rw->btn_RE, cell_click_cb, LV_EVENT_CLICKED, NULL);

    rw->cb_en = lv_checkbox_create(s_root);
    lv_obj_add_event_cb(rw->cb_en, cb_event, LV_EVENT_VALUE_CHANGED, NULL);
}

static void build_pager_and_switch(void)
{
    /* PageUp */
    s_btn_up = lv_btn_create(s_root); style_btn(s_btn_up);
    lv_obj_add_flag(s_btn_up, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(s_btn_up, PAGER_D, PAGER_D);
    lv_obj_set_style_radius(s_btn_up, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(s_btn_up, PAGER_UP_CX - PAGER_D/2, PAGER_UP_CY - PAGER_D/2);
    lv_obj_add_event_cb(s_btn_up, pager_up_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *u = lv_label_create(s_btn_up); lv_label_set_text(u, LV_SYMBOL_UP); lv_obj_center(u);

    /* PageDown */
    s_btn_dn = lv_btn_create(s_root); style_btn(s_btn_dn);
    lv_obj_add_flag(s_btn_dn, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(s_btn_dn, PAGER_D, PAGER_D);
    lv_obj_set_style_radius(s_btn_dn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(s_btn_dn, PAGER_DN_CX - PAGER_D/2, PAGER_DN_CY - PAGER_D/2);
    lv_obj_add_event_cb(s_btn_dn, pager_dn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *d = lv_label_create(s_btn_dn); lv_label_set_text(d, LV_SYMBOL_DOWN); lv_obj_center(d);

    /* 右下角“切换 1_1” */
    s_btn_switch = lv_btn_create(s_root); style_btn(s_btn_switch);
    lv_obj_add_flag(s_btn_switch, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
    lv_obj_set_pos(s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    lv_obj_add_event_cb(s_btn_switch, switch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(s_btn_switch); lv_label_set_text(lb, "1_1"); lv_obj_center(lb);
}

/* ================== 生命周期 ================== */
void ui12_timeset_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
lv_obj_set_size(s_root, 800, 400);
lv_obj_align(s_root, LV_ALIGN_TOP_MID, 0, 0);
lv_obj_set_style_bg_color(s_root, lv_color_hex(0xFFFFFF), 0); // 或你喜欢的背景色
lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
lv_obj_set_style_pad_all(s_root, 0, 0);
lv_obj_set_style_pad_row(s_root, 0, 0);
lv_obj_set_style_pad_column(s_root, 0, 0);
lv_obj_set_style_border_width(s_root, 0, 0);
lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_set_scroll_dir(s_root, LV_DIR_NONE);
lv_obj_set_scrollbar_mode(s_root, LV_SCROLLBAR_MODE_OFF);

    build_titles();
    for (int v=0; v<PAGE_ROWS; ++v) build_one_row(v);
    build_pager_and_switch();
    rebuild_rows();
}

void ui12_timeset_destroy(void)
{
    if (s_root){ lv_obj_del(s_root); s_root=NULL; }
    s_title_ls=s_title_le=s_title_rs=s_title_re=NULL;
    s_btn_up=s_btn_dn=s_btn_switch=NULL;
}

/* ================== 行数/分页 ================== */
void ui12_timeset_set_row_count(uint16_t rows)
{
    if(rows<1) rows=1; if(rows>MAX_ROWS) rows=MAX_ROWS;
    s_rows_total = rows;
    if(s_page >= ui12_timeset_get_page_count()) s_page = ui12_timeset_get_page_count()? ui12_timeset_get_page_count()-1 : 0;
    rebuild_rows();
}
uint16_t ui12_timeset_get_row_count(void){ return s_rows_total; }
uint16_t ui12_timeset_get_page(void){ return s_page; }
uint16_t ui12_timeset_get_page_count(void){ return (s_rows_total + PAGE_ROWS - 1)/PAGE_ROWS; }
void ui12_timeset_set_page(uint16_t p){ if(p>=ui12_timeset_get_page_count()) p=ui12_timeset_get_page_count()-1; s_page=p; rebuild_rows(); }
void ui12_timeset_next_page(void){ if(s_page+1<ui12_timeset_get_page_count()){ ++s_page; rebuild_rows(); } }
void ui12_timeset_prev_page(void){ if(s_page>0){ --s_page; rebuild_rows(); } }

/* ================== 数据访问 ================== */
static inline uint16_t pick16(uint16_t *a, uint16_t row){ return (row<MAX_ROWS)? a[row] : 0; }
static inline void     poke16(uint16_t *a, uint16_t row, uint16_t v){ if(row<MAX_ROWS) a[row]= (v>1439?1439:v); }

uint16_t ui12_timeset_get_left_start_min (uint16_t row){ return pick16(s_LS,row); }
uint16_t ui12_timeset_get_left_end_min   (uint16_t row){ return pick16(s_LE,row); }
uint16_t ui12_timeset_get_right_start_min(uint16_t row){ return pick16(s_RS,row); }
uint16_t ui12_timeset_get_right_end_min  (uint16_t row){ return pick16(s_RE,row); }

void     ui12_timeset_set_left_start_min (uint16_t row, uint16_t v){ poke16(s_LS,row,v); rebuild_rows(); }
void     ui12_timeset_set_left_end_min   (uint16_t row, uint16_t v){ poke16(s_LE,row,v); rebuild_rows(); }
void     ui12_timeset_set_right_start_min(uint16_t row, uint16_t v){ poke16(s_RS,row,v); rebuild_rows(); }
void     ui12_timeset_set_right_end_min  (uint16_t row, uint16_t v){ poke16(s_RE,row,v); rebuild_rows(); }

bool     ui12_timeset_get_enabled(uint16_t row){ return (row<MAX_ROWS)? (s_EN[row]!=0) : false; }
void     ui12_timeset_set_enabled(uint16_t row, bool en){ if(row<MAX_ROWS){ s_EN[row]=en?1:0; rebuild_rows(); } }

void     ui12_timeset_clear_all(void)
{
    memset(s_LS,0,sizeof(s_LS));
    memset(s_LE,0,sizeof(s_LE));
    memset(s_RS,0,sizeof(s_RS));
    memset(s_RE,0,sizeof(s_RE));
    memset(s_EN,0,sizeof(s_EN));
    rebuild_rows();
}

/* ================== 其他 ================== */
void      ui12_timeset_set_toggle_cb(ui12_timeset_toggle_cb_t cb, void *user_data){ s_toggle_cb = cb; s_toggle_ud = user_data; }
lv_obj_t* ui12_timeset_root(void){ return s_root; }

/* ================== 布局 setter（即调即生效） ================== */
void ui12_timeset_set_title_y(lv_coord_t y)
{
    TITLE_Y = y;
    if(s_title_ls) lv_obj_set_y(s_title_ls, y);
    if(s_title_le) lv_obj_set_y(s_title_le, y);
    if(s_title_rs) lv_obj_set_y(s_title_rs, y);
    if(s_title_re) lv_obj_set_y(s_title_re, y);
}

void ui12_timeset_set_rows_top_bottom_left (lv_coord_t y_top, lv_coord_t y_bottom)
{ L_ROWS_TOP_Y    = y_top;    L_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }

void ui12_timeset_set_rows_top_bottom_right(lv_coord_t y_top, lv_coord_t y_bottom)
{ R_ROWS_TOP_Y    = y_top;    R_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }

void ui12_timeset_set_left_cols (lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x)
{ L_IDX_X = idx_x; L_START_X = start_x; L_END_X = end_x; rebuild_rows(); }

void ui12_timeset_set_right_cols(lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x)
{ R_IDX_X = idx_x; R_START_X = start_x; R_END_X = end_x; rebuild_rows(); }

void ui12_timeset_set_checkbox_x(lv_coord_t x)
{ CB_X = x; rebuild_rows(); }

void ui12_timeset_set_cell_size(lv_coord_t w, lv_coord_t h)
{ CELL_W = w; CELL_H = h; rebuild_rows(); }

void ui12_timeset_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter)
{
    PAGER_UP_CX=up_cx; PAGER_UP_CY=up_cy; PAGER_DN_CX=dn_cx; PAGER_DN_CY=dn_cy; PAGER_D=diameter;
    if(s_btn_up){ lv_obj_set_pos(s_btn_up, PAGER_UP_CX - PAGER_D/2, PAGER_UP_CY - PAGER_D/2); lv_obj_set_size(s_btn_up, PAGER_D, PAGER_D); }
    if(s_btn_dn){ lv_obj_set_pos(s_btn_dn, PAGER_DN_CX - PAGER_D/2, PAGER_DN_CY - PAGER_D/2); lv_obj_set_size(s_btn_dn, PAGER_D, PAGER_D); }
}

void ui12_timeset_set_switch_pos_size(int cx, int cy, int w, int h)
{
    SWITCH_CX=cx; SWITCH_CY=cy; SWITCH_W=w; SWITCH_H=h;
    if(s_btn_switch){
        lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
        lv_obj_set_pos (s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    }
}
