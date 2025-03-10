
#include <string.h>
#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "oshal.h"


/**
*@brief   process sensor register structure 
*/
typedef  volatile struct  _T_PROCESS_REG_MAP
{
	unsigned int process_sensor_start;  
	unsigned int process_sensor_count;     
	unsigned int logic_ps_cnts_out0;       
	unsigned int logic_ps_cnts_out1;
	unsigned int logic_ps_cnts_out2;
	unsigned int logic_ps_cnts_out3;
} T_PROCESS_REG_MAP;

typedef  volatile struct _T_PROCESS__SENSOR_REG_MAP
{
	/**0x00，trng work clock enable*/
	unsigned int trng_clk_gating_en;   
	unsigned int trng_trng_val;        ///< 0x04，The value of a true random number
	unsigned int trng_prng_val;        ///< 0x08，The value of a Pseudo-random number
	unsigned int process_sensor_shift_val;///< 0x0c，process sensor shift val
	unsigned int dig_ps_hdsw_en;        ///< 0x10，process sensor enable
} T_PROCESS_SENSOR_REG_MAP;


/** @brief  Ring OSC generates a clock whose frequency is sensitive to the process
*/
void drv_process_sensor_get(unsigned int DelayTimeUs,unsigned short int *pProcesscnt)
{
    T_PROCESS_SENSOR_REG_MAP *trng_reg_base = (T_PROCESS_SENSOR_REG_MAP *)(MEM_BASE_TRNG + 0x40);
	T_PROCESS_REG_MAP *process_reg_base = (T_PROCESS_REG_MAP *)MEM_BASE_TRNG;

    unsigned long flags = system_irq_save();
	/**1.enable clk*/
	chip_clk_enable(CLK_TRNG_WORK);
	chip_clk_enable(CLK_TRNG_APB);

	/**2.ps hdsw enable*/
	trng_reg_base->dig_ps_hdsw_en = 1;

	/**3.trong work clock enable*/
	trng_reg_base->trng_clk_gating_en = 1;

	if(DelayTimeUs != 0)
	{
		process_reg_base->process_sensor_count = DelayTimeUs;
	}

	/**4. start*/
	process_reg_base->process_sensor_start = 1;
	os_usdelay(2);

	/**5.read data*/
	pProcesscnt[0] = (process_reg_base->logic_ps_cnts_out0) & 0x3FFF;//HVT
	pProcesscnt[1] = (process_reg_base->logic_ps_cnts_out1) & 0x3FFF;//LVT
	pProcesscnt[2] = (process_reg_base->logic_ps_cnts_out2) & 0x3FFF;//SVT
	
	/**6.disabel clk*/
	trng_reg_base->trng_clk_gating_en = 0;
	trng_reg_base->dig_ps_hdsw_en = 0;
	chip_clk_disable(CLK_TRNG_APB);
	chip_clk_disable(CLK_TRNG_WORK);
	
	system_irq_restore(flags);

}



