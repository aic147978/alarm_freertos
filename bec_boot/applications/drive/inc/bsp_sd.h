/**
  ******************************************************************************
  * @File name      : bsp_sd.h
  * @Description    : 
  * @Author         : lzk
  * Version         : 1.0
  * Date            : 2024/5/11
  ******************************************************************************
  */


#ifndef __BSP_SD_H__
#define __BSP_SD_H__

#include "rtthread.h"
#include <fcntl.h>


#define SD_DEVICE_NAME      ("sd")  /*sd 设备名称*/
#define SD_MOUNT_PATH       ("/")   /*sd 设备挂载路径*/
#define SD_FILE_SYSTEM      ("elm") /*sd 使用的文件系统*/
#define SD_WAIT_TIMER		(3000)	/*等待sd注册成功*/

rt_err_t bsp_sd_mount(void);
void bsp_sd_unmount(void);
void bsp_sd_format(void);
rt_err_t bsp_sd_space_info(uint64_t *totalSize,uint64_t *freeSize);
int8_t copy_file(const char *src_path, const char *dest_path);
int8_t dir_create(char *dir_path);
int delete_directory(const char *path);
void file_open(int *fd,char* filename,int flags);
void file_close(int *fd);
int file_write(int *fd,char* buff, uint16_t size);
int file_read(int *fd,char *buff,uint16_t size);
int file_delete(char *filepath);
time_t gather_file_fetch(char *dir_name);
int file_rename(char *old_name,char *new_name);
off_t get_file_size(char *file_name);
int check_fd_validity(int *fd);
#endif //
