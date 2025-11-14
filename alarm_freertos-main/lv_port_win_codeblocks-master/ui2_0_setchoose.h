#ifndef UI2_0_SETCHOOSE_H
#define UI2_0_SETCHOOSE_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 选择回调：idx 取值 1..6（从左到右、从上到下）*/
typedef void (*ui20_select_cb_t)(uint8_t idx, void *user_data);

/* 生命周期 */
void      ui20_create(lv_obj_t *parent_800x400);
void      ui20_destroy(void);
lv_obj_t* ui20_root(void);

/* 回调与外观 */
void ui20_set_select_cb(ui20_select_cb_t cb, void *user_data);
void ui20_set_button_text(uint8_t idx /*1..6*/, const char *txt);
void ui20_set_button_enabled(uint8_t idx /*1..6*/, bool en);

/* 简单布局（像素，相对 800×400 左上角） */
void ui20_layout_reset_default(void);                   /* 恢复默认：三列中心 X、两行中心 Y、按钮 180×60 */
void ui20_set_cols(int cx1, int cx2, int cx3);          /* 三列中心 X */
void ui20_set_rows(int cy1, int cy2);                   /* 两行中心 Y */
void ui20_set_button_size(int w, int h);                /* 六个按钮统一宽高 */

#ifdef __cplusplus
}
#endif
#endif /* UI2_0_SETCHOOSE_H */
