#ifndef __easyUI_H
#define __easyUI_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

static uint8_t current_screen = 0;

// 初始化UI
void UI_Init(void);

// 绘制界面
void UI_DrawMainScreen(void);

// 触摸检测
void UI_TouchHandler(uint16_t tx, uint16_t ty);

void UI_DATA_Show(u16 x, u16 y);

// 注册按钮
void UI_AddButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  const char* text, void (*callback)(void));

// 刷新按钮显示
void UI_DrawButtons(void);
void UI_ESP8266(void);

#endif











