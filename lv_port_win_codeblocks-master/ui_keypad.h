#ifndef UI_KEYPAD_H
#define UI_KEYPAD_H

#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 绑定数字显示标签与实际变量的结构体。
 */
typedef struct {
    lv_obj_t   *label;      /**< 显示数值的标签对象。 */
    int        *var;        /**< 绑定的整数变量地址。 */
    const char *suffix;     /**< 数值后缀，例如 "kg" 或 "s"。 */
    bool        is_time;    /**< 是否按 mm:ss 的时间格式展示。 */
    uint8_t     max_digits; /**< 允许输入的最大数字个数。 */
} ui_bind_entry_t;

/**
 * @brief 初始化一个绑定条目，并根据变量当前值刷新显示。
 */
void ui_keypad_entry_init(ui_bind_entry_t *entry,
                          lv_obj_t *label,
                          int *var,
                          const char *suffix,
                          bool is_time,
                          uint8_t max_digits);

/**
 * @brief 根据变量值刷新绑定条目的标签文本。
 */
void ui_keypad_refresh_entry(ui_bind_entry_t *entry);

/**
 * @brief 为按钮绑定九键输入事件。
 */
void ui_keypad_bind_button(lv_obj_t *btn, ui_bind_entry_t *entry);

/**
 * @brief 主动关闭九键键盘弹窗。
 */
void ui_keypad_close(void);

#endif /* UI_KEYPAD_H */
