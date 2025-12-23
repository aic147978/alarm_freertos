/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "bsp_sd.h"
#include "rtu_boot.h"
#include "drv_ex_gpio.h"

int main(void)
{
	drv_ex_gpio_init();
	if(RT_ERROR == bsp_sd_mount()){
		/*进行app程序跳转*/
		rb_jump_to_app();
        return RT_ERROR;
    }
    rtu_boot_startup();
    return RT_EOK;
}
