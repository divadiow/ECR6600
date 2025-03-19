/**
 * @file tuya_os_adapt_semaphore.c
 * @brief semaphore相关接口封装
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
 
#include "tkl_semaphore.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
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
 * @brief tuya_os_adapt_semaphore_create_init用于创建并初始化semaphore
 * 
 * @param[out] *pHandle semaphore句柄
 * @param[in] semCnt 
 * @param[in] sem_max 
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_create_init(TKL_SEM_HANDLE *pHandle, const unsigned int semCnt, const unsigned int sem_max)
{
    if(NULL == pHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
	*pHandle = (TKL_SEM_HANDLE)os_sem_create(sem_max, semCnt);
	
	return ((NULL == *pHandle) ? OPRT_OS_ADAPTER_SEM_CREAT_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_semaphore_wait用于wait semaphore
 * 
 * @param[in] semHandle semaphore句柄
 * @return int 0=成功，非0=失败
 */
int tuya_os_adapt_semaphore_wait(const TKL_SEM_HANDLE semHandle)
{
    if(!semHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
	int ret=os_sem_wait((os_sem_handle_t)semHandle,WAIT_FOREVER);

	return ((ret < 0) ? OPRT_OS_ADAPTER_SEM_WAIT_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_semaphore_waittimeout used fo wait semaphore with timeout
 *
 * @param[in] semHandle semaphore句柄
 * @param[in] timeout  semaphore超时时间
 * @return int 0=成功，非0=失败
*/
OPERATE_RET tkl_semaphore_wait(CONST TKL_SEM_HANDLE handle, CONST UINT_T timeout)
{
    if(!handle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    UINT_T time_out = timeout;

    if (time_out == TKL_SEM_WAIT_FOREVER)
    {
    	time_out=WAIT_FOREVER;
    }
	int ret=os_sem_wait((os_sem_handle_t)handle,time_out);
	
	return ((ret < 0) ? OPRT_OS_ADAPTER_SEM_WAIT_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_semaphore_post用于post semaphore
 * 
 * @param[in] semHandle semaphore句柄
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_post(const TKL_SEM_HANDLE semHandle)
{
    if(!semHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
	int ret=os_sem_post((os_sem_handle_t)semHandle);
	
	return ((ret < 0) ? OPRT_OS_ADAPTER_SEM_POST_FAILED : OPRT_OK);
}

/**
 * @brief tuya_os_adapt_semaphore_release用于release semaphore
 * 
 * @param[in] semHandle 
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_release(const TKL_SEM_HANDLE semHandle)
{
    if(!semHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
 
	os_sem_destroy((os_sem_handle_t) semHandle);
	
	return OPRT_OK;
}


