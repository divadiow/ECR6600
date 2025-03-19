/**
 ****************************************************************************************
 *
 * @file rtos.h
 *
 * @brief Declarations related to the WiFi stack integration within an RTOS.
 *
 * Copyright (C) RivieraWaves 2017-2018
 *
 ****************************************************************************************
 */

#ifndef RTOS_H_
#define RTOS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rtos_al.h"

/*
 * FUNCTIONS
 ****************************************************************************************
 */
	
/* reade only compile flag : start */
extern const volatile char wifi_lib_compile_date[];
extern const volatile char wifi_lib_compile_time[];
/* reade only compile flag : end */

/**
 ****************************************************************************************
 * @brief Main function of the RTOS
 *
 * Called after hardware initialization to create all RTOS tasks and start the scheduler.
 ****************************************************************************************
 */
void rtos_main(void);
void rtos_wifi_task_resume(bool isr);
int rtos_wifi_task_handle_get(void);
#if NX_FULLY_HOSTED
/**
 ****************************************************************************************
 * @brief Request the RTOS to increase the priority of the WiFi task.
 * By default the WiFi task has a low priority compared to other tasks. This function
 * allows increasing temporarily the WiFi task priority to a high value to ensure it is
 * scheduled immediately.
 * This function cannot be called from an ISR.
 ****************************************************************************************
 */
void rtos_wifi_task_prio_high(void);

/**
 ****************************************************************************************
 * @brief Request the RTOS to decrease the priority of the WiFi task.
 * This function puts back the WiFi task priority to its default value.
 * This function cannot be called from an ISR.
 ****************************************************************************************
 */
void rtos_wifi_task_prio_default(void);
#endif

#endif // RTOS_H_
