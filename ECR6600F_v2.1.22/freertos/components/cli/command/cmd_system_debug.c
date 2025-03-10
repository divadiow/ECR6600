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
CLI_SUBCMD(os_debug, heaptrace, system_heap_print, "heap stats", "os_debug heaptrace [NONE]/[task name]");
#endif



#if defined(CONFIG_RUNTIME_DEBUG)
int system_runtime_print(cmd_tbl_t *t, int argc, char *argv[])
{
	runtime_print(argc);  //runtime_init

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(os_debug, cpu, system_runtime_print, "cpu percentage", "os_debug cpu [NONE]/[reset]");
#endif //CONFIG_RUNTIME_DEBUG



#if defined(CONFIG_TASK_IRQ_RUN_NUM)
extern
int task_irq_num_fun(cmd_tbl_t *t, int argc, char *argv[])
{
    extern unsigned int g_uTaskRunNum[MAX_TASK_NUM+1];
    extern unsigned int g_uIrqRunNum[32];
    for(int i=0; i<MAX_TASK_NUM+1; i++){
        if(g_uTaskRunNum[i] != 0){
            os_printf(LM_OS, LL_INFO, "PID:%d\t%d\n", i, g_uTaskRunNum[i]);
        }
    }
    for(int i=0; i<32; i++){
        if(g_uIrqRunNum[i] != 0){
            os_printf(LM_OS, LL_INFO, "IRQ:%d\t%d\n", i, g_uIrqRunNum[i]);
        }
    }
    if(argc>1) {
        memset(g_uTaskRunNum, 0, sizeof(g_uTaskRunNum));
        memset(g_uIrqRunNum, 0, sizeof(g_uIrqRunNum));
    }
    return CMD_RET_SUCCESS;
}
CLI_SUBCMD(os_debug, switch_num, task_irq_num_fun, "task/irq run num", "os_debug switch_num [NONE]/[reset]");
#endif //CONFIG_TASK_IRQ_RUN_NUM



#if defined(CONFIG_FLASH_DUMP)
static int flash_dump_fun(cmd_tbl_t *h, int argc, char *argv[])
{
    extern void flash_dumpstack();
    flash_dumpstack();
    return CMD_RET_SUCCESS;
}
CLI_SUBCMD(os_debug, flash_dump,    flash_dump_fun, "show crash info from flash", "os_debug flash_dump");
#endif



#if defined(CONFIG_HEAP_DEBUG) || defined(CONFIG_RUNTIME_DEBUG) || defined(CONFIG_TASK_IRQ_RUN_NUM)
CLI_CMD(os_debug, NULL, "os debug function", "os_debug heaptrace/cpu/switch_num");
#endif

