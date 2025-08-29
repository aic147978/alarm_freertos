#ifndef __MYCT_IIC_H
#define __MYCT_IIC_H
#include "sys.h"	    
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//电容触摸屏-IIC 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/12/30
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
//********************************************************************************
//无
////////////////////////////////////////////////////////////////////////////////// 	

//IO方向设置
#define CT_SDA_IN()  {GPIOI->MODER&=~(3<<(3*2));GPIOI->MODER|=0<<3*2;}	//PI3输入模式
#define CT_SDA_OUT() {GPIOI->MODER&=~(3<<(3*2));GPIOI->MODER|=1<<3*2;} 	//PI3输出模式
//IO操作函数	 
#define CT_IIC_SCL  PHout(6)  //SCL
#define CT_IIC_SDA  PIout(3)  //SDA
#define CT_READ_SDA PIin(3)   //输入SDA 
 
//#define CT_IIC_SCL_GPIO_PORT              GPIOH
//#define CT_IIC_SCL_GPIO_PIN               GPIO_PIN_6
//#define CT_IIC_SCL_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOH_CLK_ENABLE(); }while(0)    /* PH口时钟使能 */

//#define CT_IIC_SDA_GPIO_PORT              GPIOI
//#define CT_IIC_SDA_GPIO_PIN               GPIO_PIN_3
//#define CT_IIC_SDA_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOI_CLK_ENABLE(); }while(0)    /* PI口时钟使能 */

///******************************************************************************************/

///* IO操作 */
//#define CT_IIC_SCL(x)       do{ x ? \
//                                   HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
//                                   HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
//                               }while(0)       /* SCL */

//#define CT_IIC_SDA(x)       do{ x ? \
//                                   HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
//                                   HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
//                               }while(0)       /* SDA */

//#define CT_READ_SDA         HAL_GPIO_ReadPin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN) /* 输入SDA */


//IIC所有操作函数
void CT_IIC_Init(void);                	//初始化IIC的IO口
void CT_IIC_Start(void);				//发送IIC开始信号
void CT_IIC_Stop(void);	  				//发送IIC停止信号
void CT_IIC_Send_Byte(uint8_t txd);			//IIC发送一个字节
uint8_t CT_IIC_Read_Byte(unsigned char ack);	//IIC读取一个字节
uint8_t CT_IIC_Wait_Ack(void); 				//IIC等待ACK信号
void CT_IIC_Ack(void);					//IIC发送ACK信号
void CT_IIC_NAck(void);					//IIC不发送ACK信号

#endif







