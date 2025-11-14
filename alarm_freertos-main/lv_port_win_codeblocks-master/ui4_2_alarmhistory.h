#ifndef UI4_2_ALARMHISTORY_H
#define UI4_2_ALARMHISTORY_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换 4_1”回调 */
typedef void (*ui42_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
void ui42_alarmhistory_create(lv_obj_t *parent_800x400);
void ui42_alarmhistory_destroy(void);

/* ===== 行数 / 分页（每页 5 行） ===== */
void     ui42_set_row_count(uint16_t rows);
uint16_t ui42_get_row_count(void);
uint16_t ui42_get_page(void);
uint16_t ui42_get_page_count(void);
void     ui42_set_page(uint16_t page);
void     ui42_next_page(void);
void     ui42_prev_page(void);

/* ===== 数据访问（行号 0 起）===== */
void     ui42_set_record(uint16_t row, const char *alarm_type, const char *timestamp);
void     ui42_get_record(uint16_t row, const char **out_alarm_type, const char **out_timestamp);
uint16_t ui42_push_record(const char *alarm_type, const char *timestamp);
void     ui42_clear_all(void);

/* ===== 其他 ===== */
void      ui42_set_toggle_cb(ui42_toggle_cb_t cb, void *user_data);
lv_obj_t* ui42_root(void);

/* =====（可选）布局调节：像素坐标，参考 800×400 左上角(0,0) ===== */
void ui42_tbl_set_frame(int x, int y, int w, int h);           /* 默认 24,12, 752,376 */
void ui42_tbl_set_col_width(int c0, int c1, int c2);           /* 默认 80, 220, 360 */
void ui42_tbl_set_row_pad(int ver_pad);                        /* 默认 10 */
void ui42_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter);
void ui42_set_switch_pos_size(int cx, int cy, int w, int h);

#ifdef __cplusplus
}
#endif
#endif /* UI4_2_ALARMHISTORY_H */
