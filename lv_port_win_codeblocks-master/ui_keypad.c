/**
 * @file ui_keypad.c
 * @brief 基于 LVGL 的简易九键数字键盘弹窗（支持纯数字/时间 mm:ss 两种显示）
 *
 * 功能要点：
 * 1) “绑定模型”（ui_bind_entry_t）：把一个 label + 一个整型变量 + 显示格式 绑定在一起；
 * 2) 调用 ui_keypad_bind_button(btn, entry) 后，点击按钮会弹出九键键盘；
 * 3) 在键盘按 OK，自动把用户输入写回 entry->var，并刷新 entry->label 的显示；
 * 4) s_block_click + 300ms 计时器用于收尾的轻微“防抖”，避免弹窗关闭瞬间又点到底层按钮；
 * 5) 键盘作为模态层（s_modal）挂到 lv_layer_top()，不会影响页面布局；
 *
 * 典型用法：
 *   // 1) 准备要绑定的变量和展示 label
 *   static int g_weight = 0;
 *   lv_obj_t *btn = lv_btn_create(parent);
 *   lv_obj_t *lab = lv_label_create(btn);
 *
 *   // 2) 初始化一个 entry
 *   static ui_bind_entry_t entry;
 *   ui_keypad_entry_init(&entry, lab, &g_weight, "kg", false, 3);
 *
 *   // 3) 绑定按钮 → 点击弹键盘
 *   ui_keypad_bind_button(btn, &entry);
 *
 * 注意：
 * - 如果 entry->is_time = true，则 var 存储的是“总秒数”，显示为 mm:ss；
 * - max_digits 控制可输入的“数字位数”，时间模式下建议写 4（mmss）；
 * - 本实现不支持负数；如需支持，可自行在 kb 逻辑中加入 “-” 按键与解析；
 */

#include "ui_keypad.h"
#include <stdlib.h>
#include <string.h>

/*========================
=  1. 运行时上下文结构体  =
========================*/

/**
 * @brief 键盘当前会话的上下文
 *
 * - target_label：当前被编辑的 label（用于居中、也可用于 debug）
 * - entry：正在编辑的绑定条目（包含变量地址/显示格式等）
 */
typedef struct {
    lv_obj_t        *target_label;
    ui_bind_entry_t *entry;
} keypad_ctx_t;

/*========================
=  2. 文件内静态状态变量  =
========================*/

static keypad_ctx_t s_ctx = {0};           /**< 当前键盘会话上下文 */
static lv_obj_t    *s_modal = NULL;        /**< 遮罩/模态层（挂在顶层） */
static lv_obj_t    *s_kb_box = NULL;       /**< 键盘面板容器（白底） */
static lv_obj_t    *s_ta = NULL;           /**< 键盘输入框（lv_textarea） */
static bool         s_block_click = false; /**< 关闭后短时防抖标记 */

static void kb_btnm_event_cb(lv_event_t *e); /* 键盘按钮矩阵的事件处理 */

/*==================================
=  3. 防抖计时器（弹窗关闭后使用）  =
==================================*/

/**
 * @brief 解除防抖的计时器回调
 * @details 在 ui_keypad_close() 收尾时调用，避免关闭弹窗瞬间点穿到底层按钮。
 */
static void unblock_cb(lv_timer_t *timer)
{
    s_block_click = false;
    lv_timer_del(timer);
}

/*==================================
=  4. 绑定条目 → 刷新标签显示文本   =
==================================*/

/**
 * @brief 按绑定条目的变量内容刷新标签显示。
 * @param entry 绑定描述：label/var/suffix/is_time
 *
 * - 时间模式：把 var（单位秒）转为 "mm:ss"
 * - 普通模式：把 var 转为十进制字符串，并可拼接单位后缀（如 "kg"）
 */
void ui_keypad_refresh_entry(ui_bind_entry_t *entry)
{
    if(!entry || !entry->label || !entry->var) {
        return;
    }

    char buf[16];
    if(entry->is_time) {
        /* var 以“秒”存储，显示为 mm:ss */
        int mm = (*entry->var) / 60;
        int ss = (*entry->var) % 60;
        lv_snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
    } else if(entry->suffix) {
        lv_snprintf(buf, sizeof(buf), "%d%s", *entry->var, entry->suffix);
    } else {
        lv_snprintf(buf, sizeof(buf), "%d", *entry->var);
    }

    lv_label_set_text(entry->label, buf);
}

/**
 * @brief 初始化一个“绑定条目”
 * @param entry      输出：被初始化的条目
 * @param label      这个条目要刷新的 label（通常放在你的按钮上）
 * @param var        这个条目绑定的整型变量地址
 * @param suffix     单位后缀，如 "kg"；可为 NULL
 * @param is_time    是否时间模式：true → var 以“秒”存，显示为 mm:ss
 * @param max_digits 最多输入的数字位数（时间建议 4，对应 mmss）
 *
 * 说明：
 * - 此函数只做数据绑定与 label 首次刷新；弹窗由 bind_button 的事件触发。
 */
void ui_keypad_entry_init(ui_bind_entry_t *entry,
                          lv_obj_t *label,
                          int *var,
                          const char *suffix,
                          bool is_time,
                          uint8_t max_digits)
{
    if(!entry) {
        return;
    }

    entry->label      = label;
    entry->var        = var;
    entry->suffix     = suffix;
    entry->is_time    = is_time;
    entry->max_digits = max_digits;

    ui_keypad_refresh_entry(entry);
}

/*========================
=  5. 关闭弹窗（含防抖）  =
========================*/

/**
 * @brief 关闭键盘弹窗，并开启一次点击防抖。
 *
 * - 重置当前输入设备，避免“释放事件”落到底层；
 * - 删除模态层，该层下所有子对象（s_kb_box/s_ta/btnmatrix）一并销毁；
 * - 开一个 300ms 计时器，避免“关闭瞬间”点穿到底层按钮引发误触。
 */
void ui_keypad_close(void)
{
    if(s_modal) {
        /* 重置当前输入设备的活动对象，避免弹窗删除后 release 事件传给底层 */
        lv_indev_reset(lv_indev_get_act(), NULL);
        lv_obj_del(s_modal);
    }

    /* 清理静态指针 */
    s_modal   = NULL;
    s_kb_box  = NULL;
    s_ta      = NULL;
    s_ctx.entry = NULL;

    /* 开启短暂防抖 */
    s_block_click = true;
    lv_timer_create(unblock_cb, 300, NULL);
}

/*=====================================
=  6. 顶层模态层事件（吃掉所有事件）  =
=====================================*/

/**
 * @brief 空回调：吃掉事件，防止继续向下传递
 *
 * 把这个回调绑定到 s_modal 上（LV_EVENT_ALL），即可禁止事件穿透到底层。
 */
static void eat_all_cb(lv_event_t *e)
{
    LV_UNUSED(e);
}

/*==============================
=  7. 打开九键键盘（核心入口）  =
==============================*/

/**
 * @brief 打开九键键盘弹窗（绑定到一个 entry）
 * @param entry 要编辑的绑定描述
 *
 * 结构：
 *   s_modal (全屏半透明遮罩)
 *     └─ s_kb_box (白色键盘面板)
 *          ├─ s_ta (输入框)
 *          └─ btnmatrix (数字键/功能键)
 */
static void keypad_open(ui_bind_entry_t *entry)
{
    if(!entry || !entry->label) {
        return;
    }

    /* 先确保没有遗留的弹窗（也顺便触发一次防抖） */
    ui_keypad_close();

    s_ctx.entry        = entry;
    s_ctx.target_label = entry->label;

    /* 1) 蒙层 */
    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_modal);               /* 去默认样式，纯自定义 */
    lv_obj_set_size(s_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_50, 0); /* 半透明 */
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_modal, eat_all_cb, LV_EVENT_ALL, NULL);

    /* 2) 键盘盒子（居中白色面板） */
    s_kb_box = lv_obj_create(s_modal);
    lv_obj_set_size(s_kb_box, LV_HOR_RES * 3 / 5, LV_VER_RES * 3 / 5);
    lv_obj_center(s_kb_box);

    /* 3) 输入框（单行） */
    s_ta = lv_textarea_create(s_kb_box);
    lv_obj_set_width(s_ta, lv_pct(90));
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_password_mode(s_ta, false);
    lv_textarea_set_max_length(s_ta, entry->max_digits);

    /* 预填充已有数值，方便用户“在原值基础上修改” */
    if(entry->var) {
        char pre[8] = {0};
        if(entry->is_time) {
            /* 时间模式预填 mmss（不带冒号，避免占位） */
            int mm = (*entry->var) / 60;
            int ss = (*entry->var) % 60;
            lv_snprintf(pre, sizeof(pre), "%02d%02d", mm, ss);
        } else {
            lv_snprintf(pre, sizeof(pre), "%d", *entry->var);
        }
        lv_textarea_set_text(s_ta, pre);
        lv_textarea_cursor_right(s_ta);
    }

    /* 4) 九键 + 功能键 的按钮矩阵 */
    static const char *map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "<-", "0", "C", "OK", ""
    };

    lv_obj_t *btnm = lv_btnmatrix_create(s_kb_box);
    lv_btnmatrix_set_map(btnm, map);
    lv_obj_set_size(btnm, lv_pct(90), lv_pct(70));
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btnm, kb_btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/*======================================
=  8. 按键矩阵事件回调（输入/确认/删除）  =
======================================*/

/**
 * @brief 九键按钮事件
 *
 * 处理：
 * - “OK”  → 把 s_ta 的内容写回 entry->var，并刷新 label；
 * - “<-”  → 删除一个字符；
 * - “C”   → 清空当前输入；
 * - “0~9” → 追加一个数字（受 max_digits 限制）。
 */
static void kb_btnm_event_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(
        btnm, lv_btnmatrix_get_selected_btn(btnm)
    );
    if(!txt) {
        return;
    }

    ui_bind_entry_t *entry = s_ctx.entry;
    if(!entry) {
        return;
    }

    if(strcmp(txt, "OK") == 0) {
        /* 用户确认 */
        const char *in = lv_textarea_get_text(s_ta);
        if(entry->is_time) {
            /* 输入按“mmss”解析并转为“秒” */
            char buf4[5] = "0000";
            size_t n = strlen(in);
            if(n > 4) {
                /* 只保留最后 4 位：例如输入 001234 → 解析为 12:34 */
                in += (n - 4);
                n = 4;
            }
            memcpy(buf4 + (4 - n), in, n);

            int mm = (buf4[0]-'0') * 10 + (buf4[1]-'0');
            int ss = (buf4[2]-'0') * 10 + (buf4[3]-'0');
            if(ss > 59) ss = 59;   /* 简单约束：秒最大 59 */

            if(entry->var) {
                *entry->var = mm * 60 + ss;
            }
        } else {
            /* 普通数字模式 */
            int val = atoi(in);    /* 负数/前导 0 不处理，按纯整型解析 */
            if(entry->var) {
                *entry->var = val;
            }
        }

        /* 刷新显示 + 关闭弹窗 */
        ui_keypad_refresh_entry(entry);
        if(entry->label) {
            lv_obj_invalidate(entry->label);
        }
        ui_keypad_close();

    } else if(strcmp(txt, "<-") == 0) {
        /* 退格 */
        lv_textarea_del_char(s_ta);

    } else if(strcmp(txt, "C") == 0) {
        /* 清空 */
        lv_textarea_set_text(s_ta, "");

    } else {
        /* 追加数字（受 max_digits 限制） */
        const char *cur = lv_textarea_get_text(s_ta);
        size_t cur_digits = strlen(cur);
        if(cur_digits < entry->max_digits && txt[0] >= '0' && txt[0] <= '9') {
            lv_textarea_add_char(s_ta, txt[0]);
        }
    }
}

/*====================================
=  9. 绑定按钮 → 点击打开键盘弹窗      =
====================================*/

/**
 * @brief 与按钮的 CLICK 事件绑定：点击弹出键盘
 */
static void value_btn_event_cb(lv_event_t *e)
{
    if(s_block_click) {
        /* 防抖命中 → 忽略本次点击 */
        return;
    }

    ui_bind_entry_t *entry = (ui_bind_entry_t *)lv_event_get_user_data(e);
    keypad_open(entry);
}

/**
 * @brief 把一个按钮与“绑定条目”关联起来
 * @param btn   被点击的按钮
 * @param entry 这个按钮要编辑的条目（变量/显示/格式）
 *
 * 注意：entry 的存续期应 ≥ btn（通常把 entry 定为静态或全局）。
 */
void ui_keypad_bind_button(lv_obj_t *btn, ui_bind_entry_t *entry)
{
    if(!btn || !entry) {
        return;
    }

    lv_obj_add_event_cb(btn, value_btn_event_cb, LV_EVENT_CLICKED, entry);
}
