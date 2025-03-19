#ifndef __RTOS_DEBUG_H__
#define __RTOS_DEBUG_H__


#define system_assert( x )       if( ( x ) == 0 ) { __nds32__gie_dis(); asm("trap 0"); for( ;; ); }


void vApplicationStackOverflowCheck(int task_handle);	


void trap_Heap_Error(int task_handle,unsigned char error_type);





#endif //__RTOS_DEBUG_H__
