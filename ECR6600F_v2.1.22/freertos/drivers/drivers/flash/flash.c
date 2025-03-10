/**
* @file       flash.c
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
#include "efuse.h"
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "chip_memmap.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "cli.h"
//#include "portmacro.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

#include "flash_internal.h"

#include "hal_aes.h"
#include "aes.h"
#include "dma.h"
#include "gpio.h"

/* xip read mode */
typedef enum
{
	SPIFLASH_READ_CMD_03 = 0,
	SPIFLASH_READ_CMD_0B,
	SPIFLASH_READ_CMD_3B,
	SPIFLASH_READ_CMD_6B,
	SPIFLASH_READ_CMD_BB,
	SPIFLASH_READ_CMD_EB,
	SPIFLASH_READ_CMD_MAX
} E_SPIFLASH_READ_CMD;

typedef enum
{
    SPI_FLASH_CTRL_DATA_EVENT,
    SPI_FLASH_RX_DATA_EVENT,
    SPI_FLASH_TX_DATA_EVENT,
} flash_event_e;

typedef void (*spi_flash_callback_func)(flash_event_e evt, unsigned int *cfg);

typedef struct {
    unsigned int tx_dma_ch;
    unsigned int rx_dma_ch;
    //unsigned int buff[16];
    spi_flash_callback_func call_func;
} spi_flash_priv_t;

spi_flash_priv_t g_flash_priv;
unsigned int p_efuse_ctrl = 0;


#define EN25Q16B			0x15701C
#define EN25QH32B			0x16701C
#define EN25Q32C			0x16301C



#if defined (CONFIG_STANDARD_SPI)

#define DRV_FLASH_XIP_CMD			SPIFLASH_READ_CMD_0B
#define DRV_FLASH_READ_CMD		0x0B
#define DRV_FLASH_READ_PARAM		( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_TRAMODE_DR				\
										| SPI_TRANSCTRL_DUALQUAD_REGULAR		\
										| SPI_TRANSCTRL_DUMMY_CNT_1)

#define DRV_FLASH_WRITE_CMD		0x02
#define DRV_FLASH_WRITE_PARAM	( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_TRAMODE_WO				\
										| SPI_TRANSCTRL_DUALQUAD_REGULAR )


#elif defined (CONFIG_DUAL_SPI)

#define DRV_FLASH_XIP_CMD			SPIFLASH_READ_CMD_BB
#define DRV_FLASH_READ_CMD		0xBB
#define DRV_FLASH_READ_PARAM		( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_ADDR_FMT				\
										| SPI_TRANSCTRL_TRAMODE_DR			\
										| SPI_TRANSCTRL_DUALQUAD_DUAL		\
										| SPI_TRANSCTRL_TOKEN_EN)

#define DRV_FLASH_WRITE_CMD		0x02
#define DRV_FLASH_WRITE_PARAM	( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_TRAMODE_WO				\
										| SPI_TRANSCTRL_DUALQUAD_REGULAR )

#elif defined (CONFIG_QUAD_SPI)

#define DRV_FLASH_XIP_CMD			SPIFLASH_READ_CMD_EB
#define DRV_FLASH_READ_CMD		0xEB
#define DRV_FLASH_READ_PARAM		( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_ADDR_FMT				\
										| SPI_TRANSCTRL_TRAMODE_DR			\
										| SPI_TRANSCTRL_DUALQUAD_QUAD		\
										| SPI_TRANSCTRL_TOKEN_EN			\
										| SPI_TRANSCTRL_DUMMY_CNT_2)

#define DRV_FLASH_WRITE_CMD		0x32
#define DRV_FLASH_WRITE_PARAM	( SPI_TRANSCTRL_CMD_EN						\
										| SPI_TRANSCTRL_ADDR_EN				\
										| SPI_TRANSCTRL_TRAMODE_WO				\
										| SPI_TRANSCTRL_DUALQUAD_QUAD )

#else

	#error no spi-flash mode cfg!!!!

#endif




static T_SPI_FLASH_DEV flash_dev;



/**    @brief		Check spi bus status.
 *	   @details 	Check spi bus status within 1000 cycles. 
 *     @return      0--spi bus status busy, 1--spi bus status not busy.
 */
int spi_bus_ready(void)
{
	unsigned int i;

	for (i = 0; i < CHECK_TIMES; i++)
	{
		if ((FLASH_SPI_REG->status & SPI_STATUS_BUSY) == 0)
		{
			return SPI_BUS_STATUS_NOBUSY;
		}
	}

	return SPI_BUS_STATUS_BUSY;
}

/**    @brief		Reset spi fifo.
 *	   @details 	Reset spi fifo and check spi fifo status within 1000 cycles.
 *     @return      0--spi fifo reset completes, 1--spi fifo reset not completes.
 */
int spi_fifo_reset(void)
{
	unsigned int i;

	FLASH_SPI_REG->ctrl = FLASH_SPI_REG->ctrl |(SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST);

	for (i = 0; i < CHECK_TIMES; i++)
	{	
		if ((FLASH_SPI_REG->ctrl & (SPI_CONTROL_RXFIFORST|SPI_CONTROL_TXFIFORST)) == 0)
		{
			return SPI_FIFO_RESET_COMPLETE;
		}
	}

	return SPI_FIFO_RESET_NOCOMPLETE;
}

/**    @brief		Check spi rx status.
 *	   @details 	Check spi rx status within 20000 cycles.
 *     @return      0--spi rx empty status, 1--spi rx not empty status.
 */
int spi_rx_ready(void)
{
	unsigned int i;

	for (i = 0; i < READY_CHECK_TIMES; i++)
	{
		if ((FLASH_SPI_REG->status & SPI_STATUS_RXEMPTY) == 0)
		{
			return SPI_RX_NOEMPTY_STATUS;
		}
	}

	return SPI_RX_EMPTY_STATUS;
}

#if 1
int spiFlash_EDP(void)
{
	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_NONE;	
	FLASH_SPI_REG->cmd = 0xB9;

	return FLASH_RET_SUCCESS;
}



/**    @brief		Release from Deep Power-down.
 *	   @details 	The RDP instruction is for releasing from Deep Power-Down Mode.
 *     @return      0--ok.
 */
int spiFlash_RDP(void)
{
	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_NONE;	
	FLASH_SPI_REG->cmd = 0xAB;

	return FLASH_RET_SUCCESS;
}
#endif

/**    @brief		Only cmd send instruction.
 *	   @details 	This configuration only supports sending instruction of CMD timing.
 *     @param[in]	cmd--Only send the instruction of CMD timing.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes.
 */
int spiFlash_format_none(int cmd)
{
	//wait spi-idle
	if (!spi_bus_ready())
	{
		return FLASH_RET_SPI_BUS_BUSY;
	}

	//clear rx & tx fifo
	if (spi_fifo_reset())
	{
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;
	}

	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_NONE;
	FLASH_SPI_REG->cmd = cmd;

	return FLASH_RET_SUCCESS;
}


/**    @brief		Read-only Cmd send instruction.
 *	   @details 	This configuration is read-only mode and supports read data length corresponding to the CMD sequential instructions.
 *     @param[in]	cmd--Read-only Cmd.
 *     @param[in]	rd_len--The read data length corresponding to the CMD sequence, max 4 byte.
 *     @return      spiReg->data--receive spi rx data, -1--spi bus status busy, -2--spi fifo reset not completes, -3--spi rx empty status, no data received.
 */
unsigned int spiFlash_format_rd(int cmd, int rd_len)
{
	//wait spi-idle
	if (!spi_bus_ready())
	{
		return FLASH_RET_SPI_BUS_BUSY;
	}

	//clear rx & tx fifo
	if (spi_fifo_reset())
	{
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;
	}

	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_RO|SPI_TRANSCTRL_RCNT(rd_len-1);
	FLASH_SPI_REG->cmd = cmd;

	//wait data ready
	if(!spi_rx_ready())
	{
		return FLASH_RET_SPI_RX_EMPTY;
	}

	return FLASH_SPI_REG->data;
}


/**    @brief		Write-only Cmd send instruction.
 *	   @details 	This configuration is Write-only mode and supports write data length corresponding to the CMD sequential instructions.
 *     @param[in]	cmd--write-only Cmd.
 *     @param[in]	data--write data.
 *     @param[in]	wr_len--The write data length corresponding to the CMD sequence, max 4 byte.
 */
void spiFlash_format_wr(int cmd, int data, int wr_len)
{
	unsigned int status;
	
	do
	{
		spiFlash_format_none(SPIFLASH_CMD_WREN);
		status = spiFlash_format_rd(SPIFLASH_CMD_RDSR_1, 1);
	} while((status & SPIFLASH_STATUS_WEL) == 0);

	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_WO |SPI_TRANSCTRL_WCNT(wr_len-1);
	FLASH_SPI_REG->cmd =  cmd;
	FLASH_SPI_REG->data = data;

	do
	{
		status = spiFlash_format_rd(SPIFLASH_CMD_RDSR_1, 1);
	} while((status & SPIFLASH_STATUS_WIP) == 1);
}

void spiFlash_format_wr_fast(int cmd, int data, int wr_len)
{
	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_WO |SPI_TRANSCTRL_WCNT(wr_len-1);
	FLASH_SPI_REG->cmd =  cmd;
	FLASH_SPI_REG->data = data;
}

void spiFlash_format_otp_wr(int cmd, int data, int wr_len)
{
	unsigned int status;
	
	FLASH_SPI_REG->transCtrl = SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_TRAMODE_WO |SPI_TRANSCTRL_WCNT(wr_len-1);
	FLASH_SPI_REG->cmd =  cmd;
	FLASH_SPI_REG->data = data;

	do
	{
		status = spiFlash_format_rd(SPIFLASH_CMD_RDSR_1, 1);
	} while((status & SPIFLASH_STATUS_WIP) == 1);
}


//todo: add function note, and then delete this note
//erase can use
int spiFlash_format_addr(int cmd, unsigned int addr, unsigned int param)
{
	//wait spi-idle
	if (!spi_bus_ready())
	{
		return FLASH_RET_SPI_BUS_BUSY;
	}

	//clear rx & tx fifo
	if (spi_fifo_reset())
	{
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;
	}

	FLASH_SPI_REG->transCtrl = param;
	FLASH_SPI_REG->addr = addr;	
	FLASH_SPI_REG->cmd =  cmd;

	return FLASH_RET_SUCCESS;
}

int spiFlash_format_addr_no(int cmd, unsigned int addr, unsigned int param)
{
	//wait spi-idle
	if (!spi_bus_ready())
	{
		return FLASH_RET_SPI_BUS_BUSY;
	}

	//clear rx & tx fifo
	if (spi_fifo_reset())
	{
		return FLASH_RET_SPI_FIFO_NOCOMPLETE;
	}

	FLASH_SPI_REG->transCtrl = param;
	FLASH_SPI_REG->cmd =  cmd;

	return FLASH_RET_SUCCESS;
}


/**    @brief		Write-enable Cmd send instruction.
 *	   @details 	You need to perform this operation before writing or erasing.
 */
void spiFlash_cmd_wrEnable(void)
{
	spiFlash_format_none(SPIFLASH_CMD_WREN);
}

#if 0
/**    @brief		Entry QPI mode.
 *	   @details 	Instructions to be sent to enter QPI mode.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes.
 */
int spiFlash_cmd_entry_QPI(void)
{
	return spiFlash_format_none(SPIFLASH_CMD_ENTER_QPI);
}


/**    @brief		Exit QPI mode.
 *	   @details 	Instructions to be sent to exit QPI mode.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes.
 */
int spiFlash_cmd_exit_QPI(void)
{
	return spiFlash_format_none(SPIFLASH_CMD_EXIT_QPI);
}
#endif

/**    @brief		Read flash ID.
 *	   @details 	Send read flash ID CMD, and return 3byte flash id.
 *     @return      spiReg->data--read flash id data, -1--spi bus status busy, -2--spi fifo reset not completes, -3--spi rx empty status, no data received.
 */
static int spiFlash_cmd_rdID(void)
{
	return spiFlash_format_rd(SPIFLASH_CMD_RDID, 3) & (~(0xFF<<24));
}


/**    @brief		read flash status register-1 information.
 *	   @details 	Read status register-1 timing instruction, different flash ID will return different effective information.
 *     @return      spiReg->data--status register-1 data(/register-1 & register-2 data), -1--spi bus status busy, -2--spi fifo reset not completes, -3--spi rx empty status, no data received.
 */
unsigned int spiFlash_cmd_rdSR_1(void)
{
	return spiFlash_format_rd(SPIFLASH_CMD_RDSR_1, 2);
}


/**    @brief		read flash status register-2 information.
 *	   @details 	Read status register-2 timing instruction, different flash ID will return different effective information data length.
 *     @return      spiReg->data--status register-2 data, -1--spi bus status busy, -2--spi fifo reset not completes, -3--spi rx empty status, no data received.
 */
unsigned int spiFlash_cmd_rdSR_2(void)
{
	return spiFlash_format_rd(SPIFLASH_CMD_RDSR_2, 2);
}


/**    @brief		Write flash status register-1 information.
 *	   @details 	The Write Status Register-1 (WRSR_1) command allows new values to be written to the Status Register,Different flash id write different data length to the status register.
 *     @param[in]	data--Data to be written to the status register-1 (/register-1 & register-2).
 */
void spiFlash_cmd_wrSR_1(int data)
{
	spiFlash_format_wr(SPIFLASH_CMD_WRSR_1, data, 2);
}

void spiFlash_cmd_otpmode_wrSR_1(int data)
{
	spiFlash_format_otp_wr(SPIFLASH_CMD_WRSR_1, data, 1);
}

/**    @brief		Write flash status register-2 information.
 *	   @details 	The Write Status Register-2 (WRSR_2) command allows new values to be written to the Status Register.
 *     @param[in]	data--Data to be written to the status register-2.
 */
void spiFlash_cmd_wrSR_2(int data)
{
	spiFlash_format_wr(SPIFLASH_CMD_WRSR_2, data, 1);
}

#if 0
/**    @brief		Chip erase instruction.
 *	   @details 	The Chip Erase (CE) instruction is for erasing the data of the whole chip to be "1".
 */
static void spiFlash_cmd_ce(void)
{
	spiFlash_format_none(SPIFLASH_CMD_CE);
}
#endif

/**    @brief		Sector erase instruction.
 *	   @details 	Send sector erase CMD, secrot erase 4k byte once time.
 *     @param[in]	add--Start erasing address.
 *     @return      0--send CMD ok, -1--spi bus status busy, -2--spi fifo reset not completes.
 */
int spiFlash_cmd_se(unsigned int addr)
{
	return spiFlash_format_addr(SPIFLASH_CMD_SE, addr, 
					SPI_TRANSCTRL_CMD_EN |SPI_TRANSCTRL_ADDR_EN|SPI_TRANSCTRL_TRAMODE_NONE);
}


/**    @brief		Flash write data instruction timing.
 *	   @details 	Write the specified length of data to the specified address into the flash memory.
 *     @param[in]	* pSpiReg--Structure of SPI register.
 *     @param[in]	addr--The address where you want to write data to flash.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash.
 *     @param[in]	length--The length of data to be written to the specified address of flash.
 */
static void spiFlash_cmd_page_program(unsigned int addr, const unsigned char * buf, unsigned int length)
{
	unsigned int i, left;

	spiFlash_format_addr(DRV_FLASH_WRITE_CMD, addr, DRV_FLASH_WRITE_PARAM | SPI_TRANSCTRL_WCNT(length - 1));
	//spiFlash_format_addr_no(DRV_FLASH_WRITE_CMD, addr, DRV_FLASH_WRITE_PARAM | SPI_TRANSCTRL_WCNT(length - 1));
	if ((unsigned int)buf%4==0)
	{
		unsigned int *pdata = (unsigned int * )buf;
		left = (length + 3)/4;

		for (i = 0; i< left; i++)
		{
			SPI_WAIT_TX_READY(FLASH_SPI_REG->status);
			FLASH_SPI_REG->addr = addr + (i*4); 
			FLASH_SPI_REG->data = *pdata++;
		}
	}
	else
	{
		while (length > 0) 
		{
			unsigned int data = 0;		/* clear the data */

			left = MIN(length, 4);		/* data are usually be read 32bits once a time */

			for (i = 0; i < left; i++) 
			{
				data |= *buf++ << (i * 8);
				length--;
			}

			SPI_WAIT_TX_READY(FLASH_SPI_REG->status);
			FLASH_SPI_REG->addr = addr + (i*4); 
			FLASH_SPI_REG->data = data;
		}
	}
}

#if 1
/**    @brief		Flash read data instruction timing. 
 *	   @details 	Read the data of the specified length from the specified address in the flash memory into the buff. max 512byte
 *     @param[in]	* pSpiReg--Structure of SPI register.
 *     @param[in]	addr--The address where you want to read data in flash.
 *     @param[in]	* buf--Data of specified length read from specified address in flash memory.
 *     @param[in]	length--The specified length read from the specified address in flash memory.
 *	   @note!!!!!	in AES encrpty mode param[in]addr & len must word aligned!!!!!!!!!!!!!!!!
 */
static void spiFlash_read(unsigned int addr, unsigned char * buf, unsigned int len)
{
	int i,rx_num;
	unsigned int data, irq_flag;	//word aligned
	unsigned int * rxBuf;
	irq_flag = system_irq_save();

	spi_fifo_reset();
	
	FLASH_SPI_REG->addr = addr;
	FLASH_SPI_REG->transCtrl = DRV_FLASH_READ_PARAM | SPI_TRANSCTRL_RCNT(len - 1);
	FLASH_SPI_REG->cmd = DRV_FLASH_READ_CMD;

	if(((unsigned int)buf%4==0) && ((unsigned int) len % 4 == 0))
	{
		rxBuf = (unsigned int *)buf;
		data = (len+3)/4;

		while((FLASH_SPI_REG->status & SPI_STATUS_BUSY) == 1)
		{
			rx_num = ((FLASH_SPI_REG->status) >> 8) & 0x1F;
			rx_num = MIN(rx_num , data);

			for(i=0; i<rx_num; i++)
			{
				*rxBuf++ = FLASH_SPI_REG->data;
			}

			data -= rx_num;

			if(data == 0)
			{
				system_irq_restore(irq_flag);
				return;
			}
		}

		for(i=0; i<data; i++)
		{
			*rxBuf++ = FLASH_SPI_REG->data;
		}
	}
	else
	{
		while(len)
		{
			rx_num = MIN(4, len);
			SPI_WAIT_RX_READY(FLASH_SPI_REG->status);
			data = FLASH_SPI_REG->data;

			for(i=0; i<rx_num; i++)
			{
				*buf++ = (unsigned char)data;
				data >>= 8;
				len--;
			}
		}
	}
	system_irq_restore(irq_flag);
}



/**    @brief		Flash read data instruction timing. 
 *	   @details 	Read the data of the specified length from the specified address in the flash memory into the buff. max 512byte
 *     @param[in]	* pSpiReg--Structure of SPI register.
 *     @param[in]	addr--The address where you want to read data in flash.
 *     @param[in]	* buf--Data of specified length read from specified address in flash memory.
 *     @param[in]	length--The specified length read from the specified address in flash memory.
 *	   @note!!!!!	in AES encrpty mode param[in]addr & len must word aligned!!!!!!!!!!!!!!!!
 */
int drv_spiflash_read_apb(unsigned int addr, unsigned char * buf, unsigned int len)
{
	if (((len + addr) > flash_dev.flash_param.flash_size) || (len ==0) )
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	//spi mode
	unsigned int len_all = 0;
	unsigned int curr_off = 0;
	unsigned int curr_len = 0;

	len_all = len;
	do
	{
		curr_len = MIN(0x200, len_all);
		spiFlash_read(addr + curr_off, (unsigned char *)(&buf[curr_off]), curr_len);
		len_all -= curr_len;
		curr_off += curr_len;
	}while(len_all);

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}

#endif


/**    @brief		Set SPI XIP read data mode.
 *	   @details 	Supported SPI Read Commands for Memory-Mapped AHB/EILM Reads.
 *     @param[in]	xip_rd_cmd--xip read mode, E_ SPIFLASH_ READ_ CMD supports this type of data.
 */
static void spiFlash_xip_read_cfg(void)
{
	unsigned int data = FLASH_SPI_REG->memCtrl;

	data &= FLASH_REG_XIP_MASK_BIT;
	data |=  DRV_FLASH_XIP_CMD;

	FLASH_SPI_REG->memCtrl = data;
}


/**    @brief		Read flash data by SPI XIP mode.
 *	   @details 	Read the data of the specified length from the specified address in the flash memory into the buff.
 *     @param[in]	addr--The address where you want to read data in flash.
 *     @param[in]	* buf--Data of specified length read from specified address in flash memory.
 *     @param[in]	length--The specified length read from the specified address in flash memory.
 */
static void spiFlash_xip_read_mem(unsigned int addr, unsigned char * buf, unsigned int len)
{
	unsigned int enc_en = READ_REG(CHIP_SMU_PD_APB_ENCRY_EN);
	WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, APB_WRIT_NOT_ENCRYPT);

	memcpy((void *)buf, (void *)(addr + MEM_BASE_XIP), len);
	
	WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, enc_en);
}


static void spiFlash_clk_cfg(void)
{
	unsigned int data = FLASH_SPI_REG->timing;

	data &= 0xFFFFFF00;

#if defined (CONFIG_SPI_CLK_DIV_0)
	data |=  SPI_SCLK_DIV_0;
#elif defined (CONFIG_SPI_CLK_DIV_2)
	data |=  SPI_SCLK_DIV_2;
#elif defined (CONFIG_SPI_CLK_DIV_4)
	data |=  SPI_SCLK_DIV_4;
#elif defined (CONFIG_SPI_CLK_DIV_6)
	data |=  SPI_SCLK_DIV_6;
#elif defined (CONFIG_SPI_CLK_DIV_8)
	data |=  SPI_SCLK_DIV_8;
#elif defined (CONFIG_SPI_CLK_DIV_10)
	data |=  SPI_SCLK_DIV_10;
#else
	data |=  SPI_SCLK_DIV_0;
#endif

	FLASH_SPI_REG->timing = data;
}

#if 0
static void spiFlash_QE_cfg(T_SPI_FLASH_DEV *p_flash_dev)
{
	unsigned int QE;
	QE = spiFlash_cmd_rdSR_2();

	if(p_flash_dev->flash_param.flash_id == 0x146085 || p_flash_dev->flash_param.flash_id == 0x1440c8 || p_flash_dev->flash_param.flash_id == 0x15400b || p_flash_dev->flash_param.flash_id == 0x14400b || p_flash_dev->flash_param.flash_id == 0x1560EB)//P25Q80H & GD25Q80E & P25Q16H & XT25F08 & XT25F16 & TH25Q16HB
	{
		spiFlash_cmd_wrSR_1(QE | SPIFLASH_STATUS_QE);
	}
	else if(p_flash_dev->flash_param.flash_id == 0x156085)
	{
		spiFlash_cmd_wrSR_1(QE | SPIFLASH_STATUS_QE);
		spiFlash_cmd_wrSR_2(QE | SPIFLASH_STATUS_2_QE);
	}
	else if(p_flash_dev->flash_param.flash_id == 0x16701C || p_flash_dev->flash_param.flash_id == 0x16301C || p_flash_dev->flash_param.flash_id == 0x15701C)	//EN25QH32B EN25Q16B EN25Q32C
	{
		//spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_2,QE | SPIFLASH_STATUS_2_QE,2);
	}
	else
	{
		spiFlash_cmd_wrSR_2(QE | SPIFLASH_STATUS_2_QE);
	}
	//QE = spiFlash_cmd_rdSR_2();
}
#else
static void spiFlash_QE_cfg_restore(T_SPI_FLASH_DEV *p_flash_dev)
{
	unsigned int QE;
	QE = spiFlash_cmd_rdSR_2();

	if(p_flash_dev->flash_param.flash_id == 0x146085 || p_flash_dev->flash_param.flash_id == 0x1440c8 || p_flash_dev->flash_param.flash_id == 0x15400b || p_flash_dev->flash_param.flash_id == 0x14400b || p_flash_dev->flash_param.flash_id == 0x1560EB)//P25Q80H & GD25Q80E & P25Q16H & XT25F08 & XT25F16 & TH25Q16HB
	{
		spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_1,QE | SPIFLASH_STATUS_QE,2);
	}
	else if(p_flash_dev->flash_param.flash_id == 0x156085)
	{
		spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_1,QE | SPIFLASH_STATUS_QE,2);
		spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_2,QE | SPIFLASH_STATUS_2_QE,2);
	}
	else if(p_flash_dev->flash_param.flash_id == 0x16701C || p_flash_dev->flash_param.flash_id == 0x16301C || p_flash_dev->flash_param.flash_id == 0x15701C)	//EN25QH32B EN25Q16B EN25Q32C
	{
		//spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_2,QE | SPIFLASH_STATUS_2_QE,2);
	}
	else
	{
		spiFlash_format_wr_fast(SPIFLASH_CMD_WRSR_2,QE | SPIFLASH_STATUS_2_QE,2);
	}

}
#endif


#if 0
/**    @brief		Configure AES encryption mode of efuse.
 *	   @details 	When using the encryption mode to write data to flash, we need to configure the AES encryption mode control bit of efuse first.
 */
void efuse_aes_config(void)
{
	unsigned int i = 0;
	
	outw(APB_ENCRYPT_EN, inw(APB_ENCRYPT_EN)|0x01);
	//cli_printf("\r\nIN32(0x202078) = %x\r\n",inw(APB_ENCRYPT_EN));

	drv_efuse_write(0x20, 0x8, 0x00);

	for(i = 0; i < 4; i++)
	{
		drv_efuse_write(0x24, key[i], 0x00);
	}
	

}


/**    @brief		AES mode writes data to flash api.
 *	   @details 	using the encryption mode to write data to flash, Write the specified length of data to the specified address into the flash memory.
 *     @param[in]	addr--The address where you want to write data to flash.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash.
 *     @param[in]	length--The length of data to be written to the specified address of flash.
 *     @return      0--write ok, -5--write enable not set,-6--the memory is busy in write, not write completely.
 */
 int spiFlash_aes_write(unsigned int addr, unsigned char * buf, unsigned int len)
{
	unsigned int j, data, irq_flag;
	unsigned int test_len = 0;
	unsigned int address, length;
	spi_reg * spiReg = spiDev.spiReg;

	drv_efuse_init();

	if(len == 0)
	{
		return FLASH_RET_SUCCESS;
	}

	length = addr % SPIFLASH_PAGE_SIZE;
	length = MIN(SPIFLASH_PAGE_SIZE - length, len);

	address = addr;

	do {
		irq_flag = system_irq_save();

		for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			spiFlash_cmd_wrEnable();
			data = spiFlash_cmd_rdSR_1();
			if(data & SPIFLASH_STATUS_WEL)
				break;
		}
		
		if((data & SPIFLASH_STATUS_WEL) == 0)
		{
			system_irq_restore(irq_flag);
			return FLASH_RET_WREN_NOT_SET;
		}

		efuse_aes_config();
		//cli_printf("\r\n==>> write star ok\r\n");
		for(test_len = 0; test_len < len; test_len = test_len+4 )
		{
			spiFlash_cmd_wrEnable();
			spiFlash_cmd_page_program(spiReg, addr + test_len , buf + test_len, 4);
		}
		//cli_printf("\r\n==>> write end ok\r\n");
		
		drv_efuse_write(0x20, 0x100, 0x00);//cnt
		//	OUT32(APB_ENCRYPT_EN,0x00);


		len -= length;
		address += length;

		length = MIN(SPIFLASH_PAGE_SIZE, len);

		for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			data = spiFlash_cmd_rdSR_1();
			if((data & SPIFLASH_STATUS_WIP) == 0){
				break;}
		}

		if(data & SPIFLASH_STATUS_WIP)
		{
			system_irq_restore(irq_flag);
			return FLASH_RET_WIP_NOT_SET;
		}
		
		system_irq_restore(irq_flag);
	}while(length);

	return FLASH_RET_SUCCESS;
}
#endif
int check_oversize(unsigned int  addr, unsigned int len)
{
	
	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	return FLASH_RET_SUCCESS;
}


//************************************************in cpu mode 
#if 1
/**    @brief		Spiflash sector erase api.
 *	   @details 	Erase data in flash with specified address and length. The erase unit is 4K byte.
 *     @param[in]	addr--The address where you want to erase data to flash.
 *     @param[in]	length--The length of data to be erased to the specified address of flash.
 *     @return      0--erase ok,-4--erase addr or length not an integral multiple of 4K ,-5--write enable not set,-6--the memory is busy in erase, not erase completely.
 */
int drv_spiflash_erase(unsigned int addr,  unsigned int len)
{
	unsigned int eraseSectorCnt, i, j, data = 0, irq_flag;

	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	if ((addr%SPIFLASH_SECTOR_SIZE) || (len%SPIFLASH_SECTOR_SIZE))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	eraseSectorCnt = len / SPIFLASH_SECTOR_SIZE;
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	for (i=0; i<eraseSectorCnt; i++)
	{
		irq_flag = system_irq_save_tick_compensation();
		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			spiFlash_cmd_wrEnable();
			if(flash_dev.flash_param.flash_id == EN25QH32B || flash_dev.flash_param.flash_id == EN25Q16B || (flash_dev.flash_param.flash_id == EN25Q32C))
			{
					break;
			}
			else
			{
				data = spiFlash_cmd_rdSR_1();
				if(data & SPIFLASH_STATUS_WEL)
				{
					break;
				}
			}
		}
		
		if((data & SPIFLASH_STATUS_WEL) == 0 && ((flash_dev.flash_param.flash_id != EN25QH32B) && (flash_dev.flash_param.flash_id != EN25Q16B) && (flash_dev.flash_param.flash_id != EN25Q32C) ))
		{
			system_irq_restore_tick_compensation(irq_flag);
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			return FLASH_RET_WREN_NOT_SET;
		}

		//section erase
		spiFlash_cmd_se(addr + i * SPIFLASH_SECTOR_SIZE);

		for(j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			data = spiFlash_cmd_rdSR_1();
			if((data & SPIFLASH_STATUS_WIP) == 0)
			{
				break;
			}
		}

		system_irq_restore_tick_compensation(irq_flag);
		if(data & SPIFLASH_STATUS_WIP)
		{
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			return FLASH_RET_WIP_NOT_SET;
		}
	}

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}

#if 0
/**    @brief		Chip erase flash data.
 *	   @details 	Erasing the data of the whole chip to be "1".
 *     @return      0--chip erase ok, -5--write enable not set,-6--the memory is busy in erase, not erase completely.
 */
int drv_spiflash_chiperase(void)
{
	unsigned int j, data = 0;
	unsigned int irq_flag = system_irq_save();

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		spiFlash_cmd_wrEnable();
		if(flash_dev.flash_param.flash_id == EN25QH32B || flash_dev.flash_param.flash_id == EN25Q16B || (flash_dev.flash_param.flash_id == EN25Q32C))
		{
			break;
		}
		else
		{
			data = spiFlash_cmd_rdSR_1();
			if(data & SPIFLASH_STATUS_WEL)
			{
				break;
			}
		}
	}

	if((data & SPIFLASH_STATUS_WEL) == 0 && ((flash_dev.flash_param.flash_id != EN25QH32B) && (flash_dev.flash_param.flash_id != EN25Q16B) && (flash_dev.flash_param.flash_id != EN25Q32C) ))
	{
	
		system_irq_restore(irq_flag);
		
		#ifdef CONFIG_PSM_SURPORT
			psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
		#endif
		return FLASH_RET_WREN_NOT_SET;
	}

	spiFlash_cmd_ce();
                  
	for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
	{
		data = spiFlash_cmd_rdSR_1();
		if ((data & SPIFLASH_STATUS_WIP) == 0)
		{
			break;
		}
	}
	system_irq_restore(irq_flag);
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
		
	if (data & SPIFLASH_STATUS_WIP)
	{
		return FLASH_RET_WIP_NOT_SET;
	}


	return FLASH_RET_SUCCESS;
}
#endif

void up_IO_driver_strength(void)
{
	unsigned int chip_id,fuse_ft;

	drv_efuse_read(EFUSE_CHIP_ID, (unsigned int *)&fuse_ft, 4);
	chip_id = ((fuse_ft >> 27) & 0x1f);
	
	//os_printf(LM_OS, LL_ERR, "chip_id = 0x%x \n",chip_id);
	if((((fuse_ft & EFUSE_FT_MASK) == EFUSE_FT_TYPE_6600) && ((chip_id == 0x12) || (chip_id == 0x13) || (chip_id == 0x14) || (chip_id == 0x15))) ||
	   (((fuse_ft & EFUSE_FT_MASK) == EFUSE_FT_TYPE_1600) && ((chip_id == 0x00) || (chip_id == 0x01) || (chip_id == 0x04) || (chip_id == 0x05) || (chip_id == 0x08) || (chip_id == 0x09))) )
	{
		WRITE_REG(MEM_BASE_SMU_AON + 0x28, READ_REG(MEM_BASE_SMU_AON + 0x28) | 0x1f80);
		
		//os_printf(LM_OS, LL_ERR, "40 pin \n");
	}

}


/**
 * @brief 计算32bit中1的个数（通用型计算函数）
 */
int count_one_bits(unsigned int n) // 计算二进制1的个数
{
	int count = 0;
	while (n)
	{
		n = n & (n - 1);
		count++;
	}
	return count;
}

/**
 * @brief flash写加密判断
 */
void flash_encrypt_write_check()
{
	int one_bits_num = count_one_bits((p_efuse_ctrl&EFUSE_CTRL_FLASH_ENCRYPT_CNT_MASK));
	if((p_efuse_ctrl&EFUSE_CTRL_FLASH_ENCRYPT_EN) && 
		
((p_efuse_ctrl&EFUSE_CTRL_FLASH_ENCRYPT_CNT_MASK) != EFUSE_CTRL_FLASH_ENCRYPT_CNT_MASK) && 
		(one_bits_num%2))  // 1、使能了加密 2、计数标识不全为F 3、计数标识中1的个数为奇数
	{
		WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, APB_WRIT_ENCRYPT);
	}
}

/**    @brief		Read flash data api.
 *	   @details 	Read the data of the specified length from the specified address in the flash memory into the buff.
 *     @param[in]	addr--The address where you want to read data in flash.
 *     @param[in]	* buf--Data of specified length read from specified address in flash memory.
 *     @param[in]	len--The specified length read from the specified address in flash memory. 
 *     @return      0--ok.
 */
int drv_spiflash_read(unsigned int addr, unsigned char * buf, unsigned int len)
{
	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

#if 1
	//xip mode 
	spiFlash_xip_read_mem(addr, (unsigned char *)buf, len);
#else
	//spi mode
	unsigned int curr = 0;
	unsigned int curr_off = 0;
	unsigned int curr_len = 0;

	curr = len;
	do
	{
		curr_len = MIN(0x200, curr);
		spiFlash_read(addr + curr_off, (unsigned char *)(&buf[curr_off]), curr_len);
		curr -= curr_len;
		curr_off += curr_len;
	}while(curr);
#endif

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}


/**    @brief		Write data to flash api.
 *	   @details 	Write the specified length of data to the specified address into the flash memory..
 *     @param[in]	addr--The address where you want to write data to flash.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash.
 *     @param[in]	length--The length of data to be written to the specified address of flash.
 *     @return      0--write ok, -5--write enable not set,-6--the memory is busy in write, not write completely.
 *	   @note!!!!!	in AES encrpty mode param[in]addr & len must word aligned!!!!!!!!!!!!!!!!
 */
int drv_spiflash_write(unsigned int addr, const unsigned char * buf, unsigned int len)
{
	unsigned int address, length;
	unsigned int start_addr,end_addr;
	unsigned int nv_customer_addr,nv_customer_len,nv_amt_addr,nv_amt_len;

	if (len == 0)
	{
		return FLASH_RET_SUCCESS;
	}

	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	
	if ((partion_info_get(PARTION_NAME_NV_CUSTOMER,  &nv_customer_addr, &nv_customer_len) != 0) || (partion_info_get(PARTION_NAME_NV_AMT, &nv_amt_addr, &nv_amt_len)!=0))
	{
		return FLASH_RET_PARTITION_ERROR;
	}
	start_addr = nv_customer_addr;
	end_addr   = nv_amt_addr + nv_amt_addr;
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	length = addr % SPIFLASH_PAGE_SIZE;
	length = MIN(SPIFLASH_PAGE_SIZE - length, len);

	address = addr;

	do
	{
		unsigned int j, data = 0, irq_flag;

		irq_flag = system_irq_save_tick_compensation();
		unsigned int enc_en = READ_REG(CHIP_SMU_PD_APB_ENCRY_EN);
		if((addr < start_addr) || (addr > end_addr) ) 
		{
			flash_encrypt_write_check();
		}
		
		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			spiFlash_cmd_wrEnable();
			if(flash_dev.flash_param.flash_id == EN25QH32B || flash_dev.flash_param.flash_id == EN25Q16B|| (flash_dev.flash_param.flash_id == EN25Q32C) )
			{
				break;
			}
			else
			{
				data = spiFlash_cmd_rdSR_1();
				if(data & SPIFLASH_STATUS_WEL)
				{
					break;
				}
			}
		}
		
		if((data & SPIFLASH_STATUS_WEL) == 0 && ((flash_dev.flash_param.flash_id != EN25QH32B) && (flash_dev.flash_param.flash_id != EN25Q16B) && (flash_dev.flash_param.flash_id != EN25Q32C) ))
		{
			WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, enc_en);
			system_irq_restore_tick_compensation(irq_flag);
			
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			
			return FLASH_RET_WREN_NOT_SET;
		}
		
		spiFlash_cmd_page_program(address, ( const unsigned char * )(buf + (address - addr)), length);

		len -= length;
		address += length;

		length = MIN(SPIFLASH_PAGE_SIZE, len);

		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			data = spiFlash_cmd_rdSR_1();
			if ((data & SPIFLASH_STATUS_WIP) == 0)
			{
				break;
			}
		}

		if (data & SPIFLASH_STATUS_WIP)
		{
			WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, enc_en);
			system_irq_restore_tick_compensation(irq_flag);
			
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			
			return FLASH_RET_WIP_NOT_SET;
		}
		
		WRITE_REG(CHIP_SMU_PD_APB_ENCRY_EN, enc_en);
		system_irq_restore_tick_compensation(irq_flag);
	}while(length);
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}

 /**	@brief		 Initialize spiflash related configuration.
  * 	@details	 Initialization of spiflash configuration before using flash, The initialization operation only needs one time.
  * 	@param[in]	 clk_div--Frequency division parameters of SPI output frequency.
  * 	@param[in]	 xip_rd_cmd--xip read mode, E_ SPIFLASH_ READ_ CMD supports this type of data.
  */

int drv_spiFlash_init(void)
{
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;

	chip_clk_enable(CLK_SPI0_APB);
	chip_clk_enable(CLK_SPI0_AHB);

	WRITE_REG(MEM_BASE_SMU_PD + 0x18, SPI0_WORK_CLK_AHB_CLK);	// spi0 work clk choose ahb clk

	PIN_FUNC_SET(IO_MUX_GPIO7, FUNC_GPIO7_MSPI_MOSI);
	PIN_FUNC_SET(IO_MUX_GPIO8, FUNC_GPIO8_MSPI_HOLD);
	PIN_FUNC_SET(IO_MUX_GPIO9, FUNC_GPIO9_MSPI_CLK);
	PIN_FUNC_SET(IO_MUX_GPIO10, FUNC_GPIO10_MSPI_CS0);
	//PIN_FUNC_SET(IO_MUX_GPIO4, FUNC_GPIO4_PSRAM_CS);
	PIN_FUNC_SET(IO_MUX_GPIO11, FUNC_GPIO11_MSPI_MISO);
	PIN_FUNC_SET(IO_MUX_GPIO12, FUNC_GPIO12_MSPI_WP);

	spiFlash_clk_cfg();					/* cfg spi clk */
	spiFlash_xip_read_cfg();			/* cfg xip read-mem-cmd */

	p_flash_dev->flash_param.flash_id = spiFlash_cmd_rdID();
	
	//spiFlash_QE_cfg(p_flash_dev);
	
	//g_flash_priv.rx_dma_ch = (unsigned char)drv_dma_ch_alloc();
	//g_flash_priv.tx_dma_ch = (unsigned char)drv_dma_ch_alloc();
	
	up_IO_driver_strength();
	drv_efuse_read(EFUSE_CTRL_OFFSET, &p_efuse_ctrl, 4);
	return spiFlash_probe(p_flash_dev);
}


void spiFlash_api_set_dma_enable(void)
{
	FLASH_SPI_REG->ctrl = SPI_CONTROL_TXDMA_EN |SPI_CONTROL_RXDMA_EN ;
}

void spiFlash_api_set_dma_disable(void)
{
	//todo: optimize
	FLASH_SPI_REG->ctrl =  (FLASH_SPI_REG->ctrl & (~(SPI_CONTROL_TXDMA_EN |SPI_CONTROL_RXDMA_EN)));
}


unsigned int spiFlash_api_get_dma_addr(void)
{
	return (MEM_BASE_SPI0+0x2C);
}



#define DMAC_BASE				MEM_BASE_DMAC
#define DMAC_INTR_STS			(DMAC_BASE +0x30)
#define DMAC_INTR_TC(X)			(((X) >> 16) & 0xFF)

void drv_spi_flash_dma_isr(void *data)
{
	//os_printf(LM_OS, LL_INFO, "spi_flash_dma_trans_end\r\n");
}


int spiflash_read_DMA(unsigned int addr, unsigned char * buf, unsigned int len)
{
	unsigned int irq_flag;
	T_DMA_CFG_INFO dma_cfg_info;
    spi_flash_priv_t *pread = &g_flash_priv;
	irq_flag = system_irq_save_tick_compensation();	
	volatile unsigned int status = READ_REG(DMAC_INTR_STS);

	dma_cfg_info.dst = (unsigned int)buf;//�����ַ
	dma_cfg_info.src = spiFlash_api_get_dma_addr();  //spi reg :2C 
	dma_cfg_info.len = len; //256byte
	dma_cfg_info.mode = DMA_CHN_SPI_FLASH_RX;//8

	spiFlash_api_set_dma_enable();
	drv_dma_cfg(pread->rx_dma_ch, &dma_cfg_info);
	drv_dma_start(pread->rx_dma_ch);

	FLASH_SPI_REG->intrEn = SPI_INTERRUPT_ENDINT_EN;
	spiFlash_format_addr(DRV_FLASH_READ_CMD, addr, DRV_FLASH_READ_PARAM | SPI_TRANSCTRL_RCNT(len - 1));

	do
	{
		status = READ_REG(DMAC_INTR_STS);
		status = DMAC_INTR_TC(status);
		status = status >> (pread->rx_dma_ch);

	}while((status & 0x01) == 0);

	FLASH_SPI_REG->intrSt = SPI_INTERRUPT_ENDINT_EN;
	FLASH_SPI_REG->intrEn = (FLASH_SPI_REG->intrEn) & (~SPI_INTERRUPT_ENDINT_EN);
	drv_dma_status_clean(pread->rx_dma_ch);
	spiFlash_api_set_dma_disable();

	
	system_irq_restore_tick_compensation(irq_flag);
	return FLASH_RET_SUCCESS;
}


int spiflash_write_DMA(unsigned int addr, const unsigned char * buf, unsigned int len)
{
	unsigned int irq_flag;
	
	irq_flag = system_irq_save_tick_compensation();	
	volatile unsigned int status = READ_REG(DMAC_INTR_STS);

	T_DMA_CFG_INFO dma_cfg_info;
    spi_flash_priv_t *pread = &g_flash_priv;

	dma_cfg_info.dst = spiFlash_api_get_dma_addr();  //spi reg :2C 
	dma_cfg_info.src = (unsigned int)buf;//�����ַ
	dma_cfg_info.len = len; //256byte
	dma_cfg_info.mode = DMA_CHN_SPI_FLASH_TX;//7

	spiFlash_api_set_dma_enable();
	drv_dma_cfg(pread->tx_dma_ch, &dma_cfg_info);
	drv_dma_start(pread->tx_dma_ch);

	FLASH_SPI_REG->intrEn = SPI_INTERRUPT_ENDINT_EN;
	spiFlash_format_addr(DRV_FLASH_WRITE_CMD, addr, DRV_FLASH_WRITE_PARAM | SPI_TRANSCTRL_WCNT(len - 1));

	do
	{
		status = READ_REG(DMAC_INTR_STS);
		status = DMAC_INTR_TC(status);
		status = status >> (pread->tx_dma_ch);

	}while((status & 0x01) == 0);

	FLASH_SPI_REG->intrSt = SPI_INTERRUPT_ENDINT_EN;
	FLASH_SPI_REG->intrEn = (FLASH_SPI_REG->intrEn) & (~SPI_INTERRUPT_ENDINT_EN);
	drv_dma_status_clean(pread->tx_dma_ch);
	spiFlash_api_set_dma_disable();
	
	system_irq_restore_tick_compensation(irq_flag);
	return FLASH_RET_SUCCESS;
}



int drv_spiflash_read_dma(unsigned int addr, unsigned char * buf, unsigned int len)
{
	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	unsigned int curr = 0;
	unsigned int curr_off = 0;
	unsigned int curr_len = 0;

	curr = len;
	do
	{
		curr_len = MIN(0x200, curr);
		spiflash_read_DMA(addr + curr_off, (unsigned char *)(&buf[curr_off]), curr_len);
		curr -= curr_len;
		curr_off += curr_len;
	}while(curr);

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}



int drv_spiflash_write_dma(unsigned int addr, const unsigned char * buf, unsigned int len)
{
	unsigned int address, length;

	if (len == 0)
	{
		return FLASH_RET_SUCCESS;
	}

	if ((len + addr) > flash_dev.flash_param.flash_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	length = addr % SPIFLASH_PAGE_SIZE;
	length = MIN(SPIFLASH_PAGE_SIZE - length, len);
	address = addr;

	do
	{
		unsigned int j, data = 0, irq_flag;

		irq_flag = system_irq_save_tick_compensation();

		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			spiFlash_cmd_wrEnable();
			if(flash_dev.flash_param.flash_id == EN25QH32B || flash_dev.flash_param.flash_id == EN25Q16B || (flash_dev.flash_param.flash_id == EN25Q32C))
			{
				break;
			}
			else
			{
				data = spiFlash_cmd_rdSR_1();
				if(data & SPIFLASH_STATUS_WEL)
				{
					break;
				}
			}
		}
		
		if((data & SPIFLASH_STATUS_WEL) == 0 && ((flash_dev.flash_param.flash_id != EN25QH32B) && (flash_dev.flash_param.flash_id != EN25Q16B) && (flash_dev.flash_param.flash_id != EN25Q32C) ))
		{
			system_irq_restore_tick_compensation(irq_flag);
			
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			
			return FLASH_RET_WREN_NOT_SET;
		}
		
		spiflash_write_DMA(address, ( const unsigned char * )(buf + (address - addr)), length);

		len -= length;
		address += length;
		length = MIN(SPIFLASH_PAGE_SIZE, len);

		for (j = 0; j<SPIFLASH_TIMEOUT_COUNT; j++)
		{
			data = spiFlash_cmd_rdSR_1();
			if ((data & SPIFLASH_STATUS_WIP) == 0)
			{
				break;
			}
		}

		if (data & SPIFLASH_STATUS_WIP)
		{
			system_irq_restore_tick_compensation(irq_flag);
			
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			
			return FLASH_RET_WIP_NOT_SET;
		}
		
		system_irq_restore_tick_compensation(irq_flag);
	}while(length);
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif

	return FLASH_RET_SUCCESS;
}




#else
//****************************************************in dma mode


#endif



/**    @brief		Initialize spiflash OTP date to encrypt.
 *	   @details 	read all OTP date,and encrypty all date,then write in otp memory.
 *     @return      0--encrypt ok, other--error.
 */
int drv_otp_encrypt_init(void)
{
	FLASH_ENCRYPT_STATE_T STATES_1,STATES_2;
	unsigned int cpu_addr = 0, cpu_length = 0;
	unsigned char sfTest[0xC00] = {0};
	int ret; 
	//read cpu partition addr & length
	if(partion_info_get(PARTION_NAME_CPU, (unsigned int *)&cpu_addr, (unsigned int *)&cpu_length) == 0)
	{
		ret = drv_spiflash_read((cpu_addr + cpu_length - ENCRYPT_STATE_SIZE), (unsigned char *)&STATES_1, sizeof(FLASH_ENCRYPT_STATE_T));
		if(ret != FLASH_RET_SUCCESS)
		{
			return ret;
		}
		
		ret = drv_spiflash_read((cpu_addr + cpu_length - (ENCRYPT_STATE_SIZE * 2)), (unsigned char *)&STATES_2, sizeof(FLASH_ENCRYPT_STATE_T));		
		if(ret != FLASH_RET_SUCCESS)
		{
			return ret;
		}
		
		if((memcmp(STATES_1.magic, "ENCRYPT:", 8) == 0) || (memcmp(STATES_2.magic, "ENCRYPT:", 8) == 0))
		{
			T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
			const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
			unsigned int otp_total_leng;
			
			otp_total_leng = param->flash_otp_block_num * param->flash_otp_block_size;

			ret = drv_spiflash_OTP_Read(0x00, otp_total_leng, (unsigned char *) sfTest);
			if(ret != FLASH_RET_SUCCESS)
			{
				return ret;
			}
			
			ret = drv_OTP_AES_Write(0x00, otp_total_leng, (const unsigned char *) sfTest);
			if(ret != FLASH_RET_SUCCESS)
			{
				return ret;
			}
			//erase 40k encrypt backup memory
			ret = drv_spiflash_erase((cpu_addr + cpu_length - ENCRYPT_SIZE_ALL), ENCRYPT_SIZE_ALL);
			if(ret != FLASH_RET_SUCCESS)
			{
				return ret;
			}
		}
	}

	return FLASH_RET_SUCCESS;
}


void drv_spiflash_restore(void)
{
	chip_clk_enable(CLK_SPI0_APB);
	chip_clk_enable(CLK_SPI0_AHB);

	WRITE_REG(MEM_BASE_SMU_PD + 0x18, SPI0_WORK_CLK_AHB_CLK);	// spi0 work clk choose ahb clk

	PIN_FUNC_SET(IO_MUX_GPIO7, FUNC_GPIO7_MSPI_MOSI);
	PIN_FUNC_SET(IO_MUX_GPIO8, FUNC_GPIO8_MSPI_HOLD);
	PIN_FUNC_SET(IO_MUX_GPIO9, FUNC_GPIO9_MSPI_CLK);
	PIN_FUNC_SET(IO_MUX_GPIO10, FUNC_GPIO10_MSPI_CS0);
	PIN_FUNC_SET(IO_MUX_GPIO11, FUNC_GPIO11_MSPI_MISO);
	PIN_FUNC_SET(IO_MUX_GPIO12, FUNC_GPIO12_MSPI_WP);

	spiFlash_clk_cfg();					/* cfg spi clk */
	spiFlash_xip_read_cfg();			/* cfg xip read-mem-cmd */
	spiFlash_QE_cfg_restore(&flash_dev);

	return;
}


int check_otp_oversize(unsigned int  addr, unsigned int len)
{
	
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);
	unsigned int otp_total_leng;

	if(ops->flash_otp_write == 0)
	{
		return FLASH_RET_ID_ERROR;
	}

	otp_total_leng = param->flash_otp_block_num * param->flash_otp_block_size;

	if ((addr + len) > otp_total_leng)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	return FLASH_RET_SUCCESS;
}


#if 0
int drv_spiflash_OTP_Read(int addr, int length, unsigned char *pdata)
{
	int ret;
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	
	if(p_flash_dev->flash_otp_ops.flash_otp_read == 0)
	{
		return FLASH_RET_ID_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
	}
	
	ret = p_flash_dev->flash_otp_ops.flash_otp_read(p_flash_dev, addr, length, (unsigned char*)pdata);
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	
	return ret;
}



int drv_spiflash_OTP_Write(unsigned int addr, unsigned int len, const unsigned char * buf)
{
	int ret;
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	
	if(p_flash_dev->flash_otp_ops.flash_otp_write == 0)
	{
		return FLASH_RET_ID_ERROR;
	}

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
	}

	ret = p_flash_dev->flash_otp_ops.flash_otp_write(p_flash_dev, addr, len, (unsigned char*)buf);
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B || p_flash_dev->flash_param.flash_id == EN25Q32C  )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	
	return ret;
}


int drv_spiflash_OTP_Erase(unsigned int addr)
{
	int ret;
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;

	if(addr % p_flash_dev->flash_param.flash_otp_block_interval)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}	

	if(p_flash_dev->flash_otp_ops.flash_otp_erase == 0)
	{
		return FLASH_RET_ID_ERROR;
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif

	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
	}

	ret = p_flash_dev->flash_otp_ops.flash_otp_erase(p_flash_dev, addr);

	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	
	return ret;
}


int drv_spiflash_otp_write_mac(unsigned char * mac)
{
	int ret = -1;
	if (mac[0]%2)
	{
		return -1;
	}
	ret = drv_spiflash_OTP_Erase(0x1000);
	if (ret)
	{
		return ret;
	}
	ret = drv_spiflash_OTP_Write(0x1000, 6, (unsigned char *) mac);
	return ret;
}

int drv_spiflash_otp_read_mac(unsigned char * mac)
{
	int ret = -1;

	ret = drv_spiflash_OTP_Read(0x1000,6,(unsigned char *)mac);
	if (mac[0]%2)
	{
		ret = -1;
	}

	return ret;
}

#else



int drv_spiflash_OTP_Read(int addr, int length, unsigned char *pdata)
{	
	unsigned int cycles_num = 0, otp_leng, otp_addr,otp_total_leng;
	unsigned int offset = 0;
	int ret = FLASH_RET_SUCCESS;
	
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);
	
	if(ops->flash_otp_read == 0)
	{
		return FLASH_RET_ID_ERROR;
	}
	
	otp_total_leng = param->flash_otp_block_num * param->flash_otp_block_size;

	if ((addr + length) > otp_total_leng)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}
	
	cycles_num =  addr / param->flash_otp_block_size;
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif
	
	do
	{
		otp_leng = addr % param->flash_otp_block_size;
		otp_leng = MIN(param->flash_otp_block_size - otp_leng, length);
		otp_addr = (addr % param->flash_otp_block_size) + param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval;

		if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B || p_flash_dev->flash_param.flash_id == EN25Q32C )
		{
			spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
		}

		ret = ops->flash_otp_read(p_flash_dev, otp_addr, otp_leng, (unsigned char*)(pdata + offset));
		
		if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B || p_flash_dev->flash_param.flash_id == EN25Q32C )
		{
			spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
		}
		
		if(ret != FLASH_RET_SUCCESS)
		{
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			return ret;
		}
		
		cycles_num++;
		addr += otp_leng;
		length -= otp_leng;
		offset += otp_leng;

	}while(length);
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	
	return ret;
}


static int drv_spiflash_OTP_Write_transform(unsigned int addr, unsigned int len, const unsigned char * buf)
{
	unsigned int cycles_num = 0, otp_leng, otp_addr;
	unsigned int offset = 0;
	int ret = FLASH_RET_SUCCESS;
	
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);
	
	cycles_num =  addr / param->flash_otp_block_size;

	do
	{
		otp_leng = addr % param->flash_otp_block_size;
		otp_leng = MIN(param->flash_otp_block_size - otp_leng, len);
		otp_addr = (addr % param->flash_otp_block_size) + param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval;

		if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C  )
		{
			spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
		}

		ret = ops->flash_otp_write(p_flash_dev, otp_addr, otp_leng, (unsigned char*)(buf + offset));
		
		if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C  )
		{
			spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
		}
		
		if(ret != FLASH_RET_SUCCESS)
		{
			return ret;
		}
		
		cycles_num++;
		addr += otp_leng;
		len -= otp_leng;
		offset += otp_leng;
	}while(len);
	
	return ret;
}


static int drv_spiflash_OTP_Erase_transform(unsigned int addr)
{
	int ret = 0;
	unsigned int cycles_num = 0,otp_addr;
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);

	if(ops->flash_otp_erase == 0)
	{
		return FLASH_RET_ID_ERROR;
	}

	if(addr % param->flash_otp_block_size)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	cycles_num =  addr / param->flash_otp_block_size;
	otp_addr = param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval;
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C  )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
	}
	
	ret = ops->flash_otp_erase(p_flash_dev, otp_addr);
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
	}
	
	return ret;
}



int drv_spiflash_OTP_Write(unsigned int addr, unsigned int len, const unsigned char * buf)
{
	unsigned int cycles_num = 0, otp_leng, otp_addr,otp_total_leng;	
	unsigned int backup_ret = READ_REG(0x202078);
	unsigned int offset = 0;
	
	int ret = FLASH_RET_SUCCESS;
	
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);
	
	if(ops->flash_otp_write == 0)
	{
		return FLASH_RET_ID_ERROR;
	}
	
	otp_total_leng = param->flash_otp_block_num * param->flash_otp_block_size;

	if ((addr + len) > otp_total_leng)
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
#endif

	if(param->flash_otp_block_size == param->flash_otp_block_interval)
	{
		unsigned char * temp_buf = (unsigned char *)os_zalloc(otp_total_leng);
		
		ret = drv_spiflash_OTP_Read(0, otp_total_leng, (unsigned char*)(temp_buf));
		if(ret != FLASH_RET_SUCCESS)
		{
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			os_free(temp_buf);
			return ret;
		}
		
		ret = drv_spiflash_OTP_Erase_transform(0);
		if(ret != FLASH_RET_SUCCESS)
		{
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			os_free(temp_buf);
			return ret;
		}
		
		memcpy((temp_buf + addr), buf, len);

		
		//close
		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, 0x00);
		}
		ret = drv_spiflash_OTP_Write_transform(0, otp_total_leng, (unsigned char*)(temp_buf));

		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, backup_ret);
		}


		if(ret != FLASH_RET_SUCCESS)
		{	
			#ifdef CONFIG_PSM_SURPORT
				psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
			#endif
			os_free(temp_buf);
			return ret;
		}
		
		os_free(temp_buf);
	}
	else
	{
		unsigned char * temp_buf = (unsigned char *)os_zalloc(param->flash_otp_block_size);

		cycles_num =  addr / param->flash_otp_block_size;
		
		do
		{
			otp_leng = addr % param->flash_otp_block_size;
			otp_leng = MIN(param->flash_otp_block_size - otp_leng, len);
			otp_addr = (addr % param->flash_otp_block_size) + param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval;

			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
			}

			ret = ops->flash_otp_read(p_flash_dev, param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval, param->flash_otp_block_size, (unsigned char*)(temp_buf));

			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
			}

			if(ret != FLASH_RET_SUCCESS)
			{
				#ifdef CONFIG_PSM_SURPORT
					psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
				#endif
				os_free(temp_buf);
				return ret;
			}


			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
			}

			ret = ops->flash_otp_erase(p_flash_dev, otp_addr);
			
			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
			}
			
			if(ret != FLASH_RET_SUCCESS)
			{
				#ifdef CONFIG_PSM_SURPORT
					psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
				#endif
				os_free(temp_buf);
				return ret;
			}

			memcpy((temp_buf + (addr % param->flash_otp_block_size)), buf + offset, otp_leng);

			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C  )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
			}
			else
			{
				//close
				if(backup_ret == 1)
				{
					WRITE_REG(0x202078, 0x00);
				}
			}

			ret = ops->flash_otp_write(p_flash_dev, param->flash_otp_star_addr + cycles_num * param->flash_otp_block_interval, param->flash_otp_block_size, (unsigned char*)(temp_buf));

			if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
			{
				spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
			}
			else
			{
				
				if(backup_ret == 1)
				{
					WRITE_REG(0x202078, backup_ret);
				}
			}

			if(ret != FLASH_RET_SUCCESS)
			{
				#ifdef CONFIG_PSM_SURPORT
					psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
				#endif
				os_free(temp_buf);
				return ret;
			}
			
			cycles_num++;
			addr += otp_leng;
			len -= otp_leng;
			offset += otp_leng;
		}while(len);
		os_free(temp_buf);
	}
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	return ret;
}

int drv_spiflash_otp_write_mac(unsigned char * mac)
{
	int ret = -1;
	if (mac[0]%2)
	{
		return -1;
	}
	ret = drv_spiflash_OTP_Write(0, 6, (unsigned char *) mac);
	return ret;
}

int drv_spiflash_otp_read_mac(unsigned char * mac)
{
	int ret = -1;

	ret = drv_spiflash_OTP_Read(0,6,(unsigned char *)mac);
	if (mac[0]%2)
	{
		ret = -1;
	}

	return ret;
}


#endif


//note : parameter[addr] & [length] must be 16 byte alignment
/**    @brief		Read flash data api.
 *	   @details 	Read and decrypt the data of the specified length from the specified address in the flash OTP memory into the buff.
 *     @param[in]	addr--The address where you want to read data in flash.
 *     @param[in]	* buf--Data of specified length read from specified address in flash OTP memory.
 *     @param[in]	len--The specified length read from the specified address in flash OTP memory. 
 *     @return      0--ok,  other--error.
 */
int drv_OTP_AES_Read(int addr, int length, unsigned char *pdata)
{
	if((addr % AES_BYTE_MAX != 0) || (length % AES_BYTE_MAX != 0))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	unsigned int ret = 0, i=0;
	unsigned int offset = 0;
	unsigned char ecb_aeskey[AES_BYTE_MAX] = {0};
	unsigned char cbuf[AES_BYTE_MAX] = {0};
	unsigned char out_buff[AES_BYTE_MAX] = {0};

	ret = drv_spiflash_OTP_Read(addr, length, (unsigned char *)pdata);
	if(ret != FLASH_RET_SUCCESS)
	{
		return ret;
	}

	while(length != offset)
	{
		memcpy(cbuf, &pdata[offset], AES_BYTE_MAX);
		
		for(i = 0; i < AES_BYTE_MAX; i++)
		{
			ecb_aeskey[i] = addr + offset + i;
			//os_printf(LM_APP, LL_INFO, "aeskey[%x]=0x%x \n",(addr + offset + i), ecb_aeskey[i]);
		}
		
		ret = hal_aes_lock();
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		
		ret = hal_aes_ecb_setkey(ecb_aeskey, DRV_AES_KEYBITS_128, DRV_AES_MODE_DEC);
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		
		hal_aes_crypt(cbuf, out_buff);
		
		ret = hal_aes_unlock();
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		
		memcpy((char *)&pdata[offset], out_buff, AES_BYTE_MAX);
		offset += AES_BYTE_MAX;
	}

	return FLASH_RET_SUCCESS;
}


//note : parameter[addr] & [length] must be 16 byte alignment
/**    @brief		Write data to flash OTP memory api.
 *	   @details 	Write the specified length of data encrypted by AES to the specified address into the flash OTP memory.
 *     @param[in]	addr--The address where you want to write data to flash OTP memory.
 *     @param[in]	* buf--The data information that you want to write to the specified address of flash OTP memory.
 *     @param[in]	length--The length of data to be written to the specified address of flash OTP memory.
 *     @return      0--write ok, -5--write enable not set,-6--the memory is busy in write, not write completely.
 */
int drv_OTP_AES_Write(unsigned int addr, unsigned int length, const unsigned char * buf)
{
	if((addr % AES_BYTE_MAX != 0) || (length % AES_BYTE_MAX != 0))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	unsigned int ret = 0, i=0;
	unsigned int offset = 0;
	unsigned char ecb_aeskey[AES_BYTE_MAX] = {0};
	unsigned char cbuf[AES_BYTE_MAX] = {0};
	unsigned char out_buff[AES_BYTE_MAX] = {0};
  
	while(length != offset)
	{
		memcpy(cbuf, &buf[offset], AES_BYTE_MAX);
		
		for(i = 0; i < AES_BYTE_MAX; i++)
		{
			ecb_aeskey[i] = addr + offset + i;
			//os_printf(LM_APP, LL_INFO, "aeskey[%x]=0x%x \n",(addr + offset + i), ecb_aeskey[i]);
		}
		
		ret = hal_aes_lock();
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		ret = hal_aes_ecb_setkey(ecb_aeskey, DRV_AES_KEYBITS_128, DRV_AES_MODE_ENC);
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		
		hal_aes_crypt(cbuf, out_buff);
		
		ret = hal_aes_unlock();
		if(ret != AES_RET_SUCCESS)
		{
			return ret;
		}
		
		memcpy((char *)&buf[offset], out_buff, AES_BYTE_MAX);
		offset += AES_BYTE_MAX;
	}

	ret = drv_spiflash_OTP_Write(addr, length, (unsigned char *)buf);
	if(ret != FLASH_RET_SUCCESS)
	{
		return ret;
	}
		
	return FLASH_RET_SUCCESS;
}



int drv_spiflash_OTP_Lock(unsigned char otp_block)
{
	int ret = 0;
	T_SPI_FLASH_DEV * p_flash_dev = &flash_dev;
	const T_SPI_FLASH_PARAM * param = &(p_flash_dev->flash_param);
	T_SPI_FLASH_OTP_OPS * ops = &(p_flash_dev->flash_otp_ops);

	if(ops->flash_otp_lock == 0)
	{
		return FLASH_RET_ID_ERROR;
	}
	
	if ((otp_block > param->flash_otp_block_num) && (otp_block != 0x0F))
	{
		return FLASH_RET_PARAMETER_ERROR;
	}

	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_ACTIVE);
	#endif
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_ENTRY);
	}

	ret = ops->flash_otp_lock(p_flash_dev, otp_block);
	
	if(p_flash_dev->flash_param.flash_id == EN25QH32B || p_flash_dev->flash_param.flash_id == EN25Q16B  || p_flash_dev->flash_param.flash_id == EN25Q32C )
	{
		spiFlash_format_none(SPIFLASH_CMD_OTP_EXIT);
	}
	
	#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_SPI_FLASH,PSM_DEVICE_STATUS_IDLE);
	#endif
	
	return ret;
}






