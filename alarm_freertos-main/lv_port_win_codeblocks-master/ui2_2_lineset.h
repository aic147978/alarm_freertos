#ifndef UI2_2_LINESET_H
#define UI2_2_LINESET_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 右下角“退出”回调（回到 ui2_0） */
typedef void (*ui22_exit_cb_t)(void *user_data);

/* 当任一参数变化时回调（可用于外部联动 ui1 的行数） */
typedef void (*ui22_apply_cb_t)(bool left_en, uint16_t left_cnt,
                                bool right_en, uint16_t right_cnt,
                                void *user_data);

/* 绑定外部数字键盘（与 ui2_1 一致的签名）：
 * anchor   : 触发按钮，用于定位
 * title    : 弹窗标题
 * init_val : 初始值
 * decimals : 小数位数（这里传 0）
 * on_ok    : 用户确认后回调；v 为最终值
 * on_ud    : 透传用户数据
 */
typedef void (*ui22_open_kb_fn)(lv_obj_t *anchor,
                                const char *title,
                                float init_val,
                                int decimals,
                                void (*on_ok)(float v, void *ud),
                                void *on_ud);

/* ===== 生命周期 ===== */
void ui22_lineset_create(lv_obj_t *parent_800x400);
void ui22_lineset_destroy(void);
lv_obj_t* ui22_root(void);

/* ===== 回调绑定 ===== */
void ui22_set_exit_cb(ui22_exit_cb_t cb, void *user_data);
void ui22_set_apply_cb(ui22_apply_cb_t cb, void *user_data);
void ui22_bind_number_keyboard(ui22_open_kb_fn fn);

/* ===== 参数存取 ===== */
void     ui22_set_left_enabled(bool en);
bool     ui22_get_left_enabled(void);
void     ui22_set_right_enabled(bool en);
bool     ui22_get_right_enabled(void);

void     ui22_set_left_count(uint16_t n);
uint16_t ui22_get_left_count(void);
void     ui22_set_right_count(uint16_t n);
uint16_t ui22_get_right_count(void);

/* 限制最大数（默认 50，用于 clamp） */
void     ui22_set_max_limit(uint16_t max_n);

/* ===== 布局（像素，参考 800×400 左上角） ===== */
void ui22_layout_reset_default(void);                 /* 恢复默认布局 */
void ui22_set_titles_y(int y);                        /* 顶部“left line / right line”文字 Y（默认 56） */
void ui22_set_left_title_x(int x_label, int x_cb);    /* 左标题文本/复选框 X（默认 112, 232） */
void ui22_set_right_title_x(int x_label, int x_cb);   /* 右标题文本/复选框 X（默认 560, 740） */
void ui22_set_count_btn_pos(int left_cx, int right_cx, int cy);  /* 左右数字按钮中心位置（默认 200/600, y=210） */
void ui22_set_exit_btn_pos_size(int cx, int cy, int w, int h);   /* 右下“exit”按钮（默认 690,332, 190×60） */

#ifdef __cplusplus
}
#endif
#endif /* UI2_2_LINESET_H */
