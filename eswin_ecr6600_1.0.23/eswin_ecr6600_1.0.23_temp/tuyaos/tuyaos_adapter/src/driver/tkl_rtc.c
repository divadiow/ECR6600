 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 *                                                                            *
 =============================================================================*/

/*============================ INCLUDES ======================================*/
#include "tkl_rtc.h"
#include "hal_rtc.h"


/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

/*============================ LOCAL VARIABLES ===============================*/

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/


OPERATE_RET tkl_rtc_init(VOID_T)
{
    return OPRT_OK;
}


OPERATE_RET tkl_rtc_time_get(TIME_T *time_sec)
{   
    *time_sec = hal_rtc_get_time();
    return OPRT_OK;
}

OPERATE_RET tkl_rtc_time_set(TIME_T time_sec)
{
    return hal_rtc_set_time((int)time_sec);
}

OPERATE_RET tkl_rtc_deinit(VOID_T)
{
    return OPRT_OK;
}

