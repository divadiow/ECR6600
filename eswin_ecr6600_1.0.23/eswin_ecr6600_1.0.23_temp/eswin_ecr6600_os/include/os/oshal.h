#ifndef __OSHAL__
#define __OSHAL__
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include "os_task_config.h"
#include "reg_macro_def.h"
#include "common_macro_def.h"


#define WAIT_FOREVER 0xffffffff


/**************************************************************************************
*线程管理API
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
*信号量API
**************************************************************************************/
typedef void* os_sem_handle_t;
os_sem_handle_t os_sem_create(size_t maxcount,size_t initalcount);
int os_sem_post(os_sem_handle_t sem);
int os_sem_wait(os_sem_handle_t sem,size_t timeout_ms);
int os_sem_destroy (os_sem_handle_t sem);
int os_sem_get(os_sem_handle_t sem);



/**************************************************************************************
*互斥量API
**************************************************************************************/
typedef void * os_mutex_handle_t;
os_mutex_handle_t os_mutex_create();
int os_mutex_lock(os_mutex_handle_t mutex, size_t timeout_ms);
int os_mutex_unlock(os_mutex_handle_t mutex);
int os_mutex_destroy(os_mutex_handle_t mutex);

/**************************************************************************************
*队列API
**************************************************************************************/
typedef void * os_queue_handle_t;
os_queue_handle_t os_queue_create(const char *name, size_t item_num, size_t item_size, size_t dummy);
int os_queue_send(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms);
int os_queue_receive(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms);
int os_queue_destory(os_queue_handle_t queue);

/**************************************************************************************
*内存管理API
**************************************************************************************/		
#if defined(CONFIG_HEAP_DEBUG)
	void *os_malloc_debug(size_t size, char *filename ,unsigned int line);	
	void * os_zalloc_debug(size_t size, char *filename ,unsigned int line);
	#define os_malloc(size) os_malloc_debug(size,__FILE__,__LINE__)
	#define os_zalloc(size) os_zalloc_debug(size,__FILE__,__LINE__)
	
#else
	void * os_malloc(size_t size);
	void * os_zalloc(size_t size);
#endif
void   os_free(void *ptr);
void * os_calloc( size_t nmemb, size_t size );
void * os_realloc(void *ptr, size_t size);
char * os_strdup(const char *str);

/**************************************************************************************
*定时器管理API
**************************************************************************************/		
typedef void * os_timer_handle_t;

os_timer_handle_t os_timer_create(const char *name, size_t period_ms, size_t is_autoreload, void (*timeout_func)(os_timer_handle_t timer), void *arg);
void * os_timer_get_arg(os_timer_handle_t timer);
int os_timer_start(os_timer_handle_t timer);
int os_timer_delete(os_timer_handle_t timer);
int os_timer_stop( os_timer_handle_t timer);
int os_timer_changeperiod(os_timer_handle_t timer, size_t period_ms);


/**************************************************************************************
*时间管理API
**************************************************************************************/
int os_msleep(size_t ms);
long long os_time_get(void);
int os_usdelay(size_t delay_us);
int os_msdelay(size_t delay_ms);

/**************************************************************************************
*中断管理API
**************************************************************************************/
void system_irq_restore(unsigned int psw);
void ble_irq_restore(void);
void wifi_irq_restore( void );
void system_irq_restore_tick_compensation(unsigned int psw);

int system_irq_is_enable(void);

#ifdef CONFIG_SYSTEM_IRQ
void system_irq_save_hook(unsigned int hook_func, unsigned int hook_line);
void system_irq_restore_hook(void);

unsigned int system_irq_save_debug(unsigned int irq_func, unsigned int irq_line);
unsigned int system_irq_save_tick_compensation_debug(unsigned int irq_func, unsigned int irq_line);

void sys_irq_show(void);
void sys_irq_sw_set(void);

#define system_irq_save()					system_irq_save_debug((unsigned int)__FUNCTION__, (unsigned int)__LINE__)
#define system_irq_save_tick_compensation()	system_irq_save_tick_compensation_debug((unsigned int)__FUNCTION__, (unsigned int)__LINE__)

#else

unsigned int system_irq_save(void);
void ble_irq_save(void);
void wifi_irq_save( void );
unsigned int system_irq_save_tick_compensation(void);
#endif


/**************************************************************************************
*log输出API
**************************************************************************************/

enum log_level 
{
	LL_NO		= 0,	///> NO log
	LL_ASSERT	= 1,	///> ASSERT log only
	LL_ERR	 	= 2,	///> Add error log
	LL_WARN		= 3,	///> Add warning log
	LL_INFO		= 4,	///> Add info log
	LL_DBG		= 5,	///> Add debug log
	LL_MAX
};
enum log_mod
{
	LM_APP		= 0,
	LM_WIFI		= 1,
	LM_BLE		= 2,
	LM_CMD		= 3,
	LM_OS		= 4,
	LM_MAX
};

struct log_ctrl_map {
	const char *mod_name;
	enum log_level 	level;
} ;

void os_printf(enum log_mod mod,  enum log_level level, const char *f, ...);
void os_vprintf(enum log_mod mod,  enum log_level level,const char *f, va_list args);

/*[Begin] [liuyong-2021/8/9] The following printing interface is not recommended */
#define system_printf(...) os_printf(LM_APP, LL_INFO, __VA_ARGS__)
#define system_vprintf(...) os_vprintf(LM_APP, LL_INFO, __VA_ARGS__)
/*[End] [liuyong-2021/8/9]*/



/**************************************************************************************
*其他API
**************************************************************************************/
unsigned long os_random(void);

#endif
