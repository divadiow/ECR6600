/**
*@file	   trng.c
*@brief    Hardware generates true random numbers and pseudo random numbers
*@author   wang chao
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include <string.h>
#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "oshal.h"
#include "trng.h"
//#include "cli.h"
#include "pit.h"


/**
*@brief   Random number register structure 
*/
typedef  volatile struct _T_TRNG_REG_MAP
{
	/**0x00，trng work clock enable*/
	unsigned int trng_clk_gating_en;   
	unsigned int trng_trng_val;        ///< 0x04，The value of a true random number
	unsigned int trng_prng_val;        ///< 0x08，The value of a Pseudo-random number
	unsigned int process_sensor_shift_val;///< 0x0c，process sensor shift val
	unsigned int dig_ps_hdsw_en;        ///< 0x10，process sensor enable
} T_TRNG_REG_MAP;


#define MEM_BASE_CNT_TRNG (MEM_BASE_TRNG + 0x40)
T_TRNG_REG_MAP *trng_reg_base = (T_TRNG_REG_MAP *)MEM_BASE_CNT_TRNG;

/**enable&disable trng work clock*/
#define DRV_TRNG_ENABLE   0x1
#define DRV_TRNG_DISABLE  0x0

#define DRV_TRNG_USEC_PER_SEC		1000000

/** @brief    Get a true random number
*   @details  After reading the random number for the first time, and abandon
*   @return   The true random number is 32 bits
*/
unsigned int drv_trng_get(void)
{	
	volatile unsigned int tmpdata = 0;
	int ps_flag = 0;

	unsigned long flags = system_irq_save();

	/**1.enable clk*/
    chip_clk_enable(CLK_TRNG_WORK);
	chip_clk_enable(CLK_TRNG_APB);
	trng_reg_base->trng_clk_gating_en = DRV_TRNG_ENABLE;  ///< enable gating clk

	ps_flag = trng_reg_base->dig_ps_hdsw_en;
	trng_reg_base->dig_ps_hdsw_en = 1;
		
	/**2.read data once,and abandon*/
	trng_reg_base->trng_trng_val;
	//os_usdelay(1);
	drv_pit_delay(CHIP_CLOCK_APB/DRV_TRNG_USEC_PER_SEC);

	/**3.read data*/
	tmpdata = trng_reg_base->trng_trng_val;

	trng_reg_base->dig_ps_hdsw_en = ps_flag;
	
	/**4.disabel clk*/
	trng_reg_base->trng_clk_gating_en = DRV_TRNG_DISABLE; ///< disable gating clk

	chip_clk_disable(CLK_TRNG_APB);
	chip_clk_disable(CLK_TRNG_WORK);

	system_irq_restore(flags);

	return tmpdata;
}


/** @brief    Get a pseudo random number
*   @details  After reading the random number for the first time, and abandon
*   @return   The pseudo random number is 32 bits
*/
unsigned int drv_prng_get(void)
{
	volatile unsigned int tmpdata = 0;
	int ps_flag = 0;

	unsigned long flags = system_irq_save();

	/**1.enable clk*/
	chip_clk_enable(CLK_TRNG_WORK);
	chip_clk_enable(CLK_TRNG_APB);
	trng_reg_base->trng_clk_gating_en = DRV_TRNG_ENABLE;  ///< enable gating clk

	ps_flag = trng_reg_base->dig_ps_hdsw_en;
	trng_reg_base->dig_ps_hdsw_en = 1;
	
	/**2.read data once,and abandon*/
	trng_reg_base->trng_trng_val;
	//os_usdelay(1);
	drv_pit_delay(CHIP_CLOCK_APB/DRV_TRNG_USEC_PER_SEC);

	/**3.read data*/
	tmpdata = trng_reg_base->trng_prng_val;

	trng_reg_base->dig_ps_hdsw_en = ps_flag;

	/**4.disabel clk*/	
	trng_reg_base->trng_clk_gating_en = DRV_TRNG_DISABLE; ///< disable gating clk

	chip_clk_disable(CLK_TRNG_APB);
	chip_clk_disable(CLK_TRNG_WORK);

	system_irq_restore(flags);

	return tmpdata;

}

