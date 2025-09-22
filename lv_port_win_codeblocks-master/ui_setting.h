#ifndef UI_SETTING_H
#define UI_SETTING_H

#include "lvgl/lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* ====== 校准页的数据模型 ====== */
typedef struct {
    uint8_t point_count;      /* 校准点个数(1..4) */
    int     max_weight_kg;    /* 最大重量 */
    int     zero_drift_kg;    /* 零点漂移 */
    uint8_t ck_type;          /* 0 hens / 1 cocks */
    int     setpoint_kg[4];   /* 每个校准点目标重量 */
} CalibConfig;

/* ====== 设置页主入口 ====== */
void settings_ui_create(lv_obj_t *parent);
void settings_ui_destroy(void);

/* 切页（菜单/校准/线数/手动/报警/出厂/信息） */
void settings_ui_show_menu(void);
void settings_ui_show_calibrate(void);
void settings_ui_show_line_setting(void);
void settings_ui_show_manual(void);
void settings_ui_show_alarm_setting(void);
void settings_ui_show_factory_reset(void);
void settings_ui_show_machine_info(void);

/* ====== 校准页对外（保持不变） ====== */
void settings_calib_set_points(uint8_t n);
void settings_calib_set_max_kg(int kg);
void settings_calib_set_zero_kg(int kg);
void settings_calib_set_ck_type(uint8_t type01);
void settings_calib_set_setpoint(uint8_t idx, int kg);
const CalibConfig* settings_calib_get(void);

/* 业务回调（弱符号，可在别处实现） */
void settings_on_calibrate_start(uint8_t point_idx, int target_kg);

/* ====== 手动页：把“自动模式”的设定同步为本页初值（可选调用） ======
 * left_w / left_t / right_w / right_t 传入你“自动模式”的数组地址（见 .c 文件里的【填写处】说明）
 * n_left/n_right 是数组有效长度（= 每条线的阀门数量）
 */
void settings_manual_sync_from_auto(const int *left_w, const int *left_t,
                                    const int *right_w, const int *right_t,
                                    int n_left, int n_right);

/* 手动页业务回调（弱符号，可在别处实现）
 * manual_on_start(resume, line, valve_idx, kg, time_s)
 *   resume=true 表示从 start 变为运行状态或从 suspend 恢复
 *   line: 0=上行(左线), 1=下行(右线)
 */
void manual_on_start(bool resume, uint8_t line, uint8_t valve_idx, int kg, int time_s);
void manual_on_stop(void);

#endif /* UI_SETTING_H */
