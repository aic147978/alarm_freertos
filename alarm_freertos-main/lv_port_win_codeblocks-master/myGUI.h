#ifndef MYGUI_H
#define MYGUI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 供外部调用的简洁接口：
 * - myGUI_create()  : 在当前活动屏(lv_scr_act)上创建整套 UI（若已创建则忽略）
 * - myGUI_destroy() : 删除整套 UI（可重复创建/销毁）
 * - myGUI_show(id)  : 切换到底部的 1..5 号子界面（3 是 Home）
 * - myGUI_home()    : 快速切到 Home(3)
 *
 * 视图 ID 说明（与 app_ui.h 保持一致）:
 *   1 → 子界面1（默认进入 1_1 weightset，可在右下角按钮切到 1_2 timeset）
 *   2 → 子界面2（占位）
 *   3 → 子界面3（Home，中间圆形按钮）
 *   4 → 子界面4（占位）
 *   5 → 子界面5（占位）
 */

void myGUI_create(void);
void myGUI_destroy(void);

/* id 取值 1..5（非法值将被忽略） */
void myGUI_show(uint8_t id);

/* 等价于 myGUI_show(3) */
static inline void myGUI_home(void) { myGUI_show(3); }

#ifdef __cplusplus
}
#endif
#endif /* MYGUI_H */
