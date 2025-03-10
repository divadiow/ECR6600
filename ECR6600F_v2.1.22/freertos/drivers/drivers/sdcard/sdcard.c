
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "arch_irq.h"
//#include "co_int.h"

#include "sdhci.h"
#include "sdcard.h"
#include "chip_irqvector.h"
#include "diskio.h"

//BYTE *sdcard_buff=(BYTE *)0xa0000;
BYTE sdcard_buff[SDHCI_MAX_BUF] __attribute__((section(".dma.data"),aligned(4)));


int sdcard_write(const BYTE *buff,LBA_t sector,UINT count)
{
	struct mmc_command sd_cmd;
	struct mmc_data sd_data;
	memset(&sd_cmd, 0, sizeof(sd_cmd));
	memset(&sd_data, 0, sizeof(sd_data));

	memcpy(sdcard_buff, buff, count*512);

	sd_data.blksz=512;
	sd_data.blocks=count;
	sd_data.buff_addr = (unsigned int *)sdcard_buff;
	//sd_data.desc_addr = (ADMA2 *)sd_adma_desc; //define in controller
	sd_data.flags = MMC_DATA_WRITE;
	sd_data.transtype = ADMA;

	sd_cmd.data = &sd_data;
	if(count == 1)
		sd_cmd.opcode = 24;
	else
		sd_cmd.opcode = 25;

	if(sdcard_get_ccs() == 0)
		sd_cmd.arg = sector*512;
	else
		sd_cmd.arg = sector;
	sd_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	if(sdhci_send_command(&sd_data,&sd_cmd))
		return RES_ERROR;
	else
		return RES_OK;
	
}

int sdcard_read(const BYTE *buff,LBA_t sector,UINT count)
{
	struct mmc_command sd_cmd;
	struct mmc_data sd_data;
	memset(&sd_cmd, 0, sizeof(sd_cmd));
	memset(&sd_data, 0, sizeof(sd_data));

	sd_data.blksz=512;
	sd_data.blocks=count;
	sd_data.buff_addr = (unsigned int *)sdcard_buff;
	sd_data.flags = MMC_DATA_READ;
	sd_data.transtype = ADMA;

	sd_cmd.data = &sd_data;
	
	if(count == 1)
		sd_cmd.opcode = 17;
	else
		sd_cmd.opcode = 18;

	if(sdcard_get_ccs() == 0)
		sd_cmd.arg = sector*512;
	else
		sd_cmd.arg = sector;
	sd_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	if(sdhci_send_command(&sd_data,&sd_cmd))
		{
		return RES_ERROR;
		}
	else
		{
		memcpy((void *)buff, sdcard_buff, count*512);
		return RES_OK;
		}
}

int sdcard_init()
{
	int ret;
	ret = sdhci_init_sdcard();
	
	if(ret)
		os_printf(LM_OS, LL_INFO, "sdcard_init failed %d\r\n",ret);
		
	return(ret);
}

unsigned int sdcard_get_blk_size()
{
	return(get_blk_size());
}

unsigned int sdcard_get_sector_count()
{
	return(get_sector_count());
}

int sdcard_status()
{
	if(is_sdcard_inserted())
		return 0;
	else
		return -1;
}

unsigned int sdcard_get_ccs()
{
	return(get_sd_ccs());
}

