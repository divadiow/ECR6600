/**
*@file	   wdt.c
*@brief    This part of program is WDT algorithm by hardware
*@author   
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/
#include <string.h>
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "arch_irq.h"
#include "chip_memmap.h"
#include "chip_irqvector.h"
#include "wdt.h"
#include "oshal.h"
#include "semphr.h" 

#define WAKEUP_ADDR_REG		MEM_BASE_SMU_AON + 0x04		///< wakeup address register
#define WAKEUP_ADDR_VALUE	0	///< After waking up, start from address 0 

/**magic number*/
#define WDT_WP_NUM      0x5aa5	///< disable the write protection
#define WDT_RESET_NUM   0xcafe	///< restart the watchdog timer

/**WDT control register*/
#define WDT_REG_CTRL_TIMER_ENABLE      (1<<0)	///< Enable the watchdog timer		
#define WDT_REG_CTRL_TIMER_DISABLE     (0<<0)	///< Disable the watchdog timer

#define WDT_REG_CTRL_CLKSEL_PCLK       (1<<1)	///< Select pclk as clock source of timer
#define WDT_REG_CTRL_CLKSEL_EXTCLK     (0<<1)	///< Select extclk as clock source of timer

#define WDT_REG_CTRL_INTEN(x)          (x<<2)	///< Enable or disable the watchdog interrupt
#define WDT_REG_CTRL_RSTEN(x)          (x<<3)	///< Enable or disable the watchdog reset


#define WDT_REG_CTRL_INTTIME(x)        ((x)<<4)    ///< The time interval of the interrupt stage
#define WDT_REG_CTRL_RSTTIME(x)        ((x)<<8)	   ///< The time interval of the reset stage

/**
*@brief   WDT register structure 
*/
typedef  volatile struct _T_WDT_REG_MAP
{
	unsigned int Resv0[4];      ///< 0x00 ~ 0x0f, reserved register 
	unsigned int Ctrl;			///< 0x10, control register 
	unsigned int Restart;       ///< 0x14, restart register 
	unsigned int WrEnable;      ///< 0x18, write enable register
	unsigned int Status;        ///< 0x1C, status register
} T_WDT_REG_MAP;

#define WDT_REG_BASE0		((T_WDT_REG_MAP *)MEM_BASE_WDT)

typedef volatile struct _T_DRV_WDT_DEV
{
	char active;
	void (*wdt_callback)(void);
	unsigned int ctrl_value;
}T_DRV_WDT_DEV;

T_DRV_WDT_DEV p_wdt_dev;


static void wdt_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	WDT_REG_BASE0->Status = 1;

	if (p_wdt_dev.wdt_callback != NULL)
	{   
    	p_wdt_dev.wdt_callback();
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

void drv_wdt_isr_register(void(*callfunc)(void))
{
	p_wdt_dev.wdt_callback = callfunc;
}

/** @brief    Feed the watchdog
*/
WDT_RET_CODE drv_wdt_feed(void)
{
	if (!p_wdt_dev.active)
	{  
    	return WDT_RET_ERR;
	}
	WDT_REG_BASE0->WrEnable = WDT_WP_NUM;
	WDT_REG_BASE0->Restart = WDT_RESET_NUM;

	return WDT_RET_OK;
}

/** @brief    Start the watchdog timer
*   @details  Before using the watchdog timer,
			  need to configure the register
*/
WDT_RET_CODE drv_wdt_start(int int_period, int rst_period, int int_enable, int rst_enable)
{
	if (p_wdt_dev.active)
	{
		return WDT_RET_ERR;
	}

	p_wdt_dev.active = 1;

	chip_clk_enable((unsigned int)CLK_WDT_APB);		///< enable APB clock
	chip_clk_enable((unsigned int)CLK_WDT_EXT);		///< enable EXTCLK clock
	
	arch_irq_register(VECTOR_NUM_WDT, wdt_isr);		///< Register watchdog interrupt service function 
	arch_irq_unmask(VECTOR_NUM_WDT);	///< open watchdog interrupt

	WDT_REG_BASE0->WrEnable = WDT_WP_NUM;	///< disable the write protection
	WDT_REG_BASE0->Ctrl = WDT_REG_CTRL_CLKSEL_EXTCLK		///< Select extclk as clock source of timer
		               | WDT_REG_CTRL_TIMER_ENABLE	///< Enable the watchdog timer
		               | WDT_REG_CTRL_INTEN(int_enable)
		               | WDT_REG_CTRL_RSTEN(rst_enable)
					   | WDT_REG_CTRL_INTTIME(int_period) 
					   | WDT_REG_CTRL_RSTTIME(rst_period);
	
	p_wdt_dev.ctrl_value = WDT_REG_BASE0->Ctrl;

	return WDT_RET_OK;
}

/** @brief    Stop the watchdog timer
*   @details  When the application completes the watchdog operation,
			  the watchdog device can be stopped
*/
WDT_RET_CODE drv_wdt_stop(void)
{
	if (!p_wdt_dev.active)
	{
		return WDT_RET_ERR;
	}
	p_wdt_dev.active = 0;

	arch_irq_mask(VECTOR_NUM_WDT);

	WDT_REG_BASE0->WrEnable = WDT_WP_NUM;	///< disable the write protection
	WDT_REG_BASE0->Ctrl = 0;		///< disable the watchdog timer,the watchdog reset,the watchdog interrupt
	WDT_REG_BASE0->Status = 1;	///< timer is expired 

	chip_clk_disable((unsigned int)CLK_WDT_APB);	///< disable APB clock
	chip_clk_disable((unsigned int)CLK_WDT_EXT);	///< disable EXTCLK clock
	return WDT_RET_OK;
}


/** @brief    Watchdog timer reset chip
*   @details  When the application completes the watchdog operation,
			  the watchdog device can be stopped
*/
WDT_RET_CODE drv_wdt_chip_reset(void)
{
	system_irq_save();

	drv_wdt_stop();
	WRITE_REG(WAKEUP_ADDR_REG, WAKEUP_ADDR_VALUE);	///< wakeup address register,CPU start address after reset
	drv_wdt_start(WDT_CTRL_INTTIME_0, WDT_CTRL_RSTTIME_0, WDT_CTRL_INTDISABLE, WDT_CTRL_RSTENABLE);
	while(1);
	
	return WDT_RET_OK;
}

WDT_RET_CODE drv_wdt_restore(void)
{
	if (p_wdt_dev.active == 1)
	{
		chip_clk_enable((unsigned int)CLK_WDT_APB); 	///< enable APB clock
		chip_clk_enable((unsigned int)CLK_WDT_EXT); 	///< enable EXTCLK clock
		WDT_REG_BASE0->WrEnable = WDT_WP_NUM;
		WDT_REG_BASE0->Ctrl = p_wdt_dev.ctrl_value;
	}
	
	return WDT_RET_OK;
}


