/**
* @file       flash.h
* @author     LvHuan
* @date       2021-2-19
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/


/****************************************************************************
* 	                                        Macros
****************************************************************************/
//#ifndef inw
//#define inw(reg)        (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef outw
//#define outw(reg, data) ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif

//#ifndef MIN
//#define MIN(x,y) (x < y ? x : y)
//#endif
//
//#ifndef MAX
//#define MAX(x,y) (x > y ? x : y)
//#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* spi return value */
#define SPI_BUS_STATUS_BUSY				0
#define SPI_FIFO_RESET_COMPLETE			0
#define SPI_RX_EMPTY_STATUS				0
#define SPI_BUS_STATUS_NOBUSY			1
#define SPI_FIFO_RESET_NOCOMPLETE		1
#define SPI_RX_NOEMPTY_STATUS			1

/* spiflash return value */
#define FLASH_RET_SUCCESS				0
#define FLASH_RET_NOT_COMPLETED			1
#define FLASH_RET_SPI_BUS_BUSY			-1
#define FLASH_RET_SPI_FIFO_NOCOMPLETE	-2
#define FLASH_RET_SPI_RX_EMPTY			-3
#define FLASH_RET_PARAMETER_ERROR		-4
#define FLASH_RET_WREN_NOT_SET			-5 
#define FLASH_RET_WIP_NOT_SET			-6
#define FLASH_RET_ID_ERROR				-7
#define FLASH_RET_PROGRAM_ERROR			-8
#define FLASH_RET_READ_ERROR			-8
#define FLASH_RET_NULL_POINT			-9
#define FLASH_RET_PARTITION_ERROR		-10

#define AES_BYTE_MAX 					16

#define APB_WRIT_ENCRYPT				(0x1)
#define APB_WRIT_NOT_ENCRYPT			(0x0)


/* FLASH ID list */
#if 0
//puya
#define MAC_OTP_ADDR_PUYA	0x1000
#define CUS_OTP_ADDR_PUYA	0x2000
//gd
#define MAC_OTP_ADDR_GD		0x0000
#define CUS_OTP_ADDR_GD		0x1000
//xt
#define MAC_OTP_ADDR_XT		0x0000
#define CUS_OTP_ADDR_XT		0x1000
#endif




/****************************************************************************
* 	                                        Function Prototypes
****************************************************************************/

//static void spiFlash_xip_read_mem(unsigned int addr, unsigned char * buf, unsigned int len);
//static int spiFlash_cmd_rdID(void);

void spiFlash_format_wr(int cmd, int data, int wr_len);
unsigned int spiFlash_format_rd(int cmd, int rd_len);
int spiFlash_format_none(int cmd);
void spiFlash_cmd_wrSR_1(int data);
void spiFlash_cmd_wrSR_2(int data);
void spiFlash_cmd_otpmode_wrSR_1(int data);

int spi_bus_ready(void);
int spi_fifo_reset(void);
int spi_rx_ready(void);
unsigned int spiFlash_cmd_rdSR_1(void);
unsigned int spiFlash_cmd_rdSR_2(void);
void spiFlash_cmd_wrEnable(void);

int drv_spiflash_erase(unsigned int addr,  unsigned int len);
int drv_spiflash_write(unsigned int addr, const unsigned char * buf, unsigned int len);
int drv_spiflash_read(unsigned int addr, unsigned char * buf, unsigned int len);
int drv_spiflash_read_apb(unsigned int addr, unsigned char * buf, unsigned int len);
int drv_spiflash_chiperase(void);
int drv_spiflash_OTP_Read(int addr, int length, unsigned char *pdata);
int drv_spiflash_OTP_Write(unsigned int addr, unsigned int len, const unsigned char * buf);
int drv_spiflash_OTP_Erase(unsigned int addr);
int drv_spiflash_OTP_Lock(unsigned char otp_block);
int drv_spiflash_otp_write_mac(unsigned char * mac);
int drv_spiflash_otp_read_mac(unsigned char * mac);
int drv_spiFlash_init(void);
int check_oversize(unsigned int  addr, unsigned int len);
int check_otp_oversize(unsigned int  addr, unsigned int len);
void drv_spiflash_restore(void);
int drv_OTP_AES_Read(int addr, int length, unsigned char *pdata);
int drv_OTP_AES_Write(unsigned int addr, unsigned int length, const unsigned char * buf);
int drv_otp_encrypt_init(void);
void up_IO_driver_strength(void);
int spiflash_read_DMA(unsigned int addr, unsigned char * buf, unsigned int len);
int spiflash_write_DMA(unsigned int addr, const unsigned char * buf, unsigned int len);
int drv_spiflash_read_dma(unsigned int addr, unsigned char * buf, unsigned int len);
int drv_spiflash_write_dma(unsigned int addr, const unsigned char * buf, unsigned int len);

int spiFlash_EDP(void);
int spiFlash_RDP(void);


