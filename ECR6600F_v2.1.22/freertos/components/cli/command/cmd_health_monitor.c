/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author wangc
 * @date 2021-5-28
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

#include "health_monitor.h"
#include "cli.h"
#include "oshal.h"

/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/

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

int test_handle_1;
int test_handle_2;

void healthmon_task1(void *data)
{  
	while(1)
	{
		health_mon_update(test_handle_1);
		os_msleep(10000);
		os_printf(LM_CMD, LL_INFO, "subcmd_hm_test good-task sche\n"); 
   	}
}


static int subcmd_hm_test1(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "subcmd_hm_test good-task\n"); 
	test_handle_1 = os_task_create("healthmon1", 1, 1536, healthmon_task1, NULL);
	health_mon_add(test_handle_1);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, testok, subcmd_hm_test1, "health mon test", "hm testok");


void healthmon_task2(void *data)
{
	int cnt = 5;
	while(1)
	{
		health_mon_update(test_handle_2);
		if (cnt)
		{
			os_msleep(5000);
		}
		else
		{
			while(1);
		}
		os_printf(LM_CMD, LL_INFO, "subcmd_hm_test sick-task sche-cnt%d\n", cnt--); 
   	}
}


static int subcmd_hm_test2(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "subcmd_hm_test sick-task\n"); 
	test_handle_2 = os_task_create("healthmon2", 1, 1536, healthmon_task2, NULL);
	health_mon_add(test_handle_2);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, testsick, subcmd_hm_test2, "health mon test", "hm testsick");


static int subcmd_hm_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "subcmd_hm_stop\n"); 
	health_mon_stop();
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, stop, subcmd_hm_stop, "health mon stop", "hm stop");

static int subcmd_hm_start(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "subcmd_hm_start\n"); 
	health_mon_start();
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, start, subcmd_hm_start, "health mon start", "hm start");



static int subcmd_hm_init(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "subcmd_hm_init\n"); 
	health_mon_init();
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, ini, subcmd_hm_init, "health mon init", "hm ini");

static int subcmd_hm_rst(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int rst = (unsigned int)strtoul(argv[1], NULL, 0);
	if (rst)
	{
		os_printf(LM_CMD, LL_INFO, "health monitor entry reset mode\n"); 
	}
	else
	{
		os_printf(LM_CMD, LL_INFO, "health monitor entry debug mode\n"); 
	}
	health_mon_debug(rst);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(hm, rst, subcmd_hm_rst, "health mon reset, 1 is reset, 0 is not reset", "hm rst [1 or 0]");


CLI_CMD(hm, NULL, "task health monitor based on wdt", "hm");

