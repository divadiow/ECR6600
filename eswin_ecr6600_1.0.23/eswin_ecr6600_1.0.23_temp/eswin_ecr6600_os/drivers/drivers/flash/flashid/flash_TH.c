/**
* @file       flash_PUYA.c
* @brief      flash driver code  
* @author     LvHuan
* @date       2021-2-19
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include <string.h>
#include <stdio.h>

#include "flash.h"
#include "arch_irq.h"
#include "oshal.h"
#include "cli.h"
#include "flash_internal.h"


#ifdef CONFIG_FLASH_TH



/* TH */
#define TH25Q16HB			0x1560EB


static const T_SPI_FLASH_PARAM th_flash_params[] = 
{

	{
		.flash_id = TH25Q16HB,
		.flash_size = FLASH_SIZE_2_MB,
		.flash_name = "TH25Q16HB",
		.flash_otp_star_addr  = FLASH_OTP_START_ADDR_0x0,
		.flash_otp_block_size = FLASH_OTP_BLOCK_SIZE_256_BYTE,
		.flash_otp_block_num  = FLASH_OTP_BLOCK_NUM_4,
		.flash_otp_block_interval = FLASH_OTP_BLOCK_INTERNAL_256_BYTE,
	},

};



static int spiFlash_TH_check_addr(unsigned int flash_id, int addr, int length)
{
	if(flash_id == TH25Q16HB)
	{
		if(!((addr>=0x000000 && addr<=0x0000FF && ((addr + length -1) <= 0x0000FF))
		   ||(addr>=0x000100 && addr<=0x0001FF && ((addr + length -1) <= 0x0001FF))
		   ||(addr>=0x000200 && addr<=0x0002FF && ((addr + length -1) <= 0x0002FF))
		   ||(addr>=0x000300 && addr<=0x0003FF && ((addr + length -1) <= 0x0003FF))))
		{
			//cli_printf("addr or length error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		//cli_printf("flashID error:0x%x\n",flash_id);
		return FLASH_RET_ID_ERROR;
	}
	
	return FLASH_RET_SUCCESS;
}



/**    @brief		Read data in flash OTP area api.
 *	   @details 	Read the flash OTP data of the specified length from the specified address in the flash memory into the buff.
 *     @param[in]	addr--The address where you want to read data in flash OTP area.
 *     @param[in]	* buf--Data of specified length read from specified address in flash OTP memory.
 *     @param[in]	len--The specified length read from the specified address in flash OTP memory. 
 *     @return      0--ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds, -7--flash id is not GD series.
 */
int spiFlash_TH_OTP_Read_CMD(struct _T_SPI_FLASH_DEV * p_flash_dev, int addr, int length,unsigned char *pdata)
{
	unsigned int *p_dst_buffer = (unsigned int *)pdata;

	spiFlash_format_addr(SPIFLASH_CMD_OTP_RD, addr, 
						SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_ADDR_EN 
						 |SPI_TRANSCTRL_TRAMODE_DR  |SPI_TRANSCTRL_DUMMY_CNT_1
						 |SPI_TRANSCTRL_DUALQUAD_REGULAR |SPI_TRANSCTRL_RCNT(length-1));
	
	unsigned int Rxcount = 0;
	unsigned int i;
	unsigned int data = FLASH_SPI_REG->status;
	while( data & SPI_STATUS_BUSY )
	{           
		if (!(data & SPI_STATUS_RXEMPTY))
	  	{
	        Rxcount = ((data & 0x00001f00)>>8) ;
	        for (i = 0; i < Rxcount; i++) 
			{
				*p_dst_buffer++ = FLASH_SPI_REG->data;
	        }
	    }
		data = FLASH_SPI_REG->status;
	}

	Rxcount = ((data & 0x00001f00)>>8) ;
	for (i = 0; i < Rxcount; i++) 
	{
		*p_dst_buffer++ = FLASH_SPI_REG->data;
	}

	return FLASH_RET_SUCCESS;
}




int spiFlash_TH_OTP_Read(struct _T_SPI_FLASH_DEV * p_flash_dev, int addr, int length,unsigned char *pdata)
{
	unsigned int address, leng;
	unsigned int flash_id;

	flash_id = p_flash_dev->flash_param.flash_id;

	if (spiFlash_TH_check_addr(flash_id, addr, length))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	if ((length <= 0) ||(!pdata))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	leng = MIN(FLASH_SIZE_READ_MAX, length);
	address = addr;	
	
	do
	{
		unsigned int irq_flag = system_irq_save();
		
		if (spiFlash_TH_OTP_Read_CMD(p_flash_dev, address, leng, pdata+(address - addr)) != FLASH_RET_SUCCESS)
		{
			system_irq_restore(irq_flag);
			return FLASH_RET_READ_ERROR;
		}

		system_irq_restore(irq_flag);
		length -= leng;
		address += leng;
		leng = MIN(FLASH_SIZE_READ_MAX, length);
	}while(leng);

	return FLASH_RET_SUCCESS;	
}



/**    @brief		lock flash OTP area data.
 *	   @details 	The Security Registers Lock Bit can be used to OTP protect the security registers. Once the LB bit is set to 1, the Security Registers will be permanently locked; the Erase and Write command will be ignored.
 *     @param[in]	LB--0x01: lock Security Register 0x000000-0x0003FF, LB--0x02: lock Security Register 0x001000-0x0013FF, LB--0x0F: lock all, default:do nothing.
 *     @return      0--ok, -4--The input parameter addr or length is out of bounds, -7--flash id is not GD series.
 */
int spiFlash_TH_OTP_Lock(struct _T_SPI_FLASH_DEV *  p_flash_dev, int LB)
{
	unsigned int irq_flag, data;
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	irq_flag = system_irq_save();
	data = spiFlash_cmd_rdSR_2();
	system_irq_restore(irq_flag);

	data = (data & 0xFF)<<8;

	if(flash_id == TH25Q16HB)//P25Q80HD
	{		
		switch (LB)
		{
			case 0x01:
				data |= 0x0400;
				break;
			default:
				//cli_printf("parameter error\n");
				return FLASH_RET_PARAMETER_ERROR;
		}	
		irq_flag = system_irq_save();
		spiFlash_cmd_wrSR_1(data);
		system_irq_restore(irq_flag);
	}
	else
	{
		//cli_printf("Flash_ID error:0x%x\n",flash_id);
		return FLASH_RET_ID_ERROR;
	}
	
	return FLASH_RET_SUCCESS;
}


/**    @brief		flash OTP sector erase.
 *	   @details 	Send sector erase CMD, secrot erase one block(0x400) once time.
 *     @param[in]	add--Start erasing address into the flash OTP memory.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds,-5--write enable not set,-6--the memory is busy in erase, not erase completely,-7--flash id is not GD series..
 */
int spiFlash_TH_OTP_Se(struct _T_SPI_FLASH_DEV *  p_flash_dev, unsigned int addr)
{
	unsigned int j, data,irq_flag;
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	if(flash_id == TH25Q16HB)
	{
		if(!(addr>=0x000000 && addr<=0x0003FF))
		{
			//cli_printf("addr error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		//cli_printf("flashID error:0x%x\n",flash_id);
		return FLASH_RET_ID_ERROR;
	}

	irq_flag = system_irq_save();
	for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		spiFlash_cmd_wrEnable();
		data = spiFlash_cmd_rdSR_1();
		if(data & SPIFLASH_STATUS_WEL)
		{
			break;
		}
	}

	if((data & SPIFLASH_STATUS_WEL) == 0)
	{
		system_irq_restore(irq_flag);
		return FLASH_RET_WREN_NOT_SET;
	}

	spiFlash_format_addr(SPIFLASH_CMD_OTP_SE, addr, SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_NONE);

	for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		data = spiFlash_cmd_rdSR_1();
		if((data & SPIFLASH_STATUS_WIP) == 0)
		{
			break;
		}
	}

	system_irq_restore(irq_flag);
		
	if(data & SPIFLASH_STATUS_WIP)
	{
		return FLASH_RET_WIP_NOT_SET;
	}

	return FLASH_RET_SUCCESS;
}


/**    @brief		Write data in OTP area of flash item.
 *	   @details 	Write the specified length of data to the specified address into the flash OTP memory.
 *     @param[in]	* pSpiReg--Structure of SPI register.
 *     @param[in]	addr--The address where you want to write data to flash OTP memory.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash OTP memory.
 *     @param[in]	length--The length of data to be written to the specified address of flash OTP memory.
 *     @return      0--ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds, -7--flash id is not GD series.
 */
static int spiFlash_TH_OTP_Program(struct _T_SPI_FLASH_DEV * p_flash_dev, unsigned int addr, unsigned char * buf, unsigned int length)
{
	unsigned int i, left, data;
	
	for (i = 0; i<SPIFLASH_TIMEOUT_COUNT; i++)
	{
		spiFlash_cmd_wrEnable();
		data = spiFlash_cmd_rdSR_1();
		if(data & SPIFLASH_STATUS_WEL)
		{
			break;
		}
	}

	if ((data & SPIFLASH_STATUS_WEL) == 0)
	{
		return FLASH_RET_WREN_NOT_SET;
	}

	spiFlash_format_addr(SPIFLASH_CMD_OTP_PP, addr, 
		SPI_TRANSCTRL_CMD_EN|SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_WO|SPI_TRANSCTRL_DUALQUAD_REGULAR|SPI_TRANSCTRL_WCNT(length - 1));

	if ((unsigned int)buf%4==0)
	{
		unsigned int *pdata = (unsigned int * )buf;
		
		left = (length + 3)/4;

		for (i = 0; i< left; i++)
		{
			SPI_WAIT_TX_READY(FLASH_SPI_REG->status);
			FLASH_SPI_REG->addr = addr + i*4; 
			FLASH_SPI_REG->data = *pdata++;
		}
	}
	else
	{
		while (length > 0) 
		{
			data = 0;					/* clear the data */			
			left = MIN(length, 4);		/* data need be read 32bits once a time */

			for (i = 0; i < left; i++) 
			{
				data |= *buf++ << (i * 8);
				length--;
			}

			SPI_WAIT_TX_READY(FLASH_SPI_REG->status);
			FLASH_SPI_REG->addr = addr + i*4; 
			FLASH_SPI_REG->data = data;
		}
	}
	return FLASH_RET_SUCCESS;
}


/**    @brief		Write data in OTP area of flash item.
 *	   @details 	Write the specified length of data to the specified address into the flash OTP memory.
 *     @param[in]	addr--The address where you want to write data to flash OTP area.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash OTP area.
 *     @param[in]	length--The length of data to be written to the specified address of flash OTP area.
 *     @return      0--write ok, -5--write enable not set,-6--the memory is busy in write, not write completely,-8--otp program error.
 */
int spiFlash_TH_OTP_Write(struct _T_SPI_FLASH_DEV *  p_flash_dev, unsigned int addr, unsigned int len, unsigned char * buf)
{
	unsigned int address, length;
	
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	if (spiFlash_TH_check_addr(flash_id, addr, len))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	if ((len <= 0) ||(!buf))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	length = addr % SPIFLASH_PAGE_SIZE;
	length = MIN(SPIFLASH_PAGE_SIZE - length, len);

	address = addr;	
	
	do
	{
		unsigned int j, data;
		unsigned int irq_flag = system_irq_save();

		if (spiFlash_TH_OTP_Program(p_flash_dev, address, buf+(address - addr), length) != FLASH_RET_SUCCESS)
		{
			system_irq_restore(irq_flag);
			return FLASH_RET_PROGRAM_ERROR;
		}

		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			data = spiFlash_cmd_rdSR_1();
			if((data & SPIFLASH_STATUS_WIP) == 0)
			{
				break;
			}
		}

		system_irq_restore(irq_flag);
		if(data & SPIFLASH_STATUS_WIP)
		{
			return FLASH_RET_WIP_NOT_SET;
		}

		len -= length;
		address += length;
		length = MIN(SPIFLASH_PAGE_SIZE, len);
	}while(length);

	return FLASH_RET_SUCCESS;	
}





int spiFlash_TH_probe(T_SPI_FLASH_DEV *p_flash_dev)
{
	int i;
	const T_SPI_FLASH_PARAM * param;
	T_SPI_FLASH_OTP_OPS * ops;

	for (i=0; i<FLASH_ARRAY_SIZE(th_flash_params); i++)
	{
		param = &th_flash_params[i];
		if (p_flash_dev->flash_param.flash_id == param->flash_id)
		{
			p_flash_dev->flash_param.flash_size = param->flash_size;
			p_flash_dev->flash_param.flash_name = param->flash_name;
			p_flash_dev->flash_param.flash_otp_star_addr  = param->flash_otp_star_addr;
			p_flash_dev->flash_param.flash_otp_block_size = param->flash_otp_block_size;
			p_flash_dev->flash_param.flash_otp_block_num  = param->flash_otp_block_num;
			p_flash_dev->flash_param.flash_otp_block_interval = param->flash_otp_block_interval;
			break;
		}
	}

	if (i == FLASH_ARRAY_SIZE(th_flash_params))
	{
		return FLASH_RET_ID_ERROR;
	}

	ops = &p_flash_dev->flash_otp_ops;
	ops->flash_otp_read = spiFlash_TH_OTP_Read;
	ops->flash_otp_write = spiFlash_TH_OTP_Write;
	ops->flash_otp_erase= spiFlash_TH_OTP_Se;
	ops->flash_otp_lock = spiFlash_TH_OTP_Lock;
	
	return FLASH_RET_SUCCESS;
}


#endif//end CONFIG_ID_P25Q16SH


