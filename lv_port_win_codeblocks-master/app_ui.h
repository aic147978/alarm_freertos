#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 子界面 ID */
typedef enum {
    VIEW_1 = 0,
    VIEW_2,
    VIEW_3,
    VIEW_4,
    VIEW_5,
} view_id_t;

/* 创建整套 UI（上电调用，默认显示子界面3） */
void app_ui_create(void);

/* 切换到指定子界面（会自动销毁其他子界面） */
void app_ui_show(view_id_t id);

/* 便捷接口：直接显示某个子界面 */
static inline void app_ui_show_view1(void) { app_ui_show(VIEW_1); }
static inline void app_ui_show_view2(void) { app_ui_show(VIEW_2); }
static inline void app_ui_show_view3(void) { app_ui_show(VIEW_3); }
static inline void app_ui_show_view4(void) { app_ui_show(VIEW_4); }
static inline void app_ui_show_view5(void) { app_ui_show(VIEW_5); }

/* 查询/获取 */
view_id_t app_ui_current_view(void);  /* 当前处于哪个子界面 */
lv_obj_t* app_ui_content(void);       /* 顶部内容区容器，可往里添加控件 */
lv_obj_t* app_ui_navbar(void);        /* 底部导航栏对象 */

#ifdef __cplusplus
}
#endif
#endif /* APP_UI_H */
