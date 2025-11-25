#include "control.h"
#include <string.h>   /* memset */
#include <stdlib.h>   /* abs */

/* ================= 参数与内部存储 ================= */

#define BULK_CHUNK_G      20000     /* 批次上限（大投，20kg） */
#define KG_TO_G(x)  ((int32_t)((x) * 1000.0f + 0.5f))

/* 左右启用与路数 */
static bool     s_left_en  = true;
static bool     s_right_en = true;
static uint16_t s_left_n   = CTRL_SIDE_CH_MAX;   /* 0..7 */
static uint16_t s_right_n  = CTRL_SIDE_CH_MAX;   /* 0..7 */

/* 目标/已下/剩余（单位：g） */
static int32_t s_target_L[CTRL_SIDE_CH_MAX];
static int32_t s_target_R[CTRL_SIDE_CH_MAX];
static int32_t s_deliv_L [CTRL_SIDE_CH_MAX];
static int32_t s_deliv_R [CTRL_SIDE_CH_MAX];
static int32_t s_remain_L[CTRL_SIDE_CH_MAX];
static int32_t s_remain_R[CTRL_SIDE_CH_MAX];

/* 手/自动输出与手动请求 */
static bool s_Y_L[CTRL_SIDE_CH_MAX];      /* 阀门输出 */
static bool s_Y_R[CTRL_SIDE_CH_MAX];
static bool s_req_L[CTRL_SIDE_CH_MAX];    /* 手动请求（UI2_3） */
static bool s_req_R[CTRL_SIDE_CH_MAX];

/* 阀门状态机 */
static ctrl_valve_state_t s_state_L[CTRL_SIDE_CH_MAX];
static ctrl_valve_state_t s_state_R[CTRL_SIDE_CH_MAX];

/* 全局执行件与标志 */
static bool        s_Ele_in = false;      /* 进料机 */
static bool        s_Y_in   = false;      /* 卸料/螺旋 */
static bool        s_Ele_L  = false;      /* 分料左 */
static bool        s_Ele_R  = false;      /* 分料右 */
static bool        s_Estop  = false;      /* 急停输入 */
static ctrl_mode_t s_mode   = CTRL_MODE_MANUAL;

/* 活动侧/索引（自动模式下使用） */
enum { SIDE_LEFT = 0, SIDE_RIGHT = 1, SIDE_NONE = -1 };
static int  s_active_side = SIDE_NONE;    /* -1/0/1 */
static int  s_active_idx  = -1;           /* -1..6  */
static bool s_left_done   = false;
static bool s_right_done  = false;

/* 称斗即时重量与单批目标（单位：g） */
static int32_t s_curWeight   = 0;
static int32_t s_batchTarget = 0;

/* 料斗最大容量（g） */
static int32_t s_hopper_max_g = 20000;

/* ================= 内部工具 ================= */

static inline int32_t clamp_nonneg(int32_t v){ return v < 0 ? 0 : v; }

/* 计算本批目标：不超过 BULK_CHUNK_G，且受料斗最大容量限制 */
static inline int32_t compute_batch_target(int32_t remain_g)
{
    int32_t tgt = (remain_g >= BULK_CHUNK_G) ? BULK_CHUNK_G : remain_g;
    if(tgt > s_hopper_max_g) tgt = s_hopper_max_g;
    return clamp_nonneg(tgt);
}

/* 清空所有输出/状态（急停或复位） */
static void reset_all(void)
{
    memset(s_Y_L,      0, sizeof(s_Y_L));
    memset(s_Y_R,      0, sizeof(s_Y_R));
    memset(s_deliv_L,  0, sizeof(s_deliv_L));
    memset(s_deliv_R,  0, sizeof(s_deliv_R));
    memset(s_remain_L, 0, sizeof(s_remain_L));
    memset(s_remain_R, 0, sizeof(s_remain_R));
    for(int i=0;i<CTRL_SIDE_CH_MAX;i++){
        s_state_L[i] = CTRL_VALVE_IDLE;
        s_state_R[i] = CTRL_VALVE_IDLE;
    }
    s_Ele_in = s_Y_in = s_Ele_L = s_Ele_R = false;
    s_left_done = s_right_done = false;
    s_active_side = SIDE_NONE;
    s_active_idx  = -1;
    s_batchTarget = 0;
}

/* ================= 手动模式 ================= */

static void manual_mode_step(void)
{
    /* 阀门输出=手动请求 */
    for(uint16_t i=0;i<CTRL_SIDE_CH_MAX;i++){
        s_Y_L[i] = (i < s_left_n  && s_left_en)  ? s_req_L[i] : false;
        s_Y_R[i] = (i < s_right_n && s_right_en) ? s_req_R[i] : false;
    }
    /* 进/卸料、分料开关保持用户设定（由外部接口设置） */
}

/* ================= 自动模式 ================= */

static bool find_next_left(int *idx_out)
{
    for(uint16_t i=0;i<s_left_n && s_left_en;i++){
        if(s_state_L[i] != CTRL_VALVE_DONE){
            *idx_out = (int)i; return true;
        }
    }
    return false;
}

static bool find_next_right(int *idx_out)
{
    for(uint16_t i=0;i<s_right_n && s_right_en;i++){
        if(s_state_R[i] != CTRL_VALVE_DONE){
            *idx_out = (int)i; return true;
        }
    }
    return false;
}

static void auto_mode_step(void)
{
    /* 选择当前处理的通道（先左后右） */
    if(s_active_idx < 0){
        int i = -1;
        if(!s_left_done && find_next_left(&i)){
            s_active_side = SIDE_LEFT;
            s_active_idx  = i;
        }else{
            s_left_done = true;
        }
    }
    if(s_active_idx < 0){
        int i = -1;
        if(!s_right_done && find_next_right(&i)){
            s_active_side = SIDE_RIGHT;
            s_active_idx  = i;
        }else{
            s_right_done = true;
        }
    }
    if(s_active_idx < 0){
        /* 两侧都已完成 */
        s_Ele_in = false;
        return;
    }

    /* 取当前侧的各项引用 */
    bool                is_left = (s_active_side == SIDE_LEFT);
    int                 i       = s_active_idx;
    int32_t            *deliv   = is_left ? &s_deliv_L[i] : &s_deliv_R[i];
    int32_t            *remain  = is_left ? &s_remain_L[i]: &s_remain_R[i];
    int32_t             target  = is_left ?  s_target_L[i]:  s_target_R[i];
    ctrl_valve_state_t *st      = is_left ? &s_state_L[i]   : &s_state_R[i];
    bool               *yout    = is_left ? &s_Y_L[i]       : &s_Y_R[i];

    /* 计算剩余 */
    *remain = target - *deliv;
    if(*remain <= 0){
        *st   = CTRL_VALVE_DONE;
        *yout = false;
        s_active_side = SIDE_NONE;
        s_active_idx  = -1;
        return;
    }

    /* 进入 RUN，并打开当前阀门输出 */
    if(*st == CTRL_VALVE_IDLE){
        *st   = CTRL_VALVE_BULK_RUN;
        s_curWeight = 0;      /* 清零称斗累计 */
        *yout = true;         /* 打开当前阀门输出 */
    }

    /* 批次或余量控制 */
    if(*st == CTRL_VALVE_BULK_RUN){
        s_batchTarget = compute_batch_target(*remain);
        s_Ele_in      = true;                 /* 打开进料电机 */
        if(s_curWeight >= s_batchTarget){
            s_Ele_in = false;                /* 停进料 */
            *deliv  += s_curWeight;
            s_curWeight = 0;                 /* 卸料清零 */
            if(*remain > BULK_CHUNK_G){
                *st = CTRL_VALVE_BULK_RUN;   /* 继续大投 */
            }else if(*remain > 0){
                *st = CTRL_VALVE_FINE_RUN;   /* 进入精投 */
            }else{
                *st   = CTRL_VALVE_DONE;
                *yout = false;
                s_active_side = SIDE_NONE;
                s_active_idx  = -1;
            }
        }
    }
    else if(*st == CTRL_VALVE_FINE_RUN){
        s_batchTarget = *remain;             /* 余数 */
        s_Ele_in      = true;
        if(s_curWeight >= s_batchTarget){
            s_Ele_in    = false;
            *deliv     += s_curWeight;
            *st         = CTRL_VALVE_DONE;
            *yout       = false;
            s_active_side = SIDE_NONE;
            s_active_idx  = -1;
        }
    }
}

/* ================= 对外接口实现 ================= */

void ctrl_init(void)
{
    s_left_en = s_right_en = true;
    s_left_n  = s_right_n  = CTRL_SIDE_CH_MAX;
    memset(s_target_L, 0, sizeof(s_target_L));
    memset(s_target_R, 0, sizeof(s_target_R));
    reset_all();
}

void ctrl_scan_cycle(void)
{
    if(s_Estop){
        reset_all();
        return;
    }
    if(s_mode == CTRL_MODE_MANUAL) manual_mode_step();
    else                           auto_mode_step();
}

void ctrl_emerg_stop(void)
{
    s_Estop = true;
    reset_all();
}

void ctrl_set_mode(ctrl_mode_t m){ s_mode = m; }
ctrl_mode_t ctrl_get_mode(void)  { return s_mode; }

void ctrl_set_cur_weight_g(int32_t g){ s_curWeight = (g < 0) ? 0 : g; }

void ctrl_set_hopper_max_kg(float kg)
{
    int32_t g = KG_TO_G(kg);
    if(g < 1000) g = 1000; /* 最小 1kg，防止 0 */
    s_hopper_max_g = g;
}
int32_t ctrl_get_hopper_max_g(void){ return s_hopper_max_g; }

void ctrl_set_estop(bool en){ s_Estop = en; }
bool ctrl_get_estop(void)   { return s_Estop; }

void ctrl_apply_lineset(bool left_enable,  uint16_t left_count,
                        bool right_enable, uint16_t right_count)
{
    s_left_en  = left_enable;
    s_right_en = right_enable;
    s_left_n   = (left_count  > CTRL_SIDE_CH_MAX) ? CTRL_SIDE_CH_MAX : left_count;
    s_right_n  = (right_count > CTRL_SIDE_CH_MAX) ? CTRL_SIDE_CH_MAX : right_count;

    /* 禁用/超出数量的路置 DONE 并关输出 */
    for(uint16_t i=0;i<CTRL_SIDE_CH_MAX;i++){
        if(!s_left_en || i >= s_left_n){  s_state_L[i] = CTRL_VALVE_DONE; s_Y_L[i]=false; s_req_L[i]=false; }
        if(!s_right_en|| i >= s_right_n){ s_state_R[i] = CTRL_VALVE_DONE; s_Y_R[i]=false; s_req_R[i]=false; }
    }
    s_active_side = SIDE_NONE;
    s_active_idx  = -1;
    s_left_done   = s_right_done = false;
}

void ctrl_set_target_weight_left(uint16_t row, float kg)
{
    if(row >= CTRL_SIDE_CH_MAX) return;
    s_target_L[row] = KG_TO_G(kg);
}
void ctrl_set_target_weight_right(uint16_t row, float kg)
{
    if(row >= CTRL_SIDE_CH_MAX) return;
    s_target_R[row] = KG_TO_G(kg);
}

int32_t ctrl_get_target_weight_left_g (uint16_t row){ return (row<CTRL_SIDE_CH_MAX)? s_target_L[row]:0; }
int32_t ctrl_get_target_weight_right_g(uint16_t row){ return (row<CTRL_SIDE_CH_MAX)? s_target_R[row]:0; }

void ctrl_manual_set_valve(bool left, uint16_t idx, bool open)
{
    if(idx >= CTRL_SIDE_CH_MAX) return;
    if(left){
        s_req_L[idx] = open;
        if(s_mode == CTRL_MODE_MANUAL) s_Y_L[idx] = open;
    }else{
        s_req_R[idx] = open;
        if(s_mode == CTRL_MODE_MANUAL) s_Y_R[idx] = open;
    }
}

void ctrl_manual_set_feeder(bool on)            { s_Ele_in = on; }
void ctrl_manual_set_screw(bool on)             { s_Y_in   = on; }
void ctrl_manual_set_distributor_left (bool on) { s_Ele_L  = on; }
void ctrl_manual_set_distributor_right(bool on) { s_Ele_R  = on; }

/* 状态读取 */
int  ctrl_get_active_side (void){ return s_active_side; }
int  ctrl_get_active_index(void){ return s_active_idx;  }

int32_t ctrl_get_delivered_g(bool left, uint16_t idx)
{
    if(idx >= CTRL_SIDE_CH_MAX) return 0;
    return left ? s_deliv_L[idx] : s_deliv_R[idx];
}
int32_t ctrl_get_remain_g(bool left, uint16_t idx)
{
    if(idx >= CTRL_SIDE_CH_MAX) return 0;
    return left ? s_remain_L[idx] : s_remain_R[idx];
}
ctrl_valve_state_t ctrl_get_state(bool left, uint16_t idx)
{
    if(idx >= CTRL_SIDE_CH_MAX) return CTRL_VALVE_IDLE;
    return left ? s_state_L[idx] : s_state_R[idx];
}

/* 外围执行件状态 */
bool ctrl_get_feeder(void)           { return s_Ele_in; }
bool ctrl_get_screw(void)            { return s_Y_in;   }
bool ctrl_get_distributor_left(void) { return s_Ele_L;  }
bool ctrl_get_distributor_right(void){ return s_Ele_R;  }
