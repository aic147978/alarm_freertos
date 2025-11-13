#ifndef KEYBOARD_H
#define KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 当使用回调方式时，确定键会回调输入文本与解析后的浮点值（若为空则 value=0） */
typedef void (*kb_submit_cb_t)(const char *text, float value, void *user_data);

/* 初始化（可选）。若不显式调用，show 接口会自动创建。parent 建议传 NULL 使用顶层层级。*/
void keyboard_init(lv_obj_t *parent);

/* 显示键盘，并把“确定”的结果写入到指定的 textarea（可传入初值，为 NULL 则保持现状） */
void keyboard_show_for_ta(lv_obj_t *target_ta, const char *initial_text);

/* 显示键盘，并在“确定”时回调（可传入初值） */
void keyboard_show_with_cb(kb_submit_cb_t cb, void *user_data, const char *initial_text);

/* 隐藏键盘（不销毁，下次可直接打开） */
void keyboard_hide(void);

/* 是否打开中 */
bool keyboard_is_open(void);

/* 设置可输入的最大长度（默认 12） */
void keyboard_set_max_len(uint16_t max_len);

/* 设置标题（默认：数字键盘） */
void keyboard_set_title(const char *title);

#ifdef __cplusplus
}
#endif
#endif /* KEYBOARD_H */
