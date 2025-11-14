#ifndef UI2_3_MANUAL_H
#define UI2_3_MANUAL_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 侧别（左/右） */
typedef enum {
    UI23_SIDE_NONE = 0,
    UI23_SIDE_LEFT = 1,
    UI23_SIDE_RIGHT = 2,
} ui23_side_t;

/* 回调：进入手动模式（创建时立刻回调，用于上层停自动） */
typedef void (*ui23_enter_manual_cb_t)(void *user_data);

/* 回调：Open/Close 按钮 */
typedef void (*ui23_open_cb_t)(ui23_side_t side, uint16_t index /*0..n-1*/, void *user_data);
typedef void (*ui23_close_cb_t)(ui23_side_t side, uint16_t index /*0..n-1*/, void *user_data);

/* 回调：退出页面（回到 ui2_0） */
typedef void (*ui23_exit_cb_t)(void *user_data);

/* 行为：联动 lineset（来自 ui2_2 的设置） */
void ui23_apply_lineset(bool left_enable,  uint16_t left_count,
                        bool right_enable, uint16_t right_count);

/* 生命周期 */
void      ui23_manual_create(lv_obj_t *parent_800x400);
void      ui23_manual_destroy(void);
lv_obj_t* ui23_manual_root(void);

/* 回调绑定 */
void ui23_set_enter_manual_cb(ui23_enter_manual_cb_t cb, void *ud);
void ui23_set_open_cb (ui23_open_cb_t  cb, void *ud);
void ui23_set_close_cb(ui23_close_cb_t cb, void *ud);
void ui23_set_exit_cb (ui23_exit_cb_t  cb, void *ud);

/* 选择状态（仅允许一个被选中） */
void         ui23_set_selected(ui23_side_t side, uint16_t index);
ui23_side_t  ui23_get_selected_side(void);
uint16_t     ui23_get_selected_index(void);

/* 继电器开关状态（仅表示当前选中的那个是否处于“打开”状态，便于 UI 显示） */
void ui23_set_relay_on(bool on);
bool ui23_is_relay_on(void);

/* 与 1_1 的行数据对应：为每个点设置/读取重量(kg)与时间(s) */
void  ui23_set_point_info(ui23_side_t side, uint16_t index, float weight_kg, float time_sec);
void  ui23_get_point_info(ui23_side_t side, uint16_t index, float *out_weight_kg, float *out_time_sec);

/* 已下料重量显示（累计值） */
void  ui23_set_dispensed_kg(float kg);
float ui23_get_dispensed_kg(void);

/* 可选：调布局（像素）——若不需要可不调用 */
void ui23_layout_reset_default(void);
void ui23_layout_set_rows_y(int top_row_y, int bottom_row_y);
void ui23_layout_set_status_y(int info_y, int disp_y);
void ui23_layout_set_buttons(int open_cx, int close_cx, int exit_cx, int btn_y, int btn_w, int btn_h);

#ifdef __cplusplus
}
#endif
#endif /* UI2_3_MANUAL_H */
