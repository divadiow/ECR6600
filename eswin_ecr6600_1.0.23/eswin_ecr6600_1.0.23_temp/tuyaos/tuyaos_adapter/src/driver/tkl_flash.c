/**
 * @file tuya_os_adapt_storge.c
 * @brief flash操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "tkl_flash.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "flash.h"
#include "oshal.h"
#include "cli.h"
#include "hal_flash.h"
#include "portmacro.h"
#include "FreeRTOSConfig.h"
#include "hal_system.h"



/***********************************************************
*************************micro define***********************
***********************************************************/

#define PARTITION_SIZE (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ PARTITION_SIZE

#if 0
//key
#define SIMPLE_FLASH_KEY_ADDR 0x1CB000 //

//KV
#define SIMPLE_FLASH_START (SIMPLE_FLASH_KEY_ADDR + PARTITION_SIZE)
#define SIMPLE_FLASH_SIZE 0xB000 // 44K

//UF
#define UF_PARTITION_NUM     1
#define UF_PARTITION_START  (SIMPLE_FLASH_START + SIMPLE_FLASH_SIZE)
#define UF_PARTITION_SIZE   0x1F000

#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
#define SIMPLE_FLASH_KV_PROTECTED_START (UF_PARTITION_START + UF_PARTITION_SIZE)
#define SIMPLE_FLASH_KV_PROTECTED_SIZE 0x1000
#endif
#endif

#if 1
//key
//#ifdef CONFIG_ENABLE_FLASH_4M
#define SIMPLE_FLASH_KEY_ADDR 0x3D4000
//#else
//#define SIMPLE_FLASH_KEY_ADDR 0x1D5000
//#endif

//KV
#define SIMPLE_FLASH_START (SIMPLE_FLASH_KEY_ADDR + PARTITION_SIZE)
#define SIMPLE_FLASH_SIZE 0x8000 // 32K

//UF
#define UF_PARTITION_NUM     1
#define UF_PARTITION_START  (SIMPLE_FLASH_START + SIMPLE_FLASH_SIZE)
#define UF_PARTITION_SIZE   0x18000 // 96K

#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
#define SIMPLE_FLASH_KV_PROTECTED_START (UF_PARTITION_START + UF_PARTITION_SIZE)
#define SIMPLE_FLASH_KV_PROTECTED_SIZE 0x1000
#endif
#endif
/***********************************************************
*************************variable define********************
***********************************************************/


/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief read data from flash
 * 
 * @param[in]       addr        flash address
 * @param[out]      dst         pointer of buffer
 * @param[in]       size        size of buffer
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_flash_read(UINT_T addr, UCHAR_T *dst, UINT_T size)
{
    if(NULL == dst) 
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    //unsigned int flag = system_irq_save_tick_compensation();
	//func_after_irq();
	
    int ret = hal_flash_read(addr,dst,size);	

	//func_before_restore();
	
    //system_irq_restore_tick_compensation(flag);
    //cli_printf("read %x %d %x %x\r\n",addr, size, dst[0], dst[size-1]);
    if (ret == FLASH_RET_SUCCESS)
    {
        return OPRT_OK;
    }
    else
    {
        return OPRT_OS_ADAPTER_FLASH_READ_FAILED;
    }
}

/**
 * @brief write data to flash
 * 
 * @param[in]       addr        flash address
 * @param[in]       src         pointer of buffer
 * @param[in]       size        size of buffer
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_flash_write(UINT_T addr, CONST UCHAR_T *src, UINT_T size)
{
    if(NULL == src) 
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //unsigned int flag = system_irq_save_tick_compensation();
	//func_after_irq();
	int ret = hal_flash_write(addr,(unsigned char *)src,size);
	//func_before_restore();
    //system_irq_restore_tick_compensation(flag);
    
    if (ret == FLASH_RET_SUCCESS)
    {
        return OPRT_OK;
    }
    else
    {
        return OPRT_OS_ADAPTER_FLASH_WRITE_FAILED;
    }

}

/**
 * @brief erase flash block
 * 
 * @param[in]       addr        flash block address
 * @param[in]       size        size of flash block
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_flash_erase(UINT_T addr, UINT_T size)
{
    //unsigned int flag = system_irq_save_tick_compensation();
	//func_after_irq();
    int ret = hal_flash_erase(addr,size);
	//func_before_restore();
    //system_irq_restore_tick_compensation(flag);
    //cli_printf(" %x %d\r\n",addr, size);
    if (ret == FLASH_RET_SUCCESS)
    {
        return OPRT_OK;
    }
    else
    {
        return OPRT_OS_ADAPTER_FLASH_ERASE_FAILED;
    }

}


/**
* @brief get one flash type info
*
* @param[in] type: flash type
* @param[in] info: flash info
*
* @note This API is used for unlock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_E type, TUYA_FLASH_BASE_INFO_T* info)
{
    if ((type > TUYA_FLASH_TYPE_MAX) || (info == NULL)) {
        return OPRT_INVALID_PARM;
    }
    switch (type) {
        case TUYA_FLASH_TYPE_UF:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = UF_PARTITION_START;
            info->partition[0].size = UF_PARTITION_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_DATA:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_START;
            info->partition[0].size = SIMPLE_FLASH_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_KEY:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_KEY_ADDR;
            info->partition[0].size = FLH_BLOCK_SZ;
            break;
#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
       case TUYA_FLASH_TYPE_KV_PROTECT:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_KV_PROTECTED_START;
            info->partition[0].size = SIMPLE_FLASH_KV_PROTECTED_SIZE;
            break;
#endif
        default:
            return OPRT_INVALID_PARM;
            break;
    }

    return OPRT_OK;
}


/**
* @brief lock flash
*
* @param[in] addr: lock begin addr
* @param[in] size: lock area size
*
* @note This API is used for lock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_lock(UINT_T addr, UINT_T size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief unlock flash
*
* @param[in] addr: unlock begin addr
* @param[in] size: unlock area size
*
* @note This API is used for unlock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_unlock(UINT_T addr, UINT_T size)
{
    return OPRT_NOT_SUPPORTED;
}

