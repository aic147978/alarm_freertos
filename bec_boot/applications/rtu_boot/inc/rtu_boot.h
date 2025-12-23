/**
  ******************************************************************************
  * @File name      : rtu_boot.h
  * @Description    : 
  * @Author         : lzk
  * Version         : 1.0
  * Date            : 2024/5/11
  ******************************************************************************
  */


#ifndef __RTU_BOOT_H__
#define __RTU_BOOT_H__

#include <stdio.h>

void rtu_boot_startup(void);
void rb_jump_to_app(void);

static int8_t shell_key_check(void);
static void rb_fw_update(void);
#endif /* __RTU_BOOT_H__ */

