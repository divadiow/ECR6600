#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "hal_adc.h"
#include "oshal.h"
#include "timer.h"
#include "pit.h"
#if 0
static int utest_adc_config(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int input_a;
	unsigned int input_b;
	unsigned int volt;

	if (argc >= 3)
	{
		input_a = strtoul(argv[1], NULL, 0);	
		input_b = strtoul(argv[2], NULL, 0);
		volt = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"\r\nunit test adc config, aux_adc config fail!\r\n");
		return CMD_RET_FAILURE;
	}
	hal_adc_config(input_a, input_b, volt);
	return CMD_RET_SUCCESS;	
}

CLI_SUBCMD(ut_adc, config,    utest_adc_config,  " utest_adc_config",  " utest_adc_config [input_a] [input_b] [max_volt]");
#endif

static int utest_adc_single_test(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int input_a;
	unsigned int input_b;
	unsigned int volt;
	int vout = 0;
	if(argc >= 3)
	{
		input_a = strtoul(argv[1], NULL, 0);	
		input_b = strtoul(argv[2], NULL, 0);
		volt = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"\r\nunit test adc config, aux_adc config fail!\r\n");
		return CMD_RET_FAILURE;
	}
	
    vout = hal_adc_get_single(input_a, input_b, volt);
    os_printf(LM_CMD,LL_ERR,"vout=%d\r\n", vout);
	return CMD_RET_SUCCESS;	
}
CLI_SUBCMD(ut_adc, signaltest, utest_adc_single_test,  "utest_adc_single_test",  "utest_adc_single_test");

static int utest_adc_diff_test(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int input_a;
	unsigned int input_b;
	unsigned int volt;
    int vout = 0;
	if (argc >= 3)
	{
		input_a = strtoul(argv[1], NULL, 0);	
		input_b = strtoul(argv[2], NULL, 0);
		volt = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"\r\nunit test adc diff, aux_adc diff fail!\r\n");
		return CMD_RET_FAILURE;
	}
    vout = hal_adc_get_diff(input_a, input_b, volt);
	os_printf(LM_CMD,LL_INFO,"diff vout=%d\r\n", vout);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_adc, difftest,   utest_adc_diff_test,  "utest_adc_diff_test",  "utest_adc_diff_test");


static int utest_adc_vbat_test(cmd_tbl_t *t, int argc, char *argv[])
{
	
	int vout =0;
	vout = hal_adc_vbat();
	os_printf(LM_CMD,LL_INFO,"vout=%d\r\n", vout);
	return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_adc, vbat,   utest_adc_vbat_test,  "utest_vbat_test",  "utest_vbat_test");

static int utest_adc_tempsensor_test(cmd_tbl_t *t, int argc, char *argv[])
{
    int temp;
    temp = hal_adc_tempsensor();
    os_printf(LM_CMD,LL_INFO,"current temp=%d\r\n", temp);
    return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_adc, temp_test, utest_adc_tempsensor_test,  "utest_adc_tempsensor_test",  "utest_adc_tempsensor_test");

static int utest_adc_dynamic_test(cmd_tbl_t *t, int argc, char *argv[])
{
    int len =100;
    int i;
    int buff[100] = {0};
    drv_aux_adc_dynamic(buff ,len);
    for(i =0 ;i<len ;i++)
    {
        os_printf(LM_CMD,LL_INFO, "vout[%d]=%d\r\n",i,buff[i]);
    }
	return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_adc, dyn_test, utest_adc_dynamic_test,  "utest_adc_dynamic_test",  "utest_adc_dynamic_test");

static int utest_adc_dynamic_init(cmd_tbl_t *t, int argc, char *argv[])
{

    unsigned int input_a;
	unsigned int input_b;
	unsigned int volt;
	if (argc >= 3)
	{
		input_a = strtoul(argv[1], NULL, 0);	
		input_b = strtoul(argv[2], NULL, 0);
		volt = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"\r\nunit test adc config, aux_adc config fail!\r\n");
		return CMD_RET_FAILURE;
	}
    hal_adc_config(input_a, input_b, volt);
    drv_aux_adc_dynamic_init();
	return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_adc, dyn_init, utest_adc_dynamic_init,  "utest_adc_dynamic_init",  "utest_adc_dynamic_init");

CLI_CMD(ut_adc, NULL, "unit test adc", "test_adc");

