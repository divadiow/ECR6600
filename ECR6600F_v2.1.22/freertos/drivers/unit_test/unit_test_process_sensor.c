#include "cli.h"

#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"

extern void drv_process_sensor_get(unsigned int DelayTimeUs,unsigned short int *pProcesscnt);

static int utest_process_sensor_out(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int  times = (int)strtoul(argv[1], NULL, 0);
    unsigned short int pro[3]= {0};
    drv_process_sensor_get(times, pro);
    os_printf(LM_CMD, LL_INFO, "hvt = %d\n", pro[0]);
    os_printf(LM_CMD, LL_INFO, "lvt = %d\n", pro[1]);
    os_printf(LM_CMD, LL_INFO, "svt = %d\n", pro[2]);

   return CMD_RET_SUCCESS;
}

CLI_SUBCMD(ut_pro, process, utest_process_sensor_out, "utest_process_sensor_out", "utest_process_sensor_out");

CLI_CMD(ut_pro, NULL, "unit test process sensor", "test process sensor");

