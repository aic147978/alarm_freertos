#ifndef UI1_1_WEIGHTSET_H
#define UI1_1_WEIGHTSET_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“切换到 1_2”回调（点击右下角按钮时触发） */
typedef void (*ui11_weightset_toggle_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
/* 在父容器(宽800, 高400)中创建 1_1 页面；使用绝对像素定位、禁滚动 */
void ui11_weightset_create(lv_obj_t *parent_800x400);
/* 删除 1_1 的 UI（静态数据保留；再次 create 仍保留上次的数值） */
void ui11_weightset_destroy(void);

/* ===== 行数 / 分页（固定 5 行/页） ===== */
void     ui11_weightset_set_row_count(uint16_t rows);  /* 1..64 */
uint16_t ui11_weightset_get_row_count(void);
uint16_t ui11_weightset_get_page(void);                /* 0-based */
uint16_t ui11_weightset_get_page_count(void);
void     ui11_weightset_set_page(uint16_t page);
void     ui11_weightset_next_page(void);
void     ui11_weightset_prev_page(void);

/* ===== 数据访问（行号 0 起）===== */
float ui11_weightset_get_left_weight (uint16_t row);
float ui11_weightset_get_left_time   (uint16_t row);   /* 秒 */
float ui11_weightset_get_right_weight(uint16_t row);
float ui11_weightset_get_right_time  (uint16_t row);

void  ui11_weightset_set_left_weight (uint16_t row, float kg);
void  ui11_weightset_set_left_time   (uint16_t row, float sec);
void  ui11_weightset_set_right_weight(uint16_t row, float kg);
void  ui11_weightset_set_right_time  (uint16_t row, float sec);

/* 一键清零（仅清数据，不动 UI） */
void  ui11_weightset_clear_all(void);

/* ===== 其他 ===== */
void      ui11_weightset_set_toggle_cb(ui11_weightset_toggle_cb_t cb, void *user_data); /* 右下角“切换 1_2” */
lv_obj_t* ui11_weightset_root(void);                                                     /* 取根对象，便于外部做样式 */

/* =====（可选）简单布局调节：像素，相对父容器(0,0) =====
   若不需要可忽略；提供少量常用调整接口 —— 标题位置、行距上下边界、列 X、按钮尺寸、翻页键与切换键位置 */
void ui11_weightset_set_title_y(lv_coord_t y);
void ui11_weightset_set_rows_top_bottom_left (lv_coord_t y_top, lv_coord_t y_bottom);
void ui11_weightset_set_rows_top_bottom_right(lv_coord_t y_top, lv_coord_t y_bottom);
void ui11_weightset_set_left_cols (lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x);
void ui11_weightset_set_right_cols(lv_coord_t idx_x, lv_coord_t set_x, lv_coord_t time_x);
void ui11_weightset_set_cell_size(lv_coord_t w, lv_coord_t h);
void ui11_weightset_set_pager_pos(int up_cx, int up_cy, int dn_cx, int dn_cy, int diameter);
void ui11_weightset_set_switch_pos_size(int cx, int cy, int w, int h);

#ifdef __cplusplus
}
#endif
#endif /* UI1_1_WEIGHTSET_H */
