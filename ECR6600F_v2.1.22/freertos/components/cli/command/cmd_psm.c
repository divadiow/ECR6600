/**
 * @file cmd_psm.c
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
#include "psm_system.h"
#include "psm_mode_ctrl.h"

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

static int psm_dbg_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_APP, LL_INFO, "psm_dbg_cmd(%d)=%s\r\n",argc - 1,argv + 1);
	psm_dbg_func_cb(argc - 1, argv + 1);
	return CMD_RET_SUCCESS;
}
CLI_CMD(psm, psm_dbg_cmd, "PSM Debug", "\n\tpsm set [modemsleep/lightsleep/deepsleep/off] [listen_interval/deepsleep_time(s)\n\tpsm pt [off/time(s)]\n\tpsm ls [3/5(modem/light)/sleep_time(s)]\n\tpsm show");




