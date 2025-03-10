/**
 * @file cmd_common.c
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
extern CLI_DEV s_cli_dev;

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/
static int cli_echo_close(cmd_tbl_t *t, int argc, char *argv[])
{
	char i;
	CLI_DEV *p_cli_dev = &s_cli_dev;
	if (argc >= 2)
	{
		i =  (char)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD, LL_ERR, "error: no enough argc!\r\n");
		return 0;
	}
	if(i != 0 && i != 1)
	{
		os_printf(LM_CMD, LL_ERR, "input error!\r\n");
		return CMD_RET_FAILURE;
	}
	
	p_cli_dev->echo_open = i;
	p_cli_dev->prefix_open = i;
	return CMD_RET_SUCCESS;
}
CLI_CMD(echoclose, cli_echo_close, "close echo", "echoclose [i]");


static int cli_echo_close_read(cmd_tbl_t *t, int argc, char *argv[])
{
	CLI_DEV *p_cli_dev = &s_cli_dev;

	if(p_cli_dev->echo_open == 0)
	{
		os_printf(LM_CMD, LL_INFO, "0,");		
	}
	else
	{
		os_printf(LM_CMD, LL_INFO, "1,");
	}

	return CMD_RET_SUCCESS;
}
CLI_CMD(echoread, cli_echo_close_read, "echoread", "echoread");


static int cmd_help(cmd_tbl_t *h, int argc, char *argv[])
{
	cmd_tbl_t *t = NULL, *start, *end;

	if (argc == 1)
	{
		for (t = cli_cmd_start(); t < cli_cmd_end(); t++)
		{
			if (!t->desc || t->flag)
			{
				continue;
			}
			if(!t->f->used)
			{
				continue;
			}
			if(s_cli_dev.mode & t->mode)
			{
				os_printf(LM_CMD, LL_INFO, "%-20s - %s\n", t->name, t->desc);
			}
		}

		goto ret_success;
	}

	// help cmd subcmd
	t = start = cli_cmd_start();
	end = cli_cmd_end();

	t = find_cmd(argv[1]);
	if (!t)
	{
		return CMD_RET_FAILURE;
	}

	if (argc == 2 && t->flag == 0)
	{
		os_printf(LM_CMD, LL_INFO, "%-16s - %s\n", t->name, t->desc);
		if (t->usage)
		{
			os_printf(LM_CMD, LL_INFO, "Usage: %s\n", t->usage);
		}

		for (t = t + 1; (t < end) && (t->flag & CMD_ATTR_SUB); t++)
		{
			if (!t->desc)
			{
				continue;
			}
			if(!t->f->used)
			{
				continue;
			}

			os_printf(LM_CMD, LL_INFO, "\t%-16s - %-32s \t- Usage:%s\n", t->name, t->desc, t->usage);
		}
		goto ret_success;
	}

	if (argc == 3)
	{
		t = find_sub_cmd(t, argv[2]);
	}

	if (!t)
	{
		goto ret_fail;
	}

	if (t->desc)
	{
		os_printf(LM_CMD, LL_INFO, "%s\n", t->desc);
	}

	if (t->usage)
	{
		os_printf(LM_CMD, LL_INFO, "Usage: %s\n", t->usage);
	}


ret_success:
	return CMD_RET_SUCCESS;

ret_fail:
	return CMD_RET_FAILURE;
}
CLI_CMD(help, cmd_help,  "display information about CLI commands",    "help [command]");


int cmd_log_prefix(cmd_tbl_t *t, int argc, char *argv[])
{
	if(s_cli_dev.prefix_open) // if open now
	{
		os_printf(LM_CMD, LL_INFO, "close log prefix\n");
		s_cli_dev.prefix_open = 0;
	} 
	else
	{
		os_printf(LM_CMD, LL_INFO, "open log prefix\n");
		s_cli_dev.prefix_open = 1;
	}
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(log_prefix, cmd_log_prefix,  "change log prefix",    "log_prefix");


char log_level_str[LL_MAX+1][8] = {"NO", "ASSERT", "ERROR", "WARNING", "INFO", "DEBUG", "ALL"};
extern struct log_ctrl_map g_log_map[LM_MAX];
static int subcmd_level_info(cmd_tbl_t *h, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_NO, "\n");
	for(int i=0; i<LM_MAX; i++)
	{
		os_printf(LM_CMD, LL_NO, "%-6s :%d - %s\n", g_log_map[i].mod_name, g_log_map[i].level, log_level_str[g_log_map[i].level]);
	}

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(log, level_info, subcmd_level_info, "show all level", "log level_info");


static int subcmd_set_level(cmd_tbl_t *h, int argc, char *argv[])
{
	char * mode;
	char * level;
	if(argc != 3)
	{
		os_printf(LM_CMD, LL_ERR, "ARG NUM(: %d) IS ERROR! Need 3!\n", argc);
		return CMD_RET_FAILURE;
	}

	level = argv[2];
	int t_level = -1;
	if(strlen(level) == 1 && '0' <= *level && *level <= '0'+LM_MAX)
	{
		t_level = atoi(level);
	}
	else
	{
		for(int i=0; i<LL_MAX; i++)
		{
			if(strcmp(log_level_str[i], strupr(level))==0)
			{
				t_level = i;
				break;
			}
		}
		if(-1 == t_level)
		{
			os_printf(LM_CMD, LL_INFO, "err\n");
			return CMD_RET_FAILURE;
		}
	}

	mode = argv[1];
	if(strcmp("ALL", strupr(mode))==0 )
	{
		for(int i=0; i<LM_MAX; i++)
		{
			g_log_map[i].level = t_level;
		}
	}
	else
	{
		int t_mode = -1;
		for(int i=0; i<LM_MAX; i++)
		{
			if(strcmp(g_log_map[i].mod_name, strlwr(mode))==0)
			{
				t_mode = i;
				break;
			}
		}
		if(-1 == t_mode)
		{
			os_printf(LM_CMD, LL_INFO, "err\n");
			return CMD_RET_FAILURE;
		}

		g_log_map[t_mode].level = t_level;
	}

	subcmd_level_info(h, argc, argv);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(log, set_level, subcmd_set_level, "set mode log level", "log set_level <mode> <level>");

CLI_CMD(log, NULL, "log cmd", "log <subcmd>");


