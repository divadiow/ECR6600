/**
* @file       pit.c
* @brief      pit driver code  
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
#include "pit.h"
#include "arch_cpu.h"
#include "oshal.h"
#include "arch_irq.h"
#include "chip_irqvector.h"


#define DRV_PIT_NUM_0		0
#define DRV_PIT_NUM_1		1

#define DRV_PIT_CH_NUM		4	 ///< one pit  has a maximum of 4 channels

#define DRV_PIT_WIFI_CH_NUM		2
#define DRV_PIT_TICK_CH_NUM		3

#define DRV_PIT_CH_OFFSET			4
#define DRV_PIT_APB_CLOCK			8
#define DRV_PIT_PWM_PARK			0x00000010

/**
 * @brief Pit register map.
 */
typedef  struct _T_PIT_REG_MAP
{
	volatile unsigned int IdRev;	    ///< 0x00, ID and Revision Register
	volatile unsigned int Resv0[3];		///< 0x04~0x0C, Reserved 
	volatile unsigned int Cfg;			///< 0x10, Hardware Configure Register
	volatile unsigned int IntEn;		///< 0x14, Interrupt enable register
	volatile unsigned int IntSt;		///< 0x18, Interrupt status register
	volatile unsigned int ChEn;			///< 0x1C, Channel enable register

	struct 
	{
		volatile unsigned int ChCtrl;	///<  0x20 + n*10, Channel control register
		volatile unsigned int ChReload;	///<  0x24 + n*10, Channel reload register	
		volatile unsigned int ChCntr;	///<  0x28 + n*10, Channel counter register	
		volatile unsigned int Resv;		///<  Reserved
		
	} Ch[DRV_PIT_CH_NUM];

} T_PIT_REG_MAP;


#define PIT_REG_BASE0		((T_PIT_REG_MAP *)MEM_BASE_PIT0)
#define PIT_REG_BASE1		((T_PIT_REG_MAP *)MEM_BASE_PIT1)


static unsigned char pit_ch_status;
static void drv_pit_timer_isr(void);


/**    @brief		Pit0/pit1 initialization.
*	   @details 	Initialize the reset value of pit1-channel3, set the time source of Pit1 as APB clock 
                    and 32-bit timer, and start pit1-channel3 channel 
*      @return      0--Pit initial succeed, 1--Pit initial failed
*/
int drv_pit_init(void)
{
	chip_clk_enable(CLK_PIT0);
	chip_clk_enable(CLK_PIT1);

	PIT_REG_BASE0->IntEn = 0;
	PIT_REG_BASE0->ChEn = 0;
	PIT_REG_BASE0->IntSt = 0xFFFFFFFF;
	PIT_REG_BASE0->Ch[0].ChReload = 0;
	PIT_REG_BASE0->Ch[1].ChReload = 0;
	PIT_REG_BASE0->Ch[2].ChReload = 0;
	PIT_REG_BASE0->Ch[3].ChReload = 0;

	PIT_REG_BASE1->IntEn = 0;
	PIT_REG_BASE1->ChEn = 0;
	PIT_REG_BASE1->IntSt = 0xFFFFFFFF;
	PIT_REG_BASE1->Ch[0].ChReload = 0;
	PIT_REG_BASE1->Ch[1].ChReload = 0;
	PIT_REG_BASE1->Ch[2].ChReload = 0;
	PIT_REG_BASE1->Ch[3].ChReload = 0;

	arch_irq_register(VECTOR_NUM_PIT1, drv_pit_timer_isr);

#if 0
	PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChReload = DRV_PIT_TICK_CH_RELOAD;

	PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCtrl 
		= DRV_PIT_CH_MODE_SET_TIMER | DRV_PIT_APB_CLOCK;

	PIT_REG_BASE1->ChEn 
		= DRV_PIT_CH_MODE_ENABLE_TIMER << (DRV_PIT_TICK_CH_NUM * DRV_PIT_CH_OFFSET);
#endif
	return PIT_RET_SUCCESS;
}

/**    @brief       Pit delay.
 *     @param[in]   delay   The number of delay, 24000000 is 1s.
 */
void drv_pit_delay( long delay)
{
#if 1
	unsigned long  flags;
	long tmo = delay * 4;
	unsigned long now, last;

	flags = system_irq_save();

	last = arch_cpu_getPFM();

	while (tmo > 0)
	{
		now = arch_cpu_getPFM();
		if (now < last)
		{
			/* count up timer overflow */
			tmo -= DRV_PIT_TICK_CH_RELOAD - last + now;
		}
		else
		{
			tmo -= now - last;
		}
		last = now;
	}
	system_irq_restore(flags);
#else
	unsigned long  flags;
	long tmo = delay;
	unsigned long now, last;

	flags = system_irq_save();

	last = PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCntr;

	while (tmo > 0)
	{
		now = PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCntr;
		if (now > last)
		{
			/* count down timer overflow */
			tmo -= DRV_PIT_TICK_CH_RELOAD - now+ last;
		}
		else
		{
			tmo -= last - now;
		}
		last = now;
	}
	system_irq_restore(flags);
#endif
}


/**    @brief		Handle pit different events.
*	   @details 	The events handled include pit channel reload set, get pit channel counter, enable pit channel interrupt, 
                    get pit channel interrupt status, pit channel mode set.
*	   @param[in]	channel_num    Specifies the channel number to open
*	   @param[in]	event       Control event type
*	   @param[in]	arg    Control parameters, the specific meaning is determined according to the event
*	   @return  	0--Handle event succeed, -1--Handle event failed
*/
int drv_pit_ioctrl(unsigned int channel_num, int event, unsigned int arg)
{
	T_PIT_REG_MAP *pit_reg_base;
	unsigned int value, ch_num;
	unsigned long  flags;
	int ret = PIT_RET_SUCCESS;

	if (channel_num < DRV_PIT_CHN_4)
	{
		pit_reg_base = PIT_REG_BASE0;
		ch_num = channel_num;
	}
	else if (channel_num < DRV_PIT_CHN_MAX)
	{
		pit_reg_base = PIT_REG_BASE1;
		ch_num = channel_num - DRV_PIT_CHN_4;
	}
	else
	{
		return PIT_RET_ERROR;
	}

	flags = system_irq_save();

	switch (event)
	{
		case DRV_PIT_CTRL_SET_COUNT:
			if (arg)
			{
				pit_reg_base->Ch[ch_num].ChReload = arg;
			}
			else
			{
				ret = PIT_RET_ERROR;
			}
			break;

		case DRV_PIT_CTRL_GET_COUNT:
			*((unsigned int *)arg) = pit_reg_base->Ch[ch_num].ChCntr;
			break;

		case DRV_PIT_CTRL_INTR_ENABLE:
			if (arg)
			{
				pit_reg_base->IntEn = pit_reg_base->IntEn | (1<<(DRV_PIT_CH_OFFSET * ch_num));
			}
			else
			{
				pit_reg_base->IntEn = pit_reg_base->IntEn & ~(1<<(DRV_PIT_CH_OFFSET * ch_num));
			}
			break;

		case DRV_PIT_CTRL_INTR_STATUS_GET:
			if (arg)
			{
				*((unsigned int *)arg) = !!(pit_reg_base->IntSt & (1 << (DRV_PIT_CH_OFFSET*(ch_num))));
			}
			else
			{
				ret = PIT_RET_ERROR;
			}
			break;

		case DRV_PIT_CTRL_INTR_STATUS_CLEAN:
			pit_reg_base->IntSt = 1 << (DRV_PIT_CH_OFFSET*(ch_num));
			break;

		case DRV_PIT_CTRL_CH_MODE_ENABLE:
			value = pit_reg_base->ChEn;
			value &= ~(0xF << DRV_PIT_CH_OFFSET *ch_num);
			value |= arg << DRV_PIT_CH_OFFSET * ch_num;
			pit_reg_base->ChEn = value;
			break;

		case DRV_PIT_CTRL_CH_MODE_SET:
			pit_reg_base->Ch[ch_num].ChCtrl = arg | DRV_PIT_APB_CLOCK;
			break;

		case DRV_PIT_CTRL_CH_ALLOC:
			if (pit_ch_status & (1<<channel_num))
			{
				ret = PIT_RET_ERROR;
			}
			else
			{
				pit_ch_status |= (1<<channel_num);
			}
			break;

		case DRV_PIT_CTRL_CH_RELEASE:
			if (pit_ch_status & (1<<channel_num))
			{
				pit_ch_status &= ~(1<<channel_num);
			}
			break;
		case DRV_PIT_CTRL_PWM_MUTEX_START:
			#if 0
			value = pit_reg_base->ChEn;
			value &= ~(0xF << DRV_PIT_CH_OFFSET *ch_num);
			value |= DRV_PIT_CH_MODE_ENABLE_PWM << DRV_PIT_CH_OFFSET * ch_num;

			T_PIT_REG_MAP *pit_reg_base_mutex;
			unsigned int ch_num_mutex, ch_level, ch_count;
			//pit_reg_base->Ch[ch_num].ChCtrl = 0x1c;
			ch_level = (pit_reg_base->Ch[ch_num].ChCtrl >> 4) & 1;
			if (arg < DRV_PIT_CHN_4)
			{
				pit_reg_base_mutex = PIT_REG_BASE0;
				ch_num_mutex = arg;
			}
			else if (arg < DRV_PIT_CHN_MAX)
			{
				pit_reg_base_mutex = PIT_REG_BASE1;
				ch_num_mutex = arg - DRV_PIT_CHN_4;
			}
			else
			{
				system_irq_restore(flags);
				return PIT_RET_ERROR;
			}

			if (ch_level)
			{
				while (1)
				{
					ch_count = (pit_reg_base_mutex->Ch[ch_num_mutex].ChCntr) >> 16;
					if ((ch_count < 8) && (ch_count > 4))
					{
						pit_reg_base->ChEn = value; 	
						//os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, h-pwm val: %x!\r\n", value);
						break;
					}
					else
					{
						system_irq_restore(flags);
						flags = system_irq_save();
					}
				}
			}
			else
			{
				while (1)
				{
					ch_count = (pit_reg_base_mutex->Ch[ch_num_mutex].ChCntr) & 0xFFFF;
					if ((ch_count < 12) && (ch_count > 7))
					{
						pit_reg_base->ChEn = value; 						
						//os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, l-pwm val: %x!\r\n", value);
						break;
					}
					else
					{
						system_irq_restore(flags);
						flags = system_irq_save();
					}
				}

			}
			#endif
			break;
		default:
			ret = PIT_RET_ERROR;
			break;
	}

	system_irq_restore(flags);

	return ret;
}


unsigned int drv_pit_get_tick(void)
{
#if 1
	return arch_cpu_getPFM();
#else
	unsigned int count;

	count =  DRV_PIT_TICK_CH_RELOAD - PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCntr;

	return count;
#endif
}

typedef struct _T_PIT_ISR
{
	unsigned int reload;
	void (* callback)(void);
} T_PIT_ISR;


typedef struct _T_DRV_PIT_DEV
{
	T_PIT_ISR isr[2];

} T_DRV_PIT_DEV;

static T_DRV_PIT_DEV drv_pit_dev;

static void drv_pit_timer_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif

	T_DRV_PIT_DEV * p_timer_dev = &drv_pit_dev;
	unsigned int chn_timer;

	for (chn_timer = DRV_PIT_WIFI_CH_NUM; chn_timer <= DRV_PIT_TICK_CH_NUM; chn_timer++)
	{
		T_PIT_ISR * p_isr = &p_timer_dev->isr[chn_timer - DRV_PIT_WIFI_CH_NUM];

		unsigned int status = PIT_REG_BASE1->IntSt;
		unsigned int mask = 1 << (DRV_PIT_CH_OFFSET*chn_timer);

		if (p_isr->callback && (status & mask))
		{
			p_isr->callback();
			if (!p_isr->reload)
			{
				unsigned int value = PIT_REG_BASE1->ChEn;
				value &= ~mask;
				PIT_REG_BASE1->ChEn = value;
			}
			PIT_REG_BASE1->IntSt = mask;
		}
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}



int drv_pit_timer_start(int chn)
{
	if ((chn != DRV_PIT_CHN_6) && (chn != DRV_PIT_CHN_7))
	{
		return PIT_RET_ERROR;
	}

	drv_pit_ioctrl(chn, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_TIMER);

	return PIT_RET_SUCCESS;
}



int drv_pit_timer_open(int chn, unsigned int period_us, void(*fn)(), int one_shot)
{
	T_DRV_PIT_DEV * p_timer_dev = &drv_pit_dev;

	if ((chn != DRV_PIT_CHN_6) && (chn != DRV_PIT_CHN_7))
	{
		return PIT_RET_ERROR;
	}

	p_timer_dev->isr[chn - DRV_PIT_CHN_6].callback = fn;
	p_timer_dev->isr[chn - DRV_PIT_CHN_6].reload = !one_shot;

	drv_pit_ioctrl(chn, DRV_PIT_CTRL_SET_COUNT, period_us * 40);
	drv_pit_ioctrl(chn, DRV_PIT_CTRL_INTR_ENABLE, DRV_PIT_INTR_ENABLE);
	drv_pit_ioctrl(chn, DRV_PIT_CTRL_CH_MODE_SET, DRV_PIT_CH_MODE_SET_TIMER);
	drv_pit_ioctrl(chn, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_TIMER);

	drv_pit_timer_start(chn);
	return PIT_RET_SUCCESS;
}

int drv_pit_mutex(int channel_num, unsigned int channel_mutex_num, unsigned int arg)
{
	T_PIT_REG_MAP *pit0_reg_base;
	T_PIT_REG_MAP *pit1_reg_base;
	unsigned int value = 0; 
	unsigned int mutex_value = 0;
	unsigned int ch_num,ch_mutex_num;
	unsigned long  flags;
	unsigned char  chan0_flags = 0;
	unsigned char  chan1_flags = 0;

	if (channel_num < DRV_PIT_CHN_4)
	{
		pit0_reg_base = PIT_REG_BASE0;
		ch_num = channel_num;
		chan0_flags = 1;
	}
	else if (channel_num < DRV_PIT_CHN_MAX)
	{
		pit0_reg_base = PIT_REG_BASE1;
		ch_num = channel_num - DRV_PIT_CHN_4;
		chan0_flags = 2;
	}
	else
	{
		return PIT_RET_ERROR;
	}

	
	if (channel_mutex_num < DRV_PIT_CHN_4)
	{
		pit1_reg_base = PIT_REG_BASE0;
		ch_mutex_num = channel_mutex_num;
		chan1_flags = 1;
	}
	else if (channel_mutex_num < DRV_PIT_CHN_MAX)
	{
		pit1_reg_base = PIT_REG_BASE1;
		ch_mutex_num = channel_mutex_num - DRV_PIT_CHN_4;
		chan1_flags = 2;
	}
	else
	{
		return PIT_RET_ERROR;
	}
	flags = system_irq_save();	
	value &= ~(0xF << DRV_PIT_CH_OFFSET *ch_num);
	value |= arg << DRV_PIT_CH_OFFSET * ch_num;
	mutex_value &= ~(0xF << DRV_PIT_CH_OFFSET *ch_mutex_num);
	mutex_value |= arg << DRV_PIT_CH_OFFSET * ch_mutex_num;
	if(chan0_flags == chan1_flags)
	{
		value |= mutex_value ;	
		pit0_reg_base->ChEn = value;
	}	
	else
	{
		pit0_reg_base->ChEn = value;
		pit1_reg_base->ChEn = mutex_value;
	}

	system_irq_restore(flags);
	return PIT_RET_SUCCESS;

}

