#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "chip_irqvector.h"
//#include "chip_configuration.h"
#include "cli.h"
#include "oshal.h"
#include "easyflash.h"
#include "hal_wdt.h"
#include "wdt.h"
#include "hal_system.h"

#ifdef CONFIG_PSM_SURPORT
//#include "psm_system.h"

#endif

#define RST_TYPE_STRING_LEN_IN_NV (4)

//ms
static unsigned int wdt_inter_time[WDT_CTRL_INTTIME_15 + 1] =
{
    2, 8, 31, 62, 125, 250, 500, 1000, 4000,
    16000, 64000, 256000, 1024000, 4096000, 16384000, 65536000
}; 

static unsigned int wtd_reset_time[WDT_CTRL_RSTTIME_7 + 1] =
{
    4,8,16,31,62,125,250,500
}; 

static WDT_RET_CODE get_ctl_para_from_time(const unsigned int time, int *ctrl_int_para, int *ctl_reset_para, unsigned int *effective_time)
{
    if ((ctrl_int_para == NULL) || (ctl_reset_para == NULL)) //effective_time has checked
    {
        return WDT_RET_ERR;
    }

    if (time >= wdt_inter_time[WDT_CTRL_INTTIME_9]) // >= 16000 ms
    {
        int max_index = 0;
        for (max_index = WDT_CTRL_INTTIME_9; max_index < WDT_CTRL_INTTIME_15; max_index++)
        {
            if ((wdt_inter_time[max_index] <= time) && (time < wdt_inter_time[max_index + 1]))
            {
                *effective_time = wdt_inter_time[max_index];
                *ctrl_int_para = max_index; 
                break;
            }
        }

        if (max_index >= WDT_CTRL_INTTIME_15)
        {
            *effective_time = wdt_inter_time[WDT_CTRL_INTTIME_15];
            *ctrl_int_para = WDT_CTRL_INTTIME_15; 
        }
        
        *ctl_reset_para = WDT_CTRL_RSTTIME_0;
    }
    else if (time >= (wdt_inter_time[WDT_CTRL_INTTIME_8] + wtd_reset_time[WDT_CTRL_RSTTIME_7])) // >= 4000 + 512ms, limit 4000 + 512ms
    {
        *effective_time = wdt_inter_time[WDT_CTRL_INTTIME_8] + wtd_reset_time[WDT_CTRL_RSTTIME_7];
        *ctrl_int_para = WDT_CTRL_INTTIME_8;
        *ctl_reset_para = WDT_CTRL_RSTTIME_7;
    }
    else if (time >= (wdt_inter_time[WDT_CTRL_INTTIME_7] + wtd_reset_time[WDT_CTRL_RSTTIME_7])) // >= 1000 + 512ms && < 4512ms, limit 1000 + 512ms
    {
        *effective_time =  wdt_inter_time[WDT_CTRL_INTTIME_7] + wtd_reset_time[WDT_CTRL_RSTTIME_7];
        *ctrl_int_para = WDT_CTRL_INTTIME_7;
        *ctl_reset_para = WDT_CTRL_RSTTIME_7;
    }
    else if (time >= wdt_inter_time[WDT_CTRL_INTTIME_7] +  wtd_reset_time[WDT_CTRL_RSTTIME_0])  // >= 1004  && < 1512ms, limit 1000 + 512ms
    {
        *effective_time =  wdt_inter_time[WDT_CTRL_INTTIME_7] + wtd_reset_time[WDT_CTRL_RSTTIME_0];
        *ctrl_int_para = WDT_CTRL_INTTIME_7;
        *ctl_reset_para = WDT_CTRL_RSTTIME_0;
    }
    else  //< 1004ms
    {
        if (time >= (wtd_reset_time[WDT_CTRL_RSTTIME_7] + wdt_inter_time[WDT_CTRL_INTTIME_0]))
        {
            *effective_time =  wtd_reset_time[WDT_CTRL_RSTTIME_7] + wdt_inter_time[WDT_CTRL_INTTIME_0];
            *ctl_reset_para = WDT_CTRL_RSTTIME_7;
        }
        else if (time <= (wtd_reset_time[WDT_CTRL_RSTTIME_0] + wdt_inter_time[WDT_CTRL_INTTIME_0]))
        {
            *effective_time =  wtd_reset_time[WDT_CTRL_RSTTIME_0] + wdt_inter_time[WDT_CTRL_INTTIME_0];
            *ctl_reset_para = WDT_CTRL_RSTTIME_0;
        }
        else
        {
            int index = 0;
            for (index = 0; index < WDT_CTRL_RSTTIME_7; index++)
            {
                if (((wtd_reset_time[index] + wdt_inter_time[WDT_CTRL_INTTIME_0]) <= time) && (time < (wtd_reset_time[index + 1] + wdt_inter_time[WDT_CTRL_INTTIME_0])))
                {
                    *effective_time =  wtd_reset_time[index] +  wdt_inter_time[WDT_CTRL_INTTIME_0];
                    *ctl_reset_para = index; 
                    break;
                }
            }
        }

        *ctrl_int_para = WDT_CTRL_INTTIME_0;
    }

    return WDT_RET_OK;
}
WDT_RET_CODE hal_wdt_init(const unsigned int time, unsigned int *effective_time)
{ 
    if (effective_time == NULL)
    {
        os_printf(LM_OS, LL_INFO, "hal_wdt_init para is err!\r\n");
        return WDT_RET_ERR;
    }
    
    os_printf(LM_OS, LL_INFO, "hal_wdt_init reset_time is %d!\r\n", time);
    int ctrl_int_para = 0;
    int ctrl_rst_para = 0;
    get_ctl_para_from_time(time, &ctrl_int_para, &ctrl_rst_para, effective_time);
    os_printf(LM_OS, LL_INFO, "ctrl_int_para= %d, ctrl_rst_para= %d, *effective_time = %d!\r\n", ctrl_int_para, ctrl_rst_para, *effective_time);
    return drv_wdt_start(ctrl_int_para, ctrl_rst_para, WDT_CTRL_INTDISABLE, WDT_CTRL_RSTENABLE);
}

WDT_RET_CODE hal_wdt_start() 
{
    return drv_wdt_start(WDT_CTRL_INTTIME_7, WDT_CTRL_RSTTIME_7, WDT_CTRL_INTDISABLE, WDT_CTRL_RSTENABLE);;
}

WDT_RET_CODE hal_wdt_stop()
{
    return drv_wdt_stop();
}

WDT_RET_CODE hal_wdt_feed()
{
    return drv_wdt_feed();
}



#if 0
void wdt_feed_task(void *arg)
{
	int count = 0;
	for(;;)
	{
		os_printf(LM_OS, LL_INFO, "entry wdt_feed_task %d\r\n", count++);
		hal_wdt_feed();
		os_msleep(1000);
	}
}


static int test_task_handle = 0;
static int cmd_hal_wdt_test(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc > 3)
	{
		os_printf(LM_OS, LL_INFO, "wdt cmd ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}

	char *cmd = argv[1];
	if((strcmp("init", cmd) == 0) && (argc == 3))
	{
	    unsigned int interval = atoi(argv[2]);
        if(interval <= 0) {
            os_printf(LM_OS, LL_INFO, "ERR : interval must be > 0\r\n");
            return CMD_RET_FAILURE;
        }
        unsigned int effect_time = 0;
        WDT_RET_CODE ret = hal_wdt_init(interval, &effect_time);
        if (ret == WDT_RET_OK)
        {
            os_printf(LM_OS, LL_INFO, "init wdt cmd success.(%d)!\r\n", effect_time);
        }
        else
        {
            os_printf(LM_OS, LL_INFO, "receive init wdt err!\r\n");
        }
	}
    else if (!strcmp("start", cmd))
    {
        hal_wdt_start();
        os_printf(LM_OS, LL_INFO, "receive start wdt cmd ,start feed!\r\n");
    }
    else if (!strcmp("stop", cmd))
    {
        hal_wdt_stop();
        os_printf(LM_OS, LL_INFO, "receive stop wdt!\r\n");
    }
    else if (!strcmp("stopfeed", cmd))
    {
        os_printf(LM_OS, LL_INFO, "receive stopfeed wdt!\r\n");
        if (test_task_handle != 0)
        {
            os_task_delete(test_task_handle);
            os_printf(LM_OS, LL_INFO, "stop wdt task ok!\r\n");
        }
    }
    else if (!strcmp("feed", cmd))
    {
        os_printf(LM_OS, LL_INFO, "receive feed wdt!\r\n");
        test_task_handle=os_task_create( "wdt-feed-task", 3, 1024, (task_entry_t)wdt_feed_task, NULL);
        if (test_task_handle != 0)
        {
            os_printf(LM_OS, LL_INFO, "create wdt-feed-task success!\r\n");
        }
    }
    else if (!strcmp("reset", cmd))
    {
        RST_TYPE rst_type = RST_TYPE_POWER_ON;
        hal_get_reset_type(&rst_type);
        os_printf(LM_OS, LL_INFO, "last reset reason is %d\r\n", rst_type);
        os_printf(LM_OS, LL_INFO, "write current reset reason\r\n");
        hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
    }
    else
    {
        os_printf(LM_OS, LL_INFO, "cmd err,use wdt start/stop/feed/reset!!!\n");
    }

	return CMD_RET_SUCCESS;
}

CLI_CMD(wdt, cmd_hal_wdt_test,  "test hal-wdt interface",  "wdt-test");
#endif




