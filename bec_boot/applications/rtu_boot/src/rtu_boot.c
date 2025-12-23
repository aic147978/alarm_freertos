#include <rtthread.h>
#include "board.h"
#include "rtu_boot.h"
#include <unistd.h>
#include <fcntl.h>
#include "crc32.h"
#include <string.h>
#include "boot_config.h"
#include "drive_inc.h"
#include <fal.h>


#define LOG_TAG             "rboot"
#define DRV_DEBUG						/*开启日志打印*/
#include <drv_log.h>

#define QBOOT_BUF_SIZE 4096        // must is 4096
#define QBOOT_CMPRS_READ_SIZE 1024 // it can is 512, 1024, 2048, 4096,
#define QBOOT_CMPRS_BUF_SIZE (QBOOT_BUF_SIZE + QBOOT_CMPRS_READ_SIZE)
#define QBOOT_ALGO2_VERIFY_CRC 1
#define QBOOT_ALGO2_VERIFY_MASK 0x0F

typedef struct
{
    uint8_t type[4];
    uint16_t algo;
    uint16_t algo2;
    uint32_t time_stamp;
    uint8_t part_name[16];
    uint8_t fw_ver[24];
    uint8_t prod_code[24];
    uint32_t pkg_crc;
    uint32_t raw_crc;
    uint32_t raw_size;
    uint32_t pkg_size;
    uint32_t hdr_crc;
} fw_info_t;

static uint8_t cmprs_buf[QBOOT_CMPRS_BUF_SIZE];

// sd卡 固件信息检测 1：合法；不合法；
static int8_t rb_sd_fw_info_check(int *fd, fw_info_t *fw)
{
    int len = file_read(fd, (char *)fw, sizeof(fw_info_t));
    if (len != sizeof(fw_info_t))
    {
        LOG_D("读取固件信息失败");
		file_close(fd);
        return 0;
    }
    if (strcmp((const char *)(fw->type), "RBL") != 0)
    {
        LOG_D("固件信息错误 !!!");
		file_close(fd);
        return 0;
    }

    uint32_t crc32 = crc32_cal((uint8_t *)fw, (sizeof(fw_info_t) - sizeof(uint32_t)));
    if (crc32 == fw->hdr_crc)
    {
        return 1;
    }
    else
    {
        LOG_D("固件信息数据校验错误！！ 0x%x != 0x%x", crc32, fw->hdr_crc);
		file_close(fd);
        return 0;
    }
}

/*sd卡 固件数据检测 1：合法；0：不合法*/
static int8_t rb_fw_crc_chreck(int *fd, fw_info_t *fw)
{
    uint32_t pos = 0;
    uint32_t crc32 = 0xFFFFFFFF;
    while (pos < fw->pkg_size)
    {
        int read_len = QBOOT_BUF_SIZE;
        int gzip_remain_len = fw->pkg_size - pos;
        if (read_len > gzip_remain_len)
        {
            read_len = gzip_remain_len;
        }
        int rc = file_read(fd, (char*)cmprs_buf, read_len);
        if (rc <= 0)
        {
            LOG_D("固件数据校验 读取错误!!");
            break;
        }
        crc32 = crc32_cyc_cal(crc32, cmprs_buf, rc);
        pos += rc;
    }
    crc32 ^= 0xFFFFFFFF;
    if (crc32 != fw->pkg_crc)
    {
        LOG_D("固件数据校验crc错误 0x%08X != %08X", crc32, fw->pkg_crc);
		file_close(fd);
        return 0;
    }
    return 1;
}

/*sd卡 固件app程序检测 1：合法；0：不合法 */
static int8_t rb_sd_fw_app_crc_chrek(int fd,fw_info_t *fw){
    if ((fw->algo2 & QBOOT_ALGO2_VERIFY_MASK) != QBOOT_ALGO2_VERIFY_CRC) {
       // LOG_D("(fw->algo2 & QBOOT_ALGO2_VERIFY_MASK) != QBOOT_ALGO2_VERIFY_CRC");
        return 1;
    }
    LOG_D("未实现对app程序进行解密或解压功能");
    return 0;
}

/*检测固件的合法性 1：合法；0：不合法,-1：固件不存在*/
static int8_t sd_check_firmware_validity(char *file_path)
{
    int fd;
    static fw_info_t fw_info;
    int8_t ret = 0;
    LOG_D("检测固件：%s", file_path);
    file_open(&fd,file_path, O_RDONLY);
    if (fd < 0)
    {
        LOG_D("该固件不存在：%s", file_path);
        ret = -1;
        goto _return;
    }
    LOG_D("固件存在");
    /*检测固件信息*/
    if (0 == rb_sd_fw_info_check(&fd, &fw_info))
        goto _return;
    LOG_D("固件信息合法");
    if (0 == rb_fw_crc_chreck(&fd, &fw_info))
        goto _return;
    LOG_D("固件合法");
    if(0 == rb_sd_fw_app_crc_chrek(fd,&fw_info))  goto _return;
    LOG_D("固件app程序解析成功");
    ret = 1;
	file_close(&fd);
    _return:
    return ret;
}

/*进度条显示*/
static void print_progress(size_t cur_size, size_t total_size)
{
#define multiple (2)
    unsigned char progress_sign[100/multiple + 1];
    uint8_t i, per = cur_size * 100 / total_size;

    if (per > 100)
    {
        per = 100;
    }

    uint8_t t_per = per/multiple;
    for (i = 0; i < 50; i++)
    {
        if (i < t_per)
        {
            progress_sign[i] = '=';
        }
        else if (t_per == i)
        {
            progress_sign[i] = '>';
        }
        else
        {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';
    LOG_D("FlashWrite: [%s] %03d%%\033[1A", progress_sign, per);
}


/*!
 * 固件写入app内存区
 * @param file_path 固件路径
 * @return 0：固件信息获取错误，1：升级成功；2：升级失败；
 */
static int8_t rb_fw_release(char *file_path){
    uint32_t dst_write_pos = 0;
    uint32_t src_read_pos = 0;
    fw_info_t fw;
    int fd;
    file_open(&fd,file_path,O_RDONLY);
    if(fd < 0){
        LOG_D("固件不存在：%s",file_path);
        return 0;
    }
    int read_len = read(fd,(void*)&fw,sizeof(fw_info_t));
    if(read_len != sizeof(fw_info_t)){
        LOG_D("读取固件头部信息失败");
        return 0;
    }
    int8_t ret = 1;
    uint32_t crc32 = CRC32_INIT_VAL;
    /*写入app内存*/
    const struct fal_partition *partition = RT_NULL;
    partition = fal_partition_find((const char*)fw.part_name);
    if (partition == RT_NULL){
        log_d("查找分区(%s)失败",fw.part_name);
        ret = 2;
        goto _return;
    }
    if(fal_partition_erase(partition,0,fw.pkg_size) != fw.pkg_size){
        log_d("分区(%s)擦除失败，地址：0~0x%08x",fw.part_name,fw.pkg_size);
        ret = 2;
        goto _return;
    }
    while(dst_write_pos < fw.pkg_size){
        int read_len = QBOOT_CMPRS_READ_SIZE;
        int remain_len = (fw.pkg_size - src_read_pos);
        if (read_len > remain_len)
        {
            read_len = remain_len;
        }
        int rc = file_read(&fd,(char*)cmprs_buf,read_len);
        if(rc != read_len){
            ret = 2;
            LOG_D("固件读取错误");
            goto _return;
        }
        rc = fal_partition_write(partition,dst_write_pos,cmprs_buf,read_len);
        if(rc != read_len){
            ret = 2;
            LOG_D("固件写入错误");
            goto _return;
        }
        int fal_read_len = fal_partition_read(partition,dst_write_pos,cmprs_buf,read_len);
        if(fal_read_len !=  read_len){
            ret = 2;
            LOG_D("分区（%s）读取失败，addr: 0x%08x,read_size: %d",fw.part_name,dst_write_pos,read_len);
            goto _return;
        }
        crc32 = crc32_cyc_cal(crc32, cmprs_buf, rc);
        dst_write_pos += rc;
        src_read_pos += rc;
        print_progress(dst_write_pos,fw.pkg_size);
    }
    crc32 ^= CRC32_INIT_VAL;
    if (crc32 != fw.pkg_crc){
        LOG_D("分区数据校验crc错误 0x%08X != 0x%08X", crc32, fw.pkg_crc);
        ret = 2;
        goto _return;
    }
    LOG_D("固件写入成功");
    _return:
    file_close(&fd);
    return ret;
}

/*rtu boot*/
void rtu_boot_startup(void)
{
    int8_t ret;
    if(0 > fal_init()){/*初始fal组件*/
        LOG_D("fal组件初始化失败");
        return;
    }
    if(1 == shell_key_check()){
        return;
    }
    ret = sd_check_firmware_validity(RTU_OTA_FILE_PATH);
    if(ret != 1){/*固件不存在 或 固件不合法*/
        if(ret == 0) {/*固件不合法*/
            remove(RTU_OTA_FILE_PATH);/*删除固件*/
        }
        rb_jump_to_app();
        return;
    }
    LOG_D("进入升级步骤...");
    LOG_D("将固件写入app内存中...");
    ret = rb_fw_release(RTU_OTA_FILE_PATH);/*将升级固件写入app中*/
    if(ret != 1){/*升级失败*/
        remove(RTU_OTA_FILE_PATH);/*删除固件*/
        LOG_D("升级失败,检测是否有备份固件");
        if(1 == sd_check_firmware_validity(RTU_BACK_FILE_PATH)){
            LOG_D("备份固件存在并合法");
            LOG_D("将备份固件写入app内存中...");
            if(1 == rb_fw_release(RTU_BACK_FILE_PATH)){
                LOG_D("备份固件写入成功");
                rb_jump_to_app();
            }else {
                LOG_D("备份固件写入失败");
                goto _last;
            }
        }else{/*上版本固件*/
            _last:
            LOG_D("检测是否有上版本固件");
            if(1 == sd_check_firmware_validity(RTU_LAST_FILE_PATH)) {
                LOG_D("上版本固件存在并合法");
                LOG_D("将上版本固件写入app内存中...");
                if (1 == rb_fw_release(RTU_LAST_FILE_PATH)) {
                    LOG_D("上版本固件写入成功");
                    rb_jump_to_app();
                }else LOG_D("上版本固件写入失败");
            }
        }
    }else{
        LOG_D("设备升级成功");
        rb_fw_update();
		remove(RTU_OTA_FILE_PATH);/*删除固件*/
        rb_jump_to_app();
    }
}

void rb_jump_to_app(void)
{
    typedef void (*app_func_t)(void);
    uint32_t app_addr = RBOOT_APP_ADDR;
    uint32_t stk_addr = *((__IO uint32_t *)app_addr);
    app_func_t app_func = (app_func_t)(*((__IO uint32_t *)(app_addr + 4)));

    if ((((uint32_t)app_func & 0xff000000) != 0x08000000) || ((stk_addr & 0x2ff00000) != 0x20000000))
    {
        LOG_E("没有合法的程序。");
        return;
    }

    rt_kprintf("跳转到应用程序运行 ... \n");
    rt_thread_mdelay(200);

    __disable_irq();
    HAL_DeInit();

    for(int i=0; i<128; i++)
    {
        HAL_NVIC_DisableIRQ((IRQn_Type)i);
        HAL_NVIC_ClearPendingIRQ((IRQn_Type)i);
    }

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    HAL_RCC_DeInit();

    __set_CONTROL(0);
    __set_MSP(stk_addr);

    app_func();
    LOG_D("跳转到应用程序失败");
}

/*!
 * 固件更新
 */
static void rb_fw_update(void){
    LOG_D("更新存储固件");
    /*检测备份固件是否存在*/
    if(1 == sd_check_firmware_validity(RTU_BACK_FILE_PATH)){
        remove(RTU_BACK_FILE_PATH);
        copy_file(RTU_BACK_FILE_PATH,RTU_LAST_FILE_PATH);
        remove(RTU_BACK_FILE_PATH);
        copy_file(RTU_OTA_FILE_PATH,RTU_BACK_FILE_PATH);
    }else{
        remove(RTU_BACK_FILE_PATH);
        remove(RTU_BACK_FILE_PATH);
        copy_file(RTU_OTA_FILE_PATH,RTU_LAST_FILE_PATH);
        copy_file(RTU_OTA_FILE_PATH,RTU_BACK_FILE_PATH);
    }
}



/*!
 * 检测是否输入回车
 * @return 1： 输入回车；0：未输入回车；
 */
static int8_t shell_key_check(void){
    char ch;
    LOG_D("输入回车（enter）退出boot流程，等待时间(%dS).", RBOOT_SHELL_KEY_CHK);
	int8_t ret = 0;
    rt_tick_t tick_start = rt_tick_get_millisecond();
    rt_tick_t tick = rt_tick_from_millisecond(RBOOT_SHELL_KEY_CHK * 1000);
    while((rt_tick_get_millisecond() - tick_start) < tick){
        if (rt_device_read(rt_console_get_device(), -1, &ch, 1) > 0){
            if (ch == 0x0d){
                ret = 1;
				break;
            }
            continue;
        }
    }
    return ret;
}

static void rbl(char agv,char *agr[]){
    if(agv == 2){
        if(0 == rt_strcmp("jump",agr[1])){
            rb_jump_to_app();
        }
        else if(0 == rt_strcmp("back",agr[1])){
            if(1 == sd_check_firmware_validity(RTU_BACK_FILE_PATH)){
                int8_t ret = rb_fw_release(RTU_BACK_FILE_PATH);
                if(ret == 1) rb_jump_to_app();
                else rt_kprintf("固件写入失败");
            }
        }
        else if(0 == rt_strcmp("last",agr[1])){
            if(1 == sd_check_firmware_validity(RTU_LAST_FILE_PATH)){
                int8_t ret = rb_fw_release(RTU_LAST_FILE_PATH);
                if(ret == 1) rb_jump_to_app();
                else rt_kprintf("固件写入失败");
            }
        }else goto _help;
    }else{
        _help:
        rt_kprintf("rbl jump，跳转到app程序\n");
        rt_kprintf("rbl back，使用备份固件\n");
        rt_kprintf("rbl last，使用上版本固件\n");
    }
}
MSH_CMD_EXPORT(rbl,rbl);

