/**
  ******************************************************************************
  * @File name      : boot_config.h
  * @Description    : 
  * @Author         : lzk
  * Version         : 1.0
  * Date            : 2024/5/11
  ******************************************************************************
  */


#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__


#define RBOOT_APP_ADDR             	(0x8040000)					/*app起始地址*/
#define CRC32_INIT_VAL              (0xFFFFFFFF)
#define RBOOT_SHELL_KEY_CHK         (3)							/*等待时间3s*/

#define RTU_OTA_FILE_PATH   		("/ota/bec_app.rbl")		/*待升级固件*/
#define RTU_BACK_FILE_PATH  		("/ota/back.rbl")			/*升级固件的备份*/
#define RTU_LAST_FILE_PATH  		("/ota/last.rbl")			/*上个版本的固件*/

#endif //
