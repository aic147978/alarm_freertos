#ifndef UI_SETTING_H
#define UI_SETTING_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* ---------- 设备参数：校准相关 ---------- */
typedef struct {
    uint8_t point_count;          /* 校准点个数：1~4 */
    int     max_weight_kg;        /* 称斗最大重量（kg，整数） */
    int     zero_drift_kg;        /* 零点漂移（kg，整数，允许为 0/正负均可按需扩展） */
    uint8_t ck_type;              /* 0=hens, 1=cocks */
    int     setpoint_kg[4];       /* 各校准点目标重量（kg） */
} CalibConfig;

/* ========== 界面生命周期 / 导航 ========== */
/* parent 建议传你的 g_center_container（800x400），本模块内部会禁滚动、固定布局 */
void settings_ui_create(lv_obj_t *parent);
void settings_ui_destroy(void);

/* 显示主菜单（六宫格）/ 显示“校准设置”页（你也可以点主菜单按钮进去） */
void settings_ui_show_menu(void);
void settings_ui_show_calibrate(void);

/* 其它 5 个子页（当前为占位页，能进入/返回，可按需替换实现） */
void settings_ui_show_line_setting(void);
void settings_ui_show_manual(void);
void settings_ui_show_alarm_setting(void);
void settings_ui_show_factory_reset(void);
void settings_ui_show_machine_info(void);

/* ========== 数据模型：写入与读取（写入会自动刷新已打开的对应控件文本） ========== */
void settings_calib_set_points(uint8_t n);              /* 1~4，动态增减右侧校准行 */
void settings_calib_set_max_kg(int kg);
void settings_calib_set_zero_kg(int kg);
void settings_calib_set_ck_type(uint8_t type01);        /* 0 hens, 1 cocks */
void settings_calib_set_setpoint(uint8_t idx, int kg);  /* idx: 0..3 */

const CalibConfig* settings_calib_get(void);

/* ========== 业务钩子 ==========
   点击“pointX”去执行实际校准时会回调这里（弱符号，留给你接设备） */
__attribute__((weak)) void settings_on_calibrate_start(uint8_t point_idx, int target_kg);
/* 校准完成（成功/失败）后你可在别处手动调用这两个，当前页会弹一个轻提示 */
void settings_calib_notify_ok(uint8_t point_idx);
void settings_calib_notify_fail(uint8_t point_idx, const char *reason);

#endif /* UI_SETTING_H */
