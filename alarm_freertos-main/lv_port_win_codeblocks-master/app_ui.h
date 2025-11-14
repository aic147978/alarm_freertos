#ifndef APP_UI_H
#define APP_UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 底部 5 个子界面 ID（3 为 Home） */
typedef enum {
    VIEW_NONE = 0,
    VIEW_1 = 1,   /* 子界面1：含 1_1(重量/秒) 与 1_2(24h时间)，默认进 1_1 */
    VIEW_2 = 2,
    VIEW_3 = 3,   /* Home（底部中间圆按钮） */
    VIEW_4 = 4,   /* 子界面4：含 4_1(重量历史) 与 4_2(报警历史)，默认进 4_1 */
    VIEW_5 = 5
} view_id_t;

/* 在 parent（通常为 lv_scr_act()）下创建整套 UI。重复调用会被忽略。 */
void app_ui_create(lv_obj_t *parent);

/* 销毁整套 UI（仅删 UI 对象；各子模块静态数据保留）。 */
void app_ui_destroy(void);

/* 外部切换到指定子界面（1..5）。非法值忽略。 */
void app_ui_show(view_id_t id);

#ifdef __cplusplus
}
#endif
#endif /* APP_UI_H */
