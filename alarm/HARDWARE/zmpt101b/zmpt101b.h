/*
 * zmpt101b.h
 * Description: ZMPT101B voltage sensor interface for STM32
 * Author: Your Name
 * Date: 2025-08-18
 */

#ifndef __ZMPT101B_H
#define __ZMPT101B_H

#include "stm32f4xx_hal.h"   // HAL ��
#include <stdint.h>

// --------------------- �������� ---------------------
// ADC ����������������ʼ�����룩
extern ADC_HandleTypeDef hadc1;

// ZMPT101B ADC ͨ��
#define ZMPT101B_ADC_CHANNEL      ADC_CHANNEL_0   // ��Ӧ��ӵ� ADC ͨ��
#define ZMPT101B_ADC_RESOLUTION   4095            // 12 λ ADC
#define ZMPT101B_VREF             3.3f            // ADC �ο���ѹ (V)

// PT ������ѹϵ��
#define ZMPT101B_VOLTAGE_SCALE    230.0f / 1.0f   // ��: ADC ��ѹ�����ʵ���е��ѹ

// ��������
#define ZMPT101B_SAMPLE_COUNT     100             // ÿ�μ��� RMS �Ĳ�������

// --------------------- �������� ---------------------

/**
 * @brief  ��ʼ�� ZMPT101B �ɼ�ģ��
 * @param  none
 * @retval none
 */
void ZMPT101B_Init(void);

/**
 * @brief  ��ȡһ�� ADC ԭʼֵ
 * @retval uint16_t ADC ��ֵ
 */
uint16_t ZMPT101B_ReadRaw(void);

/**
 * @brief  ��ȡ ZMPT101B ��ѹֵ��V��
 *         ͨ�� ADC ԭʼֵ�����ʵ���е��ѹ
 * @retval float ��ѹֵ (V)
 */
float ZMPT101B_ReadVoltage(void);

/**
 * @brief  ���� ZMPT101B ��ѹ��Чֵ RMS
 *         �����β���ȡƽ��
 * @retval float ��ѹ��Чֵ RMS (V)
 */
float ZMPT101B_ReadVoltageRMS(void);

#endif /* __ZMPT101B_H */
