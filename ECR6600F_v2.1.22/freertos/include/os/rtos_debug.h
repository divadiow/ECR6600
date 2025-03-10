#ifndef __RTOS_DEBUG_H__
#define __RTOS_DEBUG_H__


void vApplicationStackOverflowCheck(int task_handle);	


void trap_Heap_Error(int task_handle,unsigned char error_type);





#endif //__RTOS_DEBUG_H__
