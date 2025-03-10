/*******************************************************************************
 * Copyright by eswin 
 *
 * File Name:  
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v0.1
 * Author:        
 * Date:          
 * History 0:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
  ********************************************************************************/
#ifndef __AON_REG_H__
#define __AON_REG_H__

#include "chip_memmap.h"

/*refer to AON_SMU MACRO*/
#define AON_SMU_BASE_ADDR							(0x00201200)
#define AON_CHIP_ID_REG								(MEM_BASE_SMU_AON + 0x0)
/*wakeup_addr, CPU  start address after reset*/
#define AON_WAKEUP_ADDR_REG							(MEM_BASE_SMU_AON + 0x4)
/*clk_32k_src_sel 0--rc_clk_32k 1--crystal32k_clk*/
#define AON_32KCLK_SEL_REG							(MEM_BASE_SMU_AON + 0x8)
/*1:ilm   0:cache*/
#define AON_ILM_CACHE_SW_REG						(MEM_BASE_SMU_AON + 0x10)
#define AON_TEST_MODE_EXT_RST_REG					(MEM_BASE_SMU_AON + 0x14)
/*1:set gpio pull up */
#define AON_PAD_PE_REG								(MEM_BASE_SMU_AON + 0x18)
/*1:set gpio pull down*/
#define AON_PAD_PS_REG								(MEM_BASE_SMU_AON + 0x1C)
/*1:input enable*/
#define AON_PAD_IE_REG								(MEM_BASE_SMU_AON + 0x20)
/*0--from pd_core pimux £¬the IO control signals from pdcore pinmux,
1-- gpio mode£¬the control signals form gpio module*/
#define AON_PAD_MODE_REG							(MEM_BASE_SMU_AON + 0x24)
/*IO driver strength £»0--4ma 1-8ma£» bit0--GPIO0£¬bit1--GPIO1*/
#define AON_PAD_SR_REG								(MEM_BASE_SMU_AON + 0x28)
/*agc_ram_sd -> dummy[31],agc_ram_ds -> dummy[30],agc_ram_iso -> dummy[29]*/
#define AON_DUMMY_REG								(MEM_BASE_SMU_AON + 0x34)
/*0-- disenable  1--enable */
#define AON_EFUSE_JTAG_EN_REG						(MEM_BASE_SMU_AON + 0x38)
#define AON_BOOTMODE_REG							(MEM_BASE_SMU_AON + 0x3C)
#define AON_APB_BYPASS_REG 							(MEM_BASE_SMU_AON + 0x40)

#define WAKEUP_ADDR_ENA					(0x1<<0)

#define GPIO_ALL_PAD_GPPIO_MODE			0x3ffffff
#define GPIO25_PAD_GPPIO_MODE			(0x1<<25)
#define GPIO24_PAD_GPPIO_MODE			(0x1<<24)
#define GPIO23_PAD_GPPIO_MODE			(0x1<<23)
#define GPIO22_PAD_GPPIO_MODE			(0x1<<22)
#define GPIO21_PAD_GPPIO_MODE			(0x1<<21)
#define GPIO20_PAD_GPPIO_MODE			(0x1<<20)
#define GPIO19_PAD_GPPIO_MODE			(0x1<<19)
#define GPIO18_PAD_GPPIO_MODE			(0x1<<18)
#define GPIO17_PAD_GPPIO_MODE			(0x1<<17)
#define GPIO16_PAD_GPPIO_MODE			(0x1<<16)
#define GPIO15_PAD_GPPIO_MODE			(0x1<<15)
#define GPIO14_PAD_GPPIO_MODE			(0x1<<14)
#define GPIO13_PAD_GPPIO_MODE			(0x1<<13)
#define GPIO12_PAD_GPPIO_MODE			(0x1<<12)
#define GPIO11_PAD_GPPIO_MODE			(0x1<<11)
#define GPIO10_PAD_GPPIO_MODE			(0x1<<10)
#define GPIO9_PAD_GPPIO_MODE			(0x1<<9)
#define GPIO8_PAD_GPPIO_MODE			(0x1<<8)
#define GPIO7_PAD_GPPIO_MODE			(0x1<<7)
#define GPIO6_PAD_GPPIO_MODE			(0x1<<6)
#define GPIO5_PAD_GPPIO_MODE			(0x1<<5)
#define GPIO4_PAD_GPPIO_MODE			(0x1<<4)
#define GPIO3_PAD_GPPIO_MODE			(0x1<<3)
#define GPIO2_PAD_GPPIO_MODE			(0x1<<2)
#define GPIO1_PAD_GPPIO_MODE			(0x1<<1)
#define GPIO0_PAD_GPPIO_MODE			(0x1<<0)


#endif
