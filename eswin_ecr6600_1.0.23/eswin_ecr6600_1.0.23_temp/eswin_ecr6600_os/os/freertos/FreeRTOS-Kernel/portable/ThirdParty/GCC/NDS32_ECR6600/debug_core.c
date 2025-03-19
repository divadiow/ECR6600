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

static unsigned int g_irq_disable_duration;
unsigned int g_irq_disable_duration_ms;
unsigned int num_irq_1tick;
unsigned int tick_compensate;
char g_task_name[configMAX_TASK_NAME_LEN];
extern void vClearTickInterrupt( void );
void irq_status_track(unsigned int ulIrqEnable)
{
	static unsigned int irq_disable_clk=0;
	unsigned int cur_clk=0;
	unsigned int duration=0;
    static unsigned int irq_dis_pit = 0;
    unsigned int pit_clock;

    cur_clk = drv_pit_get_tick();
    //drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&cur_clk);
	//cur_clk=drv_rtc_get_32K_cnt();
	
	if(!ulIrqEnable)
	{
		if(irq_disable_clk)
		{
			//exception condition ;todo 
			return;
		}
		irq_disable_clk=cur_clk;
        drv_pit_ioctrl(DRV_PIT_CHN_6,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&irq_dis_pit);
	}
	else
	{
        //duration=irq_disable_clk-cur_clk;
        #ifdef CONFIG_PSM_SURPORT
        if(psm_get_rtc_compenstion_stat())
        {
            psm_pit_compenstion();
            irq_disable_clk=0;
            irq_dis_pit = 0;
        }
        else
        #endif
        {
            #if 0
            if (cur_clk < irq_disable_clk) {
                duration = irq_disable_clk - cur_clk;
            }
            else {
                duration = DRV_PIT_TICK_CH_RELOAD - cur_clk + irq_disable_clk;
            }
            #endif
            if (cur_clk > irq_disable_clk) {
                duration = cur_clk - irq_disable_clk;
            }
            else {
                duration = DRV_PIT_TICK_CH_RELOAD - irq_disable_clk + cur_clk;
            }

#ifdef CONFIG_PSM_SURPORT
            //If there is frequency modulation during lock interruption, pit cannot be used for tick compensation.
            if (psm_infs.cpu_freq)
                pit_clock = psm_infs.cpu_freq * 1000000;
            else
                pit_clock = CHIP_CLOCK_SOURCE;
#else
            pit_clock = CHIP_CLOCK_SOURCE;
#endif

            if(duration>g_irq_disable_duration)
            {
                //TaskHandle_t pxTcb;
                //pxTcb=xTaskGetCurrentTaskHandle();
                //memcpy(g_task_name, pcTaskGetName(pxTcb), configMAX_TASK_NAME_LEN-1);
                g_irq_disable_duration=duration;
                g_irq_disable_duration_ms = duration * 1000 / pit_clock;
            }
            if ((duration > irq_dis_pit) && \
                ((duration - irq_dis_pit) >= (pit_clock / configTICK_RATE_HZ)))
            {
                num_irq_1tick++;
                vClearTickInterrupt();
                vTaskStepTick((duration - irq_dis_pit)/(pit_clock / configTICK_RATE_HZ) + 1);
                tick_compensate += (duration - irq_dis_pit)/(pit_clock / configTICK_RATE_HZ) + 1;
            }
            //os_printf(LM_CMD, LL_INFO, "pit_clock is %u, duration is %u, irq_dis_pit is %u, .\n", pit_clock, duration, irq_dis_pit);
            irq_disable_clk=0;
            irq_dis_pit = 0;
        }
    }
}

/****************************************************************************************/
#define TASK_SWITCH_NUM 10
struct task_switch_s
{
	char *pcTaskName;
	unsigned int ulclk;
};
struct task_switch_s xTaskSwitchStats[TASK_SWITCH_NUM];

void vTaskSwitchOut(char* pcTaskName)
{
	vHeapCheckInTaskSwitch();
	os_store_regs((int )xTaskGetCurrentTaskHandle());
#if defined(CONFIG_RUNTIME_DEBUG)
	runtime_status();
#endif
}


void vTaskSwitchIn(char* pcTaskName)
{	
	static uint8_t ucxTaskSwitchIdx=0;
	xTaskSwitchStats[ucxTaskSwitchIdx].pcTaskName=pcTaskName;
	//drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&xTaskSwitchStats[ucxTaskSwitchIdx].ulclk);
	xTaskSwitchStats[ucxTaskSwitchIdx].ulclk=drv_pit_get_tick();
	os_check_regs((int)xTaskGetCurrentTaskHandle());
	ucxTaskSwitchIdx=(ucxTaskSwitchIdx+1)%TASK_SWITCH_NUM;
}



/****************************************************************************************/


#if defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)
#define MAX_TASK_NUM 32

typedef struct {
	char pcTaskName[configMAX_TASK_NAME_LEN];
	#if defined(CONFIG_HEAP_DEBUG)
		unsigned int ulCurrentHeap;
		unsigned int ulMaxHeap;
	#endif	
	#if defined(CONFIG_RUNTIME_DEBUG)
		unsigned long long ullRunTime; 
	#endif
} TaskInfo_t;

TaskInfo_t s_xTaskInfo[MAX_TASK_NUM]={{"main"}};	




uint8_t ucAllocTaskID(const char *pcTaskName)
{
	uint8_t i=0;
	
	taskENTER_CRITICAL();
	
	for (i=1;i<MAX_TASK_NUM;i++)
	{
		if(s_xTaskInfo[i].pcTaskName[0]&&0==memcmp(s_xTaskInfo[i].pcTaskName,pcTaskName,strlen(s_xTaskInfo[i].pcTaskName)))
		{
			break;
		}
		else if (0==s_xTaskInfo[i].pcTaskName[0])
		{
			memcpy(s_xTaskInfo[i].pcTaskName,pcTaskName,configMAX_TASK_NAME_LEN-1);
			s_xTaskInfo[i].pcTaskName[configMAX_TASK_NAME_LEN-1]='\0';
			break;
		}
	}
	
	taskEXIT_CRITICAL();
	if(i==MAX_TASK_NUM)
	{
		configASSERT(0);
	}
	return i;
}
uint8_t ucGetTaskIDWithName(char* pcTaskName)
{
	uint8_t i;
	for (i=0;i<MAX_TASK_NUM;i++)
	{
		if(s_xTaskInfo[i].pcTaskName[0]&&0==memcmp(s_xTaskInfo[i].pcTaskName,pcTaskName,strlen(s_xTaskInfo[i].pcTaskName)))
		{
			return i;
		}
	}
	return 0xff;
}
uint8_t ucGetTaskID(const StaticTask_t * pxTcb)
{

	if(taskSCHEDULER_NOT_STARTED==xTaskGetSchedulerState())
	{
		return 0;
	}
	else
	{
		if(NULL==pxTcb)
		{
			pxTcb=(StaticTask_t * )xTaskGetCurrentTaskHandle();
		}
		return pxTcb->ucDummy7_1;
	}
}

char* pcGetTaskName(uint8_t ucTaskID)
{
	return &s_xTaskInfo[ucTaskID].pcTaskName[0];
}



#if defined(CONFIG_HEAP_DEBUG)

void vAddTaskHeap(uint8_t ucTaskID,size_t xSize, uint8_t ucIsMalloc)
{
	
	if(ucIsMalloc)
	{
		s_xTaskInfo[ucTaskID].ulCurrentHeap+=xSize;
		if(s_xTaskInfo[ucTaskID].ulCurrentHeap>s_xTaskInfo[ucTaskID].ulMaxHeap)
		{
			s_xTaskInfo[ucTaskID].ulMaxHeap=s_xTaskInfo[ucTaskID].ulCurrentHeap;
		}
	}
	else
	{
		s_xTaskInfo[ucTaskID].ulCurrentHeap-=xSize;
	}
}
extern void vTaskHeapStats(char* pcTaskName);
int heap_print(char* pcTaskName_t)
{

	char* pcTaskName=pcTaskName_t;
	unsigned int flags=0;
	uint8_t i;
	flags=system_irq_save();
	vTaskSuspendAll();
	
	for(i=0;i<MAX_TASK_NUM;i++)
	{
		if(s_xTaskInfo[i].pcTaskName[0] && (!pcTaskName || strstr(s_xTaskInfo[i].pcTaskName,pcTaskName)))
		{
			
			os_printf(LM_CMD,LL_INFO,"Task %s:\thistory max malloc %d, current malloc %d\n",s_xTaskInfo[i].pcTaskName,s_xTaskInfo[i].ulMaxHeap,s_xTaskInfo[i].ulCurrentHeap);
			if(pcTaskName)
			{
				break;
			}
		}
			
	}
	
	vTaskHeapStats(pcTaskName);
	( void ) xTaskResumeAll();
	system_irq_restore(flags);

	return 0;
}



#endif //CONFIG_HEAP_DEBUG


#if defined(CONFIG_RUNTIME_DEBUG)
typedef struct {
	unsigned long long runtime; 
	void (*irq_isr)(int vector);
} stats_isr_t;

stats_isr_t isrstats[VECTOR_NUMINTRS]={0};	

char* isrname[VECTOR_NUMINTRS]={
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


unsigned int g_isr_runtime=0;
unsigned int g_ulTaskSwitchedInTime;
unsigned long long g_ulTotalRunTime;	



void irq_isr_register_withtrace(int vector, void *isr)
{
	isrstats[vector].irq_isr=isr;
}


void runtime_init()
{
	int i=0;
	
	//drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&g_ulTaskSwitchedInTime);
	g_ulTaskSwitchedInTime=drv_pit_get_tick();
	g_ulTotalRunTime=0;
	g_isr_runtime=0;
	
	for (i=0;i<MAX_TASK_NUM;i++)
	{
		s_xTaskInfo[i].ullRunTime=0;
		
	}
	
	for (i=0;i<VECTOR_NUMINTRS;i++)
	{
		isrstats[i].runtime=0;
	}
}

void runtime_status()
{
	unsigned int ulclk=0;
	unsigned int ulRunTime;
	unsigned char taskID=ucGetTaskID(NULL);
	
	//drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&ulclk);
	ulclk=drv_pit_get_tick();
	ulRunTime=ulclk-g_ulTaskSwitchedInTime;
	
	//if(ulclk>g_ulTaskSwitchedInTime)
	//	ulRunTime=g_ulTaskSwitchedInTime+(0xFFFFFFFF-ulclk);
	//else
	//	ulRunTime=g_ulTaskSwitchedInTime-ulclk;
	
	s_xTaskInfo[taskID].ullRunTime+=ulRunTime-g_isr_runtime;
	g_ulTotalRunTime+=ulRunTime;
	g_ulTaskSwitchedInTime=ulclk;

	g_isr_runtime=0;
}

void comm_irq_isr_withtrace(int vector)
{
	unsigned int isr_in;
	unsigned int isr_out;
	unsigned int runtime;

	//drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&isr_in);
	isr_in=drv_pit_get_tick();

	isrstats[vector].irq_isr(vector);

	//drv_pit_ioctrl(DRV_PIT_CHN_7,DRV_PIT_CTRL_GET_COUNT,(unsigned int)&isr_out);
	isr_out=drv_pit_get_tick();

	runtime=isr_out-isr_in;

	//if(isr_out>isr_in)
	//	runtime=isr_in+(0xFFFFFFFF-isr_out);
	//else
	//	runtime=isr_in-isr_out;

	isrstats[vector].runtime+=runtime;
	g_isr_runtime+=runtime;

}

int runtime_print(int runtime_init_t)
{
	int i;
	vTaskSuspendAll();
	
	for (i=1;i<MAX_TASK_NUM;i++)
	{
		if('\0'!= s_xTaskInfo[i].pcTaskName[0])
		{

			os_printf(LM_CMD,LL_INFO,"cpucpu:%s:%lld\n",s_xTaskInfo[i].pcTaskName,s_xTaskInfo[i].ullRunTime);
			
			continue;
		}
		break;
	}

	for (i=0;i<VECTOR_NUMINTRS;i++)
	{
		if(isrstats[i].runtime)
		{
			os_printf(LM_CMD,LL_INFO,"cpucpu:%s:%lld\n",isrname[i],isrstats[i].runtime);
		}
	}
		
	os_printf(LM_CMD,LL_INFO,"cpucpu:total runtime:%lld\n",g_ulTotalRunTime);
	if(runtime_init_t>1)
	{
		runtime_init();
	}
	xTaskResumeAll();

	return 0;
}


#endif//CONFIG_RUNTIME_DEBUG


#endif	// defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG)


