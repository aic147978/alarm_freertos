#include "ui_history.h"
#include <stdio.h>
#include <string.h>

/* ====== 可调参数 ====== */
#define HISTORY_ALARM_MAX   200
#define HISTORY_FEED_MAX    200
#define ROWS_PER_PAGE       7      /* 每页数据行数（不含表头） */
#define RIGHT_PANEL_W       110    /* 右侧按钮区域宽度 */
#define MARGIN_LR           20
#define MARGIN_TB           20

/* ====== 内部状态 ====== */
typedef enum { MODE_ALARM = 0, MODE_FEED = 1 } HistoryMode;

static lv_obj_t *s_root = NULL;          /* 本页根容器（放在你给的 parent 内） */
static lv_obj_t *s_table_alarm = NULL;
static lv_obj_t *s_table_feed  = NULL;
static lv_obj_t *s_btn_up = NULL;
static lv_obj_t *s_btn_down = NULL;
static lv_obj_t *s_btn_switch = NULL;

static HistoryMode s_mode = MODE_ALARM;
static int s_page_idx = 0;               /* 0=最新页；越大越旧 */

/* 数据存储（简单向量/环形加计数实现） */
static AlarmRecord g_alarm[HISTORY_ALARM_MAX];
static size_t      g_alarm_cnt = 0;

static FeedRecord  g_feed[HISTORY_FEED_MAX];
static size_t      g_feed_cnt  = 0;

/* ====== 小工具 ====== */
static inline size_t min_sz(size_t a, size_t b){ return a < b ? a : b; }

static size_t page_count(size_t total)
{
    if (total == 0) return 1; /* 空时也算 1 页，显示空白行 */
    size_t pc = (total + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
    return pc;
}

static void set_table_size_and_pos(lv_obj_t *table)
{
    if(!s_root || !table) return;
    lv_coord_t w = lv_obj_get_width(s_root);
    lv_coord_t h = lv_obj_get_height(s_root);

    lv_coord_t table_w = w - RIGHT_PANEL_W - MARGIN_LR*2;
    lv_coord_t table_h = h - MARGIN_TB*2;

    lv_obj_set_size(table, table_w, table_h);
    lv_obj_align(table, LV_ALIGN_LEFT_MID, MARGIN_LR, 0);
}

static void clear_table_rows(lv_obj_t *table, uint16_t col_cnt)
{
    /* 行数=表头(1)+数据行(ROWS_PER_PAGE) */
    lv_table_set_row_cnt(table, ROWS_PER_PAGE + 1);
    for (uint16_t r = 1; r <= ROWS_PER_PAGE; ++r) {
        for (uint16_t c = 0; c < col_cnt; ++c) {
            lv_table_set_cell_value(table, r, c, "");
        }
    }
}

/* ====== 数据填充（根据 s_mode / s_page_idx 刷表） ====== */
static void fill_alarm_page(void)
{
    lv_obj_clear_flag(s_table_alarm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (s_table_feed,  LV_OBJ_FLAG_HIDDEN);

    /* 表头 */
    lv_table_set_col_cnt(s_table_alarm, 3);
    clear_table_rows(s_table_alarm, 3);
    lv_table_set_cell_value(s_table_alarm, 0, 0, "serial num");
    lv_table_set_cell_value(s_table_alarm, 0, 1, "start time");
    lv_table_set_cell_value(s_table_alarm, 0, 2, "alarm type");

    lv_table_set_col_width(s_table_alarm, 0, 120);
    lv_table_set_col_width(s_table_alarm, 1, 220);
    lv_table_set_col_width(s_table_alarm, 2, 220);

    /* 计算当前页起点：最新在上
       page 0: 显示 [last ... last-ROWS_PER_PAGE+1] */
    size_t total = g_alarm_cnt;
    size_t pc    = page_count(total);
    if ((size_t)s_page_idx >= pc) s_page_idx = (int)pc - 1;

    for (uint16_t r = 1; r <= ROWS_PER_PAGE; ++r) {
        /* i 为要显示的数据序号（从 0..total-1），最新为 total-1 */
        long i = (long)total - 1 - (long)(s_page_idx * ROWS_PER_PAGE) - (long)(r - 1);
        if (i >= 0 && (size_t)i < total) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", g_alarm[i].serial);
            lv_table_set_cell_value(s_table_alarm, r, 0, buf);
            lv_table_set_cell_value(s_table_alarm, r, 1, g_alarm[i].start_time);
            lv_table_set_cell_value(s_table_alarm, r, 2, g_alarm[i].alarm_type);
        }
    }
}

static void fill_feed_page(void)
{
    lv_obj_clear_flag(s_table_feed,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (s_table_alarm, LV_OBJ_FLAG_HIDDEN);

    lv_table_set_col_cnt(s_table_feed, 4);
    clear_table_rows(s_table_feed, 4);
    lv_table_set_cell_value(s_table_feed, 0, 0, "serial num");
    lv_table_set_cell_value(s_table_feed, 0, 1, "time");
    lv_table_set_cell_value(s_table_feed, 0, 2, "transport weight");
    lv_table_set_cell_value(s_table_feed, 0, 3, "surplus weight");

    lv_table_set_col_width(s_table_feed, 0, 100);
    lv_table_set_col_width(s_table_feed, 1, 160);
    lv_table_set_col_width(s_table_feed, 2, 200);
    lv_table_set_col_width(s_table_feed, 3, 200);

    size_t total = g_feed_cnt;
    size_t pc    = page_count(total);
    if ((size_t)s_page_idx >= pc) s_page_idx = (int)pc - 1;

    for (uint16_t r = 1; r <= ROWS_PER_PAGE; ++r) {
        long i = (long)total - 1 - (long)(s_page_idx * ROWS_PER_PAGE) - (long)(r - 1);
        if (i >= 0 && (size_t)i < total) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", g_feed[i].serial);
            lv_table_set_cell_value(s_table_feed, r, 0, buf);
            lv_table_set_cell_value(s_table_feed, r, 1, g_feed[i].time);
            snprintf(buf, sizeof(buf), "%d", g_feed[i].transport_weight);
            lv_table_set_cell_value(s_table_feed, r, 2, buf);
            snprintf(buf, sizeof(buf), "%d", g_feed[i].surplus_weight);
            lv_table_set_cell_value(s_table_feed, r, 3, buf);
        }
    }
}

void history_ui_refresh(void)
{
    if(!s_root) return;
    set_table_size_and_pos(s_table_alarm);
    set_table_size_and_pos(s_table_feed);

    if (s_mode == MODE_ALARM) fill_alarm_page();
    else                      fill_feed_page();
}

/* ====== 事件回调 ====== */
static void ev_page_up(lv_event_t *e)
{
    LV_UNUSED(e);
    s_page_idx++;         /* 更旧一页 */
    history_ui_refresh();
}

static void ev_page_down(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_page_idx > 0) s_page_idx--;  /* 更新一页，最小 0 */
    history_ui_refresh();
}

static void ev_switch_mode(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_mode == MODE_ALARM) s_mode = MODE_FEED;
    else                      s_mode = MODE_ALARM;
    s_page_idx = 0; /* 切模式回到最新页 */
    history_ui_refresh();
}

/* ====== UI 结构创建 ====== */
static void create_buttons(lv_obj_t *parent)
{
    /* 右侧 Up */
    s_btn_up = lv_btn_create(parent);
    lv_obj_set_size(s_btn_up, 50, 50);
    lv_obj_align(s_btn_up, LV_ALIGN_RIGHT_MID, -MARGIN_LR/2, -80);
    lv_obj_add_event_cb(s_btn_up, ev_page_up, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_up), LV_SYMBOL_UP);

    /* 右侧 Down */
    s_btn_down = lv_btn_create(parent);
    lv_obj_set_size(s_btn_down, 50, 50);
    lv_obj_align(s_btn_down, LV_ALIGN_RIGHT_MID, -MARGIN_LR/2, 80);
    lv_obj_add_event_cb(s_btn_down, ev_page_down, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_down), LV_SYMBOL_DOWN);

    /* 右下角切换按钮（刷新图标） */
    s_btn_switch = lv_btn_create(parent);
    lv_obj_set_size(s_btn_switch, 60, 60);
    lv_obj_set_style_radius(s_btn_switch, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_btn_switch, LV_ALIGN_BOTTOM_RIGHT, -MARGIN_LR, -MARGIN_TB);
    lv_obj_add_event_cb(s_btn_switch, ev_switch_mode, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(s_btn_switch), LV_SYMBOL_REFRESH);
}

static void create_tables(lv_obj_t *parent)
{
    s_table_alarm = lv_table_create(parent);
    s_table_feed  = lv_table_create(parent);

    /* 初始只显示报警表 */
    lv_obj_add_flag(s_table_feed, LV_OBJ_FLAG_HIDDEN);

    /* 先做一次布局，设置大小、列宽等 */
    history_ui_refresh();
}

void history_ui_create(lv_obj_t *parent)
{
    /* 根容器（可选：你也可以直接把 table/按钮挂在 parent） */
    s_root = lv_obj_create(parent);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_root, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(s_root, LV_ALIGN_TOP_LEFT, 0, 0);

    create_tables(s_root);
    create_buttons(s_root);
    s_mode = MODE_ALARM;
    s_page_idx = 0;
    history_ui_refresh();
}

/* ====== 公共控制接口 ====== */
void history_ui_switch_to_alarm(void)
{
    s_mode = MODE_ALARM;
    s_page_idx = 0;
    history_ui_refresh();
}

void history_ui_switch_to_feed(void)
{
    s_mode = MODE_FEED;
    s_page_idx = 0;
    history_ui_refresh();
}

void history_ui_page_up(void)   { ev_page_up(NULL);   }
void history_ui_page_down(void) { ev_page_down(NULL); }

void history_ui_goto_last_page(void)
{
    size_t total = (s_mode == MODE_ALARM) ? g_alarm_cnt : g_feed_cnt;
    size_t pc    = page_count(total);
    if(pc == 0) pc = 1;
    s_page_idx = 0;  /* 0 就是最新页（我们定义的显示方式） */
    history_ui_refresh();
}

/* ====== 数据写入 ====== */
static void strlcpy_safe(char *dst, const char *src, size_t dstsz)
{
    if(!dst || dstsz == 0) return;
    if(!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if(n >= dstsz) n = dstsz - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void history_ui_clear_all(void)
{
    g_alarm_cnt = 0;
    g_feed_cnt  = 0;
    s_page_idx  = 0;
    history_ui_refresh();
}

void history_ui_add_alarm(uint32_t serial,
                          const char *start_time,
                          const char *alarm_type)
{
    if (g_alarm_cnt < HISTORY_ALARM_MAX) {
        g_alarm[g_alarm_cnt].serial = serial;
        strlcpy_safe(g_alarm[g_alarm_cnt].start_time, start_time, sizeof(g_alarm[g_alarm_cnt].start_time));
        strlcpy_safe(g_alarm[g_alarm_cnt].alarm_type, alarm_type, sizeof(g_alarm[g_alarm_cnt].alarm_type));
        g_alarm_cnt++;
    } else {
        /* 简单“滚动”策略：丢掉最旧的一条 */
        memmove(&g_alarm[0], &g_alarm[1], (HISTORY_ALARM_MAX-1)*sizeof(AlarmRecord));
        g_alarm[HISTORY_ALARM_MAX-1].serial = serial;
        strlcpy_safe(g_alarm[HISTORY_ALARM_MAX-1].start_time, start_time, sizeof(g_alarm[0].start_time));
        strlcpy_safe(g_alarm[HISTORY_ALARM_MAX-1].alarm_type, alarm_type, sizeof(g_alarm[0].alarm_type));
        g_alarm_cnt = HISTORY_ALARM_MAX;
    }

    /* 若当前显示报警模式且在最新页，直接刷新即可看到变化 */
    if (s_mode == MODE_ALARM && s_page_idx == 0) history_ui_refresh();
}

void history_ui_add_feed(uint32_t serial,
                         const char *time,
                         int transport_weight,
                         int surplus_weight)
{
    if (g_feed_cnt < HISTORY_FEED_MAX) {
        g_feed[g_feed_cnt].serial = serial;
        strlcpy_safe(g_feed[g_feed_cnt].time, time, sizeof(g_feed[g_feed_cnt].time));
        g_feed[g_feed_cnt].transport_weight = transport_weight;
        g_feed[g_feed_cnt].surplus_weight   = surplus_weight;
        g_feed_cnt++;
    } else {
        memmove(&g_feed[0], &g_feed[1], (HISTORY_FEED_MAX-1)*sizeof(FeedRecord));
        g_feed[HISTORY_FEED_MAX-1].serial = serial;
        strlcpy_safe(g_feed[HISTORY_FEED_MAX-1].time, time, sizeof(g_feed[0].time));
        g_feed[HISTORY_FEED_MAX-1].transport_weight = transport_weight;
        g_feed[HISTORY_FEED_MAX-1].surplus_weight   = surplus_weight;
        g_feed_cnt = HISTORY_FEED_MAX;
    }

    if (s_mode == MODE_FEED && s_page_idx == 0) history_ui_refresh();
}

size_t history_ui_get_alarm_count(void) { return g_alarm_cnt; }
size_t history_ui_get_feed_count(void)  { return g_feed_cnt;  }
