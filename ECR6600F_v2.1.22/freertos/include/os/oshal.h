/**
 * @file OSHAL
 * @brief Operating system interface provided by eswin
 * @details This is the detail description
 * @author liuyong
 * @date 2022-9-14
 * @version V0.1
 * @Copyright © 2022 BEIJING ESWIN COMPUTING TECHINOLOGY CO., LTD and its affiliates (“ESWIN”).
 * 			All rights reserved. Any modification, reproduction, adaption, translation,
 * 			distribution is prohibited without consent.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */

#ifndef __OSHAL__
#define __OSHAL__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "os_task_config.h"
#include "reg_macro_def.h"
#include "common_macro_def.h"



/**************************************************************************************
*Macros
**************************************************************************************/
#define WAIT_FOREVER    0xffffffff



#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************
*Task API
**************************************************************************************/
typedef void (*task_entry_t)(void *arg);
int os_task_create(const char *name, int priority, int stack_size, task_entry_t entry, void *const arg);
int os_task_get_running_handle();
int os_task_delete(int task_handle);
int os_task_suspend(int task_handle);
int os_task_resume(int task_handle);
int os_scheduler_lock();
int os_scheduler_unlock();



/**************************************************************************************
*Semaphore API
**************************************************************************************/
typedef void* os_sem_handle_t;
os_sem_handle_t os_sem_create(size_t maxcount, size_t initalcount);
int os_sem_post(os_sem_handle_t sem);
int os_sem_wait(os_sem_handle_t sem, size_t timeout_ms);
int os_sem_destroy(os_sem_handle_t sem);
int os_sem_get(os_sem_handle_t sem);



/**************************************************************************************
*Mutex API
**************************************************************************************/
typedef void * os_mutex_handle_t;
os_mutex_handle_t os_mutex_create();
int os_mutex_lock(os_mutex_handle_t mutex, size_t timeout_ms);
int os_mutex_unlock(os_mutex_handle_t mutex);
int os_mutex_destroy(os_mutex_handle_t mutex);



/**************************************************************************************
*Queue API
**************************************************************************************/
typedef void * os_queue_handle_t;
os_queue_handle_t os_queue_create(const char *name, size_t item_num, size_t item_size, size_t dummy);
int os_queue_send(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms);
int os_queue_receive(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms);
int os_queue_destory(os_queue_handle_t queue);



/**************************************************************************************
*Memory API
**************************************************************************************/
void * os_malloc_debug(size_t size, char *filename, unsigned int line);
void * os_zalloc_debug(size_t size, char *filename, unsigned int line);
#if defined(CONFIG_HEAP_DEBUG)
    #define os_malloc(size)    os_malloc_debug(size, __FILE__, __LINE__)
    #define os_zalloc(size)    os_zalloc_debug(size, __FILE__, __LINE__)
#else
    #define os_malloc(size)    os_malloc_debug(size, (char*)0, 0)
    #define os_zalloc(size)    os_zalloc_debug(size, (char*)0, 0)
#endif
void   os_free(void *ptr);
void * os_calloc(size_t nmemb, size_t size);
void * os_realloc(void *ptr, size_t size);
char * os_strdup(const char *str);



/**************************************************************************************
*SoftTimer API
**************************************************************************************/
typedef void * os_timer_handle_t;
os_timer_handle_t os_timer_create(const char *name, size_t period_ms, size_t is_autoreload, void (*timeout_func)(os_timer_handle_t timer), void *arg);
void * os_timer_get_arg(os_timer_handle_t timer);
int os_timer_start(os_timer_handle_t timer);
int os_timer_delete(os_timer_handle_t timer);
int os_timer_stop(os_timer_handle_t timer);
int os_timer_changeperiod(os_timer_handle_t timer, size_t period_ms);



/**************************************************************************************
*Sleep API
**************************************************************************************/
int os_msleep(size_t ms);
long long os_time_get(void);
int os_usdelay(size_t delay_us);
int os_msdelay(size_t delay_ms);



/**************************************************************************************
*Interrupt API
**************************************************************************************/
void system_irq_restore(unsigned int psw);
void ble_irq_restore(void);
void wifi_irq_restore(void);
void system_irq_restore_tick_compensation(unsigned int psw);
int system_irq_is_enable(void);

#ifdef CONFIG_SYSTEM_IRQ
void system_irq_save_hook(unsigned int hook_func, unsigned int hook_line);
void system_irq_restore_hook(void);
unsigned int system_irq_save_debug(unsigned int irq_func, unsigned int irq_line);
unsigned int system_irq_save_tick_compensation_debug(unsigned int irq_func, unsigned int irq_line);
void sys_irq_show(void);
void sys_irq_sw_set(void);
#define system_irq_save()                      system_irq_save_debug((unsigned int)__FUNCTION__, (unsigned int)__LINE__)
#define system_irq_save_tick_compensation()    system_irq_save_tick_compensation_debug((unsigned int)__FUNCTION__, (unsigned int)__LINE__)
#else
unsigned int system_irq_save(void);
void ble_irq_save(void);
void wifi_irq_save(void);
unsigned int system_irq_save_tick_compensation(void);
#endif



/**************************************************************************************
*Assert API
**************************************************************************************/
#define system_assert( x )       if( ( x ) == 0 ) { __nds32__gie_dis(); asm("trap 0"); for( ;; ); }



/**************************************************************************************
*Log print API
**************************************************************************************/
enum log_level
{
    LL_NO     = 0,              ///> NO log
    LL_ASSERT = 1,              ///> ASSERT log only
    LL_ERR    = 2,              ///> Add error log
    LL_WARN   = 3,              ///> Add warning log
    LL_INFO   = 4,              ///> Add info log
    LL_DBG    = 5,              ///> Add debug log
    LL_MAX
};
enum log_mod
{
    LM_APP  = 0,
    LM_WIFI = 1,
    LM_BLE  = 2,
    LM_CMD  = 3,
    LM_OS   = 4,
    LM_MAX
};

struct log_ctrl_map
{
    const char *   mod_name;
    enum log_level level;
};

void os_printf(enum log_mod mod, enum log_level level, const char *f, ...);
void os_vprintf(enum log_mod mod, enum log_level level, const char *f, va_list args);

/*[Begin] [liuyong-2021/8/9] The following printing interface is not recommended */
#ifndef system_printf
#define system_printf(...)            os_printf(LM_APP, LL_INFO, __VA_ARGS__)
#endif
#ifndef system_vprintf
#define system_vprintf(...)           os_vprintf(LM_APP, LL_INFO, __VA_ARGS__)
#endif
#ifndef printf
#define printf(format, ...)           os_printf(LM_APP, LL_INFO, format, ## __VA_ARGS__)
#endif
#ifndef fprintf
#define fprintf(file, format, ...)    os_printf(LM_APP, LL_INFO, format, ## __VA_ARGS__)
#endif
/*[End] [liuyong-2021/8/9]*/



/**************************************************************************************
*Other API
**************************************************************************************/
unsigned long os_random(void);



#ifdef __cplusplus
}
#endif



#endif

