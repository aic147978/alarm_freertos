#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "touch.h"
#include "easyUI.h"
#include "w25qxx.h"
#include "ff.h"
#include "exfuns.h"
#include "string.h"
#include "sdio_sdcard.h"
#include "fontupd.h"
#include "text.h"
#include "malloc.h"
#include "pcf8574.h"
#include "dht11.h"
#include "usmart.h"
#include "rtc.h"
#include "esp8266.h"
#include "FreeRTOS.h"
#include "task.h"
#include "midware.h"



/* 温度采集与LED闪烁任务，5秒执行一次 */


int main(void)
{
    HAL_Init();                     //初始化HAL库
    Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
    delay_init(180);                //初始化延时函数
    uart_init(115200);              //初始化USART
    LED_Init();                     //初始化LED
    KEY_Init();                     //初始化按键
    SDRAM_Init();                   //初始化SDRAM
    LCD_Init();                     //初始化LCD
        tp_dev.init();              //触摸屏初始化
        POINT_COLOR=RED;
        PCF8574_Init();
        PCF8574_ReadBit(BEEP_IO);
    DHT11_Init();
        font_init();
    my_mem_init(SRAMIN);            //初始化内部内存池
    my_mem_init(SRAMEX);            //初始化外部SDRAM内存池
    my_mem_init(SRAMCCM);           //初始化内部CCM内存池
    exfuns_init();                  //为fatfs相关变量申请内存
        W25QXX_Init();
        RTC_Init();                 //初始化RTC
        ESP8266_Init(115200);
        f_mount(fs[0],"0:",1);      //挂载SD卡
        f_mount(fs[1],"1:",1);      //挂载SPI FLASH
        UI_Init();
		

		
		void main_demo(void);

//    vTaskStartScheduler();

    while(1);      //不应运行到这里
}

