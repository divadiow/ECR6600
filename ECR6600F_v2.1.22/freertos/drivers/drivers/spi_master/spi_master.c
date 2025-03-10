/**
* @file       spi_master.c
* @brief      spi_master driver code  
* @author     Wei feiting
* @date       2021-2-23
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include "spi.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "arch_irq.h"
#include <string.h>


static spi_buff_dev * p_spi_dev;
static spi_buff_dev_dma * p_spi_dev_dma;
static spi_interface_config_t spi_master;
static os_sem_handle_t spi_tx_process;
static os_sem_handle_t spi_rx_process;

#define SPI1_WORK_CLK_AHB_CLK 0x88
#define SPI_WAIT_FOREVER 0xffffffff
#define min(x, y)               (((x) < (y))? (x) : (y))	
#define SPI_MASTER_BUF_SIZE	1024
static unsigned char dma_spi_src[SPI_MASTER_BUF_SIZE] __attribute__((section(".dma.data"),aligned(4)));
static unsigned char dma_spi_dst[SPI_MASTER_BUF_SIZE] __attribute__((section(".dma.data"),aligned(4)));


/**
@brief   Judge the bus status and wait for the bus idle status
@return 	 0---spi bus status not busy.
*/
int spi_master_bus_ready()
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int spi_status;
	do {
		spi_status = p_spi_reg->status;
	} while(spi_status & SPI_STATUS_BUSY);

	return 0;
}

/**
@brief   Before the spi works, clear the data in tx\rx FIFO
@details	   Reset spi fifo and check spi fifo status within 1000 cycles.
@return	   0--spi fifo reset completes, 1--spi fifo reset not completes.
*/
int spi_clear_fifo()
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int i;
	p_spi_reg->ctrl = (p_spi_reg->ctrl) |(SPI_CONTROL_RXFIFORST|SPI_CONTROL_TXFIFORST);

	for(i = 0; i < 1000; i++)
	{	
		if((p_spi_reg->ctrl) & (SPI_CONTROL_RXFIFORST|SPI_CONTROL_TXFIFORST))
		{
			return 0;
		}
	}
	return 1;
}

/**
@brief   Number of data existing in rx fifo
@return	  The number of data existing in rx fifo.
*/
unsigned int spi_data_num_in_rx_fifo(void)
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	return ((p_spi_reg->status) >> 8) & 0x1F;
}

/**
@brief   tx dma isr function
*/
void drv_spi_tx_dma_isr(void *data)
{
//	os_printf(LM_OS, LL_INFO, "spi_dma_tx_trans_end\r\n");
	os_sem_post(spi_tx_process);
}

/**
@brief   rx dma isr function
*/
void drv_spi_rx_dma_isr(void *data)
{
	os_sem_post(spi_rx_process);
//	os_printf(LM_OS, LL_INFO, "spi_dma_rx_trans_end\r\n");
}

/**
@brief  Configuration required for spi initialization.
@param[in]  spi_interface_config_t pointer structure.
@return	   -2--spi rx_dma mode allocate channel failure, 
		   -1--spi tx_dma mode allocate channel failure,
		    0--spi init config success.
*/
int spi_init_cfg(spi_interface_config_t *spi_master_dev)
{

	spi_master =  *spi_master_dev;
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int value;

	/*enable spi1 clock*/
	chip_clk_enable(CLK_SPI1_APB);
	chip_clk_enable(CLK_SPI1_SEL);
	WRITE_REG(MEM_BASE_SMU_PD + 0x18, SPI1_WORK_CLK_AHB_CLK);	// spi1 work clk choose ahb clk

	PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_SPI1_CLK);
	PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_SPI1_CS0);
	PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_SPI1_MOSI);
	PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_SPI1_MIS0);


	spi_tx_process = os_sem_create(1, 0);
	spi_rx_process = os_sem_create(1, 0);
	
	/* dma enable*/
	if(spi_master_dev->spi_dma_enable == 1)
	{
		p_spi_dev_dma = os_zalloc(sizeof(spi_buff_dev_dma));	
		p_spi_dev_dma->buffer_tx_length = SPI_TX_BUF_SIZE;
		p_spi_dev_dma->buffer_rx_length = SPI_RX_BUF_SIZE;
		p_spi_dev_dma->spi_tx_buf_head = 0;
		p_spi_dev_dma->spi_tx_buf_tail = 0;
		p_spi_dev_dma->spi_rx_buf_head = 0;
		p_spi_dev_dma->spi_rx_buf_tail = 0;
		p_spi_dev_dma->regbase = MEM_BASE_SPI1;
		/*enable DMA and set TXTHRES*/
		value = p_spi_reg->ctrl | BIT(3)|BIT(4) | BIT(16);
		p_spi_reg->ctrl = value;
		/*enable datamerge*/
		value = p_spi_reg ->transFmt |BIT(7);
		p_spi_reg ->transFmt = value ;
		
		p_spi_dev_dma->spi_tx_dma_ch = drv_dma_ch_alloc();
		if (p_spi_dev_dma->spi_tx_dma_ch < 0)
		{
			return SPI_TX_RET_ENODMA;
		}
		drv_dma_isr_register(p_spi_dev_dma->spi_tx_dma_ch, drv_spi_tx_dma_isr, NULL);
		
		p_spi_dev_dma->spi_rx_dma_ch = drv_dma_ch_alloc();
		if (p_spi_dev_dma->spi_rx_dma_ch < 0)
		{
			return SPI_RX_RET_ENODMA;
		}

		drv_dma_isr_register(p_spi_dev_dma->spi_rx_dma_ch, drv_spi_rx_dma_isr, NULL);
	}	
	else
	{
		p_spi_dev =  os_zalloc(sizeof(spi_buff_dev));
		memset(p_spi_dev->spi_tx_buf , 0 ,SPI_TX_BUF_SIZE);
		memset(p_spi_dev->spi_rx_buf , 0 ,SPI_RX_BUF_SIZE);
		p_spi_dev->spi_tx_buf_head = 0;
		p_spi_dev->spi_tx_buf_tail = 0;
		p_spi_dev->spi_rx_buf_head = 0;
		p_spi_dev->spi_rx_buf_tail = 0;
		p_spi_dev_dma->regbase = MEM_BASE_SPI1;
	}
	
	/* cfg spi master mode*/ 
	
	value = p_spi_reg ->transFmt & 0xFFFFFFFB;

	/*cfg spi plo and pha*/
	if(spi_master_dev->spi_clk_pol==0)
	{
		value &=(~0x2);
	}
	else
	{
		value|=0x2;
	}

	if(spi_master_dev->spi_clk_pha==0)
	{
		value &= (~0x1);
	}
	else
	{
		value|=0x1;
	}
	
	/* cfg spi trans mode */
	//bi-direction signal
	if(spi_master_dev->spi_trans_mode==SPI_MODE_BI_DIRECTION_MISO)
	{
		value |=0x10;
	}
	else
	{	
		value &=(~0x10);
		if(spi_master_dev->spi_trans_mode == SPI_MODE_DUAL)
		{
			spi_master.cmd_write |= SPI_TRANSCTRL_DUALQUAD_DUAL;
			spi_master.cmd_read |= SPI_TRANSCTRL_DUALQUAD_DUAL;
		}
		else if(spi_master_dev->spi_trans_mode == SPI_MODE_QUAD)
		{
			spi_master.cmd_write |= SPI_TRANSCTRL_DUALQUAD_QUAD;
			spi_master.cmd_read |= SPI_TRANSCTRL_DUALQUAD_QUAD;
		}
	}
	
	/*addr length*/
	value|=((spi_master_dev->addr_len-1) << 16);
	/*data length*/
	value|=((spi_master_dev->data_len-1) << 8);
	p_spi_reg ->transFmt = value;

	/*address bit enable*/
	if(spi_master_dev->addr_pha_enable == 1)
	{
		if((spi_master_dev->spi_trans_mode == SPI_MODE_DUAL)|(spi_master_dev->spi_trans_mode == SPI_MODE_QUAD))
		{
			//value = p_spi_reg ->transCtrl |BIT(28) |BIT(29);	
			spi_master.addr_pha_enable = p_spi_reg ->transCtrl |BIT(28) |BIT(29);	
		}
		else
		{
			//value = p_spi_reg ->transCtrl |BIT(29);
			spi_master.addr_pha_enable = p_spi_reg ->transCtrl |BIT(29);	
		}

		//p_spi_reg ->transCtrl = value;
	}
	else
	{
		spi_master.addr_pha_enable = p_spi_reg ->transCtrl ;
	}

	/* cfg spi clk */	
	value =p_spi_reg ->timing & 0xFFFFFF00;
	p_spi_reg ->timing = value |(spi_master_dev->master_clk & 0xFF);

	return SPI_INIT_SUCCESS;
}

/**
@brief  SPI host transfer function.
@param[in]  *bufptr:Transmitted data
@param[in]  trans_desc :Transmission parameter pointer structure,It contains the length of the data to be transmitted, address and command.
@return	   -2--clear fifo is not comoletion, 
		   -1-- transmittend data length is not between 0 and 512,
		    0--spi transfer process comoletion.
*/
int spi_master_write(unsigned char *bufptr,spi_transaction_t* trans_desc )
{
	if((trans_desc->length) >512 || (trans_desc->length) <0 )
	{
		os_printf(LM_OS, LL_INFO, "Tansfer length Error\n");
		return -1;
	}
	unsigned long flags = system_irq_save();	
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int spi_dma_enable = spi_master.spi_dma_enable;
	unsigned int left,data;
	unsigned int i = 0;
	if(spi_dma_enable == 0)
	{
		memcpy(p_spi_dev->spi_tx_buf, bufptr, trans_desc->length);
		spi_master_bus_ready();
		//clear rx & tx fifo
		if(spi_clear_fifo()) 
		{
			system_irq_restore(flags);
			return -2;
		}
		if(spi_master.addr_pha_enable == 1)
		{
			p_spi_reg->addr = trans_desc->addr;
		}
		p_spi_reg->transCtrl = spi_master.addr_pha_enable | spi_master.dummy_bit | spi_master.cmd_write | SPI_TRANSCTRL_WCNT((trans_desc->length)-1);
		p_spi_reg->cmd =  trans_desc->cmmand;

		while((trans_desc->length)> 0)
		{
			data = 0;	
			left = min((trans_desc->length), 4);
			for (i = 0; i < left; i++) 
			{
				data |= p_spi_dev->spi_tx_buf[p_spi_dev->spi_tx_buf_head] << (i * 8);	
				p_spi_dev->spi_tx_buf_head++;
			}
			SPI_WAIT_TX_READY(p_spi_reg->status); 	
			p_spi_reg->data = data;
			trans_desc->length -=left;	
		}
		p_spi_dev->spi_tx_buf_head = 0;
		system_irq_restore(flags);
	}
	else
	{
		if(spi_master.addr_pha_enable == 1)
		{
			p_spi_reg->addr = trans_desc->addr;
		}
		memcpy(dma_spi_src, bufptr, (trans_desc->length));
		T_DMA_CFG_INFO dma_cfg_info;
		dma_cfg_info.dst = (unsigned int)&(p_spi_reg->data);
		dma_cfg_info.src = (unsigned int)dma_spi_src;
		dma_cfg_info.len = (trans_desc->length);
		dma_cfg_info.mode = DMA_CHN_SPI_TX;
		drv_dma_cfg(p_spi_dev_dma->spi_tx_dma_ch, &dma_cfg_info);
		p_spi_reg->transCtrl = spi_master.addr_pha_enable |spi_master.dummy_bit | spi_master.cmd_write | SPI_TRANSCTRL_WCNT((trans_desc->length)-1);
		p_spi_reg->cmd = trans_desc->cmmand;
		drv_dma_start(p_spi_dev_dma->spi_tx_dma_ch);
		system_irq_restore(flags);
		os_sem_wait(spi_tx_process, SPI_WAIT_FOREVER);
	} 
	system_irq_restore(flags);
	return 0;
}

/**
@brief  SPI host receive function.
@param[in]  *bufptr:Transmitted data
@param[in]  trans_desc :receive parameter pointer structure,It contains the length of the data to be receive, address and command.
@return	    -1-- receive data length is not between 0 and 512 ,
		    0--spi transfer process comoletion.
*/
int spi_master_read_buffer( spi_transaction_t* trans_desc )
{
	if((trans_desc->length) >512 || (trans_desc->length) <0 )
	{
		os_printf(LM_OS, LL_INFO, "Receive length Error\n");
		return -1;
	}
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int spi_dma_enable = spi_master.spi_dma_enable;
	spi_master_bus_ready();
	//clear rx & tx fifo
	if(spi_clear_fifo()) 
		return -2;
	unsigned long flags = system_irq_save();
	if(spi_dma_enable == 0)
	{
		if(spi_master.addr_pha_enable == 1)
		{
			p_spi_reg->addr = trans_desc->addr;
		}
		unsigned int * rxBuf = (unsigned int *)(p_spi_dev->spi_rx_buf+p_spi_dev->spi_rx_buf_head);
		unsigned int rx_num ,i;
		p_spi_reg->transCtrl = spi_master.addr_pha_enable |spi_master.dummy_bit | spi_master.cmd_read | SPI_TRANSCTRL_RCNT((trans_desc->length) - 1);
		p_spi_reg->cmd = trans_desc->cmmand;
		while(((p_spi_reg->status)& SPI_STATUS_BUSY) == 1)
		{
			rx_num = spi_data_num_in_rx_fifo();
			for(i=0; i<rx_num; i++)
			{
				*rxBuf++ = p_spi_reg->data;
			}
		}
		
		rx_num = spi_data_num_in_rx_fifo();
		for(i=0; i<rx_num; i++)
		{
			*rxBuf++ = p_spi_reg->data;	
		}
		system_irq_restore(flags);
	}
	else
	{
		if(spi_master.addr_pha_enable == 1)
		{
			p_spi_reg->addr = trans_desc->addr;
		}
		T_DMA_CFG_INFO dma_cfg_info;
		dma_cfg_info.dst = (unsigned int)dma_spi_dst;
		dma_cfg_info.src = (unsigned int)&p_spi_reg->data;
		dma_cfg_info.len = (trans_desc->length);
		dma_cfg_info.mode = DMA_CHN_SPI_RX;	

		spi_clear_fifo();
		drv_dma_cfg(p_spi_dev_dma->spi_rx_dma_ch, &dma_cfg_info);
		drv_dma_start(p_spi_dev_dma->spi_rx_dma_ch);
		p_spi_reg->transCtrl =spi_master.addr_pha_enable | spi_master.dummy_bit | spi_master.cmd_read |SPI_TRANSCTRL_RCNT((trans_desc->length) - 1);
		p_spi_reg->cmd = trans_desc->cmmand;
		system_irq_restore(flags);
		os_sem_wait(spi_rx_process, SPI_WAIT_FOREVER);
	}
	return 0;
}

int spi_master_read(unsigned char *bufptr, spi_transaction_t* trans_desc )
{
	int ret;
	if(spi_master.spi_dma_enable == 1)
	{
		ret = spi_master_read_buffer(trans_desc);
		memcpy(bufptr,dma_spi_dst,trans_desc->length);
	}
	else
	{
		ret = spi_master_read_buffer(trans_desc);
		memcpy(bufptr,p_spi_dev->spi_rx_buf,trans_desc->length);
	}
	return ret;
}

