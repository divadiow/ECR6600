#include "cli.h"
#include "pwm.h"
#include "oshal.h"
#include "chip_pinmux.h"
#include "stdlib.h"

static int utest_pwm_init(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num = 0, gpio_num = 22;
	int i;
	for(i=1; i<argc; i+=2)
	{
	
		if(strcmp(argv[i], "pwm")==0)
		{
			pwm_num = atoi(argv[i+1]);
			switch(pwm_num)
			{
				case 0:
					gpio_num = 22;
					break;
				
				case 1:
					gpio_num = 23;
					break;
				
				case 2:
					gpio_num = 24;
					break;
				
				case 3:
					gpio_num = 25;
					break;
				
				case 4:
					gpio_num = 4;
					break;
				
				case 5:
					gpio_num = 15;
					break;
				
				default:
				os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm-num input error!\r\n");	
				return 0;	
					
			}
		}
		else if(strcmp(argv[i], "gpio")==0)
		{
			gpio_num = atoi(argv[i+1]);
		}
		else
        {;}
	}
	

	if(pwm_num == 0 && gpio_num == 0)
	{
		chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO0);
	}
	
	else if(pwm_num == 0 && gpio_num == 22)
	{
		chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO22);
	}
	
	else if(pwm_num == 1 && gpio_num == 1)
	{
		chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO1);
	}
	
	else if(pwm_num == 1 && gpio_num == 23)
	{
	chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO23);
	}
	
	else if(pwm_num == 2 && gpio_num == 2)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO2);
	}
	
	else if(pwm_num == 2 && gpio_num == 24)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO24);
	}
	
	else if(pwm_num == 2 && gpio_num == 16)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO16);
	}
	
	else if(pwm_num == 3 && gpio_num == 3)
	{
		chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO3);
	}
	
	else if(pwm_num == 3 && gpio_num == 25)
	{
		chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO25);
	}

	else if(pwm_num == 4 && gpio_num == 4)
	{
		chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO4);
	}
	
	else if(pwm_num == 4 && gpio_num == 14)
	{
		chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO14);
	}	
	
	else if(pwm_num == 5 && gpio_num == 15)
	{
		chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO15);
	}
	
	else if(pwm_num == 5 && gpio_num == 17)
	{
		chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO17);
	}
	
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\ninput error!\r\n");
		return 0;
	}
	
	if (hal_pwm_init(pwm_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] init ok!\r\n", pwm_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] init failed!\r\n", pwm_num);
	}

	return 0;
}

CLI_SUBCMD(ut_pwm, init, utest_pwm_init, "unit test pwm init", "ut_pwm init [pwm] [num] [gpio] [num]");

static int utest_pwm_config(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num, pwm_freq, pwm_ratio;

	if (argc >= 4)
	{
		pwm_num = (int)strtoul(argv[1], NULL, 0);
		pwm_freq = (int)strtoul(argv[2], NULL, 0);
		pwm_ratio = (int)strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, err: no enough argc!\r\n");
		return 0;
	}

	if (hal_pwm_config(pwm_num, pwm_freq, pwm_ratio) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, set pwm[%d] frequence = %d(Hz), ratio = %d%%!\r\n", pwm_num, pwm_freq, pwm_ratio/10);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] config failed!\r\n", pwm_num);
	}

	return 0;
}

CLI_SUBCMD(ut_pwm, config, utest_pwm_config, "unit test pwm config", "ut_pwm config [pwm-num] [pwm-freq] [pwm-ratio]");


static int utest_pwm_start(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num;

	if (argc >= 2)
	{
		pwm_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, err: no pwm-num input!\r\n");
		return 0;
	}

	if (hal_pwm_start(pwm_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] start ok!\r\n", pwm_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] start failed!\r\n", pwm_num);
	}

	return 0;
}

CLI_SUBCMD(ut_pwm, start, utest_pwm_start, "unit test pwm start", "ut_pwm start [pwm-num]");


static int utest_pwm_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num;

	if (argc >= 2)
	{
		pwm_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, err: no pwm-num input!\r\n");
		return 0;
	}

	if (hal_pwm_stop(pwm_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] stop ok!\r\n", pwm_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] stop failed!\r\n", pwm_num);
	}

	return 0;
}

CLI_SUBCMD(ut_pwm, stop, utest_pwm_stop, "unit test pwm stop", "ut_pwm stop [pwm-num]");

static int utest_pwm_test(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num = 0, gpio_num = 22, pwm_freq = 1000, pwm_ratio = 500;

	int i;
	
	for(i=1; i<argc; i+=2)
	{
		if(strcmp(argv[i], "pwm")==0)
		{
			pwm_num = atoi(argv[i+1]);
			switch(pwm_num)
			{
				case 0:
					gpio_num = 22;
					break;
				
				case 1:
					gpio_num = 23;
					break;
				
				case 2:
					gpio_num = 24;
					break;
				
				case 3:
					gpio_num = 25;
					break;
				
				case 4:
					gpio_num = 4;
					break;
				
				case 5:
					gpio_num = 15;
					break;
				
				default:
				os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm-num input error!\r\n");	
				return 0;	
					
			}
		}
		else if(strcmp(argv[i], "gpio")==0)
		{
			gpio_num = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "freq")==0)
		{
			pwm_freq = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "ratio")==0)
		{
			pwm_ratio = atoi(argv[i+1]);
		}
		else
        {;}
	}

	if(pwm_num == 0 && gpio_num == 0)
	{
		chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO0);
	}
	else if(pwm_num == 0 && gpio_num == 22)
	{
		chip_pwm_pinmux_cfg(PWM_CH0_USED_GPIO22);
	}
	else if(pwm_num == 1 && gpio_num == 1)
	{
		chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO1);
	}
	else if(pwm_num == 1 && gpio_num == 23)
	{
		chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO23);
	}
	else if(pwm_num == 2 && gpio_num == 2)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO2);
	}
	else if(pwm_num == 2 && gpio_num == 24)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO24);
	}
	else if(pwm_num == 2 && gpio_num == 16)
	{
		chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO16);
	}
	else if(pwm_num == 3 && gpio_num == 3)
	{
		chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO3);
	}
	else if(pwm_num == 3 && gpio_num == 25)
	{
		chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO25);
	}
	else if(pwm_num == 4 && gpio_num == 4)
	{
		chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO4);
	}
	else if(pwm_num == 4 && gpio_num == 14)
	{
		chip_pwm_pinmux_cfg(PWM_CH4_USED_GPIO14);
	}	
	else if(pwm_num == 5 && gpio_num == 15)
	{
		chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO15);
	}
	else if(pwm_num == 5 && gpio_num == 17)
	{
		chip_pwm_pinmux_cfg(PWM_CH5_USED_GPIO17);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\ninput error!\r\n");
		return 0;
	}
	
	if(pwm_ratio > 1000)
	{
		os_printf(LM_CMD,LL_INFO,"\r\ninput ratio error!\r\n");
		return 0;
	}
	
 	hal_pwm_init(pwm_num);
	hal_pwm_config(pwm_num, pwm_freq, pwm_ratio);
	hal_pwm_start(pwm_num);

	return 0;
}

CLI_SUBCMD(ut_pwm, test, utest_pwm_test, "unit test pwm", "ut_pwm test");

static int utest_pwm_close(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num;

	if (argc >= 2)
	{
		pwm_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, err: no pwm-num input!\r\n");
		return 0;
	}

	if (hal_pwm_close(pwm_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] close ok!\r\n", pwm_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, pwm[%d] close failed!\r\n", pwm_num);
	}

	return 0;
}

CLI_SUBCMD(ut_pwm, close, utest_pwm_close, "unit test pwm close", "ut_pwm close [pwm-num]");

static int utest_pwm_update(cmd_tbl_t *t, int argc, char *argv[])
{
	int pwm_num;

	if (argc >= 2)
	{
		pwm_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, err: no enough argc!\r\n");
		return 0;
	}

	if (hal_pwm_update(pwm_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, update pwm[%d] ok!\r\n", pwm_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test pwm, update pwm[%d] failed!\r\n", pwm_num);
	}

	return 0;

}

CLI_SUBCMD(ut_pwm, update, utest_pwm_update, "unit test pwm update", "ut_pwm update [pwm-num]");


CLI_CMD(ut_pwm, NULL, "unit test pwm", "test_pwm");

