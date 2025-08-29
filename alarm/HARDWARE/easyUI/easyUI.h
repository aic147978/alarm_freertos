#ifndef __easyUI_H
#define __easyUI_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

static uint8_t current_screen = 0;

// ��ʼ��UI
void UI_Init(void);

// ���ƽ���
void UI_DrawMainScreen(void);

// �������
void UI_TouchHandler(uint16_t tx, uint16_t ty);

void UI_DATA_Show(u16 x, u16 y);

// ע�ᰴť
void UI_AddButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  const char* text, void (*callback)(void));

// ˢ�°�ť��ʾ
void UI_DrawButtons(void);
void UI_ESP8266(void);

#endif











