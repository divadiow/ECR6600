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
#ifndef __PD_CONRE_REG_H__
#define __PD_CONRE_REG_H__

#include "chip_memmap.h"

/*refer to PD_SMU macro*/
#define PD_SMU_BASE_ADDR 				 			(0x00202000)
#define PD_CORE_CTRL_REG 				 			(PD_SMU_BASE_ADDR + 0x4)/*cpu clk mux sel ctrl*/
#define PD_CLK_ENABLE_CTRL_REG0 				 	(PD_SMU_BASE_ADDR + 0xC)/*clk gating enable*/
#define PD_CLK_ENABLE_CTRL_REG1				 		(PD_SMU_BASE_ADDR + 0x10)/*clk gating enable*/
#define PD_SW_RST_CTRL_REG				 			(PD_SMU_BASE_ADDR + 0x14)/*software reset reg*/
#define PD_SPI_CONFIG_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x18)/*spi config*/
#define PD_SDIO_CONFIG_CTRL_REG0				 	(PD_SMU_BASE_ADDR + 0x20)/*sdio config*/
#define PD_SDIO_CONFIG_CTRL_REG1				 	(PD_SMU_BASE_ADDR + 0x24)/*sdio config*/
#define PD_SDIO_HOST_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x28)/*sdio host*/
#define PD_SDIO_SLAVE_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x2C)/*sdio slave*/
#define PD_PIN_MUX_CTRL_REG0				 		(PD_SMU_BASE_ADDR + 0x30)/*pin mux0*/
#define PD_PIN_MUX_CTRL_REG1				 		(PD_SMU_BASE_ADDR + 0x34)/*pin mux1*/
#define PD_PIN_MUX_CTRL_REG2				 		(PD_SMU_BASE_ADDR + 0x38)/*pin mux2*/
#define PD_AUX_ADC_SEL_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x3C)/*aux adc sel*/
#define PD_AUX_ADC_OUT_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x40)/*aux adc out*/
#define PD_RTC_CAL_CTRL_REG				 			(PD_SMU_BASE_ADDR + 0x44)/*rtc cal config*/
#define PD_RTC_CNT_CTRL_REG				 			(PD_SMU_BASE_ADDR + 0x48)/*rtc cnt*/
#define PD_I2S_MOD_CTRL_REG				 			(PD_SMU_BASE_ADDR + 0x4C)/*i2s mode*/
#define PD_UART_CLK_SEL_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x50)/*uart clk sel*/
#define PD_WIFI_BLE_SEL_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x54)/*wifi ble sel*/
#define PD_ENDIAN_CTRL_REG				 			(PD_SMU_BASE_ADDR + 0x58)/*endian*/
#define PD_HASH_DONE_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x5C)/*hash done*/
#define PD_I2S_DIV_INT_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x60)/*i2s div int reg*/
#define PD_I2S_DIV_FRAC_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x64)/*i2s div frac reg*/
#define PD_TRX_BYPASS_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x68)/*trx bypass ctrl*/
#define PD_IVIC_TRIG_TYPE_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x6C)/*ivic trig type*/
#define PD_PIN_MUX_CTRL_REG3				 		(PD_SMU_BASE_ADDR + 0x70)/*pin mux3*/
#define PD_CLk_DIV_ENBLE_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x74)/*clk div enable ctrl*/
#define PD_APB_ENCRYPT_EN_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x78)/*apb_encrypt_en*/
#define PD_RAM_ECC_HASH_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x7C)/*ram_ecc_hash_sel*/
#define PD_AUX_CLK_40M_32M_SEC_CTRL_REG				(PD_SMU_BASE_ADDR + 0x80)/*aux_clk_40m_32m_sel*/
#define PD_AUX_ADC_FIFO_DATA_CTRL_REG				(PD_SMU_BASE_ADDR + 0x84)/*aux adc fifo data*/
#define PD_AUX_AUX_ADC_FIFO_WR_CFG_CTRL_REG			(PD_SMU_BASE_ADDR + 0x88)/*aux adc fifo wr cfg*/
#define PD_WIFI_BLE_MANUAL_MOD_CTRL_REG				(PD_SMU_BASE_ADDR + 0x8C)/*wifi ble manual mode*/
#define PD_AUX_GAIN_CAL_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x90)/*aux gain cal fix*/
#define PD_AUX_VOS_DATA_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0x94)/*aux vos data fix*/
#define PD_AUX_ADC_CLR_CTRL_REG				 		(PD_SMU_BASE_ADDR + 0x98)/*aux adc clr*/
#define PD_APB_PDSMU_CLK_BYPASS_CTRL_REG			(PD_SMU_BASE_ADDR + 0x9C)/*apb pdsmu clk bypass*/
#define PD_DFE_RST_BYPASS_CTRL_REG				 	(PD_SMU_BASE_ADDR + 0xA0)/*dfe rst bypass*/

/*PD_SMU*/
#define CPU_CLK_FREQ_DIV0				(0x1<<2)
#define CPU_CLK_FREQ_DIV2				(0x0<<2)
#define CPU_CLK_FREQ_DIV4				(0x2<<2)
#define CPU_CLK_FREQ_DIV8				(0x3<<2)

#define CPU_CLK_26_40M					(0x0<<0)
#define CPU_CLK_240M					(0x1<<0)
#define CPU_CLK_160M					(0x2<<0)
#define CPU_CLK_96M						(0x3<<0)

#define CPU_CLK_26_40M					(0x0<<0)
#define CPU_CLK_240M					(0x1<<0)
#define CPU_CLK_160M					(0x2<<0)
#define CPU_CLK_96M						(0x3<<0)

#define CLK_ALL_ENABLE					0x7FFFFFFF
#define CLK1_ALL_ENABLE					0xFFFFFF	
#define CLK_ALL_DISABLE					0x0
#define CLK1_ALL_DISABLE				0x0
#define PHY_AHB_HCLK_EN					(0x1<<30)
#define I2S_AHB_HCLK_EN					(0x1<<29)
#define FLASH_AHB_HCLK_EN				(0x1<<28)
#define IRAM1_AHB_HCLK_EN				(0x1<<27)
#define IRAM0_AHB_HCLK_EN				(0x1<<26)
#define SDIO_H_AHB_HCLK_EN				(0x1<<25)
#define SDIO_S_AHB_HCLK_EN				(0x1<<24)
#define IQ_DUMP_APB_PCLK_EN				(0x1<<23)
#define ADC_CAL_40M_CLK_EN				(0x1<<22)
#define PIT1_APB_PCLK_EN				(0x1<<21)
#define PIT0_APB_PCLK_EN				(0x1<<20)
#define RF_REG_APB_PCLK_EN				(0x1<<19)
#define EFUSE_APB_PCLK_EN				(0x1<<18)
#define EFUSE_CLK_EN					(0x1<<17)
#define TRNG_APB_PCLK_EN				(0x1<<16)
#define TRNG_CLK_EN						(0x1<<15)
#define WDT_EXT_CLK_32K_EN				(0x1<<14)
#define WDT_APB_PCLK_EN					(0x1<<13)
//#define reserved						(0x1<<12)
#define IR_APB_PCLK_EN					(0x1<<11)
#define FREF_CLK_EN						(0x1<<10)
#define I2C_APB_CLK_EN					(0x1<<9)
#define SPI1_APB_CLK_EN					(0x1<<8)
#define SPI0_APB_CLK_EN					(0x1<<7)
#define UART2_APB_CLK_EN				(0x1<<6)
#define UART2_UCLK						(0x1<<5)
#define UART1_APB_CLK_EN				(0x1<<4)
#define UART1_UCLK						(0x1<<3)
#define UART0_APB_CLK_EN				(0x1<<2)
#define UART0_UCLK						(0x1<<1)
#define SMU_APB_PCLK_EN					(0x1<<0)

#endif
