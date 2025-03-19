#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "oshal.h"

#if defined(CONFIG_CMD_READ)

#define WORD_SIZE			sizeof(unsigned int)
//#define READ_REG(reg)         (*(volatile unsigned int*)(reg))

static int cmd_read(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr, size , offset;

	if (argc != 3)
	{
		return CMD_RET_USAGE;
	}

	addr = (unsigned int)strtoul(argv[1], NULL, 0);
	size = atoi(argv[2]);

	// forward alignment
	offset = addr % WORD_SIZE;
	addr -= offset;
	size += offset;

	// backward alignment
	offset = size % WORD_SIZE;
	if (offset)
	{
		size += WORD_SIZE - offset;
	}

	while(size)
	{
		os_printf(LM_CMD, LL_INFO, "%08x: %08x\n", addr, READ_REG(addr));
		addr += WORD_SIZE;
		size -= WORD_SIZE;
	}

	return CMD_RET_SUCCESS;
}


CLI_CMD(read,cmd_read, "read 32-bit value(s) from a memory location",    "read <address> <size in byte>");


#endif //CONFIG_CMD_READ
