/* ui_manual.c — 手动模式独立界面
 *
 * 依赖：
 *   - LVGL
 *   - ui_keypad（九键输入框）
 *   - ui_left1（提供自动模式的只读数据 & 快照接口）
 *
 * 弱依赖（可选）：
 *   - settings_ui_show_menu()   // 退出时回到设置菜单；如果没有此函数，退出按钮不做导航
 */

#include "ui_manual.h"
#include "ui_keypad.h"
#include "ui_left1.h"     /* 取阀门数量与自动模式数据 */
#include <string.h>

/* ===== 可视布局参数（可按屏幕微调） ===== */
#define TOP_H            260      /* 顶部绘图区域高度（两条线+阀门） */
#define MARGIN_LR        20
#define MARGIN_TB        20

#define SILO_W           170      /* 左侧料塔宽/高 */
#define SILO_H           170

#define LINE_THICK       12       /* 水平线条厚度 */
#define VALVE_W          56       /* 阀门“方块”的大小（默认用按钮代替图片） */
#define VALVE_H          72
#define VALVE_GAP_MIN    8        /* 阀门最小间隙 */

#define CTRL_ROW_Y       (TOP_H + 16) /* 控制区起始Y */
#define BTN_W            180
#define BTN_H            56

/* ===== 运行时对象 ===== */
static lv_obj_t *s_root = NULL;       /* 本页根容器（放在你传入的 parent 内） */

/* 顶部绘制用对象缓存 */
#define MANUAL_MAX  32
static lv_obj_t *s_silo      = NULL;
static lv_obj_t *s_line_top  = NULL;
static lv_obj_t *s_line_bot  = NULL;
static lv_obj_t *s_valve_top[MANUAL_MAX]; /* 上行每个阀门对象（按钮/图片） */
static lv_obj_t *s_valve_bot[MANUAL_MAX]; /* 下行每个阀门对象 */
static int       s_valve_count = 0;

/* 选中态：只允许一个被选中（跨两行互斥） */
static int s_sel_line  = 0;     /* 0=上(左线) 1=下(右线) */
static int s_sel_index = 0;     /* 0..count-1 */

/* 下方参数与按钮 */
static lv_obj_t *s_lbl_title_w = NULL; /* "setting weigh" */
static lv_obj_t *s_lbl_title_t = NULL; /* "setting time"  */

static lv_obj_t *s_btn_weigh   = NULL; /* 数值按钮（九键） */
static lv_obj_t *s_btn_time    = NULL;
static lv_obj_t *s_lbl_weigh   = NULL; /* 数值文字（按钮里的 label） */
static lv_obj_t *s_lbl_time    = NULL;

static ui_bind_entry_t s_ent_weigh = {0};
static ui_bind_entry_t s_ent_time  = {0};

static lv_obj_t *s_btn_start = NULL;
static lv_obj_t *s_btn_stop  = NULL;
static lv_obj_t *s_btn_exit  = NULL;

static manual_exit_cb_t s_exit_cb = NULL;

void manual_ui_set_exit_cb(manual_exit_cb_t cb) {
    s_exit_cb = cb;
}
/* ===== 手动页“副本数据”——与自动模式完全解耦 ===== */
static int  g_m_left_w [MANUAL_MAX] = {0};
static int  g_m_left_t [MANUAL_MAX] = {0}; /* 秒 */
static int  g_m_right_w[MANUAL_MAX] = {0};
static int  g_m_right_t[MANUAL_MAX] = {0};
static bool g_m_enabled [MANUAL_MAX] = {0}; /* 目前没用到，预留 */

/* 运行状态与对外输出位（★★由你在外部绑定实际变量地址） */
static bool g_running = false;

/* ★★你要填写地址的变量（通过 manual_ui_bind_outputs 绑定） */
static bool *ADDR_MANUAL_START = NULL; /* Start/Suspend 输出位地址（外部变量地址） */
static bool *ADDR_MANUAL_STOP  = NULL; /* Stop 输出位地址（外部变量地址） */

/* 退出回调（弱符号） */
__attribute__((weak)) void settings_ui_show_menu(void){}

/* ========== 小工具 ========== */
static void no_scroll(lv_obj_t *o){
    if(!o) return;
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(o, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static inline int clampi(int v, int lo, int hi){
    if(v < lo) return lo; if(v > hi) return hi; return v;
}

/* ========== 选中态刷新（高亮当前阀门） ========== */
static void apply_valve_highlight(void)
{
    for(int i=0;i<s_valve_count;i++){
        if(s_valve_top[i]){
            if(s_sel_line==0 && s_sel_index==i) lv_obj_add_state(s_valve_top[i], LV_STATE_CHECKED);
            else                                 lv_obj_clear_state(s_valve_top[i], LV_STATE_CHECKED);
        }
        if(s_valve_bot[i]){
            if(s_sel_line==1 && s_sel_index==i) lv_obj_add_state(s_valve_bot[i], LV_STATE_CHECKED);
            else                                 lv_obj_clear_state(s_valve_bot[i], LV_STATE_CHECKED);
        }
    }
}

/* 将底部两个数值按钮“绑定”到当前选中的副本变量 */
static void bind_bottom_entries_to_selected(void)
{
    int *pW, *pT;
    if(s_sel_line==0){ /* 上行=left line */
        pW = &g_m_left_w[s_sel_index];
        pT = &g_m_left_t[s_sel_index];
    }else{
        pW = &g_m_right_w[s_sel_index];
        pT = &g_m_right_t[s_sel_index];
    }

    /* 重新初始化 entry，使九键弹窗操作的是当前指针 */
    ui_keypad_entry_init(&s_ent_weigh, s_lbl_weigh, pW, "kg", false, 3);
    ui_keypad_entry_init(&s_ent_time,  s_lbl_time,  pT, "s",  false, 4);
}

/* 点击阀门（统一回调） */
static void ev_valve_clicked(lv_event_t *e)
{
    uintptr_t pack = (uintptr_t)lv_event_get_user_data(e);
    int line  = (int)((pack >> 16) & 0xFFFF);
    int index = (int)(pack & 0xFFFF);

    s_sel_line  = clampi(line,  0, 1);
    s_sel_index = clampi(index, 0, s_valve_count-1);

    apply_valve_highlight();
    bind_bottom_entries_to_selected();
}

/* ========== 顶部绘制（料塔+两条线+阀门） ========== */
static void rebuild_top_area(void)
{
    /* 清旧阀门对象 */
    for(int i=0;i<MANUAL_MAX;i++){
        if(s_valve_top[i]){ lv_obj_del(s_valve_top[i]); s_valve_top[i]=NULL; }
        if(s_valve_bot[i]){ lv_obj_del(s_valve_bot[i]); s_valve_bot[i]=NULL; }
    }

    /* 读取阀门数量（来自自动模式） */
    s_valve_count = ui_left1_get_valve1_count();
    if(s_valve_count < 1) s_valve_count = 1;
    if(s_valve_count > MANUAL_MAX) s_valve_count = MANUAL_MAX;

    /* 根容器大小 */
    lv_coord_t W = lv_obj_get_width (s_root);
    lv_coord_t H = lv_obj_get_height(s_root);

    /* 料塔 */
    if(!s_silo) s_silo = lv_obj_create(s_root);
    no_scroll(s_silo);
    lv_obj_set_size(s_silo, SILO_W, SILO_H);
    lv_obj_align(s_silo, LV_ALIGN_TOP_LEFT, MARGIN_LR, MARGIN_TB);
    lv_obj_set_style_bg_color(s_silo, lv_palette_main(LV_PALETTE_BLUE), 0);

    /* 两条横线位置（Y 坐标） */
    int line_y_top = MARGIN_TB + SILO_H/2 - 32;   /* 上线 */
    int line_y_bot = MARGIN_TB + SILO_H/2 + 32;   /* 下线 */

    /* 线条：从料塔右侧到屏幕右边距 */
    int line_x0 = MARGIN_LR + SILO_W + 12;
    int line_x1 = W - MARGIN_LR - 16;

    if(!s_line_top) s_line_top = lv_obj_create(s_root);
    if(!s_line_bot) s_line_bot = lv_obj_create(s_root);

    for(int k=0;k<2;k++){
        lv_obj_t *o = (k==0)? s_line_top : s_line_bot;
        no_scroll(o);
        lv_obj_set_size(o, line_x1 - line_x0, LINE_THICK);
        lv_obj_align(o, LV_ALIGN_TOP_LEFT, line_x0, (k==0)? line_y_top : line_y_bot);
        lv_obj_set_style_bg_color(o, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_set_style_radius(o, LINE_THICK/2, 0);
    }

    /* 阀门的横向排布：等间距 */
    int usable_w = (line_x1 - line_x0) - 40;          /* 预留一点边距 */
    int total_valve_w = s_valve_count * VALVE_W;
    int gaps = s_valve_count + 1;
    int gap_w = (usable_w - total_valve_w) / gaps;
    if(gap_w < VALVE_GAP_MIN) gap_w = VALVE_GAP_MIN;

    int x = line_x0 + 20 + gap_w;

    for(int i=0;i<s_valve_count;i++){
        /* 上行（left line） */
        s_valve_top[i] = lv_btn_create(s_root);
        lv_obj_set_size(s_valve_top[i], VALVE_W, VALVE_H);
        lv_obj_align(s_valve_top[i], LV_ALIGN_TOP_LEFT, x, line_y_top - VALVE_H/2);
        lv_obj_add_event_cb(s_valve_top[i], ev_valve_clicked, LV_EVENT_CLICKED, (void*)(uintptr_t)((0<<16)|i));
        lv_obj_set_style_bg_color(s_valve_top[i], lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_set_style_radius(s_valve_top[i], 6, 0);

        /* 下行（right line） */
        s_valve_bot[i] = lv_btn_create(s_root);
        lv_obj_set_size(s_valve_bot[i], VALVE_W, VALVE_H);
        lv_obj_align(s_valve_bot[i], LV_ALIGN_TOP_LEFT, x, line_y_bot - VALVE_H/2);
        lv_obj_add_event_cb(s_valve_bot[i], ev_valve_clicked, LV_EVENT_CLICKED, (void*)(uintptr_t)((1<<16)|i));
        lv_obj_set_style_bg_color(s_valve_bot[i], lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_obj_set_style_radius(s_valve_bot[i], 6, 0);

        x += VALVE_W + gap_w;

        /* 提示：你可以把上面两个 lv_btn 替换成 lv_imgbtn，用你的两张数组图片：
           - 普通图（未选中）、高亮图（选中）
           - ev_valve_clicked 里仍使用相同 user_data 方案
        */
    }

    /* 确保一个有效的默认选中 */
    s_sel_index = clampi(s_sel_index, 0, s_valve_count-1);
    s_sel_line  = clampi(s_sel_line,  0, 1);
    apply_valve_highlight();
}

/* ========== 底部控制区 ========== */
static void ev_start(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!ADDR_MANUAL_START || !ADDR_MANUAL_STOP) return; /* 未绑定就不动作 */

    g_running = !g_running;
    if(g_running){
        *ADDR_MANUAL_START = true;
        *ADDR_MANUAL_STOP  = false;
        lv_label_set_text(lv_label_create(s_btn_start), ""); /* 清旧 */
        lv_obj_clean(s_btn_start);
        lv_obj_t *lab = lv_label_create(s_btn_start);
        lv_label_set_text(lab, "suspend");
        lv_obj_center(lab);
    }else{
        *ADDR_MANUAL_START = false;  /* 如果你有单独“暂停位”，可在此写入 */
        *ADDR_MANUAL_STOP  = false;
        lv_obj_clean(s_btn_start);
        lv_obj_t *lab = lv_label_create(s_btn_start);
        lv_label_set_text(lab, "start");
        lv_obj_center(lab);
    }
}

static void ev_stop(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!ADDR_MANUAL_START || !ADDR_MANUAL_STOP) return;
    *ADDR_MANUAL_STOP  = true;
    *ADDR_MANUAL_START = false;
    g_running = false;

    lv_obj_clean(s_btn_start);
    lv_obj_t *lab = lv_label_create(s_btn_start);
    lv_label_set_text(lab, "start");
    lv_obj_center(lab);
}

static void manual_on_exit(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_exit_cb) s_exit_cb();   // 交给外部处理隐藏/返回
}



static void build_bottom_controls(void)
{
    lv_coord_t W = lv_obj_get_width(s_root);

    /* 左：setting weigh */
    s_lbl_title_w = lv_label_create(s_root);
    lv_label_set_text(s_lbl_title_w, "setting weigh");
    lv_obj_align(s_lbl_title_w, LV_ALIGN_TOP_LEFT, MARGIN_LR + 80, CTRL_ROW_Y);

    s_btn_weigh = lv_btn_create(s_root);
    lv_obj_set_size(s_btn_weigh, BTN_W, BTN_H);
    lv_obj_align(s_btn_weigh, LV_ALIGN_TOP_LEFT, MARGIN_LR + 60, CTRL_ROW_Y + 28);
    s_lbl_weigh = lv_label_create(s_btn_weigh);
    lv_label_set_text(s_lbl_weigh, "0kg");
    lv_obj_center(s_lbl_weigh);

    /* 右：setting time */
    s_lbl_title_t = lv_label_create(s_root);
    lv_label_set_text(s_lbl_title_t, "setting time");
    lv_obj_align(s_lbl_title_t, LV_ALIGN_TOP_RIGHT, -MARGIN_LR - 260, CTRL_ROW_Y);

    s_btn_time = lv_btn_create(s_root);
    lv_obj_set_size(s_btn_time, BTN_W, BTN_H);
    lv_obj_align(s_btn_time, LV_ALIGN_TOP_RIGHT, -MARGIN_LR - 260, CTRL_ROW_Y + 28);
    s_lbl_time = lv_label_create(s_btn_time);
    lv_label_set_text(s_lbl_time, "0s");
    lv_obj_center(s_lbl_time);

    /* 绑定九键 */
    ui_keypad_entry_init(&s_ent_weigh, s_lbl_weigh, &g_m_left_w[0], "kg", false, 3);
    ui_keypad_bind_button(s_btn_weigh, &s_ent_weigh);

    ui_keypad_entry_init(&s_ent_time,  s_lbl_time,  &g_m_left_t[0], "s",  false, 4);
    ui_keypad_bind_button(s_btn_time,  &s_ent_time);

    /* 底部：start / stop / exit */
    s_btn_start = lv_btn_create(s_root);
    lv_obj_set_size(s_btn_start, 260, BTN_H);
    lv_obj_align(s_btn_start, LV_ALIGN_BOTTOM_LEFT, MARGIN_LR + 80, -MARGIN_TB);
    lv_obj_add_event_cb(s_btn_start, ev_start, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_start), "start");

    s_btn_stop = lv_btn_create(s_root);
    lv_obj_set_size(s_btn_stop, 260, BTN_H);
    lv_obj_align(s_btn_stop, LV_ALIGN_BOTTOM_MID, 0, -MARGIN_TB);
    lv_obj_add_event_cb(s_btn_stop, ev_stop, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_stop), "stop");



    // ... 创建 Exit 按钮处：
lv_obj_t *btn_exit = lv_btn_create(s_root);
lv_obj_set_size(btn_exit, 160, 56);
lv_obj_align(btn_exit, LV_ALIGN_BOTTOM_RIGHT, -24, -16);
lv_obj_add_event_cb(btn_exit, manual_on_exit, LV_EVENT_CLICKED, NULL);
lv_label_set_text(lv_label_create(btn_exit), "exit");
}

/* ========== 对外接口实现 ========== */
void manual_ui_create(lv_obj_t *parent)
{
    if(s_root) return;
    s_root = lv_obj_create(parent);
    no_scroll(s_root);
    lv_obj_set_size(s_root, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 先做一次数据同步（如果外部还没调，也无妨） */
    manual_ui_sync_from_auto();

    rebuild_top_area();
    build_bottom_controls();

    /* 初次绑定当前选中项 */
    bind_bottom_entries_to_selected();
}

void manual_ui_destroy(void)
{
    ui_keypad_close(); /* 关掉可能存在的键盘弹窗 */

    if(s_root){ lv_obj_del(s_root); s_root = NULL; }

    s_silo = s_line_top = s_line_bot = NULL;
    for(int i=0;i<MANUAL_MAX;i++){ s_valve_top[i]=s_valve_bot[i]=NULL; }

    s_valve_count = 0;
    s_sel_line = s_sel_index = 0;

    s_lbl_title_w = s_lbl_title_t = NULL;
    s_btn_weigh = s_btn_time = NULL;
    s_lbl_weigh = s_lbl_time = NULL;
    memset(&s_ent_weigh, 0, sizeof(s_ent_weigh));
    memset(&s_ent_time,  0, sizeof(s_ent_time));

    s_btn_start = s_btn_stop = s_btn_exit = NULL;

    /* 不清空绑定地址，允许复用 */
}

void manual_ui_sync_from_auto(void)
{
    /* 拷贝 A 页的四组数组（0..count-1）到手动页 */
    int count = ui_left1_get_valve1_count();
    if(count > MANUAL_MAX) count = MANUAL_MAX;
    if(count < 1) count = 1;

    ui_left1_snapshot_pageA(0, g_m_left_w, g_m_left_t, g_m_right_w, g_m_right_t, count);

    /* 如果你还想拷贝 B 页（启用/时间区间）：
       int tmp_start[MANUAL_MAX], tmp_end[MANUAL_MAX];
       ui_left1_snapshot_pageB(0, tmp_start, tmp_end, g_m_enabled, count);
       // 此处先不使用 start/end；保留 g_m_enabled 可用于后续逻辑
     */

    s_valve_count = count;
    s_sel_line    = clampi(s_sel_line, 0, 1);
    s_sel_index   = clampi(s_sel_index, 0, s_valve_count-1);
}

void manual_ui_bind_outputs(bool *p_start, bool *p_stop)
{
    /* ★★你要在外部把“业务层变量地址”传进来 */
    ADDR_MANUAL_START = p_start;
    ADDR_MANUAL_STOP  = p_stop;
}

void manual_ui_set_selected(int line, int index)
{
    s_sel_line  = clampi(line,  0, 1);
    s_sel_index = clampi(index, 0, s_valve_count-1);
    apply_valve_highlight();
    bind_bottom_entries_to_selected();
}

int  manual_ui_get_selected_line(void)  { return s_sel_line;  }
int  manual_ui_get_selected_index(void) { return s_sel_index; }

void manual_ui_redraw(void)
{
    if(!s_root) return;
    rebuild_top_area();
    bind_bottom_entries_to_selected();
}

bool manual_ui_is_running(void) { return g_running; }
