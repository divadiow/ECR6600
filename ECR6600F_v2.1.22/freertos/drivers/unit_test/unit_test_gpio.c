#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#include "cli.h"
#include "oshal.h"
#include "gpio.h"
#include "hal_gpio.h"
#include "chip_pinmux.h"

static unsigned int data = 0;
static T_GPIO_ISR_CALLBACK p_callback;

static unsigned int set_gpio_function(unsigned int gpio_num)
{
	unsigned int reg,offset;
	unsigned int bit = 7;

	//must set first 
	if((gpio_num >= 0) && (gpio_num <= 7))
	{
		reg = CHIP_SMU_PD_PIN_MUX_0;
	}
	else if((gpio_num >= 8) && (gpio_num <= 15))
	{
		reg = CHIP_SMU_PD_PIN_MUX_1;
	}
	else if((gpio_num >= 16) && (gpio_num <= 23) && (gpio_num != 19))
	{
		reg = CHIP_SMU_PD_PIN_MUX_2;
		if(gpio_num == 18)
		{
			bit = 3;
		}
	}
	else if((gpio_num >= 24) && (gpio_num <= 25))
	{
		reg = CHIP_SMU_PD_PIN_MUX_3;
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"this num = [%d] no GPIO function! \n",gpio_num);
		return CMD_RET_FAILURE;
	}
	
	offset = ((gpio_num % 8) * 4);
	PIN_MUX_SET(reg, bit,offset,1);
	return CMD_RET_SUCCESS;
}

static int utest_gpio_dbtset(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
 	unsigned int gpio_num;
	unsigned int time_ns;

	if (argc == 3)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
		time_ns = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio dbtset [num] [ns]\n", argc);
		return CMD_RET_FAILURE;
	}

	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}

	ret = hal_gpio_set_debounce_time(gpio_num, time_ns);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"gpio DB time set error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD,LL_INFO,"gpio[%d] deb time = %d ns\n",gpio_num,time_ns);
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, dbtset, utest_gpio_dbtset, "unit test gpio DB time set", "ut_gpio dbtset [num] [ns]");


static int utest_gpio_dbclose(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
 	unsigned int gpio_num;
	
	if (argc == 2)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio dbclose [num]\n", argc);
		return CMD_RET_FAILURE;
	}

	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_set_debounce_close(gpio_num);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_INFO,"gpio DB time close error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, dbclose, utest_gpio_dbclose, "unit test gpio DB time close", "ut_gpio dbclose [num]");


static int utest_gpio_dirction_set(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
 	unsigned int gpio_num;
	unsigned int gpio_dir;

	if (argc == 3)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
		gpio_dir = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio dirction_set [num] [dir(0--in,1--out)]\n", argc);
		return CMD_RET_FAILURE;
	}


	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_dir_set(gpio_num, gpio_dir);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"gpio dirction set error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD,LL_INFO,"gpio[%d] dir = %x\n",gpio_num,gpio_dir);
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, dirction_set, utest_gpio_dirction_set, "unit test gpio dirction set", "ut_gpio dirction_set [num] [dir(0--in,1--out)]");


static int utest_gpio_dirction_get(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
 	unsigned int gpio_num;

	if (argc == 2)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio dirction_get [num]\n", argc);
		return CMD_RET_FAILURE;
	}

	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_dir_get(gpio_num);
	if((ret != DRV_GPIO_ARG_DIR_IN) && (ret != DRV_GPIO_ARG_DIR_OUT))
	{
		os_printf(LM_CMD,LL_ERR,"gpio dirction get error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	os_printf(LM_CMD,LL_INFO,"gpio[%d] dir = %x\n",gpio_num,ret);
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, dirction_get, utest_gpio_dirction_get, "unit test gpio dirction get", "ut_gpio dirction_get [num]");


static int utest_gpio_read(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
 	unsigned int gpio_num  = strtoul(argv[1], NULL, 0);

	if (argc == 2)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio read [num]\n", argc);
		return CMD_RET_FAILURE;
	}
	
	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_read(gpio_num);
	
	if((ret != DRV_GPIO_LEVEL_HIGH) && (ret != DRV_GPIO_LEVEL_LOW) )
	{
		os_printf(LM_CMD,LL_ERR,"gpio read error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	os_printf(LM_CMD,LL_INFO,"gpio[%d] value = %x\n",gpio_num,ret);

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, read, utest_gpio_read, "unit test gpio read get", "ut_gpio read [num]");


static int utest_gpio_write(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
	unsigned int value = 0;
	unsigned int gpio_num  = strtoul(argv[1], NULL, 0);          

	if (argc == 3)
	{
		gpio_num  = strtoul(argv[1], NULL, 0);
		
		if (strcmp(argv[2], "high") == 0){
			value = 1;
		}
		else if (strcmp(argv[2], "low") == 0)
		{
			value = 0;		
		}
		else
		{
			os_printf(LM_CMD,LL_ERR,"PARAMETER IS ERROR!! eg: ut_gpio write [num] ['high'/'low']\n");
		}
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio write [num] ['high'/'low']\n", argc);
		return CMD_RET_FAILURE;
	}
	
	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}

	ret = hal_gpio_write(gpio_num,value);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_INFO,"gpio write error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_dir_set(gpio_num, DRV_GPIO_ARG_DIR_OUT);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_INFO,"gpio dirction set error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD,LL_INFO,"write gpio[%d] value = %x\n",gpio_num,value);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, write, utest_gpio_write, "unit test gpio write", "ut_gpio write [num] ['high'/'low']");


static int utest_gpio_pull(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int ret = 0;
 	unsigned int gpio_num; 
	unsigned int type_val;
	
	if (argc == 3)
	{
		gpio_num = strtoul(argv[1], NULL, 0); 
		type_val = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio pull [num] [0--'up'/1--'down']\n", argc);
		return CMD_RET_FAILURE;
	}
	
	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_set_pull_mode(gpio_num, type_val);
	if(ret != 0)
	{	
		os_printf(LM_CMD,LL_ERR,"gpio pull error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	os_printf(LM_CMD,LL_INFO,"gpio[%d] pull = %x\n",gpio_num,type_val);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, pull, utest_gpio_pull, "unit test gpio pull", "ut_gpio pull [num] [0--'up'/1--'down']");

static void gpio_isr_test(void)
{
	os_printf(LM_APP, LL_INFO, "isr coming~ !\n");
}

static void gpio_isr_withdata_test(void * data_d)
{
	os_printf(LM_APP, LL_INFO, "isr coming~ !\n");
	os_printf(LM_APP, LL_INFO, "data_d = %d !\n",*(int *)data_d);
	
}

static int utest_gpio_interrupt(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int ret = 0;
 	unsigned int gpio_num; 
	unsigned int type_intr;
	p_callback.gpio_callback =(void *)(&gpio_isr_test);
	p_callback.gpio_data = &data;

	if (argc == 3)
	{
		gpio_num = strtoul(argv[1], NULL, 0); 
		type_intr = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio interrupt [num] [0:nop/2:high/3:low/5:N/6:P/7:D]\n", argc);
		return CMD_RET_FAILURE;
	}

	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_intr_mode_set(gpio_num, type_intr);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"gpio interrupt error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	hal_gpio_callback_register(gpio_num, p_callback.gpio_callback, p_callback.gpio_data);
	hal_gpio_intr_enable(gpio_num, 1);
	
	os_printf(LM_CMD,LL_INFO,"gpio[%d] interrupt = %x\n",gpio_num,type_intr);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, interrupt, utest_gpio_interrupt, "unit test gpio interrupt", "ut_gpio interrupt [num] [0:nop/2:high/3:low/5:N/6:P/7:D]");


static int utest_gpio_interrupt_withdata(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int ret = 0;
 	unsigned int gpio_num; 
	unsigned int type_intr;
	p_callback.gpio_callback =(void *)(&gpio_isr_withdata_test);
	p_callback.gpio_data = &data;

	if (argc == 4)
	{
		gpio_num = strtoul(argv[1], NULL, 0); 
		type_intr = strtoul(argv[2], NULL, 0);
		data = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_gpio interrupt [num] [0:nop/2:high/3:low/5:N/6:P/7:D] [data]\n", argc);
		return CMD_RET_FAILURE;
	}

	//must set first 
	if(set_gpio_function(gpio_num))
	{
		return CMD_RET_FAILURE;
	}
	
	ret = hal_gpio_intr_mode_set(gpio_num, type_intr);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"gpio interrupt error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	hal_gpio_callback_register(gpio_num, p_callback.gpio_callback, p_callback.gpio_data);
	hal_gpio_intr_enable(gpio_num, 1);
	
	os_printf(LM_CMD,LL_INFO,"gpio[%d] interrupt = %x\n",gpio_num,type_intr);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_gpio, interrupt_withdata, utest_gpio_interrupt_withdata, "unit test gpio interrupt_withdata", "ut_gpio interrupt_withdata [num] [0:nop/2:high/3:low/5:N/6:P/7:D] [data]");


CLI_CMD(ut_gpio, NULL, "unit test gpio", "test_gpio");


