/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
 *
 * Copyright (C) RivieraWaves 2017-2019
 *
 ****************************************************************************************
 */
/* Kernel includes. */
//#include "FreeRTOS.h"
//#include "task.h"
//#include "timers.h"
//#include "semphr.h"

#include "oshal.h"

#include "chip_irqvector.h"
#include "chip_memmap.h"
#include "arch_irq.h"
#include "cli.h"
#include "uart.h"
#include "flash.h"
#include "ble_thread.h"
#include "sdk_version.h"
#include "easyflash.h"
//#include "adc.h"
#include "wdt.h"
#define DFE_VAR
#include "tx_power_config.h"
#include "at_customer_wrapper.h"

#include "psm_system.h"
#include "psm_user.h"

#ifdef CONFIG_RTC
#include "rtc.h"
#endif
#include "nvds.h"

#if defined(CONFIG_OTA_LOCAL)
    #include "local_ota.h"
#endif
#if defined(CONFIG_OTA_SERVICE)
    #include "mqtt_ota.h"
#endif

#include "at.h"
//#include "at_common.h"


#if defined(CONFIG_BLE_TEST_NETCFG)
extern int netcfg_task_handle;
extern void test_netcfg_task(void *arg);
#endif //CONFIG_BLE_TEST_NETCFG

extern void vTaskStartScheduler( void );
extern void wifi_main();
extern void ke_init(void);
extern void rf_platform_int();
extern void amt_cal_info_obtain();
#if defined(CONFIG_WIFI_AMT_VERSION)
extern int AmtGetVerStart();
extern void AmtInit();
extern void modem_init();
#endif

#if defined(CONFIG_ASIC_RF) && defined(CFG_WLAN_COEX)
extern void rf_pta_config();
#endif// CFG_WLAN_COEX

extern void wifi_conn_main();

// at_result_code_t at_handle_CUSTOMER_TEST(void* dce, void* group, int kind, size_t argc, at_arg_t* argv)
// {
    
//     if (kind & AT_READ){ //AT COMMAND:"AT+CUSTOMER_TEST?
//         at_emit_information_response("READ CMD",-1);
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_WRITE) {  //AT COMMAND:"AT+CUSTOMER_TEST=1
//         at_arg_t result[] = {
//                         {AT_ARG_TYPE_NUMBER, .value.number=argv[0].value.number},
//                         {AT_ARG_TYPE_NUMBER, .value.number=argv[0].value.number},
//                     };
//         at_emit_extended_result_code_with_args("WRITE CMD ",-1 ,result,2,1,false);
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_EXEC){ //AT COMMAND:"AT+CUSTOMER_TEST
//         at_emit_information_response("EXE CMD",-1);
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     } else if(kind & AT_TEST){ //AT COMMAND:"AT+CUSTOMER_TEST=?"
//         at_emit_information_response("TEST CMD",-1);
//         at_emit_basic_result(AT_OK);
//         return AT_OK;
//     }
//     at_emit_basic_result(AT_ERROR);
//     return AT_OK;
// }

// static const at_command_list CUS_commands[] = {
//     {"CUSTOMER_TEST"     , (user_cmdhandler_t)at_handle_CUSTOMER_TEST   , AT_TEST| AT_EXEC |AT_WRITE | AT_READ},
// };
#if 0
#define uint8_t unsigned char
#define BUILD_UINT8(loByte, hiByte ) \
    ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))
 
int test_main()
{
	char ID1[12] = "012a3b4c5b6d";
	//uint8_t mesh_id[6];
    uint8_t temp_id[12];
	int i;
	
	for(i = 0; i < 12 ; i++) {
        if(ID1[i] >= '0' && ID1[i] <= '9') {
            temp_id[i] = ID1[i] - '0';
        } else {
            temp_id[i] = ID1[i] - 'a' + 10;
        }
       os_printf(LM_OS, LL_INFO," %d = %d  ", i, temp_id[i]);
    }
	
	os_printf(LM_OS, LL_INFO,"\r\n");
    for(i = 0; i < 12 ;) {
        temp_id[i/2] = BUILD_UINT8(temp_id[i+1], temp_id[i]);
        i += 2;
    }
	
	for(i = 0; i < 6 ; i++) {
		os_printf(LM_OS, LL_INFO,"%02x\n", temp_id[i]);
	}
	
	return 0;

}
#endif
os_sem_handle_t psm_sleep_off_sem;
uint8_t psm_usr_sleep_mode_status=0;

void psm_usr_disable_callback()
{
	if(psm_usr_sleep_mode_status){
		psm_usr_sleep_mode_status = 0;
		os_sem_post(psm_sleep_off_sem);	
		os_printf(LM_OS, LL_INFO, "psm_usr_disable_callback----------\r");
		
	}
}

extern void  target_dce_transmit(const char* data, size_t size);
void tiangong_psm_sleep_task()
{
	while(1){
		os_sem_wait(psm_sleep_off_sem, 0xffffffff);
		psm_set_sleep_mode(SLEEP_OFF, 0);
		os_printf(LM_OS, LL_INFO, "tiangong_psm_sleep_task\r\n");
		target_dce_transmit("[DA]SLEEP:0\r\n", strlen("[DA]SLEEP:0\r\n"));
	}
}

signed int skylab_psm_sleep_func_cb(void)
{
	signed int ret = true;
	/*for example hold gpio4 keep output when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_4,PSM_SLEEP_IN,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	
	if(!psm_hold_gpio_intf(GPIO_NUM_3,PSM_SLEEP_IN,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	return ret;	
}

signed int skylab_psm_wakeup_func_cb(void)
{
	signed int ret = true;
	/*for example hold gpio4 keep output when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_4,PSM_SLEEP_OUT,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	if(!psm_hold_gpio_intf(GPIO_NUM_3,PSM_SLEEP_OUT,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	return ret;
}


extern void psm_at_callback(void_fn cb);
int main(void)
{
	component_cli_init(E_UART_SELECT_BY_KCONFIG);
	os_printf(LM_OS, LL_INFO, "SDK version %s, Release version %s\n",sdk_version,RELEASE_VERSION);
	
#if defined(CONFIG_ASIC_RF)
	rf_platform_int();
#endif //CONFIG_ASIC_RF


#if defined(CONFIG_NV)
	if(partion_init() != 0)
	{
		os_printf(LM_OS, LL_ERR, "partition init error !!!!!! \n");
	}
	if(easyflash_init() != 0)
	{
		os_printf(LM_OS, LL_ERR, "easyflash init error !!!!!! \n");
	}
#endif //CONFIG_NV

#if defined(CONFIG_STANDALONE_UART) || defined(CFG_TRC_EN)
	extern void ble_hci_uart_init(E_DRV_UART_NUM uart_num);
    ble_hci_uart_init(E_UART_SELECT_BY_KCONFIG);
#endif //CONFIG_STANDALONE_UART

    ke_init();

#if defined(CONFIG_ECR_BLE)
    ble_main();

#if defined(CONFIG_BLE_TEST_NETCFG)
		netcfg_task_handle=os_task_create( "netcfg-task", BLE_NETCFG_PRIORITY, BLE_NETCFG_STACK_SIZE, (task_entry_t)test_netcfg_task, NULL);
#endif //CONFIG_BLE_TEST_NETCFG

#endif
#if defined(CFG_WLAN_COEX)
    rf_pta_config();
#endif //CONFIG_ASIC_RF && CFG_WLAN_COEX
#ifdef CONFIG_ADC
    drv_adc_init();
    get_volt_calibrate_data();
#endif

    amt_cal_info_obtain();
#if defined(CONFIG_WIFI_AMT_VERSION)
    if (AmtGetVerStart() == 1) {
        modem_init();
        AmtInit();
        vTaskStartScheduler();
        return 0;
    }
#endif
#ifdef CONFIG_ECR6600_WIFI
	wifi_main();
#endif
    #ifdef CONFIG_PSM_SURPORT
	psm_wifi_ble_init();
	psm_boot_flag_dbg_op(true, 1);
    #endif

#if defined(CONFIG_RTC)
	extern volatile int rtc_task_handle;
	extern void calculate_rtc_task();
	rtc_task_handle = os_task_create("calculate_rtc_task",SYSTEM_EVENT_LOOP_PRIORITY,4096,calculate_rtc_task,NULL);
	if(rtc_task_handle) os_printf(LM_OS, LL_INFO, "rtc calculate start!\r\n");
#endif

#if 0	
#ifdef CONFIG_WDT_FEED_TASK
	extern WDT_RET_CODE creat_wdt_feed_task_with_nv();
	creat_wdt_feed_task_with_nv();
#endif
#endif

#ifdef CONFIG_HEALTH_MONITOR
	extern int health_mon_create_by_nv();
	health_mon_create_by_nv();
#endif

    wifi_conn_main();

#if defined(CONFIG_OTA_LOCAL)
	local_ota_main();
#endif
#if defined(CONFIG_OTA_SERVICE)
	service_ota_main();
#endif
#ifdef CONFIG_APP_AT_COMMAND
    AT_command_init();
#endif
    psm_sleep_off_sem = os_sem_create(1, 0);	
	os_task_create("psm_sleep_off",SYSTEM_EVENT_LOOP_PRIORITY,4096,tiangong_psm_sleep_task,NULL);

	psm_proc_init_cb(PSM_SLEEP_IN,skylab_psm_sleep_func_cb);
	psm_proc_init_cb(PSM_SLEEP_OUT,skylab_psm_wakeup_func_cb);
	psm_at_callback(psm_usr_disable_callback);
	vTaskStartScheduler();
	return 0;
}



