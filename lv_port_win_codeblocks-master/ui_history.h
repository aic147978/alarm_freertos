#ifndef UI_HISTORY_H
#define UI_HISTORY_H

#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== 数据结构 ====== */
typedef struct {
    uint32_t serial;            /* 序号 */
    char     start_time[20];    /* 例: "2025-09-19 14:05" */
    char     alarm_type[24];    /* 例: "Overload" */
} AlarmRecord;

typedef struct {
    uint32_t serial;            /* 序号 */
    char     time[20];          /* 例: "2025-09-19 14:06" */
    int      transport_weight;  /* 输送重量 */
    int      surplus_weight;    /* 剩余重量 */
} FeedRecord;

/* ====== 创建/显示 ====== */
/* 在你的中心容器(parent)里创建历史数据界面 */
void history_ui_create(lv_obj_t *parent);

/* 让界面跳到最新页（通常新增数据后调用，让用户看到最新） */
void history_ui_goto_last_page(void);

/* 强制刷新当前页（通常不必手动调，翻页/切换模式/新增数据都会自动刷新） */
void history_ui_refresh(void);

/* ====== 模式与翻页控制（也可直接点按钮） ====== */
void history_ui_switch_to_alarm(void);
void history_ui_switch_to_feed(void);
void history_ui_page_up(void);     /* 更旧一页 */
void history_ui_page_down(void);   /* 更新一页 */

/* ====== 数据接口（供业务层写入） ====== */
void history_ui_clear_all(void);

void ui_history_page_create(lv_obj_t *center_container);

void history_ui_add_alarm(uint32_t serial,
                          const char *start_time,
                          const char *alarm_type);

void history_ui_add_feed(uint32_t serial,
                         const char *time,
                         int transport_weight,
                         int surplus_weight);

/* 可选：获取统计信息 */
size_t history_ui_get_alarm_count(void);
size_t history_ui_get_feed_count(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* UI_HISTORY_H */
