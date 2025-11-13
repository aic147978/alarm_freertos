#ifndef TIMESET_H
#define TIMESET_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换 1_1/1_2”回调 */
typedef void (*timeset_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
/* 在父容器(宽800, 高400)中创建 1_2 页面；使用绝对像素定位、禁滚动 */
void timeset_create(lv_obj_t *parent_800x400);
/* 删除 1_2 的 UI（静态数据保留） */
void timeset_destroy(void);

/* ===== 行数 / 分页（与 1_1 保持一致：5 行/页） ===== */
void     timeset_set_row_count(uint16_t rows);  /* 1..64 */
uint16_t timeset_get_row_count(void);
uint16_t timeset_get_page(void);                /* 0-based */
uint16_t timeset_get_page_count(void);
void     timeset_set_page(uint16_t page);
void     timeset_next_page(void);
void     timeset_prev_page(void);

/* ===== 数据访问（行号 0 起；时间以“分钟数(0..1439)”存取）===== */
uint16_t timeset_get_left_start_min (uint16_t row);
uint16_t timeset_get_left_end_min   (uint16_t row);
uint16_t timeset_get_right_start_min(uint16_t row);
uint16_t timeset_get_right_end_min  (uint16_t row);

void     timeset_set_left_start_min (uint16_t row, uint16_t min_of_day);
void     timeset_set_left_end_min   (uint16_t row, uint16_t min_of_day);
void     timeset_set_right_start_min(uint16_t row, uint16_t min_of_day);
void     timeset_set_right_end_min  (uint16_t row, uint16_t min_of_day);

/* 每行的启用复选框 */
bool     timeset_get_enabled(uint16_t row);
void     timeset_set_enabled(uint16_t row, bool en);

/* 一键清零（把所有时间置 00:00，勾选清空） */
void     timeset_clear_all(void);

/* ===== 其他 ===== */
void      timeset_set_toggle_cb(timeset_toggle_cb_t cb, void *user_data); /* 右下角“切换”回调 */
lv_obj_t* timeset_root(void);                                             /* 取根对象 */

/* ===== 显式布局接口（全部像素，参考 800×400 左上角(0,0)） =====
   —— 直接在 app_ui.c 里调用这些 setter 即刻生效 —— */

/* 【标题行纵向位置】默认 20。整体移动四个标题与上边缘的距离 */
void timeset_set_title_y(lv_coord_t y);

/* 【左侧五行上下基线】默认 top=74, bottom=360。只影响左半区行距 */
void timeset_set_rows_top_y_left(lv_coord_t y_top);
void timeset_set_rows_bottom_y_left(lv_coord_t y_bottom);

/* 【右侧五行上下基线】默认与左相同。只影响右半区行距 */
void timeset_set_rows_top_y_right(lv_coord_t y_top);
void timeset_set_rows_bottom_y_right(lv_coord_t y_bottom);

/* 【左侧三列 X】默认 idx=96, start=276, end=468 */
void timeset_set_left_cols(lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x);

/* 【右侧三列 X】默认 idx=760, start=940, end=1132 */
void timeset_set_right_cols(lv_coord_t idx_x, lv_coord_t start_x, lv_coord_t end_x);

/* 【复选框 X】默认 1180（靠右） */
void timeset_set_checkbox_x(lv_coord_t x);

/* 【时间按钮尺寸】默认 160×54 */
void timeset_set_cell_size(lv_coord_t w, lv_coord_t h);

#ifdef __cplusplus
}
#endif
#endif /* TIMESET_H */
