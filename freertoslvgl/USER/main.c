#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "touch.h"
#include "btim.h"
#include "lvgl.h"
//#include "lv_port_disp_template.h"
//#include "lv_port_indev_template.h"
////#include "lv_timer.h"
//#include "lv_demo_stress.h"
//#include "lv_demo_music.h"
#include "lvgl_demo.h"

/************************************************
 ALIENTEK 阿波罗STM32F429开发板实验30
 LVGL实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com  
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//清空屏幕并在右上角显示"RST"

int main(void)
{
//    HAL_Init();                     //初始化HAL库   
//    Stm32_Clock_Init(360, 25, 2, 8);   //设置时钟,180Mhz
//    delay_init(180);                //初始化延时函数
//    uart_init(115200);              //初始化USART
//    LED_Init();                     //初始化LED 
//    KEY_Init();                     //初始化按键
//    SDRAM_Init();                   //初始化SDRAM
//	

//	btim_timx_int_init(10-1,9000-1);
//	lv_init();
//	lv_port_disp_init();
//	lv_port_indev_init();	
//	
////	lv_obj_t* switch_obj = lv_switch_create(lv_scr_act());
////    lv_obj_set_size(switch_obj, 240, 120);
////    lv_obj_align(switch_obj, LV_ALIGN_CENTER, 0, 0);
//	
//	//lv_demo_stress();
//	lv_demo_music();
//	
//	while(1)
//	{
//		delay_ms(100);
//		lv_timer_handler();
//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // 让 LED 闪烁
//lv_tick_inc(1);
	   /* 初始化硬件 */
    HAL_Init();                     //初始化HAL库   
    Stm32_Clock_Init(360, 25, 2, 8);   //设置时钟,180Mhz
    delay_init(180);                //初始化延时函数
    usart_init(115200);              //初始化USART
    LED_Init();                     //初始化LED           // LCD、触摸、LED、定时器初始化
	  SDRAM_Init();                   //初始化SDRAM
//    lv_init();
//    lv_port_disp_init();
//    lv_port_indev_init();

//    lv_demo_music();     // 或者 lv_demo_printer()
	
	lvgl_demo();

    while(1)
    {
//        lv_timer_handler();  // 刷新 LVGL
//        HAL_Delay(20);        // 延时不要太长
//        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // LED 闪烁，确认循环没卡
    

	}	    
}
