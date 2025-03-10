//#include "chip_irqvector.h"
//#include "chip_clk_ctrl.h"
//#include "arch_irq.h"
#include "timer.h"
#include "oshal.h"
#include "hal_timer.h"
//#include "pit.h"

int hal_timer_start(int timer_handle)
{
	return drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_START, NULL);;
}

int hal_timer_stop(int timer_handle)
{   
	return drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_STOP, NULL);;
}

int hal_timer_change_period(int timer_handle, int period_us)
{
	return drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_SET_PERIOD, &period_us);
}

int hal_timer_create(int period_us, void (* timer_callback)(void *), void * data, int one_shot)
{
	int timer_handle;	
	T_TIMER_ISR_CALLBACK  m_callback;
	m_callback.timer_callback = timer_callback;
	m_callback.timer_data = data;
	int mode = !one_shot;

	if (timer_callback == NULL)
	{
		return TIMER_RET_ERROR;
	}

	for (timer_handle = DRV_TIMER0; timer_handle < DRV_TIMER_MAX; timer_handle++)
	{
		if (drv_timer_open(timer_handle) == TIMER_RET_SUCCESS)
		{
			drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_SET_MODE, &mode);
			drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_SET_PERIOD, &period_us);
			drv_timer_ioctrl(timer_handle, DRV_TIMER_CTRL_REG_CALLBACK, &m_callback);
			return timer_handle;
		}
	}

	return TIMER_RET_ERROR;
}

int hal_timer_delete(int timer_handle)
{
	return drv_timer_close(timer_handle);
}

int hal_timer_get_current_time(int timer_handle, unsigned int *us)
{
	return drv_timer_ioctrl(timer_handle,DRV_TIMER_CALCULATE_CURRRET_TIME, us);
}


