#include "control_bridge.h"
#include "lvgl.h"
#include <string.h>

/* ===== 内部缓存：lineset 的可见配置（用于快照里直接返回） ===== */
static bool     s_left_en  = true;
static bool     s_right_en = true;
static uint16_t s_left_cnt  = CTRL_SIDE_CH_MAX;   /* 0..7 */
static uint16_t s_right_cnt = CTRL_SIDE_CH_MAX;

/* ===== lv_timer 驱动 ===== */
static lv_timer_t         *s_timer   = NULL;
static ctrlb_ui_tick_cb_t  s_cb      = NULL;
static void               *s_cb_user = NULL;

/* ===== 把底层状态拷到快照 ===== */
static void fill_snapshot(ctrlb_snapshot_t *o)
{
    memset(o, 0, sizeof(*o));

    o->mode         = ctrl_get_mode();
    o->estop        = ctrl_get_estop();
    o->active_side  = ctrl_get_active_side();
    o->active_index = ctrl_get_active_index();
    o->hopper_max_g = ctrl_get_hopper_max_g();

    /* 实时重量：底层只提供 set，不提供 get；由上层自己缓存或在此保存上次 set 的值。
       简单方案：快照里暂不返回“读回”，默认 0；若你想显示这个值，请在调用 ctrlb_set_cur_weight_g 时顺便缓存。*/
    /* 这里示例做一个本地缓存： */
    static int32_t s_last_weight_g = 0;
    (void)s_last_weight_g;
    o->cur_weight_g = s_last_weight_g;

    /* lineset 的可见配置（来自桥接层最近一次设置） */
    o->left_enable  = s_left_en;
    o->right_enable = s_right_en;
    o->left_count   = s_left_cnt;
    o->right_count  = s_right_cnt;

    /* 逐路读取底层 */
    for(uint16_t i=0; i<CTRL_SIDE_CH_MAX; ++i){
        o->target_L[i]    = ctrl_get_target_weight_left_g(i);
        o->target_R[i]    = ctrl_get_target_weight_right_g(i);
        o->delivered_L[i] = ctrl_get_delivered_g(true,  i);
        o->delivered_R[i] = ctrl_get_delivered_g(false, i);
        o->remain_L[i]    = ctrl_get_remain_g(true,  i);
        o->remain_R[i]    = ctrl_get_remain_g(false, i);
        o->state_L[i]     = ctrl_get_state(true,  i);
        o->state_R[i]     = ctrl_get_state(false, i);
    }

    o->feeder        = ctrl_get_feeder();
    o->screw         = ctrl_get_screw();
    o->distributor_L = ctrl_get_distributor_left();
    o->distributor_R = ctrl_get_distributor_right();
}

/* ===== 定时器回调：跑一拍状态机 → 推快照给 UI ===== */
static void timer_cb(lv_timer_t *t)
{
    (void)t;

    /* 跑一拍控制主循环 */
    ctrl_scan_cycle();

    /* 推送快照 */
    if(s_cb){
        ctrlb_snapshot_t s;
        fill_snapshot(&s);
        s_cb(&s, s_cb_user);
    }
}

/* ====== 对外实现 ====== */

void ctrlb_init(void)
{
    ctrl_init();
    s_left_en  = true;  s_right_en = true;
    s_left_cnt = CTRL_SIDE_CH_MAX; s_right_cnt = CTRL_SIDE_CH_MAX;

    if(s_timer){ lv_timer_del(s_timer); s_timer = NULL; }
    s_cb = NULL; s_cb_user = NULL;
}

void ctrlb_attach_timer_lvgl(uint32_t period_ms)
{
    if(s_timer) lv_timer_del(s_timer);
    if(period_ms == 0) period_ms = 20;   /* 默认 20ms */
    s_timer = lv_timer_create(timer_cb, period_ms, NULL);
}

void ctrlb_detach_timer_lvgl(void)
{
    if(s_timer){ lv_timer_del(s_timer); s_timer = NULL; }
}

void ctrlb_set_ui_tick_cb(ctrlb_ui_tick_cb_t cb, void *user)
{
    s_cb = cb; s_cb_user = user;
}

void ctrlb_make_snapshot(ctrlb_snapshot_t *out)
{
    if(!out) return;
    fill_snapshot(out);
}

void ctrlb_set_mode_auto(bool auto_on)
{
    ctrl_set_mode(auto_on ? CTRL_MODE_AUTO : CTRL_MODE_MANUAL);
}

void ctrlb_apply_lineset(bool left_enable, uint16_t left_count,
                         bool right_enable, uint16_t right_count)
{
    s_left_en  = left_enable;
    s_right_en = right_enable;
    s_left_cnt  = (left_count  > CTRL_SIDE_CH_MAX) ? CTRL_SIDE_CH_MAX : left_count;
    s_right_cnt = (right_count > CTRL_SIDE_CH_MAX) ? CTRL_SIDE_CH_MAX : right_count;

    ctrl_apply_lineset(s_left_en, s_left_cnt, s_right_en, s_right_cnt);
}

void ctrlb_set_target_left(uint16_t row, float kg)
{
    ctrl_set_target_weight_left(row, kg);
}
void ctrlb_set_target_right(uint16_t row, float kg)
{
    ctrl_set_target_weight_right(row, kg);
}

void ctrlb_set_hopper_max_kg(float kg)
{
    ctrl_set_hopper_max_kg(kg);
}

void ctrlb_set_cur_weight_g(int32_t g)
{
    ctrl_set_cur_weight_g(g);

    /* 可选：顺便把“最近一次写入的重量”缓存到快照里返回，便于 UI 显示 */
    static int32_t s_last_weight_g = 0;
    s_last_weight_g = g;
}

void ctrlb_manual_set_valve(bool left, uint16_t idx, bool open)
{
    ctrl_manual_set_valve(left, idx, open);
}

void ctrlb_manual_set_feeder(bool on)            { ctrl_manual_set_feeder(on); }
void ctrlb_manual_set_screw(bool on)             { ctrl_manual_set_screw(on); }
void ctrlb_manual_set_distributor_left (bool on) { ctrl_manual_set_distributor_left(on); }
void ctrlb_manual_set_distributor_right(bool on) { ctrl_manual_set_distributor_right(on); }

void ctrlb_emergency_stop(void)
{
    ctrl_emerg_stop();
}
