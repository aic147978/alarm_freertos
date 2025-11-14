#ifndef UI4_1_WEIGHTHISTORY_H
#define UI4_1_WEIGHTHISTORY_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换 4_2”回调 */
typedef void (*ui41_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
void ui41_weighthistory_create(lv_obj_t *parent_800x400); /* 在父容器(800×400)中创建 */
void ui41_weighthistory_destroy(void);                    /* 仅删UI，保留静态数据 */

/* ===== 行数 / 分页（每页 5 行） ===== */
void     ui41_set_row_count(uint16_t rows);  /* 1..200 */
uint16_t ui41_get_row_count(void);
uint16_t ui41_get_page(void);                /* 0-based */
uint16_t ui41_get_page_count(void);
void     ui41_set_page(uint16_t page);
void     ui41_next_page(void);
void     ui41_prev_page(void);

/* ===== 数据访问（行号 0 起）===== */
void     ui41_set_record(uint16_t row, const char *timestamp, float delivered_kg, float remain_kg);
void     ui41_get_record(uint16_t row, const char **out_timestamp, float *out_delivered_kg, float *out_remain_kg);
uint16_t ui41_push_record(const char *timestamp, float delivered_kg, float remain_kg);
void     ui41_clear_all(void);

/* ===== 其他 ===== */
void      ui41_set_toggle_cb(ui41_toggle_cb_t cb, void *user_data);
lv_obj_t* ui41_root(void);

/* =====（可选）布局调节：像素坐标，参考 800×400 左上角(0,0) =====
   注意：换成表格后不再有“标题 y / 行上下基线”概念；
   这里提供几个与表格匹配的调节接口，可按需调用。 */
void ui41_tbl_set_frame(int x, int y, int w, int h);                /* 表格所在的内框（默认 24,12, 752, 376） */
void ui41_tbl_set_col_width(int c0, int c1, int c2, int c3);        /* 列宽（默认 80, 240, 180, 180） */
void ui41_tbl_set_row_pad(int ver_pad);                             /* 行内上下留白（默认 10） */
void ui41_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter);
void ui41_set_switch_pos_size(int cx, int cy, int w, int h);

#ifdef __cplusplus
}
#endif
#endif /* UI4_1_WEIGHTHISTORY_H */
