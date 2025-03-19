/**
 * @file dnv.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-8
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
	
#include "cli.h"
#include "nvds.h"

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
#ifdef CONFIG_COMP_NVDS
static int put(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
	char key;
	char * value;

	if (argc < 3) {
		os_printf(LM_CMD, LL_ERR, "put [key] [value]\n");
		return CMD_RET_FAILURE;
	}
	
	key = strtoul(argv[1], NULL, 0);
	value = argv[2];
	ret = nvds_put(key, (nvds_tag_len_t)strlen(value), (unsigned char * )value);
	if(ret)
	{
		os_printf(LM_CMD, LL_ERR, "nvgs put error,ret = %d !\n",ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD, LL_INFO, "nvds_put: key[0x%x] value = %s\n",key,value);

	return CMD_RET_SUCCESS;
}
CLI_CMD(put, put, "nvds put", "put [key] [value]");

static int get(cmd_tbl_t *t, int argc, char *argv[])
{
	char addr = 128;
	unsigned char value[128];
	char key;
	int ret = 0;
	
	if (argc < 2) {
		
		os_printf(LM_CMD, LL_ERR, "get [key]\n");
		return CMD_RET_FAILURE;
	}
	
	memset( value, 0, strlen((char *)value));
	key = strtoul(argv[1], NULL, 0);

	ret = nvds_get(key, (nvds_tag_len_t *)&addr, (unsigned char *)value);
	if(ret)
	{
		os_printf(LM_CMD, LL_ERR, "nvgs get error,ret = %d !\n",ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD, LL_INFO, "nvds get key: 0x%x, value: %s\n", key, value);

	return CMD_RET_SUCCESS;
}
CLI_CMD(get, get, "nvds get", "get [key]");

static int del(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char key;
	
	if (argc < 2) {
		
		os_printf(LM_CMD, LL_ERR, "del [key]\n");
		return CMD_RET_FAILURE;
	}

	
	key = strtoul(argv[1], NULL, 0);

	ret = nvds_del(key);
	if(ret)
	{
		os_printf(LM_CMD, LL_ERR, "nvgs del error,ret = %d !\n",ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD, LL_INFO, "nvds del key: 0x%x\n", key);

	return CMD_RET_SUCCESS;
}
CLI_CMD(del, del, "nvds del", "del [key]");
#endif



//nv tese CMD
//*********************************    nv customer    *********************************//
static int cmd_CustomerNVDel(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key;
	//char * value;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	ret = customer_del_env(key);
	if(ret)
	{
		os_printf(LM_CMD, LL_ERR, "NV DEL ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "NV DEL key: %s\n", key);
	return CMD_RET_SUCCESS;
}

CLI_CMD(cnvd, cmd_CustomerNVDel,  "delete customer nv",  "nvd <key>");

static int cmd_CustomerNVSet(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key, *value;

	if(argc != 3)
	{
		os_printf(LM_CMD, LL_ERR, "NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];
	value = argv[2];

	ret = customer_set_env_blob(key, value, strlen(value));
	if(ret)
	{
		os_printf(LM_CMD, LL_ERR, "NV SET ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "NV SET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(cnvs, cmd_CustomerNVSet,  "setting customer nv value",  "nvs <key> <value>");

static int cmd_CustomerNVGet(cmd_tbl_t *t, int argc, char *argv[])
{
	char * key;
	char value[256];

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "NV GET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	if(strcmp("all", key)==0)
	{
		customer_print_env();
		return CMD_RET_SUCCESS;
	}

	memset(value, 0, 256);

	if(customer_get_env_blob(key, value, 256, NULL) == 0)
	{
		os_printf(LM_CMD, LL_ERR, "NV GET FAILED, NO SUCH NV!!\n");
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "NV GET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(cnvg, cmd_CustomerNVGet,  "getting customer nv value",  "nvg <key>");




//*********************************    nv develop    *********************************//
static int cmd_DevelopNVDel(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key;
	//char * value;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "DEVELOP NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	ret = develop_del_env(key);
	if(ret)
	{
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD, LL_ERR, "DEVELOP NV DEL ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "DEVELOP NV DEL key: %s\n", key);
	return CMD_RET_SUCCESS;
}

CLI_CMD(dnvd, cmd_DevelopNVDel,  "delete develop nv",  "dnvd <key>");


static int cmd_DevelopNVSet(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key;
	char * value;

	if(argc != 3)
	{
		os_printf(LM_CMD, LL_ERR, "DEVELOP NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];
	value = argv[2];

	ret = develop_set_env_blob(key, value, strlen(value));
	
	if(ret)
	{
	
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD, LL_ERR, "DEVELOP NV SET ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "DEVELOP NV SET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(dnvs, cmd_DevelopNVSet,  "setting develop nv value",  "dnvs <key> <value>");


static int cmd_DevelopNVGet(cmd_tbl_t *t, int argc, char *argv[])
{
	char * key;
	char value[256];
	char ret = 0;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "DEVELOP NV GET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	if(strcmp("all", key)==0)
	{
		if(develop_get_env_blob(key, value, 256, NULL)== 0xffffffff)	
			return CMD_RET_FAILURE;
		develop_print_env();
		return CMD_RET_SUCCESS;
	}

	memset(value, 0, 256);

	if((ret = develop_get_env_blob(key, value, 256, NULL) )== 0)
	{
		os_printf(LM_CMD, LL_ERR, "DEVELOP NV GET FAILED, NO SUCH NV!!\n");
		return CMD_RET_FAILURE;
	}
	if(ret == 0xffffffff)
		return CMD_RET_FAILURE;

	os_printf(LM_CMD, LL_INFO, "DEVELOP NV GET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(dnvg, cmd_DevelopNVGet,  "getting develop nv value",  "dnvg <key>");




//*********************************    nv amt    *********************************//
static int cmd_AmtNVDel(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key;
	//char * value;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "AMT NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	ret = amt_del_env(key);
	if(ret)
	{
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD, LL_ERR, "AMT NV DEL ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "AMT NV DEL key: %s\n", key);
	return CMD_RET_SUCCESS;
}

CLI_CMD(anvd, cmd_AmtNVDel,  "delete amt nv",  "anvd <key>");


static int cmd_AmtNVSet(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	char * key, *value;

	if(argc != 3)
	{
		os_printf(LM_CMD, LL_ERR, "AMT NV SET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];
	value = argv[2];

	ret = amt_set_env_blob(key, value, strlen(value));
	
	if(ret)
	{
	
		if(ret != EF_WRITE_ERR)
			os_printf(LM_CMD, LL_ERR, "AMT NV SET ERROR(%d)!!\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD, LL_INFO, "AMT NV SET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(anvs, cmd_AmtNVSet,  "setting amt nv value",  "anvs <key> <value>");


static int cmd_AmtNVGet(cmd_tbl_t *t, int argc, char *argv[])
{
	char * key;
	char value[256];
	char ret = 0;

	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "AMT NV GET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];

	if(strcmp("all", key)==0)
	{
		if(amt_get_env_blob(key, value, 256, NULL)== 0xffffffff)	
			return CMD_RET_FAILURE;
		extern void amt_print_env(void);
		amt_print_env();
		return CMD_RET_SUCCESS;
	}

	memset(value, 0, 256);

	if((ret = amt_get_env_blob(key, value, 256, NULL) )== 0)
	{
		os_printf(LM_CMD, LL_ERR, "AMT NV GET FAILED, NO SUCH NV!!\n");
		return CMD_RET_FAILURE;
	}
	if(ret == 0xffffffff)
		return CMD_RET_FAILURE;

	os_printf(LM_CMD, LL_INFO, "AMT NV GET key: %s, value: %s\n", key, value);
	return CMD_RET_SUCCESS;
}

CLI_CMD(anvg, cmd_AmtNVGet,  "getting amt nv value",  "anvg <key>");


static int cmd_PartionGet(cmd_tbl_t *t, int argc, char *argv[])
{
	char * key;
	unsigned int base, len;
	
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "PARTION GET ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	key = argv[1];
	os_printf(LM_CMD, LL_INFO, "\n");

	if(strcmp("all", key)==0)
	{
		partion_print_all();
		return CMD_RET_SUCCESS;
	}

	partion_info_get(key, &base, &len);

	os_printf(LM_CMD, LL_INFO, "PARTION key: %15s, base: 0x%08x, len: %08d\n", key, base, len);
	return CMD_RET_SUCCESS;
}

CLI_CMD(part, cmd_PartionGet,  "getting partition information",  "part <key>/<all>");


