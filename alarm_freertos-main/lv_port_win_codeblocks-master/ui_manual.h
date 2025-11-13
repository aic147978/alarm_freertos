/* ui_manual.h — 手动模式独立页（创建/销毁/同步/运行控制）
 *
 * 用法小抄：
 *   1) #include "ui_manual.h"
 *   2) manual_ui_create(parent);          // 在你的中心容器里创建界面
 *   3) manual_ui_sync_from_auto();        // 从自动模式(ui_left1)拷贝一份“副本”
 *   4) manual_ui_bind_outputs(&start,&stop); // 绑定两路输出布尔量（★★你来传地址）
 *   5) 需要时 manual_ui_destroy();
 *
 * 说明：
 *   - 顶部绘制主料塔和两条出料线；两行阀门个数 = ui_left1 的阀门数量。
 *   - 点击任一阀门进入“单选高亮”，下方的 weigh/time 显示对应阀门的“手动副本数据”。
 *   - weigh/time 可在此页修改，但不会反写自动模式；只改手动页副本。
 *   - Start 按钮会在“Start ⇄ Suspend”间切换，同时写出你绑定的 start/stop 两个布尔位。
 */

#pragma once
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 创建与销毁 */
void manual_ui_create(lv_obj_t *parent);
void manual_ui_destroy(void);

/* 从自动模式拿一份“副本”到手动页（不影响自动） */
void manual_ui_sync_from_auto(void);

/* 绑定运行输出位（★★你来传入业务层变量地址） */
void manual_ui_bind_outputs(bool *p_start, bool *p_stop);

/* 可选：外部设置/获取“当前选中阀门” */
void manual_ui_set_selected(int line /*0 left, 1 right*/, int index /*0..count-1*/);
int  manual_ui_get_selected_line(void);
int  manual_ui_get_selected_index(void);

/* 可选：刷新整页（阀门数变化/窗口尺寸变化时） */
void manual_ui_redraw(void);

/* 运行状态（Start后为true，Suspend/Stop为false） */
bool manual_ui_is_running(void);

// ui_manual.h
typedef void (*manual_exit_cb_t)(void);

/** 由外部（ui_setting.c）设置 Exit 回调 */
void manual_ui_set_exit_cb(manual_exit_cb_t cb);

#ifdef __cplusplus
}
#endif
