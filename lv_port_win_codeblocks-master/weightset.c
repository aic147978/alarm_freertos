#include "weightset.h"
#include "keyboard.h"   /* 使用 keyboard_show_with_cb(...) 调出数字键盘 */
#include <stdio.h>
#include <string.h>

/* ================== 位置与尺寸（全部像素，含中文注释） ================== */
/* —— 标题行纵向位置 —— */
static lv_coord_t TITLE_Y = 12;  /* 标题行的 Y 坐标（越小越靠上；影响“标题↔第一行”的距离） */

/* —— 左侧五行的“第一行/第五行”基线 ——
 * 说明：第 v 行(0..4) 的 Y = L_ROWS_TOP_Y + v*(L_ROWS_BOTTOM_Y - L_ROWS_TOP_Y)/4
 *       只影响左侧；你可以把左/右设置为不同范围以“左右行距分开设计”。 */
static lv_coord_t L_ROWS_TOP_Y    = 64;   /* 左侧第一行的基线 Y（越小越靠上） */
static lv_coord_t L_ROWS_BOTTOM_Y = 330;  /* 左侧第五行的基线 Y（越大越靠下） */

/* —— 右侧五行的“第一行/第五行”基线 ——
 * 说明：同上，但只影响右侧。默认与左侧相同，也可以单独调以改变右侧行距。 */
static lv_coord_t R_ROWS_TOP_Y    = 64;   /* 右侧第一行基线 Y */
static lv_coord_t R_ROWS_BOTTOM_Y = 330;  /* 右侧第五行基线 Y */

/* —— 左侧三列的 X 坐标 ——
 * 说明：控制左侧“序号/重量/时间”三列横向位置；你可以整体向左/右平移或改变列间距。 */
static lv_coord_t L_IDX_X  = 1;   /* 左序号的 X（例如“1.”） */
static lv_coord_t L_SET_X  = 32;   /* 左“重量(set)”按钮的 X */
static lv_coord_t L_TIME_X = 210;  /* 左“时间(time)”按钮的 X */

/* —— 右侧三列的 X 坐标 ——
 * 说明：同上，但作用于右侧；可与左侧不同，实现左右独立的列距/边距。 */
static lv_coord_t R_IDX_X  = 360;  /* 右序号的 X */
static lv_coord_t R_SET_X  = 404;  /* 右“重量(set)”按钮的 X */
static lv_coord_t R_TIME_X = 570;  /* 右“时间(time)”按钮的 X */

/* —— 数据按钮尺寸 ——
 * 说明：四个蓝色按钮（左/右的 set/time）统一尺寸；放大可让“行看起来更高”。 */
static lv_coord_t CELL_W = 130;    /* 按钮宽度 */
static lv_coord_t CELL_H = 48;     /* 按钮高度 */

/* —— 右侧翻页按钮（忽略布局，按“中心点”绝对定位） ——
 * 说明：PageUp/Down 两个圆形按钮的中心点坐标与直径。 */
#define PAGER_UP_CX   740
#define PAGER_UP_CY   180
#define PAGER_DN_CX   740
#define PAGER_DN_CY   242
#define PAGER_D       48    /* 圆形按钮的直径 */

/* —— 右下角“1_2”切换按钮（忽略布局，绝对定位） —— */
#define SWITCH_W     64
#define SWITCH_H     40
#define SWITCH_CX    732
#define SWITCH_CY    356

/* —— 背景与按钮颜色（可按需调整） —— */
#define COL_PANEL_BG 0xF2F6FB   /* 背景淡蓝灰 */
#define COL_BTN      0x4D73C9   /* 正常蓝 */
#define COL_BTN_PR   0x395AA8   /* 按下更深蓝 */

/* —— 数据容量与分页 —— */
#define MAX_ROWS   64
#define PAGE_ROWS  5

/* ================== 模块状态 ================== */
static lv_obj_t *s_root = NULL;          /* 根容器(800×400) */
static lv_obj_t *s_title_l = NULL;       /* 左标题：“left set   time” */
static lv_obj_t *s_title_r = NULL;       /* 右标题：“right set  time” */
static lv_obj_t *s_btn_up = NULL;        /* PageUp */
static lv_obj_t *s_btn_dn = NULL;        /* PageDown */
static lv_obj_t *s_btn_switch = NULL;    /* 右下角“1_2”切换 */

static uint16_t s_rows_total = 5;
static uint16_t s_page = 0;

/* —— 数据（静态缓冲；destroy 不清空，重新 create 会保留） —— */
static float s_wL[MAX_ROWS] = {0};
static float s_tL[MAX_ROWS] = {0};
static float s_wR[MAX_ROWS] = {0};
static float s_tR[MAX_ROWS] = {0};

static weightset_toggle_cb_t s_toggle_cb = NULL;
static void *s_toggle_ud = NULL;

/* 每行控件缓存（便于翻页刷新，不重复创建） */
typedef struct {
    lv_obj_t *numL;
    lv_obj_t *btn_wL; lv_obj_t *lbl_wL;
    lv_obj_t *btn_tL; lv_obj_t *lbl_tL;
    lv_obj_t *numR;
    lv_obj_t *btn_wR; lv_obj_t *lbl_wR;
    lv_obj_t *btn_tR; lv_obj_t *lbl_tR;
} row_widgets_t;
static row_widgets_t s_rows[PAGE_ROWS];

/* ================== 小工具函数 ================== */
static void style_btn(lv_obj_t *btn)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN),   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COL_BTN_PR),LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (btn, LV_OPA_COVER,            LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (btn, 16,                      LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(btn, lv_color_white(),      LV_PART_MAIN | LV_STATE_ANY);
}

static void set_lbl_w(lv_obj_t *lbl, float v){
    char buf[24]; snprintf(buf, sizeof(buf), "%.2fkg", v);
    int n = (int)strlen(buf)-3;
    while(n>0 && buf[n]=='0'){ buf[n]='\0'; n--; if(buf[n]=='.'){ buf[n]='\0'; break; } }
    lv_label_set_text(lbl, buf);
}
static void set_lbl_t(lv_obj_t *lbl, float v){
    char buf[24]; snprintf(buf, sizeof(buf), "%.2fs", v);
    int n = (int)strlen(buf)-2;
    while(n>0 && buf[n]=='0'){ buf[n]='\0'; n--; if(buf[n]=='.'){ buf[n]='\0'; break; } }
    lv_label_set_text(lbl, buf);
}

/* 左/右各自的“第 v 行(0..4) 基线 Y”——用于实现“左右行距分开设计” */
static inline lv_coord_t row_y_left (int v){
    return L_ROWS_TOP_Y + (lv_coord_t)((L_ROWS_BOTTOM_Y - L_ROWS_TOP_Y) * v / 4.0f);
}
static inline lv_coord_t row_y_right(int v){
    return R_ROWS_TOP_Y + (lv_coord_t)((R_ROWS_BOTTOM_Y - R_ROWS_TOP_Y) * v / 4.0f);
}

/* ================== 键盘回调与点击事件 ================== */
typedef enum { CELL_WL=0, CELL_TL=1, CELL_WR=2, CELL_TR=3 } cell_t;

static void kb_done(const char *text, float val, void *ud)
{
    LV_UNUSED(text);
    uint16_t packed = (uint16_t)(uintptr_t)ud;
    uint16_t vis = (packed >> 8) & 0xFF;
    cell_t   ck  = (cell_t)(packed & 0xFF);
    uint16_t idx = s_page*PAGE_ROWS + vis;
    if(idx >= s_rows_total) return;

    switch(ck){
        case CELL_WL: s_wL[idx]=val; set_lbl_w(s_rows[vis].lbl_wL,val); break;
        case CELL_TL: s_tL[idx]=val; set_lbl_t(s_rows[vis].lbl_tL,val); break;
        case CELL_WR: s_wR[idx]=val; set_lbl_w(s_rows[vis].lbl_wR,val); break;
        case CELL_TR: s_tR[idx]=val; set_lbl_t(s_rows[vis].lbl_tR,val); break;
    }
}

static void cell_click_cb(lv_event_t *e)
{
    if(e->code != LV_EVENT_CLICKED) return;
    lv_obj_t *target = lv_event_get_target(e);
    for(int v=0; v<PAGE_ROWS; ++v){
        if(target==s_rows[v].btn_wL){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|CELL_WL),""); return; }
        if(target==s_rows[v].btn_tL){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|CELL_TL),""); return; }
        if(target==s_rows[v].btn_wR){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|CELL_WR),""); return; }
        if(target==s_rows[v].btn_tR){ keyboard_show_with_cb(kb_done,(void*)(uintptr_t)((v<<8)|CELL_TR),""); return; }
    }
}

/* ================== 刷新当前页 ================== */
static void rebuild_rows(void)
{
    uint16_t start = s_page*PAGE_ROWS;

    for(int v=0; v<PAGE_ROWS; ++v){
        row_widgets_t *rw = &s_rows[v];

        /* 左右各自的行基线 Y（实现“左右行距分开”） */
        lv_coord_t yL = row_y_left(v);
        lv_coord_t yR = row_y_right(v);

        uint16_t idx = start + v;
        bool valid = idx < s_rows_total;

        /* 左：序号/重量/时间 */
        lv_obj_set_pos(rw->numL,  L_IDX_X,  yL);
        lv_obj_set_pos(rw->btn_wL, L_SET_X,  yL-12);
        lv_obj_set_pos(rw->btn_tL, L_TIME_X, yL-12);

        /* 右：序号/重量/时间 */
        lv_obj_set_pos(rw->numR,  R_IDX_X,  yR);
        lv_obj_set_pos(rw->btn_wR, R_SET_X,  yR-12);
        lv_obj_set_pos(rw->btn_tR, R_TIME_X, yR-12);

        /* 按钮尺寸 */
        lv_obj_set_size(rw->btn_wL, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_tL, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_wR, CELL_W, CELL_H);
        lv_obj_set_size(rw->btn_tR, CELL_W, CELL_H);

        if(valid){
            char no[8]; snprintf(no, sizeof(no), "%u.", idx+1);
            lv_label_set_text(rw->numL, no);
            lv_label_set_text(rw->numR, no);

            set_lbl_w(rw->lbl_wL, s_wL[idx]);
            set_lbl_t(rw->lbl_tL, s_tL[idx]);
            set_lbl_w(rw->lbl_wR, s_wR[idx]);
            set_lbl_t(rw->lbl_tR, s_tR[idx]);

            lv_obj_clear_flag(rw->numL,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_wL,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_tL,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->numR,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_wR,LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(rw->btn_tR,LV_OBJ_FLAG_HIDDEN);
        }else{
            lv_obj_add_flag(rw->numL,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_wL,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_tL,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->numR,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_wR,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(rw->btn_tR,LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ================== 翻页与切换 ================== */
static void pager_up_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page>0){ --s_page; rebuild_rows(); } }
static void pager_dn_cb(lv_event_t *e){ LV_UNUSED(e); if(s_page+1<weightset_get_page_count()){ ++s_page; rebuild_rows(); } }
static void switch_cb  (lv_event_t *e){ LV_UNUSED(e); if(s_toggle_cb) s_toggle_cb(s_toggle_ud); }

/* ================== 构建一次性控件 ================== */
static void build_titles(void)
{
    /* 左标题（连写空格保证“set”和“time”在一行内视觉居中） */
    s_title_l = lv_label_create(s_root);
    lv_label_set_text(s_title_l, "left set                time");
    lv_obj_set_pos(s_title_l, L_SET_X, TITLE_Y);

    /* 右标题 */
    s_title_r = lv_label_create(s_root);
    lv_label_set_text(s_title_r, "right set               time");
    lv_obj_set_pos(s_title_r, R_SET_X, TITLE_Y);
}

static void build_one_row(int v)
{
    row_widgets_t *rw = &s_rows[v];

    /* 左编号 */
    rw->numL = lv_label_create(s_root);

    /* 左 set */
    rw->btn_wL = lv_btn_create(s_root); style_btn(rw->btn_wL);
    rw->lbl_wL = lv_label_create(rw->btn_wL); lv_obj_center(rw->lbl_wL);
    lv_obj_add_event_cb(rw->btn_wL, cell_click_cb, LV_EVENT_CLICKED, NULL);

    /* 左 time */
    rw->btn_tL = lv_btn_create(s_root); style_btn(rw->btn_tL);
    rw->lbl_tL = lv_label_create(rw->btn_tL); lv_obj_center(rw->lbl_tL);
    lv_obj_add_event_cb(rw->btn_tL, cell_click_cb, LV_EVENT_CLICKED, NULL);

    /* 右编号 */
    rw->numR = lv_label_create(s_root);

    /* 右 set */
    rw->btn_wR = lv_btn_create(s_root); style_btn(rw->btn_wR);
    rw->lbl_wR = lv_label_create(rw->btn_wR); lv_obj_center(rw->lbl_wR);
    lv_obj_add_event_cb(rw->btn_wR, cell_click_cb, LV_EVENT_CLICKED, NULL);

    /* 右 time */
    rw->btn_tR = lv_btn_create(s_root); style_btn(rw->btn_tR);
    rw->lbl_tR = lv_label_create(rw->btn_tR); lv_obj_center(rw->lbl_tR);
    lv_obj_add_event_cb(rw->btn_tR, cell_click_cb, LV_EVENT_CLICKED, NULL);
}

static void build_pager_and_switch(void)
{
    /* PageUp（忽略布局，绝对定位；圆形） */
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

    /* 右下角“1_2” */
    s_btn_switch = lv_btn_create(s_root); style_btn(s_btn_switch);
    lv_obj_add_flag(s_btn_switch, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(s_btn_switch, SWITCH_W, SWITCH_H);
    lv_obj_set_pos(s_btn_switch, SWITCH_CX - SWITCH_W/2, SWITCH_CY - SWITCH_H/2);
    lv_obj_add_event_cb(s_btn_switch, switch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(s_btn_switch); lv_label_set_text(lb, "1_2"); lv_obj_center(lb);
}

/* ================== 生命周期实现 ================== */
void weightset_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    lv_obj_set_size(s_root, 800, 400);
    lv_obj_align(s_root, LV_ALIGN_TOP_MID, 0, 0);

    /* —— 完全禁用滑动 —— */
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);       /* 不可滚动 */
    lv_obj_set_scroll_dir(s_root, LV_DIR_NONE);              /* 不允许任何方向的滚动 */
    lv_obj_set_scrollbar_mode(s_root, LV_SCROLLBAR_MODE_OFF);/* 关闭滚动条 */

    /* 背景 */
    lv_obj_set_style_bg_color(s_root, lv_color_hex(COL_PANEL_BG), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);

    build_titles();
    for (int v=0; v<PAGE_ROWS; ++v) build_one_row(v);
    build_pager_and_switch();
    rebuild_rows();
}

void weightset_destroy(void)
{
    if (s_root){ lv_obj_del(s_root); s_root=NULL; }
    s_title_l=s_title_r=s_btn_up=s_btn_dn=s_btn_switch=NULL;
}

/* ================== 行数/分页 ================== */
void weightset_set_row_count(uint16_t rows)
{
    if(rows<1) rows=1; if(rows>MAX_ROWS) rows=MAX_ROWS;
    s_rows_total = rows;
    if(s_page >= weightset_get_page_count()) s_page = weightset_get_page_count()? weightset_get_page_count()-1 : 0;
    rebuild_rows();
}
uint16_t weightset_get_row_count(void){ return s_rows_total; }
uint16_t weightset_get_page(void){ return s_page; }
uint16_t weightset_get_page_count(void){ return (s_rows_total + PAGE_ROWS - 1)/PAGE_ROWS; }
void weightset_set_page(uint16_t p){ if(p>=weightset_get_page_count()) p=weightset_get_page_count()-1; s_page=p; rebuild_rows(); }
void weightset_next_page(void){ if(s_page+1<weightset_get_page_count()){ ++s_page; rebuild_rows(); } }
void weightset_prev_page(void){ if(s_page>0){ --s_page; rebuild_rows(); } }

/* ================== 数据访问 ================== */
static inline float pick(float *a, uint16_t row){ return (row<MAX_ROWS)? a[row] : 0.f; }
static inline void  poke(float *a, uint16_t row, float v){ if(row<MAX_ROWS) a[row]=v; }

float weightset_get_left_weight (uint16_t row){ return pick(s_wL,row); }
float weightset_get_left_time   (uint16_t row){ return pick(s_tL,row); }
float weightset_get_right_weight(uint16_t row){ return pick(s_wR,row); }
float weightset_get_right_time  (uint16_t row){ return pick(s_tR,row); }

void  weightset_set_left_weight (uint16_t row,float v){ poke(s_wL,row,v); rebuild_rows(); }
void  weightset_set_left_time   (uint16_t row,float v){ poke(s_tL,row,v); rebuild_rows(); }
void  weightset_set_right_weight(uint16_t row,float v){ poke(s_wR,row,v); rebuild_rows(); }
void  weightset_set_right_time  (uint16_t row,float v){ poke(s_tR,row,v); rebuild_rows(); }

void  weightset_clear_all(void)
{
    memset(s_wL,0,sizeof(s_wL)); memset(s_tL,0,sizeof(s_tL));
    memset(s_wR,0,sizeof(s_wR)); memset(s_tR,0,sizeof(s_tR));
    rebuild_rows();
}

/* ================== 其他 ================== */
void weightset_set_toggle_cb(weightset_toggle_cb_t cb, void *user_data){ s_toggle_cb = cb; s_toggle_ud = user_data; }
lv_obj_t* weightset_root(void){ return s_root; }

/* ================== 显式布局 setter（即调即生效） ================== */
void weightset_set_title_y(lv_coord_t y)
{
    TITLE_Y = y;
    if(s_title_l) lv_obj_set_y(s_title_l, y);
    if(s_title_r) lv_obj_set_y(s_title_r, y);
}

/* 左侧行距：上/下基线 */
void weightset_set_rows_top_y_left(lv_coord_t y_top)          { L_ROWS_TOP_Y = y_top;       rebuild_rows(); }
void weightset_set_rows_bottom_y_left(lv_coord_t y_bottom)    { L_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }

/* 右侧行距：上/下基线 */
void weightset_set_rows_top_y_right(lv_coord_t y_top)         { R_ROWS_TOP_Y = y_top;       rebuild_rows(); }
void weightset_set_rows_bottom_y_right(lv_coord_t y_bottom)   { R_ROWS_BOTTOM_Y = y_bottom; rebuild_rows(); }

/* 左/右三列横向位置 */
void weightset_set_left_cols (lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x)
{ L_IDX_X = idx_x; L_SET_X = set_x; L_TIME_X = time_x; rebuild_rows(); }
void weightset_set_right_cols(lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x)
{ R_IDX_X = idx_x; R_SET_X = set_x; R_TIME_X = time_x; rebuild_rows(); }

/* 数据按钮尺寸 */
void weightset_set_cell_size(lv_coord_t w, lv_coord_t h)
{ CELL_W = w; CELL_H = h; rebuild_rows(); }
