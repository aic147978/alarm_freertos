#include "myGUI.h"
#include "app_ui.h"   /* 我们之前给你的 app_ui.h：app_ui_create/destroy/show */

#include <stdbool.h>

static bool s_created = false;

void myGUI_create(void)
{
    if (s_created) return;
lv_obj_t *scr = lv_scr_act();
lv_obj_remove_style_all(scr);
lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, 0);
lv_obj_set_style_pad_all(scr, 0, 0);
lv_obj_set_style_pad_row(scr, 0, 0);
lv_obj_set_style_pad_column(scr, 0, 0);
lv_obj_set_style_border_width(scr, 0, 0);
lv_obj_set_style_outline_width(scr, 0, 0);
lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_set_scroll_dir(scr, LV_DIR_NONE);
lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    /* 把 UI 创建到当前活动屏幕上 */
    app_ui_create(lv_scr_act());
    s_created = true;

    /* 可选：上电后直接进入 Home(3)，当前 app_ui 已默认就是 3，这里不用再调
       如果你想一启动就到子界面1(1_1)，可取消注释下面一行：
       app_ui_show(VIEW_1);
    */
}

void myGUI_destroy(void)
{
    if (!s_created) return;
    app_ui_destroy();
    s_created = false;
}

void myGUI_show(uint8_t id)
{
    if (!s_created) {
        /* 防呆：若还没创建，先创建 */
        myGUI_create();
    }

    switch (id) {
        case 1: app_ui_show(VIEW_1); break;
        case 2: app_ui_show(VIEW_2); break;
        case 3: app_ui_show(VIEW_3); break;  /* Home */
        case 4: app_ui_show(VIEW_4); break;
        case 5: app_ui_show(VIEW_5); break;
        default: /* 忽略非法 id */ break;
    }
}
