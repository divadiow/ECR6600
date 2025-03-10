/**
 * @file debug_core.c
 * @brief Kernel debug code
 * @details Including tick compensation, thread switching records, memory detection, and thread CPU utilization statistics
 * @author zhudeming
 * @date
 * @version V0.1
 * @Copyright © 2022 BEIJING ESWIN COMPUTING TECHINOLOGY CO., LTD and its affiliates (“ESWIN”).
 *          All rights reserved. Any modification, reproduction, adaption, translation,
 *          distribution is prohibited without consent.
 * @par History 1:
 *      Date:2022-8-16
 *      Version:V0.2
 *      Author:liuyong
 *      Modification:modified CPU utilization statistics
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
 *												Include files
 *  --------------------------------------------------------------------------*/
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "oshal.h"
#include "pit.h"
#include "cli.h"
#include "arch_defs.h"
#include "chip_clk_ctrl.h"
#include "debug_core.h"
#include "rtc.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

/*--------------------------------------------------------------------------
 *                                               Local Macros
 *  --------------------------------------------------------------------------*/
/** Description of the macro */
#if defined(CONFIG_TASK_IRQ_SWITCH_TRACE)
#define TASK_SWITCH_NUM    CONFIG_TASK_IRQ_SWITCH_TRACE_NUM
#define IRQ_SWITCH_NUM     CONFIG_TASK_IRQ_SWITCH_TRACE_NUM
#endif

/*--------------------------------------------------------------------------
 *                                               Local Types
 *  --------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */
struct task_switch_s
{
    char         *pcTaskName;
    unsigned int ulclk;
};

typedef struct
{
    char               pcTaskName[configMAX_TASK_NAME_LEN];
        #if defined(CONFIG_HEAP_DEBUG)
    unsigned int       ulCurrentHeap;
    unsigned int       ulMaxHeap;
        #endif
        #if defined(CONFIG_RUNTIME_DEBUG)
    unsigned long long ullRunTime;
    unsigned int       sort_flag;
        #endif
} TaskInfo_t;

typedef struct
{
    unsigned long long runtime;
    unsigned int       sort_flag;
    void (*irq_isr)(int vector);
} stats_isr_t;

struct irq_switch_s
{
    unsigned int          irq;
    unsigned int          ulclk;
    struct isr_reg_layout cur_isr_layout;
};

/*--------------------------------------------------------------------------
 *                                               Local Constants
 *  --------------------------------------------------------------------------*/
char * isrname[VECTOR_NUMINTRS] = {
    "IRQ_WIFI_CPU_SINGLE",
    "IRQ_WIFI_TICK_TIMER",
    "IRQ_WIFI_SOFT",
    "IRQ_WIFI_HOST",
    "IRQ_PIT0",
    "IRQ_PIT1",
    "IRQ_SDIO_SLAVE",
    "IRQ_WDT",
    "IRQ_GPIO",
    "IRQ_I2C",
    "IRQ_SPI2",
    "IRQ_SPI_FLASH",
    "IRQ_PCU",
    "IRQ_DMA",
    "IRQ_RTC",
    "IRQ_UART0",
    "IRQ_UART1",
    "IRQ_UART2",
    "IRQ_SW",
    "IRQ_I2S",
    "IRQ_HASH",
    "IRQ_ECC",
    "IRQ_AES",
    "IRQ_IR",
    "IRQ_SDIO_HOST",
    "IRQ_BLE",
    "IRQ_BLE_PHY",
    "IRQ_BLE_ERROR",
    "IRQ_BMC_PM",
    "IRQ_BLE_HOPPING",
    "IRQ_AUX_ADC"
};

/*--------------------------------------------------------------------------
 *                                               Local Function Prototypes
 *  --------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 *                                               Global Constants
 *  --------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 *                                               Global Variables
 *  --------------------------------------------------------------------------*/
/**  Description of global variable  */
static unsigned int   g_irq_disable_duration;
unsigned int          g_irq_disable_duration_ms;
unsigned int          num_irq_1tick;
unsigned int          tick_compensate;
char                  g_task_name[configMAX_TASK_NAME_LEN];

#if defined(CONFIG_TASK_IRQ_SWITCH_TRACE)
struct task_switch_s  xTaskSwitchStats[TASK_SWITCH_NUM];
uint8_t               ucxTaskSwitchIdx = 0;

struct irq_switch_s   xIrqSwitchStats[IRQ_SWITCH_NUM];
uint8_t               ucxIrqSwitchIdx = 0;
struct isr_reg_layout *g_cur_irq      = NULL;
#endif

#if defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)
TaskInfo_t s_xTaskInfo[MAX_TASK_NUM] = { { "main" } };
#endif

#if defined(CONFIG_RUNTIME_DEBUG)
stats_isr_t        isrstats[VECTOR_NUMINTRS] = { 0 };

unsigned int       g_isr_runtime = 0;
unsigned int       g_ulTaskSwitchedInTime;
unsigned long long g_ulTotalRunTime;
#endif

#if defined(CONFIG_TASK_IRQ_RUN_NUM)
unsigned int g_uIrqRunNum[32];
#endif


/*--------------------------------------------------------------------------
 *                                               Global Function Prototypes
 *  --------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 *                                               Function Definitions
 *  --------------------------------------------------------------------------*/

/******************************************* Tick compensation *********************************************/
void irq_status_track(unsigned int ulIrqEnable)
{
    static unsigned int irq_disable_clk = 0;
    unsigned int        cur_clk         = 0;
    unsigned int        duration        = 0;
    static unsigned int irq_dis_pit     = 0;
    unsigned int        pit_clock;

    cur_clk = drv_pit_get_tick();
    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&cur_clk);
    //cur_clk=drv_rtc_get_32K_cnt();

    if (!ulIrqEnable)
    {
        if (irq_disable_clk)
        {
            //exception condition ;todo
            return;
        }
        irq_disable_clk = cur_clk;
        drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_GET_COUNT, (unsigned int)&irq_dis_pit);
    }
    else
    {
        //duration=irq_disable_clk-cur_clk;
        #ifdef CONFIG_PSM_SURPORT
        if (psm_get_rtc_compenstion_stat())
        {
            psm_pit_compenstion();
            irq_disable_clk = 0;
            irq_dis_pit     = 0;
        }
        else
        #endif
        {
            #if 0
            if (cur_clk < irq_disable_clk)
            {
                duration = irq_disable_clk - cur_clk;
            }
            else
            {
                duration = DRV_PIT_TICK_CH_RELOAD - cur_clk + irq_disable_clk;
            }
            #endif
            if (cur_clk > irq_disable_clk)
            {
                duration = cur_clk - irq_disable_clk;
            }
            else
            {
                duration = DRV_PIT_TICK_CH_RELOAD - irq_disable_clk + cur_clk;
            }

#ifdef CONFIG_PSM_SURPORT
            unsigned int cpu_freq = psm_cpu_freq_op(false, 0);
            //If there is frequency modulation during lock interruption, pit cannot be used for tick compensation.
            if (cpu_freq)
            {
                pit_clock = cpu_freq * 1000000 / 4;
            }
            else
            {
                pit_clock = CHIP_CLOCK_APB;
            }
#else
            pit_clock = CHIP_CLOCK_APB;
#endif

            if (duration > g_irq_disable_duration)
            {
                //TaskHandle_t pxTcb;
                //pxTcb=xTaskGetCurrentTaskHandle();
                //memcpy(g_task_name, pcTaskGetName(pxTcb), configMAX_TASK_NAME_LEN-1);
                g_irq_disable_duration    = duration;
                g_irq_disable_duration_ms = duration * 1000 / pit_clock;
            }
            if ((duration > irq_dis_pit) && \
                ((duration - irq_dis_pit) >= (pit_clock / configTICK_RATE_HZ)))
            {
                num_irq_1tick++;
                void vClearTickInterrupt(void);
                vClearTickInterrupt();
                vTaskStepTick((duration - irq_dis_pit) / (pit_clock / configTICK_RATE_HZ) + 1);
                tick_compensate += (duration - irq_dis_pit) / (pit_clock / configTICK_RATE_HZ) + 1;
            }
            //os_printf(LM_CMD, LL_INFO, "pit_clock is %u, duration is %u, irq_dis_pit is %u, .\n", pit_clock, duration, irq_dis_pit);
            irq_disable_clk = 0;
            irq_dis_pit     = 0;
        }
    }
}



/******************************************* IRQ Switch Tracking *********************************************/

int irq_in(unsigned int irq, uint32_t stack)
{
#if defined(CONFIG_TASK_IRQ_RUN_NUM)
    g_uIrqRunNum[irq]++;
#endif
#if defined(CONFIG_TASK_IRQ_SWITCH_TRACE)
    if (g_cur_irq)
    {
        system_assert(0);
    }
    xIrqSwitchStats[ucxIrqSwitchIdx].irq   = irq;
    xIrqSwitchStats[ucxIrqSwitchIdx].ulclk = drv_pit_get_tick();
    g_cur_irq                              = &(xIrqSwitchStats[ucxIrqSwitchIdx].cur_isr_layout);
    g_cur_irq->sp                          = stack;
    memcpy((char*)g_cur_irq, (char *)stack, sizeof(struct isr_reg_layout) - 4);
    ucxIrqSwitchIdx = (ucxIrqSwitchIdx + 1) % IRQ_SWITCH_NUM;
#endif
    return(irq);
}
int irq_out(uint32_t stack)
{
#if defined(CONFIG_TASK_IRQ_SWITCH_TRACE)
    if (stack != g_cur_irq->sp || memcmp((char*)g_cur_irq, (char *)stack, sizeof(struct isr_reg_layout) - 4))
    {
        system_assert(0);
    }
    g_cur_irq = NULL;
#endif
    return(0);
}

void os_store_regs(int task_handle)
{
    StaticTask_t            * pxTcb      = (StaticTask_t*)task_handle;
    uint32_t                * sp         = pxTcb->pxDummy1;
    struct stack_reg_layout * reg_layout = (struct stack_reg_layout *)pxTcb->reg_dummy;

#if (configSUPPORT_FPU == 1)
    /* FPU registers */
    sp += portFPU_REGS;
#endif

#if ((configSUPPORT_IFC == 1) && (configSUPPORT_ZOL == 1) || (configSUPPORT_IFC != 1) && (configSUPPORT_ZOL != 1))
    /* Dummy word for 8-byte stack alignment */
    sp += 1;
#endif
    memcpy((char*)reg_layout, (char*)sp, sizeof(pxTcb->reg_dummy));
}

void os_check_regs(int task_handle)
{
    StaticTask_t            * pxTcb      = (StaticTask_t*)task_handle;
    uint32_t                * sp         = pxTcb->pxDummy1;
    struct stack_reg_layout * reg_layout = (struct stack_reg_layout *)pxTcb->reg_dummy;

#if (configSUPPORT_FPU == 1)
    /* FPU registers */
    sp += portFPU_REGS;
#endif

#if ((configSUPPORT_IFC == 1) && (configSUPPORT_ZOL == 1) || (configSUPPORT_IFC != 1) && (configSUPPORT_ZOL != 1))
    /* Dummy word for 8-byte stack alignment */
    sp += 1;
#endif

    if(memcmp((char*)reg_layout, (char*)sp, sizeof(pxTcb->reg_dummy)))
    {
        system_assert(0);
    }
}


/******************************************* Task Switch Tracking *********************************************/
void vTaskSwitchIn(char* pcTaskName)
{
#if defined(CONFIG_TASK_IRQ_SWITCH_TRACE)
    xTaskSwitchStats[ucxTaskSwitchIdx].pcTaskName = pcTaskName;
    xTaskSwitchStats[ucxTaskSwitchIdx].ulclk = drv_pit_get_tick();
    os_check_regs((int)xTaskGetCurrentTaskHandle());
    ucxTaskSwitchIdx = (ucxTaskSwitchIdx + 1) % TASK_SWITCH_NUM;
#endif
}

void vTaskSwitchOut(char* pcTaskName)
{
    vHeapCheckInTaskSwitch();  // Default: full memory detection is not performed, which is time-consuming
    os_store_regs((int )xTaskGetCurrentTaskHandle());
#if defined(CONFIG_RUNTIME_DEBUG)
    runtime_status();
#endif
}



/*********************************** heap debug OR task run timer debug ***************************************/

#if defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)

uint8_t ucAllocTaskID(const char *pcTaskName)
{
    uint8_t i = 0;

    taskENTER_CRITICAL();

    for (i = 1; i < MAX_TASK_NUM; i++)
    {
        if (s_xTaskInfo[i].pcTaskName[0] && 0 == memcmp(s_xTaskInfo[i].pcTaskName, pcTaskName, strlen(s_xTaskInfo[i].pcTaskName)))
        {
            break;
        }
        else if (0 == s_xTaskInfo[i].pcTaskName[0])
        {
            memcpy(s_xTaskInfo[i].pcTaskName, pcTaskName, configMAX_TASK_NAME_LEN - 1);
            s_xTaskInfo[i].pcTaskName[configMAX_TASK_NAME_LEN - 1] = '\0';
            break;
        }
    }

    taskEXIT_CRITICAL();
    if (i == MAX_TASK_NUM)
    {
        configASSERT(0);
    }
    return(i);
}

uint8_t ucGetTaskIDWithName(char* pcTaskName)
{
    uint8_t i;

    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        if (s_xTaskInfo[i].pcTaskName[0] && 0 == memcmp(s_xTaskInfo[i].pcTaskName, pcTaskName, strlen(s_xTaskInfo[i].pcTaskName)))
        {
            return(i);
        }
    }
    return(0xff);
}

uint8_t ucGetTaskID(const StaticTask_t * pxTcb)
{
    if (taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState())
    {
        return(0);
    }
    else
    {
        if (NULL == pxTcb)
        {
            pxTcb = (StaticTask_t * )xTaskGetCurrentTaskHandle();
        }
        return(pxTcb->ucDummy7_1);
    }
}

char* pcGetTaskName(uint8_t ucTaskID)
{
    return(&s_xTaskInfo[ucTaskID].pcTaskName[0]);
}


#if defined(CONFIG_HEAP_DEBUG)

void vAddTaskHeap(uint8_t ucTaskID, size_t xSize, uint8_t ucIsMalloc)
{
    if (ucIsMalloc)
    {
        s_xTaskInfo[ucTaskID].ulCurrentHeap += xSize;
        if (s_xTaskInfo[ucTaskID].ulCurrentHeap > s_xTaskInfo[ucTaskID].ulMaxHeap)
        {
            s_xTaskInfo[ucTaskID].ulMaxHeap = s_xTaskInfo[ucTaskID].ulCurrentHeap;
        }
    }
    else
    {
        s_xTaskInfo[ucTaskID].ulCurrentHeap -= xSize;
    }
}
int heap_print(char* pcTaskName_t)
{
    char         * pcTaskName = pcTaskName_t;
    unsigned int flags        = 0;
    uint8_t      i;

    flags = system_irq_save();
    vTaskSuspendAll();

    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        if (s_xTaskInfo[i].pcTaskName[0] && (!pcTaskName || strstr(s_xTaskInfo[i].pcTaskName, pcTaskName)))
        {
            os_printf(LM_CMD, LL_INFO, "Task %s:\thistory max malloc %d, current malloc %d\n", s_xTaskInfo[i].pcTaskName, s_xTaskInfo[i].ulMaxHeap, s_xTaskInfo[i].ulCurrentHeap);
            if (pcTaskName)
            {
                break;
            }
        }
    }

    extern void vTaskHeapStats(char* pcTaskName);

    vTaskHeapStats(pcTaskName);
    ( void )xTaskResumeAll();
    system_irq_restore(flags);

    return(0);
}

#endif //CONFIG_HEAP_DEBUG



#if defined(CONFIG_RUNTIME_DEBUG)

void irq_isr_register_withtrace(int vector, void *isr)
{
    isrstats[vector].irq_isr = isr;
}

void comm_irq_isr_withtrace(int vector)
{
    unsigned int isr_in;
    unsigned int isr_out;
    unsigned int runtime;

    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&isr_in);
    isr_in = drv_pit_get_tick();

    isrstats[vector].irq_isr(vector);

    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&isr_out);
    isr_out = drv_pit_get_tick();

    runtime = isr_out - isr_in;

    //if(isr_out>isr_in)
    //	runtime=isr_in+(0xFFFFFFFF-isr_out);
    //else
    //	runtime=isr_in-isr_out;

    isrstats[vector].runtime += runtime;
    g_isr_runtime            += runtime;
}

void runtime_init()
{
    int i = 0;

    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&g_ulTaskSwitchedInTime);
    g_ulTaskSwitchedInTime = drv_pit_get_tick();
    g_ulTotalRunTime       = 0;
    g_isr_runtime          = 0;

    for (i = 0; i < MAX_TASK_NUM; i++)
    {
        s_xTaskInfo[i].ullRunTime = 0;
        s_xTaskInfo[i].sort_flag  = 0;
    }

    for (i = 0; i < VECTOR_NUMINTRS; i++)
    {
        isrstats[i].runtime   = 0;
        isrstats[i].sort_flag = 0;
    }
}

void runtime_status()
{
    unsigned int  ulclk = 0;
    unsigned int  ulRunTime;
    unsigned char taskID = ucGetTaskID(NULL);

    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&ulclk);
    ulclk     = drv_pit_get_tick();
    ulRunTime = ulclk - g_ulTaskSwitchedInTime;

    //if(ulclk>g_ulTaskSwitchedInTime)
    //	ulRunTime=g_ulTaskSwitchedInTime+(0xFFFFFFFF-ulclk);
    //else
    //	ulRunTime=g_ulTaskSwitchedInTime-ulclk;

    s_xTaskInfo[taskID].ullRunTime += (ulRunTime - g_isr_runtime);
    g_ulTotalRunTime               += ulRunTime;
    g_ulTaskSwitchedInTime          = ulclk;

    g_isr_runtime = 0;
}

int runtime_print(int runtime_init_t)
{
    int i, j;

    vTaskSuspendAll();
#if 0  // liuyong delete
    os_printf(LM_CMD, LL_INFO, "- * - * - * - * - * - * -\r\n");
    for (i = 1; i < MAX_TASK_NUM; i++)
    {
        if ('\0' != s_xTaskInfo[i].pcTaskName[0])
        {
            os_printf(LM_CMD, LL_INFO, "cpucpu:%s:%lld\n", s_xTaskInfo[i].pcTaskName, s_xTaskInfo[i].ullRunTime);
            continue;
        }
        break;
    }

    for (i = 0; i < VECTOR_NUMINTRS; i++)
    {
        if (isrstats[i].runtime)
        {
            os_printf(LM_CMD, LL_INFO, "cpucpu:%s:%lld\n", isrname[i], isrstats[i].runtime);
        }
    }

    os_printf(LM_CMD, LL_INFO, "cpucpu:total runtime:%lld\n", g_ulTotalRunTime);
    os_printf(LM_CMD, LL_INFO, "- * - * - * - * - * - * -\r\n");
#endif

    os_printf(LM_CMD, LL_INFO, "%20s%12s\t%s\r\n", "- task -", "- pit clk -", "- CPU utilization -");
    int                get_max_index   = -1;
    unsigned long long get_max_runtime = 0;

    for (j = 1; j < MAX_TASK_NUM; j++)
    {
        for (i = 1; i < MAX_TASK_NUM; i++)
        {
             if ('\0' != s_xTaskInfo[i].pcTaskName[0]
                && s_xTaskInfo[i].sort_flag == 0
                && s_xTaskInfo[i].ullRunTime >= get_max_runtime)
            {
                get_max_index   = i;
                get_max_runtime = s_xTaskInfo[i].ullRunTime;
            }
        }
        if (get_max_index != -1)
        {
            os_printf(LM_CMD, LL_INFO, "%20s%12lld\t%.6f%%\n", s_xTaskInfo[get_max_index].pcTaskName, s_xTaskInfo[get_max_index].ullRunTime,
                      (float)s_xTaskInfo[get_max_index].ullRunTime * 100 / g_ulTotalRunTime);
            s_xTaskInfo[get_max_index].sort_flag = 1;
            get_max_index                        = -1;
            get_max_runtime                      = 0;
        }
    }
    os_printf(LM_CMD, LL_INFO, "%20s%12s\t%s\r\n", "- IRQ -", "- pit clk -", "- CPU utilization -");
    get_max_index   = -1;
    get_max_runtime = 0;
    for (j = 0; j < VECTOR_NUMINTRS; j++)
    {
        for (i = 0; i < VECTOR_NUMINTRS; i++)
        {
            if (isrstats[i].sort_flag == 0 && isrstats[i].runtime > get_max_runtime)
            {
                get_max_index   = i;
                get_max_runtime = isrstats[i].runtime;
            }
        }
        if (get_max_index != -1)
        {
            os_printf(LM_CMD, LL_INFO, "%20s%12lld\t%.6f%%\n", isrname[get_max_index], isrstats[get_max_index].runtime,
                      (float)isrstats[get_max_index].runtime * 100 / g_ulTotalRunTime);
            isrstats[get_max_index].sort_flag = 1;
            get_max_index                     = -1;
            get_max_runtime                   = 0;
        }
    }
    // os_printf(LM_CMD,LL_INFO,"total pit clk:%lld\n",g_ulTotalRunTime);
    if (runtime_init_t > 1)
    {
        runtime_init();
    }
    else
    {
        for (j = 1; j < MAX_TASK_NUM; j++)
        {
            s_xTaskInfo[j].sort_flag = 0;
        }
        for (j = 0; j < VECTOR_NUMINTRS; j++)
        {
            isrstats[j].sort_flag = 0;
        }
    }
    xTaskResumeAll();

    return(0);
}

#endif//CONFIG_RUNTIME_DEBUG

#endif  // defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)

