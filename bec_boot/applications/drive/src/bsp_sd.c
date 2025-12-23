/**
  ******************************************************************************
  * @File name      : bsp_sd.c
  * @Description    : 
  * @Author         : lzk
  * Version         : 1.0
  * Date            : 2024/5/11
  ******************************************************************************
  */


#include "bsp_sd.h"
#include "dfs.h"
#include "dfs_fs.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h> // 对于 close 函数


#define DRV_DEBUG						/*开启日志打印*/
#define LOG_TAG             "file"
#include <drv_log.h>

#define ERROR_TYR_NUM       6 /*出现错误尝试的次数*/

/*!
 * sd 挂载文件系统
 * @return RT_EOK: 挂载成功；RT_ERROR: 挂载失败；
 */
rt_err_t bsp_sd_mount(void){
    rt_tick_t tick = rt_tick_get_millisecond()+SD_WAIT_TIMER;
    rt_device_t dev = rt_device_find(SD_DEVICE_NAME);
    while(dev == RT_NULL){
        dev = rt_device_find(SD_DEVICE_NAME);
        if(rt_tick_get_millisecond() > tick){
            break;
        }
        rt_thread_mdelay(100);
    }
    if(dev == RT_NULL){
        LOG_D("sd 设备不存在: %s",SD_DEVICE_NAME);
        return RT_ERROR;
    }
    _try:
    if(dfs_mount(SD_DEVICE_NAME,SD_MOUNT_PATH,SD_FILE_SYSTEM,0,0)){
        LOG_D("sd设备（%s），挂载%s文件系统失败",SD_DEVICE_NAME,SD_FILE_SYSTEM);
        LOG_D("格式化该设备");
        if(dfs_mkfs(SD_DEVICE_NAME,SD_FILE_SYSTEM)){
            LOG_D("设备格式化失败");
            return RT_ERROR;
        }else goto _try;
    }
    LOG_D("文件系统挂载成功");
    return RT_EOK;
}

/*!
 * sd 卸载文件系统
 */
void bsp_sd_unmount(void){
    dfs_unmount(SD_MOUNT_PATH);
}

/*!
 * sd 格式化文件系统
 */
void bsp_sd_format(void){
    dfs_unmount(SD_MOUNT_PATH);
    if(dfs_mkfs(SD_DEVICE_NAME,SD_FILE_SYSTEM)){
        LOG_D("设备格式化失败");
    }
    LOG_D("格式化成功");
    bsp_sd_mount();
}

/*!
 * 获取sd内存使用情况
 * @param totalSize 总内存大小
 * @param freeSize 可用内存大小
 * @return RT_EOK:获取成功；RT_ERROR:获取失败；
 */
rt_err_t bsp_sd_space_info(uint64_t *totalSize,uint64_t *freeSize){
    struct statfs fs_stat;
    int ret;
    ret = statfs(SD_MOUNT_PATH, &fs_stat);
    if(ret != 0){
        LOG_D("获取sd内存使用情况");
        return RT_ERROR;
    }
    *totalSize = (unsigned long long)fs_stat.f_blocks * (unsigned long long)fs_stat.f_bsize;
    *freeSize = (unsigned long long)fs_stat.f_bfree * (unsigned long long)fs_stat.f_bsize;
    LOG_D("总空间: %llu bytes", *totalSize);
    LOG_D("已使用空间: %llu bytes", (unsigned long long)(fs_stat.f_blocks - fs_stat.f_bfree) * (unsigned long long)fs_stat.f_bsize);
    LOG_D("可使用空间: %llu bytes", *freeSize);
    return RT_EOK;
}

/*!
 * 拷贝文件
 * @param dest_path 目标文件
 * @param src_path 原文件
 * @return 0:拷贝成功，-1：拷贝失败，
 */
int8_t copy_file(const char *src_path,const char *dest_path) {
#define BUFFER_SIZE (1024)
    int src_fd, dest_fd;
    int8_t ret = -1;
    ssize_t bytes_read, bytes_written;
    char *buffer = (char*)rt_malloc(BUFFER_SIZE);
    if(buffer == RT_NULL) return -1;
	uint8_t try_num  = 3;
    dest_fd = -1;
    // 打开源文件
    file_open(&src_fd,(char*)src_path,O_RDWR);
    if (src_fd == -1) {
        LOG_D("无法打开源文件 %s.", src_path);     
        goto exit;
    }

    // 创建或打开目标文件
    file_open(&dest_fd,(char*)dest_path,O_CREAT|O_RDWR);
    if (dest_fd == -1) {
        LOG_D("无法创建文件:%s", dest_path);
		file_close(&src_fd);
        goto exit;
    }

    // 逐个字节复制文件内容
    while ((bytes_read = file_read(&src_fd, buffer, BUFFER_SIZE)) > 0) {
		_try:
        bytes_written = file_write(&dest_fd, buffer, bytes_read);
		if (bytes_written != bytes_read) {
			if((--try_num) > 0) {
				file_open(&dest_fd,(char*)dest_path,O_APPEND|O_RDWR);/*重新打开文件*/
				goto _try;
			}
			LOG_D("写入目标文件时发生错误,%s",dest_path);
			file_close(&src_fd);
			goto exit;
		}else try_num  = 3;
    }
	if(bytes_read == 0) file_close(&src_fd);
	file_close(&dest_fd);
    ret = 0;
    LOG_D("已成功复制文件 %s 到 %s", src_path, dest_path);
    exit:
    rt_free(buffer);
    return ret;
}

int folder_exists(const char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1; // 文件夹存在
    } else {
        return 0; // 文件夹不存在
    }
}

/*创建文件夹*/
int8_t dir_create(char *dir_path){
	DIR* dir = opendir(dir_path);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
		LOG_D("文件夹：'%s' 存在", dir_path);
		return 1;
    } else {
        /* Directory does not exist. */
        if (mkdir(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            LOG_D("文件夹：'%s' 创建失败", dir_path);
			return 0;
        }
        LOG_D("文件夹：'%s' 创建成功", dir_path);
		return 1;
    }
}

//强制删除目录,只能删除空目录和其中的文件，如果目录中有正在使用的文件或者子目录非空
int delete_directory(const char *path) {
#define SIZE		(256)
    int ret = 0;
    DIR *dir;
    struct dirent *entry;
    if (rmdir(path) == 0) return 0;//该目录为空目录，直接删除
    char *child_path = (char*)rt_malloc(SIZE);
    if(child_path == RT_NULL) return -1;
    dir = opendir(path);
    if (dir == NULL) {
        LOG_D("%s打开失败",path);
        ret =  -1;
        goto exit;
    }
    while ((entry = readdir(dir)) != NULL) {
        if(entry->d_name[0] == '.') continue;
        rt_snprintf(child_path, SIZE, "%s/%s", path, entry->d_name);
        if (entry->d_type == DT_DIR) {
            if (delete_directory(child_path) != 0) {
//                closedir(dir);
//                ret =  -1;
//                goto exit;
            }
			delete_directory(child_path);
        } else {
            if (remove(child_path) != 0) {
                LOG_D("%s删除失败",child_path);
//                closedir(dir);
//                ret =  -1;
//                goto exit;
            }
        }
    }
    closedir(dir);
    if (rmdir(path) != 0) {
        LOG_D("%s删除失败",path);
        ret =  -1;
    }
    exit:
    rt_free(child_path);
    return ret;
}

//文件创建
void file_open(int *fd,char* filename,int flags){
    *fd  = -1;
    if(filename == RT_NULL) return;
    // uint8_t try_num = ERROR_TYR_NUM;
    // _try:
    *fd = open(filename, flags);
    // if(*fd < 0) {
    //     if((--try_num) > 0) goto _try;
    //     else LOG_D("无法打开文件:%s",filename);
    // }
    check_fd_validity(fd);
}

/*文件指针移动*/
off_t file_lseek(int *fd,off_t offset,int whence){
    if(*fd < 0) return -1;
    return lseek(*fd,offset,whence);
}

//文件关闭
void file_close(int *fd){
	uint8_t try_num = ERROR_TYR_NUM;
    if(*fd < 0) return;
	_try:
    if(-1 == close(*fd)) {
		if((--try_num) > 0) goto _try;
    }else *fd = -1;
}

//文件写入
int file_write(int *fd,char* buff, uint16_t size){
    if(*fd < 0)  return -1;
    int len =  write(*fd, buff, size);
    if(len < 0) {
        file_close(fd);
    }
    return len;
}

//文件读取
int file_read(int *fd,char *buff,uint16_t size){
    if(*fd < 0) return -1;
	uint8_t try_num = ERROR_TYR_NUM;
    int ret;
	_try:
	ret	= read(*fd,buff,size);
    if(ret < 0) {
		if((--try_num) > 0) goto _try;
        file_close(fd);
    }
    return ret;
}


//文件删除
int file_delete(char *filepath){
    if(filepath == RT_NULL){
        return -1;
    }
    return remove(filepath);
}

int check_fd_validity(int *fd) {
    if(*fd < 0) return 0;
    if (fcntl(*fd, F_GETFL) == -1) {
        *fd = -1;// 文件描述符无效
        return 0; // 返回 0 表示无效
    }
    // 文件描述符有效
    return 1; // 返回 1 表示有效
}

/*采集数据文件获取*/
time_t gather_file_fetch(char *dir_name){
#define SIZE		(256)
    time_t ret = 0;
    DIR *dir;
    struct dirent *entry;
    static char child_path[SIZE];
    if(child_path == RT_NULL) return 0;
    dir = opendir(dir_name);
    if (dir == NULL) {
        LOG_D("%s打开失败",dir_name);
        goto _exit;
    }
    while ((entry = readdir(dir)) != NULL) {
        if(entry->d_name[0] == '.') continue;
        snprintf(child_path, SIZE, "%s/%s", dir_name, entry->d_name);
        if (entry->d_type == DT_DIR) {/*非法目录进行删除*/
            delete_directory(child_path);
        } else {
            time_t temp = atoi(entry->d_name);
            if(temp == 0){/*无效数据进行删除*/
                remove(child_path);
            }else{/*找到有效文件结束循环*/
                ret = temp;
                break;
            }
        }
    }
    closedir(dir);
    _exit:
    return ret;
}

/*文件重命名*/
int file_rename(char *old_name,char *new_name){
    return rename(old_name,new_name);
}

/*获取文件大小*/
off_t get_file_size(char *file_name){
    int ret;
    int fd = -1;
    struct stat file_stat;

    file_open(&fd,file_name, O_RDONLY);	// 打开文件
    if (fd == -1) {
        printf("Open file %s failed : %s\n", file_name, strerror(errno));
        return -1;
    }
    ret = fstat(fd, &file_stat);	// 获取文件状态
    if (ret == -1) {
        file_stat.st_size = 0;
        printf("Get file %s stat failed:%s\n", file_name, strerror(errno));
        file_close(&fd);
        return -1;
    }
    file_close(&fd);
    return file_stat.st_size;
}


static void sd(char agv,char *agr[]){
    if(agv == 2){
        if(0 == rt_strcmp("format",agr[1])){
            bsp_sd_format();
        }
        else if(0 == rt_strcmp("space",agr[1])){
            uint64_t t,a;
            bsp_sd_space_info(&t,&a);
        }
        else goto _help;
    }else{
        _help:
        rt_kprintf("sd format：格式化sd \n");
        rt_kprintf("sd space：查看sd空间使用情况\n");
    }
}
MSH_CMD_EXPORT(sd,sd);


static void copy(char agr,char *agc[]){
	if(agr < 3) return;
	switch(agr){
		case 3: copy_file(agc[1],agc[2]);break;
		case 4:{
			uint16_t index = atoi(agc[3]);
			char file_path[128];
			for(uint16_t i=0;i<index;i++){
				rt_sprintf(file_path,"%s_%d",agc[2],i);
				copy_file(agc[1],file_path);
				rt_thread_mdelay(10);
			}
		}break;
		default:{
			rt_kprintf("copy /1.txt /2.txt;拷贝单个文件\n");
			rt_kprintf("copy /1.txt /back 10;一个文件拷贝10次\n");
		}break;
	}
}
MSH_CMD_EXPORT(copy,copy);


static void file_contrast(char agr,char *agc[]){
	if(agr < 3) return;
	char buf[2][1024];
	int read_len[2];
	int fd[2];
    file_open(&fd[0],agc[1],O_RDONLY);
    file_open(&fd[1],agc[2],O_RDONLY);
	uint8_t falg = 1;
	while(1){
		read_len[0] = file_read(&fd[0],buf[0],1024);
		read_len[1] = file_read(&fd[1],buf[1],1024);
		if(read_len[0] != read_len[1]){
			falg = 0;
			break;
		}
		if(0 != rt_memcmp(buf[0],buf[1],read_len[0])){
			falg = 0;
			break;
		}
		if(read_len[0] == 0) break;
	}
	file_close(&fd[0]);
	file_close(&fd[1]);
	rt_kprintf("%s 和 %s %s\n",agc[1],agc[2],falg?"内容一致":"内容不一致");
}
MSH_CMD_EXPORT(file_contrast,file_contrast);

static void dir_delete_entry(void *par){
	delete_directory("/history");
}

static void rtu_dir_delete(void){
	rt_thread_t th = rt_thread_create("ddir",dir_delete_entry,RT_NULL,4096,20,100);
	rt_thread_startup(th);
}
MSH_CMD_EXPORT(rtu_dir_delete,rtu_dir_delete);
