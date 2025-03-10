/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-5-1
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
#include <stdlib.h>
#include "stdio.h"
#include "stdint.h"
#include "cli.h"
#include "hal_system.h"
#include "rtc.h"
#include "wdt.h"
#include "pit.h"
#include "gpio.h"
#include "chip_pinmux.h"
#include "oshal.h"
#include "aon_reg.h"
#include "hal_system.h"
#include "easyflash.h"


/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */
#define WDT_STATUS_STOP			1
#define WDT_STATUS_RUN			2
#define WDT_STATUS_STOPFEED		0

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

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/



int utest_wdt_task_handle;
int wdt_status = WDT_STATUS_STOP;

#ifdef CONFIG_ANALYZE_REBOOT_TYPE
void gpio_isr_fun()
{
	hal_system_reset(RST_TYPE_HARDWARE_REBOOT);
}
#endif


extern void hal_set_reset_type(RST_TYPE type);
void cmd_wdt_feed_task(void *arg)
{
	for(;;)
	{
		if(wdt_status != WDT_STATUS_STOPFEED)
		{
			drv_wdt_feed();
			
#ifdef CONFIG_ANALYZE_REBOOT_TYPE
			hal_system_reset_key_enable();
			hal_set_reset_type(RST_TYPE_HARDWARE_REBOOT);
#endif
		}
		else
		{
			os_printf(LM_CMD, LL_INFO, "STOP feed wdt\n");
			while(1);
		}
		os_msleep(1000);
	}
}

#ifdef CONFIG_ANALYZE_REBOOT_TYPE
void wdt_isr_func(void)
{
	T_GPIO_ISR_CALLBACK p_callback;
	p_callback.gpio_callback = gpio_isr_fun;
	p_callback.gpio_data = 0;
	
	hal_system_reset_key_disable();

	PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_GPIO18);
	drv_gpio_ioctrl(GPIO_NUM_18, DRV_GPIO_CTRL_REGISTER_ISR, (int)&p_callback);
	drv_gpio_ioctrl(GPIO_NUM_18, DRV_GPIO_CTRL_INTR_MODE, DRV_GPIO_ARG_INTR_MODE_N_EDGE);
	drv_gpio_ioctrl(GPIO_NUM_18, DRV_GPIO_CTRL_INTR_ENABLE, 1);

	hal_set_reset_type(RST_TYPE_HARDWARE_WDT_TIMEOUT);
}
#endif


WDT_RET_CODE creat_wdt_feed_task()
{
	if(wdt_status == WDT_STATUS_RUN)
	{
        os_printf(LM_CMD, LL_ERR, "wdt feed task already alive!\n");
		return WDT_RET_OK;
	}
	
#ifdef CONFIG_ANALYZE_REBOOT_TYPE
    drv_wdt_isr_register(wdt_isr_func);
    drv_wdt_start(WDT_CTRL_INTTIME_8, WDT_CTRL_RSTTIME_7, WDT_CTRL_INTENABLE, WDT_CTRL_RSTENABLE);
#endif
	
    drv_wdt_start(WDT_CTRL_INTTIME_8, WDT_CTRL_RSTTIME_7, WDT_CTRL_INTDISABLE, WDT_CTRL_RSTENABLE);
    
    utest_wdt_task_handle = os_task_create( "wdt-feed-task", 3, 1024, (task_entry_t)cmd_wdt_feed_task, NULL);
    if (utest_wdt_task_handle != 0)
    {
        os_printf(LM_CMD, LL_ERR, "create wdt-feed-task success!\n");
		wdt_status = WDT_STATUS_RUN;
    }
	else
	{
        os_printf(LM_CMD, LL_ERR, "create wdt-feed-task error!\n");
		wdt_status = WDT_STATUS_STOP;
	}
    return WDT_RET_OK;
}

WDT_RET_CODE creat_wdt_feed_task_with_nv()
{
	char value[16] = {0};
	char ret = 0;
	if((ret = develop_get_env_blob("WdtFlag", value, 1, NULL) ) == 0)
	{
		os_printf(LM_CMD, LL_ERR,"DEVELOP NV NO WdtFlag !!\n");
		return WDT_RET_ERR;
	}
	else if(ret == 0xffffffff)
	{
		os_printf(LM_CMD, LL_ERR,"NV ERROR !!\n");
		return WDT_RET_ERR;
	}
	else
	{
		if('1' == value[0])
		{
			os_printf(LM_CMD, LL_INFO,"creat_wdt_feed_task\n");
			creat_wdt_feed_task();
		}
		else
		{
			os_printf(LM_CMD, LL_INFO,"don't creat_wdt_feed_task\n");
			return WDT_RET_ERR;
		}
	}
	return WDT_RET_OK;
}

CLI_SUBCMD(wdt, start, creat_wdt_feed_task, "start wdt feed task", "wdt start");

WDT_RET_CODE subcmd_wdt_stop_feed()
{
	if(wdt_status == WDT_STATUS_RUN)
	{
		os_printf(LM_CMD, LL_INFO,"STOP FEED WDT\n");
		wdt_status = WDT_STATUS_STOPFEED;
	}
	else
	{
		os_printf(LM_CMD, LL_INFO,"wdt feed task not alive!\n");
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(wdt, stop_feed, subcmd_wdt_stop_feed, "stop feed wdt for reset", "wdt stop_feed");

WDT_RET_CODE subcmd_wdt_stop_fun()
{
	if(wdt_status == WDT_STATUS_RUN)
	{
		os_task_delete(utest_wdt_task_handle);
		drv_wdt_stop();
		wdt_status = WDT_STATUS_STOP;
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(wdt, stop, subcmd_wdt_stop_fun, "stop wdt", "wdt stop");

WDT_RET_CODE subcmd_wdt_get_fun()
{
	switch(wdt_status)
	{
		case WDT_STATUS_STOPFEED:
			os_printf(LM_CMD, LL_INFO, "wdt stop feed\n"); break;
		case WDT_STATUS_STOP:
			os_printf(LM_CMD, LL_INFO, "wdt stop\n"); break;
		case WDT_STATUS_RUN:
			os_printf(LM_CMD, LL_INFO, "wdt feed task run\n"); break;
		default:
			os_printf(LM_CMD, LL_INFO, "wdt status err\n"); break;
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(wdt, get, subcmd_wdt_get_fun, "get wdt status", "wdt get");



CLI_CMD(wdt, NULL, "Watch Dog Timer", "wdt <sub cmd>");







//static int set_reset_type(cmd_tbl_t *t, int argc, char *argv[])
//{
//	if(argc != 2)
//	{
//		cli_printf("need 1 parameter\r\n");
//		return CMD_RET_FAILURE;
//	}
//	RST_TYPE temp = atoi(argv[1]);
//	cli_printf("set_reset_type\r\n");
//	drv_pit_delay(100000);
//	switch(temp)
//	{
//		case RST_TYPE_POWER_ON: hal_system_reset(RST_TYPE_POWER_ON); break;
//    	case RST_TYPE_FATAL_EXCEPTION: hal_system_reset(RST_TYPE_FATAL_EXCEPTION); break;
//    	case RST_TYPE_SOFTWARE_REBOOT: hal_system_reset(RST_TYPE_SOFTWARE_REBOOT); break;
//    	case RST_TYPE_HARDWARE_REBOOT: hal_system_reset(RST_TYPE_HARDWARE_REBOOT); break;
//    	case RST_TYPE_OTA: hal_system_reset(RST_TYPE_OTA); break;
//    	case RST_TYPE_WAKEUP: hal_system_reset(RST_TYPE_WAKEUP); break;
//    	case RST_TYPE_SOFTWARE: hal_system_reset(RST_TYPE_SOFTWARE); break;
//    	case RST_TYPE_HARDWARE_WDT_TIMEOUT: hal_system_reset(RST_TYPE_HARDWARE_WDT_TIMEOUT); break;
//    	case RST_TYPE_SOFTWARE_WDT_TIMEOUT: hal_system_reset(RST_TYPE_SOFTWARE_WDT_TIMEOUT); break;
//		case RST_TYPE_UNKOWN: hal_system_reset(RST_TYPE_UNKOWN); break;
//		default:
//			cli_printf("RST_TYPE default\r\n");
//			hal_system_reset(temp);
//			break;
//	}
//	return CMD_RET_SUCCESS;
//}
//CLI_CMD(s_reset_t, set_reset_type,  "",    "");


