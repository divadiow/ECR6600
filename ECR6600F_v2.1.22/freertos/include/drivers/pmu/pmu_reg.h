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
#ifndef __PMU_REG_H__
#define __PMU_REG_H__

#include "chip_memmap.h"

/*refer to PMU macro*/
#define PCU_AO_CTRL_REG0       						(MEM_BASE_PCU + 0)/*base addr: 0x00201000*/
#define PCU_AO_CTRL_REG1 							(MEM_BASE_PCU + 0x4)
#define PCU_BUCK_CTRL_REG0 							(MEM_BASE_PCU + 0x8)
#define PCU_BUCK_CTRL_REG1 							(MEM_BASE_PCU + 0xC)
#define PCU_BUCK_CTRL_REG2 							(MEM_BASE_PCU + 0x10)
#define PCU_BG_CTRL_REG 							(MEM_BASE_PCU + 0x14)
#define PCU_DCXO_CTRL_REG0 							(MEM_BASE_PCU + 0x18)
#define PCU_DCXO_CTRL_REG1 							(MEM_BASE_PCU + 0x1C)

/*refer to PCU macro */
#define PCU_BASE_ADDR 				 		  		(MEM_BASE_PCU)/*base addr: 0x00201000*/
#define PCU_RST_BYPASS_CTRL_REG						(MEM_BASE_PCU + 0x20) 	/*reset bypass*/
#define PCU_FSM_TIMINT_CTRL_REG						(MEM_BASE_PCU + 0x24)	/*pmu fsm timing*/
#define PCU_PSHOLD_CTRL_REG							(MEM_BASE_PCU + 0x28)	/*pshold*/
#define PCU_SLEEP_CTRL_REG 							(MEM_BASE_PCU + 0x2C)	/*sleep ctrl*/
#define PCU_POWER_CTRL_REG							(MEM_BASE_PCU + 0x30)	/*power ctrl*/
#define PCU_TEST_MODE_CTRL_REG						(MEM_BASE_PCU + 0x34)	/*test mode ctrl*/
#define PCU_WFI_CTRL_REG							(MEM_BASE_PCU + 0x38)	/*WFI sleep ctrl*/
#define PCU_DEEP_SLEEP_CTRL_REG						(MEM_BASE_PCU + 0x3C)	/*deep sleep ctrl*/
#define PCU_SDN_CTRL_REG							(MEM_BASE_PCU + 0x40)	/*sdn ctrl*/
#define PCU_SDN_PRT_CTRL_REG						(MEM_BASE_PCU + 0x44)	/*sdn protect ctrl*/
#define PCU_WAKEUP_CTRL_REG							(MEM_BASE_PCU + 0x48)	/*wakeup ctrl*/
#define PCU_INT_ENA_CTRL_REG						(MEM_BASE_PCU + 0x4C)	/*int ena ctrl*/
#define PCU_INT_CLEAR_CTRL_REG						(MEM_BASE_PCU + 0x50)	/*int clear ctrl*/
#define PCU_INT_STATUS_CTRL_REG						(MEM_BASE_PCU + 0x54)	/*int status ctrl*/
#define PCU_FSM_STATE_CTRL_REG						(MEM_BASE_PCU + 0x58)	/*fsm state ctrl*/
#define PCU_STANDBY_CTRL_REG						(MEM_BASE_PCU + 0x5C)	/*standby ok bypass ctrl*/

/*PMU_AO_CTRL_REG0*/
#define DIG_FLASH_PWR_EN			    (1<<15)
#define DIG_LPBG_VTRIM_SLEEP(x)		    ((x)<<6)
#define DIG_LPBG_VTRIM_FUNC(x)		    ((x)<<2)

#define DIG_LPLDO_LOADOFF_EN		    (1<<1)
#define DIG_AO_ATST_EN				    (1<<0)

/*PMU_AO_CTRL_REG1*/
#define DIG_32KXTAL_EN				    (1<<27)
#define DIG_32KRC_EN				    (1<<17)

/*PMU_BUCK_CTRL_REG2	buck control*/
#define DIG_DIGLDO_ATST_EN 			    (1<<9)
#define DIG_ANALDO_ATST_EN			    (1<<0)

/*PMU_BUCK_CTRL_REG2	buck test*/
#define DIG_SIMO_DISPSW_HALF 		    (1<<13)
#define DIG_SIMO_DISNSW_HALF		    (1<<12)
#define DIG_SIMO_TEST_EN			    (1<<2)

/*PMU_BG_CTRL_REG5		MBG*/
#define DIG_BGM_ADC_0P8_SEL 		    (1<<20)
#define DIG_BGM_LPF_FASTCHARGE_EN	    (1<<0)

/*RF_CTRL_DCXO_REG6		dcxo_core*/
#define DIG_ARI_WIFI_DCXO_PU 		    (1<<24)
#define DIG_ARI_WIFI_DCXO_FBRES_EN	    (1<<9)
#define DIG_ARI_WIFI_DCXO_FBRES_FASTSET	(1<<8)

/*PCU_CTRL_REG8			reset bypass*/
#define REG_AON_RST_BYPASS 		        (1<<2)
#define REG_PDCORE_RST_BYPASS	        (1<<1)
#define PMU_HOT_RSTN_BYPASS		        (1<<0)
#define PMU_ALL_RSTN_BYPASS_DISABLE	    0x0


/*PCU_CTRL_REG10	pshold*/
#define PS_HOLD 		                (1<<0)

/*PCU_CTRL_REG11	sleep ctrl*/
#define PWR_CTRL_STATE_BASE_ADDR 		(0xaf800<<8)
#define PWR_CTRL_RESUME_FROM_RESET		(0x2<<5)
#define PWR_CTRL_RESUME_FROM_IRAM		(0x3<<5)
#define PWR_CTRL_ENTER_STANDBY			(0x0<<2)
#define PWR_CTRL_STORE_GPR				(0x1<<2)
#define NO_CACHE_INIT_ENA		 		(0x1<<1)
#define KEYON_NEGEDGE_EDGE_TURNOFF_ENA	(0x1<<0)

/*PCU_CTRL_REG14	WFI sleep*/
#define WFI_SLEEP_ENA					(0x1<<0)
#define WFI_SLEEP_DISABLE				(0x0<<0)

/*PCU_CTRL_REG15	deep sleep*/
#define PWR_REQ_EN						(0x1<<8)
#define IRAM_RET_ENA					(0x1<<7)
#define DLM1_RET_ENA					(0x1<<6)
#define ILM1_DLM0_RET_ENA				(0x1<<5)
#define ILM0_RET_ENA					(0x1<<4)
#define CHIP_SLEEP_MODE_ENA				(0x1<<0)
#define DEEP_SLEEP_MEMORY_ON			(0x0f1)
#define DEEP_SLEEP_MEMORY_RETENTION		(0x001)

/*#define LIGHT_SLEEP_ENA					(0x1f1)*/
#define LIGHT_SLEEP_ENA					(0x1f1)

/*PCU_CTRL_REG16	sdn*/
#define CHIP_SDN_MODE_ENA				(0x1<<0)

//PCU_CTRL_REG17	sdn protect

/*PCU_CTRL_REG18	wakeup ena*/
#define GPIO2_WAKUP_ENA					(0x1<<6)
#define GPIO1_WAKUP_ENA					(0x1<<5)
#define GPIO0_WAKUP_ENA					(0x1<<4)
#define GPIO18_WAKUP_ENA				(0x1<<3)
#define GPIO17_WAKUP_ENA				(0x1<<2)
#define CLK_32K_GATING_EN_BYPASS		(0x1<<1)
#define MAIN_CLKEN_OFF_ENABLE			(0x1<<0)

/*PCU_CTRL_REG19	int ena*/
#define PCU_INT_ALL_DISABLE				0
#define PCU_INT_TO_CPU_MASK				(0x1<<4)
#define RTC_WAKEUP_INT_ENABLE			(0x1<<3)
#define WAKEUP_EXT_FALL_INT_ENABLE		(0x1<<2)
#define WAKEUP_EXT_RISE_INT_ENABLE		(0x1<<1)
#define KEYON_WAKEUP_NE_INT_ENABLE		(0x1<<0)

/*PCU_CTRL_REG20	int clear*/			
#define PCU_INT_CLEAR					(0x1<<0)

/*PCU_CTRL_REG21	int status	*/
#define RTC_WAKEUP_INT_STATUS			(0x1<<11)	
#define WAKEUP_EXT_FALL_INT_STATUS		(0x1<<6)
#define WAKEUP_EXT_RISE_INT_STATUS		(0x1<<1)
#define KEYON_WAKEUP_NE_INT_STATUS	 	(0x1<<0)

/*PCU_CTRL_REG23	standby ok bypass*/
#define WAIT_CPU_STANDBY				(0x0<<0)

/*clk_enable_ctrl_reg1*/
//#define reserved						(0x1<<30)
//#define reserved						(0x1<<29)
//#define reserved						(0x1<<28)
//#define reserved						(0x1<<27)
//#define reserved						(0x1<<26)
//#define reserved						(0x1<<25)
//#define reserved						(0x1<<24)
#define AUX_ADC_CLK_EN					(0x1<<23)
#define TX_CLK_EN						(0x1<<22)
#define BLE_RX_CLK_EN					(0x1<<21)
#define BLE_TX_CLK_EN					(0x1<<20)
#define AX_CLK_160M_LDPC_EN				(0x1<<19)
#define TRNG_RBG_AHB_HCLK_EN			(0x1<<18)	//map to o_spi1_work_clk_40M_sel 0---crystal 40M 1--- 160M_div4
#define ADC_AUX_40M_CLK_EN				(0x1<<17)
#define DFE_AHB_HCLK_EN					(0x1<<16)
//#define reserved						(0x1<<15)
#define RF_REG_40M_CLK_EN				(0x1<<14)
#define DOT_RX_CLK_EN					(0x1<<13)
#define DOT_CLK_CGEN					(0x1<<12)
#define SMIH_LP_CLK_EN					(0x1<<11)
#define BLE_CLK_EN						(0x1<<10)
#define RX_CLK_EN						(0x1<<9)
#define WLAN_CLK_EN						(0x1<<8)
//#define reserved						(0x1<<7)
//#define reserved						(0x1<<6)
#define I2S_CLK_EN						(0x1<<5)
#define DMA_AHB_HCLK_EN					(0x1<<4)
#define AES_AHB_HCLK_EN					(0x1<<3)
#define HASH_AHB_HCLK_EN				(0x1<<2)
#define ECC_AHB_HCLK_EN					(0x1<<1)
#define BLE_AHB_HCLK_EN					(0x1<<0)

#endif
