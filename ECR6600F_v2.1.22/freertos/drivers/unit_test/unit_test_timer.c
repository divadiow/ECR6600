
#include "stdlib.h"
#include "cli.h"
#include "hal_timer.h"

#include "oshal.h"


//#define DRV_TIMER_CNT_PER_US(x)			(24 * (x))


typedef struct _utest_timer_dev
{
	int used;
	int num;
} utest_timer_dev;


utest_timer_dev ut_timer_dev[4];


void utest_timer_isr(void * data)
{
	utest_timer_dev * dev = (utest_timer_dev *) data;
	os_printf(LM_CMD,LL_INFO,"unit test timer, timer[%d]-isr comes!\r\n", dev->num);
}


static int utest_timer_set_time(cmd_tbl_t *t, int argc, char *argv[])
{
	int num, time;

	if (argc >= 3)
	{
		num = (int)strtoul(argv[1], NULL, 0);
		time = (int)strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, err: no enough argc!\r\n");
		return 0;
	}

	if (hal_timer_change_period(num, time) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, set timer[%d] time = %d(us)!\r\n", num, time);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, set[%d] timer failed!\r\n", num);
	}

	return 0;
}

CLI_SUBCMD(ut_timer, set, utest_timer_set_time, "unit test set time", "ut_timer set [timer-num] [time]");


static int utest_timer_start(cmd_tbl_t *t, int argc, char *argv[])
{
	int timer_num;

	if (argc >= 2)
	{
		timer_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, err: no timer-num input!\r\n");
		return 0;
	}

	if (hal_timer_start(timer_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] start ok!\r\n", timer_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] start failed!\r\n", timer_num);
	}

	return 0;
}

CLI_SUBCMD(ut_timer, start, utest_timer_start, "unit test start timer", "ut_timer start [timer-num]");


static int utest_timer_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	int timer_num;

	if (argc >= 2)
	{
		timer_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, err: no timer-num input!\r\n");
		return 0;
	}

	if (hal_timer_stop(timer_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] stop ok!\r\n", timer_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] stop failed!\r\n", timer_num);
	}

	return 0;
}


CLI_SUBCMD(ut_timer, stop, utest_timer_stop, "unit test stop timer", "ut_timer stop [timer-num]");


static int utest_timer_create(cmd_tbl_t *t, int argc, char *argv[])
{
	int i;
	int one_shot;
	int timer_num;
	utest_timer_dev * dev;

	for (i=0; i<4; i++)
	{
		dev = &ut_timer_dev[i];

		if (dev->used == 0)
		{
			break;
		}
	}

	if (i == 4)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, err: all timer uesed!\r\n");
		return 0;
	}

	if (argc == 1)
	{
		one_shot = 0;
	}
	else
	{
		one_shot = (int)strtoul(argv[1], NULL, 0);
	}

	timer_num = hal_timer_create(200000, utest_timer_isr, (void *)dev, one_shot);

	if (timer_num < 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer create failed!\r\n");
		return 0;
	}


	dev->used = 1;
	dev->num = timer_num;

	os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] create ok!\r\n", timer_num);

	return 0;
}


CLI_SUBCMD(ut_timer, create, utest_timer_create, "unit test create timer", "ut_timer create");


static int utest_timer_delete(cmd_tbl_t *t, int argc, char *argv[])
{
	int timer_num, i;
	utest_timer_dev * dev;

	if (argc >= 2)
	{
		timer_num = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, err: no timer-num input!\r\n");
		return 0;
	}


	if (hal_timer_delete(timer_num) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] delete ok!\r\n", timer_num);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test timer, timer[%d] delete failed!\r\n", timer_num);
	}

	for (i=0; i<4; i++)
	{
		dev = &ut_timer_dev[i];
		if (dev->num == timer_num)
		{
			dev->used = 0;
			break;
		}
	}

	return 0;
}


CLI_SUBCMD(ut_timer, delete, utest_timer_delete, "unit test delete timer", "ut_timer delete [timer-num]");

#define TIMER_WAIT_FOREVER 0xffffffff
static os_sem_handle_t timer_process;
int timer_ch = 0;
int timer_task_flag = 0;
void  timer_task(void *arg)
{
	unsigned int time;
	while(1)
	{
		os_sem_wait(timer_process, TIMER_WAIT_FOREVER);
		os_usdelay(40000);
		hal_timer_get_current_time(timer_ch, &time);
		os_printf(LM_CMD,LL_INFO,"current time = %d us\r\n", time);
	}
}

static int utest_timer_calculate_time(cmd_tbl_t *t, int argc, char *argv[])
{
	if(timer_task_flag == 0)
	{
		timer_process = os_sem_create(1, 0);
		os_task_create("timer-task", 3, 4096, (task_entry_t)timer_task, NULL);
		timer_task_flag++;
	}
	hal_timer_start(timer_ch);	
	os_sem_post(timer_process);
	return 0;

}

CLI_SUBCMD(ut_timer, cal, utest_timer_calculate_time, "utest_timer_calculate_time", "utest_timer_calculate_time");


CLI_CMD(ut_timer, NULL, "unit test timer", "test_timer");


