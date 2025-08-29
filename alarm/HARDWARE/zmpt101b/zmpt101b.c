/*
 * zmpt101b.c
 * Description: ZMPT101B voltage sensor driver for STM32
 * Author: Your Name
 * Date: 2025-08-18
 */

#include "zmpt101b.h"
#include "sys.h"
//#include "stm32f4xx_hal_adc_ex.h"
#include <math.h>
#include "adc.h"

// --------------------- 内部函数声明 ---------------------
static float ADC_ToVoltage(uint16_t adc_value);

// --------------------- 函数实现 ---------------------

/**
 * @brief  初始化 ZMPT101B 采集模块
 *         此例假设 ADC 已在 CubeMX 中初始化
 */
void ZMPT101B_Init(void)
{
    // ADC 初始化在 CubeMX 中完成
    // 可以在这里启动 ADC DMA 或校准
    MY_ADC_Init();
}

/**
 * @brief  读取一次 ADC 原始值
 * @retval uint16_t ADC 数值
 */
uint16_t ZMPT101B_ReadRaw(void)
{
    return Get_Adc(ADC_CHANNEL_5); 
}

/**
 * @brief  ADC 原始值换算成实际电压
 * @param  adc_value ADC 原始值
 * @retval float 电压值 (V)
 */
static float ADC_ToVoltage(uint16_t adc_value)
{
    float v_adc = ((float)adc_value / ZMPT101B_ADC_RESOLUTION) * ZMPT101B_VREF;
    float v_real = v_adc * ZMPT101B_VOLTAGE_SCALE; // 根据 PT 和分压系数换算
    return v_real;
}

/**
 * @brief  读取一次瞬时电压
 * @retval float 电压值 (V)
 */
float ZMPT101B_ReadVoltage(void)
{
    uint16_t adc = ZMPT101B_ReadRaw();
    return ADC_ToVoltage(adc);
}

/**
 * @brief  计算 RMS 电压
 * @retval float 电压有效值 RMS (V)
 */



float ZMPT101B_ReadVoltageRMS(void)
{
    float sum_squares = 0.0f;
	uint16_t i;
	float mean;
    for (i = 0; i < ZMPT101B_SAMPLE_COUNT; i++)
    {
        uint16_t adc = ZMPT101B_ReadRaw();
        float v = ADC_ToVoltage(adc);
        sum_squares += v * v;
        HAL_Delay(1); // 采样间隔可调，保证覆盖至少一个电网周期
    }

    mean = sum_squares / ZMPT101B_SAMPLE_COUNT;
    return sqrtf(mean); // 有效值 RMS
}
