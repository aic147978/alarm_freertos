#include "led.h"


//初始化PB1为输出.并使能时钟	    
//LED IO初始化
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_Initure.Pin=GPIO_PIN_0|GPIO_PIN_1;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull=GPIO_PULLUP;
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
	


    GPIO_Initure.Pin=GPIO_PIN_14|GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);

    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_14,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
}

void Temp_Control(uint8_t temp,uint8_t set_temp)
{
    if(temp>set_temp)
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_14,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
    }
    else if(temp<set_temp)
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_14,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_14,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
    }
}



