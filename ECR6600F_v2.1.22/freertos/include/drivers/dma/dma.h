

#ifndef DRV_DMA_H
#define DRV_DMA_H

#include "chip_configuration.h"

typedef struct
{
	void (* callback)(void *);
	void *data;
}T_DRV_DMA_CALLBACK;


typedef enum
{
	DMA_CHN_UART0_TX = 0,
	DMA_CHN_UART0_RX,
	DMA_CHN_UART1_TX,
	DMA_CHN_UART1_RX,
	DMA_CHN_I2C_TX,
	DMA_CHN_I2C_RX,
	DMA_CHN_SPI_TX,
	DMA_CHN_SPI_RX,
	DMA_CHN_SPI_FLASH_TX,
	DMA_CHN_SPI_FLASH_RX,
	DMA_CHN_I2S_TX,
	DMA_CHN_I2S_RX,
	DMA_CHN_UART2_TX,
	DMA_CHN_UART2_RX,
	DMA_CHN_MEM
} E_DMA_CHN_MODE;

typedef struct
{
	unsigned int src;
	unsigned int dst;
	unsigned int len;
	E_DMA_CHN_MODE mode;
} T_DMA_CFG_INFO;

void drv_dma_status_clean(int num);
int drv_dma_cfg(int num, T_DMA_CFG_INFO * cfg);
void drv_dma_start(int num);
unsigned int drv_dma_stop(int num);
void drv_dma_isr_register(int num, void (* callback)(void *), void  *data);
int drv_dma_ch_alloc(void);


#endif /* DRV_DMA_H */

