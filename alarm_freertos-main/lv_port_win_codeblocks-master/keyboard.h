#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OK 回调：text 为当前输入的字符串（以 '\0' 结尾）；value 为按 C 标准库 strtof() 解析的浮点值
   user_data 为你 show 时传入的指针；在 1_2 时间页可以忽略 value，只用 text 做自定义解析 */
typedef void (*keyboard_done_cb_t)(const char *text, float value, void *user_data);

/* 打开键盘（允许 0-9 和 '.' 小数点） */
void keyboard_show_with_cb(keyboard_done_cb_t cb, void *user_data, const char *initial_text);

/* 打开键盘（仅允许 0-9；不允许 '.'，适合 24 小时制输入） */
void keyboard_show_digits_with_cb(keyboard_done_cb_t cb, void *user_data, const char *initial_text);

/* 关闭键盘（不销毁对象，仅隐藏） */
void keyboard_hide(void);

/* 是否正在显示 */
bool keyboard_is_visible(void);

/* 设置最大输入长度（默认 12，最大不超过内部缓冲 31） */
void keyboard_set_max_len(uint8_t max_len);

/* （可选）设置标题文本（默认 "INPUT"） */
void keyboard_set_title(const char *title);

/* （可选）设置面板大小（默认 360×260），会居中显示 */
void keyboard_set_panel_size(lv_coord_t w, lv_coord_t h);

#ifdef __cplusplus
}
#endif
#endif /* KEYBOARD_H */
