#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "oshal.h"
#include "hal_system.h"
#include "rtos_debug.h"
#include "telnet.h"
#include "hal_wdt.h"

static int wdt_reboot_func(cmd_tbl_t *t, int argc, char *argv[])
{
	if(2 == argc && 0 == strcmp(argv[1], "0"))
	{
		void (*function)(void *) = 0;
		function(NULL);
	}
	
	hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);

	return CMD_RET_SUCCESS;
}
CLI_CMD(reboot, wdt_reboot_func,  "reboot",    "reboot");


int cmd_assert(cmd_tbl_t *t, int argc, char *argv[])
{
	hal_wdt_stop();
	system_assert(0);
	return CMD_RET_SUCCESS;
}
CLI_CMD(assert, cmd_assert, "assert for debug",NULL);


#ifdef CONFIG_TELNET
static int telnet_cmd(cmd_tbl_t *h, int argc, char *argv[])
{
	extern uint8_t telnet_is_inited;
	if(telnet_is_inited)
	{
		os_printf(LM_APP,LL_INFO,"telnet is running\r\n");
	}
	else
	{	
		if(telnet_init())
		{
			telnet_is_inited=1;
		}
		else
		{
			
		}
	}
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(telnet,    telnet_cmd, "run telnet", "no");
#endif

#ifdef CONFIG_SYSTEM_IRQ
static int sys_irq_cmd(cmd_tbl_t *h, int argc, char *argv[])
{
	sys_irq_show();
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(sys_irq,   sys_irq_cmd, "sys irq", "no");
#endif


