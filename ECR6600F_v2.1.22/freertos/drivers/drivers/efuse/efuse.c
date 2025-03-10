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
	desc	add notes & move reload in write api \n
*/




#include <stdio.h>
#include <string.h>
#include "chip_memmap.h"
#include "chip_smupd.h"
#include "chip_clk_ctrl.h"
#include "pmu_reg.h"
#include "efuse.h"
#include "oshal.h"
#include "cli.h"
#include "pit.h"
#include "aon_reg.h"
#include "psm_mode_ctrl.h"

#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

static unsigned int efuse_status_map = 0;
static unsigned int efuse_read_buf[EFUSE_BUFF_MAX_INDEX] = {0};
//#ifndef CHIP_GET_REG
//#define CHIP_GET_REG(reg)                (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef CHIP_SET_REG
//#define CHIP_SET_REG(reg, data)         ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif



typedef  volatile struct _T_EFUSE_REG_MAP
{
	unsigned int PgmTiming1;	/* 0x00  -- program timing 1 register */
	unsigned int PgmTiming2;	/* 0x04  -- program timing 2 register */
	unsigned int PgmTiming3;	/* 0x08  -- program timing 3 register */
	unsigned int Resv0;			/* 0x0C  --reserved register */
	unsigned int RdTiming1;		/* 0x10  -- read timing 1 register */
	unsigned int Resv1;			/* 0x14  -- reserved register */
	unsigned int RdTiming2;		/* 0x18  -- read timing 2register */
	unsigned int InitDone;		/* 0x1C  -- init done register */
	unsigned int Reload;		/* 0x20  -- reload register */
	unsigned int PgmMask;		/* 0x24  -- reserved register */
	unsigned int Resv2[54];		/* 0x28 ~ 0xFC  -- reserved register */	
	unsigned int Info[32];		/* 0x100 -- store efuse info register */
	unsigned int Resv3[32];		/* 0x180 ~ 0x1FC  -- reserved register */	
	unsigned int Pgm[32];		/* 0x200 -- efuse program register */
	unsigned int Resv4[32];		/* 0x280 ~ 0x2FC  -- reserved register */	
	unsigned int Rd[32];		/* 0x300 -- efuse read register */
} T_EFUSE_REG_MAP;


/**    @brief		Read efuse data api.
 *	   @details 	Read the data of the specified length from the specified address in the efuse memory into the *value.
 *     @param[in]	addr--The address where you want to read data in efuse.
 *     @param[in]	* value--Data of specified length read from specified address in efuse memory.
 *     @param[in]	length--The specified length read from the specified address in efuse memory. 
 */
int drv_efuse_read(unsigned int addr,unsigned int * value, unsigned int length)
{
	T_EFUSE_REG_MAP * p_efuse_reg = (T_EFUSE_REG_MAP * )MEM_BASE_EFUSE;
	unsigned int index = addr/4;
	unsigned int len = (length+3)/4;
	unsigned int irq_flag;
	unsigned int clk_reg_0 = 0, clk_reg_1 = 0;
	
	if((index >= EFUSE_MAX_LENGTH) || ((index + len) > EFUSE_MAX_LENGTH) || (value == NULL))
	{
		return EFUSE_RET_PARAMETER_ERROR;
	}
	
	irq_flag = system_irq_save();

	while(p_efuse_reg->InitDone == 0);

	while(len--)
	{
		if((efuse_status_map & (1 << (index))) == 0)			
		{
			if(index < EFUSE_AON_MAX_INDEX)
			{
				clk_reg_0 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
				clk_reg_1 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_1);
				//enable agc 
				chip_clk_enable(CLK_DIG_PLL_FREF);
				chip_clk_enable(CLK_PHY);
				chip_clk_enable(CLK_WIFI);
				WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) | (0x1 << 12));	//AGCFSMRESET //SWITCH TO AHB CONTROL
				WRITE_REG(AON_DUMMY_REG,READ_REG(AON_DUMMY_REG) & ~( AGC_RAM_DS |AGC_RAM_ISO));
				
				efuse_read_buf[index] = READ_REG(EFUSE_AON_DATA + (index * 4));
				efuse_status_map |= (1 << (index));
					
				//disable agc 
				WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) & ~(0x1 << 12));  //AGCFSMRESET //SWITCH TO AGC FSM
				WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, clk_reg_0);
				WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_1, clk_reg_1);
			}
			else
			{
				unsigned int clk_enable, core_ctrl, clk_dev_en;
				//Backup configuration 
				clk_enable = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
				core_ctrl = READ_REG(CHIP_SMU_PD_CORE_CTRL);
				clk_dev_en = READ_REG(CHIP_SMU_PD_CLK_DIV_EN);
				
				//change CPU clk to 40/26M
				chip_clk_config_40M_26M_efuse();
				
				//read efuse to buff 
				efuse_read_buf[index] = p_efuse_reg->Rd[index];
				efuse_status_map |= (1 << (index));
				
				//Restore backup configuration
				WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0,clk_enable);
				WRITE_REG(CHIP_SMU_PD_CORE_CTRL, core_ctrl);
				drv_pit_delay(100);
				WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN,clk_dev_en);
			}
		}

		*value = efuse_read_buf[index];
		index++;
		value++;
	}
	
	system_irq_restore(irq_flag);
	return EFUSE_RET_SUCCESS;
}

void drv_efuse_read_all(void)
{
	T_EFUSE_REG_MAP * p_efuse_reg = (T_EFUSE_REG_MAP * )MEM_BASE_EFUSE;
	unsigned int index = 0;
	unsigned int irq_flag;
	unsigned int clk_reg_0 = 0, clk_reg_1 = 0;
	
	irq_flag = system_irq_save();
	
	while(p_efuse_reg->InitDone == 0);

	for(index = 0; index < EFUSE_MAX_LENGTH; index++)
	{
		if(index < EFUSE_AON_MAX_INDEX)
		{
			clk_reg_0 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
			clk_reg_1 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_1);
			//enable agc 
			chip_clk_enable(CLK_DIG_PLL_FREF);
			chip_clk_enable(CLK_PHY);
			chip_clk_enable(CLK_WIFI);
			WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) | (0x1 << 12));	//AGCFSMRESET //SWITCH TO AHB CONTROL

			WRITE_REG((EFUSE_AON_DATA + (index * 4)), p_efuse_reg->Rd[index]);
			//WRITE_REG(EFUSE_AON_MAP, READ_REG(EFUSE_AON_MAP) | (1 << index));	
			
			efuse_read_buf[index] = p_efuse_reg->Rd[index];
			efuse_status_map |= (1 << (index));
			
			//disable agc 
			WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) & ~(0x1 << 12));  //AGCFSMRESET //SWITCH TO AGC FSM
			WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, clk_reg_0);
			WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_1, clk_reg_1);
		}
		else
		{
			efuse_read_buf[index] = p_efuse_reg->Rd[index];
			efuse_status_map |= (1 << (index));
		}
	}
	
	system_irq_restore(irq_flag);
}


/**    @brief		Write data to efuse api.
 *	   @details 	Write the specified length of data to the specified address into the efuse memory.
 *     @param[in]	addr--The address where you want to write data to efuse.
 *     @param[in]	* value--The data information that you want to write to the specified address of efuse.
 *     @param[in]	mask--Controls which bits can be written.In the mask, bit 0 means that it can be written, and bit 1 means that it cannot be written.
 */
int drv_efuse_write(unsigned int addr, unsigned int value, unsigned int mask)
{	
	T_EFUSE_REG_MAP * p_efuse_reg = (T_EFUSE_REG_MAP * )MEM_BASE_EFUSE;
	unsigned int index = addr/4;
	unsigned int irq_flag;
	unsigned int clk_reg_0 = 0, clk_reg_1 = 0;
	unsigned int clk_enable, core_ctrl, clk_dev_en;
	if(index >= EFUSE_MAX_LENGTH)
	{
		return EFUSE_RET_PARAMETER_ERROR;
	}
	
	irq_flag = system_irq_save();
	//Backup configuration 
    clk_enable = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
    core_ctrl = READ_REG(CHIP_SMU_PD_CORE_CTRL);
    clk_dev_en = READ_REG(CHIP_SMU_PD_CLK_DIV_EN);
	
	//change CPU clk to 40/26M
	chip_clk_config_40M_26M_efuse();
	
	//write efuse
	while(p_efuse_reg->InitDone == 0);
	p_efuse_reg->PgmMask = mask;
	p_efuse_reg->Pgm[index] = value;

	//write AON & buff	
	if(index < EFUSE_AON_MAX_INDEX)
	{
		clk_reg_0 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
		clk_reg_1 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_1);
		//enable agc 
		chip_clk_enable(CLK_DIG_PLL_FREF);
		chip_clk_enable(CLK_PHY);
		chip_clk_enable(CLK_WIFI);
		WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) | (0x1 << 12));	//AGCFSMRESET //SWITCH TO AHB CONTROL
		
		WRITE_REG((EFUSE_AON_DATA + (index * 4)),(READ_REG(EFUSE_AON_DATA + (index * 4)) | (~mask & value)));
		
		//disable agc 
		WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) & ~(0x1 << 12));  //AGCFSMRESET //SWITCH TO AGC FSM
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, clk_reg_0);
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_1, clk_reg_1);
	}
	
		efuse_read_buf[index] |= (~mask & value);

	
	//Restore backup configuration
    WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0,clk_enable);
 	WRITE_REG(CHIP_SMU_PD_CORE_CTRL, core_ctrl);
    drv_pit_delay(100);
    WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN,clk_dev_en);
	system_irq_restore(irq_flag);
	return EFUSE_RET_SUCCESS;
}


/**    @brief       Initialize efuse related configuration.
 *     @details     Select the corresponding configuration according to different frequencies.
 */
void drv_efuse_init(void)
{
	T_EFUSE_REG_MAP * p_efuse_reg = (T_EFUSE_REG_MAP * )MEM_BASE_EFUSE;
	unsigned int clk_reg_0 = 0, clk_reg_1 = 0;
	unsigned int refresh = 0;
	chip_clk_enable(CLK_EFUSE);
    #ifdef CONFIG_PSM_SURPORT
	/*for psm sleep age goto sleep too*/
    WRITE_REG(AON_DUMMY_REG,READ_REG(AON_DUMMY_REG)&(~(0x3<<29)));
    #endif
	// 40/26M config 
	p_efuse_reg->PgmTiming1 = 0x0101020C;
	p_efuse_reg->PgmTiming2 = 0x00000078;
	p_efuse_reg->PgmTiming3 = 0x03020101;
	p_efuse_reg->RdTiming1 = 0x0201010B;
	p_efuse_reg->RdTiming2 = 0x00010101;

	refresh = ((READ_REG(AON_DUMMY_REG) & (1<<27)) >> 27);
	
	//ps hold
	if(((READ_REG(PCU_PSHOLD_CTRL_REG) & 0x01) == 0) || (refresh == 1))
	{
		//buff init
		efuse_status_map = 0;
		memset(efuse_read_buf,0,EFUSE_BUFF_LENGTH);

		clk_reg_0 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_0);
		clk_reg_1 = READ_REG(CHIP_SMU_PD_CLK_ENABLE_1);
		//enable agc 
		chip_clk_enable(CLK_DIG_PLL_FREF);
		chip_clk_enable(CLK_PHY);
		chip_clk_enable(CLK_WIFI);
		WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) | (0x1 << 12));  //AGCFSMRESET //SWITCH TO AHB CONTROL

		memset((void *)EFUSE_AON_DATA, 0, EFUSE_AON_LENGTH);
		
		//disable agc 
		WRITE_REG(AGCFSMRESET, READ_REG(AGCFSMRESET) & ~(0x1 << 12));  //AGCFSMRESET //SWITCH TO AGC FSM
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, clk_reg_0);
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_1, clk_reg_1);
		
		drv_efuse_read_all();
		#ifndef TUYA_SDK_ADPT
		WRITE_REG(PCU_PSHOLD_CTRL_REG, READ_REG(PCU_PSHOLD_CTRL_REG) | 0x01);
		#endif

		//reset refresh
		if(refresh == 1)
		{
			WRITE_REG(AON_DUMMY_REG, READ_REG(AON_DUMMY_REG) & (~(0x1 << 27)));
		}
	}
}

#if 0
int drv_efuse_write_mac(unsigned char * mac)
{
	if (mac[0] % 2)
	{
		return -1;
	}
	unsigned int mac_value1 = 0, mac_value2 = 0;
	mac_value1 = (mac[0] | (mac[1]<<8) | (mac[2]<<16) | (mac[3]<<24));
	mac_value2 = (mac[4] | (mac[5]<<8));
	drv_efuse_write(EFUSE_MAC_ADDR, mac_value1, 0xFFFFFFFF);	
	drv_efuse_write(EFUSE_MAC_ADDR + 4, mac_value2, 0xFFFF);

	//check
	char mac_check[8] = {0};
	drv_efuse_read(EFUSE_MAC_ADDR, (unsigned int *)mac_check, 2);
	if ((mac_check[0] == mac_value1) && ((mac_check[1]&0xFFFF) == mac_value2))
	{
		os_printf(LM_OS, LL_INFO, "efuse write mac success\n");
		return 0;
	}
	else
	{
		os_printf(LM_OS, LL_INFO, "write mac = %d:%d:%d:%d:%d:%d\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		os_printf(LM_OS, LL_INFO, "read mac = %d:%d:%d:%d:%d:%d\n",mac_check[0],mac_check[1],mac_check[2],mac_check[3],mac_check[4],mac_check[5]);
		return -1;
	}

}

int drv_efuse_read_mac(unsigned char * mac)
{
	drv_efuse_read(EFUSE_MAC_ADDR, (unsigned int *)mac, 6);
	if (!(mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]))
	{
		//os_printf(LM_OS, LL_INFO, "No Mac In Efuse\n");
		return -1;
	}
	if (mac[0] % 2)
	{
		//os_printf(LM_OS, LL_INFO, "Mac In Efuse Error\n");
		return -1;
	}
	return 0;	
}
#endif


