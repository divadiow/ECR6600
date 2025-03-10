


//#define IN8(reg)				(char)( (*(volatile unsigned long *)(reg)) & 0x000000FF )
//#define IN16(reg)				(short int)( (*(volatile unsigned long *)(reg)) & 0x0000FFFF )
//#define IN32(reg)				(  *( (volatile unsigned int *) (reg) ) )
//
//#define OUT8(reg, data)		((*((volatile unsigned char *)(reg)))=(unsigned char)(data))
//#define OUT16(reg, data)		((*((volatile unsigned short int *)(reg)))=(unsigned short int)(data))
//#define OUT32(reg, data)		( (*( (volatile unsigned int *) (reg) ) ) = (unsigned int)(data) )
#define MAX_BLK_SIZE 512
#define MAX_BUF_BLOCK 2
#define SDHCI_MAX_BUF   (MAX_BLK_SIZE*MAX_BUF_BLOCK)



typedef  struct  adma2 {
    unsigned short int wAttribute;
    unsigned short int wLen;
    char * dwAddress;
} ADMA2;

struct mmc_data {
	unsigned int		timeout_ns;	/* data timeout (in ns, max 80ms) */
	unsigned int		timeout_clks;	/* data timeout (in clocks) */
	unsigned int		blksz;		/* data block size */
	unsigned int		blocks;		/* number of blocks */
	unsigned int		error;		/* data error */

	unsigned int		*buff_addr;
	ADMA2				*desc_addr;
	unsigned int		pio_size;

#if 1
	unsigned int		transtype;
#define SDMA 0
#define ADMA 1
#define PIO	 2
#endif
	unsigned int		flags;



#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

	struct mmc_command	*stop;		/* stop command */
	struct mmc_request	*mrq;		/* associated request */
};

struct mmc_command {
	unsigned int			opcode;
	unsigned int			arg;
	unsigned int			resp[4];
	unsigned int		flags;		/* expected response type */
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)
#define MMC_CMD_BCR	(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)		/* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define mmc_resp_type(cmd)	((cmd)->flags & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))

/*
 * These are the SPI response types for MMC, SD, and SDIO cards.
 * Commands return R1, with maybe more info.  Zero is an error type;
 * callers must always provide the appropriate MMC_RSP_SPI_Rx flags.
 */
#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B	(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)
#define MMC_RSP_SPI_R2	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R3	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R4	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R5	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R7	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)

#define mmc_spi_resp_type(cmd)	((cmd)->flags & \
		(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY|MMC_RSP_SPI_S2|MMC_RSP_SPI_B4))

/*
 * These are the command types.
 */
#define mmc_cmd_type(cmd)	((cmd)->flags & MMC_CMD_MASK)

	unsigned int		retries;	/* max number of retries */
	unsigned int		error;		/* command error */

/*
 * Standard errno values are used for errors, but some have specific
 * meaning in the MMC layer:
 *
 * ETIMEDOUT    Card took too long to respond
 * EILSEQ       Basic format problem with the received or sent data
 *              (e.g. CRC check failed, incorrect opcode in response
 *              or bad end bit)
 * EINVAL       Request cannot be performed because of restrictions
 *              in hardware and/or the driver
 * ENOMEDIUM    Host can determine that the slot is empty and is
 *              actively failing requests
 */

	struct mmc_data		*data;		/* data segment associated with cmd */
	//struct mmc_request	*mrq;		/* associated request */
};


/*    -----  sdhci   ----       */
void sdhci_init(void);
int sdhci_init_sdcard(void);
void sdhci_card_detect(void);
#ifdef SDCARD_TEST
int sdhci_test_write_block(unsigned int transtype,unsigned int blocks,unsigned int card_addr);
int sdhci_test_read_block(unsigned int transtype,unsigned int blocks,unsigned int card_addr);
int sdhci_test_write_and_read(unsigned int transtype,unsigned int start_addr,unsigned int end_addr);
#endif
//void sdhci_clear_set_irq(unsigned int clear,unsigned int set);
//int mmc_sd_switch(int mode, int group,char value);
//void mmc_sleep_wakeup(int mode);

#ifdef SDIO_TEST
int sdhci_test_init_sdio_card(void);
int sdhci_test_sdio_write(unsigned int transtype,unsigned int blocks,unsigned int card_addr);
int sdhci_test_sdio_read(unsigned int transtype,unsigned int blocks,unsigned int card_addr);
#endif
int sdhci_send_command(struct mmc_data *data, struct mmc_command *cmd);
unsigned int get_blk_size(void);
unsigned int get_sector_count(void);
char is_sdcard_inserted(void);
unsigned int get_sd_ccs(void);













