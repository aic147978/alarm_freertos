#ifndef __MYCT_IIC_H
#define __MYCT_IIC_H
#include "sys.h"	    
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//���ݴ�����-IIC ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/12/30
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
//********************************************************************************
//��
////////////////////////////////////////////////////////////////////////////////// 	

//IO��������
#define CT_SDA_IN()  {GPIOI->MODER&=~(3<<(3*2));GPIOI->MODER|=0<<3*2;}	//PI3����ģʽ
#define CT_SDA_OUT() {GPIOI->MODER&=~(3<<(3*2));GPIOI->MODER|=1<<3*2;} 	//PI3���ģʽ
//IO��������	 
#define CT_IIC_SCL  PHout(6)  //SCL
#define CT_IIC_SDA  PIout(3)  //SDA
#define CT_READ_SDA PIin(3)   //����SDA 
 
//#define CT_IIC_SCL_GPIO_PORT              GPIOH
//#define CT_IIC_SCL_GPIO_PIN               GPIO_PIN_6
//#define CT_IIC_SCL_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOH_CLK_ENABLE(); }while(0)    /* PH��ʱ��ʹ�� */

//#define CT_IIC_SDA_GPIO_PORT              GPIOI
//#define CT_IIC_SDA_GPIO_PIN               GPIO_PIN_3
//#define CT_IIC_SDA_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOI_CLK_ENABLE(); }while(0)    /* PI��ʱ��ʹ�� */

///******************************************************************************************/

///* IO���� */
//#define CT_IIC_SCL(x)       do{ x ? \
//                                   HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
//                                   HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
//                               }while(0)       /* SCL */

//#define CT_IIC_SDA(x)       do{ x ? \
//                                   HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
//                                   HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
//                               }while(0)       /* SDA */

//#define CT_READ_SDA         HAL_GPIO_ReadPin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN) /* ����SDA */


//IIC���в�������
void CT_IIC_Init(void);                	//��ʼ��IIC��IO��
void CT_IIC_Start(void);				//����IIC��ʼ�ź�
void CT_IIC_Stop(void);	  				//����IICֹͣ�ź�
void CT_IIC_Send_Byte(uint8_t txd);			//IIC����һ���ֽ�
uint8_t CT_IIC_Read_Byte(unsigned char ack);	//IIC��ȡһ���ֽ�
uint8_t CT_IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void CT_IIC_Ack(void);					//IIC����ACK�ź�
void CT_IIC_NAck(void);					//IIC������ACK�ź�

#endif







