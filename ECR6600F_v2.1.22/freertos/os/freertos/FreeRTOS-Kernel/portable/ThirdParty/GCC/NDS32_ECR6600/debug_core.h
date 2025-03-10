/**
 * @file DEBUG_CORE
 * @brief Kernel debug code
 * @author liuyong
 * @date 2022-8-16
 * @version V0.1
 * @Copyright © 2022 BEIJING ESWIN COMPUTING TECHINOLOGY CO., LTD and its affiliates (“ESWIN”).
 *          All rights reserved. Any modification, reproduction, adaption, translation,
 *          distribution is prohibited without consent.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


#ifndef __DEBUG_CORE_H
#define __DEBUG_CORE_H


/*--------------------------------------------------------------------------
*                                               Include files
*  --------------------------------------------------------------------------*/
#include "rtos_debug.h"
#include "FreeRTOS.h"

/*--------------------------------------------------------------------------
*                                               Macros
*  --------------------------------------------------------------------------*/
/** Description of the macro */
#define MAX_TASK_NUM       32

/*--------------------------------------------------------------------------
*                                               Types
*  --------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */
struct isr_reg_layout
{
#if (configSUPPORT_IFC == 1)
    uint32_t ipc;
    uint32_t ifc_lp;
#endif

#if (configSUPPORT_FPU == 1)
    uint32_t fpu[portFPU_REGS / 2];
#endif
    uint32_t r0[16];
    uint32_t r1[6];
    uint32_t sp;
};

/*--------------------------------------------------------------------------
*                                               Constants
*  --------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
*                                               Global  Variables
*  --------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
*                                               Function Prototypes
*  --------------------------------------------------------------------------*/

void vTaskSwitchOut(char* pcTaskName);
void vTaskSwitchIn(char* pcTaskName);
void vHeapCheckInTaskSwitch();
void irq_status_track(unsigned int ulIrqEnable);
void os_store_regs(int task_handle);
void os_check_regs(int task_handle);


#if defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)
uint8_t ucAllocTaskID(const char *pcTaskName);
uint8_t ucGetTaskID(const StaticTask_t * pxTcb);
uint8_t ucGetTaskIDWithName(char* pcTaskName);
char* pcGetTaskName(uint8_t ucTaskID);
void runtime_init();
void runtime_status();
void vAddTaskHeap(uint8_t ucTaskID, size_t xSize, uint8_t ucIsMalloc);
int heap_print(char* pcTaskName_t);
int runtime_print(int runtime_init_t);
#endif //defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)

#endif/*_DEBUG_CORE_H*/

