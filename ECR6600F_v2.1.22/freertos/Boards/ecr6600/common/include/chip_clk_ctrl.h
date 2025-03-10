

#ifndef _CHIP_CLK_CTRL_H
#define _CHIP_CLK_CTRL_H



#if defined (CONFIG_CPU_CLK_SRC_96M)

#define CHIP_CLOCK_SOURCE		96000000
#define UART_U_CLOCK			80000000

#elif defined (CONFIG_CPU_CLK_SRC_160M)

#define CHIP_CLOCK_SOURCE		160000000
#define UART_U_CLOCK			80000000

#elif defined (CONFIG_CPU_CLK_SRC_240M)

#define CHIP_CLOCK_SOURCE		240000000
#define UART_U_CLOCK			80000000

#elif defined (CONFIG_CPU_CLK_SRC_40m)

#define CHIP_CLOCK_SOURCE		40000000
#define UART_U_CLOCK			40000000

#else

#error CPU_CLOCK_SOURCE must be defined 

#endif


#if defined (CONFIG_CPU_CLK_FREQ_DIV_0)

#define CHIP_CLOCK_CPU			(CHIP_CLOCK_SOURCE)

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_2)

#define CHIP_CLOCK_CPU			(CHIP_CLOCK_SOURCE/2)

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_4)

#define CHIP_CLOCK_CPU			(CHIP_CLOCK_SOURCE/4)

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_8)

#define CHIP_CLOCK_CPU			(CHIP_CLOCK_SOURCE/8)

#else

#error CHIP_CLOCK_CPU must be defined 

#endif

#ifdef CONFIG_FPGA
#define CHIP_CLOCK_AHB		20000000
#define CHIP_CLOCK_APB		10000000
#else
#define CHIP_CLOCK_AHB		(CHIP_CLOCK_CPU/2)
#define CHIP_CLOCK_APB		(CHIP_CLOCK_CPU/4)
#endif




#define CLK_SMU_APB			0x0000
#define CLK_UART0_UCLK		0x0001
#define CLK_UART0_APB		0x0002
#define CLK_UART1_UCLK		0x0003
#define CLK_UART1_APB		0x0004
#define CLK_UART2_UCLK		0x0005
#define CLK_UART2_APB		0x0006
#define CLK_SPI0_APB		0x0007
#define CLK_SPI1_APB		0x0008
#define CLK_I2C				0x0009
#define CLK_DIG_PLL_FREF	0x000A
#define CLK_IR				0x000B
#define CLK_RESERVED0		0x000C
#define CLK_WDT_APB		0x000D
#define CLK_WDT_EXT		0x000E
#define CLK_TRNG_WORK		0x000F
#define CLK_TRNG_APB		0x0010
#define CLK_RF_FREF			0x0011
#define CLK_EFUSE			0x0012
#define CLK_RF_APB			0x0013
#define CLK_PIT0				0x0014
#define CLK_PIT1				0x0015
#define CLK_ADC_CAL_40M	0x0016
#define CLK_IQ_DUMP			0x0017
#define CLK_SDIO_SLAVE		0x0018
#define CLK_SDIO_HOST		0x0019
#define CLK_IRAM0			0x001A
#define CLK_IRAM1			0x001B
#define CLK_SPI0_AHB		0x001C
#define CLK_I2S_AHB			0x001D
#define CLK_PHY				0x001E

#define CLK_BLE_AHB			0x0100
#define CLK_ECC				0x0101
#define CLK_HASH			0x0102
#define CLK_AES				0x0103
#define CLK_DMA				0x0104
#define CLK_I2S				0x0105
#define CLK_RESERVED1		0x0106
#define CLK_RESERVED2		0x0107
#define CLK_WIFI				0x0108
#define CLK_WIFI_RX			0x0109
#define CLK_BLE				0x010A
#define CLK_SMIH_LP			0x010B
#define CLK_DOT				0x010C
#define CLK_DOT_RX			0x010D
#define CLK_RFC_40M			0x010E
#define CLK_RESERVED3		0x010F
#define CLK_WIFI_DFE		0x0110
#define CLK_ADC_AUX_40M	0x0111
#define CLK_SPI1_SEL		0x0112	
#define CLK_WIFI_LDPC		0x0113
#define CLK_BLE_TX			0x0114
#define CLK_BLE_RX			0x0115
#define CLK_WIFI_TX			0x0116
#define CLK_AUX_ADC			0x0117


void chip_clk_enable(unsigned int clk_id);
void chip_clk_disable(unsigned int clk_id);

void chip_clk_init(void);
void chip_clk_config_40M_26M(void);
void chip_clk_config_40M_26M_efuse(void);
//void chip_clk_config_retore(void);

#endif

