#ifndef UI_LEFT1_H
#define UI_LEFT1_H

#include "lvgl/lvgl.h"

/**
 * @brief 构建“左一”功能页面。
 *
 * @param center_container 主界面提供的内容容器，页面元素会被添加到其中。
 */
void ui_left1_create(lv_obj_t *center_container);

/**
 * @brief 清理“左一”页面产生的附加控件。
 *
 * 在页面切换前调用，用于关闭九键键盘、删除悬浮的切换按钮等。
 */
void ui_left1_cleanup(void);

#endif /* UI_LEFT1_H */
