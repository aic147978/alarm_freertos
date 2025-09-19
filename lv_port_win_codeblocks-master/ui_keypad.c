#include "ui_keypad.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 九键输入上下文结构。
 */
typedef struct {
    lv_obj_t        *target_label; /**< 当前正在编辑的标签。 */
    ui_bind_entry_t *entry;        /**< 对应的绑定条目。 */
} keypad_ctx_t;

/** 记录当前键盘状态的静态变量。 */
static keypad_ctx_t s_ctx = {0};
static lv_obj_t    *s_modal = NULL;    /**< 蒙层对象。 */
static lv_obj_t    *s_kb_box = NULL;   /**< 键盘面板容器。 */
static lv_obj_t    *s_ta = NULL;       /**< 键盘输入框。 */
static bool         s_block_click = false; /**< 用于防抖的开关。 */

static void kb_btnm_event_cb(lv_event_t *e);

/**
 * @brief 解除防抖限制的计时器回调。
 */
static void unblock_cb(lv_timer_t *timer)
{
    s_block_click = false;
    lv_timer_del(timer);
}

/**
 * @brief 按绑定条目的变量内容刷新标签显示。
 */
void ui_keypad_refresh_entry(ui_bind_entry_t *entry)
{
    if(!entry || !entry->label || !entry->var) {
        return;
    }

    char buf[16];
    if(entry->is_time) {
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

/**
 * @brief 关闭键盘弹窗，并开启一次点击防抖。
 */
void ui_keypad_close(void)
{
    if(s_modal) {
        lv_indev_reset(lv_indev_get_act(), NULL);
        lv_obj_del(s_modal);
    }

    s_modal   = NULL;
    s_kb_box  = NULL;
    s_ta      = NULL;
    s_ctx.entry = NULL;

    s_block_click = true;
    lv_timer_create(unblock_cb, 300, NULL);
}

/**
 * @brief 防止事件继续向下传递的空回调。
 */
static void eat_all_cb(lv_event_t *e)
{
    LV_UNUSED(e);
}

/**
 * @brief 打开九键键盘。
 */
static void keypad_open(ui_bind_entry_t *entry)
{
    if(!entry || !entry->label) {
        return;
    }

    ui_keypad_close();

    s_ctx.entry        = entry;
    s_ctx.target_label = entry->label;

    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_50, 0);
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_modal, eat_all_cb, LV_EVENT_ALL, NULL);

    s_kb_box = lv_obj_create(s_modal);
    lv_obj_set_size(s_kb_box, LV_HOR_RES * 3 / 5, LV_VER_RES * 3 / 5);
    lv_obj_center(s_kb_box);

    s_ta = lv_textarea_create(s_kb_box);
    lv_obj_set_width(s_ta, lv_pct(90));
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_password_mode(s_ta, false);
    lv_textarea_set_max_length(s_ta, entry->max_digits);

    /* 预填充已有的数值，方便用户编辑。 */
    if(entry->var) {
        char pre[8] = {0};
        if(entry->is_time) {
            int mm = (*entry->var) / 60;
            int ss = (*entry->var) % 60;
            lv_snprintf(pre, sizeof(pre), "%02d%02d", mm, ss);
        } else {
            lv_snprintf(pre, sizeof(pre), "%d", *entry->var);
        }
        lv_textarea_set_text(s_ta, pre);
        lv_textarea_cursor_right(s_ta);
    }

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

/**
 * @brief 九键键盘按钮事件回调。
 */
static void kb_btnm_event_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btnm,
                                                lv_btnmatrix_get_selected_btn(btnm));
    if(!txt) {
        return;
    }

    ui_bind_entry_t *entry = s_ctx.entry;
    if(!entry) {
        return;
    }

    if(strcmp(txt, "OK") == 0) {
        const char *in = lv_textarea_get_text(s_ta);
        if(entry->is_time) {
            char buf4[5] = "0000";
            size_t n = strlen(in);
            if(n > 4) {
                in += (n - 4);
                n = 4;
            }
            memcpy(buf4 + (4 - n), in, n);
            int mm = (buf4[0]-'0') * 10 + (buf4[1]-'0');
            int ss = (buf4[2]-'0') * 10 + (buf4[3]-'0');
            if(ss > 59) {
                ss = 59;
            }
            if(entry->var) {
                *entry->var = mm * 60 + ss;
            }
        } else {
            int val = atoi(in);
            if(entry->var) {
                *entry->var = val;
            }
        }

        ui_keypad_refresh_entry(entry);
        if(entry->label) {
            lv_obj_invalidate(entry->label);
        }
        ui_keypad_close();
    } else if(strcmp(txt, "<-") == 0) {
        lv_textarea_del_char(s_ta);
    } else if(strcmp(txt, "C") == 0) {
        lv_textarea_set_text(s_ta, "");
    } else {
        const char *cur = lv_textarea_get_text(s_ta);
        size_t cur_digits = strlen(cur);
        if(cur_digits < entry->max_digits && txt[0] >= '0' && txt[0] <= '9') {
            lv_textarea_add_char(s_ta, txt[0]);
        }
    }
}

/**
 * @brief 数值按钮点击事件回调。
 */
static void value_btn_event_cb(lv_event_t *e)
{
    if(s_block_click) {
        return;
    }

    ui_bind_entry_t *entry = (ui_bind_entry_t *)lv_event_get_user_data(e);
    keypad_open(entry);
}

void ui_keypad_bind_button(lv_obj_t *btn, ui_bind_entry_t *entry)
{
    if(!btn || !entry) {
        return;
    }

    lv_obj_add_event_cb(btn, value_btn_event_cb, LV_EVENT_CLICKED, entry);
}
