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

#define DRV_PWM_CHN_RATIO_FLAG_GET(X)		((drv_pwm_ratio_flag & 3<<2*X)>>2*X)
#define DRV_PWM_CHN_RATIO_0_SET(X)			(drv_pwm_ratio_flag |= (1<<2*X))
#define DRV_PWM_CHN_RATIO_1000_SET(X)		(drv_pwm_ratio_flag |= (2<<2*X))
#define DRV_PWM_CHN_RATIO_CLEAR(X)			(drv_pwm_ratio_flag &= ~3<<2*X)
#define DRV_PWM_CHN_RATIO_0					1
#define DRV_PWM_CHN_RATIO_1000	 			2

#define DRV_PWM_DUTY_RATIO_MIN				0
#define DRV_PWM_DUTY_RATIO_MAX				1000
#define DRV_PWM_DUTY_RATIO_PERMILLAGE		1000

#define DRV_PWM_OUTPUT_LOW			0x00
#define DRV_PWM_OUTPUT_HIGH			0x10

#define DRV_PWM_TICK_DEFAULT			0xF

static unsigned int drv_pwm_status = 0;
static unsigned int drv_pwm_ratio_flag = 0;

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

