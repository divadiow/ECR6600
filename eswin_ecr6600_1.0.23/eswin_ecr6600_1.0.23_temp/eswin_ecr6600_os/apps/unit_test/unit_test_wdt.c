#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "hal_wdt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"

int utest_wdt_task_handle;

#if 0
void wdt_isr_handler(void)
{
	os_printf(LM_CMD, LL_INFO, "\r\nunit test wdt, wdt isr comes!\r\n");
}

static int utest_wdt_isr(void)
{
	drv_wdt_start(WDT_CTRL_INTTIME_8, WDT_CTRL_RSTTIME_0, WDT_CTRL_INTENABLE, WDT_CTRL_RSTDISABLE);
	drv_wdt_isr_register(wdt_isr_handler);
}

CLI_SUBCMD(ut_wdt, isr, utest_wdt_isr, "unit test wdt isr", "ut_wdt isr");
#endif


void utest_wdt_feed_task(void *arg)
{
	int count = 0;
	for(;;)
	{
		os_printf(LM_CMD,LL_INFO,"entry wdt_feed_task %d\r\n", count++);
		hal_wdt_feed();
		os_msleep(1000);
	}
}

static int utest_wdt_init(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int time; 
	unsigned int effective_time;
	if (argc >= 3)
	{
		time = strtoul(argv[1], NULL, 0);
		effective_time = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, err: no enough argt!\r\n");
		return 0;
	}

	if (hal_wdt_init(time, &effective_time) == 0)
	{	
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt init success!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt init failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_wdt, init, utest_wdt_init, "unit test wdt init", "ut_wdt init [time] [effective_time]");

static int utest_wdt_start(cmd_tbl_t *t, int argc, char *argv[])
{
	if (hal_wdt_start() == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt start success!\r\n");	 
		os_printf(LM_CMD,LL_INFO,"\r\nsend wdt feed cmd!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt start failed!\r\n");	
	}

	return 0;
}

CLI_SUBCMD(ut_wdt, start, utest_wdt_start, "unit test wdt start", "ut_wdt start");

static int utest_wdt_feed(cmd_tbl_t *t, int argc, char *argv[])
{
	utest_wdt_task_handle = os_task_create( "wdt-feed-task", 3, 1024, (task_entry_t)utest_wdt_feed_task, NULL);
	if (utest_wdt_task_handle != 0)
	{
		os_printf(LM_CMD,LL_INFO,"create wdt-feed-task success!\r\n");
	}
	return 0;
}

CLI_SUBCMD(ut_wdt, feed, utest_wdt_feed, "unit test wdt feed", "ut_wdt feed");

static int utest_wdt_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	if (hal_wdt_stop() == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt stop success!\r\n");	 
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test wdt, wdt stop failed!\r\n");	
	}

	if (utest_wdt_task_handle != 0)
	{
    	os_task_delete(utest_wdt_task_handle);
		os_printf(LM_CMD,LL_INFO,"stop wdt task ok!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_wdt, stop, utest_wdt_stop, "unit test wdt stop", "ut_wdt stop");

CLI_CMD(ut_wdt, NULL, "unit test wdt", "test_wdt");





