/*
 * zmpt101b.h
 * Description: ZMPT101B voltage sensor interface for STM32
 * Author: Your Name
 * Date: 2025-08-18
 */

#ifndef __ZMPT101B_H
#define __ZMPT101B_H

#include "stm32f4xx_hal.h"   // HAL 库
#include <stdint.h>

// --------------------- 配置区域 ---------------------
// ADC 句柄（在主程序里初始化后传入）
extern ADC_HandleTypeDef hadc1;

// ZMPT101B ADC 通道
#define ZMPT101B_ADC_CHANNEL      ADC_CHANNEL_0   // 对应你接的 ADC 通道
#define ZMPT101B_ADC_RESOLUTION   4095            // 12 位 ADC
#define ZMPT101B_VREF             3.3f            // ADC 参考电压 (V)

// PT 变比与分压系数
#define ZMPT101B_VOLTAGE_SCALE    230.0f / 1.0f   // 例: ADC 电压换算回实际市电电压

// 采样参数
#define ZMPT101B_SAMPLE_COUNT     100             // 每次计算 RMS 的采样点数

// --------------------- 函数声明 ---------------------

/**
 * @brief  初始化 ZMPT101B 采集模块
 * @param  none
 * @retval none
 */
void ZMPT101B_Init(void);

/**
 * @brief  读取一次 ADC 原始值
 * @retval uint16_t ADC 数值
 */
uint16_t ZMPT101B_ReadRaw(void);

/**
 * @brief  读取 ZMPT101B 电压值（V）
 *         通过 ADC 原始值换算成实际市电电压
 * @retval float 电压值 (V)
 */
float ZMPT101B_ReadVoltage(void);

/**
 * @brief  计算 ZMPT101B 电压有效值 RMS
 *         建议多次采样取平均
 * @retval float 电压有效值 RMS (V)
 */
float ZMPT101B_ReadVoltageRMS(void);

#endif /* __ZMPT101B_H */
