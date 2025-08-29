#include "easyUI.h"
#include <string.h>
#include <stdio.h>
#include "lcd.h"   // ���Լ���LCD����
#include "touch.h" // ����������
#include "text.h"
#include "stdint.h"
#include "dht11.h"
#include "pcf8574.h"
#include "rtc.h"
#include "esp8266.h"


// Target temperature (0.1C units)
static int target_temp = 250;

#define ESP8266_MAX_RESP 256
static char esp8266_resp[ESP8266_MAX_RESP];
//const unsigned char hanzi_wen[32] = {
//    0x00,0x10,0x00,0x10,0x3F,0xFC,0x20,0x08,
//    0x20,0x08,0x3F,0xF8,0x22,0x48,0x22,0x48,
//    0x3E,0x48,0x22,0x48,0x22,0x48,0x3E,0x48,
//    0x20,0x48,0x20,0x48,0x20,0x48,0x00,0x48
//};
static u8 rtc_set_year, rtc_set_month, rtc_set_day;
static u8 rtc_set_hour, rtc_set_min, rtc_set_sec;
static u8 rtc_set_initialized = 0;


void UI_Main_show(void){
		u8 T,dT,temperature, humidity;
        static u8 T_last=0;
		char buf[32]; 
        u8 tbuf[40];	
	    RTC_TimeTypeDef RTC_TimeStruct;
        RTC_DateTypeDef RTC_DateStruct;
		PCF8574_ReadBit(BEEP_IO);
		DHT11_Read_Data(&temperature, &humidity);
	    dT = abs(T - T_last);
	
			if(dT>10){
				sprintf(buf, "%d ��", T_last); 
				Show_Str(360, 100, 300, 16, (uint8_t*)buf, 16, 0);
			}else{
				T=temperature;
				T_last=T;
				sprintf(buf, "%d ��", T); 
				Show_Str(360, 100, 300, 16, (uint8_t*)buf, 16, 0);
			}
			
			
			HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
			sprintf((char*)tbuf,"%02d:%02d:%02d",RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds); 
			LCD_ShowString(80,100,210,16,16,tbuf);	
            HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);
//			sprintf((char*)tbuf,"Date:20%02d-%02d-%02d",RTC_DateStruct.Year,RTC_DateStruct.Month,RTC_DateStruct.Date); 
//			LCD_ShowString(30,160,210,16,16,tbuf);


		}

void UI_Main(void) {
    LCD_Clear(WHITE);

	Show_Str_Mid(130,259,"�����������",32,220);
	Show_Str_Mid(130,334,"�ɼ��豸����",32,220);
	Show_Str_Mid(130,409,"�¶ȱ�������",32,220);
	Show_Str_Mid(130,484,"����״̬��ѯ",32,220);
	Show_Str_Mid(130,559,"������������",32,220);
	Show_Str_Mid(130,634,"ϵͳά������",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);
	LCD_DrawRectangle(130,475,350,525);   
    LCD_DrawRectangle(130,550,350,600);   
    LCD_DrawRectangle(130,625,350,675);
	
	Show_Str(30,100,200,16,"ʱ�䣺",16,0); 
    Show_Str(30,150,200,16,"�������䣺",16,0);    
    Show_Str(280,100,200,16,"��ǰ�¶ȣ�",16,0);
	Show_Str(280,150,200,16,"Ŀ���¶ȣ�",16,0);
	
    current_screen = 0;
}
//�����������
void UI_100(void) {
    LCD_Clear(WHITE);
	
	Show_Str_Mid(130,259,"�����״̬�鿴",32,220);
	Show_Str_Mid(130,334,"������ѹ�鿴",32,220);
	Show_Str_Mid(130,409,"�������ֵ����",32,220);

	
    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);

	
	Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 
	
    current_screen = 20;
}

// �Ӳ˵��������״̬�鿴
void UI_110(void) {
    LCD_Clear(WHITE);
	
	Show_Str_Mid(130,259,"�Ƿ�ϵ磺",32,220);
	Show_Str_Mid(130,334,"�����⣺",32,220);
	Show_Str_Mid(130,409,"ȱ���⣺",32,220);
	Show_Str_Mid(130,484,"��ߵ�ѹ��",32,220);
	Show_Str_Mid(130,559,"��͵�ѹ��",32,220);
	
    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);
	LCD_DrawRectangle(130,475,350,525);   
    LCD_DrawRectangle(130,550,350,600);
	
	Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 
	
    current_screen = 21;
}

// �Ӳ˵�����ߵ�ѹ
void UI_111(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"A��",32,220);
    Show_Str_Mid(130,334,"B��",32,220);
    Show_Str_Mid(130,409,"C��",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 

    current_screen = 22;
}

// �Ӳ˵�����͵�ѹ
void UI_112(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"A��",32,220);
    Show_Str_Mid(130,334,"B��",32,220);
    Show_Str_Mid(130,409,"C��",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 

    current_screen = 23;
}

// �Ӳ˵���������ѹ�鿴
void UI_120(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"A��",32,220);
    Show_Str_Mid(130,334,"B��",32,220);
    Show_Str_Mid(130,409,"C��",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   
    LCD_DrawRectangle(130,400,350,450);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 

    current_screen = 24;
}

// �Ӳ˵����������ֵ����
void UI_130(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"��ߵ�ѹ",32,220);
    Show_Str_Mid(130,334,"��͵�ѹ",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 

    current_screen = 25;
}

// ���㣺�ɼ��豸����
void UI_200(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"������",32,220);
    Show_Str_Mid(130,334,"FutureX",32,220);
    Show_Str_Mid(130,409,"����",32,220);
    Show_Str_Mid(130,484,"�η�",32,220);
    Show_Str_Mid(130,559,"���ֻ�",32,220);
    Show_Str_Mid(130,634,"ˮ��",32,220);

    LCD_DrawRectangle(130,250,350,300);
    LCD_DrawRectangle(130,325,350,375);
    LCD_DrawRectangle(130,400,350,450);
    LCD_DrawRectangle(130,475,350,525);
    LCD_DrawRectangle(130,550,350,600);
    LCD_DrawRectangle(130,625,350,675);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 30;
}

// �Ӳ˵���������
void UI_210(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"״̬����",32,220);
    Show_Str_Mid(130,334,"��ַ����",32,220);

    LCD_DrawRectangle(130,250,350,300);
    LCD_DrawRectangle(130,325,350,375);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 31;
}

// �Ӳ˵���FutureX
void UI_220(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"FutureX����",32,220);

    LCD_DrawRectangle(130,250,350,300);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 32;
}

// �Ӳ˵�������
void UI_230(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"��������",32,220);

    LCD_DrawRectangle(130,250,350,300);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 33;
}

// �Ӳ˵����η�
void UI_240(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"�η�����",32,220);

    LCD_DrawRectangle(130,250,350,300);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 34;
}

// �Ӳ˵������ֻ�
void UI_250(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"���ֻ�����",32,220);

    LCD_DrawRectangle(130,250,350,300);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 35;
}

// �Ӳ˵���ˮ��
void UI_260(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"ˮ������",32,220);

    LCD_DrawRectangle(130,250,350,300);

    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 36;
}

// �¶ȱ������� - ��1ҳ
void UI_300(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"��ǰ�¶�",32,220);
 //   Show_Str_Mid(130,284,"Ŀ���¶�",32,220);
	
	Show_Str_Mid(130,284,"Ŀ���¶�",32,220);
    char buf_target[16];
    sprintf(buf_target, "%d.%d", target_temp/10, target_temp%10);
    Show_Str(240,284,100,16,buf_target,16,0);
	
    Show_Str_Mid(130,359,"�¶�����",32,220);
    Show_Str_Mid(130,434,"�¶�У׼",32,220);
    Show_Str_Mid(130,509,"���ƫ��mqtt",32,220);
    Show_Str_Mid(130,584,"���ƫ��",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    // ��ҳ��ť
//    Show_Str(60,710,100,16,"��һҳ",16,0);
//    LCD_DrawRectangle(50,700,150,750);

    Show_Str(340,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(330,700,430,750);

    // ���ذ�ť
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 40; // ��1ҳ
}

// �¶ȱ������� - ��2ҳ
void UI_301(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"�������",32,220);
    LCD_DrawRectangle(130,200,350,250);

    // ��ҳ��ť
    Show_Str(60,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(50,700,150,750);

//    Show_Str(340,710,100,16,"��һҳ",16,0);
//    LCD_DrawRectangle(330,700,430,750);

    // ���ذ�ť
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 41; // ��2ҳ
}

void UI_310(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"Target Temp",32,220);
    char buf[16];
    sprintf(buf,"%d.%d", target_temp/10, target_temp%10);
    Show_Str_Mid(130,284,buf,32,220);

    Show_Str(150,340,40,16,"+",16,0);
    LCD_DrawRectangle(140,330,190,380);
    Show_Str(150,400,40,16,"-",16,0);
    LCD_DrawRectangle(140,390,190,440);

    Show_Str(260,340,40,16,"+",16,0);
    LCD_DrawRectangle(250,330,300,380);
    Show_Str(260,400,40,16,"-",16,0);
    LCD_DrawRectangle(250,390,300,440);

    Show_Str(25,23,200,16,"Back",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 42;
}



// ����״̬��ѯ ���˵�
void UI_400(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"������ʱ",32,220);
    Show_Str_Mid(130,284,"��������״̬",32,220);
    Show_Str_Mid(130,359,"����������״̬",32,220);
    Show_Str_Mid(130,434,"��������״̬",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 50;
}

void UI_410(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"�ϵ籨��",32,220);
    Show_Str_Mid(130,284,"��ѹ����",32,220);
    Show_Str_Mid(130,359,"��ѹ����",32,220);
    Show_Str_Mid(130,434,"ȱ�౨��",32,220);
    Show_Str_Mid(130,509,"���򱨾�",32,220);
    Show_Str_Mid(130,584,"�¶ȹ���",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    // ��ҳ��ť
    Show_Str(340,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(330,700,430,750);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 51;
}

void UI_411(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"���±���",32,220);
    Show_Str_Mid(130,284,"���±���",32,220);
    Show_Str_Mid(130,359,"ϵͳ����",32,220);
    Show_Str_Mid(130,434,"ϵͳ����",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);

    // ��ҳ��ť
    Show_Str(60,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(50,700,150,750);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 52;
}

void UI_420(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"������",32,220);
    Show_Str_Mid(130,284,"FutureX",32,220);
    Show_Str_Mid(130,359,"����",32,220);
    Show_Str_Mid(130,434,"�η�",32,220);
    Show_Str_Mid(130,509,"���ֻ�",32,220);
    Show_Str_Mid(130,584,"ˮ��",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 53;
}

void UI_421(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"���±���",32,220);
    Show_Str_Mid(130,284,"���±���",32,220);
    Show_Str_Mid(130,359,"��ʪ����",32,220);
    Show_Str_Mid(130,434,"��ʪ����",32,220);
    Show_Str_Mid(130,509,"CO2Ũ�ȸ�",32,220);
    Show_Str_Mid(130,584,"CO2Ũ�ȵ�",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    // ��ҳ��ť
    Show_Str(340,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(330,700,430,750);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 54;
}

void UI_422(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"NH3Ũ�ȸ�",32,220);
    Show_Str_Mid(130,284,"NH3Ũ�ȵ�",32,220);
    Show_Str_Mid(130,359,"�¶ȹ���",32,220);
    Show_Str_Mid(130,434,"ʪ�ȹ���",32,220);
    Show_Str_Mid(130,509,"CO2����",32,220);
    Show_Str_Mid(130,584,"NH3����",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    // ��ҳ��ť
    Show_Str(60,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(50,700,150,750);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 55;
}

void UI_430(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"�ⲿ����1",32,220);
    Show_Str_Mid(130,284,"�ⲿ����2",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);

    // ����
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 56;
}

// ������������
void UI_500(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,259,"�ⲿ����1",32,220);
    Show_Str_Mid(130,334,"�ⲿ����2",32,220);

    LCD_DrawRectangle(130,250,350,300);   
    LCD_DrawRectangle(130,325,350,375);   

    // ������һ��
    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40); 

    current_screen = 60;  // ���������
}

void UI_600(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"�豸��Ϣ����",32,220);
    Show_Str_Mid(130,284,"�豸��Ϣά��",32,220);
    Show_Str_Mid(130,359,"�ɼ���������",32,220);
    Show_Str_Mid(130,434,"������������",32,220);
    Show_Str_Mid(130,509,"mqttTest",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 70;
}
void UI_610(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"ʱ������",32,220);
    Show_Str_Mid(130,284,"�豸���",32,220);
    Show_Str_Mid(130,359,"��������",32,220);
    Show_Str_Mid(130,434,"������",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 71;
}

void UI_611(void) {
    if(!rtc_set_initialized){
        RTC_TimeTypeDef t;
        RTC_DateTypeDef d;
        HAL_RTC_GetTime(&RTC_Handler,&t,RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&RTC_Handler,&d,RTC_FORMAT_BIN);
        rtc_set_year = d.Year;
        rtc_set_month = d.Month;
        rtc_set_day = d.Date;
        rtc_set_hour = t.Hours;
        rtc_set_min = t.Minutes;
        rtc_set_sec = t.Seconds;
        rtc_set_initialized = 1;
    }
    LCD_Clear(WHITE);
    char buf[8];
    Show_Str(40,100,80,16,"Year",16,0);
    sprintf(buf,"%02d",rtc_set_year);
    Show_Str(150,100,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,90,260,120);
    Show_Str(235,93,80,16,"+",16,0);
    LCD_DrawRectangle(270,90,300,120);
    Show_Str(275,93,80,16,"-",16,0);
    Show_Str(40,160,80,16,"Mon",16,0);
    sprintf(buf,"%02d",rtc_set_month);
    Show_Str(150,160,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,150,260,180);
    Show_Str(235,153,80,16,"+",16,0);
    LCD_DrawRectangle(270,150,300,180);
    Show_Str(275,153,80,16,"-",16,0);
    Show_Str(40,220,80,16,"Day",16,0);
    sprintf(buf,"%02d",rtc_set_day);
    Show_Str(150,220,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,210,260,240);
    Show_Str(235,213,80,16,"+",16,0);
    LCD_DrawRectangle(270,210,300,240);
    Show_Str(275,213,80,16,"-",16,0);
    Show_Str(40,280,80,16,"Hour",16,0);
    sprintf(buf,"%02d",rtc_set_hour);
    Show_Str(150,280,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,270,260,300);
    Show_Str(235,273,80,16,"+",16,0);
    LCD_DrawRectangle(270,270,300,300);
    Show_Str(275,273,80,16,"-",16,0);
    Show_Str(40,340,80,16,"Min",16,0);
    sprintf(buf,"%02d",rtc_set_min);
    Show_Str(150,340,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,330,260,360);
    Show_Str(235,333,80,16,"+",16,0);
    LCD_DrawRectangle(270,330,300,360);
    Show_Str(275,333,80,16,"-",16,0);
    Show_Str(40,400,80,16,"Sec",16,0);
    sprintf(buf,"%02d",rtc_set_sec);
    Show_Str(150,400,80,16,(uint8_t*)buf,16,0);
    LCD_DrawRectangle(230,390,260,420);
    Show_Str(235,393,80,16,"+",16,0);
    LCD_DrawRectangle(270,390,300,420);
    Show_Str(275,393,80,16,"-",16,0);
    Show_Str_Mid(130,459,"OK",32,220);
    LCD_DrawRectangle(130,450,350,500);
    Show_Str(25,23,200,16,"Back",16,0);
    LCD_DrawRectangle(20,20,110,40);
    current_screen = 78;
}


void UI_620(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"��ص�ѹ��ѯ",32,220);
    Show_Str_Mid(130,284,"�¶ȼ��",32,220);
    Show_Str_Mid(130,359,"��������",32,220);
    Show_Str_Mid(130,434,"��ʱ��������",32,220);
    Show_Str_Mid(130,509,"��������ģʽ",32,220);
    Show_Str_Mid(130,584,"�ָ���������",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);
    LCD_DrawRectangle(130,500,350,550);
    LCD_DrawRectangle(130,575,350,625);

    Show_Str(340,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(330,700,430,750);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 72;
}
void UI_621(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"����",32,220);
    Show_Str_Mid(130,284,"����汾",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);

    Show_Str(60,710,100,16,"��һҳ",16,0);
    LCD_DrawRectangle(50,700,150,750);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 73;
}
void UI_630(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"��������",32,220);
    Show_Str_Mid(130,284,"�������",32,220);
    Show_Str_Mid(130,359,"����ʱ������",32,220);
    Show_Str_Mid(130,434,"������ʱ",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);
    LCD_DrawRectangle(130,425,350,475);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 74;
}
void UI_640(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"������",32,220);

    LCD_DrawRectangle(130,200,350,250);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 75;
}
void UI_650(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"�����ϱ�",32,220);
    Show_Str_Mid(130,284,"������",32,220);
    Show_Str_Mid(130,359,"�ϱ�����",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 76;
}
void UI_660(void) {
    LCD_Clear(WHITE);

    Show_Str_Mid(130,209,"mqttdebug",32,220);
    Show_Str_Mid(130,284,"send",32,220);
    Show_Str_Mid(130,359,"00000",32,220);

    LCD_DrawRectangle(130,200,350,250);
    LCD_DrawRectangle(130,275,350,325);
    LCD_DrawRectangle(130,350,350,400);

    Show_Str(25,23,200,16,"������һ��",16,0);	
    LCD_DrawRectangle(20,20,110,40);

    current_screen = 77;
}

static void UI_ESP8266_Send(const char *cmd) {
    u16 len=0;
    memset(esp8266_resp,0,sizeof(esp8266_resp));
    ESP8266_SendCmd((char*)cmd,"OK",2000);
    ESP8266_Receive_Data((u8*)esp8266_resp,&len);
    esp8266_resp[len]=0;
    LCD_Fill(25,520,455,560,WHITE);
    Show_Str(25,520,430,16,(u8*)esp8266_resp,16,0);
}

void UI_ESP8266(void) {
    LCD_Clear(WHITE);
    Show_Str_Mid(130,209,"ESP8266",32,220);
    Show_Str_Mid(130,259,"AT",32,220);
    Show_Str_Mid(130,334,"AT+RST",32,220);
    Show_Str_Mid(130,409,"AT+GMR",32,220);
    LCD_DrawRectangle(130,250,350,300);
    LCD_DrawRectangle(130,325,350,375);
    LCD_DrawRectangle(130,400,350,450);
    Show_Str(25,23,200,16,"������һ��",16,0);
    LCD_DrawRectangle(20,20,110,40);
    Show_Str(25,480,200,16,"Resp:",16,0);
    current_screen = 80;
}

void UI_TouchHandler(u16 x, u16 y){
LCD_GetCursor(&x, &y);
    switch (current_screen)
    {
/* -------------------------------- ����������� --------------------------------*/
/* -------------------------------- ����������� --------------------------------*/
    case 20: // �Ӳ˵�1
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            current_screen = 21;
            UI_110(); // �����״̬�鿴
        }
		else if (x > 130 && x < 350 && y > 325 && y < 375) {
            current_screen = 22;
            UI_120(); // ������ѹ�鿴
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            current_screen = 23;
            UI_130(); // �������ֵ����
        }
        else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 0;
            UI_Main();
        } 
		
        break;
		
/* -------------------------------- �����״̬�鿴 --------------------------------*/
    case 21: // �����״̬�鿴 (UI_110)
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // �Ƿ�ϵ�
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // ������
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            // ȱ����
        }
        else if (x > 130 && x < 350 && y > 475 && y < 525) {
            current_screen = 22;
            UI_111(); // ��ߵ�ѹ
        }
        else if (x > 130 && x < 350 && y > 550 && y < 600) {
            current_screen = 23;
            UI_112(); // ��͵�ѹ
        }
		else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 20;
            UI_100();
        } 
        break;

    case 22: // ��ߵ�ѹ (UI_111)
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // A��
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // B��
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            // C��
        }
		else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 21;
            UI_110();
		}
        break;

    case 23: // ��͵�ѹ (UI_112)
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // A��
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // B��
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            // C��
        }
		else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 21;
            UI_110();
		}
        break;

/* -------------------------------- ������ѹ�鿴 --------------------------------*/
    case 24: // ������ѹ�鿴 (UI_120)
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // A��
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // B��
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            // C��
        }
		else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 20;
            UI_100();
		}
        break;

/* -------------------------------- �������ֵ�鿴 --------------------------------*/
    case 25: // �������ֵ���� (UI_130)
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // ��ߵ�ѹ����
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // ��͵�ѹ����
        }
		else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 20;
            UI_100();
		}
        break;
		
/* -------------------------------- �ɼ��豸���� --------------------------------*/
/* -------------------------------- �ɼ��豸���� --------------------------------*/
	case 30: // ����˵� UI_200
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            current_screen = 31;
            UI_210(); // ������
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            current_screen = 32;
            UI_220(); // FutureX
        }
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            current_screen = 33;
            UI_230(); // ����
        }
        else if (x > 130 && x < 350 && y > 475 && y < 525) {
            current_screen = 34;
            UI_240(); // �η�
        }
        else if (x > 130 && x < 350 && y > 550 && y < 600) {
            current_screen = 35;
            UI_250(); // ���ֻ�
        }
        else if (x > 130 && x < 350 && y > 625 && y < 675) {
            current_screen = 36;
            UI_260(); // ˮ��
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 0;
            UI_Main(); // ����������
        }
        break;

/* -------------------------------- ������ --------------------------------*/
    case 31: // UI_210
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // ״̬����
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // ��ַ����
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200(); // ���زɼ��豸����
        }
        break;

/* -------------------------------- FutureX --------------------------------*/
    case 32: // UI_220
        if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200();
        }
        break;

/* -------------------------------- ���� --------------------------------*/
    case 33: // UI_230
        if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200();
        }
        break;

/* -------------------------------- �η� --------------------------------*/
    case 34: // UI_240
        if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200();
        }
        break;

/* -------------------------------- ���ֻ� --------------------------------*/
    case 35: // UI_250
        if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200();
        }
        break;

/* -------------------------------- ˮ�� --------------------------------*/
    case 36: // UI_260
        if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 30;
            UI_200();
        }
        break;
		
/* -------------------------------- �¶ȱ������� --------------------------------*/
/* ----------------------------- �¶ȱ������õ�һҳ------------------------------*/
    case 40: // UI_300
        if (x > 130 && x < 350 && y > 200 && y < 250) {
            // ��ǰ�¶�
        }
        else if (x > 130 && x < 350 && y > 275 && y < 325) {
			current_screen = 42;
            UI_310();
            // Ŀ���¶�
        }
        else if (x > 130 && x < 350 && y > 350 && y < 400) {
            // �¶�����
        }
        else if (x > 130 && x < 350 && y > 425 && y < 475) {
            // �¶�У׼
        }
        else if (x > 130 && x < 350 && y > 500 && y < 550) {
            // ���ƫ��
        }
        else if (x > 130 && x < 350 && y > 575 && y < 625) {
            // ���ƫ��
        }
        // ��ҳ
        else if (x > 330 && x < 430 && y > 700 && y < 750) {
            current_screen = 41;
            UI_301(); // ��һҳ
        }
        // ����
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 20;
            UI_Main(); // ������һ��
        }
        break;

/* ----------------------------- �¶ȱ������õڶ�ҳ------------------------------*/
    case 41: // UI_301
        if (x > 130 && x < 350 && y > 200 && y < 250) {
            // �������
        }
        // ��ҳ
        else if (x > 50 && x < 150 && y > 700 && y < 750) {
            current_screen = 40;
            UI_300(); // ��һҳ
        }

        // ����
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 20;
            UI_Main(); // ������һ��
        }
        break;
		
		
    case 42: // UI_310
        if (x > 140 && x < 190 && y > 330 && y < 380) {
            target_temp += 10;
            UI_310();
        }
        else if (x > 140 && x < 190 && y > 390 && y < 440) {
            if(target_temp >= 10) target_temp -= 10;
            UI_310();
        }
        else if (x > 250 && x < 300 && y > 330 && y < 380) {
            if((target_temp %10) < 9) target_temp += 1;
            UI_310();
        }
        else if (x > 250 && x < 300 && y > 390 && y < 440) {
            if((target_temp %10) > 0) target_temp -= 1;
            UI_310();
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 40;
            UI_300();
        }
        break;
		
/* -------------------------------- ����״̬��ѯ---------------------------------*/
/* -------------------------------- ����״̬��ѯ---------------------------------*/
    case 50: // UI_400
        if (x > 130 && x < 350 && y > 200 && y < 250) {
            // ������ʱ
        }
        else if (x > 130 && x < 350 && y > 275 && y < 325) {
            current_screen = 51; UI_410(); // ��������״̬
        }
        else if (x > 130 && x < 350 && y > 350 && y < 400) {
            current_screen = 53; UI_420(); // ����������״̬
        }
        else if (x > 130 && x < 350 && y > 425 && y < 475) {
            current_screen = 56; UI_430(); // ��������״̬
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 0; UI_Main(); // ����
        }
        break;

/* -------------------------------- ��������״̬1---------------------------------*/
    case 51: // UI_410
        if (x > 330 && x < 430 && y > 700 && y < 750) {
            current_screen = 52; UI_411(); // ��һҳ
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 50; UI_400(); // ����
        }
        break;
/* -------------------------------- ��������״̬2---------------------------------*/
    case 52: // UI_411
        if (x > 50 && x < 150 && y > 700 && y < 750) {
            current_screen = 51; UI_410(); // ��һҳ
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 50; UI_400(); // ����
        }
        break;

/* -------------------------------- ����������״̬---------------------------------*/
    case 53: // UI_420
        if (x > 130 && x < 350 && y > 200 && y < 250) {
            current_screen = 54; UI_421(); // �������Ӳ˵�
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 50; UI_400(); // ����
        }
        break;
/* -------------------------------- ������1---------------------------------*/
    case 54: // UI_421
        if (x > 330 && x < 430 && y > 700 && y < 750) {
            current_screen = 55; UI_422(); // ��һҳ
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 53; UI_420(); // ����
        }
        break;
/* -------------------------------- ������2---------------------------------*/
    case 55: // UI_422
        if (x > 50 && x < 150 && y > 700 && y < 750) {
            current_screen = 54; UI_421(); // ��һҳ
        }
        else if (x > 0 && x < 80 && y > 0 && y < 80) {
            current_screen = 53; UI_420(); // ����
        }
        break;
/* -------------------------------- ��������״̬---------------------------------*/
    case 56: // UI_430
        if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 50; UI_400(); // ����
        }
        break;
/* -------------------------------- ������������---------------------------------*/
    case 60: // UI_500
        if (x > 130 && x < 350 && y > 250 && y < 300) {
            // �ⲿ����1 ����
        }
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            // �ⲿ����2 ����
        }
        else if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 0; // �������˵�
            UI_Main();
        }
        break;
/* -------------------------------- ϵͳά������---------------------------------*/
/* -------------------------------- ϵͳά������---------------------------------*/
    case 70: // UI_600
        if (x > 130 && x < 350 && y > 200 && y < 250) { current_screen = 71; UI_610(); }
        else if (x > 130 && x < 350 && y > 275 && y < 325) { current_screen = 72; UI_620(); }
        else if (x > 130 && x < 350 && y > 350 && y < 400) { current_screen = 75; UI_640(); }
        else if (x > 130 && x < 350 && y > 425 && y < 475) { current_screen = 76; UI_650(); }
        else if (x > 130 && x < 350 && y > 500 && y < 550) { current_screen = 77; UI_660(); }
        else if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 0; UI_Main(); }
        break;

    // -------- �豸��Ϣ���� --------
    case 71:
        if (x > 130 && x < 350 && y > 200 && y < 250) { rtc_set_initialized = 0; current_screen = 78; UI_611(); }
        else if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); }
        break;
    // -------- �豸��Ϣά�� --------
    case 72: // UI_620
        if (x > 330 && x < 430 && y > 700 && y < 750) { current_screen = 73; UI_621(); }
        else if (x > 130 && x < 350 && y > 425 && y < 475) { current_screen = 74; UI_630(); } // ��ʱ��������
        else if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); }
        break;

    case 73: // UI_621
        if (x > 50 && x < 150 && y > 700 && y < 750) { current_screen = 72; UI_620(); }
        else if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); }
        break;

    // -------- ��ʱ�������� --------
    case 74: if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 72; UI_620(); } break;

    // -------- �ɼ��������� --------
    case 75: if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); } break;

    // -------- ������������ --------
    case 76: if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); } break;

    // -------- �����޸� --------
    case 77:
		        if (x > 130 && x < 350 && y > 275 && y < 325) { ESP8266_MQTT_Debug(); }
        else if (x < 80 && x>0 && y < 80 && y > 0) { current_screen = 70; UI_600(); }
        break;
		
     case 78:
        if (x < 80 && x>0 && y < 80 && y > 0) { rtc_set_initialized = 0; current_screen = 71; UI_610(); }
        else if (x > 230 && x < 260 && y > 90 && y < 120) { rtc_set_year = (rtc_set_year + 1) % 100; UI_611(); }
        else if (x > 270 && x < 300 && y > 90 && y < 120) { if (rtc_set_year > 0) rtc_set_year--; else rtc_set_year = 99; UI_611(); }
        else if (x > 230 && x < 260 && y > 150 && y < 180) { rtc_set_month++; if (rtc_set_month > 12) rtc_set_month = 1; UI_611(); }
        else if (x > 270 && x < 300 && y > 150 && y < 180) { if (rtc_set_month > 1) rtc_set_month--; else rtc_set_month = 12; UI_611(); }
        else if (x > 230 && x < 260 && y > 210 && y < 240) { rtc_set_day++; if (rtc_set_day > 31) rtc_set_day = 1; UI_611(); }
        else if (x > 270 && x < 300 && y > 210 && y < 240) { if (rtc_set_day > 1) rtc_set_day--; else rtc_set_day = 31; UI_611(); }
        else if (x > 230 && x < 260 && y > 270 && y < 300) { rtc_set_hour++; if (rtc_set_hour > 23) rtc_set_hour = 0; UI_611(); }
        else if (x > 270 && x < 300 && y > 270 && y < 300) { if (rtc_set_hour > 0) rtc_set_hour--; else rtc_set_hour = 23; UI_611(); }
        else if (x > 230 && x < 260 && y > 330 && y < 360) { rtc_set_min++; if (rtc_set_min > 59) rtc_set_min = 0; UI_611(); }
        else if (x > 270 && x < 300 && y > 330 && y < 360) { if (rtc_set_min > 0) rtc_set_min--; else rtc_set_min = 59; UI_611(); }
        else if (x > 230 && x < 260 && y > 390 && y < 420) { rtc_set_sec++; if (rtc_set_sec > 59) rtc_set_sec = 0; UI_611(); }
        else if (x > 270 && x < 300 && y > 390 && y < 420) { if (rtc_set_sec > 0) rtc_set_sec--; else rtc_set_sec = 59; UI_611(); }
        else if (x > 130 && x < 350 && y > 450 && y < 500) {
            RTC_Set_Date(rtc_set_year, rtc_set_month, rtc_set_day, 0);
            RTC_Set_Time(rtc_set_hour, rtc_set_min, rtc_set_sec, RTC_HOURFORMAT12_AM);
            rtc_set_initialized = 0;
            current_screen = 71;
            UI_610();
        }
        break;

	case 80: // ESP8266 AT
		if (x > 130 && x < 350 && y > 250 && y < 300) {
			UI_ESP8266_Send("AT\r\n");
		} else if (x > 130 && x < 350 && y > 325 && y < 375) {
			UI_ESP8266_Send("AT+RST\r\n");
		} else if (x > 130 && x < 350 && y > 400 && y < 450) {
			UI_ESP8266_Send("AT+GMR\r\n");
		} else if (x < 80 && x>0 && y < 80 && y > 0) {
			current_screen = 0;
			UI_Main();
		}
	break;

		
	case 0: // ���˵�


        if (x > 130 && x < 350 && y > 250 && y < 300) {

            UI_100(); 
        } 
        else if (x > 130 && x < 350 && y > 325 && y < 375) {
            
            UI_200();
        } 
        else if (x > 130 && x < 350 && y > 400 && y < 450) {
            
            UI_300();
        }
		else if (x > 130 && x < 350 && y > 475 && y < 525) {
            
            UI_400();
        } 
        else if (x > 130 && x < 350 && y > 550 && y < 600) {
            
            UI_ESP8266();
        }
		else if (x > 130 && x < 350 && y > 600 && y < 675) {
            
            UI_600();
        } 

        break;
		
		
    default:
        if (x < 80 && x>0 && y < 80 && y > 0) {
            current_screen = 0;
            UI_Main(); // �������˵�
        }
        break;
    }
}

void UI_DATA_Show(u16 x, u16 y){
//LCD_GetCursor(&x, &y);
	    switch (current_screen)
    {
	case 0:
		{
		UI_Main_show();
		}

	}


}

void UI_Init(void) {
    current_screen = 0;
    LCD_Clear(WHITE);
	UI_Main();
	UI_Main_show();
	
}







