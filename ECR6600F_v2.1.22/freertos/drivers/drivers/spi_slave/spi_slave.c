/**
* @file       spi_slave.c
* @brief      spi_slave driver code  
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


static spi_slave_interface_config_t spi_slave;
static spi_slave_buff_dev * p_spi_slave_dev;
unsigned int spi_slave_dma_tx_ch = 0;
unsigned int spi_slave_dma_rx_ch = 0;

#define SPI_FIFO_DEPTH 64
#define TEST_MODE_EXTPAD_RST_EN			0x201214 
#define SPI1_WORK_CLK_AHB_CLK 0x88
#define SPI_WAIT_FOREVER 0xffffffff
#define min(x, y)               (((x) < (y))? (x) : (y))	

unsigned int spi_slave_remain_len=0;

unsigned char *dma_spi_slave_src=(unsigned char *)0xA2000;
unsigned int *dma_spi_slave_dst=(unsigned int *)0xA3000;

os_sem_handle_t spi_slave_tx_process;
extern os_sem_handle_t spi_slave_rx_process;


uint32_t spi_rx_buffer_wr = 0;
uint32_t spi_rx_buffer_rd = 0;
uint32_t spi_tx_buffer_wr = 0;
uint32_t spi_tx_buffer_rd = 0;

unsigned int spi_length = 0;
unsigned int new_len = 0;
unsigned int remain_read = 0;
unsigned int real_len = 0;

/**
@brief   tx dma isr function
*/
void drv_spi_slave_tx_dma_isr(void *data)
{
	spi_tx_buffer_rd += real_len;
	if(spi_tx_buffer_rd == SPI_SLAVE_TX_BUF_SIZE)
		spi_tx_buffer_rd = 0;
}

/**
@brief   rx dma isr function
*/
void drv_spi_slave_rx_dma_isr(void *data)
{
	os_sem_post(spi_slave_rx_process);
}

/**
@brief   spi slave rx dma isr function
@param[in]  length:dma transport length
*/
void hal_spi_dma_rx_config(unsigned int length)
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	p_spi_reg ->intrEn = spi_slave.inten ;
	T_DMA_CFG_INFO dma_cfg_info;
	dma_cfg_info.dst = (unsigned int)(dma_spi_slave_dst + spi_rx_buffer_wr);
	dma_cfg_info.src = (unsigned int)&p_spi_reg->data;
	dma_cfg_info.len = length;
	dma_cfg_info.mode = DMA_CHN_SPI_RX; 
	drv_dma_cfg(spi_slave_dma_rx_ch, &dma_cfg_info);
	drv_dma_start(spi_slave_dma_rx_ch);
}

/**
@brief  Configuration required for spi initialization
*/
int spi_init_slave_cfg(spi_slave_interface_config_t *spi_slave_dev)
{
	spi_slave =  *spi_slave_dev;
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;

	spi_slave_tx_process = os_sem_create(1, 0);
	spi_slave_rx_process = os_sem_create(1, 0);
	
	unsigned int value = 0;

	p_spi_reg->ctrl = (p_spi_reg->ctrl) |(SPI_CONTROL_RXFIFORST|SPI_CONTROL_TXFIFORST);

	WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
	os_usdelay(10);
	chip_clk_enable(CLK_SPI1_SEL);
	WRITE_REG(MEM_BASE_SMU_PD + 0x18, SPI1_WORK_CLK_AHB_CLK);	// spi1 work clk choose ahb clk

	p_spi_reg->intrSt = 0xFFFFFFFF;
	/*enable spi1 clock*/
	chip_clk_enable(CLK_SPI1_APB);
	PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_SPI1_CLK);
	PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_SPI1_CS0);
	PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_SPI1_MOSI);
	PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_SPI1_MIS0);

	p_spi_slave_dev = os_zalloc(sizeof(spi_slave_buff_dev));
	p_spi_slave_dev->rx_num = 0;
	p_spi_slave_dev->tx_num = 0;
	memset(p_spi_slave_dev->spi_rx_slave_buf,0,SPI_SLAVE_RX_BUF_SIZE * 4);
	memset(p_spi_slave_dev->spi_tx_slave_buf,0,SPI_SLAVE_TX_BUF_SIZE);

	/*cfg spi plo and pha*/
	if(spi_slave_dev->spi_clk_pol==0)
	{
		value &=(~0x2);
	}
	else
	{
		value|=0x2;
	}

	if(spi_slave_dev->spi_clk_pha==0)
	{
		value &= (~0x1);
	}
	else
	{
		value|=0x1;
	}
	value |=  BIT(2) |BIT(7) |((spi_slave_dev->addr_len-1) << 16) |((spi_slave_dev->data_len-1) << 8);
	p_spi_reg->transFmt = value;
	/* cfg spi clk */
	value = p_spi_reg->timing & 0xFFFFFF00;
	p_spi_reg->timing = value |(spi_slave_dev->slave_clk & 0xFF);

	p_spi_reg->ctrl |=  0x70000;

	p_spi_reg ->intrEn = spi_slave_dev ->inten ;
	arch_irq_register(VECTOR_NUM_SPI2, spi_slave_load);
	arch_irq_clean(VECTOR_NUM_SPI2);
	arch_irq_unmask(VECTOR_NUM_SPI2);

	if(spi_slave_dev->spi_slave_dma_enable == 1)
	{
		value = p_spi_reg->ctrl | BIT(3)|BIT(4) | BIT(16);
		p_spi_reg->ctrl = value;
		/*enable datamerge*/
		value = p_spi_reg ->transFmt |BIT(7);
		p_spi_reg ->transFmt = value ;
		
		spi_slave_dma_tx_ch  = drv_dma_ch_alloc();
		if (spi_slave_dma_tx_ch  < 0)
		{
			return SPI_TX_RET_ENODMA;
		}
		drv_dma_isr_register(spi_slave_dma_tx_ch , drv_spi_slave_tx_dma_isr, NULL);
		
		spi_slave_dma_rx_ch  = drv_dma_ch_alloc();
		if (spi_slave_dma_rx_ch < 0)
		{
			return SPI_RX_RET_ENODMA;
		}
		drv_dma_isr_register(spi_slave_dma_rx_ch, drv_spi_slave_rx_dma_isr, NULL);
	}
	
	return SPI_INIT_SUCCESS;
}


/**
@brief  Available buffer space ,
@return	  avil_len--available buffer space.
*/
unsigned int hal_spi_slave_rxbuff_avail_len()
{
	unsigned int avil_len_remain0, avil_len_remain1, avil_len ;
	if((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == 0))
	{
		avil_len_remain0 = SPI_SLAVE_RX_BUF_SIZE * 4  ;
		avil_len_remain1 = 0;
	}
	else if ((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == SPI_SLAVE_RX_BUF_SIZE *4))
	{
		avil_len_remain0 = 0 ;
		avil_len_remain1 = 0;
	}
	if(spi_rx_buffer_wr >= spi_rx_buffer_rd)
	{
		avil_len_remain0 =	SPI_SLAVE_RX_BUF_SIZE * 4 - spi_rx_buffer_wr;
		avil_len_remain1 =	spi_rx_buffer_rd;
	}
	else
	{
		avil_len_remain0 = spi_rx_buffer_wr - spi_rx_buffer_rd;
		avil_len_remain1 = 0;
	}
	
	avil_len = avil_len_remain0 +avil_len_remain1 ;

	return avil_len ;
}

/**
	@brief  Receive data from FIFO and store it in buffer,
*/
void spi_slave_read_to_buffer()
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int rx_num ;
	while(((p_spi_reg->status)& SPI_STATUS_BUSY) == 1)
	{
		rx_num = ((p_spi_reg->status) >> 8) & 0x1F;
		if(rx_num > 0)
		{
			dma_spi_slave_dst[spi_rx_buffer_wr] = p_spi_reg->data;
		 	spi_rx_buffer_wr += 1;
			p_spi_slave_dev->rx_num += 4;
			if((spi_rx_buffer_wr == spi_rx_buffer_rd)&&(p_spi_slave_dev->rx_num ==SPI_SLAVE_RX_BUF_SIZE ))
			{
				os_printf(LM_OS, LL_INFO, "buffer full\r\n");
			}
		}
		spi_rx_buffer_wr = spi_rx_buffer_wr % SPI_SLAVE_RX_BUF_SIZE ;

	}

	rx_num = ((p_spi_reg->status) >> 8) & 0x1F;
	if(rx_num > 0)
	{
		dma_spi_slave_dst[spi_rx_buffer_wr] = p_spi_reg->data;		
	 	spi_rx_buffer_wr += 1;
		p_spi_slave_dev->rx_num += 4;
		if((spi_rx_buffer_wr == spi_rx_buffer_rd)&&(p_spi_slave_dev->rx_num ==SPI_SLAVE_RX_BUF_SIZE))
		{
			os_printf(LM_OS, LL_INFO, "buffer full\r\n");
		}
	}
	spi_rx_buffer_wr = spi_rx_buffer_wr % SPI_SLAVE_RX_BUF_SIZE ;	
}

/**
	@brief  the length of the data already in the buffer.
	return data_len:the number of rx receive data 
*/
unsigned int hal_spi_slave_get_rx_data_len()
{
	unsigned int data_len;
	if(spi_slave.spi_slave_dma_enable == 0)
	{
		if((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == 0))
		{
			data_len = 0;
		}
		else if ((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == SPI_SLAVE_RX_BUF_SIZE *4))
		{
			data_len = SPI_SLAVE_RX_BUF_SIZE *4;
		}
		if(spi_rx_buffer_wr >= spi_rx_buffer_rd)
		{
			data_len = (spi_rx_buffer_wr - spi_rx_buffer_rd) * 4;
		}
		else
		{
			data_len = (SPI_SLAVE_RX_BUF_SIZE -spi_rx_buffer_rd + spi_rx_buffer_wr ) * 4;
		}	
	}
	else
	{
		if((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == 0))
		{
			data_len = 0;
		}
		else if ((spi_rx_buffer_wr == spi_rx_buffer_rd) &&(p_spi_slave_dev->rx_num == SPI_SLAVE_RX_BUF_SIZE *4))
		{
			data_len = SPI_SLAVE_RX_BUF_SIZE *4;
		}
		if(spi_rx_buffer_wr >= spi_rx_buffer_rd)
		{
			data_len = spi_rx_buffer_wr - spi_rx_buffer_rd;
		}
		else
		{
			data_len = SPI_SLAVE_RX_BUF_SIZE *4 -spi_rx_buffer_rd + spi_rx_buffer_wr ;
		}	
	}
	return data_len ;
}

/**
	@brief  the length of the data already in the buffer.
	return 	avil_len:the number of rx buffer's avilable space
*/
unsigned int hal_spi_slave_get_tx_avil_len()
{
	unsigned int avil_len;
	if((spi_tx_buffer_wr == spi_tx_buffer_rd) &&(p_spi_slave_dev->tx_num == 0))
	{
		avil_len = SPI_SLAVE_TX_BUF_SIZE;
	}
	else if ((spi_tx_buffer_wr == spi_tx_buffer_rd) &&(p_spi_slave_dev->tx_num == SPI_SLAVE_TX_BUF_SIZE))
	{
		avil_len = 0;
	}
	if(spi_tx_buffer_wr >= spi_tx_buffer_rd)
	{
		avil_len = SPI_SLAVE_TX_BUF_SIZE -spi_tx_buffer_wr;
	}
	else
	{
		avil_len = spi_tx_buffer_rd - spi_tx_buffer_wr;
	}
	return avil_len ;
}



void hal_spi_get_rx_data_end(unsigned len)
{
	spi_rx_buffer_rd += len;
	p_spi_slave_dev->rx_num -= len;
	if (spi_rx_buffer_rd == SPI_SLAVE_RX_BUF_SIZE)
		spi_rx_buffer_rd = 0;

}

void spi_slave_rx_dma_trans()
{
	spi_length = hal_spi_slave_rxbuff_avail_len();
	if (spi_length > 0)
	{
		hal_spi_dma_rx_config(spi_length);		
	}	
}


/**
	@brief  Receive data from FIFO and store it in buffer,
	@param[in]  *bufptr: The buffer where the received data is stored,
	@param[in]   len: the length of the data to be receive,
	@return len-- The remaining data length to be accepted. 
*/
unsigned int spi_slave_read(unsigned char *bufptr, unsigned int len)
{
	int get_data_length =hal_spi_slave_get_rx_data_len();
	if(spi_slave.spi_slave_dma_enable == 0)
	{
		if(len >= get_data_length)
		{
			memcpy(bufptr ,(unsigned char*)(dma_spi_slave_dst + spi_rx_buffer_rd), get_data_length);
			hal_spi_get_rx_data_end((get_data_length  + 3)/4);
			len -= get_data_length;
			
		}
		else
		{
			memcpy(bufptr,(unsigned char*)(dma_spi_slave_dst + spi_rx_buffer_rd) ,len);
			hal_spi_get_rx_data_end((len + 3) /4);	
			len = 0;
		}
	}
	else
	{
		if(len >= get_data_length)
		{
			memcpy(bufptr ,(unsigned char*)(dma_spi_slave_dst + spi_rx_buffer_rd), get_data_length);
			hal_spi_get_rx_data_end(get_data_length);
			len -= get_data_length;
			
		}
		else
		{
			memcpy(bufptr,(unsigned char*)(dma_spi_slave_dst + spi_rx_buffer_rd) ,len);
			hal_spi_get_rx_data_end(len);	
			len = 0;
		}
	}

	return len;
}


/**
	@brief  SPI slave transfer function.
	@param[in]  *bufptr: Data to be sent,
	@param[in]   len: the length of the data to be sent,
	@return  -1-- tx_fifo full. 
			  0-- sent compeletion.
*/
 int spi_slave_write(unsigned char *bufptr,unsigned int length )
{
	if(spi_slave.spi_slave_dma_enable == 0)
	{
		unsigned long flags = arch_irq_save();	
		spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
		unsigned int * buff = (unsigned int *)bufptr;
		int i,avil_lenth , length_head_to_end ,value;
		if(length > SPI_FIFO_DEPTH)
		{
			for(i = 0 ;i< (SPI_FIFO_DEPTH /4) ;i++)
			{
				if((p_spi_reg ->status & SPI_STATUS_TXFULL) == 0)
				{
					p_spi_reg ->data = buff[i];
				}
				else
				{
					os_printf(LM_OS, LL_INFO, "spi tx fifo full\r\n");
					return -1;
				}
			}
			spi_slave_remain_len = length - SPI_FIFO_DEPTH;

			if((spi_tx_buffer_wr >= spi_tx_buffer_rd)&&(p_spi_slave_dev->tx_num != SPI_SLAVE_TX_BUF_SIZE))
			{
				avil_lenth = SPI_SLAVE_TX_BUF_SIZE - spi_tx_buffer_wr + spi_tx_buffer_rd;
				if(avil_lenth < spi_slave_remain_len)
				{
					os_printf(LM_OS, LL_INFO, "No Enough Buffer Space \r\n");
					value = p_spi_reg->intrEn | BIT3;
					p_spi_reg->intrEn = value;
					return -1;
				}
				length_head_to_end = SPI_SLAVE_TX_BUF_SIZE - spi_tx_buffer_wr;
				if(spi_slave_remain_len > length_head_to_end)
				{
					memcpy(&p_spi_slave_dev->spi_tx_slave_buf[spi_tx_buffer_wr],&buff[16],length_head_to_end);
					spi_tx_buffer_wr = 0;
					memcpy(&p_spi_slave_dev->spi_tx_slave_buf[spi_tx_buffer_wr],&buff[16 + length_head_to_end/4],spi_slave_remain_len - length_head_to_end);
					spi_tx_buffer_wr = spi_slave_remain_len - length_head_to_end;
				}
				else
				{
					memcpy(&p_spi_slave_dev->spi_tx_slave_buf[spi_tx_buffer_wr],&buff[16],spi_slave_remain_len);
					spi_tx_buffer_wr += spi_slave_remain_len;
				}
			}
			else if(spi_tx_buffer_wr < spi_tx_buffer_rd)
			{
				avil_lenth = spi_tx_buffer_rd - spi_tx_buffer_wr;
				if(avil_lenth < spi_slave_remain_len)
				{
					os_printf(LM_OS, LL_INFO, "No Enough Buffer Space \r\n");
					value = p_spi_reg->intrEn | BIT3;
					p_spi_reg->intrEn = value;
					return -1;
				}
				memcpy(&p_spi_slave_dev->spi_tx_slave_buf[spi_tx_buffer_wr],&buff[16],spi_slave_remain_len);
				spi_tx_buffer_wr += spi_slave_remain_len;
			}
			p_spi_slave_dev->tx_num += spi_slave_remain_len;

			value = p_spi_reg->intrEn | BIT3;
			p_spi_reg->intrEn = value;

			if((spi_tx_buffer_wr == spi_tx_buffer_rd) && (p_spi_slave_dev->tx_num == SPI_SLAVE_TX_BUF_SIZE))
			{
				os_printf(LM_OS, LL_INFO, "No Enough Buffer Space\n");
				value = p_spi_reg->intrEn | BIT3;
				p_spi_reg->intrEn = value;
			}

		}
		else
		{
			int len = (length + 3) /4 ;
			for(i = 0 ;i< len ;i++)
			{
				if((p_spi_reg ->status & SPI_STATUS_TXFULL) == 0)
				{
					p_spi_reg ->data = buff[i];
				}
				else
				{
					os_printf(LM_OS, LL_INFO, "spi tx fifo full\r\n");
					return -1;
				}
			}
		}

		arch_irq_restore(flags);
		
	}
	else
	{
		unsigned int avil_len=hal_spi_slave_get_tx_avil_len();
		if(avil_len >= length)
		{
			memcpy(dma_spi_slave_src + spi_tx_buffer_wr, bufptr, length);
			spi_tx_buffer_wr += length;	
			real_len = length;
			
		}
		else
		{
			memcpy(dma_spi_slave_src + spi_tx_buffer_wr, bufptr, avil_len);
			spi_tx_buffer_wr += avil_len;
			real_len = avil_len;
			os_printf(LM_OS, LL_INFO, "No Enough Buffer Space\n");
		}
		
		if(spi_tx_buffer_wr == SPI_SLAVE_TX_BUF_SIZE)
				spi_tx_buffer_wr = 0;
		spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
		T_DMA_CFG_INFO dma_cfg_info;
		dma_cfg_info.dst = (unsigned int)&(p_spi_reg->data);
		dma_cfg_info.src = (unsigned int)(dma_spi_slave_src + spi_tx_buffer_rd) ;
		dma_cfg_info.len = real_len;
		dma_cfg_info.mode = DMA_CHN_SPI_TX;
		drv_dma_cfg(spi_slave_dma_tx_ch, &dma_cfg_info);
		drv_dma_start(spi_slave_dma_tx_ch);
	}
	return 0;
}

/**
	@brief  Send the data in the Buffer.
*/	
void spi_slave_write_spi()
{
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int data,i,left,value;	
	if((spi_tx_buffer_wr == spi_tx_buffer_rd) && p_spi_slave_dev->tx_num ==0)
	{
		value = p_spi_reg->intrEn & (~BIT3);
		p_spi_reg->intrEn = value;
		return ;
	}
	else
	{
		int length = 0;
		if(spi_tx_buffer_wr>=spi_tx_buffer_rd)
			length = spi_tx_buffer_wr - spi_tx_buffer_rd;
		else
			length = spi_tx_buffer_wr + SPI_SLAVE_TX_BUF_SIZE - spi_tx_buffer_rd;
		while(length > 0)	
		{
			while((p_spi_reg->status & SPI_STATUS_TXFULL));
			data = 0;
			left =min (length, 4);
			for(i=0; i<left; i++)
			{
				data |= p_spi_slave_dev->spi_tx_slave_buf[spi_tx_buffer_rd] << (i * 8);
				spi_tx_buffer_rd++;
				if(spi_tx_buffer_rd == SPI_SLAVE_TX_BUF_SIZE)
					spi_tx_buffer_rd = 0;
			}
			p_spi_reg->data = data;
			p_spi_slave_dev->tx_num -= left;
			if(spi_tx_buffer_wr >= spi_tx_buffer_rd)
				length = spi_tx_buffer_wr - spi_tx_buffer_rd;
			else
				length = spi_tx_buffer_wr + SPI_SLAVE_TX_BUF_SIZE - spi_tx_buffer_rd;
		}
		if(length == 0)
		{
			spi_tx_buffer_wr = 0;
			spi_tx_buffer_rd = 0;
			value = p_spi_reg->intrEn & (~BIT3);
			p_spi_reg->intrEn = value;
		}
	}

}

/**
	@brief  spi slave interrupt function.
*/	
void spi_slave_load(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
	unsigned int temp;
	p_spi_reg->intrSt = SPI_STS_SLAVE_CMD_INT;
	temp = p_spi_reg->cmd  & 0xFF;
	switch(temp)
	{
		case 0x51:
		case 0x52:
		case 0x54:
			if(spi_slave.spi_slave_dma_enable == 0)
			{
				spi_slave_read_to_buffer();
			}
			break;
		case 0x0B:
		case 0x0C:
		case 0x0E:
			if(spi_slave.spi_slave_dma_enable == 0)
			{
				spi_slave_write_spi();
			}
			break;
		case 0x05:
		case 0x15:
		case 0x25:							
			p_spi_reg->data = 0xff;
			break;					
		default:
			os_printf(LM_OS, LL_INFO, "spiSlave: unknow cmd: %d\n", temp);
			break;
	}
	if((((((p_spi_reg->intrSt) & 0x10)>>4) == 1)&&(spi_slave.spi_slave_dma_enable == 1)) ==1)
	{
		remain_read = drv_dma_stop(spi_slave_dma_rx_ch);
		new_len = spi_length - remain_read * 4;
		spi_rx_buffer_wr += new_len;
		p_spi_slave_dev->rx_num += new_len;
		if(spi_rx_buffer_wr == SPI_SLAVE_RX_BUF_SIZE *4)
			spi_rx_buffer_wr = 0;	
		p_spi_reg ->intrEn = 0;
		os_sem_post(spi_slave_rx_process);
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

