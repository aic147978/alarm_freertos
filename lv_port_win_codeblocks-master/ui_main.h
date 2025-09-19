#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "lvgl/lvgl.h"

/**
 * @brief 创建应用的主界面，包括顶部内容区和底部导航栏。
 *
 * 该函数只会在第一次调用时创建承载页面内容的中心容器，并
 * 搭建底部导航按钮和 Home 按钮；之后的调用会复用已存在的
 * 中心容器，仅刷新底部控件。
 */
void ui_main_create(void);

#endif /* UI_MAIN_H */
