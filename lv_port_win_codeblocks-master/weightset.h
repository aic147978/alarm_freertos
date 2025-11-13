#ifndef WEIGHTSET_H
#define WEIGHTSET_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换 1_2”回调 */
typedef void (*weightset_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
/* 在父容器(宽800, 高400)中创建 1_1 页面；本模块使用绝对像素，不依赖自动布局 */
void weightset_create(lv_obj_t *parent_800x400);
/* 删除 1_1 的 UI（静态数组中的数据不清空；再次创建会保留上次的数值） */
void weightset_destroy(void);

/* ===== 行数 / 分页 ===== */
void     weightset_set_row_count(uint16_t rows);  /* 设置总行数(1..64)，分页自动计算 */
uint16_t weightset_get_row_count(void);
uint16_t weightset_get_page(void);                /* 当前页(0-based) */
uint16_t weightset_get_page_count(void);
void     weightset_set_page(uint16_t page);
void     weightset_next_page(void);
void     weightset_prev_page(void);

/* ===== 数据访问（行号 0 起）===== */
float weightset_get_left_weight (uint16_t row);
float weightset_get_left_time   (uint16_t row);   /* 秒 */
float weightset_get_right_weight(uint16_t row);
float weightset_get_right_time  (uint16_t row);

void  weightset_set_left_weight (uint16_t row, float kg);
void  weightset_set_left_time   (uint16_t row, float sec);
void  weightset_set_right_weight(uint16_t row, float kg);
void  weightset_set_right_time  (uint16_t row, float sec);

/* 一键清空为 0（仅清数据，不动 UI） */
void  weightset_clear_all(void);

/* ===== 其他 ===== */
void      weightset_set_toggle_cb(weightset_toggle_cb_t cb, void *user_data); /* 设置右下角“1_2”切换回调 */
lv_obj_t* weightset_root(void);                                               /* 取得根对象(便于外部做样式) */

/* ===== 显式布局接口（全部是“像素”，相对于 800×400 顶左(0,0)） =====
   —— 你可以在 app_ui.c 中直接调用以下 setter 即刻生效 —— */

/* 【标题行纵向位置】默认 12。
   作用：整体移动“left set / time, right set / time”这一行与上边缘的距离 */
void weightset_set_title_y(lv_coord_t y);

/* 【左侧五行的上下基线】默认 top=64, bottom=360。
   作用：决定左侧五行的“第一行基线 Y”和“第五行基线 Y”，两者等分得出 5 行位置；
         只影响左侧（right 不受影响）。 */
void weightset_set_rows_top_y_left(lv_coord_t y_top);
void weightset_set_rows_bottom_y_left(lv_coord_t y_bottom);

/* 【右侧五行的上下基线】默认与左侧相同：top=64, bottom=360。
   作用：同上，但只影响右侧，便于“左右行距分开设计”。 */
void weightset_set_rows_top_y_right(lv_coord_t y_top);
void weightset_set_rows_bottom_y_right(lv_coord_t y_bottom);

/* 【左侧三列的横向位置】默认 idx=32, set=76, time=242。
   作用：控制左侧“序号/重量/时间”三列的 X 坐标(像素)。 */
void weightset_set_left_cols(lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x);

/* 【右侧三列的横向位置】默认 idx=420, set=464, time=630。
   作用：控制右侧“序号/重量/时间”三列的 X 坐标(像素)。 */
void weightset_set_right_cols(lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x);

/* 【数据按钮尺寸】默认 150×46。
   作用：控制四个数据按钮(左/右的“set/time”)的宽/高；“行看起来的高度”主要由此决定。 */
void weightset_set_cell_size(lv_coord_t w, lv_coord_t h);

#ifdef __cplusplus
}
#endif
#endif /* WEIGHTSET_H */
