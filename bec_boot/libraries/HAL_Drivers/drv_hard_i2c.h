/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-20     cheung       first version
 */

#ifndef __DRV_I2C__
#define __DRV_I2C__

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <board.h>

#ifdef BSP_USING_HI2C



/* stm32 config class */
typedef void (*pI2CInit)(void);
typedef void (*pI2CIrqInit)(void);
struct stm32_hard_i2c_config
{
	const char *bus_name;
    rt_base_t scl_pin;
    rt_base_t sda_pin;
    pI2CInit   pI2CFunc;
	pI2CIrqInit pI2CIrqFunc;
    I2C_HandleTypeDef *pHi2c;
    struct rt_i2c_bus_device i2c_bus;
	
	/* notice and lock define */
    rt_sem_t tx_notice;
	rt_sem_t rx_notice;
    rt_mutex_t lock;
};

int rt_hw_hardi2c_init(void);

#endif

#endif /* RT_USING_I2C */
