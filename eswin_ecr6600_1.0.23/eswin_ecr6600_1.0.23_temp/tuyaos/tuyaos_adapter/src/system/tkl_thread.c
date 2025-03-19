/**
 * @file tuya_os_adapt_thread.c
 * @brief 线程操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */

#include "tkl_thread.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include <FreeRTOS.h>
#include <task.h>
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
 * @brief create and start a tuya sdk thread
 * 
 * @param[out] thread       the thread handle
 * @param[in] name          the thread name
 * @param[in] stack_size    stack size of thread
 * @param[in] priority      priority of thread
 * @param[in] func          the main thread process function
 * @param[in] arg           the args of the func, can be null
 * @retval OPRT_OK          success
 * @retval Other            fail
 */
OPERATE_RET tkl_thread_create(TKL_THREAD_HANDLE* thread,
                              CONST CHAR_T* name,
                              UINT_T stack_size,
                              UINT_T priority,
                              CONST THREAD_FUNC_T func,
                              VOID_T* CONST arg)
{
    BaseType_t ret = 0;
    ret = xTaskCreate(func, name, stack_size / sizeof(portSTACK_TYPE), (void *const)arg, priority, thread);
    if (ret != pdPASS) {
        return OPRT_OS_ADAPTER_THRD_CREAT_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief terminal thread and release thread resources
* 
* @param[in] thread    the input thread handle
* @retval OPRT_OK      success
* @retval Other        fail
*/
OPERATE_RET tkl_thread_release(CONST TKL_THREAD_HANDLE thread)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }
    
    vTaskDelete(thread);
    
    return OPRT_OK;
}

/**
 * @brief check thread is self thread
 * 
 * @param[in] thread    the thread handle
 * @param[out] is_self  output is self thread
 * @retval OPRT_OK      success
 * @retval Other        fail
 */
OPERATE_RET tkl_thread_is_self(TKL_THREAD_HANDLE thread, BOOL_T* is_self)
{
    if (NULL == thread || NULL == is_self) {
        return OPRT_INVALID_PARM;
    }

    TKL_THREAD_HANDLE self_handle = (TKL_THREAD_HANDLE)xTaskGetCurrentTaskHandle();
    if (NULL == self_handle) {
        return OPRT_OS_ADAPTER_THRD_JUDGE_SELF_FAILED;
    }

    *is_self = (thread == self_handle);
    
    return OPRT_OK;
}

/**
 * @brief set name of self thread
 * 
 * @param[in] name      thread name
 * @retval OPRT_OK      success
 * @retval Other        fail
 */
OPERATE_RET tkl_thread_set_self_name(CONST CHAR_T* name)
{
    return OPRT_OK;
}

/**
* @brief set thread priority
*
* @param[in] thread       the input thread handle
* @param[in] prio_thread  the priority thread send to
* @retval void
*/
OPERATE_RET tkl_thread_set_priority(TKL_THREAD_HANDLE thread, INT_T priority)
{
    if (NULL == thread) {
        return OPRT_INVALID_PARM;
    }

    vTaskPrioritySet(thread, priority);

    return OPRT_OK;
}

/**
* @brief get thread priority
*
* @param[in]  thread     the input thread handle
* @param[out] priority   the priority of thread
* @retval void
*/
OPERATE_RET tkl_thread_get_priority(TKL_THREAD_HANDLE thread, INT_T *priority)
{
    if (NULL == thread || NULL == priority) {
        return OPRT_INVALID_PARM;
    }

    *priority = (UINT_T)uxTaskPriorityGet(thread);

    return OPRT_OK;
}


OPERATE_RET tkl_thread_get_watermark(CONST TKL_THREAD_HANDLE thread, UINT_T* watermark)
{
    if (NULL == thread || NULL == watermark) {
        return OPRT_INVALID_PARM;
    }

    *watermark = uxTaskGetStackHighWaterMark(thread) * sizeof(UBaseType_t);

    return OPRT_OK;
}

OPERATE_RET tkl_thread_get_id(TKL_THREAD_HANDLE *thread)
{
    *thread = (TKL_THREAD_HANDLE)xTaskGetCurrentTaskHandle();

    return OPRT_OK;
}

/**
* @brief Diagnose the thread(dump task stack, etc.)
*
* @param[in] thread: thread handle
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_diagnose(TKL_THREAD_HANDLE thread)
{
    vTaskStackDump(thread);
    return OPRT_OK;
}

