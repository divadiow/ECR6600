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
#include "oshal.h"


#define DRV_PIT_NUM_0		0
#define DRV_PIT_NUM_1		1

#define DRV_PIT_CH_NUM		4	 ///< one pit  has a maximum of 4 channels

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

	PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChReload = DRV_PIT_TICK_CH_RELOAD;

	PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCtrl 
		= DRV_PIT_CH_MODE_SET_TIMER | DRV_PIT_APB_CLOCK;

	PIT_REG_BASE1->ChEn 
		= DRV_PIT_CH_MODE_ENABLE_TIMER << (DRV_PIT_TICK_CH_NUM * DRV_PIT_CH_OFFSET);

	return PIT_RET_SUCCESS;
}

/**    @brief       Pit delay.
 *     @param[in]   delay   The number of delay, 24000000 is 1s.
 */
void drv_pit_delay( long delay)
{
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

		default:
			ret = PIT_RET_ERROR;
			break;
	}

	system_irq_restore(flags);

	return ret;
}


unsigned int drv_pit_get_tick(void)
{
	unsigned int count;

	count =  DRV_PIT_TICK_CH_RELOAD - PIT_REG_BASE1->Ch[DRV_PIT_TICK_CH_NUM].ChCntr;

	return count;
}

