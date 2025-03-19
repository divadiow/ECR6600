#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "oshal.h"

#if defined(CONFIG_CMD_READ)

#ifndef WRITE_REG
#define WRITE_REG(offset,value) (*(volatile unsigned int *)(offset) = (unsigned int)(value));
#endif

#define WR_ADDR_CHEAK(X)		((X) & 3)

static int cmd_write(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr, val;

	if (argc != 3)
	{
		return CMD_RET_USAGE;
	}

	addr = strtoul(argv[1], NULL, 0);	
	
	if (WR_ADDR_CHEAK(addr))
	{
		os_printf(LM_CMD, LL_ERR,"write err: addr misalignment!\n");
		return CMD_RET_USAGE;
	}
	
	val = strtoul(argv[2], NULL, 0);

	WRITE_REG(addr, val);

	return CMD_RET_SUCCESS;
}


CLI_CMD(write, cmd_write,   "write a 32-bit value to a memory location",    "write <address> <data>");

#endif //CONFIG_CMD_READ

