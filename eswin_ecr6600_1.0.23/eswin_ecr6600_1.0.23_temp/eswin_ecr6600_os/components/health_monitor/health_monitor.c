



#include "arch_irq.h"
#include "wdt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "hal_system.h"
#include "health_monitor.h"
#include "easyflash.h"





//#define CFG_HEALTH_MON_TASK_MAX_NUM		32
#ifdef CONFIG_HEALTH_MON_DEBUG
#define CFG_HEALTH_MON_TASK_PANIC		0
#else
#define CFG_HEALTH_MON_TASK_PANIC		1
#endif
#define CFG_HEALTH_MON_PERIOD				WDT_CTRL_INTTIME_10


#define HEALTH_MON_IDLE_TASK_ID			0

typedef struct  _T_HEALTH_MON_TASK
{
	int task_handle;
} T_HEALTH_MON_TASK;


typedef struct _T_HEALTH_MON_DEV
{
	T_HEALTH_MON_TASK task[CONFIG_HEALTH_MON_TASK_MAX_NUM];
	unsigned int task_reset_map_current;
	unsigned int task_reset_map_default;
	unsigned int mon_period;
	char panic;
	char init;
} T_HEALTH_MON_DEV;


static T_HEALTH_MON_DEV gHealthMonDev;

static void health_mon_isr(void)
{
	T_HEALTH_MON_DEV * pHealthMonDev =  &gHealthMonDev;
	int i;

	if (pHealthMonDev->task_reset_map_current == 0)
	{
		drv_wdt_feed();
		pHealthMonDev->task_reset_map_current = pHealthMonDev->task_reset_map_default;
	}
	else
	{
		hal_set_reset_type(RST_TYPE_HARDWARE_WDT_TIMEOUT);

		os_printf(LM_APP, LL_ERR, "[health_mon] error!\n");

		if (pHealthMonDev->panic)
		{
			drv_wdt_stop();

			if (pHealthMonDev->task_reset_map_current & (1<<HEALTH_MON_IDLE_TASK_ID))
			{
				os_printf(LM_APP, LL_ERR, "[health_mon] task idle is sick!\n");
			}
			
			for (i=1; i<CONFIG_HEALTH_MON_TASK_MAX_NUM; i++)
			{
				if (pHealthMonDev->task_reset_map_current & (1<<i))
				{
					os_printf(LM_APP, LL_ERR, "[health_mon] task %s is sick!\n", 
						pcTaskGetName((TaskHandle_t)pHealthMonDev->task[i].task_handle));
				}
			}
			configASSERT(0);
		}
		else
		{
			while(1);
		}
	}
}


static int health_mon_reg(int handle)
{
	T_HEALTH_MON_DEV * pHealthMonDev =  &gHealthMonDev;
	int i;
	int task_id = HM_RET_ERROR_FULL;

	for (i=1; i<CONFIG_HEALTH_MON_TASK_MAX_NUM; i++)
	{
		if (pHealthMonDev->task[i].task_handle == 0)
		{
			if (task_id == HM_RET_ERROR_FULL)
			{
				task_id = i;
			}
		} 
		else if (pHealthMonDev->task[i].task_handle == handle)
		{
			return i;
		}
	}

	if (task_id >= 0)
	{
		pHealthMonDev->task[task_id].task_handle = handle;
	}

	return task_id;
}

void vApplicationIdleHook(void)
{
	if (gHealthMonDev.init)
	{
		health_mon_update(HEALTH_MON_IDLE_TASK_ID);
	}
}

int health_mon_init(void)
{
	T_HEALTH_MON_DEV * pHealthMonDev =  &gHealthMonDev;

	unsigned long flags = system_irq_save();
	if (pHealthMonDev->init == 0)
	{
		pHealthMonDev->mon_period = CFG_HEALTH_MON_PERIOD;
		pHealthMonDev->init = 1;
		pHealthMonDev->panic = CFG_HEALTH_MON_TASK_PANIC;
		pHealthMonDev->task_reset_map_current = 1<<HEALTH_MON_IDLE_TASK_ID;
		pHealthMonDev->task_reset_map_default = 1<<HEALTH_MON_IDLE_TASK_ID;
		drv_wdt_isr_register(health_mon_isr);
		drv_wdt_start(pHealthMonDev->mon_period, WDT_CTRL_RSTTIME_6, 
			WDT_CTRL_INTENABLE, WDT_CTRL_RSTENABLE);
	}
	system_irq_restore(flags);

	return HM_RET_SUCCESS;
}


int health_mon_add(int handle)
{
	T_HEALTH_MON_DEV * pHealthMonDev =  &gHealthMonDev;
	int task_id;

	unsigned long flags = system_irq_save();
	if (!handle)
	{
		handle = os_task_get_running_handle();
		if (!handle)
		{
			system_irq_restore(flags);
			return HM_RET_ERROR_HANDLE;
		}
	}

	task_id = health_mon_reg(handle);
	if (task_id >= 0)
	{
		pHealthMonDev->task_reset_map_default |= (1<<task_id);
	}

	system_irq_restore(flags);

	return task_id;
}


int health_mon_del(int task_id)
{
	T_HEALTH_MON_DEV * pHealthMonDev =  &gHealthMonDev;
	int task_id_bit = 1 << task_id;
	int ret = HM_RET_SUCCESS;

	if (task_id == HEALTH_MON_IDLE_TASK_ID)
	{
		return HM_RET_ERROR_ID;
	}

	unsigned long flags = system_irq_save();
	if (pHealthMonDev->task_reset_map_default & task_id_bit)
	{
		pHealthMonDev->task_reset_map_default &= ~task_id_bit;
		pHealthMonDev->task_reset_map_current &= ~task_id_bit;
		pHealthMonDev->task[task_id].task_handle = 0;
	}
	else
	{
		ret = HM_RET_ERROR_ID;
	}
	system_irq_restore(flags);

	return ret;
}


int health_mon_update(int task_id)
{
	T_HEALTH_MON_DEV * pHealthMonDev = &gHealthMonDev;
	int task_id_bit = 1 << task_id;
	int ret = HM_RET_SUCCESS;

	unsigned long flags = system_irq_save();
	if (pHealthMonDev->task_reset_map_default & task_id_bit)
	{
		pHealthMonDev->task_reset_map_current &= ~task_id_bit;
	}
	else
	{
		ret = HM_RET_ERROR_ID;
	}
	system_irq_restore(flags);

	return ret;
}

int health_mon_start(void)
{
	T_HEALTH_MON_DEV * pHealthMonDev = &gHealthMonDev;

	unsigned long flags = system_irq_save();
	pHealthMonDev->task_reset_map_current = pHealthMonDev->task_reset_map_default;
	drv_wdt_start(pHealthMonDev->mon_period, WDT_CTRL_RSTTIME_6, 
		WDT_CTRL_INTENABLE, WDT_CTRL_RSTENABLE);
	system_irq_restore(flags);

	return HM_RET_SUCCESS;
}


int health_mon_stop(void)
{
	unsigned long flags = system_irq_save();
	drv_wdt_stop();
	system_irq_restore(flags);
	return HM_RET_SUCCESS;
}


int health_mon_create_by_nv(void)
{
	char value[16] = {0};
	char ret = 0;
	if((ret = develop_get_env_blob("WdtFlag", value, 1, NULL) ) == 0)
	{
		os_printf(LM_CMD, LL_ERR,"DEVELOP NV NO WdtFlag !!\n");
		return HM_RET_ERROR;
	}
	else if(ret == 0xffffffff)
	{
		os_printf(LM_CMD, LL_ERR,"NV ERROR !!\n");
		return HM_RET_ERROR;
	}
	else
	{
		if('1' == value[0])
		{
			os_printf(LM_CMD, LL_INFO,"health_monitor ready\n");
			health_mon_init();
			health_mon_start();
		}
		else
		{
			os_printf(LM_CMD, LL_INFO,"health_monitor not ready\n");
			return HM_RET_ERROR;
		}
	}
	return HM_RET_SUCCESS;
}



int health_mon_debug(int reset)
{
	unsigned long flags = system_irq_save();
	gHealthMonDev.panic = !reset;
	system_irq_restore(flags);
	return HM_RET_SUCCESS;
}

