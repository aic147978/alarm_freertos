/**
  ******************************************************************************
  * @File name      : crc32.h
  * @Description    : 
  * @Author         : lzk
  * Version         : 1.0
  * Date            : 2024/5/11
  ******************************************************************************
  */


#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdio.h>
#include <stdint.h>


/*
 * @brief   cyclic calculation crc check value
 * @param   init_val    - initial value
 * @param   pdata       - datas pointer
 * @param   len         - datas len
 * @retval  calculated result
 */
uint32_t crc32_cyc_cal(uint32_t init_val, uint8_t *pdata, uint32_t len);

/*
 * @brief   calculation crc check value, initial is CRC32_INIT_VAL
 * @param   pdata       - datas pointer
 * @param   len         - datas len
 * @retval  calculated result
 */
uint32_t crc32_cal(uint8_t *pdata, uint32_t len);

#endif

