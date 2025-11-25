#ifndef UI2_1_CALIBRATE_H
#define UI2_1_CALIBRATE_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 鸡只类型 */
typedef enum {
    UI21_BIRD_HEN = 0,     /* 母鸡 */
    UI21_BIRD_ROOSTER = 1  /* 公鸡 */
} ui21_bird_t;

/* 右下角“退出”回调 */
typedef void (*ui21_exit_cb_t)(void *user_data);

/* 右列“校准 pointX”按钮的统一回调：idx 取值 0..2 对应 point1..point3 */
typedef void (*ui21_calib_cb_t)(uint8_t idx, void *user_data);

/* 绑定数字键盘的统一接口：
 *  anchor   : 触发的控件（用于居中或参考定位）
 *  title    : 提示标题（如“最大容量(kg)”）
 *  init_val : 初始值（浮点）
 *  decimals : 小数位数（0 表示整数）
 *  on_ok    : 用户确认后回调，返回最终浮点值
 *  on_ud    : 透传的用户数据
 *
 *  你可以把这里绑定到自己实现的 keyboard 模块。
 */
typedef void (*ui21_open_kb_fn)(lv_obj_t *anchor,
                                const char *title,
                                float init_val,
                                int decimals,
                                void (*on_ok)(float v, void *ud),
                                void *on_ud);

/* ========== 生命周期 ========== */
void ui21_calib_create(lv_obj_t *parent_800x400);  /* 在 800×400 容器中创建 */
void ui21_calib_destroy(void);                     /* 删除 UI（保留静态数据） */

/* ========== 绑定回调 ========== */
void ui21_set_exit_cb(ui21_exit_cb_t cb, void *user_data);
void ui21_set_calib_cb(ui21_calib_cb_t cb, void *user_data);
void ui21_bind_number_keyboard(ui21_open_kb_fn fn);   /* 绑定数字键盘 */

/* ========== 参数存取（左列） ========== */
void     ui21_set_points(uint8_t n);      /* 1..3，超出将被裁剪 */
uint8_t  ui21_get_points(void);

void     ui21_set_max_weight(float kg);
float    ui21_get_max_weight(void);

void     ui21_set_empty_weight(float kg);
float    ui21_get_empty_weight(void);

void        ui21_set_bird(ui21_bird_t t);
ui21_bird_t ui21_get_bird(void);

/* ========== 参数存取（中列：设置权重） ========== */
void  ui21_set_point_weight(uint8_t idx/*0..2*/, float kg);
float ui21_get_point_weight(uint8_t idx/*0..2*/);

/* ========== 可选：外观/布局（像素，参考 800×400 左上角） ========== */
void ui21_set_layout_defaults(void);  /* 恢复默认布局（如果改乱了） */
void ui21_set_titles_y(int y);        /* 三列标题文字的 Y（默认 22） */
void ui21_set_rows_y(int first_y, int row_gap); /* 每行起始Y与行距（默认 92, 70） */

#ifdef __cplusplus
}
#endif
#endif /* UI2_1_CALIBRATE_H */
