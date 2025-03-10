/**
* @file       efuse.c
* @brief      efuse driver code  
* @author     WangChao
* @date       2021-2-19
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: v0.2
	author	LvHuan, 
	date	2021-03-01, 
	desc	modify read api(Add direction judgment) \n
*/


#include <stdio.h>
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "chip_clk_ctrl.h"
#include "chip_memmap.h"
#include "system_def.h"
#include "gpio.h"
#include "efuse.h"
#include "oshal.h"


typedef  struct _T_GPIO_REG_MAP
{
	/* ID and Revision Register */
	volatile unsigned int IdRev;			// 0x00, ID and Revision Register
	volatile unsigned int Resv0[3];			// 0x04~0x0C, Reserved 

	/* Configuration Register */
	volatile unsigned int Cfg;				// 0x10, Hardware Configure Register
	volatile unsigned int Resv1[3];			// 0x14~0x1C, Reserved 

	/* Channel Control Register */
	volatile unsigned int DataIn;			// 0x20, Channel data-in register
	volatile unsigned int DataOut;			// 0x24, Channel data-out register
	volatile unsigned int ChannelDir;		// 0x28, Channel direction register
	volatile unsigned int DoutCLear;		// 0x2C, Channel data-out clear register
	volatile unsigned int DoutSet;			// 0x30, Channel data-out set register
	volatile unsigned int Resv2[3];			// 0x34~0x3C, Reserved 

	/* Pull Control Register */
	volatile unsigned int PullEn;			// 0x40, Pull enable register
	volatile unsigned int PullType;			// 0x44, Pull type register
	volatile unsigned int Resv3[2];			// 0x48~0x4C, Reserved 

	/* Interrupt Control register */
	volatile unsigned int IntrEn;			// 0x50, Interrupt enable register
	volatile unsigned int IntrMOde0;		// 0x54, Interrupt mode register (0~7)
	volatile unsigned int IntrMOde1;		// 0x58, Interrupt mode register (8~15)
	volatile unsigned int IntrMOde2;		// 0x5C, Interrupt mode register (16~23)
	volatile unsigned int IntrMOde3;		// 0x60, Interrupt mode register (24~31)
	volatile unsigned int IntrStatus;		// 0x64, Interrupt status register
	volatile unsigned int Resv4[2];			// 0x68~0x6C, Reserved 

	/* De-bounce Control register */
	volatile unsigned int DeBounceEn;		// 0x70, De-bounce enable register
	volatile unsigned int DeBounceCtrl;		// 0x74, De-bounce control register
	volatile unsigned int Resv5[2];			// 0x78~0x7C, Reserved
} T_GPIO_REG_MAP;


typedef struct _T_DRV_GPIO_DEV
{
	unsigned int gpio_initialized;
	T_GPIO_REG_MAP * gpio_reg_base;
	T_GPIO_ISR_CALLBACK  gpio_callback[CHIP_CFG_GPIO_NUM_MAX];

} T_DRV_GPIO_DEV;

static T_DRV_GPIO_DEV * gpio_dev;


static void drv_gpio_isr(int vector) __attribute__((section(".ilm_text_drv")));
int drv_gpio_write(int gpio_num, GPIO_LEVEL_VALUE gpio_level) __attribute__((section(".ilm_text_drv")));



/**    @brief       gpio isr.
 *     @details     Callback function configuration of GPIO interrupt.
 *     @param[in]   vector--Interrupt vector number.
 */
static void drv_gpio_isr(int vector)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	T_GPIO_REG_MAP * p_gpio_reg = p_gpio_dev->gpio_reg_base;
	T_GPIO_ISR_CALLBACK  * p_gpio_callback;
	unsigned int sts = p_gpio_reg->IntrStatus;
	int i = 0;

	for (; i < CHIP_CFG_GPIO_NUM_MAX; i++)
	{
		p_gpio_callback = &p_gpio_dev->gpio_callback[i];
		if (((sts >> i) & 0x1) && (p_gpio_callback->gpio_callback!= NULL))
		{
			p_gpio_callback->gpio_callback(p_gpio_callback->gpio_data);
		}
	}

	p_gpio_reg->IntrStatus = sts;

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}


/**    @brief       gpio interrupt mode configuration.
 *     @details     Configure the interrupt mode corresponding to GPIO.
 *     @param[in]   p_gpio_reg--Structure of GPIO register.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @param[in]   mode--Interrupt mode include NOP,HIGH,LOW,N_EDGE,P_EDGE,DUAL_EDGE; 
 */
static void drv_gpio_intr_mode(T_GPIO_REG_MAP * p_gpio_reg, int gpio_num, int mode)
{
	unsigned int value;

	if (gpio_num < GPIO_NUM_8)
	{
		value = p_gpio_reg->IntrMOde0;
		value &= ~(0xF << ((gpio_num % 8) * 4));
		value |= mode << ((gpio_num % 8) * 4);
		p_gpio_reg->IntrMOde0 = value;
	}
	else if (gpio_num < GPIO_NUM_16)
	{
		value = p_gpio_reg->IntrMOde1;
		value &= ~(0xF << ((gpio_num % 8) * 4));
		value |= mode << ((gpio_num % 8) * 4);
		p_gpio_reg->IntrMOde1 = value;
	}
	else if (gpio_num < GPIO_NUM_24)
	{
		value = p_gpio_reg->IntrMOde2;
		value &= ~(0xF << ((gpio_num % 8) * 4));
		value |= mode << ((gpio_num % 8) * 4);
		p_gpio_reg->IntrMOde2 = value;
	}
	else
	{
		value = p_gpio_reg->IntrMOde3;
		value &= ~(0xF << ((gpio_num % 8) * 4));
		value |= mode << ((gpio_num % 8) * 4);
		p_gpio_reg->IntrMOde3 = value;
	}
}


/**    @brief       Read GPIO direction value.
 *     @details     Read GPIO_ Num corresponding to the direction value and return.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @return      0/1--ok; other--error.
 */
int drv_gpio_dir_get(int gpio_num)
{
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	T_GPIO_REG_MAP * p_gpio_reg = p_gpio_dev->gpio_reg_base;
	if ((p_gpio_dev == NULL) || (p_gpio_dev->gpio_initialized != 1))
	{
		return GPIO_RET_INIT_ERROR;
	}

	if( (gpio_num < GPIO_NUM_0 ) || (gpio_num > GPIO_NUM_25 ) || (gpio_num == GPIO_NUM_19 ) )
	{
		return GPIO_RET_PARAMETER_ERROR;
	}
	unsigned int gpio_dir = p_gpio_reg->ChannelDir;

	gpio_dir = (gpio_dir >> gpio_num) & 0x01 ;

	return gpio_dir;

}


/**    @brief       Read GPIO level value.
 *     @details     Read GPIO_ Num corresponding to the voltage value and return.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @return      0/1--ok; other--error.
 */
int drv_gpio_read(int gpio_num)
{
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	T_GPIO_REG_MAP * p_gpio_reg = p_gpio_dev->gpio_reg_base;

	if ((p_gpio_dev == NULL) || (p_gpio_dev->gpio_initialized != 1))
	{
		return GPIO_RET_INIT_ERROR;
	}
	
	if( (gpio_num < GPIO_NUM_0 ) || (gpio_num > GPIO_NUM_25 ) || (gpio_num == GPIO_NUM_19 ) )
	{
		return GPIO_RET_PARAMETER_ERROR;
	}
	unsigned int val, gpio_dir = p_gpio_reg->ChannelDir;

	gpio_dir = (gpio_dir >> gpio_num) & 0x01 ;
	
	if(gpio_dir == DRV_GPIO_ARG_DIR_IN)
	{
		val = p_gpio_reg->DataIn;
	}
	else
	{
		val = p_gpio_reg->DataOut;
	}
	val = (val >> gpio_num) & 0x01 ;
	
	return val;
}


/**    @brief       Write GPIO level value.
 *     @details     Write the input parameter gpio_level value to GPIO corresponding to gpionum.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @param[in]   gpio_level--The voltage value write to gpio_num. 
 *     @return      0--ok ,other--error.
 */
int drv_gpio_write(int gpio_num, GPIO_LEVEL_VALUE gpio_level)
{
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	T_GPIO_REG_MAP * p_gpio_reg = p_gpio_dev->gpio_reg_base;
	unsigned int irq_flag;
	if ((p_gpio_dev == NULL) || (p_gpio_dev->gpio_initialized != 1))
	{
		return GPIO_RET_INIT_ERROR;
	}
	
	if((gpio_num < GPIO_NUM_0 ) || (gpio_num > GPIO_NUM_25 ) || (gpio_num == GPIO_NUM_19 ))
	{
		return GPIO_RET_PARAMETER_ERROR;
	}

	if((gpio_level!= LEVEL_LOW ) && (gpio_level!= LEVEL_HIGH ))
	{
		return GPIO_RET_PARAMETER_ERROR;
	}
	
	irq_flag = system_irq_save();
	
	unsigned int val = p_gpio_reg->DataOut;
	val &= ~(1 << gpio_num);
	val |= (gpio_level  << gpio_num);

	p_gpio_reg->DataOut = val;
	system_irq_restore(irq_flag);
	
	return GPIO_RET_SUCCESS;
}


/**    @brief       Read GPIO PULL value.
 *     @details     Read GPIO_ Num corresponding to the pull value and return.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @return      0/1 pull value; other--error.
 */
int drv_gpio_pull_read(int gpio_num)
{
	if((gpio_num < GPIO_NUM_0 ) || (gpio_num > GPIO_NUM_25 ) || (gpio_num == GPIO_NUM_19 ))
	{
		return GPIO_RET_PARAMETER_ERROR;
	}

	unsigned  int fuse_ft = 0;
	unsigned int pull_value = 0;

	drv_efuse_read(EFUSE_FT_ADDR, &fuse_ft, EFUSE_FT_LENG);
	if((fuse_ft & EFUSE_FT_MASK) == EFUSE_FT_TYPE_1600)
	{
		unsigned  int * pull_en 	  = (unsigned  int *)AON_PULLUP_GPIO_REG;
		unsigned  int * pull_select   = (unsigned  int *)AON_PULLDOWN_GPIO_REG;
		
		if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
		{
			pull_value = (*pull_en & (1 << gpio_num)) >> gpio_num;
		}
		else
		{
			pull_value = (*pull_select & (1 << gpio_num))>> gpio_num;
		}
	}
	else
	{
		unsigned  int * pull_down = (unsigned  int *)AON_PULLDOWN_GPIO_REG;
		unsigned  int * pull_up   = (unsigned  int *)AON_PULLUP_GPIO_REG;
		if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
		{
			pull_value = (*pull_down & (1 << gpio_num))>> gpio_num;
		}
		else
		{
			pull_value = (*pull_up & (1 << gpio_num))>> gpio_num;
		}
	}
	return pull_value;
}


/**    @brief       Initialize gpio related configuration.
 *     @details     Initialization of gpio configuration before using gpio, The initialization operation only needs one time.
 *     @return      0--ok , -1--error.
 */
int drv_gpio_init(void)
{
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	
	if (p_gpio_dev)
	{
		if (p_gpio_dev->gpio_initialized)
		{
			return GPIO_RET_SUCCESS;
		}
		else
		{
			return GPIO_RET_ERROR;
		}
	}
	else
	{
		p_gpio_dev = (T_DRV_GPIO_DEV *)os_zalloc(sizeof(T_DRV_GPIO_DEV));
		if (!p_gpio_dev)
		{
			return GPIO_RET_ERROR;
		}
	}

	p_gpio_dev->gpio_reg_base = (T_GPIO_REG_MAP *)MEM_BASE_GPIO;
	p_gpio_dev->gpio_reg_base->IntrEn = 0;
			
	arch_irq_register(VECTOR_NUM_GPIO, (void_fn)drv_gpio_isr);
	arch_irq_clean(VECTOR_NUM_GPIO);
	arch_irq_unmask(VECTOR_NUM_GPIO);

	p_gpio_dev->gpio_initialized = 1;
	gpio_dev = p_gpio_dev;
	return GPIO_RET_SUCCESS;
}


/**    @brief       gpio IO control configuration.
 *     @details     The events handled include ISR, INTR_MODE, INTR_ENABLE, INTR_DISABLE,SET_DIRCTION,PULL_TYPE,SET_DEBOUNCE,CLOSE_DEBOUNCE.
 *     @param[in]   gpio_num--The GPIO number you want to operate on. 
 *     @param[in]   event--The GPIO Control event type. 
 *     @param[in]   arg--Control parameters, the specific meaning is determined according to the event. 
 *     @return      0--ok , -1--error.
 */
int drv_gpio_ioctrl(int gpio_num, int event, int arg)
{
	T_DRV_GPIO_DEV * p_gpio_dev = gpio_dev;
	T_GPIO_REG_MAP * p_gpio_reg;
	T_GPIO_ISR_CALLBACK *p_callback;
	unsigned int value, irq_flag;
//	unsigned  int * pull_down = (unsigned  int *)AON_PULLDOWN_GPIO_REG;
//	unsigned  int * pull_up   = (unsigned  int *)AON_PULLUP_GPIO_REG;
	unsigned  int fuse_ft = 0;
	if ((p_gpio_dev == NULL) || (p_gpio_dev->gpio_initialized != 1))
	{
		return GPIO_RET_INIT_ERROR;
	}
	
	if((gpio_num < GPIO_NUM_0 ) || (gpio_num > GPIO_NUM_25 ) || (gpio_num == GPIO_NUM_19 ))
	{
		return GPIO_RET_PARAMETER_ERROR;
	}

	if ((event < DRV_GPIO_CTRL_REGISTER_ISR) || (event > DRV_GPIO_CTRL_CLOSE_DEBOUNCE))
	{
		return GPIO_RET_PARAMETER_ERROR;
	}
	
	irq_flag = system_irq_save();
	p_gpio_reg = p_gpio_dev->gpio_reg_base;

	switch (event)
	{
		case DRV_GPIO_CTRL_REGISTER_ISR:
			p_callback = (T_GPIO_ISR_CALLBACK *)arg;
			if(p_callback == NULL){
				system_irq_restore(irq_flag);
				return GPIO_RET_PARAMETER_ERROR;}
			p_gpio_dev->gpio_callback[gpio_num].gpio_data = p_callback->gpio_data;
			p_gpio_dev->gpio_callback[gpio_num].gpio_callback = p_callback->gpio_callback;
			break;

		case DRV_GPIO_CTRL_INTR_MODE:
			if((arg < 0) || (arg > 7) || (arg == 1) || (arg == 4))
			{	
				system_irq_restore(irq_flag);
				return GPIO_RET_PARAMETER_ERROR;
			}
			drv_gpio_intr_mode(p_gpio_reg, gpio_num, arg);
			break;
			
		case DRV_GPIO_CTRL_INTR_ENABLE:
			value = p_gpio_reg->IntrEn;
			p_gpio_reg->IntrEn = value | (1<<gpio_num );
			break;

		case DRV_GPIO_CTRL_INTR_DISABLE:
			value = p_gpio_reg->IntrEn;
			p_gpio_reg->IntrEn = value & (~(1<<gpio_num ));
			break;

		case DRV_GPIO_CTRL_SET_DIRCTION:
			if((arg < DRV_GPIO_ARG_DIR_IN) || (arg > DRV_GPIO_ARG_DIR_OUT))
			{		
				system_irq_restore(irq_flag);
				return GPIO_RET_PARAMETER_ERROR;
			}
			value= p_gpio_reg->ChannelDir;
			value &= ~(1 << gpio_num);
			value |= (arg << gpio_num);
			p_gpio_reg->ChannelDir = value;
			break;

		case DRV_GPIO_CTRL_PULL_TYPE:
			drv_efuse_read(EFUSE_FT_ADDR, &fuse_ft, EFUSE_FT_LENG);

			if((fuse_ft & EFUSE_FT_MASK) == EFUSE_FT_TYPE_1600)
			{
				unsigned  int * pull_en		  = (unsigned  int *)AON_PULLUP_GPIO_REG;
				unsigned  int * pull_select   = (unsigned  int *)AON_PULLDOWN_GPIO_REG;
				if (arg == DRV_GPIO_ARG_PULL_DOWN)
				{
					//cli_printf("pull down \r\n");
					if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
					{
						*pull_select |= (1 << gpio_num);
						*pull_en &= ~(1 << gpio_num);

					}
					else
					{
						*pull_en |= (1 << gpio_num);
						*pull_select &= ~(1 << gpio_num);
					}
				}
				else if (arg == DRV_GPIO_ARG_PULL_UP)
				{
					//cli_printf("pull up \r\n");
					if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
					{
						*pull_select |= (1 << gpio_num);
						*pull_en |= (1 << gpio_num);
					}
					else
					{
						*pull_en |= (1 << gpio_num);
						*pull_select |= (1 << gpio_num);
					}
				}
				else
				{
					system_irq_restore(irq_flag);
					return GPIO_RET_PARAMETER_ERROR;
				}
			}
			else
			{
				unsigned  int * pull_down = (unsigned  int *)AON_PULLDOWN_GPIO_REG;
				unsigned  int * pull_up   = (unsigned  int *)AON_PULLUP_GPIO_REG;
				if (arg == DRV_GPIO_ARG_PULL_DOWN)
				{
					//cli_printf("pull down \r\n");
					if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
					{
						*pull_down &= ~(1 << gpio_num);
						*pull_up |= (1 << gpio_num);
					}
					else
					{
						*pull_up &= ~(1 << gpio_num);
						*pull_down |= (1 << gpio_num);
					}
				}
				else if (arg == DRV_GPIO_ARG_PULL_UP)
				{
					//cli_printf("pull up \r\n");
					if((gpio_num == GPIO_NUM_14) || (gpio_num == GPIO_NUM_15))
					{
						*pull_up &= ~(1 << gpio_num);
						*pull_down |= (1 << gpio_num);
					}
					else
					{
						*pull_down &= ~(1 << gpio_num);
						*pull_up |= (1 << gpio_num);
					}
				}
				else
				{
					system_irq_restore(irq_flag);
					return GPIO_RET_PARAMETER_ERROR;
				}
			}
			break;
			
		case DRV_GPIO_CTRL_SET_DEBOUNCE:
			value = ((arg * (CHIP_CLOCK_APB / FRQ_MHZ)) / TIME_NS);
			if(value > 0x100)
			{
				system_irq_restore(irq_flag);
				return GPIO_RET_PARAMETER_ERROR;
			}
			else if(value != 0 )
			{
				value -= 1;
			}
			p_gpio_reg->DeBounceEn |= (1 << gpio_num);
			p_gpio_reg->DeBounceCtrl &= (~ 0xff);
			p_gpio_reg->DeBounceCtrl |= ((value & 0xff) |  BIT31);
			break;		
			
		case DRV_GPIO_CTRL_CLOSE_DEBOUNCE:
			value = p_gpio_reg->DeBounceEn;
			p_gpio_reg->DeBounceEn = value & (~(1<<gpio_num ));
			break;		
			
	}
	system_irq_restore(irq_flag);
	return GPIO_RET_SUCCESS;
}

