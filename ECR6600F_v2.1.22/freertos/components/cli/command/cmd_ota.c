/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-4-22
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

#include "mqtt_client.h"
#include "oshal.h"
#include "os.h"
#include "cli.h"
#include "easyflash.h"
#include "hal_system.h"

#include "local_ota.h"
#include "mqtt_ota.h"
#include "http_ota.h"
#include "ota.h"

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

int cmd_get_sn(cmd_tbl_t *h, int argc, char *argv[])
{
	char value[OTA_SN_LEN];
	char ret = 0;

	memset(value, 0, OTA_SN_LEN);
	
	if((ret = amt_get_env_blob(OTA_SN, value, OTA_SN_LEN, NULL) ) == 0)
	{
		os_printf(LM_CMD, LL_INFO, "NO %s!!\n", OTA_SN);
		return CMD_RET_FAILURE;
	}
	if(ret == 0xffffffff)
		return CMD_RET_FAILURE;

	os_printf(LM_CMD, LL_INFO, "%s is %s\n", OTA_SN, value);
	return CMD_RET_SUCCESS;
}
CLI_CMD(get_sn, cmd_get_sn, "get eswin sn", "get_sn");



int cmd_set_sn(cmd_tbl_t *h, int argc, char *argv[])
{	
	int ret;
	char *value;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	value = argv[1];

	ret = amt_set_env_blob(OTA_SN, value, strlen(value));
	
	if(ret)
	{
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD,LL_INFO, "SET ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD,LL_INFO, "%s is %s\n", OTA_SN, value);
	return CMD_RET_SUCCESS;
}
CLI_CMD(set_sn, cmd_set_sn, "set eswin sn", "set_sn 123456");



int cmd_get_project_id(cmd_tbl_t *h, int argc, char *argv[])
{	
	char value[OTA_PROJECT_ID_LEN];
	char ret = 0;

	memset(value, 0, OTA_PROJECT_ID_LEN);
	
	if((ret = amt_get_env_blob(OTA_PROJECT_ID, value, OTA_PROJECT_ID_LEN, NULL) )== 0)
	{
		os_printf(LM_CMD,LL_INFO, "NO %s!!\n", OTA_PROJECT_ID);
		return CMD_RET_FAILURE;
	}
	if(ret == 0xffffffff)
		return CMD_RET_FAILURE;

	os_printf(LM_CMD,LL_INFO, "%s is %s\n", OTA_PROJECT_ID, value);
	return CMD_RET_SUCCESS;
}
CLI_CMD(get_project_id, cmd_get_project_id, "get eswin project id", "get_project_id");



int cmd_set_project_id(cmd_tbl_t *h, int argc, char *argv[])
{	
	int ret;
	char *value;

	if(argc != 2)
	{
		os_printf(LM_CMD,LL_INFO, "SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	value = argv[1];

	ret = amt_set_env_blob(OTA_PROJECT_ID, value, strlen(value));
	
	if(ret)
	{
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD,LL_INFO, "SET ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD,LL_INFO, "%s is %s\n", OTA_PROJECT_ID, value);
	return CMD_RET_SUCCESS;
}
CLI_CMD(set_project_id, cmd_set_project_id, "set eswin project id", "set_project_id led");



//int cmd_service_ota_start(cmd_tbl_t *h, int argc, char *argv[])
//{
//	service_ota_main();
//	
//	return CMD_RET_SUCCESS;
//}
//CLI_CMD(service_ota_start, cmd_service_ota_start, "ota start use mqtt", "service_ota_start");


