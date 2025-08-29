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
 ALIENTEK ������STM32F429������ʵ��30
 LVGLʵ��-HAL�⺯����
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com  
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

//�����Ļ�������Ͻ���ʾ"RST"

int main(void)
{
//    HAL_Init();                     //��ʼ��HAL��   
//    Stm32_Clock_Init(360, 25, 2, 8);   //����ʱ��,180Mhz
//    delay_init(180);                //��ʼ����ʱ����
//    uart_init(115200);              //��ʼ��USART
//    LED_Init();                     //��ʼ��LED 
//    KEY_Init();                     //��ʼ������
//    SDRAM_Init();                   //��ʼ��SDRAM
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
//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // �� LED ��˸
//lv_tick_inc(1);
	   /* ��ʼ��Ӳ�� */
    HAL_Init();                     //��ʼ��HAL��   
    Stm32_Clock_Init(360, 25, 2, 8);   //����ʱ��,180Mhz
    delay_init(180);                //��ʼ����ʱ����
    usart_init(115200);              //��ʼ��USART
    LED_Init();                     //��ʼ��LED           // LCD��������LED����ʱ����ʼ��
	  SDRAM_Init();                   //��ʼ��SDRAM
//    lv_init();
//    lv_port_disp_init();
//    lv_port_indev_init();

//    lv_demo_music();     // ���� lv_demo_printer()
	
	lvgl_demo();

    while(1)
    {
//        lv_timer_handler();  // ˢ�� LVGL
//        HAL_Delay(20);        // ��ʱ��Ҫ̫��
//        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); // LED ��˸��ȷ��ѭ��û��
    

	}	    
}
