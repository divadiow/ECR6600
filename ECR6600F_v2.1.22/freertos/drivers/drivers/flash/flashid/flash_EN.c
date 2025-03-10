/**
* @file       flash_EN.c
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



#ifdef CONFIG_FLASH_EN

/* EN */
#define EN25Q16B			0x15701C
#define EN25QH32B			0x16701C
#define EN25Q32C			0x16301C


#if 1


static const T_SPI_FLASH_PARAM en_flash_params[] = 
{
	{
		.flash_id 	= EN25Q16B,
		.flash_size = FLASH_SIZE_2_MB,
		.flash_name = "EN25Q16B",
		.flash_otp_star_addr  = FLASH_OTP_START_ADDR_0x1FD000,
		.flash_otp_block_size = FLASH_OTP_BLOCK_SIZE_512_BYTE,
		.flash_otp_block_num  = FLASH_OTP_BLOCK_NUM_3,
		.flash_otp_block_interval = FLASH_OTP_BLOCK_INTERNAL_4096_BYTE,
	},

	{
		.flash_id	= EN25QH32B,
		.flash_size = FLASH_SIZE_4_MB,
		.flash_name = "EN25QH32B",
		.flash_otp_star_addr  = FLASH_OTP_START_ADDR_0x3FD000,
		.flash_otp_block_size = FLASH_OTP_BLOCK_SIZE_512_BYTE,
		.flash_otp_block_num  = FLASH_OTP_BLOCK_NUM_3,
		.flash_otp_block_interval = FLASH_OTP_BLOCK_INTERNAL_4096_BYTE,
	},

	{
		.flash_id	= EN25Q32C,
		.flash_size = FLASH_SIZE_4_MB,
		.flash_name = "EN25Q32C",
		.flash_otp_star_addr  = FLASH_OTP_START_ADDR_0x3FF000,
		.flash_otp_block_size = FLASH_OTP_BLOCK_SIZE_512_BYTE,
		.flash_otp_block_num  = FLASH_OTP_BLOCK_NUM_1,
		.flash_otp_block_interval = FLASH_OTP_BLOCK_INTERNAL_4096_BYTE,
	},

};

static int spiFlash_EN_check_addr(unsigned int flash_id, int addr, int length)
{
	if (flash_id == EN25Q16B)
	{
		if(!((addr>=0x1FF000 && addr<=0x1FF1FF && (addr + length -1) <= 0x1FF1FF)
		   ||(addr>=0x1FE000 && addr<=0x1FE1FF && (addr + length -1) <= 0x1FE1FF)
		   ||(addr>=0x1FD000 && addr<=0x1FD1FF && (addr + length -1) <= 0x1FD1FF)))
		{
			//cli_printf("addr or length error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else if (flash_id == EN25QH32B)
	{
		if(!((addr>=0x3FF000 && addr<=0x3FF1FF && (addr + length -1) <= 0x3FF1FF)
		   ||(addr>=0x3FE000 && addr<=0x3FE1FF && (addr + length -1) <= 0x3FE1FF)
		   ||(addr>=0x3FD000 && addr<=0x3FD1FF && (addr + length -1) <= 0x3FD1FF)))
		{
			//cli_printf("addr or length error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else if (flash_id == EN25Q32C)
	{
		if(!(addr>=0x3FF000 && addr<=0x3FF1FF && (addr + length -1) <= 0x3FF1FF))
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
 *     @return      0--ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds, -7--flash id is not EN series.
 */

int spiFlash_EN_OTP_Read(struct _T_SPI_FLASH_DEV * p_flash_dev, int addr, int length,unsigned char *pdata)
{
	unsigned int flash_id;

	flash_id = p_flash_dev->flash_param.flash_id;

	if (spiFlash_EN_check_addr(flash_id, addr, length))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	if ((length <= 0) ||(!pdata))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	
	if (drv_spiflash_read(addr, pdata, length) != FLASH_RET_SUCCESS)
	{
		return FLASH_RET_READ_ERROR;
	}


	return FLASH_RET_SUCCESS;	
}


/**    @brief		lock flash OTP area data.
 *	   @details 	The Security Registers Lock Bit can be used to OTP protect the security registers. Once the LB bit is set to 1, the Security Registers will be permanently locked; the Erase and Write command will be ignored.
 *     @param[in]	LB--0x01: lock Security Register 0x000000-0x0003FF, LB--0x02: lock Security Register 0x001000-0x0013FF, LB--0x0F: lock all, default:do nothing.
 *     @return      0--ok, -4--The input parameter addr or length is out of bounds, -7--flash id is not EN series.
 */
int spiFlash_EN_OTP_Lock(struct _T_SPI_FLASH_DEV *  p_flash_dev, int LB)
{
	unsigned int irq_flag, data;
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	irq_flag = system_irq_save();
	data = spiFlash_cmd_rdSR_1();
	system_irq_restore(irq_flag);

	data = (data & 0xFF)<<8;

	if ((flash_id == EN25Q16B) || (flash_id == EN25QH32B) )
	{		
		switch (LB)
		{
			case 0x01:
				data |= 0x80;
				break;
			case 0x02:
				data |= 0x04;
				break;
			case 0x03:
				data |= 0x02;
				break;
			case 0x0F:
				data |= 0x86;
				break;
			default:
				//cli_printf("parameter error\n");
				return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else if(flash_id == EN25Q32C)
	{
		switch (LB)
		{
			case 0x01:
				data |= 0x80;
				break;
			
			default:
				//cli_printf("parameter error\n");
				return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		//cli_printf("flashID error:0x%x\n",flash_id);
		return FLASH_RET_ID_ERROR;
	}
	

	irq_flag = system_irq_save();
	
	spiFlash_format_none(SPIFLASH_CMD_WREN);
	spiFlash_cmd_otpmode_wrSR_1(data);
	system_irq_restore(irq_flag);
	
	return FLASH_RET_SUCCESS;
}


/**    @brief		flash OTP sector erase.
 *	   @details 	Send sector erase CMD, secrot erase one block(0x400) once time.
 *     @param[in]	add--Start erasing address into the flash OTP memory.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds,-5--write enable not set,-6--the memory is busy in erase, not erase completely,-7--flash id is not EN series..
 */
 
int spiFlash_EN_OTP_Se(struct _T_SPI_FLASH_DEV *  p_flash_dev, unsigned int addr)
{
	unsigned int j, data,irq_flag;
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	if (flash_id == EN25Q16B)
	{
		if(!((addr>=0x1FF000 && addr<=0x1FF1FF)||(addr>=0x1FE000 && addr<=0x1FE1FF)||(addr>=0x1FD000 && addr<=0x1FD1FF )))
		{
			//cli_printf("addr error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else if (flash_id == EN25QH32B)
	{
		if(!((addr>=0x3FF000 && addr<=0x3FF1FF)||(addr>=0x3FE000 && addr<=0x3FE1FF)||(addr>=0x3FD000 && addr<=0x3FD1FF )))
		{
			//cli_printf("addr error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else if (flash_id == EN25Q32C)
	{
		if(!(addr>=0x3FF000 && addr<=0x3FF1FF))
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
	
	spiFlash_cmd_wrEnable();

	spiFlash_format_addr(SPIFLASH_CMD_SE, addr, SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_NONE);

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
 *     @param[in]	addr--The address where you want to write data to flash OTP area.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash OTP area.
 *     @param[in]	length--The length of data to be written to the specified address of flash OTP area.
 *     @return      0--write ok, -5--write enable not set,-6--the memory is busy in write, not write completely,-8--otp program error.
 */
 
int spiFlash_EN_OTP_Write(struct _T_SPI_FLASH_DEV *  p_flash_dev, unsigned int addr, unsigned int len, unsigned char * buf)
{
	unsigned int flash_id = p_flash_dev->flash_param.flash_id;

	if (spiFlash_EN_check_addr(flash_id, addr, len))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	if ((len <= 0) ||(!buf))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
		
	if (drv_spiflash_write(addr, buf, len) != FLASH_RET_SUCCESS)
	{
		return FLASH_RET_PROGRAM_ERROR;
	}

	return FLASH_RET_SUCCESS;	
}



int spiFlash_EN_probe(T_SPI_FLASH_DEV *p_flash_dev)
{
	int i;
	const T_SPI_FLASH_PARAM * param;
	T_SPI_FLASH_OTP_OPS * ops;

	for (i=0; i<FLASH_ARRAY_SIZE(en_flash_params); i++)
	{
		param = &en_flash_params[i];
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

	if (i == FLASH_ARRAY_SIZE(en_flash_params))
	{
		return FLASH_RET_ID_ERROR;
	}

	ops = &p_flash_dev->flash_otp_ops;
	ops->flash_otp_read = spiFlash_EN_OTP_Read;
	ops->flash_otp_write = spiFlash_EN_OTP_Write;
	ops->flash_otp_erase= spiFlash_EN_OTP_Se;
	ops->flash_otp_lock = spiFlash_EN_OTP_Lock;

	return FLASH_RET_SUCCESS;
}



#else
/**    @brief		Choose the working mode of flash.
 *	   @details 	Different working modes of flash are configured according to parameters.
 *     @param[in]	mode--E_SPI_MODE supports this type of data.
 *     @return      0--ok, -7--flash id is not EN series.
 */
int spiFlash_api_select_mode(E_SPI_MODE mode)
{
	if((Flash_ID != EN25Q16B)){
		return FLASH_RET_ID_ERROR;
	}

	if (mode == SPI_MODE_QUAD)
	{
		Cmd_assign = &(gflashobj->QUAD);
	}
	else if (mode == SPI_MODE_STANDARD)
	{
		Cmd_assign = &(gflashobj->REGULAR);
	}
	else if(mode == SPI_MODE_DUAL)
	{
		Cmd_assign = &(gflashobj->DUAL);
	}

	return FLASH_RET_SUCCESS;
}


/**    @brief		Read data in flash OTP area api.
 *	   @details 	Read the flash OTP data of the specified length from the specified address in the flash memory into the buff.
 *     @param[in]	addr--The address where you want to read data in flash OTP area.
 *     @param[in]	* buf--Data of specified length read from specified address in flash OTP memory.
 *     @param[in]	len--The specified length read from the specified address in flash OTP memory. 
 *     @return      0--ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds, -7--flash id is not EN series.
 */
int spiFlash_OTP_Read(int addr,int length,unsigned char *pdata)
{
	unsigned int *p_dst_buffer = (unsigned int *)pdata;
	spi_reg * spiReg = spiDev.spiReg;

	if(length <= 0)
	{
		cli_printf("length error\n");
		return FLASH_RET_PARAMETER_ERROR;
	}
	if(Flash_ID == EN25Q16B)
	{
		if(!((addr>=0x1FF000 && addr<=0x1FF1FF && (addr + length -1) <= 0x1FF1FF)
		   ||(addr>=0x1FE000 && addr<=0x1FE1FF && (addr + length -1) <= 0x1FE1FF)
		   ||(addr>=0x1FD000 && addr<=0x1FD1FF && (addr + length -1) <= 0x1FD1FF)))
		{
			cli_printf("addr or length error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		cli_printf("flashID error:0x%x\n",Flash_ID);
		return FLASH_RET_ID_ERROR;
	}
	
	//wait spi-idle
	if(!spi_bus_ready()) 
		return FLASH_RET_SPI_BUS_BUSY;

	//clear rx & tx fifo
	if(spi_fifo_reset())
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;

	spiReg->transCtrl = SPI_TRANSCTRL_CMD_EN 
						 |SPI_TRANSCTRL_ADDR_EN 
						 |SPI_TRANSCTRL_TRAMODE_DR 
						 |SPI_TRANSCTRL_DUMMY_CNT_1
						 |SPI_TRANSCTRL_DUALQUAD_REGULAR
						 |SPI_TRANSCTRL_RCNT(length-1);
	spiReg->addr =  addr;
	spiReg->cmd = SPIFLASH_CMD_OTP_RD;
	
	
	unsigned int Rxcount = 0;
	unsigned int i;
	unsigned int data = spiReg->status;
	while( data & SPI_STATUS_BUSY )
	{           
		if(!(data & SPI_STATUS_RXEMPTY))
	  	{
	        Rxcount = ((data & 0x00001f00)>>8) ;
	        for (i = 0; i < Rxcount; i++) 
			{
				*p_dst_buffer++ = spiReg->data;
	        }
      	}
		data = spiReg->status;
    }
	Rxcount = ((data & 0x00001f00)>>8) ;
    for (i = 0; i < Rxcount; i++) 
	{
		*p_dst_buffer++ = spiReg->data;
    }

	return FLASH_RET_SUCCESS;
}


/**    @brief		lock flash OTP area data.
 *	   @details 	The Security Registers Lock Bit can be used to OTP protect the security registers. Once the LB bit is set to 1, the Security Registers will be permanently locked; the Erase and Write command will be ignored.
 *     @param[in]	LB--0x01: lock all(lock Security Register 0x000000-0x0003FF),  default:do nothing.
 *     @return      0--ok, -4--The input parameter addr or length is out of bounds, -7--flash id is not EN series.
 */
int spiFlash_OTP_Lock(int LB)
{
	unsigned int data = spiFlash_cmd_rdSR_2() & 0xFF;
	data = data<<8;
	
	if(Flash_ID == EN25Q16B)
	{	
		switch (LB)
		{
			case 0x01:
				data |= 0x0400; 
				break;
			default:
				cli_printf("parameter error\n");
				return FLASH_RET_PARAMETER_ERROR;
		}
		spiFlash_cmd_wrSR_1(data);
	}
	else
	{
		cli_printf("flashID error:0x%x\n",Flash_ID);
		return FLASH_RET_ID_ERROR;
	}
	
	return FLASH_RET_SUCCESS;
}


/**    @brief		flash OTP sector erase.
 *	   @details 	Send sector erase CMD, secrot erase one block(0x400) once time.
 *     @param[in]	add--Start erasing address into the flash OTP memory.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds,-5--write enable not set,-6--the memory is busy in erase, not erase completely,-7--flash id is not EN series..
 */
int spiFlash_OTP_Se(unsigned int addr)
{
	unsigned int j, data,irq_flag;
	spi_reg * spiReg = spiDev.spiReg;
	
    if(Flash_ID == EN25Q16B)
	{
		if(!((addr>=0x1FF000 && addr<=0x1FF1FF)||(addr>=0x1FE000 && addr<=0x1FE1FF)||(addr>=0x1FD000 && addr<=0x1FD1FF )))
		{
			cli_printf("addr error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		cli_printf("flashID error:0x%x\n",Flash_ID);
		return FLASH_RET_ID_ERROR;
	}

	irq_flag = system_irq_save();
	for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		spiFlash_format_none(SPIFLASH_CMD_WREN);
		data = spiFlash_cmd_rdSR_1();
		if(data & SPIFLASH_STATUS_WEL)
			break;
	}

	if((data & SPIFLASH_STATUS_WEL) == 0)
	{
		system_irq_restore(irq_flag);
		return FLASH_RET_WREN_NOT_SET;
	}

	//wait spi-idle
	if(!spi_bus_ready())
		return FLASH_RET_SPI_BUS_BUSY;

	//clear rx & tx fifo
	if(spi_fifo_reset())
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;

	spiReg->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_NONE;
	spiReg->addr = addr;
	spiReg->cmd = SPIFLASH_CMD_OTP_SE;

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
		return FLASH_RET_WIP_NOT_SET;
	
	return FLASH_RET_SUCCESS;
}


/**    @brief		Write data in OTP area of flash item.
 *	   @details 	Write the specified length of data to the specified address into the flash OTP memory.
 *     @param[in]	* pSpiReg--Structure of SPI register.
 *     @param[in]	addr--The address where you want to write data to flash OTP memory.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash OTP memory.
 *     @param[in]	length--The length of data to be written to the specified address of flash OTP memory.
 *     @return      0--ok, -1--spi bus status busy, -2--spi fifo reset not completes, -4--The input parameter addr or length is out of bounds, -7--flash id is not EN series.
 */
int spiFlash_OTP_Program(spi_reg * pSpiReg, unsigned int addr, unsigned char * buf, unsigned int length)
{
	unsigned int i, j, left, data;

	if(length <= 0)
	{
			cli_printf("length error\n");
			return FLASH_RET_PARAMETER_ERROR;
	}

	if(Flash_ID == EN25Q16B)
	{
		if(!((addr>=0x1FF000 && addr<=0x1FF1FF && ((addr + length -1) <= 0x1FF1FF))
		   ||(addr>=0x1FE000 && addr<=0x1FE1FF && ((addr + length -1) <= 0x1FE1FF))
		   ||(addr>=0x1FD000 && addr<=0x1FD1FF && ((addr + length -1) <= 0x1FD1FF))))
		{
			cli_printf("addr or length error\n");
			return FLASH_RET_PARAMETER_ERROR;
		}
	}
	else
	{
		cli_printf("flashID error:0x%x\n",Flash_ID);
		return FLASH_RET_ID_ERROR;
	}	
	
	for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		spiFlash_format_none(SPIFLASH_CMD_WREN);
		data = spiFlash_cmd_rdSR_1();
		if(data & SPIFLASH_STATUS_WEL)
			break;
	}

	if((data & SPIFLASH_STATUS_WEL) == 0)
	{
		return FLASH_RET_WREN_NOT_SET;
	}
	
	//wait spi-idle
	if(!spi_bus_ready())
		return FLASH_RET_SPI_BUS_BUSY;

	//clear rx & tx fifo
	if(spi_fifo_reset())
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;
	
	pSpiReg->addr = addr;
	pSpiReg->transCtrl = SPI_TRANSCTRL_CMD_EN|SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_WO|SPI_TRANSCTRL_DUALQUAD_REGULAR|SPI_TRANSCTRL_WCNT(length - 1);
	pSpiReg->cmd = SPIFLASH_CMD_OTP_PP;

	if((unsigned int)buf%4==0)
	{
		unsigned int *pdata = (unsigned int * )buf;

		left = (length + 3)/4;

		for(i = 0; i< left; i++)
		{
			SPI_WAIT_TX_READY(pSpiReg->status);
			pSpiReg->data = *pdata++;
		}
	}
	else
	{
		while (length > 0) 
		{
			/* clear the data */
			data = 0;

			/* data are usually be read 32bits once a time */
			left = MIN(length, 4);

			for (i = 0; i < left; i++) 
			{
				data |= *buf++ << (i * 8);
				length--;
			}
			/* wait till TXFULL is deasserted */
			SPI_WAIT_TX_READY(pSpiReg->status);				
			pSpiReg->data = data;
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
int spiFlash_OTP_Write(unsigned int addr, unsigned int len, unsigned char * buf)
{
	unsigned int address,  j, length, data,irq_flag;
	spi_reg * spiReg = spiDev.spiReg;

	length = addr % SPIFLASH_PAGE_SIZE;

	if(len > (SPIFLASH_PAGE_SIZE - length))
		length = SPIFLASH_PAGE_SIZE - length;
	else
		length = len;

	address = addr;	
	
	do {
		irq_flag = system_irq_save();

		for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			spiFlash_format_none(SPIFLASH_CMD_WREN);
			data = spiFlash_cmd_rdSR_1();
			if(data & SPIFLASH_STATUS_WEL)
				break;
		}

		if((data & SPIFLASH_STATUS_WEL) == 0)
		{
			system_irq_restore(irq_flag);
			return FLASH_RET_WREN_NOT_SET;
		}

		if(spiFlash_OTP_Program(spiReg, address, buf+(address - addr), length) != FLASH_RET_SUCCESS){
			system_irq_restore(irq_flag);
			return FLASH_RET_PROGRAM_ERROR;}

		len -= length;
		address += length;

		if(len > SPIFLASH_PAGE_SIZE)
			length = SPIFLASH_PAGE_SIZE;
		else
			length = len;

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
	}while(length);

	return FLASH_RET_SUCCESS;	
}



void drv_spiflash_OTP_Read(int addr,int length,unsigned char *pdata)
{
	;
}



void drv_spiflash_OTP_Write(unsigned int addr, unsigned int len, unsigned char * buf)
{
	;
}
#endif

#endif//end CONFIG_ID_EN25Q16B



