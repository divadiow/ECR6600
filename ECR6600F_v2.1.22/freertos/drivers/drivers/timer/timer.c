/**
* @file       timer.c
* @brief      timer driver code  
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include "chip_irqvector.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "timer.h"
#include "oshal.h"


#define DRV_TIMER_STS_SET(c, s)			((s) |= (1<<(c)))
#define DRV_TIMER_STS_CLEAR(c, s)		((s) &= (~(1<<(c))))
#define DRV_TIMER_STS_GET(c, s)			(((s)>>(c)) & 1)

#define DRV_TIMER_CNT_PER_US			(CHIP_CLOCK_APB/1000/1000)
#define DRV_TIMER_US_TO_CNT(t)			((t) * DRV_TIMER_CNT_PER_US)


#define DRV_TIMER_FREQ_TO_CNT(f)		(CHIP_CLOCK_APB/(f))

#define DRV_TIMER_PERIOD_MAX			(0xFFFFFFFF / DRV_TIMER_CNT_PER_US)
#define DRV_TIMER_FREQ_MAX			(CHIP_CLOCK_APB)

/**
 * @brief Timer interrupt structure.
 */
typedef struct _T_TIMER_ISR
{
	unsigned int reload;
	void (* callback)(void * );
	void * user_data;
} T_TIMER_ISR;

/**
 * @brief Timer device.
 */
typedef struct _T_DRV_TIMER_DEV
{
	unsigned char init_irq;
	unsigned char status;
	T_TIMER_ISR isr[DRV_TIMER_MAX];
	unsigned int period[DRV_TIMER_MAX];
} T_DRV_TIMER_DEV;

static T_DRV_TIMER_DEV drv_timer_dev;



/**    @brief       Pit timer isr.
 *     @details     Pit timer interrupt mode.
 */
static void drv_timer_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	T_DRV_TIMER_DEV * p_timer_dev = &drv_timer_dev;
	int chn_timer, status;
	
	for (chn_timer = DRV_PIT_CHN_0; chn_timer < DRV_TIMER_MAX; chn_timer++)
	{
		T_TIMER_ISR * p_isr = &p_timer_dev->isr[chn_timer];

		drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_STATUS_GET, (unsigned int)&status);
		if (status)
		{
			p_isr->callback(p_isr->user_data);
			if (p_isr->reload == 0)
			{
				drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
				DRV_TIMER_STS_CLEAR(chn_timer, p_timer_dev->status);
			}
			drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_STATUS_CLEAN, 0);
		}
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

/**    @brief		Open pit timer.
*	   @details 	Open and register the related interrupt of pit timer.
*	   @param[in]	chn_timer    Specifies the channel number to open 
*/
int drv_timer_open(unsigned int chn_timer)
{
	unsigned int flags;
	T_DRV_TIMER_DEV * p_timer_dev;

	if (chn_timer > DRV_TIMER3)
	{
		return TIMER_RET_EINVAL;
	}

	p_timer_dev = &drv_timer_dev;

	flags = system_irq_save();

	if (p_timer_dev->init_irq == 0)
	{
		arch_irq_register(VECTOR_NUM_PIT0, (void_fn)drv_timer_isr);
		arch_irq_clean(VECTOR_NUM_PIT0);
		arch_irq_unmask(VECTOR_NUM_PIT0);
		p_timer_dev->init_irq = 1;
	}

	if (drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_ALLOC, 0))
	{
	 	system_irq_restore(flags);
		return TIMER_RET_ERROR;
	}


	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_TIMER);
	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_STATUS_CLEAN, 0);
	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_ENABLE, DRV_PIT_INTR_ENABLE);

 	system_irq_restore(flags);

	return TIMER_RET_SUCCESS;
}

/**    @brief		Close pit timer.
*	   @details 	Disable pit timer channel and pit timer interrupt.
*	   @param[in]	chn_timer    Specifies the channel number to open 
*/
int drv_timer_close(unsigned int chn_timer)
{
	unsigned int flags;
	T_DRV_TIMER_DEV * p_timer_dev;

	if (chn_timer > DRV_TIMER3)
	{
		return TIMER_RET_EINVAL;
	}

	p_timer_dev = &drv_timer_dev;

	flags = system_irq_save();

	if (DRV_TIMER_STS_GET(chn_timer, p_timer_dev->status))
	{
		drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
		DRV_TIMER_STS_CLEAR(chn_timer, p_timer_dev->status);
	}

	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_ENABLE, DRV_PIT_INTR_DISABLE);
	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_INTR_STATUS_CLEAN, 0);
	drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_RELEASE, 0);

 	system_irq_restore(flags);
	return TIMER_RET_SUCCESS;
}

/**    @brief		Handle pit timer different events.
*	   @details 	The events handled include enable pit timer, disable pit timer, enable pit channel interrupt, 
                    config timer interrupt mode, set pit timer period and frequency, config interrupt callback function.
*	   @param[in]	chn_timer    Specifies the channel number to open
*	   @param[in]	event       Control event type
*	   @param[in]	* arg    Pointer to control parameters, the specific meaning is determined according to the event
*	   @return  	ret=0--Handle event succeed, ret=other--Handle event failed
*/
int drv_timer_ioctrl(unsigned int chn_timer, int event, void * arg)
{
	unsigned int flags, val;
	unsigned int curr_count = 0;
	T_DRV_TIMER_DEV * p_timer_dev = &drv_timer_dev;
	int ret = TIMER_RET_SUCCESS;

	if (chn_timer > DRV_TIMER3)
	{
		return TIMER_RET_EINVAL;
	}

	flags = system_irq_save();
	switch (event)
	{
		case DRV_TIMER_CTRL_START:
			if (!DRV_TIMER_STS_GET(chn_timer, p_timer_dev->status))
			{
				drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_TIMER);
				DRV_TIMER_STS_SET(chn_timer, p_timer_dev->status);
			}
			break;

		case DRV_TIMER_CTRL_STOP:
			if (DRV_TIMER_STS_GET(chn_timer, p_timer_dev->status))
			{
				drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
				DRV_TIMER_STS_CLEAR(chn_timer, p_timer_dev->status);
			}
			break;

		case DRV_TIMER_CTRL_SET_MODE:
			p_timer_dev->isr[chn_timer].reload = *((unsigned int *)arg);
			break;

		case DRV_TIMER_CTRL_SET_PERIOD:
			val = *((unsigned int *)arg);
			if ((val == 0) || (val > DRV_TIMER_PERIOD_MAX))
			{
				ret = TIMER_RET_ERROR;
			}
			else
			{
				drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_SET_COUNT, DRV_TIMER_US_TO_CNT(val) -1);
				p_timer_dev->period[chn_timer] = val;
			}
			break;

		case DRV_TIMER_CTRL_SET_FREQ:
			val = *((unsigned int *)arg);
			if ((val == 0) || (val > DRV_TIMER_FREQ_MAX))
			{
				ret = TIMER_RET_ERROR;
			}
			else
			{
				drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_SET_COUNT, DRV_TIMER_FREQ_TO_CNT(val) -1);
			}
			break;

		case DRV_TIMER_CTRL_REG_CALLBACK:
			if (!arg)
			{
				ret = TIMER_RET_ERROR;
			}
			else
			{
				T_TIMER_ISR_CALLBACK * p_callback;
				p_callback = (T_TIMER_ISR_CALLBACK * )arg;
				p_timer_dev->isr[chn_timer].callback = p_callback->timer_callback;
				p_timer_dev->isr[chn_timer].user_data = p_callback->timer_data;
			}
			break;
		case DRV_TIMER_CALCULATE_CURRRET_TIME:	
			drv_pit_ioctrl(chn_timer, DRV_PIT_CTRL_GET_COUNT, (unsigned int)&curr_count);
			*((unsigned int *)arg) = (DRV_TIMER_US_TO_CNT(p_timer_dev->period[chn_timer]) - curr_count -1)/DRV_TIMER_CNT_PER_US;
			break;

	}
 	system_irq_restore(flags); 

	return ret;
}

