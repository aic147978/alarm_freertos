#include "ui2_3_manual.h"
#include <stdio.h>
#include <string.h>

/* ============= 颜色 & 样式 ============= */
#define COL_BG      0xF5FAFF
#define COL_BTN     0x4D73C9
#define COL_BTN_PR  0x395AA8
#define COL_BTN_CK  0x2D5CC0
#define COL_TEXT    0xFFFFFF
#define COL_TITLE   0x333333

/* ============= 默认布局（像素） ============= */
static int ROW_TOP_Y    = 110;  /* 上排按钮中心 Y */
static int ROW_BOTTOM_Y = 190;  /* 下排按钮中心 Y */
static int INFO_Y       = 250;  /* “重量/时间”文本 Y */
static int DISP_Y       = 285;  /* “已下料重量”文本 Y */

static int BTN_W_DEF = 70;      /* 单个点按钮默认宽度（会按数量自适应缩放） */
static int BTN_H_DEF = 44;
static int MARGIN_X  = 32;      /* 两侧外边距 */

static int ACT_BTN_Y = 330;     /* Open/Close/Exit 按钮的 Y */
static int ACT_W = 140, ACT_H = 50;
static int OPEN_CX  = 250;
static int CLOSE_CX = 420;
static int EXIT_CX  = 690;

/* ============= 上限 ============= */
#define MAX_POINTS  64

/* ============= 状态 ============= */
static lv_obj_t *s_root = NULL;

static bool     s_left_en  = true;
static bool     s_right_en = true;
static uint16_t s_left_n   = 6;
static uint16_t s_right_n  = 6;

/* 两排按钮（动态创建/销毁） */
static lv_obj_t *s_btn_l[MAX_POINTS];
static lv_obj_t *s_btn_r[MAX_POINTS];

/* 当前选中 */
static ui23_side_t s_sel_side  = UI23_SIDE_LEFT;
static uint16_t    s_sel_index = 0;
static bool        s_relay_on  = false;

/* 每点的数据（与 1_1 对应） */
static float s_w_l[MAX_POINTS], s_t_l[MAX_POINTS];
static float s_w_r[MAX_POINTS], s_t_r[MAX_POINTS];

/* 文本 */
static lv_obj_t *s_lbl_left  = NULL;
static lv_obj_t *s_lbl_right = NULL;
static lv_obj_t *s_lbl_info  = NULL;
static lv_obj_t *s_lbl_disp  = NULL;

/* 动作按钮 */
static lv_obj_t *s_btn_open  = NULL;
static lv_obj_t *s_btn_close = NULL;
static lv_obj_t *s_btn_exit  = NULL;

/* 回调 */
static ui23_enter_manual_cb_t s_cb_enter = NULL; static void *s_ud_enter = NULL;
static ui23_open_cb_t         s_cb_open  = NULL; static void *s_ud_open  = NULL;
static ui23_close_cb_t        s_cb_close = NULL; static void *s_ud_close = NULL;
static ui23_exit_cb_t         s_cb_exit  = NULL; static void *s_ud_exit  = NULL;

/* 其他 */
static float s_dispensed_kg = 0.0f;

/* ============= 工具 ============= */
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

static lv_obj_t* make_action_btn(int w, int h, const char *txt)
{
    lv_obj_t *b = lv_btn_create(s_root);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (b, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (b, 14,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(b, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_style_shadow_width(b, 10, 0);
    lv_obj_set_style_shadow_opa (b, LV_OPA_20, 0);
    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt?txt:"");
    lv_obj_center(lab);
    return b;
}

static lv_obj_t* make_point_btn(const char *txt)
{
    lv_obj_t *b = lv_btn_create(s_root);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, BTN_W_DEF, BTN_H_DEF);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN),    LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_CK), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_BTN_PR), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa  (b, LV_OPA_COVER,             LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_radius  (b, 12,                       LV_PART_MAIN | LV_STATE_ANY);
    lv_obj_set_style_text_color(b, lv_color_hex(COL_TEXT), 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_t *lab = lv_label_create(b);
    lv_label_set_text(lab, txt?txt:"1");
    lv_obj_center(lab);
    return b;
}

static void set_btn_center(lv_obj_t *b, int cx, int cy)
{
    lv_obj_set_pos(b, cx - lv_obj_get_width(b)/2, cy - lv_obj_get_height(b)/2);
}

static void update_info_label(void)
{
    char buf[128];
    float w = 0, t = 0;
    if (s_sel_side == UI23_SIDE_LEFT && s_sel_index < s_left_n) {
        w = s_w_l[s_sel_index]; t = s_t_l[s_sel_index];
    } else if (s_sel_side == UI23_SIDE_RIGHT && s_sel_index < s_right_n) {
        w = s_w_r[s_sel_index]; t = s_t_r[s_sel_index];
    }
    snprintf(buf, sizeof(buf), "weight: %.2f kg   time: %.1f s   [%s - %u]",
             w, t, (s_sel_side==UI23_SIDE_LEFT?"L":"R"), (unsigned)(s_sel_index+1));
    lv_label_set_text(s_lbl_info, buf);

    char b2[64];
    snprintf(b2, sizeof(b2), "dispensed: %.2f kg", s_dispensed_kg);
    lv_label_set_text(s_lbl_disp, b2);
}

/* 取消所有 checked 状态（两排） */
static void clear_all_checked(void)
{
    for (uint16_t i=0;i<s_left_n && i<MAX_POINTS;++i)  lv_obj_clear_state(s_btn_l[i], LV_STATE_CHECKED);
    for (uint16_t i=0;i<s_right_n && i<MAX_POINTS;++i) lv_obj_clear_state(s_btn_r[i], LV_STATE_CHECKED);
}

/* ============= 点按钮事件（两排公用） ============= */
static void on_point_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uintptr_t sid  = (uintptr_t)lv_event_get_user_data(e) >> 16;
    uintptr_t idx  = (uintptr_t)lv_event_get_user_data(e) & 0xFFFFu;

    /* 单选：清除其他 checked，仅保持当前 */
    clear_all_checked();
    lv_obj_add_state(btn, LV_STATE_CHECKED);

    s_sel_side  = (ui23_side_t)sid;
    s_sel_index = (uint16_t)idx;

    update_info_label();
}

/* ============= 生成两排按钮 ============= */
static void rebuild_rows(void)
{
    /* 清理旧的 */
    for (uint16_t i=0;i<MAX_POINTS;++i){
        if (s_btn_l[i]) { lv_obj_del(s_btn_l[i]); s_btn_l[i]=NULL; }
        if (s_btn_r[i]) { lv_obj_del(s_btn_r[i]); s_btn_r[i]=NULL; }
    }

    /* 计算统一的按钮宽度与间距（尽量铺满 800-两侧边距） */
    int w = BTN_W_DEF;
    int total_w = 800 - 2*MARGIN_X;
    uint16_t nmax = 0;
    if (s_left_en)  if (s_left_n  > nmax)  nmax = s_left_n;
    if (s_right_en) if (s_right_n > nmax)  nmax = s_right_n;
    if (nmax < 1) nmax = 1;

    /* 优先使用默认宽度，若过宽则压缩 */
    int spacing = (total_w - (int)nmax*w) / ((int)nmax + 1);
    if (spacing < 6) {
        /* 按比例缩小 w，使 spacing 不小于 6 */
        w = (total_w - 6*(int)(nmax+1)) / (int)nmax;
        if (w < 40) w = 40; /* 留一点最小宽 */
        spacing = (total_w - (int)nmax*w) / ((int)nmax + 1);
    }

    /* 上排（左） */
    if (s_left_en) {
        for (uint16_t i=0;i<s_left_n && i<MAX_POINTS;++i){
            char t[6]; snprintf(t,sizeof t,"%u",(unsigned)(i+1));
            s_btn_l[i] = make_point_btn(t);
            lv_obj_set_size(s_btn_l[i], w, BTN_H_DEF);
            int cx = MARGIN_X + spacing*(i+1) + w*i + w/2;
            set_btn_center(s_btn_l[i], cx, ROW_TOP_Y);
            /* 用 user_data 打包 side(高16位)+idx(低16位) */
            uintptr_t ud = (((uintptr_t)UI23_SIDE_LEFT)<<16) | (uintptr_t)i;
            lv_obj_add_event_cb(s_btn_l[i], on_point_clicked, LV_EVENT_CLICKED, (void*)ud);
        }
    }

    /* 下排（右） */
    if (s_right_en) {
        for (uint16_t i=0;i<s_right_n && i<MAX_POINTS;++i){
            char t[6]; snprintf(t,sizeof t,"%u",(unsigned)(i+1));
            s_btn_r[i] = make_point_btn(t);
            lv_obj_set_size(s_btn_r[i], w, BTN_H_DEF);
            int cx = MARGIN_X + spacing*(i+1) + w*i + w/2;
            set_btn_center(s_btn_r[i], cx, ROW_BOTTOM_Y);
            uintptr_t ud = (((uintptr_t)UI23_SIDE_RIGHT)<<16) | (uintptr_t)i;
            lv_obj_add_event_cb(s_btn_r[i], on_point_clicked, LV_EVENT_CLICKED, (void*)ud);
        }
    }

    /* 选中一个合理的默认项 */
    if (s_left_en && s_left_n>0)      { s_sel_side = UI23_SIDE_LEFT;  s_sel_index = 0; }
    else if (s_right_en && s_right_n>0){ s_sel_side = UI23_SIDE_RIGHT; s_sel_index = 0; }
    else                               { s_sel_side = UI23_SIDE_NONE;  s_sel_index = 0; }

    clear_all_checked();
    if (s_sel_side == UI23_SIDE_LEFT  && s_left_n>0)  lv_obj_add_state(s_btn_l[0], LV_STATE_CHECKED);
    if (s_sel_side == UI23_SIDE_RIGHT && s_right_n>0) lv_obj_add_state(s_btn_r[0], LV_STATE_CHECKED);

    update_info_label();
}

/* ============= 按钮事件 ============= */
static void on_open(lv_event_t *e)
{
    LV_UNUSED(e);
    if (!s_cb_open) return;
    if (s_sel_side == UI23_SIDE_NONE) return;
    s_relay_on = true;
    s_cb_open(s_sel_side, s_sel_index, s_ud_open);
}
static void on_close(lv_event_t *e)
{
    LV_UNUSED(e);
    if (!s_cb_close) return;
    if (s_sel_side == UI23_SIDE_NONE) return;
    s_relay_on = false;
    s_cb_close(s_sel_side, s_sel_index, s_ud_close);
}
static void on_exit(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_cb_exit) s_cb_exit(s_ud_exit);
}

/* ============= 创建 / 销毁 ============= */
void ui23_manual_create(lv_obj_t *parent_800x400)
{
    if (s_root || !parent_800x400) return;

    s_root = lv_obj_create(parent_800x400);
    make_plain(s_root, 800, 400, lv_color_hex(COL_BG), LV_OPA_TRANSP);

    /* 顶部左右标题（说明上/下排含义） */
    s_lbl_left  = lv_label_create(s_root);
    s_lbl_right = lv_label_create(s_root);
    lv_obj_remove_style_all(s_lbl_left);
    lv_obj_remove_style_all(s_lbl_right);
    lv_label_set_text(s_lbl_left,  "left line");
    lv_label_set_text(s_lbl_right, "right line");
    lv_obj_set_style_text_color(s_lbl_left,  lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_color(s_lbl_right, lv_color_hex(COL_TITLE), 0);
    lv_obj_align(s_lbl_left,  LV_ALIGN_TOP_LEFT,  80, 60);
    lv_obj_align(s_lbl_right, LV_ALIGN_TOP_RIGHT, -120, 60);

    /* 信息栏 */
    s_lbl_info = lv_label_create(s_root);
    s_lbl_disp = lv_label_create(s_root);
    lv_obj_remove_style_all(s_lbl_info);
    lv_obj_remove_style_all(s_lbl_disp);
    lv_obj_set_style_text_color(s_lbl_info, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_color(s_lbl_disp, lv_color_hex(COL_TITLE), 0);
    lv_obj_align(s_lbl_info, LV_ALIGN_TOP_LEFT,  40, INFO_Y);
    lv_obj_align(s_lbl_disp, LV_ALIGN_TOP_LEFT,  40, DISP_Y);

    /* 动作按钮：Open / Close / Exit */
    s_btn_open  = make_action_btn(ACT_W, ACT_H, "Open");
    s_btn_close = make_action_btn(ACT_W, ACT_H, "Close");
    s_btn_exit  = make_action_btn(ACT_W, ACT_H, "Exit");
    set_btn_center(s_btn_open,  OPEN_CX,  ACT_BTN_Y);
    set_btn_center(s_btn_close, CLOSE_CX, ACT_BTN_Y);
    set_btn_center(s_btn_exit,  EXIT_CX,  ACT_BTN_Y);
    lv_obj_add_event_cb(s_btn_open,  on_open,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_close, on_close, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_exit,  on_exit,  LV_EVENT_CLICKED, NULL);

    /* 生成两排按钮 */
    rebuild_rows();

    /* 进入手动模式的通知 */
    if (s_cb_enter) s_cb_enter(s_ud_enter);
}

void ui23_manual_destroy(void)
{
    if (s_root) { lv_obj_del(s_root); s_root = NULL; }
    memset(s_btn_l, 0, sizeof(s_btn_l));
    memset(s_btn_r, 0, sizeof(s_btn_r));
    s_lbl_left = s_lbl_right = s_lbl_info = s_lbl_disp = NULL;
    s_btn_open = s_btn_close = s_btn_exit = NULL;
}

/* ============= Root & 回调绑定 ============= */
lv_obj_t* ui23_manual_root(void){ return s_root; }
void ui23_set_enter_manual_cb(ui23_enter_manual_cb_t cb, void *ud){ s_cb_enter = cb; s_ud_enter = ud; }
void ui23_set_open_cb (ui23_open_cb_t  cb, void *ud){ s_cb_open  = cb; s_ud_open  = ud; }
void ui23_set_close_cb(ui23_close_cb_t cb, void *ud){ s_cb_close = cb; s_ud_close = ud; }
void ui23_set_exit_cb (ui23_exit_cb_t  cb, void *ud){ s_cb_exit  = cb; s_ud_exit  = ud; }

/* ============= Lineset 联动 ============= */
void ui23_apply_lineset(bool left_enable, uint16_t left_count,
                        bool right_enable, uint16_t right_count)
{
    s_left_en  = left_enable;
    s_right_en = right_enable;
    if (left_count  > MAX_POINTS) left_count  = MAX_POINTS;
    if (right_count > MAX_POINTS) right_count = MAX_POINTS;
    s_left_n  = left_count;
    s_right_n = right_count;
    if (s_root) rebuild_rows();
}

/* ============= 选择/状态 ============= */
void ui23_set_selected(ui23_side_t side, uint16_t index)
{
    s_sel_side = side; s_sel_index = index;
    clear_all_checked();
    if (side == UI23_SIDE_LEFT  && index < s_left_n  && s_btn_l[index]) lv_obj_add_state(s_btn_l[index], LV_STATE_CHECKED);
    if (side == UI23_SIDE_RIGHT && index < s_right_n && s_btn_r[index]) lv_obj_add_state(s_btn_r[index], LV_STATE_CHECKED);
    update_info_label();
}
ui23_side_t ui23_get_selected_side(void){ return s_sel_side; }
uint16_t    ui23_get_selected_index(void){ return s_sel_index; }

void ui23_set_relay_on(bool on){ s_relay_on = on; /* 你也可以在这里改变 Open/Close 按钮的外观 */ }
bool ui23_is_relay_on(void){ return s_relay_on; }

/* ============= 点数据 ============= */
void ui23_set_point_info(ui23_side_t side, uint16_t index, float weight_kg, float time_sec)
{
    if (index >= MAX_POINTS) return;
    if (side == UI23_SIDE_LEFT)  { s_w_l[index]=weight_kg; s_t_l[index]=time_sec; }
    if (side == UI23_SIDE_RIGHT) { s_w_r[index]=weight_kg; s_t_r[index]=time_sec; }
    if (side==s_sel_side && index==s_sel_index && s_root) update_info_label();
}
void ui23_get_point_info(ui23_side_t side, uint16_t index, float *out_w, float *out_t)
{
    float w=0, t=0;
    if (index < MAX_POINTS){
        if (side==UI23_SIDE_LEFT)  { w=s_w_l[index]; t=s_t_l[index]; }
        if (side==UI23_SIDE_RIGHT) { w=s_w_r[index]; t=s_t_r[index]; }
    }
    if (out_w) *out_w = w; if (out_t) *out_t = t;
}

/* ============= 累计下料重量 ============= */
void  ui23_set_dispensed_kg(float kg){ s_dispensed_kg = kg; if (s_root) update_info_label(); }
float ui23_get_dispensed_kg(void){ return s_dispensed_kg; }

/* ============= 可选：布局 ============= */
void ui23_layout_reset_default(void)
{
    ROW_TOP_Y=110; ROW_BOTTOM_Y=190; INFO_Y=250; DISP_Y=285;
    BTN_W_DEF=70; BTN_H_DEF=44; MARGIN_X=32;
    ACT_BTN_Y=330; ACT_W=140; ACT_H=50; OPEN_CX=250; CLOSE_CX=420; EXIT_CX=690;

    if (!s_root) return;
    if (s_btn_open){ lv_obj_set_size(s_btn_open, ACT_W, ACT_H); set_btn_center(s_btn_open, OPEN_CX, ACT_BTN_Y); }
    if (s_btn_close){ lv_obj_set_size(s_btn_close, ACT_W, ACT_H); set_btn_center(s_btn_close, CLOSE_CX, ACT_BTN_Y); }
    if (s_btn_exit){ lv_obj_set_size(s_btn_exit, ACT_W, ACT_H); set_btn_center(s_btn_exit, EXIT_CX, ACT_BTN_Y); }
    lv_obj_align(s_lbl_info, LV_ALIGN_TOP_LEFT, 40, INFO_Y);
    lv_obj_align(s_lbl_disp, LV_ALIGN_TOP_LEFT, 40, DISP_Y);
    /* 重新排布两排按钮 */
    rebuild_rows();
}
void ui23_layout_set_rows_y(int top_row_y, int bottom_row_y)
{
    ROW_TOP_Y = top_row_y; ROW_BOTTOM_Y = bottom_row_y;
    if (s_root) rebuild_rows();
}
void ui23_layout_set_status_y(int info_y, int disp_y)
{
    INFO_Y = info_y; DISP_Y = disp_y;
    if (s_lbl_info) lv_obj_align(s_lbl_info, LV_ALIGN_TOP_LEFT, 40, INFO_Y);
    if (s_lbl_disp) lv_obj_align(s_lbl_disp, LV_ALIGN_TOP_LEFT, 40, DISP_Y);
}
void ui23_layout_set_buttons(int open_cx, int close_cx, int exit_cx, int btn_y, int btn_w, int btn_h)
{
    OPEN_CX=open_cx; CLOSE_CX=close_cx; EXIT_CX=exit_cx; ACT_BTN_Y=btn_y; ACT_W=btn_w; ACT_H=btn_h;
    if (s_btn_open){  lv_obj_set_size(s_btn_open,  ACT_W, ACT_H); set_btn_center(s_btn_open,  OPEN_CX,  ACT_BTN_Y); }
    if (s_btn_close){ lv_obj_set_size(s_btn_close, ACT_W, ACT_H); set_btn_center(s_btn_close, CLOSE_CX, ACT_BTN_Y); }
    if (s_btn_exit){  lv_obj_set_size(s_btn_exit,  ACT_W, ACT_H); set_btn_center(s_btn_exit,  EXIT_CX,  ACT_BTN_Y); }
}
