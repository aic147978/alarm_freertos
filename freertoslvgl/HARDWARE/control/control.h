#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 每侧通道数（若以后变更，只需改这里） */
#define CTRL_SIDE_CH_MAX   7

/* 运行模式 */
typedef enum {
    CTRL_MODE_MANUAL = 0,
    CTRL_MODE_AUTO   = 1
} ctrl_mode_t;

/* 阀门状态机 */
typedef enum {
    CTRL_VALVE_IDLE = 0,
    CTRL_VALVE_BULK_RUN,
    CTRL_VALVE_FINE_RUN,
    CTRL_VALVE_DONE
} ctrl_valve_state_t;

/* ========== 生命周期 / 基本控制 ========== */

/* 初始化全部状态（上电/复位时调用） */
void ctrl_init(void);

/* 扫描一拍（建议 10~20ms 周期调用） */
void ctrl_scan_cycle(void);

/* 急停：置停并清所有输出/状态 */
void ctrl_emerg_stop(void);

/* 设置当前运行模式（自动/手动） */
void ctrl_set_mode(ctrl_mode_t m);
ctrl_mode_t ctrl_get_mode(void);

/* 更新称斗实时重量（单位：g）——在你的采样处调用 */
void ctrl_set_cur_weight_g(int32_t g);

/* 料斗最大容量（单位：kg），用于限制单批目标 */
void ctrl_set_hopper_max_kg(float kg);
int32_t ctrl_get_hopper_max_g(void);

/* 外部急停量输入（true=急停） */
void ctrl_set_estop(bool en);
bool ctrl_get_estop(void);

/* ========== 行启用与目标重量（UI → 控制） ========== */

/* 设置左右是否启用及启用数量（0..7） */
void ctrl_apply_lineset(bool left_enable,  uint16_t left_count,
                        bool right_enable, uint16_t right_count);

/* 设置目标重量（单位：kg；row:0..6），内部以 g 保存 */
void ctrl_set_target_weight_left (uint16_t row, float kg);
void ctrl_set_target_weight_right(uint16_t row, float kg);

/* 读取目标重量（g） */
int32_t ctrl_get_target_weight_left_g (uint16_t row);
int32_t ctrl_get_target_weight_right_g(uint16_t row);

/* ========== 手动控制（UI2_3 → 控制） ========== */

/* 手动开/关某一路阀门（left=true 表示左侧，idx:0..6） */
void ctrl_manual_set_valve(bool left, uint16_t idx, bool open);

/* 手动控制外围执行件 */
void ctrl_manual_set_feeder(bool on);             /* 进料机 Ele_in */
void ctrl_manual_set_screw(bool on);              /* 卸料/螺旋 Y_in */
void ctrl_manual_set_distributor_left (bool on);  /* 分料机左 Ele_L */
void ctrl_manual_set_distributor_right(bool on);  /* 分料机右 Ele_R */

/* ========== 运行状态（控制 → UI，可选读取） ========== */

/* 当前活动侧与索引：side = -1（无）/0（左）/1（右），idx=-1..6 */
int  ctrl_get_active_side(void);
int  ctrl_get_active_index(void);

/* 读取每路的已下料/剩余（g）与状态机 */
int32_t              ctrl_get_delivered_g(bool left, uint16_t idx);
int32_t              ctrl_get_remain_g   (bool left, uint16_t idx);
ctrl_valve_state_t   ctrl_get_state      (bool left, uint16_t idx);

/* 外围执行件状态（手动/自动都可能改变这些量） */
bool ctrl_get_feeder(void);           /* Ele_in */
bool ctrl_get_screw(void);            /* Y_in   */
bool ctrl_get_distributor_left(void); /* Ele_L  */
bool ctrl_get_distributor_right(void);/* Ele_R  */

#ifdef __cplusplus
}
#endif
#endif /* CONTROL_H */
