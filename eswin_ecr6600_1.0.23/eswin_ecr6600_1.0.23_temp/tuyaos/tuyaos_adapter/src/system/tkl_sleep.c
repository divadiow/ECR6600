/**
* @file tkl_sleep.h
* @brief Common process - adapter the sleep manage api
* @version 0.1
* @date 2021-08-18
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/

#include "tkl_sleep.h"

/**
 * @brief sleep callback register
 * 
 * @param[in] sleep_cb:  sleep callback
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cpu_sleep_callback_register(TUYA_SLEEP_CB_T *sleep_cb)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief allow to sleep
 * 
 * @param[in] none
 *
 * @return none
 */
VOID_T tkl_cpu_allow_sleep(VOID_T)
{
    return ;
}

/**
 * @brief force wakeup
 * 
 * @param[in] none
 *
 * @return none
 */
VOID_T tkl_cpu_force_wakeup(VOID_T)
{
    return ;
}

