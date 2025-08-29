#include "midware.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
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




/******************************************************************************************************/
/*FreeRTOS����*/

/* START_TASK ���� ����
 * ����: ������ �������ȼ� ��ջ��С ��������
 */
#define START_TASK_PRIO     1           /* �������ȼ� */
#define START_STK_SIZE      128         /* �����ջ��С */
TaskHandle_t StartTask_Handler;         /* ������ */
void start_task(void *pvParameters);    /* ������ */

/* LV_DEMO_TASK ���� ����
 * ����: ������ �������ȼ� ��ջ��С ��������
 */
#define CTP_TASK_PRIO   3           /* �������ȼ� */
#define CTP_STK_SIZE    1024        /* �����ջ��С */
TaskHandle_t CTPTask_Handler;       /* ������ */
void ctp_task(void *pvParameters);  /* ������ */

/* LED_TASK ���� ����
 * ����: ������ �������ȼ� ��ջ��С ��������
 *temp_led_task
 */
#define TEMP_LED_TASK_PRIO       4           /* �������ȼ� */
#define TEMP_LED_STK_SIZE        128         /* �����ջ��С */
TaskHandle_t TEMPLEDTask_Handler;           /* ������ */
void temp_led_task(void *pvParameters);      /* ������ */
/******************************************************************************************************/


void main_demo(void)
{


    xTaskCreate((TaskFunction_t )start_task,            /* ������ */
                (const char*    )"start_task",          /* �������� */
                (uint16_t       )START_STK_SIZE,        /* �����ջ��С */
                (void*          )NULL,                  /* ���ݸ��������Ĳ��� */
                (UBaseType_t    )START_TASK_PRIO,       /* �������ȼ� */
                (TaskHandle_t*  )&StartTask_Handler);   /* ������ */

    vTaskStartScheduler();                              /* ����������� */
}

/**
 * @brief       start_task
 * @param       pvParameters : �������(δ�õ�)
 * @retval      ��
 */
void start_task(void *pvParameters)
{
    pvParameters = pvParameters;
    
    taskENTER_CRITICAL();           /* �����ٽ��� */

    /* ����LVGL���� */
    xTaskCreate((TaskFunction_t )ctp_task,
                (const char*    )"ctp_task",
                (uint16_t       )CTP_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )CTP_TASK_PRIO,
                (TaskHandle_t*  )&CTPTask_Handler);

    /* LED�������� */
    xTaskCreate((TaskFunction_t )temp_led_task,
                (const char*    )"led_task",
                (uint16_t       )TEMP_LED_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TEMP_LED_TASK_PRIO,
                (TaskHandle_t*  )&TEMPLEDTask_Handler);

    taskEXIT_CRITICAL();            /* �˳��ٽ��� */
    vTaskDelete(StartTask_Handler); /* ɾ����ʼ���� */
}




void Load_Drow_Dialog(void)
{
        LCD_Clear(WHITE);                 //����
        POINT_COLOR=BLUE;                 //��������Ϊ��ɫ
        LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");
        POINT_COLOR=RED;                  //�ָ�������ɫ
}

void gui_draw_hline(u16 x0,u16 y0,u16 len,u16 color)
{
        if(len==0)return;
    if((x0+len-1)>=lcddev.width)x0=lcddev.width-len-1;  //�������귶Χ
    if(y0>=lcddev.height)y0=lcddev.height-1;            //�������귶Χ
        LCD_Fill(x0,y0,x0+len-1,y0,color);
}

//��ʵ��Բ
void gui_fill_circle(u16 x0,u16 y0,u16 r,u16 color)
{
        u32 i;
        u32 imax = ((u32)r*707)/1000+1;
        u32 sqmax = (u32)r*(u32)r+(u32)r/2;
        u32 x=r;
        gui_draw_hline(x0-r,y0,2*r,color);

        for (i=1;i<=imax;i++)
        {
                if ((i*i+x*x)>sqmax)
                {
                        if (x>imax)
                        {
                                gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
                                gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
                        }
                        x--;
                }
                gui_draw_hline(x0-x,y0+i,2*x,color);
                gui_draw_hline(x0-x,y0-i,2*x,color);
        }
}

//������֮��ľ���ֵ
u16 my_abs(u16 x1,u16 x2)
{
        if(x1>x2)return x1-x2;
        else return x2-x1;
}

//��һ������
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2,u8 size,u16 color)
{
        u16 t;
        int xerr=0,yerr=0,delta_x,delta_y,distance;
        int incx,incy,uRow,uCol;
        if(x1<size|| x2<size||y1<size|| y2<size)return;
        delta_x=x2-x1;             //������������
        delta_y=y2-y1;
        uRow=x1;
        uCol=y1;
        if(delta_x>0)incx=1;       //���õ�������
        else if(delta_x==0)incx=0; //��ֱ��
        else {incx=-1;delta_x=-delta_x;}
        if(delta_y>0)incy=1;
        else if(delta_y==0)incy=0; //ˮƽ��
        else{incy=-1;delta_y=-delta_y;}
        if( delta_x>delta_y)distance=delta_x; //ѡȡ��������������
        else distance=delta_y;
        for(t=0;t<=distance+1;t++ )
        {
                gui_fill_circle(uRow,uCol,size,color);
                xerr+=delta_x ;
                yerr+=delta_y ;
                if(xerr>distance)
                {
                        xerr-=distance;
                        uRow+=incx;
                }
                if(yerr>distance)
                {
                        yerr-=distance;
                        uCol+=incy;
                }
        }
}

const u16 POINT_COLOR_TBL[10]={RED,GREEN,BLUE,BROWN,GRED,BRED,GBLUE,LIGHTBLUE,BRRED,GRAY};


/* ���������� */
void ctp_task(void *pvParameters)
{
        u16 x,y;
        u16 abc,bac;
        u8 t=0;
//        u8 i=0;
        u16 lastpos[10][2];             //���һ�ε�����
        u8 maxp=5;

        if(lcddev.id==0X1018)maxp=10;
        while(1)
        {
                tp_dev.scan(0);

                for(t=0;t<maxp;t++)
                {
                        if((tp_dev.sta)&(1<<t))
                        {
                                if(tp_dev.x[t]<lcddev.width&&tp_dev.y[t]<lcddev.height)
                                {
                                        if(lastpos[t][0]==0XFFFF)
                                        {
                                                lastpos[t][0] = tp_dev.x[t];
                                                lastpos[t][1] = tp_dev.y[t];
                                        }
                                        lcd_draw_bline(lastpos[t][0],lastpos[t][1],tp_dev.x[t],tp_dev.y[t],2,POINT_COLOR_TBL[t]);
                                        lastpos[t][0]=tp_dev.x[t];
                                        lastpos[t][1]=tp_dev.y[t];

                                        x=tp_dev.x[t];
                                        y=tp_dev.y[t];
                                        UI_TouchHandler(x,y);

                                        if(tp_dev.x[t]>(lcddev.width-24)&&tp_dev.y[t]<20)
                                        {
                                                Load_Drow_Dialog();
                                        }
                                }
                        }else lastpos[t][0]=0XFFFF;
                }
                ESP8266_MQTT_LED_Process();
                vTaskDelay(pdMS_TO_TICKS(5));
 //               i++;
                UI_DATA_Show(abc,bac);
 //               if(i%20==0)LED0=!LED0;

        }
}

/**
 * @brief       led_task
 * @param       pvParameters : �������(δ�õ�)
 * @retval      ��
 */
void temp_led_task(void *pvParameters)
{
        u8 temp,humi;
        while(1)
        {
                if(DHT11_Read_Data(&temp,&humi)==0)
                {
                        printf("T:%d H:%d\r\n",temp,humi);
                }
                LED0=!LED0;
                vTaskDelay(pdMS_TO_TICKS(5000));
        }
}


