/**
* @file       pwm.c
* @brief      pwm driver code  
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "pwm.h"
#include "oshal.h"
#include "cli.h"
#include "oshal.h"
#include "chip_pinmux.h"
#include "config.h"
#include "psm_system.h"
#define DRV_PWM_HIGH_TICK_OFFSET		16
#define DRV_PWM_CHN_COUNT(h, l)		(((h)<<DRV_PWM_HIGH_TICK_OFFSET) | (l))

#define DRV_PWM_CHN_WORKING(X)		(drv_pwm_status & (1<<(X)))
#define DRV_PWM_CHN_STS_SET(X)		(drv_pwm_status |= (1<<(X)))
#define DRV_PWM_CHN_STS_CLEAR(X)		(drv_pwm_status &= ~(1<<(X)))

#define DRV_PWM_CHN_RATIO_FLAG_GET(X)		((drv_pwm_ratio_flag[X] & 3<<2*X)>>2*X)
#define DRV_PWM_CHN_RATIO_0_SET(X)			(drv_pwm_ratio_flag[X] |= (1<<2*X))
#define DRV_PWM_CHN_RATIO_1000_SET(X)		(drv_pwm_ratio_flag[X] |= (2<<2*X))
#define DRV_PWM_CHN_RATIO_CLEAR(X)			(drv_pwm_ratio_flag[X] &= ~3<<2*X)
#define DRV_PWM_CHN_RATIO_0					1
#define DRV_PWM_CHN_RATIO_1000	 			2

#define DRV_PWM_DUTY_RATIO_MIN				0
#define DRV_PWM_DUTY_RATIO_MAX				1000
#define DRV_PWM_DUTY_RATIO_PERMILLAGE		1000

#define DRV_PWM_OUTPUT_LOW			0x00
#define DRV_PWM_OUTPUT_HIGH			0x10

#define DRV_PWM_TICK_DEFAULT			0xF

static unsigned int drv_pwm_status = 0;
//static unsigned int drv_pwm_ratio_flag = 0;
static unsigned int drv_pwm_ratio_flag[DRV_PWM_MAX];


static unsigned short tick_high_num[DRV_PWM_MAX];
static unsigned short tick_low_num[DRV_PWM_MAX];

/**    @brief		Open pwm.
*	   @details 	Open the channel in use, config APB clock and pwm mode.
*	   @param[in]	chn_pwm    Specifies the channel number to open 
*/
int drv_pwm_open(unsigned int chn_pwm)
{    
	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if (drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_ALLOC, 0))
	{
		return PWM_RET_ERROR;
	}

	drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM);

	return PWM_RET_SUCCESS;
}

/**    @brief		Enable pwm.
*	   @param[in]	chn_pwm    Specifies the channel number to start
*	   @return  	0--Enable pwm succeed, -1--Enable pwm failed
*/
int drv_pwm_start(unsigned int chn_pwm)
{
	unsigned int flags;

	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ( DRV_PWM_CHN_WORKING(chn_pwm))
	{
		return PWM_RET_SUCCESS;
	}

	flags = system_irq_save();

#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_PWM, PSM_DEVICE_STATUS_ACTIVE);
#endif

	DRV_PWM_CHN_STS_SET(chn_pwm);	

	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_0)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_1000)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
	}
	else
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm], tick_low_num[chn_pwm]));
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
	}
	system_irq_restore(flags); 
	
	return PWM_RET_SUCCESS;
}

/**    @brief		Disable pwm.
*	   @param[in]	chn_pwm    Specifies the channel number to stop
*	   @return  	0--Disable pwm succeed, -1--Disable pwm failed
*/
int drv_pwm_stop(unsigned int chn_pwm)
{
	unsigned int flags;

	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ( !DRV_PWM_CHN_WORKING(chn_pwm))
	{
		return PWM_RET_SUCCESS;
	}

	flags = system_irq_save();
	drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
	drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	DRV_PWM_CHN_STS_CLEAR(chn_pwm);
#ifdef CONFIG_PSM_SURPORT
	if(drv_pwm_status == 0)	
	{
		psm_set_device_status(PSM_DEVICE_PWM, PSM_DEVICE_STATUS_IDLE);
	}
#endif
	
	system_irq_restore(flags); 

	return PWM_RET_SUCCESS;
}

/**    @brief		pwm update.
*	   @param[in]	chn_pwm    Specifies the channel number to update
*	   @return  	0--Update pwm succeed, -1--Update pwm failed
*/
int drv_pwm_update(unsigned int chn_pwm)
{
	unsigned int flags;
	
	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ( !DRV_PWM_CHN_WORKING(chn_pwm))
	{
		return PWM_RET_SUCCESS;
	}

	flags = system_irq_save();
	
	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_0)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_1000)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	}
	else
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm], tick_low_num[chn_pwm]));
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
	}
	
	system_irq_restore(flags); 
	return PWM_RET_SUCCESS;
}


/**    @brief		Config pwm.
*	   @param[in]	chn_pwm    Specifies the channel number to config
*	   @param[in]	freq    Desired pwm frequency
*	   @param[in]	duty_ratio    Ratio of pulse width to cycle time
*	   @return  	0--Config reload values succeed, -1--Config reload values failed
*/
int drv_pwm_config(unsigned int chn_pwm, unsigned int freq, unsigned int duty_ratio)
{
	unsigned int flags;

	if ((chn_pwm > DRV_PWM_CHN5) || (freq == 0) || (duty_ratio >  DRV_PWM_DUTY_RATIO_MAX))
	{
		return PWM_RET_EINVAL;
	}

	flags = system_irq_save();
	DRV_PWM_CHN_RATIO_CLEAR(chn_pwm);
	if (duty_ratio == DRV_PWM_DUTY_RATIO_MIN)
	{
		DRV_PWM_CHN_RATIO_0_SET(chn_pwm);
	}
	else if (duty_ratio == DRV_PWM_DUTY_RATIO_MAX)
	{
		DRV_PWM_CHN_RATIO_1000_SET(chn_pwm);
	}
	else
	{
		int high = (int)(CHIP_CLOCK_APB /DRV_PWM_DUTY_RATIO_PERMILLAGE*duty_ratio/freq);
		int low = (int)(CHIP_CLOCK_APB /freq - high);
		--high;
		--low;
		if ((high > 0xffff) || (low > 0xffff) || (high < 0) || (low < 0))
		{
			system_irq_restore(flags);
			return PWM_RET_ERROR;
		}

		tick_high_num[chn_pwm] = (unsigned short)(high);
		tick_low_num[chn_pwm] = (unsigned short)(low);
	}
	system_irq_restore(flags); 
	
	return PWM_RET_SUCCESS;
}

/**    @brief		Close pwm.
*	   @details 	Close the pwm channel that has been opened.
*	   @param[in]	chn_pwm  Specifies the channel number to close 
*/
int drv_pwm_close(unsigned int chn_pwm)
{
	if (drv_pwm_stop(chn_pwm) == PWM_RET_SUCCESS)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_RELEASE, 0);
		return PWM_RET_SUCCESS;
	}

	return PWM_RET_ERROR;
}
#if 0
int drv_pwm_mutex_start(unsigned int chn_pwm_start, unsigned int chn_pwm_mutex)
{
	unsigned int flags;

	if (chn_pwm_start > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ( DRV_PWM_CHN_WORKING(chn_pwm_start))
	{
		return PWM_RET_SUCCESS;
	}

	if ( !DRV_PWM_CHN_WORKING(chn_pwm_mutex))
	{
		drv_pwm_start(chn_pwm_start);
		return PWM_RET_SUCCESS;
	}

	flags = system_irq_save();

#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_PWM, PSM_DEVICE_STATUS_ACTIVE);
#endif

	DRV_PWM_CHN_STS_SET(chn_pwm_start);	

	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm_start)) == DRV_PWM_CHN_RATIO_0)
	{
		drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
		system_irq_restore(flags); 
		return PWM_RET_SUCCESS;
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm_start)) == DRV_PWM_CHN_RATIO_1000)
	{
		drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
		system_irq_restore(flags); 
		return PWM_RET_SUCCESS;
	}
	else if(((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm_mutex)) == DRV_PWM_CHN_RATIO_0)||((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm_mutex)) == DRV_PWM_CHN_RATIO_1000))
	{
		drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm_start], tick_low_num[chn_pwm_start]));
		system_irq_restore(flags); 
		return drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
	}
	
	drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm_start], tick_low_num[chn_pwm_start]));
	system_irq_restore(flags); 

	return drv_pit_ioctrl(chn_pwm_start, DRV_PIT_CTRL_PWM_MUTEX_START, chn_pwm_mutex);
}
#endif
int drv_pwm_mutex_start_check(unsigned int chn_mutex_pwm)
{
	if(chn_mutex_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_mutex_pwm)) == DRV_PWM_CHN_RATIO_0)
	{
		drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
		drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
		return 0;
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_mutex_pwm)) == DRV_PWM_CHN_RATIO_1000)
	{
		drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
		drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
		return 1;
	}
	else
	{
		drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_mutex_pwm], tick_low_num[chn_mutex_pwm]));
		return 2;
	}
}


int drv_pwm_mutex_start(unsigned int chn_pwm,unsigned int chn_mutex_pwm)
{
	unsigned int flags;

	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	flags = system_irq_save();

#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_PWM, PSM_DEVICE_STATUS_ACTIVE);
#endif

	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_0)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
		if(drv_pwm_mutex_start_check(chn_mutex_pwm) == 2)
		{
			drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
		}
		//os_printf(LM_CMD, LL_ERR, "%d\n",  __LINE__);
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_1000)
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
		if(drv_pwm_mutex_start_check(chn_mutex_pwm) == 2)
		{
			drv_pit_ioctrl(chn_mutex_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
		}
		//os_printf(LM_CMD, LL_ERR, "%d\n",  __LINE__);
	}
	else
	{
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm], tick_low_num[chn_pwm]));
		if(drv_pwm_mutex_start_check(chn_mutex_pwm) == 2)
		{
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
			drv_pit_mutex(chn_pwm, chn_mutex_pwm,DRV_PIT_CH_MODE_ENABLE_PWM);
		}
		else
		{
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
		}
		//os_printf(LM_CMD, LL_ERR, "%d\n",  __LINE__);
	}	
	system_irq_restore(flags); 
	
	return PWM_RET_SUCCESS;
}

int pwm_ratio_0, pwm_ratio_1000;
 int drv_pwm_polarity_set(unsigned int chn_pwm, PWM_POLARITY_E polarity)
{
	unsigned int flags;
	unsigned short tick_mediate_val;
	if (chn_pwm > DRV_PWM_CHN5)
	{
		return PWM_RET_EINVAL;
	}

	if ( !DRV_PWM_CHN_WORKING(chn_pwm))
	{
		return PWM_RET_SUCCESS;
	}

	flags = system_irq_save();
	
	if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_0)
	{
		if((polarity == PWM_NEGATIVE) && (pwm_ratio_0== 0))
		{	
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
			pwm_ratio_0 ++;
		}
		else
		{
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
			pwm_ratio_0 = 0;
		}
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	}
	else if ((DRV_PWM_CHN_RATIO_FLAG_GET(chn_pwm)) == DRV_PWM_CHN_RATIO_1000)
	{
		if((polarity == PWM_NEGATIVE) && (pwm_ratio_1000 == 0))
		{
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_LOW);
			pwm_ratio_1000 ++;		
		}
		else
		{
			drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_PWM | DRV_PWM_OUTPUT_HIGH);
			pwm_ratio_1000 = 0;
		}
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	}
	else
	{
		if(polarity == PWM_NEGATIVE)
		{
			tick_mediate_val = tick_high_num[chn_pwm];
			tick_high_num[chn_pwm] = tick_low_num[chn_pwm];
			tick_low_num[chn_pwm] = tick_mediate_val;
		}
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_SET_COUNT, DRV_PWM_CHN_COUNT(tick_high_num[chn_pwm], tick_low_num[chn_pwm]));
		drv_pit_ioctrl(chn_pwm, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_PWM);
	}
	
	system_irq_restore(flags); 
	return PWM_RET_SUCCESS;
}

