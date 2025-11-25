#ifndef CONTROL_BRIDGE_H
#define CONTROL_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "control.h"   /* 依赖你刚换好的左右分离版 control.h */

#ifdef __cplusplus
extern "C" {
#endif

/* ========== UI 可订阅的运行快照 ========== */
typedef struct {
    ctrl_mode_t mode;                /* 当前模式：自动/手动 */
    bool        estop;               /* 急停输入 */
    int         active_side;         /* -1 无 / 0 左 / 1 右 */
    int         active_index;        /* -1..6 */
    int32_t     cur_weight_g;        /* 称斗即时重量(g) */
    int32_t     hopper_max_g;        /* 料斗上限(g) */

    /* 左右行配置（来自近期一次 lineset 设置） */
    bool        left_enable;
    bool        right_enable;
    uint16_t    left_count;          /* 0..7 */
    uint16_t    right_count;         /* 0..7 */

    /* 目标/已下/剩余 与 状态机（单位 g） */
    int32_t           target_L  [CTRL_SIDE_CH_MAX];
    int32_t           target_R  [CTRL_SIDE_CH_MAX];
    int32_t           delivered_L[CTRL_SIDE_CH_MAX];
    int32_t           delivered_R[CTRL_SIDE_CH_MAX];
    int32_t           remain_L   [CTRL_SIDE_CH_MAX];
    int32_t           remain_R   [CTRL_SIDE_CH_MAX];
    ctrl_valve_state_t state_L   [CTRL_SIDE_CH_MAX];
    ctrl_valve_state_t state_R   [CTRL_SIDE_CH_MAX];

    /* 外围执行件状态 */
    bool feeder;                     /* 进料 Ele_in */
    bool screw;                      /* 卸料/螺旋 Y_in */
    bool distributor_L;              /* 分料左 Ele_L */
    bool distributor_R;              /* 分料右 Ele_R */
} ctrlb_snapshot_t;

/* UI 订阅快照回调 */
typedef void (*ctrlb_ui_tick_cb_t)(const ctrlb_snapshot_t *snap, void *user);

/* ========== 桥接层：生命周期/定时器 ========== */
void ctrlb_init(void);                          /* 调用 ctrl_init() 并清内部缓存 */
void ctrlb_attach_timer_lvgl(uint32_t period_ms);/* 用 LVGL lv_timer 周期驱动 scan_cycle（默认 20ms） */
void ctrlb_detach_timer_lvgl(void);             /* 删除定时器（可选） */

/* 订阅/退订运行快照（每拍调用一次） */
void ctrlb_set_ui_tick_cb(ctrlb_ui_tick_cb_t cb, void *user);

/* 立即抓取一份快照（不等定时器） */
void ctrlb_make_snapshot(ctrlb_snapshot_t *out);

/* ========== UI → 控制：设置/写入接口（照“之前模板”全给齐） ========== */

/* 模式：true=自动 / false=手动 */
void ctrlb_set_mode_auto(bool auto_on);

/* 行启用与数量（0..7），会调用底层 ctrl_apply_lineset，并缓存给快照 */
void ctrlb_apply_lineset(bool left_enable, uint16_t left_count,
                         bool right_enable, uint16_t right_count);

/* 目标重量（kg；row:0..6），内部自动 kg→g */
void ctrlb_set_target_left (uint16_t row, float kg);
void ctrlb_set_target_right(uint16_t row, float kg);

/* 料斗最大容量（kg） */
void ctrlb_set_hopper_max_kg(float kg);

/* 实时重量（g）：在你的采样处调用这个写入 */
void ctrlb_set_cur_weight_g(int32_t g);

/* 手动控制（2_3 页面用） */
void ctrlb_manual_set_valve(bool left, uint16_t idx, bool open);
void ctrlb_manual_set_feeder(bool on);
void ctrlb_manual_set_screw(bool on);
void ctrlb_manual_set_distributor_left (bool on);
void ctrlb_manual_set_distributor_right(bool on);

/* 急停/复位 */
void ctrlb_emergency_stop(void);

#ifdef __cplusplus
}
#endif
#endif /* CONTROL_BRIDGE_H */
