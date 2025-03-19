/**
 * @file cmd_adc.c
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
#include "adc.h"

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
static int aux_adc_test(cmd_tbl_t *t, int argc, char *argv[])
{
	int vout;
	unsigned int signal_a = strtoul(argv[1], NULL, 0);	
	unsigned int signal_b = strtoul(argv[2], NULL, 0);
	unsigned int dividea_enable = strtoul(argv[3], NULL, 0);
	unsigned int divideb_enable = strtoul(argv[4], NULL, 0);
	unsigned int divider_a = strtoul(argv[5], NULL, 0);
	unsigned int divider_b = strtoul(argv[6], NULL, 0);
	unsigned int gain_mode = strtoul(argv[7], NULL, 0);
	unsigned int mode_flag = strtoul(argv[8], NULL, 0);
	vout = hal_aux_adc_hardware_test(signal_a,signal_b,dividea_enable,divideb_enable,divider_a,divider_b,gain_mode,mode_flag);
	os_printf(LM_CMD, LL_INFO, "current vout =%d\n", vout);
	return CMD_RET_SUCCESS;
}
CLI_CMD(adc, aux_adc_test,	"aux_adc_test",  "aux_adc_test");



