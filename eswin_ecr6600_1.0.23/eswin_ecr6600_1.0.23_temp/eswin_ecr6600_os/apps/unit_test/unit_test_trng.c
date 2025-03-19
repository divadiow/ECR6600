

#include "cli.h"

#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "hal_trng.h"




static int utest_trng_out(cmd_tbl_t *t, int argc, char *argv[])
{
   unsigned int trng_value = 0;
   trng_value = hal_trng_get();
   os_printf(LM_CMD, LL_INFO, "trng_value = 0x%x\n", trng_value);

   return CMD_RET_SUCCESS;
}

CLI_SUBCMD(ut_trng, trng, utest_trng_out, "unit test trng out", "ut_trng trng");


static int utest_prng_out(cmd_tbl_t *t, int argc, char *argv[])
{
   unsigned int prng_value = 0;
   prng_value = hal_prng_get();
   os_printf(LM_CMD, LL_INFO, "prng_value = 0x%x\n", prng_value);

   return CMD_RET_SUCCESS;
}

CLI_SUBCMD(ut_trng, prng, utest_prng_out, "unit test prng out", "ut_trng prng");


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

CLI_SUBCMD(ut_trng, process, utest_process_sensor_out, "utest_process_sensor_out", "utest_process_sensor_out");

CLI_CMD(ut_trng, NULL, "unit test trng", "test_trng");






