/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-04     armink       the first version
 */

#include <rthw.h>
#include <ulog.h>
#include <rtdevice.h>

#ifdef ULOG_BACKEND_USING_CONSOLE

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend console = { 0 };
#ifdef ULOG_CONSOLE_DMATX
static struct rt_completion completion;
#endif

void ulog_console_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, rt_size_t len)
{
#ifdef RT_USING_DEVICE
    rt_device_t dev = rt_console_get_device();

    if (dev == RT_NULL){
        rt_hw_console_output(log);
    }
    else{
        rt_device_write(dev, 0, log, len);
	#ifdef ULOG_CONSOLE_DMATX
		rt_completion_wait(&completion, 0x00ff);
	#endif
    }
#else
    rt_hw_console_output(log);
#endif
}

#ifdef ULOG_CONSOLE_DMATX
static rt_err_t drv_serial_tx_complete(rt_device_t dev, void *buffer){
    rt_completion_done(&completion);
    return RT_EOK;
}
#endif


int ulog_console_backend_init(void)
{
#ifdef ULOG_CONSOLE_DMATX /*¿ªÆôdebug´®¿ÚµÄDMA*/
	rt_device_t dev = rt_console_get_device();
	if(dev != RT_NULL){
	#ifdef RT_USING_FINSH
		finsh_set_device(dev->parent.name);
	#endif
		while(RT_EOK == rt_device_close(dev));
		rt_device_open(dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX |RT_DEVICE_FLAG_DMA_TX);
		rt_completion_init(&completion);
		rt_device_set_tx_complete(dev,drv_serial_tx_complete);
	}
#endif
    ulog_init();
    console.output = ulog_console_backend_output;

    ulog_backend_register(&console, "console", RT_TRUE);

    return 0;
}
INIT_APP_EXPORT(ulog_console_backend_init);

#ifdef  ULOG_CONSOLE_DMATX
int rt_kprintf(const char *fmt, ...)
{
    va_list args;
    rt_size_t length = 0;
    static char rt_log_buf[RT_CONSOLEBUF_SIZE];
	static char fmt_temp[RT_CONSOLEBUF_SIZE+48];
	uint16_t str_len = rt_strlen(fmt) + 1;
	if(str_len > RT_CONSOLEBUF_SIZE) str_len = RT_CONSOLEBUF_SIZE;
	if(ulog_init_get()){
		for(uint16_t i=0;i<str_len;i++){
			if(fmt[i] == '\n'){
				fmt_temp[length++] = '\r';
				if(length == (sizeof(fmt_temp) -1)) break;
			}
			fmt_temp[length++] = fmt[i];
		}
		fmt = (const char*)fmt_temp;
	}
	
    va_start(args,fmt);
    length = rt_vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, fmt, args);
    if (length > sizeof(rt_log_buf) - 1)
    {
        length = sizeof(rt_log_buf) - 1;
    }
	if(ulog_init_get()){
		ulog_raw("%s",rt_log_buf);
	}else{
		 rt_device_write(rt_console_get_device(), 0, rt_log_buf, length);
	}
    va_end(args);

    return length;
}
RTM_EXPORT(rt_kprintf);
#endif

#endif /* ULOG_BACKEND_USING_CONSOLE */
