/**
 * @file tuya_os_adapt_mutex.c
 * @brief 互斥锁操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#define _UNI_MUTEX_GLOBAL

#include "tkl_mutex.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tuya_cloud_types.h"
#include "oshal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief tuya_os_adapt_mutex_create_init用于创建并初始化tuya mutex
 * 
 * @param[out] pMutexHandle 返回mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_create_init(TKL_MUTEX_HANDLE *pMutexHandle)
{
    if (!pMutexHandle) {
        return OPRT_INVALID_PARM;
    }
   
	*pMutexHandle=(TKL_MUTEX_HANDLE)os_mutex_create();

	return ((NULL == *pMutexHandle) ? OPRT_MALLOC_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_mutex_lock用于lock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_lock(const TKL_MUTEX_HANDLE mutexHandle)
{
    if (!mutexHandle) {
        return OPRT_INVALID_PARM;
    }
        
	int ret = os_mutex_lock((os_mutex_handle_t)mutexHandle,WAIT_FOREVER);
	
	return ((ret < 0) ? OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_mutex_unlock用于unlock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_unlock(const TKL_MUTEX_HANDLE mutexHandle)
{
    if (!mutexHandle) {
        return OPRT_INVALID_PARM;
    }
        
	int ret = os_mutex_unlock((os_mutex_handle_t)mutexHandle);
	
	return ((ret < 0) ? OPRT_OS_ADAPTER_MUTEX_UNLOCK_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_mutex_release用于释放tuya mutex
 * 
 * @param[in] mutexHandle TKL_MUTEX_HANDLE tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_release(const TKL_MUTEX_HANDLE mutexHandle)
{
    if (!mutexHandle) {
        return OPRT_INVALID_PARM;
    }

	os_mutex_destroy((os_mutex_handle_t)mutexHandle);
	
	return OPRT_OK;
}

/**
 * @brief tkl_mutex_trylock用于try lock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_trylock(const TKL_MUTEX_HANDLE mutexHandle)
{
    if (!mutexHandle) {
        return OPRT_INVALID_PARM;
    }
        
	int ret = os_mutex_lock((os_mutex_handle_t)mutexHandle,0);
	
	return ((ret < 0) ? OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED : OPRT_OK);
}


