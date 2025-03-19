#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cli.h"
#include "oshal.h"
#include "hal_efuse.h"

static int utest_efuse_read(cmd_tbl_t *t, int argc, char *argv[])
{
 	unsigned int addr = strtoul(argv[1], NULL, 0);
	unsigned int length = strtoul(argv[2], NULL, 0);
	unsigned char value[100] = {0};
	int ret,i = 0;

	if (argc == 3)
	{
		addr  = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_efuse read [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}

	ret = hal_efuse_read(addr, (unsigned int *)value, length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"efuse_read error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	for(i=0;i<length;i++)
	{
		os_printf(LM_CMD,LL_INFO,"data[%d] = %x\n",i,value[i]);
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_efuse, read, utest_efuse_read, "unit test efuse read", "ut_efuse read [address] [length]");


static int utest_efuse_write(cmd_tbl_t *t, int argc, char *argv[])
{
 	unsigned int addr;
	unsigned int value;
	unsigned int mask;
	int ret;
	
	if (argc == 4)
	{
		addr  = strtoul(argv[1], NULL, 0);
		value = strtoul(argv[2], NULL, 0);
		mask  = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_efuse write [address] [value] [mask]\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = hal_efuse_write(addr, value, mask);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"efuse_write error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	os_printf(LM_CMD,LL_INFO,"efuse_write,addr = 0x%x  value = 0x%x mask = 0x%x\n", addr, value, mask);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_efuse, write, utest_efuse_write, "unit test efuse write", "ut_efuse write [address] [value] [mask]");



CLI_CMD(ut_efuse, NULL, "unit test efuse", "test_efuse");


