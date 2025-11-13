#include "timeset.h"
#include "keyboard.h"   /* 使用 keyboard_show_with_cb(...) 弹出数字键盘 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================== 位置与尺寸（像素，含中文注释） ================== */
/* —— 标题行纵向位置 —— */
static lv_coord_t TITLE_Y = 20;  /* 标题行整体的 Y（越小越靠上；影响“标题↔第一行”的距离） */

/* —— 左侧五行的上下基线 ——
 * 第 v 行(0..4) 的基线 Y = L_ROWS_TOP_Y + v*(L_ROWS_BOTTOM_Y - L_ROWS_TOP_Y)/4 */
static lv_coord_t L_ROWS_TOP_Y    = 74;   /* 左：第一行基线 Y */
static lv_coord_t L_ROWS_BOTTOM_Y = 360;  /* 左：第五行基线 Y */

/* —— 右侧五行的上下基线 —— */
static lv_coord_t R_ROWS_TOP_Y    = 74;   /* 右：第一行基线 Y */
static lv_coord_t R_ROWS_BOTTOM_Y = 360;  /* 右：第五行基线 Y */

/* —— 左侧三列 X（序号 / start / end） —— */
static lv_coord_t L_IDX_X   =  96;  /* 左：序号“1.”的 X */
static lv_coord_t L_START_X = 276;  /* 左：start time 按钮的 X */
static lv_coord_t L_END_X   = 468;  /* 左：end   time 按钮的 X */

/* —— 右侧三列 X（序号 / start / end） —— */
static lv_coord_t R_IDX_X   = 760;  /* 右：序号“1.”的 X */
static lv_coord_t R_START_X = 940;  /* 右：start time 按钮的 X */
static lv_coord_t R_END_X   = 1132; /* 右：end   time 按钮的 X */

/* —— 复选框列 X（每行一个启用勾选） —— */
static lv_coord_t CB_X      = 1180; /* 复选框的 X（默认靠右） */

/* —— 时间按钮尺寸 —— */
static lv_coord_t CELL_W = 160;     /* 宽度 */
static lv_coord_t CELL_H = 54;      /* 高度 */

/* —— 右侧翻页按钮（中心点绝对定位） —— */
#define PAGER_UP_CX   1220
#define PAGER_UP_CY   180
#define PAGER_DN_CX   1220
#define PAGER_DN_CY   242
#define PAGER_D       48

/* —— 右下角“切换 1_1/1_2”按钮 —— */
#define SWITCH_W     96
#define SWITCH_H     40
#define SWITCH_CX    1216
#define SWITCH_CY    378

/* —— 颜色 —— */
#define COL_PANEL_BG 0xFFFFFF
#define COL_BTN      0x4D73C9
#define COL_BTN_PR   0x395AA8

/* —— 数据容量与分页 —— */
#define MAX_ROWS   64
#define PAGE_ROWS  5

/* ================== 模块状态 ================== */
static lv_obj_t *s_root = NULL;   /* 根容器(800×400) */
static lv_obj_t *s_title_ls = NULL, *s_title_le = NULL;  /* 左：start / end 标题 */
static lv_obj_t *s_title_rs = NULL, *s_title_re = NULL;  /* 右：start / end 标题 */
static lv_obj_t *s_btn_up = NULL, *s_btn_dn = NULL;      /* 翻页 */
static lv_obj_t *s_btn_switch = NULL;                    /* 右下角切换 */

static uint16_t s_rows_total = 5;
static uint16_t s_page = 0;

/* —— 数据（以“分钟数 0..1439”存储）；destroy 不清空 —— */
static uint16_t s_LS[MAX_ROWS] = {0};  /* 左 start */
static uint16_t s_LE[MAX_ROWS] = {0};  /* 左 end   */
static uint16_t s_RS[MAX_ROWS] = {0};  /* 右 start */
static uint16_t s_RE[MAX_ROWS] = {0};  /* 右 end   */
static uint8_t  s_EN[MAX_ROWS] = {0};  /* 复选框（启用） */

static timeset_toggle_cb_t s_toggle_cb = NULL;
static void *s_toggle_ud = NULL;

/* 每行控件缓存（便于翻页刷新） */
typedef struct {
    lv_obj_t *numL;
    lv_obj_t *btn_LS; lv_obj_t *lbl_LS;
    lv_obj_t *btn_LE; lv_obj_t *lbl_LE;

    lv_obj_t *numR;
    lv_obj_t *btn_RS; lv_obj_t *lbl_RS;
    lv_obj_t *btn_RE; lv_obj_t *lbl_RE;

    lv_obj_t *cb_en;  /* 启用复选框（放右侧） */
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

/* 左/右第 v(0..4) 行的基线 Y */
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
        lv_obj_set_pos(rw->numL,  L_IDX_X,   yL);
        lv_obj_set_pos(rw->btn_LS, L_START_X, yL-16);
        lv_obj_set_pos(rw->btn_LE, L_END_X,   yL-16);

        /* 右侧：序号/两个时间/复选框 */
        lv_obj_set_pos(rw->numR,  R_IDX_X,   yR);
        lv_obj_set_pos(rw->btn_RS, R_START_X, yR-16);
        lv_obj_set_pos(rw->btn_RE, R_END_X,   yR-16);
        lv_obj_set_pos(rw->cb_en,   CB_X,     yR-8);

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
            lv_obj_clear_flag(rw->cb_en,  LV_OBJ_FLAG_HIDDEN);
        }else{
            lv_obj_add_flag(rw->numL,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_LS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_LE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->numR,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_RS,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_RE,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->cb_en,  LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ================== 翻页与切换 ================== */
static void pager_up_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page>0){ --s_page; rebuild_rows(); } }
static void pager_dn_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page+1<timeset_get_page_count()){ ++s_page; rebuild_rows(); } }
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

    /* 右下角“切换” */
    s_btn_switch = lv_btn_create(s_root); style_btn(s_btn_switch);
    lv_obj_add_flag(s_btn_switch, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
    lv_obj_set_pos(s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    lv_obj_add_event_cb(s_btn_switch, switch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(s_btn_switch); lv_label_set_text(lb, "⟳"); lv_obj_center(lb);
}

/* ================== 生命周期 ================== */
void timeset_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    lv_obj_set_size(s_root, 800, 400);
    lv_obj_align(s_root, LV_ALIGN_TOP_MID, 0, 0);

    /* —— 完全禁用滑动 —— */
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_root, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_root, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_style_bg_color(s_root, lv_color_hex(COL_PANEL_BG), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);

    build_titles();
    for (int v=0; v<PAGE_ROWS; ++v) build_one_row(v);
    build_pager_and_switch();
    rebuild_rows();
}

void timeset_destroy(void)
{
    if (s_root){ lv_obj_del(s_root); s_root=NULL; }
    s_title_ls=s_title_le=s_title_rs=s_title_re=NULL;
    s_btn_up=s_btn_dn=s_btn_switch=NULL;
}

/* ================== 行数/分页 ================== */
void timeset_set_row_count(uint16_t rows)
{
    if(rows<1) rows=1; if(rows>MAX_ROWS) rows=MAX_ROWS;
    s_rows_total = rows;
    if(s_page >= timeset_get_page_count()) s_page = timeset_get_page_count()? timeset_get_page_count()-1 : 0;
    rebuild_rows();
}
uint16_t timeset_get_row_count(void){ return s_rows_total; }
uint16_t timeset_get_page(void){ return s_page; }
uint16_t timeset_get_page_count(void){ return (s_rows_total + PAGE_ROWS - 1)/PAGE_ROWS; }
void timeset_set_page(uint16_t p){ if(p>=timeset_get_page_count()) p=timeset_get_page_count()-1; s_page=p; rebuild_rows(); }
void timeset_next_page(void){ if(s_page+1<timeset_get_page_count()){ ++s_page; rebuild_rows(); } }
void timeset_prev_page(void){ if(s_page>0){ --s_page; rebuild_rows(); } }

/* ================== 数据访问 ================== */
static inline uint16_t pick16(uint16_t *a, uint16_t row){ return (row<MAX_ROWS)? a[row] : 0; }
static inline void     poke16(uint16_t *a, uint16_t row, uint16_t v){ if(row<MAX_ROWS) a[row]= (v>1439?1439:v); }

uint16_t timeset_get_left_start_min (uint16_t row){ return pick16(s_LS,row); }
uint16_t timeset_get_left_end_min   (uint16_t row){ return pick16(s_LE,row); }
uint16_t timeset_get_right_start_min(uint16_t row){ return pick16(s_RS,row); }
uint16_t timeset_get_right_end_min  (uint16_t row){ return pick16(s_RE,row); }

void     timeset_set_left_start_min (uint16_t row, uint16_t v){ poke16(s_LS,row,v); rebuild_rows(); }
void     timeset_set_left_end_min   (uint16_t row, uint16_t v){ poke16(s_LE,row,v); rebuild_rows(); }
void     timeset_set_right_start_min(uint16_t row, uint16_t v){ poke16(s_RS,row,v); rebuild_rows(); }
void     timeset_set_right_end_min  (uint16_t row, uint16_t v){ poke16(s_RE,row,v); rebuild_rows(); }

bool     timeset_get_enabled(uint16_t row){ return (row<MAX_ROWS)? (s_EN[row]!=0) : false; }
void     timeset_set_enabled(uint16_t row, bool en){ if(row<MAX_ROWS){ s_EN[row]=en?1:0; rebuild_rows(); } }

void     timeset_clear_all(void)
{
    memset(s_LS,0,sizeof(s_LS));
    memset(s_LE,0,sizeof(s_LE));
    memset(s_RS,0,sizeof(s_RS));
    memset(s_RE,0,sizeof(s_RE));
    memset(s_EN,0,sizeof(s_EN));
    rebuild_rows();
}

/* ================== 其他 ================== */
void      timeset_set_toggle_cb(timeset_toggle_cb_t cb, void *user_data){ s_toggle_cb = cb; s_toggle_ud = user_data; }
lv_obj_t* timeset_root(void){ return s_root; }

/* ================== 布局 setter（即调即生效） ================== */
void timeset_set_title_y(lv_coord_t y)
{
    TITLE_Y = y;
    if(s_title_ls) lv_obj_set_y(s_title_ls, y);
    if(s_title_le) lv_obj_set_y(s_title_le, y);
    if(s_title_rs) lv_obj_set_y(s_title_rs, y);
    if(s_title_re) lv_obj_set_y(s_title_re, y);
}

/* 左/右行距：上/下基线 */
void timeset_set_rows_top_y_left   (lv_coord_t y_top)    { L_ROWS_TOP_Y    = y_top;    rebuild_rows(); }
void timeset_set_rows_bottom_y_left(lv_coord_t y_bottom) { L_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }
void timeset_set_rows_top_y_right  (lv_coord_t y_top)    { R_ROWS_TOP_Y    = y_top;    rebuild_rows(); }
void timeset_set_rows_bottom_y_right(lv_coord_t y_bottom){ R_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }

/* 左/右列 X */
void timeset_set_left_cols (lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x)
{ L_IDX_X = idx_x; L_START_X = start_x; L_END_X = end_x; rebuild_rows(); }

void timeset_set_right_cols(lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x)
{ R_IDX_X = idx_x; R_START_X = start_x; R_END_X = end_x; rebuild_rows(); }

/* 复选框 X */
void timeset_set_checkbox_x(lv_coord_t x)
{ CB_X = x; rebuild_rows(); }

/* 按钮尺寸 */
void timeset_set_cell_size(lv_coord_t w, lv_coord_t h)
{ CELL_W = w; CELL_H = h; rebuild_rows(); }
