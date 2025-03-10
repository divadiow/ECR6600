



#include "chip_memmap.h"
#include "spi.h"



/* spi return value */
#define SPI_BUS_STATUS_BUSY				0
#define SPI_FIFO_RESET_COMPLETE			0
#define SPI_RX_EMPTY_STATUS				0
#define SPI_BUS_STATUS_NOBUSY			1
#define SPI_FIFO_RESET_NOCOMPLETE		1
#define SPI_RX_NOEMPTY_STATUS			1

#define FLASH_SIZE_READ_MAX	0x00000200


#define FLASH_SIZE_500_KB	0x00080000
#define FLASH_SIZE_1_MB		0x00100000
#define FLASH_SIZE_2_MB		0x00200000
#define FLASH_SIZE_4_MB		0x00400000
#define FLASH_SIZE_8_MB		0x00800000

#define FLASH_OTP_START_ADDR_0x0			0x00000000
#define FLASH_OTP_START_ADDR_0x1000			0x00001000
#define FLASH_OTP_START_ADDR_0x1FD000		0x001FD000
#define FLASH_OTP_START_ADDR_0x3FD000		0x003FD000
#define FLASH_OTP_START_ADDR_0x3FF000		0x003FF000



#define FLASH_OTP_BLOCK_SIZE_256_BYTE		0x00000100
#define FLASH_OTP_BLOCK_SIZE_512_BYTE		0x00000200
#define FLASH_OTP_BLOCK_SIZE_1024_BYTE		0x00000400

#define FLASH_OTP_BLOCK_NUM_1		0x1
#define FLASH_OTP_BLOCK_NUM_2		0x2
#define FLASH_OTP_BLOCK_NUM_3		0x3
#define FLASH_OTP_BLOCK_NUM_4		0x4

#define FLASH_OTP_BLOCK_INTERNAL_256_BYTE	0x00000100
#define FLASH_OTP_BLOCK_INTERNAL_1024_BYTE	0x00000400
#define FLASH_OTP_BLOCK_INTERNAL_4096_BYTE	0x00001000


#define FLASH_ID_GD			0xC8
#define FLASH_ID_PUYA		0x85
#define FLASH_ID_ZB			0x5E
#define FLASH_ID_FM			0xA1
#define FLASH_ID_XM			0x20
#define FLASH_ID_XT			0x0B
#define FLASH_ID_W			0xEF
#define FLASH_ID_EN			0x1C
#define FLASH_ID_TH			0xEB
#define FLASH_ID_BOYA		0x68


/* Number of cycles */
#define CHECK_TIMES						1000
#define READY_CHECK_TIMES				20000
#define SPIFLASH_TIMEOUT_COUNT			0xFFFFFFFF

/* SPI-FLASH COMMAND SET  */
#define SPIFLASH_CMD_WREN				0x06
#define SPIFLASH_CMD_RDID				0x9F
#define SPIFLASH_CMD_RDSR_1				0x05
#define SPIFLASH_CMD_RDSR_2				0x35
#define SPIFLASH_CMD_RDSR_CFG			0x15
#define SPIFLASH_CMD_WRSR_1				0x01
#define SPIFLASH_CMD_WRSR_2				0x31

/* Program instructions */
#define SPIFLASH_CMD_PP					0x02
#define SPIFLASH_CMD_QUAD_INPUT_PP		0x32

/* Erase instructions */
#define SPIFLASH_CMD_SE					0x20
#define SPIFLASH_CMD_CE					0xC7

/* Read instructions */
#define SPIFLASH_CMD_READ_NORMAL					0x03
#define SPIFLASH_CMD_READ_FAST						0x0B
#define SPIFLASH_CMD_READ_FAST_DUAL_OUTPUT			0x3B
#define SPIFLASH_CMD_READ_FAST_DUAL_IO				0xBB
#define SPIFLASH_CMD_READ_FAST_QUAD_OUTPUT			0x6B
#define SPIFLASH_CMD_READ_FAST_QUAD_IO				0xEB

/* OTP instructions */
#define SPIFLASH_CMD_OTP_SE 			0x44
#define SPIFLASH_CMD_OTP_PP 			0x42
#define SPIFLASH_CMD_OTP_RD 			0x48
#define SPIFLASH_CMD_OTP_ENTRY			0x3A
#define SPIFLASH_CMD_OTP_EXIT			0x04

/* QPI instructions */
#define SPIFLASH_CMD_ENTER_QPI			0x38
#define SPIFLASH_CMD_EXIT_QPI			0xFF

/* SPI-FLASH STATUS-1 REG */
#define SPIFLASH_STATUS_WIP				0x01
#define SPIFLASH_STATUS_WEL				0x02
#define SPIFLASH_STATUS_BPX				0x7C
#define SPIFLASH_STATUS_ST				0x0000
#define SPIFLASH_STATUS_QE				0x0200

/* SPI-FLASH STATUS-2 REG */
#define SPIFLASH_STATUS_2_ST			0x00
#define SPIFLASH_STATUS_2_QE			0x02



#define FLASH_SPI_REG		((spi_reg *)MEM_BASE_SPI0)

#define FLASH_ARRAY_SIZE(X)		(sizeof(X)/sizeof((X)[0]))

#define SPIFLASH_PAGE_SIZE				0x100	//256B
#define SPIFLASH_SECTOR_SIZE			0x1000 	// 4KB
/* PD_SMU address */
#define CLK_ENABLE_CTRL_REG 			0x20200C
#define APB_ENCRYPT_EN					0x202078

/* Flash memory information */
#define BASE_FLASH_MEM					0x00800000
#define SPIFLASH_MEM_LENGTH				(0x00200000)  //2MB
#define ENCRYPT_BACKUP_BUFF_SIZE 		0x8000
#define ENCRYPT_STATE_SIZE 				0x1000
#define ENCRYPT_SIZE_ALL 				(ENCRYPT_BACKUP_BUFF_SIZE + (2*ENCRYPT_STATE_SIZE))


#define FLASH_REG_XIP_MASK_BIT			0xFFFFFF00
#define SPI0_WORK_CLK_AHB_CLK			0x80

typedef struct flash_encrypt_state
{
	unsigned char magic[8];
	unsigned int cpu_addr;
	unsigned int cpu_len;
	unsigned int nv_addr;
	unsigned int nv_len;
	unsigned int backup_addr;
	
	unsigned int flash_size;
	
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned int encrypt_length;

	unsigned int state;

	unsigned int crc;
}FLASH_ENCRYPT_STATE_T;


typedef struct _T_SPI_FLASH_PARAM
{
	unsigned int flash_id;
	unsigned int flash_size;
	const char * flash_name;
	const char	 Resv4[3];
	unsigned int flash_otp_star_addr;
	unsigned int flash_otp_block_size;
	unsigned int flash_otp_block_num;
	unsigned int flash_otp_block_interval;
} T_SPI_FLASH_PARAM;

 struct _T_SPI_FLASH_DEV;

typedef struct _T_SPI_FLASH_OTP_OPS
{
	int (*flash_otp_read)(struct _T_SPI_FLASH_DEV * p_flash, int addr,int length,unsigned char *pdata);
	int (*flash_otp_erase)(struct _T_SPI_FLASH_DEV * p_flash, unsigned int addr);
	int (*flash_otp_write)(struct _T_SPI_FLASH_DEV * p_flash, unsigned int addr, unsigned int len, unsigned char * buf);
	int (*flash_otp_lock)(struct _T_SPI_FLASH_DEV * p_flash, int lb);
} T_SPI_FLASH_OTP_OPS;


typedef struct _T_SPI_FLASH_DEV
{
	T_SPI_FLASH_PARAM     flash_param;
	T_SPI_FLASH_OTP_OPS  flash_otp_ops;
} T_SPI_FLASH_DEV;



int spiFlash_GD_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_PUYA_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_FM_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_HB_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_XM_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_XT_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_ZB_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_TH_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_EN_probe(T_SPI_FLASH_DEV *p_flash_dev);
int spiFlash_BOYA_probe(T_SPI_FLASH_DEV *p_flash_dev);


int spiFlash_probe(T_SPI_FLASH_DEV *p_flash_dev);



int spi_bus_ready(void);
int spi_fifo_reset(void);
int spi_rx_ready(void);
int spiFlash_format_addr(int cmd, unsigned int addr, unsigned int param);









