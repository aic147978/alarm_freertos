#ifndef UI_LEFT1_H
#define UI_LEFT1_H

#include "lvgl/lvgl.h"

/**
 * @brief 构建“左一”功能页面。
 *
 * @param center_container 主界面提供的内容容器，页面元素会被添加到其中。
 */
void ui_left1_create(lv_obj_t *center_container);
void ui_left1_set_valve_count(int n);

/**
 * @brief 清理“左一”页面产生的附加控件。
 *
 * 在页面切换前调用，用于关闭九键键盘、删除悬浮的切换按钮等。
 */
void ui_left1_cleanup(void);

/* ===== ui_left1.h: 对外数据读取接口 ===== */

/* 有效行数（两列共用同一计数） */
int ui_left1_get_valve1_count(void);

/* —— A 面：左右列 set/time（单位：kg / s），返回内部只读指针 —— */
/* 注意：指针长度为 MAX_VALVES，有效元素个数请配合 ui_left1_get_valve1_count() 使用 */
const int* ui_left1_left_set_data(void);
const int* ui_left1_left_time_data(void);
const int* ui_left1_right_set_data(void);
const int* ui_left1_right_time_data(void);

/* —— B 面：start/end（秒）与每行启用开关 —— */
const int*  ui_left1_start_time_data(void);
const int*  ui_left1_end_time_data(void);
const bool* ui_left1_row_enable_data(void);

/* （可选）把当前页所有行拷贝到你的缓冲区里，避免直接用指针 */
int ui_left1_snapshot_pageA(int first_index, int* l_set, int* l_time,
                             int* r_set, int* r_time, int max_n);

int ui_left1_snapshot_pageB(int first_index, int* start_s, int* end_s,
                             bool* enabled, int max_n);


#endif /* UI_LEFT1_H */



