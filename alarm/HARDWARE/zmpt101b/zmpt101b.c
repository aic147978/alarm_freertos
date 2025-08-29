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

// --------------------- �ڲ��������� ---------------------
static float ADC_ToVoltage(uint16_t adc_value);

// --------------------- ����ʵ�� ---------------------

/**
 * @brief  ��ʼ�� ZMPT101B �ɼ�ģ��
 *         �������� ADC ���� CubeMX �г�ʼ��
 */
void ZMPT101B_Init(void)
{
    // ADC ��ʼ���� CubeMX �����
    // �������������� ADC DMA ��У׼
    MY_ADC_Init();
}

/**
 * @brief  ��ȡһ�� ADC ԭʼֵ
 * @retval uint16_t ADC ��ֵ
 */
uint16_t ZMPT101B_ReadRaw(void)
{
    return Get_Adc(ADC_CHANNEL_5); 
}

/**
 * @brief  ADC ԭʼֵ�����ʵ�ʵ�ѹ
 * @param  adc_value ADC ԭʼֵ
 * @retval float ��ѹֵ (V)
 */
static float ADC_ToVoltage(uint16_t adc_value)
{
    float v_adc = ((float)adc_value / ZMPT101B_ADC_RESOLUTION) * ZMPT101B_VREF;
    float v_real = v_adc * ZMPT101B_VOLTAGE_SCALE; // ���� PT �ͷ�ѹϵ������
    return v_real;
}

/**
 * @brief  ��ȡһ��˲ʱ��ѹ
 * @retval float ��ѹֵ (V)
 */
float ZMPT101B_ReadVoltage(void)
{
    uint16_t adc = ZMPT101B_ReadRaw();
    return ADC_ToVoltage(adc);
}

/**
 * @brief  ���� RMS ��ѹ
 * @retval float ��ѹ��Чֵ RMS (V)
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
        HAL_Delay(1); // ��������ɵ�����֤��������һ����������
    }

    mean = sum_squares / ZMPT101B_SAMPLE_COUNT;
    return sqrtf(mean); // ��Чֵ RMS
}
