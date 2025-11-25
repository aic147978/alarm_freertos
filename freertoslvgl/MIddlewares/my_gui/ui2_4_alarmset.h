#ifndef UI2_4_ALARMSET_H
#define UI2_4_ALARMSET_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 退出回调：点击右下角“exit”时触发 */
typedef void (*ui24_exit_cb_t)(void *user_data);

/* ===== 生命周期 ===== */
/* 在父容器(宽800, 高400)中创建 2_4 页面；绝对定位、禁滚动 */
void ui24_alarmset_create(lv_obj_t *parent_800x400);
/* 删除 2_4 UI（数据保留在静态区） */
void ui24_alarmset_destroy(void);

/* 根对象（如需附加样式/调试） */
lv_obj_t* ui24_root(void);

/* 设置退出回调与 user_data */
void ui24_set_exit_cb(ui24_exit_cb_t cb, void *user_data);

/* ===== 参数存取 ===== */
/* 满料报警阈值（百分比，例：115 表示 115%） */
void     ui24_set_full_percent(uint16_t pct);
uint16_t ui24_get_full_percent(void);

/* 空料报警阈值（百分比，例：10 表示 10%） */
void     ui24_set_empty_percent(uint16_t pct);
uint16_t ui24_get_empty_percent(void);

/* 超时报警（秒） */
void     ui24_set_timeout_sec(uint32_t sec);
uint32_t ui24_get_timeout_sec(void);

/* 勾选项（共 10 个，索引 0..9；左列 0..4，右列 5..9） */
void ui24_set_check(uint8_t idx, bool enable);
bool ui24_get_check(uint8_t idx);

/* 批量设置所有勾选项（true=全选，false=全清） */
void ui24_set_check_all(bool enable);

/* ===== 布局微调（像素，以 800×400 左上角为基点）===== */
void ui24_set_titles_y(int y);                     /* 左列标题 Y（默认 28） */
void ui24_set_left_cols_x(int lab_x, int btn_x);   /* 左列标签/按钮的 X（默认 lab=58, btn=182） */
void ui24_set_rows_y(int first_y, int row_gap);    /* 左列三行起始 Y 与行距（默认 first=88, gap=66） */
void ui24_set_checks_xy(int left_x, int right_x, int top_y, int v_gap); /* 右 2 列勾选列位置（默认 420/600/88/50） */
void ui24_set_exit_btn_pos_size(int cx, int cy, int w, int h);          /* 退出按钮位置与尺寸 */

#ifdef __cplusplus
}
#endif
#endif /* UI2_4_ALARMSET_H */
