#ifndef UI1_2_TIMESET_H
#define UI1_2_TIMESET_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换回 1_1”回调 */
typedef void (*ui12_timeset_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
/* 在父容器(宽800, 高400)中创建 1_2 页面；使用绝对像素定位、禁滚动 */
void ui12_timeset_create(lv_obj_t *parent_800x400);
/* 删除 1_2 的 UI（静态数据保留） */
void ui12_timeset_destroy(void);

/* ===== 行数 / 分页（固定 5 行/页） ===== */
void     ui12_timeset_set_row_count(uint16_t rows);  /* 1..64 */
uint16_t ui12_timeset_get_row_count(void);
uint16_t ui12_timeset_get_page(void);                /* 0-based */
uint16_t ui12_timeset_get_page_count(void);
void     ui12_timeset_set_page(uint16_t page);
void     ui12_timeset_next_page(void);
void     ui12_timeset_prev_page(void);

/* ===== 数据访问（行号 0 起；时间以“分钟数(0..1439)”存取）===== */
uint16_t ui12_timeset_get_left_start_min (uint16_t row);
uint16_t ui12_timeset_get_left_end_min   (uint16_t row);
uint16_t ui12_timeset_get_right_start_min(uint16_t row);
uint16_t ui12_timeset_get_right_end_min  (uint16_t row);

void     ui12_timeset_set_left_start_min (uint16_t row, uint16_t min_of_day);
void     ui12_timeset_set_left_end_min   (uint16_t row, uint16_t min_of_day);
void     ui12_timeset_set_right_start_min(uint16_t row, uint16_t min_of_day);
void     ui12_timeset_set_right_end_min  (uint16_t row, uint16_t min_of_day);

/* 每行启用复选框 */
bool     ui12_timeset_get_enabled(uint16_t row);
void     ui12_timeset_set_enabled(uint16_t row, bool en);

/* 一键清零（把所有时间置 00:00，勾选清空） */
void     ui12_timeset_clear_all(void);

/* ===== 其他 ===== */
void      ui12_timeset_set_toggle_cb(ui12_timeset_toggle_cb_t cb, void *user_data); /* 右下角“切换 1_1”回调 */
lv_obj_t* ui12_timeset_root(void);                                                  /* 取根对象 */

/* ===== 布局接口（像素，参考 800×400 左上角(0,0)） =====
   —— 即调即生效，可在 create 前/后调用 —— */

/* 标题行纵向位置（默认 20） */
void ui12_timeset_set_title_y(lv_coord_t y);

/* 左/右五行的上下基线（默认 top=64, bottom=360） */
void ui12_timeset_set_rows_top_bottom_left (lv_coord_t y_top, lv_coord_t y_bottom);
void ui12_timeset_set_rows_top_bottom_right(lv_coord_t y_top, lv_coord_t y_bottom);

/* 左/右三列 X（序号 / start / end） */
void ui12_timeset_set_left_cols (lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x);
void ui12_timeset_set_right_cols(lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x);

/* 复选框列 X（默认 750） */
void ui12_timeset_set_checkbox_x(lv_coord_t x);

/* 时间按钮尺寸（默认 150×46） */
void ui12_timeset_set_cell_size(lv_coord_t w, lv_coord_t h);

/* 翻页按钮（圆形）位置：传中心点与直径 */
void ui12_timeset_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter);

/* 右下角“切换 1_1”按钮位置与尺寸：传中心点与宽高 */
void ui12_timeset_set_switch_pos_size(int cx, int cy, int w, int h);

#ifdef __cplusplus
}
#endif
#endif /* UI1_2_TIMESET_H */
