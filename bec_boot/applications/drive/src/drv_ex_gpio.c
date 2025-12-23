#include "drv_ex_gpio.h"
#include "74hc595.h"
#include "rtdevice.h"

static rt_device_t s_dev;
static uint32_t s_last_status;
void drv_ex_gpio_init(void){
    s_dev = rt_device_find("74hc595");
    if(s_dev == RT_NULL){
        rt_kprintf("74hc595 device not found!\n");
    }
    rt_device_open(s_dev, RT_DEVICE_FLAG_RDWR);
    s_last_status = 1;
    drv_ex_gpio_set_level(0xffffffff, 0);
}

void drv_ex_gpio_set_level(uint32_t ex_pin, uint8_t value){
    uint32_t status = s_last_status;
    for(uint8_t i = 0; i< 32; i++){
        if(ex_pin == 0) break;
        if(ex_pin & (1<<i)){
            if(1 == value){
                status |= (1<<(31-i));
            }else{
                status &= ~(1<<(31-i));
            }
            ex_pin &= ~(1<<i);
        }
    }
    if(status != s_last_status){
		s_last_status = status;
        rt_device_write(s_dev, 0, &s_last_status, 4);
    }
}
void drv_ex_gpio_status_refresh(void){
    rt_device_read(s_dev, 0, &s_last_status, 4);
}
