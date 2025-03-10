
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "arch_irq.h"
#include "oshal.h"

#include "sdhci.h"
#include "chip_irqvector.h"
#include "chip_pinmux.h"

#include "gpio.h"


//#define RCV_DATA_DEBUG

#define SDHCI_BASE_CLOCK 48000000
#define SDHCI_WORK_CLOCK (48*1000*1000)

#define SDCARD_DETECT_GPIO	(20)
#define SDCARD_DETECT

/*
 *SDHCI Controller registers
 */

#define SDHCI_BASE			0x00450000

#define SDHCI_DMA_ADDRESS	(SDHCI_BASE + 0x00)

#define SDHCI_BLOCK_SIZE	(SDHCI_BASE + 0x04)
#define  SDHCI_MAKE_BLKSZ(dma, blksz) (((dma & 0x7) << 12) | (blksz & 0xFFF))

#define SDHCI_BLOCK_COUNT	(SDHCI_BASE + 0x06)

#define SDHCI_ARGUMENT		(SDHCI_BASE + 0x08)

#define SDHCI_TRANSFER_MODE	(SDHCI_BASE + 0x0C)
#define  SDHCI_TRNS_DMA		0x01
#define  SDHCI_TRNS_BLK_CNT_EN	0x02
#define  SDHCI_TRNS_ACMD12	0x04
#define  SDHCI_TRNS_READ	0x10
#define  SDHCI_TRNS_MULTI	0x20

#define SDHCI_COMMAND		(SDHCI_BASE + 0x0E)
#define  SDHCI_CMD_RESP_MASK	0x03
#define  SDHCI_CMD_CRC		0x08
#define  SDHCI_CMD_INDEX	0x10
#define  SDHCI_CMD_DATA		0x20

#define  SDHCI_CMD_RESP_NONE	0x00
#define  SDHCI_CMD_RESP_LONG	0x01
#define  SDHCI_CMD_RESP_SHORT	0x02
#define  SDHCI_CMD_RESP_SHORT_BUSY 0x03

#define SDHCI_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))

#define SDHCI_RESPONSE		(SDHCI_BASE + 0x10)

#define SDHCI_BUFFER		(SDHCI_BASE + 0x20)

#define SDHCI_PRESENT_STATE	(SDHCI_BASE + 0x24)
#define  SDHCI_CMD_INHIBIT	0x00000001
#define  SDHCI_DATA_INHIBIT	0x00000002
#define  SDHCI_DOING_WRITE	0x00000100
#define  SDHCI_DOING_READ	0x00000200
#define  SDHCI_SPACE_AVAILABLE	0x00000400
#define  SDHCI_DATA_AVAILABLE	0x00000800
#define  SDHCI_CARD_PRESENT	0x00010000
#define  SDHCI_WRITE_PROTECT	0x00080000

#define SDHCI_HOST_CONTROL 	(SDHCI_BASE + 0x28)
#define  SDHCI_CTRL_LED		0x01
#define  SDHCI_CTRL_4BITBUS	0x02
#define  SDHCI_CTRL_HISPD	0x04
#define  SDHCI_CTRL_DMA_MASK	0x18
#define   SDHCI_CTRL_SDMA	0x00
#define   SDHCI_CTRL_ADMA1	0x08
#define   SDHCI_CTRL_ADMA32	0x10
#define   SDHCI_CTRL_ADMA64	0x18

#define SDHCI_POWER_CONTROL	(SDHCI_BASE + 0x29)
#define  SDHCI_POWER_ON		0x01
#define  SDHCI_POWER_180	0x0A
#define  SDHCI_POWER_300	0x0C
#define  SDHCI_POWER_330	0x0E

#define SDHCI_BLOCK_GAP_CONTROL	(SDHCI_BASE + 0x2A)

#define SDHCI_WAKE_UP_CONTROL	(SDHCI_BASE + 0x2B)

#define SDHCI_CLOCK_CONTROL	(SDHCI_BASE + 0x2C)
#define  SDHCI_DIVIDER_SHIFT	8
#define  SDHCI_CLOCK_CARD_EN	0x0004
#define  SDHCI_CLOCK_INT_STABLE	0x0002
#define  SDHCI_CLOCK_INT_EN	0x0001

#define SDHCI_TIMEOUT_CONTROL	(SDHCI_BASE + 0x2E)

#define SDHCI_SOFTWARE_RESET	(SDHCI_BASE + 0x2F)
#define  SDHCI_RESET_ALL	0x01
#define  SDHCI_RESET_CMD	0x02
#define  SDHCI_RESET_DATA	0x04

#define SDHCI_INT_STATUS	(SDHCI_BASE + 0x30)
#define SDHCI_ERR_STATUS	(SDHCI_BASE + 0x32)

#define SDHCI_INT_ENABLE	(SDHCI_BASE + 0x34)
#define SDHCI_SIGNAL_ENABLE	(SDHCI_BASE + 0x38)
#define  SDHCI_INT_RESPONSE	0x00000001
#define  SDHCI_INT_DATA_END	0x00000002
#define  SDHCI_INT_DMA_END	0x00000008
#define  SDHCI_INT_SPACE_AVAIL	0x00000010
#define  SDHCI_INT_DATA_AVAIL	0x00000020
#define  SDHCI_INT_CARD_INSERT	0x00000040
#define  SDHCI_INT_CARD_REMOVE	0x00000080
#define  SDHCI_INT_CARD_INT	0x00000100
#define  SDHCI_INT_ERROR	0x00008000
#define  SDHCI_INT_TIMEOUT	0x00010000
#define  SDHCI_INT_CRC		0x00020000
#define  SDHCI_INT_END_BIT	0x00040000
#define  SDHCI_INT_INDEX	0x00080000
#define  SDHCI_INT_DATA_TIMEOUT	0x00100000
#define  SDHCI_INT_DATA_CRC	0x00200000
#define  SDHCI_INT_DATA_END_BIT	0x00400000
#define  SDHCI_INT_BUS_POWER	0x00800000
#define  SDHCI_INT_ACMD12ERR	0x01000000
#define  SDHCI_INT_ADMA_ERROR	0x02000000
#define   SDHCI_TUNING_EVENT    0x00001000 //added SD3.0
#define   SDHCI_TUNING_ERROR    0x08000000

#define  SDHCI_INT_NORMAL_MASK	0x00007FFF
#define  SDHCI_INT_ERROR_MASK	0xFFFF8000

#define  SDHCI_INT_CMD_MASK	(SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define  SDHCI_INT_DATA_MASK	(SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL | \
		SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
		SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK	((unsigned int)-1)

#define SDHCI_ACMD12_ERR	(SDHCI_BASE + 0x3C)

#define SDHCI_HOST_CONTROL2     (SDHCI_BASE + 0x3E)
/* 3E-3F reserved */

#define SDHCI_CAPABILITIES	(SDHCI_BASE + 0x40)
#define  SDHCI_TIMEOUT_CLK_MASK	0x0000003F
#define  SDHCI_TIMEOUT_CLK_SHIFT 0
#define  SDHCI_TIMEOUT_CLK_UNIT	0x00000080
#define  SDHCI_CLOCK_BASE_MASK	0x00003F00
#define  SDHCI_CLOCK_BASE_3     0x0000FF00
#define  SDHCI_CLOCK_BASE_SHIFT	8
#define  SDHCI_MAX_BLOCK_MASK	0x00030000
#define  SDHCI_MAX_BLOCK_SHIFT  16
#define  SDHCI_CAN_DO_ADMA2	0x00080000
#define  SDHCI_CAN_DO_ADMA1	0x00100000
#define  SDHCI_CAN_DO_HISPD	0x00200000
#define  SDHCI_CAN_DO_SDMA	0x00400000
#define  SDHCI_CAN_VDD_330	0x01000000
#define  SDHCI_CAN_VDD_300	0x02000000
#define  SDHCI_CAN_VDD_180	0x04000000
#define  SDHCI_CAN_64BIT	0x10000000

//eMMMC fields

#define eMMC_BOOT_ACK_INT      0x00002000
#define eMMC_BOOT_DONE_INT     0x00004000
//#define eMMC_BOOT_ACK_INT      0x00002000




/* 44-47 reserved for more caps */

#define SDHCI_MAX_CURRENT	(SDHCI_BASE + 0x48)

/* 4C-4F reserved for more max current */

#define  SDHCI_SET_ACMD12_ERROR	0x50
#define  SDHCI_SET_INT_ERROR	0x52

#define  SDHCI_ADMA_ERROR	(SDHCI_BASE + 0x54)

/* 55-57 reserved */

#define  SDHCI_ADMA_ADDRESS	(SDHCI_BASE + 0x58)

/* 60-FB reserved */

#define  SDHCI_SLOT_INT_STATUS	(SDHCI_BASE + 0xFC)

#define  SDHCI_HOST_VERSION	(SDHCI_BASE + 0xFE)
#define  SDHCI_VENDOR_VER_MASK	0xFF00
#define  SDHCI_VENDOR_VER_SHIFT	8
#define  SDHCI_SPEC_VER_MASK	0x00FF
#define  SDHCI_SPEC_VER_SHIFT	0
#define  SDHCI_SPEC_100	0
#define  SDHCI_SPEC_200	1
#define  SDHCI_SPEC_300	2


//power config
#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */


//bus width
#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const unsigned int __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		unsigned int __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})


static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};
	

#define WRITE_REG_8BIT(offset,value) \
	{ \
		unsigned int REG_MACRO_FLAG=portSET_INTERRUPT_MASK_FROM_ISR(); \
		(*(volatile unsigned char *)(offset) = (unsigned char)(value)); \
		portCLEAR_INTERRUPT_MASK_FROM_ISR(REG_MACRO_FLAG); \
	}
#define WRITE_REG_16BIT(offset,value) \
	{ \
		unsigned int REG_MACRO_FLAG=portSET_INTERRUPT_MASK_FROM_ISR(); \
		(*(volatile unsigned short *)(offset) = (unsigned short)(value)); \
		portCLEAR_INTERRUPT_MASK_FROM_ISR(REG_MACRO_FLAG); \
	}

#define IN8(reg)				(char)( READ_REG_MASK(reg,0x000000FF,0) )
#define IN16(reg)				(short int)( READ_REG_MASK(reg,0x0000FFFF,0) )
//
#define OUT8(reg, data)			WRITE_REG_8BIT(reg,data)
#define OUT16(reg, data)		WRITE_REG_16BIT(reg,data)

//+++++++++++++++++++++++++++++++++++++
//struct






struct mmc_cid {
	unsigned int		manfid;
	char			prod_name[8];
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
};

struct mmc_csd {
	unsigned char		mmca_vsn;
	unsigned short		cmdclass;
	unsigned short		tacc_clks;
	unsigned int		tacc_ns;
	unsigned int		r2w_factor;
	unsigned int		max_dtr;
	unsigned int		read_blkbits;
	unsigned int		write_blkbits;
	unsigned int		capacity;
	unsigned int		read_partial:1,
				read_misalign:1,
				write_partial:1,
				write_misalign:1;
    unsigned int            card_type;
};

struct mmc_request {
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_command	*stop;

	void			*done_data;	/* completion data */
	void			(*done)(struct mmc_request *);/* completion function */
};




struct sdcard_info{
	unsigned int	 ccs;
	unsigned int 	s18a;
	unsigned int 	sd_cap;
	unsigned int 	cid[4];
	unsigned int 	rca;
	unsigned int	csd[4];
	struct mmc_csd	de_csd;
	struct mmc_cid	de_cid;
	char			sdcard_inserted;
};

struct sdhci {
	//host
	unsigned int		version;	/* SDHCI spec. version */
	unsigned int		caps;		//SDHCI capability
	unsigned int		max_clk;	/* Max possible freq (MHz) */
	unsigned int		timeout_clk;	/* Timeout freq (KHz) */
	unsigned int		clock;		/* Current clock (MHz) */
	char				pwr;		/* Current voltage */
	char				host_ready;
	//sd card
	struct sdcard_info  sdcard;

	//option
	struct mmc_command	cmd;		/* Current command */
	struct mmc_data		data;		/* Current data request */
	int 				cmd_err;
};



struct sdhci sdhci_host;

#ifdef SDCARD_TEST
static char sd_write_data[SDHCI_MAX_BUF];
static char sd_read_data[SDHCI_MAX_BUF];
static int sd_test_data_size=SDHCI_MAX_BUF;
static char *sd_test_data=(char *)0xa0000;
#endif


//static ADMA2 *sd_adma_desc=(ADMA2 *)0xaf000;
static ADMA2 sd_adma_desc[MAX_BUF_BLOCK]  __attribute__((section(".dma.data"),aligned(4)));


volatile unsigned int cmd_busy=0;
volatile unsigned int data_busy=0;
volatile unsigned int completed_per=1;





static int sdhci_pio_transfer(struct mmc_data *data,struct mmc_command *cmd,unsigned int intmask);
static int sdhci_dma_transfer(struct mmc_data *data,struct mmc_command *cmd,unsigned int intmask);
static int sdhci_command_complete(struct mmc_data *data,struct mmc_command *cmd);
static int sdhci_data_irq(struct mmc_data *data, struct mmc_command *cmd, unsigned int intmask);
//int sdhci_send_command(struct mmc_data *data, struct mmc_command *cmd);
//static void sdhci_dumpregs(void);
//void sdhci_test_go_idle();
static int sdhci_send_if_cond(unsigned int ocr);
static int mmc_send_app_op_cond(unsigned int ocr);
static int mmc_all_send_cid(unsigned int *cid);
static int mmc_send_relative_addr(unsigned int *rca);
static int mmc_all_send_csd(unsigned int *csd);


/*
static void sdhci_dumpregs(void)
{
	os_printf(LM_OS, LL_INFO,   ": ============== REGISTER DUMP ==============\n");
	os_printf(LM_OS, LL_INFO,   ": COMMAND Register: 0x%x\n",
			IN16(SDHCI_COMMAND));

	os_printf(LM_OS, LL_INFO,   ": Host Control 2 Register: 0x%x\n",
			IN16(SDHCI_HOST_CONTROL2));
	os_printf(LM_OS, LL_INFO,   ": Sys addr: 0x%x | Version:  0x%x\n",
			IN32(SDHCI_DMA_ADDRESS),
			IN16(SDHCI_HOST_VERSION));
	os_printf(LM_OS, LL_INFO,   ": Blk size: 0x%x | Blk cnt:  0x%x\n",
			IN16(SDHCI_BLOCK_SIZE),
			IN16(SDHCI_BLOCK_COUNT));
	os_printf(LM_OS, LL_INFO,   ": Argument: 0x%x | Trn mode: 0x%x\n",
			IN32(SDHCI_ARGUMENT),
			IN16(SDHCI_TRANSFER_MODE));
	os_printf(LM_OS, LL_INFO,   ": Present:  0x%x | Host ctl: 0x%x\n",
			IN32(SDHCI_PRESENT_STATE),
			IN8(SDHCI_HOST_CONTROL));
	os_printf(LM_OS, LL_INFO,   ": Power:    0x%x | Blk gap:  0x%x\n",
			IN8(SDHCI_POWER_CONTROL),
			IN8(SDHCI_BLOCK_GAP_CONTROL));
	os_printf(LM_OS, LL_INFO,   ": Wake-up:  0x%x | Clock:    0x%x\n",
			IN8(SDHCI_WAKE_UP_CONTROL),
			IN16(SDHCI_CLOCK_CONTROL));
	os_printf(LM_OS, LL_INFO,   ": Timeout:  0x%x | Int stat: 0x%x\n",
			IN8(SDHCI_TIMEOUT_CONTROL),
			IN32(SDHCI_INT_STATUS));
	os_printf(LM_OS, LL_INFO,   ": Int enab: 0x%x | Sig enab: 0x%x\n",
			IN32(SDHCI_INT_ENABLE),
			IN32(SDHCI_SIGNAL_ENABLE));
	os_printf(LM_OS, LL_INFO,   ": AC12 err: 0x%x | Slot int: 0x%x\n",
			IN16(SDHCI_ACMD12_ERR),
			IN16(SDHCI_SLOT_INT_STATUS));
	os_printf(LM_OS, LL_INFO,   ": Caps:     0x%x | Max curr: 0x%x\n",
			IN32(SDHCI_CAPABILITIES),
			IN32(SDHCI_MAX_CURRENT));

	os_printf(LM_OS, LL_INFO,   ": ADMA Err: 0x%x | ADMA Ptr: 0x%x\n",
			IN32(SDHCI_ADMA_ERROR),
			IN32(SDHCI_ADMA_ADDRESS));

	os_printf(LM_OS, LL_INFO,   ": ===========================================\n");
}
*/




static void sdhci_clear_set_irq(unsigned int clear,unsigned int set)
{
	unsigned int ier;
	ier = READ_REG(SDHCI_INT_ENABLE);
	ier &= ~clear;
	ier |= set;
	WRITE_REG(SDHCI_INT_ENABLE, ier);
	WRITE_REG(SDHCI_SIGNAL_ENABLE, ier);
}


static void sdhci_abort_cmd()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=12;
	sdhci_host.cmd.arg = 0;
	//sdhci_host.cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B| MMC_CMD_AC;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;//no wait for cmd resp
	sdhci_send_command(NULL, &sdhci_host.cmd);
}
static int sdhci_err_int()
{
	unsigned short int err_int;
	err_int = IN16(SDHCI_ERR_STATUS);
	int op_timer = 200;

	if(err_int & 0xf)	//cmd line error occurse
		{
		os_printf(LM_OS, LL_INFO, "cmd line error occurse\r\n");
		OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_CMD);
		cmd_busy = 0;
		sdhci_host.cmd_err = -1;
		op_timer = 200;
		while (IN8(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_CMD)
			{
			//wait for cr ready
			op_timer--;
			if (op_timer == 0)
				{
				return -1;
				}
			}
		}
	if(err_int & 0x70)
		{
		os_printf(LM_OS, LL_INFO, "data line error occurse\r\n");
		OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_DATA);
		data_busy = 0;
		sdhci_host.cmd_err = -2;
		op_timer = 200;
		while (IN8(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_DATA)
			{
			//wait for dr ready
			op_timer--;
			if (op_timer == 0)
				{
				return -1;
				}
			}
		}
	
	OUT16(SDHCI_ERR_STATUS, IN16(SDHCI_ERR_STATUS));//clear error status

	//abort cmd
	sdhci_abort_cmd();
	OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_ALL);

	//check cmd data inhibit
	op_timer = 200;
	while (READ_REG(SDHCI_PRESENT_STATE) & (SDHCI_CMD_INHIBIT|SDHCI_DATA_INHIBIT))
		{
		//wait for cmd data status
		op_timer--;
		if (op_timer == 0)
			{
			return -1;
			}
		}
	
	err_int = IN16(SDHCI_ERR_STATUS);
	if(err_int & 0xf)	//cmd line error occurse
		{
		os_printf(LM_OS, LL_INFO, "cmd line error occurse (CMD12)\r\n");
		return -1;
		}
	if(err_int & SDHCI_INT_DATA_TIMEOUT)
		{
		os_printf(LM_OS, LL_INFO, "timeout of data line (CMD12)\r\n");
		return -1;
		}

	os_printf(LM_OS, LL_INFO, "wait for more than 40us\r\n");
	

	if((READ_REG(SDHCI_PRESENT_STATE) & 0xF00000) != 0xF00000)
		return -1;
	else
		return 0;
	
}

void sdhci_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	unsigned short int intmask=0;
	intmask = IN16(SDHCI_INT_STATUS);
	arch_irq_clean(VECTOR_NUM_SDIO_HOST);

	if(intmask & SDHCI_INT_ERROR)
		{
		os_printf(LM_OS, LL_INFO, "!!!err interrupt occurs= %x,cmd num:%d\r\n",IN16(SDHCI_ERR_STATUS),sdhci_host.cmd.opcode);
		//disable error interrupt signal
		sdhci_clear_set_irq(SDHCI_INT_ERROR_MASK,0);
		
		if(sdhci_err_int(intmask))
			os_printf(LM_OS, LL_INFO, "Non recoverable error\r\n");
		else
			os_printf(LM_OS, LL_INFO, "error recovery\r\n");

		
		//enable error interrupt signal
		sdhci_clear_set_irq(0,SDHCI_INT_ERROR_MASK);
		}
	if (intmask & (SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE  )) 
		{
		OUT16(SDHCI_INT_STATUS, intmask & (SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE ));
		os_printf(LM_OS, LL_INFO, "sdcard insert or remove:%x\r\n",intmask);
		}
	if (intmask & SDHCI_INT_CMD_MASK) 
		{
		OUT16(SDHCI_INT_STATUS, intmask & SDHCI_INT_CMD_MASK);
		if(intmask & SDHCI_INT_RESPONSE)
			{
			sdhci_command_complete(&sdhci_host.data,&sdhci_host.cmd);
			}
		else
			{
			os_printf(LM_OS, LL_INFO, "some error occurs for cmd %x\r\n",intmask);
			}
		}
	if (intmask & SDHCI_INT_DATA_MASK) 
		{
		OUT16(SDHCI_INT_STATUS, intmask & SDHCI_INT_DATA_MASK);
		sdhci_data_irq(&sdhci_host.data,&sdhci_host.cmd,intmask);
		}

	if(intmask & SDHCI_INT_CARD_INT)
		{
		sdhci_clear_set_irq(SDHCI_INT_CARD_INT,0);
		}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

static int sdhci_data_irq(struct mmc_data *data, struct mmc_command *cmd, unsigned int intmask)
{
	if(data->transtype == PIO)
		{
		sdhci_pio_transfer(data,cmd,intmask);
		}
	else
		{
		sdhci_dma_transfer(data,cmd,intmask);
		}
	data_busy=0;
	return 0;
}


static void sdhci_set_clock(unsigned int clock)
{
	int div,div1;
	int version;
	unsigned short int clk = 0;
	//unsigned int caps = 0;
	unsigned long timeout=80000;
	OUT16(SDHCI_CLOCK_CONTROL, 0);

	//get caps
	//caps = IN32(SDHCI_CAPABILITIES);
	
	//read sdio ip version
	version = IN8(SDHCI_HOST_VERSION);
	
	if((version==0x00)||(version==0x01))
	{
		for (div = 1;div < 256;div *= 2) {
			if ((SDHCI_BASE_CLOCK / div) <= clock)
				break;
		}
		div >>= 1;
		clk = div << SDHCI_DIVIDER_SHIFT;
		clk |= SDHCI_CLOCK_INT_EN;
	}

	else
	{
		if(version==0x02)
		{
			for (div = 1;div <=2046;div *= 2) {
				if ((SDHCI_BASE_CLOCK / div) <= clock)
					break;
			}
			div >>= 1;
			//div = div << 0x5;
			div1 = div>> 0x08;
			//div = div & 0xFF3F;
			div=(div<<8)&0xFFFF;
			clk = div |(div1<<6);
			clk |= SDHCI_CLOCK_INT_EN;
		}
	}
	OUT16(SDHCI_CLOCK_CONTROL, clk);

	while (!((clk = IN16(SDHCI_CLOCK_CONTROL))& SDHCI_CLOCK_INT_STABLE)) 
		{
		if (timeout == 0) 
			{
			os_printf(LM_OS, LL_INFO, "\r\n***** Internal clock never stabilised *****\r\n");
			return;
			}
		timeout--;
		}

	clk |= SDHCI_CLOCK_CARD_EN;
	OUT16(SDHCI_CLOCK_CONTROL, clk);
}

static void sdhci_set_power(unsigned short power)
{
	char pwr;
	switch (1<<power)
		{
		case MMC_VDD_165_195:
			pwr = SDHCI_POWER_180;
			break;
		case MMC_VDD_29_30:
		case MMC_VDD_30_31:
			pwr = SDHCI_POWER_300;
			break;
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			pwr = SDHCI_POWER_330;
			break;
		default:
			os_printf(LM_OS, LL_INFO, "\r\n***** Can not set power *****\r\n");
		}
	OUT8(SDHCI_POWER_CONTROL,pwr);
	OUT8(SDHCI_POWER_CONTROL,pwr|SDHCI_POWER_ON);
}

static void sdhci_set_ctrl(unsigned char bus_width)
{
	char ctrl;
	ctrl=IN8(SDHCI_HOST_CONTROL);

	if(bus_width == MMC_BUS_WIDTH_4)
		ctrl |= SDHCI_CTRL_4BITBUS;
	else
		ctrl &= ~SDHCI_CTRL_4BITBUS;

	//ctrl |= SDHCI_CTRL_HISPD;
	//os_printf(LM_OS, LL_INFO, "\r\n set ctrl:%x *****\r\n",ctrl);
	OUT8(SDHCI_HOST_CONTROL, ctrl);

}

void sdhci_init(void)
{
	WRITE_REG(0x20200c, READ_REG(0x20200c)|1<<25);//clock enable
	WRITE_REG(0x202010, READ_REG(0x202010)|1<<11);//low power clock enable
	WRITE_REG(0x202014, READ_REG(0x202014)|1<<2);//system software reset
	WRITE_REG(0x202074, READ_REG(0x202074)|1<<3);//div5 enable

	//PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_GPIO20);

	//reset
	OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_ALL);

	sdhci_set_clock(400*1000);//init clock,400KHZ

	//set sdcard power
	sdhci_set_power(21); //3.3v to be modified

	//card status detect mode	0x80:DAT3
	//hardware not supported
	//OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)|0x80);

	sdhci_set_ctrl(MMC_BUS_WIDTH_1); // set 1 bit width
	
	arch_irq_clean(VECTOR_NUM_SDIO_HOST);
	arch_irq_unmask(VECTOR_NUM_SDIO_HOST);
	arch_irq_register(VECTOR_NUM_SDIO_HOST, sdhci_isr);


	sdhci_clear_set_irq(SDHCI_INT_ALL_MASK,
		    SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
			SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
			//SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_INT |SDHCI_INT_CARD_REMOVE|
			SDHCI_INT_DATA_END | 
			SDHCI_INT_RESPONSE| SDHCI_TUNING_EVENT | SDHCI_TUNING_ERROR// |
			//eMMC_BOOT_ACK_INT | eMMC_BOOT_DONE_INT
			);

	memset(&sdhci_host, 0,sizeof(sdhci_host));
	sdhci_host.host_ready = 1;
	
	#ifdef SDCARD_DETECT
	sdhci_card_detect();
	#else
	sdhci_host.sdcard.sdcard_inserted = 1;
	#endif
}

void sdhci_deinit(void)
{
	sdhci_host.host_ready = 0;
	
	sdhci_clear_set_irq(SDHCI_INT_ALL_MASK,0);	
	arch_irq_clean(VECTOR_NUM_SDIO_HOST);
	arch_irq_mask(VECTOR_NUM_SDIO_HOST);
	arch_irq_register(VECTOR_NUM_SDIO_HOST, NULL);

	//reset
	OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_ALL);

	sdhci_set_ctrl(MMC_BUS_WIDTH_1); // set 1 bit width

	//set sdcard power
	//sdhci_set_power(21); //3.3v to be modified

	sdhci_set_clock(400*1000);//init clock,400KHZ

	WRITE_REG(0x20200c, READ_REG(0x20200c)&(~(1<<25)));//clock disable
	WRITE_REG(0x202010, READ_REG(0x202010)&(~(1<<11)));//low power clock disable
	WRITE_REG(0x202014, READ_REG(0x202014)|1<<2);//system software reset
	WRITE_REG(0x202074, READ_REG(0x202074)&(~(1<<3)));//div5 disable

}


static void sdhci_adma_table_pre(struct mmc_data *data, struct mmc_command *cmd)
{
	int i;
	ADMA2 *AdmaDes;
	AdmaDes = sd_adma_desc;
	//maybe blocksize is bigger than buff,sometimes we need split a block.
	for(i=0;i<data->blocks;i++)
		{
		AdmaDes[i].dwAddress = (char *)data->buff_addr + i*data->blksz;
		AdmaDes[i].wLen = data->blksz;
		if (i == data->blocks-1)
			{
			AdmaDes[i].wAttribute = 0x23;//end desc attribute
			}
		else
			AdmaDes[i].wAttribute = 0x21;
		}
	WRITE_REG(SDHCI_ADMA_ADDRESS, AdmaDes);
}

static int sdhci_command_complete(struct mmc_data *data,struct mmc_command *cmd)
{
	int i;

	//get response data
	if (cmd->flags & MMC_RSP_PRESENT) 
		{
		if(IN16(SDHCI_ERR_STATUS) & (1<<0))
			{
			os_printf(LM_OS, LL_INFO, "wait response timeout!!![%x]\r\n",cmd->opcode);
			return -2;
			}

		
		if (cmd->flags & MMC_RSP_136)
			{
			for (i = 0;i < 4;i++)
				{
				cmd->resp[i] = READ_REG(SDHCI_RESPONSE + (3-i)*4) << 8;
				if (i != 3)
					cmd->resp[i] |= (READ_REG(SDHCI_RESPONSE + (2-i)*4) >> 24);
				}
			}
		else
			{
			cmd->resp[0] = READ_REG(SDHCI_RESPONSE);
			}
		
		}

	cmd_busy=0;//command finished.

	return 0;
}


static void sdhci_set_transfer_irqs(struct mmc_data *data)
{
	
	unsigned int pio_irqs = SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL;
	unsigned int dma_irqs = SDHCI_INT_DMA_END | SDHCI_INT_ADMA_ERROR;

	if (data->transtype == SDMA || data->transtype == ADMA)
		sdhci_clear_set_irq(pio_irqs, dma_irqs);
	else
		sdhci_clear_set_irq(dma_irqs, pio_irqs);
}

int sdhci_send_command(struct mmc_data *data, struct mmc_command *cmd)
{
	unsigned short int mode=0;
	unsigned int flags=0;
	unsigned short int ctrl=0;
	int ret=0;
	int op_timer=200;
	
	cmd_busy =1;//command start.
	op_timer=200;
	while (READ_REG(SDHCI_PRESENT_STATE) & 1<<0 )
		{
		op_timer--;
		if(op_timer == 0)
			return -1;
		}
	op_timer=200;
	while (READ_REG(SDHCI_PRESENT_STATE) & 1<<1 )
		{
		op_timer--;
		if(op_timer == 0)
			return -1;
		}
	if(NULL != data)
		{
		data_busy=1;//data trans start.
		ctrl = IN8(SDHCI_HOST_CONTROL);
			
		if(SDMA == data->transtype)
			{
			//os_printf(LM_OS, LL_INFO, "----buff_addr-----:%x\r\n",data->buff_addr);
			WRITE_REG(SDHCI_DMA_ADDRESS, data->buff_addr);

			ctrl &= 0xE7;//clear DMA select
			ctrl |= SDHCI_CTRL_SDMA;
			}
		else if (ADMA == data->transtype)
			{
			sdhci_adma_table_pre(data,cmd);
			ctrl &= 0xE7;//clear DMA select
			ctrl |= SDHCI_CTRL_ADMA32;
			}
		//set host control
		OUT8(SDHCI_HOST_CONTROL, ctrl);

		//set data timeout
		OUT8(SDHCI_TIMEOUT_CONTROL, 0xe);

		//set transfer irqs
		sdhci_set_transfer_irqs(data);

		op_timer=200;
		while (!(READ_REG(SDHCI_PRESENT_STATE) & 0x0f00000))
			{
			op_timer--;
			if(op_timer == 0)
				return -1;
			}

		OUT16(SDHCI_BLOCK_SIZE, (data->blksz));
		OUT16(SDHCI_BLOCK_COUNT, data->blocks);
		
		}

	//set argument 
	WRITE_REG(SDHCI_ARGUMENT, cmd->arg);

	if(NULL != data)
		{
		//set transfer mode
		mode = SDHCI_TRNS_BLK_CNT_EN | mode;
		if (data->blocks > 1)
			{
			mode |= SDHCI_TRNS_MULTI;
			mode |= SDHCI_TRNS_ACMD12; //enable auto cmd12
			}
		if (data->flags & MMC_DATA_READ)
			mode |= SDHCI_TRNS_READ;
		if (data->transtype == SDMA || data->transtype == ADMA)
			{
			mode |= SDHCI_TRNS_DMA;
			}
		
		
		OUT16(SDHCI_TRANSFER_MODE, mode);
		}

	//set flags
	if (!(cmd->flags & MMC_RSP_PRESENT))
		flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->flags & MMC_RSP_136)
		flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->flags & MMC_RSP_BUSY)
		flags = SDHCI_CMD_RESP_SHORT_BUSY;
	else
		flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->flags & MMC_RSP_CRC)
		flags |= SDHCI_CMD_CRC;
	if (cmd->flags & MMC_RSP_OPCODE)
		flags |= SDHCI_CMD_INDEX;
	if (cmd->data != NULL)
		flags |= SDHCI_CMD_DATA;

	if(cmd->opcode == 12)
		flags |=0xc0;
	else
		flags &= (~(0xc0));
		
	//commamd issuse	
	OUT16(SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd->opcode, flags));


	//command 
	if((flags & SDHCI_CMD_RESP_MASK))//if command wait resp
		{
		while (cmd_busy)//wait cmd complete
			{
			}

		if (cmd->data != NULL)
			{
			while (data_busy)//wait data complete
				{
				}
			}
		ret = sdhci_host.cmd_err;
		sdhci_host.cmd_err = 0;
		}
		else
		{
		//no need wait response
		cmd_busy = 0;
		}

	return ret;
}


static int sdhci_dma_transfer(struct mmc_data *data,struct mmc_command *cmd,unsigned int intmask)
{
	//sdhci_prepare_data(data,cmd);
#ifdef RCV_DATA_DEBUG	
	int i;
#endif

	#if LOOP_TEST
	char *read_c;
	#endif

	if(intmask & (1<<3))
		{
		WRITE_REG(SDHCI_DMA_ADDRESS,READ_REG(SDHCI_DMA_ADDRESS)+4096*(1<<(IN16(SDHCI_BLOCK_SIZE)>>12 & 0x7)));
		}
	if(intmask&(1<<1))
			{
			//transfer complete
			//only for read test
			#if LOOP_TEST
			if(data->flags & MMC_DATA_READ) //data read
				{
				read_c = (char *)data->buff_addr;
				memcpy(sd_read_data,read_c,(data->blksz)*(data->blocks));
#ifdef RCV_DATA_DEBUG				
				os_printf(LM_OS, LL_INFO, "[read buffer start] addr:0x%x\r\n",data->buff_addr);
				for(i=0;i<(data->blksz*data->blocks);i++)
					{
					os_printf(LM_OS, LL_INFO, "%x ",sd_read_data[i]&0xff);
					//sd_read_data[i]=read_c[i]&0xff;
					}
				os_printf(LM_OS, LL_INFO, "\r\n [read buffer end]\r\n");
#endif
				}
			#endif

			}
	else
		{
		os_printf(LM_OS, LL_INFO, "transfer miss complete int:%x\r\n");
		}
	return 0;
}


int sdhci_pio_transfer(struct mmc_data *data,struct mmc_command *cmd,unsigned int intmask)
{
	//unsigned int mask;
	//sdhci_prepare_data(data,cmd);
	int k,i;
#ifdef RCV_DATA_DEBUG
	char *read_c;
#endif
	unsigned int blksize,blocks;
	blksize = data->blksz;
	blocks = data->blocks;

	for(k=0;k<blocks;k++)
		{
		if(intmask &  (1<<5)) //data read
			{
			for(i=0;i<(blksize/4);i++)
				{
				data->buff_addr[i+k*(blksize/4)] = READ_REG(SDHCI_BUFFER);
				}
			}
		if(intmask &  (1<<5)) //data write
			{
			for(i=0;i<(blksize/4);i++)
				{
				WRITE_REG(SDHCI_BUFFER,data->buff_addr[i+k*(blksize/4)]);
				}
			}
		}
	if(intmask &  (1<<1))
		{
#ifdef RCV_DATA_DEBUG
	//only for read test
		if(data->flags & MMC_DATA_READ) //data read
			{
			read_c = (char *)data->buff_addr;
			os_printf(LM_OS, LL_INFO, "[read buffer start], addr :0x%x\r\n",data->buff_addr);
			for(i=0;i<(blksize*blocks);i++)
				os_printf(LM_OS, LL_INFO, "%x ",read_c[i]&0xff);
			os_printf(LM_OS, LL_INFO, "\r\n [read buffer end]\r\n");
			}
#endif	
		}
	return 0;
}





//test function

static void mmc_decode_cid(unsigned int *raw_cid,struct mmc_cid *cid)
{
	unsigned int *resp = raw_cid;

	memset(cid, 0, sizeof(struct mmc_cid));

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	cid->manfid		= UNSTUFF_BITS(resp, 120, 8);
	cid->oemid			= UNSTUFF_BITS(resp, 104, 16);
	cid->prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	cid->prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	cid->prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	cid->prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	cid->prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	cid->hwrev			= UNSTUFF_BITS(resp, 60, 4);
	cid->fwrev			= UNSTUFF_BITS(resp, 56, 4);
	cid->serial		= UNSTUFF_BITS(resp, 24, 32);
	cid->year			= UNSTUFF_BITS(resp, 12, 8);
	cid->month			= UNSTUFF_BITS(resp, 8, 4);

	cid->year += 2000; /* SD cards year offset */

	//os_printf(LM_OS, LL_INFO, "prod_name is:%s\r\n",cid->prod_name);
	//os_printf(LM_OS, LL_INFO, "prod_year:%d,month:%d\r\n",cid->year,cid->month);
}

static int mmc_decode_csd(unsigned int *raw_csd,struct mmc_csd *csd)
{
	unsigned int e, m, csd_struct;
	//unsigned short int temp;
	
	unsigned int *resp = raw_csd;
	csd_struct = UNSTUFF_BITS(resp, 126, 2);
	switch (csd_struct) {
	case 0:
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
		csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
		csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
		csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
		csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
		csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
		csd->write_partial = UNSTUFF_BITS(resp, 21, 1);
		csd->card_type = 0;
		//temp = UNSTUFF_BITS(resp, 96, 8);
		//printk("Highspeed card capability = %x\n",temp);
		//os_printf(LM_OS, LL_INFO, "Highspeed card capability = %x\n",temp);
		break;
	case 1:
		/*
		 * This is a block-addressed SDHC card. Most
		 * interesting fields are unused and have fixed
		 * values. To avoid getting tripped by buggy cards,
		 * we assume those fixed values ourselves.
		 */
		//mmc_card_set_blockaddr(card);

		csd->tacc_ns	 = 0; /* Unused */
		csd->tacc_clks	 = 0; /* Unused */

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		m = UNSTUFF_BITS(resp, 48, 22);
		//printk("value of m = %x\n",m);
		csd->capacity     = (1 + m) << 10;
		if(m>0xFF5F)
		{
		//SDXC = 1;
                csd->card_type = 1;
		//printk("SDXC SUPPORTED CARD\n");
		//os_printf(LM_OS, LL_INFO, "SDXC SUPPORTED CARD\r\n");
		}
                else if(m>0x1010)
                {
                csd->card_type=2;
                //printk("SDHC card \n ");
                os_printf(LM_OS, LL_INFO, "SDHC card\n");
                }
                else
                //printk("Normal SD card \n");
                os_printf(LM_OS, LL_INFO, "Normal SD card\n");
		//printk("card capacity in sectors = %d\n",csd->capacity);
		//os_printf(LM_OS, LL_INFO, "card capacity in sectors = %d\n",csd->capacity);
		csd->read_blkbits = 9;
		csd->read_partial = 0;
		csd->write_misalign = 0;
		csd->read_misalign = 0;
		csd->r2w_factor = 4; /* Unused */
		csd->write_blkbits = 9;
		csd->write_partial = 0;
		break;
	default:
		//printk(KERN_ERR "%s: unrecognised CSD structure version %d\n",
		//	mmc_hostname(card->host), csd_struct);
		os_printf(LM_OS, LL_INFO, "unrecognised CSD structure version\n");
		
		return -2;
	}

	return 0;
}


static void sdhci_go_idle()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));

	//go idle state
	sdhci_host.cmd.opcode=0;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;
	sdhci_host.cmd.data = NULL;
	sdhci_send_command(NULL, &sdhci_host.cmd);

}

static int sdhci_send_if_cond(unsigned int ocr)
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	static const char test_pattern = 0xAA;
	char result_pattern = 0x00;
	int ret;
	//go idle state
	sdhci_host.cmd.opcode=8;
	/*

	command format:
	
	  -----39:20-----19:16-----15:8-----
	     reserved    VHS	 Check pattern

	VHS:
	0000b	Not define
	0001b	2.7-3.6v
	0010b	reserved for low voltage range
	0100b	reserved
	1000b	reserved
	other	Not defined
	  
	*/

	sdhci_host.cmd.arg=0x1aa;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R7 | MMC_RSP_R7 | MMC_CMD_BCR;
	sdhci_host.cmd.data=NULL;

	ret = sdhci_send_command(NULL, &sdhci_host.cmd);
	if(ret)
		{
		os_printf(LM_OS, LL_INFO, "!!!CMD failed\r\n");
		return ret;
		}

	else
		{
		result_pattern = sdhci_host.cmd.resp[0] & 0xFF;
		if (result_pattern != test_pattern)
			{
			os_printf(LM_OS, LL_INFO, "!!!result_pattern failed:%x\r\n",result_pattern);
			return -1;
			}
		else
			return 0;
		}

}

static int mmc_app_cmd()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=55;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_BCR;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}

static int mmc_app_cmd_rca()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=55;
	sdhci_host.cmd.arg=sdhci_host.sdcard.rca;
	sdhci_host.cmd.flags= MMC_RSP_R1 | MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}

static int mmc_send_app_op_cond(unsigned int ocr)
{
	//app cmd CMD55+CMD41

	//CMD55
	int ret;
	ret = mmc_app_cmd();

	if(ret)
		return -1;

	memset(sdhci_host.cmd.resp, 0, sizeof(sdhci_host.cmd.resp));
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));

	sdhci_host.cmd.opcode=41;

	//31:busy	30:ccs(card capacity status)	29:UHS-II card	24:S18A(switching to 1.8v accepted)
	//According physical layer document
	sdhci_host.cmd.arg=ocr;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R1 | MMC_RSP_R3 | MMC_CMD_BCR;
	ret = sdhci_send_command(NULL, &sdhci_host.cmd);
	if(ret)
		return -1;
	
	//31:busy status
	//30:ccs(card capacity status)	
	//29:UHS-II
	//24:S18A

	if ( sdhci_host.cmd.resp[0] & 1<<31)
		{
		//os_printf(LM_OS, LL_INFO, "sd card is ready\r\n");

		if(sdhci_host.cmd.resp[0] & 1<<30)
			{
			sdhci_host.sdcard.ccs=1;
			}
		else
			{
			sdhci_host.sdcard.ccs=0;
			}

		if (sdhci_host.cmd.resp[0] & 1<<24)
			{
			sdhci_host.sdcard.s18a=1;
			}
		else
			{
			}

		return 0;
		}
	else
		{
		return -1;
		}
	return 0;
}

static int mmc_all_send_cid(unsigned int *cid)
{
	int ret;
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode = 2;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_R2 | MMC_CMD_BCR;
	ret=sdhci_send_command(NULL, &sdhci_host.cmd);
	if(ret)
		{
		return -1;
		}
	else
		{
		memcpy(cid,sdhci_host.cmd.resp,sizeof(unsigned int)*4);
		return 0;
		}
}

static int mmc_all_send_csd(unsigned int *csd)
{
	int ret;
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=9;
	sdhci_host.cmd.arg=sdhci_host.sdcard.rca;
	sdhci_host.cmd.flags=MMC_RSP_R2 | MMC_CMD_AC;

	ret = sdhci_send_command(NULL, &sdhci_host.cmd);
	if(ret)
		{
		return -1;
		}
	else
		{
		memcpy(csd,sdhci_host.cmd.resp,sizeof(unsigned int)*4);
		return 0;
		}
}

static int mmc_all_set_data_mode(void)
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=7;
	sdhci_host.cmd.arg=sdhci_host.sdcard.rca;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}
static int mmc_all_set_blk_size(unsigned int blk_sz)
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	
	sdhci_host.cmd.opcode=16;
	sdhci_host.cmd.arg=blk_sz;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}

static int mmc_all_send_cmd13(void)
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=13;
	sdhci_host.cmd.arg=sdhci_host.sdcard.rca;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}


static int mmc_all_send_acmd6(void)
{
	mmc_app_cmd_rca();
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=6;
	sdhci_host.cmd.arg=1<<1;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}

/*
static int mmc_send_cmd11()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=11;
	sdhci_host.cmd.arg = 0;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1| MMC_CMD_AC;
	return(sdhci_send_command(NULL, &sdhci_host.cmd));
}
*/

static int mmc_send_relative_addr(unsigned int *rca)
{
	int ret;
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));	
	sdhci_host.cmd.opcode=3;
	sdhci_host.cmd.arg = 0;
	sdhci_host.cmd.flags = MMC_RSP_R6 | MMC_CMD_BCR;
	ret=sdhci_send_command(NULL, &sdhci_host.cmd);
	if(ret)
		{
		return -1;
		}
	else
		{
		*rca = sdhci_host.cmd.resp[0];
		return 0;
		}
}


#if 0
static int mmc_sd_switch(int mode, int group,char value)
{	

	int ret;
	
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	memset(&sdhci_host.data, 0, sizeof(struct mmc_data));

#if 0
	int i =0;

	for (i = 0; i < sd_test_data_size; i++)
		{
		sd_test_data[i]=0xaa;
		}

	for (i = 0; i < 10; i++)
		{
		sd_adma_desc[i].wAttribute=0;
		sd_adma_desc[i].wLen=0;
		sd_adma_desc[i].dwAddress=NULL;
		}
#endif

	/* NOTE: caller guarantees resp is heap-allocated */

	mode = !!mode;
	value &= 0xF;


	sdhci_host.data.blksz = 64;
	sdhci_host.data.blocks = 1;
	sdhci_host.data.flags = MMC_DATA_READ;
	sdhci_host.data.buff_addr = (unsigned int *)sd_test_data;
	sdhci_host.data.desc_addr = (ADMA2 *)sd_adma_desc;
	sdhci_host.data.flags = MMC_DATA_READ;
	sdhci_host.data.transtype = ADMA;


	sdhci_host.cmd.opcode = 6;
	sdhci_host.cmd.arg = mode << 31 | 0x00FFFFFF;
	sdhci_host.cmd.arg &= ~(0xF << (group * 4));
	sdhci_host.cmd.arg |= value << (group * 4);
	sdhci_host.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	sdhci_host.cmd.data = &sdhci_host.data;

	ret=sdhci_send_command(&sdhci_host.data,&sdhci_host.cmd);
	if(ret)
		return ret;
	else
		{
#if 0
		os_printf(LM_OS, LL_INFO, "\r\n cmd6 response:\r\n");
		for(i=0;i<sdhci_host.data.blksz;i++)
			{
			os_printf(LM_OS, LL_INFO, "%x ",sd_write_data[i] & 0xff);
			}
		os_printf(LM_OS, LL_INFO, "\r\n");
#endif
		OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)|SDHCI_CTRL_HISPD);
		return 0;
		}
}
#endif

void mmc_sleep_wakeup(int mode)
{
	if(mode == 0)
		{
		//set to 1bit mode
		sdhci_set_ctrl(MMC_BUS_WIDTH_1); // set 1 bit width

		//stop clk
		OUT16(SDHCI_CLOCK_CONTROL, IN16(SDHCI_CLOCK_CONTROL)&~(1<<2));

		//clear interrupt and set wakeup interrupt
		
		sdhci_clear_set_irq(SDHCI_INT_ALL_MASK,
					//SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
					//SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
					//SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
					SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_INT |SDHCI_INT_CARD_REMOVE//|
					//SDHCI_INT_DATA_END | 
					//SDHCI_INT_RESPONSE| SDHCI_TUNING_EVENT | SDHCI_TUNING_ERROR// |
					//eMMC_BOOT_ACK_INT | eMMC_BOOT_DONE_INT
					);
		}

	if(mode == 1)
		{
		//disable each factor
		/*
		sdhci_clear_set_irq(SDHCI_INT_ALL_MASK,
					//SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
					//SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
					//SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
					SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_INT |SDHCI_INT_CARD_REMOVE//|
					//SDHCI_INT_DATA_END | 
					//SDHCI_INT_RESPONSE| SDHCI_TUNING_EVENT | SDHCI_TUNING_ERROR// |
					//eMMC_BOOT_ACK_INT | eMMC_BOOT_DONE_INT
					);
		*/
		
		//enable clk
		OUT16(SDHCI_CLOCK_CONTROL, IN16(SDHCI_CLOCK_CONTROL)|(1<<2));

		//set to 4bit mode
		sdhci_set_ctrl(MMC_BUS_WIDTH_4); 

		sdhci_clear_set_irq(SDHCI_INT_ALL_MASK,
					SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
					SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
					SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
					SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_INT |SDHCI_INT_CARD_REMOVE|
					SDHCI_INT_DATA_END | 
					SDHCI_INT_RESPONSE| SDHCI_TUNING_EVENT | SDHCI_TUNING_ERROR// |
					//eMMC_BOOT_ACK_INT | eMMC_BOOT_DONE_INT
					);
		}
}



unsigned int get_blk_size(void)
{
	return MAX_BLK_SIZE;
}
unsigned int get_sd_ccs(void)
{
	return sdhci_host.sdcard.ccs;
}


unsigned int get_sector_count(void)
{
	return (sdhci_host.sdcard.de_csd.capacity * ((1 << sdhci_host.sdcard.de_csd.read_blkbits)/MAX_BLK_SIZE));
}

char is_sdcard_inserted(void)
{
	return sdhci_host.sdcard.sdcard_inserted;
}



//sdcard 
int sdhci_init_sdcard(void)
{
	int err,ret;
	int timer_op_cond = 1000;
	unsigned int ocr = 0xFF8000;//if support all voltage
	//memset(&sdhci_host, 0, sizeof(sdhci_host));

	if(sdhci_host.sdcard.sdcard_inserted == 0)
		return 3;//SDCARD NOT INSERTED

	//go to idle state CMD0
	sdhci_go_idle();
	os_msdelay(40);//delay 40ms

	//reset clock/power/width/highspeed mode

	sdhci_set_clock(400*1000);//reinit clock
	//set sdcard power
	sdhci_set_power(21); //3.3v to be modified
	//set ctrl
	sdhci_set_ctrl(MMC_BUS_WIDTH_1); // set 1 bit width
	//clear high-speed flag
	OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)&(~SDHCI_CTRL_HISPD));
	
	
	//if cond CMD8
	err = sdhci_send_if_cond(ocr);
	if(!err)
		ocr |= 1<<30 | 1<<28 | 1<<24;	
	else
		return -8;
	
	//31:busy	30:ccs(card capacity status)	29:UHS-II card	24:S18A(switching to 1.8v accepted)
	//According physical layer document
	
	//CMD55+CMD41
	
	while (mmc_send_app_op_cond(ocr))
		{
		//wait sdcard ready
		timer_op_cond--;
		if(timer_op_cond == 0)
			{
			os_printf(LM_OS, LL_INFO, "wait sdcard ready timeout!\r\n");
			return -41;
			}
		}
	
#if 0
	//switch voltage for UHS-I
	//CMD11
	if(ccs & s18a)
		{
		while (cmd_busy)
		{
		//wait for cmd ready
		}
		mmc_send_cmd11();
		//sdhci_set_power(7); //set power 1.8v.
		OUT16(SDHCI_HOST_CONTROL2, IN16(SDHCI_HOST_CONTROL2)|(1<<3));
		}


	

	sdhci_set_power(7); //set power 1.8v.
	//OUT8(SDHCI_POWER_CONTROL, pwr);
	os_printf(LM_OS, LL_INFO, "set power 1.8v\r\n");
	

	while (!(IN32(SDHCI_PRESENT_STATE) & 0x00f00000))
		{
		os_printf(LM_OS, LL_INFO, "wait for sdcard status\r\n");
		}
#endif

	//CMD2
	ret = mmc_all_send_cid(sdhci_host.sdcard.cid);
	if(ret)
		return -2;
	else
		mmc_decode_cid(sdhci_host.sdcard.cid, &sdhci_host.sdcard.de_cid);

	//CMD3
	ret = mmc_send_relative_addr(&sdhci_host.sdcard.rca);
	if(ret)
		return -3;
	
	//CMD9
	ret = mmc_all_send_csd(sdhci_host.sdcard.csd);
	if(ret)
		return -9;
	else
		{
		mmc_decode_csd(sdhci_host.sdcard.csd,&sdhci_host.sdcard.de_csd);
		sdhci_host.sdcard.sd_cap = sdhci_host.sdcard.de_csd.capacity;
		}

	//goto data mode CMD7
	ret = mmc_all_set_data_mode();
	if(ret)
		return -7;
	
	//get scr
	//mmc_app_send_scr(scr);	
	
	ret = mmc_all_send_acmd6();//acmd, set sd card width to 4bit
	if(ret)
		return -6;
	else
		sdhci_set_ctrl(MMC_BUS_WIDTH_4);//switch to 4bit

	//switch to high speed mode
#if 0
	if(sdhci_host.sdcard.de_csd.cmdclass & (1<<10))//if sd card support high speed switch
		{
		ret = mmc_sd_switch(1,0,1);
		if(ret)
			return -6;
		}
#endif
	sdhci_set_clock(SDHCI_WORK_CLOCK);	//set clock 

	//set block size CMD16
	ret = mmc_all_set_blk_size(MAX_BLK_SIZE);
	if(ret)
		return -16;

	//send status
	ret = mmc_all_send_cmd13();
	if(ret)
		return -13;

	//os_printf(LM_OS, LL_INFO, "prod_name:%s,year:%d,month:%d\r\n",sdhci_host.sdcard.de_cid.prod_name,sdhci_host.sdcard.de_cid.year,sdhci_host.sdcard.de_cid.month);
	//os_printf(LM_OS, LL_INFO, "card capacity in sectors = %d,card type :%d,cmdswitch:%x\r\n",sdhci_host.sdcard.de_csd.capacity,sdhci_host.sdcard.de_csd.card_type,sdhci_host.sdcard.de_csd.cmdclass);
	//os_printf(LM_OS, LL_INFO, "rca value:%x\r\n",sdhci_host.sdcard.rca);
	//os_printf(LM_OS, LL_INFO, "work clock is:%d mhz\r\n",SDHCI_WORK_CLOCK/(1000*1000));

	//gpio_intr_mode(20, GPIO_INTR_MODE_N_EDGE);
	//os_printf(LM_OS, LL_INFO, "sdhci init\r\n");

	return 0;
}


//#define SDCARD_TEST

#ifdef SDCARD_TEST
int sdhci_test_sdio_write(unsigned int transtype,unsigned int blocks,unsigned int card_addr)
{
	int i;
	char test_data_s=0x00;
	int blk_pad;

	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	memset(&sdhci_host.data, 0, sizeof(struct mmc_data));

	for (i = 0; i < sd_test_data_size; i++)
		{
		sd_test_data[i]=test_data_s++;
		//sd_test_data[i]=rand()%256;
		}
	memcpy(sd_write_data,sd_test_data,MAX_BLK_SIZE*blocks);
	

	for (i = 0; i < 10; i++)
			{
			sd_adma_desc[i].wAttribute=0;
			sd_adma_desc[i].wLen=0;
			sd_adma_desc[i].dwAddress=NULL;
			}

	sdhci_host.data.blksz=MAX_BLK_SIZE;
	sdhci_host.data.blocks=blocks;
	sdhci_host.data.buff_addr = (unsigned int *)sd_test_data;
	sdhci_host.data.desc_addr = (ADMA2 *)sd_adma_desc;
	sdhci_host.data.flags = MMC_DATA_WRITE;
	sdhci_host.data.transtype = transtype;

	sdhci_host.cmd.data = &sdhci_host.data;
	if(blocks == 1)
		{
		sdhci_host.cmd.opcode = 53;
		}
	else
		{
		sdhci_host.cmd.opcode = 53;
		}

	if (sdhci_host.data.blksz == MAX_BLK_SIZE)
		{
		blk_pad = 0;
		}
	else
		blk_pad = sdhci_host.data.blksz;
	

	sdhci_host.cmd.arg = 1<<31 | 1<<28 | 1<<26 | card_addr<<9;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;


	sdhci_send_command(&sdhci_host.data,&sdhci_host.cmd);

	return 0;
	
}


int sdhci_test_sdio_read(unsigned int transtype,unsigned int blocks,unsigned int card_addr)
{
	//struct mmc_data data;
	//struct mmc_command cmd;
	int i,k;
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	memset(&sdhci_host.data, 0, sizeof(struct mmc_data));


	for (i = 0; i < sd_test_data_size; i++)
		{
		sd_test_data[i]=0xaa;
		}


	for (i = 0; i < 10; i++)
		{
		sd_adma_desc[i].wAttribute=0;
		sd_adma_desc[i].wLen=0;
		sd_adma_desc[i].dwAddress=NULL;
		}

	//os_printf(LM_OS, LL_INFO, "sdhci_test_read_block\r\n");

	sdhci_host.data.blksz=MAX_BLK_SIZE;
	sdhci_host.data.blocks=blocks;
	sdhci_host.data.buff_addr = (unsigned int *)sd_test_data;
	sdhci_host.data.desc_addr = (ADMA2 *)sd_adma_desc;
	sdhci_host.data.flags = MMC_DATA_READ;
	sdhci_host.data.transtype = transtype;


	sdhci_host.cmd.data = &sdhci_host.data;
	
	if(blocks == 1)
		{
		sdhci_host.cmd.opcode = 53;
		//sdhci_host.cmd.arg = 0x200000;
		}
	else
		{
		sdhci_host.cmd.opcode = 53;
		//sdhci_host.cmd.arg = 0x300000;
		}
	sdhci_host.cmd.arg = 1<<28 | 1<<26 | card_addr<<9;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

	sdhci_send_command(&sdhci_host.data,&sdhci_host.cmd);

	/*
	while (data_busy)
		{
		}
	*/
	
	for(k=0;k<MAX_BLK_SIZE*blocks;k++)
		{
		os_printf(LM_OS, LL_INFO, "%x ",sd_test_data[k]&0xff);
		}
	os_printf(LM_OS, LL_INFO, "\r\n");
	
	return 0;
}


int sdhci_test_write_block(unsigned int transtype,unsigned int blocks,unsigned int card_addr)
{
	int i;
	//char test_data_s=0xbb;

	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	memset(&sdhci_host.data, 0, sizeof(struct mmc_data));
/*
	for (i = 0; i < sd_test_data_size; i++)
		{
		//sd_test_data[i]=test_data_s++;
		sd_test_data[i]=rand()%256;
		}


	memcpy(sd_write_data,sd_test_data,MAX_BLK_SIZE*blocks);
*/	

	for (i = 0; i < MAX_BUF_BLOCK; i++)
		{
		sd_adma_desc[i].wAttribute=0;
		sd_adma_desc[i].wLen=0;
		sd_adma_desc[i].dwAddress=NULL;
		}

	sdhci_host.data.blksz=MAX_BLK_SIZE;
	sdhci_host.data.blocks=blocks;
	sdhci_host.data.buff_addr = (unsigned int *)sd_test_data;
	sdhci_host.data.desc_addr = (ADMA2 *)sd_adma_desc;
	sdhci_host.data.flags = MMC_DATA_WRITE;
	sdhci_host.data.transtype = transtype;

	sdhci_host.cmd.data = &sdhci_host.data;
	if(blocks == 1)
		{
		sdhci_host.cmd.opcode = 24;
		}
	else
		{
		sdhci_host.cmd.opcode = 25;
		}

	sdhci_host.cmd.arg = card_addr;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;


	sdhci_send_command(&sdhci_host.data,&sdhci_host.cmd);

	return 0;
	
}

int sdhci_test_read_block(unsigned int transtype,unsigned int blocks,unsigned int card_addr)
{
	//struct mmc_data data;
	//struct mmc_command cmd;
	int i;
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	memset(&sdhci_host.data, 0, sizeof(struct mmc_data));


	for (i = 0; i < sd_test_data_size; i++)
		{
		sd_test_data[i]=0xaa;
		}

	for (i = 0; i < 10; i++)
		{
		sd_adma_desc[i].wAttribute=0;
		sd_adma_desc[i].wLen=0;
		sd_adma_desc[i].dwAddress=NULL;
		}

	//os_printf(LM_OS, LL_INFO, "sdhci_test_read_block\r\n");

	sdhci_host.data.blksz=MAX_BLK_SIZE;
	sdhci_host.data.blocks=blocks;
	sdhci_host.data.buff_addr = (unsigned int *)sd_test_data;
	sdhci_host.data.desc_addr = (ADMA2 *)sd_adma_desc;
	sdhci_host.data.flags = MMC_DATA_READ;
	sdhci_host.data.transtype = transtype;


	sdhci_host.cmd.data = &sdhci_host.data;
	
	if(blocks == 1)
		{
		sdhci_host.cmd.opcode = 17;
		//sdhci_host.cmd.arg = 0x200000;
		}
	else
		{
		sdhci_host.cmd.opcode = 18;
		//sdhci_host.cmd.arg = 0x300000;
		}
	sdhci_host.cmd.arg = card_addr;
	sdhci_host.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	sdhci_send_command(&sdhci_host.data,&sdhci_host.cmd);

	//while (data_busy)
		//	{
			//}
#if 0
	utils_Printf("\n");
	//for (i = 0; i < blocks*MAX_BLK_SIZE; ++i)
	for (i = 0; i < 16; ++i)
		{
		utils_Printf("%x ",sd_read_data[i]&0xff);
		}
	utils_Printf("\n");
#endif
	return 0;
}


int sdhci_test_erase_block(unsigned int begain_blocks,unsigned int end_blocks)
{
	
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=32;
	sdhci_host.cmd.arg=begain_blocks;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	
	sdhci_send_command(NULL, &sdhci_host.cmd);

	
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=33;
	sdhci_host.cmd.arg=end_blocks;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	
	sdhci_send_command(NULL, &sdhci_host.cmd);

	
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=38;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_R1 | MMC_CMD_AC;
	
	sdhci_send_command(NULL, &sdhci_host.cmd);

	return 0;
}



int sdhci_test_write_and_read(unsigned int test_type,unsigned int start_addr,unsigned int end_addr)
{
	unsigned int i,k,blk_cnt,ablk;
	//char tmp;
	completed_per=1;

	for (i = 0; i < sd_test_data_size; i++)
		{
		sd_test_data[i]=i%256;
		//sd_test_data[i]=rand()%256;
		}
	
	if(end_addr == 0)
		end_addr = sdhci_host.sdcard.sd_cap-1;
	ablk = end_addr-start_addr+1;

	os_printf(LM_OS, LL_INFO, "sdhci_test_write_and_read :%d,%d ,ablk:%d\r\n",start_addr,end_addr,ablk);
	for (i = start_addr; i <= end_addr;)
		{
		if (end_addr-i < MAX_BUF_BLOCK-1)
			{
			blk_cnt = end_addr-i+1;
			}
		else
			blk_cnt = MAX_BUF_BLOCK;

		if((i*100)/ablk == completed_per)
			{
			os_printf(LM_OS, LL_INFO, "[%d]complete %d percent\r\n",test_type,completed_per);
			completed_per++;
			}
		
		
		if(test_type & 1<<0)
			{
			if (sdhci_test_write_block(ADMA,blk_cnt,i))
				{
				//write error
				return -1;
				}
			}

		if(test_type & 1<<1)
			{
			if (sdhci_test_read_block(ADMA,blk_cnt,i))
				{
				//read error
				return -1;
				}
			}

#if 0
#if 1
		for(k=0;k<MAX_BLK_SIZE*blk_cnt;k++)
			{
			if((sd_read_data[k]) != (sd_write_data[k]))
				{
				os_printf(LM_OS, LL_INFO, "data diff!! [%d]%x:%x\r\n",k,sd_read_data[k],sd_write_data[k]);
				return -1;
				}
			}

#else


		for(k=0;k<MAX_BLK_SIZE*blk_cnt;k++)
			{
			os_printf(LM_OS, LL_INFO, "%x ",sd_read_data[k] & 0xff);
			
			}

		os_printf(LM_OS, LL_INFO, "\r\ndata write\r\n");

		for(k=0;k<MAX_BLK_SIZE*blk_cnt;k++)
			{
			os_printf(LM_OS, LL_INFO, "%x ",sd_write_data[k] & 0xff);
			
			/*
			if(sd_test_data[k] != sd_write_data[k])
				{
				os_printf(LM_OS, LL_INFO, "data diff!!\r\n");
				return -1;
				}
			*/
			}
#endif		
#endif

		i+=blk_cnt;
		}
	return 0;
}

#endif



//for sdio test


#ifdef SDIO_TEST
void sdhci_test_sdio_cmd52(unsigned int arg)
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));

	//reset 
	sdhci_host.cmd.opcode=52;
	sdhci_host.cmd.arg=arg;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_AC;
	sdhci_host.cmd.data = NULL;
	sdhci_send_command(NULL, &sdhci_host.cmd);
}
void sdhci_test_sdio_reset()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));

	//reset 
	sdhci_host.cmd.opcode=52;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;
	sdhci_host.cmd.data = NULL;
	sdhci_send_command(NULL, &sdhci_host.cmd);

	sdhci_host.cmd.opcode=52;
	sdhci_host.cmd.arg=0x80000c08;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R1 | MMC_RSP_NONE | MMC_CMD_BC;
	sdhci_host.cmd.data = NULL;
	sdhci_send_command(NULL, &sdhci_host.cmd);
}

static void mmc_send_cmd5()
{
	memset(&sdhci_host.cmd, 0, sizeof(struct mmc_command));
	sdhci_host.cmd.opcode=5;
	sdhci_host.cmd.arg=0;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;
	sdhci_send_command(NULL, &sdhci_host.cmd);

	sdhci_host.cmd.opcode=5;
	sdhci_host.cmd.arg=0x200000;
	sdhci_host.cmd.flags=MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;
	sdhci_send_command(NULL, &sdhci_host.cmd);

}


int sdhci_test_init_sdio_card(void)
{

	memset(&sdhci_host.sdcard.de_cid, 0, sizeof(struct mmc_cid));
	memset(&sdhci_host.sdcard.de_csd, 0, sizeof(struct mmc_csd));



	sdhci_go_idle();

	
	//reset CMD52
	sdhci_test_sdio_reset();
	os_printf(LM_OS, LL_INFO, "for cmd52 delay\r\n");//or delay 40ms


	//reset clock/power/width/highspeed mode

	sdhci_set_clock(400*1000);//reinit clock
	//set sdcard power
	sdhci_set_power(21); //3.3v to be modified
	//set ctrl
	sdhci_set_ctrl(MMC_BUS_WIDTH_1); // set 1 bit width
	//clear high-speed flag
	OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)&(~SDHCI_CTRL_HISPD));

#if 0	

	//if cond CMD8
	err = sdhci_send_if_cond(ocr);
	


	if(!err)
		ocr |= 1 << 30 | 1<<28 | 1<<24;	
	else
		return -1;
	
	//31:busy	30:ccs(card capacity status)	29:UHS-II card	24:S18A(switching to 1.8v accepted)
	//According physical layer document
	
	//CMD55+CMD41
	
	sdhci_host.cmd.resp[0]=0;

	while (mmc_send_app_op_cond(ocr))
		{
		//wait sdcard is ready

		timer_op_cond--;
		if(timer_op_cond == 0)
			return 0;

		}
#endif

	mmc_send_cmd5();

	

/*
	//switch voltage for UHS-I
	//CMD11
	if(ccs & s18a)
		{
		while (cmd_busy)
		{
		//wait for cmd ready
		}
		mmc_send_cmd11();
		//sdhci_set_power(7); //set power 1.8v.
		OUT16(SDHCI_HOST_CONTROL2, IN16(SDHCI_HOST_CONTROL2)|(1<<3));
		}


	

	sdhci_set_power(7); //set power 1.8v.
	//OUT8(SDHCI_POWER_CONTROL, pwr);
	os_printf(LM_OS, LL_INFO, "set power 1.8v\r\n");
	

	while (!(IN32(SDHCI_PRESENT_STATE) & 0x00f00000))
		{
		os_printf(LM_OS, LL_INFO, "wait for sdcard status\r\n");
		}
*/

	//CMD2
	//mmc_all_send_cid(cid);

	//mmc_decode_cid(cid, &sdhci_host.sdcard.de_cid);

	//CMD3
	mmc_send_relative_addr(&sdhci_host.sdcard.rca);
	
	//CMD9
	//mmc_all_send_csd(csd);
	//mmc_decode_csd(csd,&sdhci_host.sdcard.de_csd);
	//sdhci_host.sdcard.sd_cap = sdhci_host.sdcard.de_csd.capacity;


	//goto data mode CMD7
	mmc_all_set_data_mode();
	
	//get scr
	//mmc_app_send_scr(scr);	
	
	//sdhci_set_ctrl(MMC_BUS_WIDTH_4); // set 4 bit width
	//mmc_all_send_acmd6();//acmd, set sd card width to 4bit
	
	

	//switch to high speed mode
	/*
	if(sdhci_host.sdcard.de_csd.cmdclass & (1<<10))//if sd card support high speed switch
		{
		mmc_sd_switch(1,0,1);
		}
	*/
	//OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)|SDHCI_CTRL_HISPD);
	//sdhci_set_clock(SDHCI_WORK_CLOCK);	//set clock 

	//set block size CMD16
	//mmc_all_set_blk_size();

	//send status
	//mmc_all_send_cmd13();
	os_printf(LM_OS, LL_INFO, "for cmd7 delay\r\n");//or delay 40ms

	sdhci_test_sdio_cmd52(0x80002601);//set,bus speed select:SHS
	sdhci_test_sdio_cmd52(0x80000E46);//set,bus interface control:scsi/s8b/width 1

	sdhci_set_ctrl(MMC_BUS_WIDTH_4);//switch to 4bit
	
	OUT8(SDHCI_HOST_CONTROL, IN8(SDHCI_HOST_CONTROL)|SDHCI_CTRL_HISPD);
	sdhci_set_clock(SDHCI_WORK_CLOCK);

	
	os_printf(LM_OS, LL_INFO, "clock set delay\r\n");//or delay 40ms
	
	sdhci_test_sdio_cmd52(0x00000400);//read,IO function enable
	sdhci_test_sdio_cmd52(0x80000402);//set,IO1 function enable
	
	sdhci_test_sdio_cmd52(0x00000600);//read,IO function ready

	//function base register
	/*
	sdhci_test_sdio_cmd52(0x80020000);//
	sdhci_test_sdio_cmd52(0x80022002);//io block size
	*/
	
	//os_printf(LM_OS, LL_INFO, "prod_name:%s,year:%d,month:%d\r\n",sdhci_host.sdcard.de_cid.prod_name,sdhci_host.sdcard.de_cid.year,sdhci_host.sdcard.de_cid.month);
	//os_printf(LM_OS, LL_INFO, "card capacity in sectors = %d,card type :%d,cmdswitch:%x\r\n",sdhci_host.sdcard.de_csd.capacity,sdhci_host.sdcard.de_csd.card_type,sdhci_host.sdcard.de_csd.cmdclass);
	//os_printf(LM_OS, LL_INFO, "rca value:%x\r\n",rca);
	//os_printf(LM_OS, LL_INFO, "work clock is:%d khz\r\n",SDHCI_WORK_CLOCK/1000);
	
	//reset data line
	//OUT8(SDHCI_SOFTWARE_RESET, SDHCI_RESET_DATA);

	return 0;
}

#endif


#ifdef SDCARD_DETECT
void detect_isr(void *data)
{
	//os_printf(LM_OS, LL_INFO, "\r\n==>>[gpio-%d] detect ok\r\n", 20);

	//sdhci_test_init_sdcard();
	//gpio_intr_mode(20, GPIO_INTR_MODE_N_EDGE);
	//os_msdelay(40);
	if(drv_gpio_read(SDCARD_DETECT_GPIO))
		{
		os_printf(LM_OS, LL_INFO, "\r\n==>>[gpio-%d] detect high\r\n", SDCARD_DETECT_GPIO);
		sdhci_host.sdcard.sdcard_inserted = 0;
		}
	else
		{
		os_printf(LM_OS, LL_INFO, "\r\n==>>[gpio-%d] detect low\r\n", SDCARD_DETECT_GPIO);
		sdhci_host.sdcard.sdcard_inserted = 1;
		}
	drv_gpio_ioctrl(20,DRV_GPIO_CTRL_INTR_MODE,DRV_GPIO_ARG_INTR_MODE_DUAL_EDGE);
	
}


void sdhci_card_detect()
{
	T_GPIO_ISR_CALLBACK detect_cb;

	if (drv_gpio_init())
	{
		os_printf(LM_OS, LL_INFO, "gpio init failed\r\n");
	}
	else
	{
		os_printf(LM_OS, LL_INFO, "gpio init ok\r\n");
	}

	if(SDCARD_DETECT_GPIO == 20)
		{
		PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_GPIO20);
		}
	detect_cb.gpio_callback=detect_isr;
	detect_cb.gpio_data=NULL;
	drv_gpio_ioctrl(SDCARD_DETECT_GPIO,DRV_GPIO_CTRL_SET_DIRCTION,DRV_GPIO_ARG_DIR_IN);
	drv_gpio_ioctrl(SDCARD_DETECT_GPIO,DRV_GPIO_CTRL_REGISTER_ISR,(int)&detect_cb);
	drv_gpio_ioctrl(SDCARD_DETECT_GPIO,DRV_GPIO_CTRL_INTR_MODE,DRV_GPIO_ARG_INTR_MODE_DUAL_EDGE);
	
	//interrupt_unmask(VECTOR_NUM_GPIO);
	drv_gpio_ioctrl(SDCARD_DETECT_GPIO,DRV_GPIO_CTRL_INTR_ENABLE,0);
	//gpio_intr_enable(num);
	os_printf(LM_OS, LL_INFO, "\r\n==>>[gpio-%d] cfg gpio dual-edge intr ok\r\n", SDCARD_DETECT_GPIO);

	//init card status
	if(drv_gpio_read(SDCARD_DETECT_GPIO))
		{
		sdhci_host.sdcard.sdcard_inserted = 0;
		}
	else
		{
		sdhci_host.sdcard.sdcard_inserted = 1;
		}
}
#endif

