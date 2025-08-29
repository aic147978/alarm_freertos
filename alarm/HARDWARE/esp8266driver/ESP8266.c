#include "esp8266.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "led.h"

UART_HandleTypeDef ESP8266_UART_Handler;    //USART3

#if EN_USART3_RX
u8 ESP8266_RX_BUF[512];
u16 ESP8266_RX_CNT=0;

void USART3_IRQHandler(void)
{
    u8 res;
    if((__HAL_UART_GET_FLAG(&ESP8266_UART_Handler,UART_FLAG_RXNE)!=RESET))
    {
        HAL_UART_Receive(&ESP8266_UART_Handler,&res,1,1000);
        if(ESP8266_RX_CNT<sizeof(ESP8266_RX_BUF))
        {
            ESP8266_RX_BUF[ESP8266_RX_CNT]=res;
            ESP8266_RX_CNT++;
        }
    }
}
#endif

static void ESP8266_Clear(void)
{
    ESP8266_RX_CNT=0;
    memset(ESP8266_RX_BUF,0,sizeof(ESP8266_RX_BUF));
}

//USART3初始化
void ESP8266_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    GPIO_Initure.Pin=GPIO_PIN_10|GPIO_PIN_11;    //PC10 PC11
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;
    GPIO_Initure.Pull=GPIO_PULLUP;
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;
    GPIO_Initure.Alternate=GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    ESP8266_UART_Handler.Instance=USART3;
    ESP8266_UART_Handler.Init.BaudRate=bound;
    ESP8266_UART_Handler.Init.WordLength=UART_WORDLENGTH_8B;
    ESP8266_UART_Handler.Init.StopBits=UART_STOPBITS_1;
    ESP8266_UART_Handler.Init.Parity=UART_PARITY_NONE;
    ESP8266_UART_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;
    ESP8266_UART_Handler.Init.Mode=UART_MODE_TX_RX;
    HAL_UART_Init(&ESP8266_UART_Handler);

    __HAL_UART_DISABLE_IT(&ESP8266_UART_Handler,UART_IT_TC);
#if EN_USART3_RX
    __HAL_UART_ENABLE_IT(&ESP8266_UART_Handler,UART_IT_RXNE);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_SetPriority(USART3_IRQn,3,3);
#endif
}

u8 ESP8266_SendCmd(char *cmd,char *ack,u16 waittime)
{
    u8 res=0;
    ESP8266_Clear();
    HAL_UART_Transmit(&ESP8266_UART_Handler,(u8*)cmd,strlen(cmd),1000);
    while(waittime--)
    {
        delay_ms(1);
        if(ack && strstr((char*)ESP8266_RX_BUF,ack))
        {
            res=0;break;
        }
    }
    if(waittime==0)res=1;
    return res;
}

u8 ESP8266_JoinAP(char *ssid,char *pwd)
{
    char cmd[120];
    sprintf(cmd,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pwd);
    return ESP8266_SendCmd(cmd,"WIFI GOT IP",5000);
}

u8 ESP8266_StartTCP(char *ip,char *port)
{
    char cmd[60];
    sprintf(cmd,"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",ip,port);
    return ESP8266_SendCmd(cmd,"CONNECT",4000);
}

u8 ESP8266_SendData(char *data)
{
    char cmd[32];
    sprintf(cmd,"AT+CIPSEND=%d\r\n",strlen(data));
    if(ESP8266_SendCmd(cmd,">",2000))return 1;
    return ESP8266_SendCmd(data,"SEND OK",4000);
}

void ESP8266_Receive_Data(u8 *buf,u16 *len)
{
    u16 rxlen=ESP8266_RX_CNT;
    *len=0;
    delay_ms(10);
    if(rxlen==ESP8266_RX_CNT && rxlen)
    {
        memcpy(buf,ESP8266_RX_BUF,rxlen);
        *len=rxlen;
        ESP8266_RX_CNT=0;
    }
}

void ESP8266_MQTT_Debug(void)
{
    ESP8266_SendCmd("AT+MQTTUSERCFG=0,1,\"aic147978\",\"admin\",\"admin123\",0,0,\"\"\r\n","OK",2000);
    ESP8266_SendCmd("AT+MQTTCONN=0,\"broker.emqx.io\",1883,1\r\n","OK",4000);
    ESP8266_SendCmd("AT+MQTTSUB=0,\"esp/test1\",1\r\n","OK",4000);
    ESP8266_SendCmd("AT+MQTTPUB=0,\"esp/test1\",\"hello world\",1,0\r\n","OK",4000);
}

void ESP8266_MQTT_LED_Setup(void)
{
    ESP8266_SendCmd("AT+MQTTUSERCFG=0,1,\"aic147978\",\"admin\",\"admin123\",0,0,\"\"\r\n","OK",2000);
    ESP8266_SendCmd("AT+MQTTCONN=0,\"broker.emqx.io\",1883,1\r\n","OK",4000);
    ESP8266_SendCmd("AT+MQTTSUB=0,\"esp/test1\",1\r\n","OK",4000);
}

void ESP8266_MQTT_LED_Process(void)
{
    u8 buf[512];
    u16 len;
    ESP8266_Receive_Data(buf, &len);
    if(len)
    {
        buf[len] = 0;  // 末尾加 '\0'，变成 C 字符串
        char *payload = NULL;

        // 找到第三个逗号（payload 起始位置）
		char *p = (char *)buf;
        int comma_count = 0;
        while(*p && comma_count < 3)
        {
            if(*p == ',') comma_count++;
            p++;
        }
        if(comma_count == 3)
        {
            payload = p;  // payload 起始
        }

        if(payload)
        {
            if(strncmp(payload, "lightup", 7) == 0)
            {
                LED1 = 0;
            }
            else if(strncmp(payload, "lightdown", 9) == 0)
            {
                LED1 = 1;
            }
            else if(strncmp(payload, "blink", 5) == 0)   // 你可以继续扩展命令
            {
                for(int i=0; i<5; i++){
                    LED1 = 0; delay_ms(200);
                    LED1 = 1; delay_ms(200);
                }
            }
        }
    }
}



