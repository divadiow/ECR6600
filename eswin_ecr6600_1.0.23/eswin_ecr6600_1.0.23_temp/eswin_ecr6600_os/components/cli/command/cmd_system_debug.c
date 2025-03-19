/**
 * @file cmd_system_debug.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-9
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
#include "debug_core.h"

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
#if defined(CONFIG_HEAP_DEBUG)
int system_heap_print(cmd_tbl_t *t, int argc, char *argv[])
{
	
	if(argc>1)
	{
		heap_print(argv[1]); //pcTaskName
	}
	else
	{
		heap_print(NULL);
	}

	return CMD_RET_SUCCESS; 
}
CLI_CMD(heaptrace, system_heap_print, "heap stats", NULL);
#endif



#if defined(CONFIG_RUNTIME_DEBUG)
int system_runtime_print(cmd_tbl_t *t, int argc, char *argv[])
{
	runtime_print(argc);  //runtime_init
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(cpu, system_runtime_print, "cpu  percentage", NULL);
#endif//CONFIG_RUNTIME_DEBUG

#ifdef CONFIG_PSM_SURPORT
#endif


