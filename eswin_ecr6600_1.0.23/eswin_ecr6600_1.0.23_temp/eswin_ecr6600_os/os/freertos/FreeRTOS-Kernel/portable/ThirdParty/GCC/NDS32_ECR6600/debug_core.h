#ifndef __DEBUG_CORE_H__
#define __DEBUG_CORE_H__

#include "rtos_debug.h"
#include "FreeRTOS.h"

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

void vAddTaskHeap(uint8_t ucTaskID,size_t xSize, uint8_t ucIsMalloc);

int heap_print(char* pcTaskName_t);

int runtime_print(int runtime_init_t);





#endif //defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)

#endif //__DEBUG_CORE_H__
