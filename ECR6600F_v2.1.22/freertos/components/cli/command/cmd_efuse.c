/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-8-12
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uart.h"
#include "cli.h"
#include "efuse.h"
#include "trng.h"

/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/
	
#define EFUSE_DEFAULT_ADDR (0xFFFFFFFFU)

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

static unsigned int g_efuse_w_addr = EFUSE_DEFAULT_ADDR;
static unsigned int g_efuse_w_value = 0;
static unsigned int g_efuse_w_mask = 0;
// AES key used
static unsigned int g_efuse_AES_KEY_buff[4] = {0};
static unsigned int g_efuse_sw_len = 0;

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/
	
extern void efuse_write(unsigned int addr, unsigned int value, unsigned int mask);
extern void efuse_write_series(unsigned int addr, unsigned int *value, unsigned int mask, unsigned int length);
extern void efuse_read_series(unsigned int addr,unsigned int * value, unsigned int length);

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/
	
static int cmd_efuse_read(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr = strtoul(argv[1], NULL, 0);
	unsigned int length = strtoul(argv[2], NULL, 0);
	unsigned int len_4 = length>>2;
	unsigned int value[32] = {0};
	int i = 0;

	if(addr > 0x7c || addr%4){
		os_printf(LM_CMD, LL_ERR, "Addr error! Addr < 0x7c and should be a multiple of 4\n");
		return CMD_RET_FAILURE;
	}
	if((addr+length) > 0x80 || length%4){
		os_printf(LM_CMD, LL_ERR, "Length error! Addr+length <= 0x80 and should be a multiple of 4\n");
		return CMD_RET_FAILURE;
	}

	drv_efuse_read(addr, value, length);
	for(i=0;i<len_4;i++)
	{
		if(i==len_4-1)
		{
			os_printf(LM_CMD, LL_INFO, "0x%08x\n", value[i]);
		}
		else
		{
			os_printf(LM_CMD, LL_INFO, "0x%08x, ", value[i]);
		}
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(efuse, read, cmd_efuse_read,  "efuse read",  "efuse read <addr> <length>");

static int cmd_efuse_write(cmd_tbl_t *t, int argc, char *argv[])
{
	g_efuse_w_addr  = strtoul(argv[1], NULL, 0);
	g_efuse_w_value = strtoul(argv[2], NULL, 0);
	g_efuse_w_mask  = strtoul(argv[3], NULL, 0);

	if(g_efuse_w_addr < 0x20 || g_efuse_w_addr > 0x7c || g_efuse_w_addr%4){
		os_printf(LM_CMD, LL_ERR, "Addr error! 0x20 < ddr < 0x7c and should be a multiple of 4\n");
		g_efuse_w_addr = EFUSE_DEFAULT_ADDR;
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "Need 1.8V power!!!\n");
	os_printf(LM_CMD, LL_INFO, "Will write value:0x%08x to addr:0x%08x, input 'efuse write_confirmation' to write to efuse\n", 
		g_efuse_w_value&~g_efuse_w_mask, g_efuse_w_addr);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(efuse, write, cmd_efuse_write,  "efuse write",	"efuse write <addr> <value> <mask>");

static int cmd_efuse_write_aes_trng(cmd_tbl_t *t, int argc, char *argv[])
{
	drv_efuse_read(EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET, g_efuse_AES_KEY_buff, 4);
	if(g_efuse_AES_KEY_buff[0]==0x0U && g_efuse_AES_KEY_buff[1]==0x0U && g_efuse_AES_KEY_buff[2]==0x0U && g_efuse_AES_KEY_buff[3]==0x0U)
	{
		for(int k_i=0; k_i<4; k_i++)
		{
			g_efuse_AES_KEY_buff[k_i]=drv_trng_get();
		}
		g_efuse_w_addr = EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET;
		g_efuse_sw_len = 4;
		os_printf(LM_CMD, LL_INFO, "Need 1.8V power!!!\n");
		os_printf(LM_CMD, LL_INFO, "Will write AES key:0x%08x,0x%08x,0x%08x,0x%08x, input 'efuse write_confirmation' to write to efuse\n", 
			g_efuse_AES_KEY_buff[0],g_efuse_AES_KEY_buff[1],g_efuse_AES_KEY_buff[2],g_efuse_AES_KEY_buff[3]);
		return CMD_RET_SUCCESS;
	}
	else
	{
		os_printf(LM_CMD, LL_INFO, "Already have an AES key:0x%08x,0x%08x,0x%08x,0x%08x\n",
			g_efuse_AES_KEY_buff[0],g_efuse_AES_KEY_buff[1],g_efuse_AES_KEY_buff[2],g_efuse_AES_KEY_buff[3]);
		g_efuse_w_addr = EFUSE_DEFAULT_ADDR;
		g_efuse_sw_len = 0;
		return CMD_RET_FAILURE;
	}
}
CLI_SUBCMD(efuse, write_AES_TRNG, cmd_efuse_write_aes_trng,  "efuse write aes key from TRNG",	"efuse write_AES_TRNG");

static int cmd_efuse_write_confirmation(cmd_tbl_t *t, int argc, char *argv[])
{
	if(EFUSE_DEFAULT_ADDR == g_efuse_w_addr)
	{
		os_printf(LM_CMD, LL_INFO, "Need write info, use 'efuse write <addr> <value> <mask>'\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		if(4==g_efuse_sw_len && g_efuse_w_addr==EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET)
		{
			drv_efuse_write(EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET, g_efuse_AES_KEY_buff[0], 0);
			drv_efuse_write(EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET+4, g_efuse_AES_KEY_buff[1], 0);
			drv_efuse_write(EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET+8, g_efuse_AES_KEY_buff[2], 0);
			drv_efuse_write(EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET+12, g_efuse_AES_KEY_buff[3], 0);
		}
		else
		{	
			drv_efuse_write(g_efuse_w_addr, g_efuse_w_value, g_efuse_w_mask);
		}
		os_printf(LM_CMD, LL_INFO, "Power down to make write effective\n");
		g_efuse_w_addr = EFUSE_DEFAULT_ADDR;
		g_efuse_sw_len = 0;
		return CMD_RET_SUCCESS;
	}
}
CLI_SUBCMD(efuse, write_confirmation, cmd_efuse_write_confirmation,  "efuse write confirmation",	"efuse write_confirmation");


CLI_CMD(efuse, NULL,  "efuse read/write",	"efuse read/write ...");


