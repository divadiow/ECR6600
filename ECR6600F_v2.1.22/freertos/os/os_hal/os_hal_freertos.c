/**
 * @file os_hal_freertos.c
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

#include "oshal.h"
#include "chip_clk_ctrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "chip_clk_ctrl.h"
#include "trng.h"
#include "arch_irq.h"
#include "arch_defs.h"
#include "pit.h"
#include "telnet.h"



/**************************************************************************************
*Task API
**************************************************************************************/
static TickType_t ms_to_tick(size_t timeout_ms)
{
    TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS + ((timeout_ms % portTICK_PERIOD_MS)?1:0);

    return(xTicksToWait);
}

int os_task_create(const char *name, int priority, int stack_size, task_entry_t entry, void *const arg)
{
    TaskHandle_t task_handle;

    if (pdPASS != xTaskCreate(entry, name, stack_size / sizeof(StackType_t), arg, priority, &task_handle))
    {
        return(-1);
    }
    else
    {
        return((int)task_handle);
    }
}

int os_task_get_running_handle()
{
    return((int)xTaskGetCurrentTaskHandle());
}

int os_task_delete(int task_handle)
{
    vTaskDelete((TaskHandle_t)task_handle);
    return(0);
}
int os_task_suspend(int task_handle)
{
    vTaskSuspend((TaskHandle_t)task_handle);
    return(0);
}
int os_task_resume(int task_handle)
{
    vTaskResume((TaskHandle_t)task_handle);
    return(0);
}

int os_scheduler_lock()
{
    vTaskSuspendAll();
    return(0);
}

int os_scheduler_unlock()
{
    xTaskResumeAll();
    return(0);
}



/**************************************************************************************
*Semaphore API
**************************************************************************************/
os_sem_handle_t os_sem_create(size_t maxcount, size_t initalcount)
{
    return(xSemaphoreCreateCounting(maxcount, initalcount));
}

int os_sem_post(os_sem_handle_t sem)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int        ret;

    if (arch_irq_context())
    {
        ret = xSemaphoreGiveFromISR(( QueueHandle_t )sem, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xSemaphoreGive(( QueueHandle_t )sem);
    }

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_sem_wait(os_sem_handle_t sem, size_t timeout_ms)
{
    TickType_t xTicksToWait = portMAX_DELAY;
    int        ret;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (timeout_ms < WAIT_FOREVER)
    {
        xTicksToWait = ms_to_tick(timeout_ms);
    }

    if (arch_irq_context())
    {
        ret = xSemaphoreTakeFromISR(( QueueHandle_t )sem, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xSemaphoreTake(( QueueHandle_t )sem, xTicksToWait);
    }
    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_sem_destroy(os_sem_handle_t sem)
{
    vSemaphoreDelete(( QueueHandle_t )sem);
    return(0);
}

int os_sem_get(os_sem_handle_t sem)
{
    if (arch_irq_context())
    {
        return(uxQueueMessagesWaitingFromISR(( QueueHandle_t )sem));
    }
    else
    {
        return(uxSemaphoreGetCount(( QueueHandle_t )sem));
    }
}



/**************************************************************************************
*Mutex API
**************************************************************************************/
os_mutex_handle_t os_mutex_create()
{
    return(xQueueCreateMutex(queueQUEUE_TYPE_RECURSIVE_MUTEX));
}

int os_mutex_lock(os_mutex_handle_t mutex, size_t timeout_ms)
{
    TickType_t xTicksToWait = portMAX_DELAY;
    int        ret;

    if (timeout_ms < WAIT_FOREVER)
    {
        xTicksToWait = ms_to_tick(timeout_ms);
    }

    ret = xQueueTakeMutexRecursive((QueueHandle_t)mutex, xTicksToWait);
    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_mutex_unlock(os_mutex_handle_t mutex)
{
    int ret = xQueueGiveMutexRecursive((QueueHandle_t)mutex);

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_mutex_destroy(os_mutex_handle_t mutex)
{
    vQueueDelete((QueueHandle_t)mutex);
    return(0);
}



/**************************************************************************************
*Queue API
**************************************************************************************/
os_queue_handle_t os_queue_create(const char *name, size_t item_num, size_t item_size, size_t dummy)
{
    return(xQueueCreate(item_num, item_size));
}

int os_queue_send(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms)
{
    TickType_t xTicksToWait             = portMAX_DELAY;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int        ret;
    //ESWIN ADD for debug get Queue Spaces Available status by taodong
    int        queueSpaceStatus;

    //ESWIN END

    if (arch_irq_context())
    {
        ret = xQueueSendFromISR((QueueHandle_t)queue, msg, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        if (timeout_ms < WAIT_FOREVER)
        {
            xTicksToWait = ms_to_tick(timeout_ms);
        }
        ret = xQueueSend((QueueHandle_t)queue, msg, xTicksToWait);
    }

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        //ESWIN ADD for get Queue Spaces Available status when send return error by taodong
        queueSpaceStatus = xQueueGetSpaceStatus((QueueHandle_t)queue);
        os_printf(LM_OS, LL_INFO, "queue Space status = %d \n", queueSpaceStatus);
        //ESWIN END
        return(-1);
    }
}

int os_queue_receive(os_queue_handle_t queue, char *msg, size_t msglen, size_t timeout_ms)
{
    TickType_t xTicksToWait             = portMAX_DELAY;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int        ret;

    if (timeout_ms < WAIT_FOREVER)
    {
        xTicksToWait = ms_to_tick(timeout_ms);
    }
    if (arch_irq_context())
    {
        ret = xQueueReceiveFromISR((QueueHandle_t)queue, msg, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xQueueReceive((QueueHandle_t)queue, msg, xTicksToWait);
    }
    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_queue_destory(os_queue_handle_t queue)
{
    vQueueDelete((QueueHandle_t)queue);
    return(0);
}



/**************************************************************************************
*Memory API
**************************************************************************************/
void   os_free(void *ptr)
{
    vPortFree(ptr);
}

void * os_calloc(size_t nmemb, size_t size)
{
    return(os_zalloc(nmemb * size));
}

void * os_realloc(void *ptr, size_t size)
{
    return(pvPortReMalloc(ptr, size));
}

char * os_strdup(const char *str)
{
    char   *dup;
    size_t size = strlen(str);

    dup = os_zalloc(size + 1);
    if (dup)
    {
        memcpy(dup, str, size);
    }
    return(dup);
}

void * malloc(size_t size)
{
    return(os_malloc(size));
}

void free(void* ptr)
{
    os_free(ptr);
}

void * calloc(size_t nmemb, size_t size)
{
    return(os_calloc(nmemb, size));
}

void * realloc(void *ptr, size_t size)
{
    return(os_realloc(ptr, size));
}

char * strdup(const char *str)
{
    return(os_strdup(str));
}

#if defined(CONFIG_HEAP_DEBUG)
void * os_malloc_debug(size_t size, char*filename, unsigned int line)
{
    return(pvPortMallocDebug(size, filename, line));
}

void * os_zalloc_debug(size_t size, char*filename, unsigned int line)
{
    void *p = pvPortMallocDebug(size, filename, line);

    if (p)
    {
        memset(p, 0, size);
    }
    return(p);
}

#else
void * os_malloc_debug(size_t size, char*filename, unsigned int line)
{
    return(pvPortMalloc(size));
}

void * os_zalloc_debug(size_t size, char*filename, unsigned int line)
{
    void *p = pvPortMalloc(size);

    if (p)
    {
        memset(p, 0, size);
    }
    return(p);
}
#endif



/**************************************************************************************
*SoftTimer API
**************************************************************************************/
os_timer_handle_t os_timer_create(const char *name, size_t period_ms, size_t is_autoreload, void (*timeout_func)(os_timer_handle_t timer), void *arg)
{
    TickType_t    timer_period = ms_to_tick(period_ms);
    TimerHandle_t timer        = xTimerCreate(name, timer_period, is_autoreload, arg, (TimerCallbackFunction_t)timeout_func);

    if (is_autoreload)
    {
        vTimerSetReloadMode((TimerHandle_t)timer, pdTRUE);
    }
    return((void *)timer);
}

void * os_timer_get_arg(os_timer_handle_t timer)
{
    return(pvTimerGetTimerID((TimerHandle_t)timer));
}

int os_timer_start(os_timer_handle_t timer)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int        ret                      = 0;

    if (arch_irq_context())
    {
        ret = xTimerStartFromISR((TimerHandle_t)timer, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xTimerStart((TimerHandle_t)timer, portMAX_DELAY);
    }

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_timer_delete(os_timer_handle_t timer)
{
    int ret = xTimerDelete((TimerHandle_t)timer, portMAX_DELAY);

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_timer_stop(os_timer_handle_t timer)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int        ret                      = 0;

    if (arch_irq_context())
    {
        ret = xTimerStopFromISR((TimerHandle_t)timer, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xTimerStop((TimerHandle_t)timer, portMAX_DELAY);
    }

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}

int os_timer_changeperiod(os_timer_handle_t timer, size_t period_ms)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t xNewPeriod               = ms_to_tick(period_ms);
    int        ret                      = 0;

    if (arch_irq_context())
    {
        ret = xTimerChangePeriodFromISR((TimerHandle_t)timer, xNewPeriod, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        ret = xTimerChangePeriod((TimerHandle_t)timer, xNewPeriod, portMAX_DELAY);
    }

    if (pdPASS == ret)
    {
        return(0);
    }
    else
    {
        return(-1);
    }
}



/**************************************************************************************
*Sleep API
**************************************************************************************/
#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC    1000000L

int os_msleep(size_t ms)
{
    TickType_t xTicksToDelay = ms_to_tick(ms);

    vTaskDelay(xTicksToDelay);
    return(0);
}

long long os_time_get(void)
{
    int unit = 1000 / configTICK_RATE_HZ;
    int ret  = 0;

    if (arch_irq_context())
    {
        ret = xTaskGetTickCountFromISR() * unit;
    }
    else
    {
        ret = xTaskGetTickCount() * unit;
    }
    return(ret);
}

int os_usdelay(size_t delay_us)
{
    size_t       delay = delay_us * (CHIP_CLOCK_APB / USEC_PER_SEC);
    unsigned int flag  = system_irq_save();

    drv_pit_delay(delay);
    system_irq_restore(flag);
    return(0);
}

int os_msdelay(size_t delay_ms)
{
    size_t       delay = delay_ms * (CHIP_CLOCK_APB / MSEC_PER_SEC);
    unsigned int flag  = system_irq_save();

    drv_pit_delay(delay);
    system_irq_restore(flag);
    return(0);
}



/**************************************************************************************
*Interrupt API
**************************************************************************************/
#ifdef CONFIG_SYSTEM_IRQ
#include "rtc.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

#define SYSTEM_IRQ_EMB_MAX    20
#define SYSTEM_IRQ_NUM_MAX    400
#define SYSTEM_PIT_FREQ_1M    1000000

typedef struct
{
    unsigned int irq_time;
    unsigned int irq_line;
    unsigned int irq_func;
}system_irq;

volatile unsigned char g_sys_irq_sw                      = 0;         /*pit/rtc初始化标识*/
volatile unsigned int  g_emb_layer                       = 0;         /*嵌套层数*/
volatile unsigned int  g_emb_max                         = 0;         /*最大桥套层数*/
volatile unsigned int  g_cur_max                         = 0;         /*当前中断最大数目*/
volatile unsigned int  g_sys_over_flow                   = 0;
volatile system_irq    g_sys_irq[SYSTEM_IRQ_NUM_MAX]     = { { 0 } }; /*全局中断*/
volatile system_irq    g_emb_irq[SYSTEM_IRQ_EMB_MAX + 1] = { { 0 } }; /*最大嵌套5层*/

int system_irq_binsearch(system_irq *cur_irq, int *Find)
{
    volatile int low, high, mid;

    *Find = 0;
    low   = 0; high = g_cur_max - 1;
    while (low <= high)
    {
        mid = (low + high) >> 1;
        if (cur_irq->irq_func == g_sys_irq[mid].irq_func)
        {
            if (cur_irq->irq_line == g_sys_irq[mid].irq_line)
            {
                *Find = 1;
                return(mid);
            }
            else if (cur_irq->irq_line < g_sys_irq[mid].irq_line)
            {
                high = mid - 1;
            }
            else
            {
                low = mid + 1;
            }
        }
        else if (cur_irq->irq_func < g_sys_irq[mid].irq_func)
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }
    return(low);
}

int system_irq_update(system_irq *cur_irq)
{
    volatile int index;
    int          Find;

    /*search*/
    index = system_irq_binsearch(cur_irq, &Find);
    if (!Find && g_cur_max == SYSTEM_IRQ_NUM_MAX)
    {
        g_sys_over_flow++;
        return(-1);
    }

    if (Find)
    {
        g_sys_irq[index].irq_time = MAX(g_sys_irq[index].irq_time, cur_irq->irq_time);
        return(0);
    }

    int cnt;

    for (cnt = g_cur_max - 1; cnt >= index; cnt--)
    {
        memcpy((system_irq *)&g_sys_irq[cnt + 1], (system_irq *)&g_sys_irq[cnt], sizeof(system_irq));
    }
    memcpy((system_irq *)&g_sys_irq[index], cur_irq, sizeof(system_irq));
    g_cur_max++;
    return(0);
}

void system_irq_save_hook(unsigned int hook_func, unsigned int hook_line)
{
    if (g_sys_irq_sw)
    {
        g_emb_layer++;
        if (g_emb_layer > g_emb_max)
        {
            g_emb_max = g_emb_layer;
        }

        if (g_emb_layer <= SYSTEM_IRQ_EMB_MAX)
        {
            g_emb_irq[g_emb_layer].irq_line = hook_line;
            g_emb_irq[g_emb_layer].irq_func = hook_func;
            g_emb_irq[g_emb_layer].irq_time = drv_pit_get_tick();
        }
    }
}

void system_irq_restore_hook(void)
{
    if (g_sys_irq_sw)
    {
        unsigned int tmp_time = drv_pit_get_tick();

        if (g_emb_layer <= SYSTEM_IRQ_EMB_MAX)
        {
            g_emb_irq[g_emb_layer].irq_time = tmp_time - g_emb_irq[g_emb_layer].irq_time + 17;
            system_irq_update((system_irq *)&g_emb_irq[g_emb_layer]);
            g_emb_layer--;

            if (g_emb_layer >= 1)
            {
                g_emb_irq[g_emb_layer].irq_time += drv_pit_get_tick() - tmp_time + 100;
            }
        }
    }
}

unsigned int system_irq_save_debug(unsigned int irq_func, unsigned int irq_line)
{
    unsigned int ulPSW = portSET_INTERRUPT_MASK_FROM_ISR();

    system_irq_save_hook(irq_func, irq_line);
    return(ulPSW);
}

void system_irq_restore(unsigned int psw)
{
    system_irq_restore_hook();
    return(portCLEAR_INTERRUPT_MASK_FROM_ISR(psw));
}

unsigned int system_irq_save_tick_compensation_debug(unsigned int irq_func, unsigned int irq_line)
{
    unsigned int psw = portSET_INTERRUPT_MASK_FROM_ISR_TICK_COMPENSATION();

    system_irq_save_hook(irq_func, irq_line);
    return(psw);
}

void system_irq_restore_tick_compensation(unsigned int psw)
{
    system_irq_restore_hook();
    return(portCLEAR_INTERRUPT_MASK_FROM_ISR_TICK_COMPENSATION(psw));
}

void sys_irq_sw_set(void)
{
    g_sys_irq_sw = 1;
}

void sys_irq_show(void)
{
    int          index;
    unsigned int cpu_freq = 0;
    float        irq_time_us;

    g_sys_irq_sw = 0;
    for (index = 0; index < g_cur_max; index++)
    {
#ifdef CONFIG_PSM_SURPORT
        cpu_freq = psm_cpu_freq_op(false, 0);
#endif
        irq_time_us = cpu_freq > 0 ? (cpu_freq / 4): (CHIP_CLOCK_APB / SYSTEM_PIT_FREQ_1M);
        irq_time_us = g_sys_irq[index].irq_time / irq_time_us;
        os_printf(LM_OS, LL_INFO, "%d/%d(/pit)/%.3f(/us)/%d/%s/%d\r\n", index, g_sys_irq[index].irq_time, irq_time_us, g_sys_irq[index].irq_line, g_sys_irq[index].irq_func, g_sys_irq[index].irq_func);
    }
    os_printf(LM_OS, LL_INFO, "sys_cur_max = %d, sys_emb_max = %d, sys_over_flow=%d\r\n", g_cur_max, g_emb_max, g_sys_over_flow);
    g_sys_irq_sw = 1;
}

#else

unsigned int system_irq_save(void)
{
    return(portSET_INTERRUPT_MASK_FROM_ISR());
}
void system_irq_restore(unsigned int psw)
{
    portCLEAR_INTERRUPT_MASK_FROM_ISR(psw);
}

unsigned int        irq_os = 0;
static unsigned int irq_ble;
void ble_irq_save(void)
{
    int mask = __nds32__mfsr(NDS32_SR_INT_MASK2);

    mask &= ~((1 << VECTOR_NUM_BLE_ERROR) | (1 << VECTOR_NUM_BLE) | (1 << VECTOR_NUM_BLE_PHY));
    __nds32__mtsr(mask, NDS32_SR_INT_MASK2);
    __nds32__dsb();
    irq_ble++;
}
void ble_irq_restore(void)
{
    irq_ble--;
    if (irq_ble == 0)
    {
        int mask = __nds32__mfsr(NDS32_SR_INT_MASK2);
        mask |= ((1 << VECTOR_NUM_BLE_ERROR) | (1 << VECTOR_NUM_BLE) | (1 << VECTOR_NUM_BLE_PHY));
        __nds32__mtsr(mask, NDS32_SR_INT_MASK2);
        __nds32__dsb();
    }
}

static unsigned int irq_wifi;
static unsigned int irq_wifi_val;
void wifi_irq_save(void)
{
    volatile unsigned int mask = __nds32__mfsr(NDS32_SR_INT_MASK2);

    __nds32__mtsr(((1 << VECTOR_NUM_BLE_ERROR) | (1 << VECTOR_NUM_BLE) | (1 << VECTOR_NUM_BLE_PHY)), NDS32_SR_INT_MASK2);
    __nds32__dsb();
    if (0 == irq_wifi)
    {
        irq_wifi_val = mask;
    }
    irq_wifi++;
}

void wifi_irq_restore(void)
{
    irq_wifi--;
    if (irq_wifi == 0)
    {
        __nds32__mtsr(irq_wifi_val, NDS32_SR_INT_MASK2);
        __nds32__dsb();
    }
}

unsigned int system_irq_save_tick_compensation(void)
{
    return(portSET_INTERRUPT_MASK_FROM_ISR_TICK_COMPENSATION());
}

void system_irq_restore_tick_compensation(unsigned int psw)
{
    portCLEAR_INTERRUPT_MASK_FROM_ISR_TICK_COMPENSATION(psw);
}
#endif

int system_irq_is_enable(void)
{
#ifdef CONFIG_SYSTEM_IRQ
    return(__nds32__mfsr(NDS32_SR_PSW) & PSW_mskGIE);
#else
    return((__nds32__mfsr(NDS32_SR_PSW) & PSW_mskGIE) && (!irq_wifi));
#endif
}



/**************************************************************************************
*Log print API
**************************************************************************************/
struct log_ctrl_map g_log_map[LM_MAX] =
{
    [LM_APP]  = { "app",  LL_INFO },
    [LM_WIFI] = { "wifi", LL_INFO },
    [LM_BLE]  = { "ble",  LL_INFO },
    [LM_CMD]  = { "cmd",  LL_INFO },
    [LM_OS]   = { "os",   LL_INFO }
};

void os_printf(enum log_mod mod, enum log_level level, const char *f, ...)
{
    if (level > g_log_map[mod].level)
    {
        return;
    }
    
    va_list ap;
    va_start(ap, f);
    os_vprintf(mod, level, f, ap);
    va_end(ap);
}

void os_vprintf(enum log_mod mod, enum log_level level, const char *f, va_list args)
{
    if (level > g_log_map[mod].level)
    {
        return;
    }
    
    if (telnet_vprintf(f, args) < 0)
    {
        extern void cli_vprintf(const char *f, va_list ap);
        cli_vprintf(f, args);
    }
}



/**************************************************************************************
*Other API
**************************************************************************************/
#ifndef __OS_RANDOM__
unsigned long os_random(void)
{
    //srand(os_time_get());
    //return rand();
    return(drv_trng_get());
}
#endif

