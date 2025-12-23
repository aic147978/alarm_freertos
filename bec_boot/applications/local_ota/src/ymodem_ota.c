/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-30     armink       the first version
 * 2018-08-27     Murphy       update log
 */
#include <rtthread.h>
#include <ymodem.h>
#include <stdlib.h>
#include "drive_inc.h"

#define DOWNLOAD_DIR_PATH					("/ota")
#define DEFAULT_DOWNLOAD_FILE	 	("bec_app.rbl")


static size_t update_file_cur_size,update_file_total_size;
static int s_fd;
static char file_path[128];

static enum rym_code ymodem_on_begin(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    char *file_name, *file_size;
    /* calculate and store file size */
    file_name = (char *)&buf[0];
    file_size = (char *)&buf[rt_strlen(file_name) + 1];
	update_file_total_size = atol(file_size);
    update_file_cur_size = 0;
    return RYM_CODE_ACK;
}

static enum rym_code ymodem_on_data(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    /* write data of application to DL partition  */
	if(update_file_total_size < len) len = update_file_total_size;
	uint8_t try_num = 3;
	_try:
	if(file_write(&s_fd,(char*)buf,len) < 0){
		if((--try_num) > 0){
			file_open(&s_fd,file_path,O_APPEND|O_WRONLY);
			goto _try;
		}
		return RYM_CODE_CAN;
	}
    update_file_cur_size += len;
	update_file_total_size -= len;
    return RYM_CODE_ACK;
}


static void ymodem_ota(uint8_t argc, char **argv)
{
    struct rym_ctx rctx;
    int i;
    char* recv_file = DEFAULT_DOWNLOAD_FILE;
    rt_device_t dev = rt_console_get_device();
   
    for (i=1; i<argc;)
    {
        /* change default partition to save firmware */
        if (!strcmp(argv[i], "-p"))
        {
            if (argc <= (i+1))
            {
				goto _help;
            }
            recv_file = argv[i+1];
            i += 2;
        }else{/* NOT supply parameter */
			_help:
			rt_kprintf("ymodem_ota -p 1.txt： 设置存储文件名。\n");
            return;
        }
    }
    dir_create(DOWNLOAD_DIR_PATH);
	rt_sprintf(file_path,"%s/%s",DOWNLOAD_DIR_PATH,recv_file);
    file_delete(file_path);
	file_open(&s_fd,file_path,O_APPEND|O_CREAT|O_RDWR);
	if(s_fd < 0){
		rt_kprintf("创建文件（%s）失败!!!\n",file_path);
		return ;
	}
    rt_kprintf("关闭数据采集");
	rt_kprintf("存储文件为：%s，使用的串口为：%s。\n",file_path,dev->parent.name);
    rt_kprintf("警告:Ymodem已启动!该操作将无法恢复。\n");
    rt_kprintf("请选择ota固件文件并使用Ymodem发送。\n");
	rt_thread_delay(200);
    if (!rym_recv_on_device(&rctx, dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                            ymodem_on_begin, ymodem_on_data, NULL, RT_TICK_PER_SECOND))
    {
		file_close(&s_fd);
		rt_kprintf("\n");
        rt_kprintf("下载固件成功。\n");
        /* wait some time for terminal response finish */
        rt_thread_delay(rt_tick_from_millisecond(200));
		extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
    }
    else
    {
		file_close(&s_fd);
        file_delete(file_path);
        /* wait some time for terminal response finish */
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_kprintf("升级固件失败！！！");
    }
}
/**
 * msh />ymodem_ota
*/
MSH_CMD_EXPORT(ymodem_ota, Use Y-MODEM to download the firmware);



