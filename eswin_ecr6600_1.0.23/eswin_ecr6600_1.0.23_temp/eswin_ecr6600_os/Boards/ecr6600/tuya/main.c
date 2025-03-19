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
#include "FreeRTOS.h"
#include "task.h"
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
#if defined(CONFIG_ASIC_RF)
//#include "hal_rf_6600.h"
#endif //CONFIG_ASIC_RF
#include "sdk_version.h"
#define DFE_VAR
#include "tx_power_config.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
extern unsigned int psm_boot_flag_dbg;
#endif


#include <stdio.h>
#include "os_task_config.h"
#include "system_event.h"
#include "cli.h"

#include "adc.h"

//#include "tx_power_config.h"
//#include "tuya_hal_wifi.h"

//#include "drv_uart.h"
//#include "util_cmd.h"
//#include "util_cli_freertos.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "standalone.h"
//#include "drv_adc.h"
#ifdef CONFIG_RTC
#include "rtc.h"
#endif


#if defined(CONFIG_WIFI_AMT_VERSION)
extern void amt_cal_info_obtain();
extern int AmtGetVerStart();
extern void AmtInit();
extern void modem_init();
#endif


//extern void hal_rtc_init(void);
//extern void wifi_drv_init(void);
//extern void user_main(void);

extern void tuya_app_main(void);

extern void vPortHeapInit( void );
extern void vTaskStartScheduler( void );
extern void wifi_main();
extern void ke_init(void);
//void spiFlash_api_init(E_SPI_SCLK_DIV clk_div, E_SPIFLASH_READ_CMD xip_rd_cmd);
extern void rf_platform_int();
#if defined(CONFIG_ASIC_RF) && defined(CFG_WLAN_COEX)
extern void rf_pta_config();
#endif// CFG_WLAN_COEX

#if 0
static TaskHandle_t user_app_task_handle = NULL;

static void user_app_main(void *param)
{
    /* ͿѻAPP */
    user_main();
    vTaskDelete(user_app_task_handle);
}

static void user_app_entry()
{
    system_printf("enter user_entry\n");
#if 1    
    if(pdTRUE != xTaskCreate(user_app_main, (const char *)"user_app_entry", 2*1024, NULL, TUTA_APP_TASK_PRIORITY, &user_app_task_handle))
    {
        if(user_app_task_handle)
        {
            vTaskDelete(user_app_task_handle);
        }

        system_printf("tuya APP start fail!!!\n");
        return;
    }
#endif
	system_printf("tuya APP start success!!!\n");
}
#endif
extern 	int tuya_cli_init(int argc, char *argv[]);
extern int tuya_cli_printf_option(unsigned char  enable,const char *msg, ...);
#define tuya_auto_test_printf(fmt,args...) tuya_cli_printf_option(1,fmt,##args)

int main(void)
{

#if defined(CONFIG_NV)
	if(partion_init() != 0)
	{
		os_printf(LM_APP, LL_INFO, "partition init error !!!!!! \n");
	}
	if(easyflash_init() != 0)
	{
		os_printf(LM_APP, LL_INFO, "easyflash init error !!!!!! \n");
	}
#endif //CONFIG_NV

	//component_cli_init(E_UART_SELECT_BY_KCONFIG);

#if defined(CONFIG_WIFI_AMT_VERSION)
	if (AmtGetVerStart() == 1)
	{
		AmtInit();		
	}
	else
	{				
		component_cli_init(E_UART_SELECT_BY_KCONFIG);
	}
#endif
	os_printf(LM_OS, LL_INFO, "SDK version %s, Release version %s \n", sdk_version,RELEASE_VERSION);
#if defined(CONFIG_ASIC_RF)
	rf_platform_int();
#endif //CONFIG_ASIC_RF


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
        //AmtInit();
        vTaskStartScheduler();
        return 0;
    }
#endif
#ifdef CONFIG_ECR6600_WIFI
	wifi_main();
#endif
    
#ifdef CONFIG_PSM_SURPORT
	psm_wifi_ble_init();
	psm_boot_flag_dbg  = 1;
#endif
#if defined(CONFIG_RTC)
	extern volatile int rtc_task_handle;
	extern void calculate_rtc_task();
	rtc_task_handle = os_task_create("calculate_rtc_task",SYSTEM_EVENT_LOOP_PRIORITY,4096,calculate_rtc_task,NULL);
	//if(rtc_task_handle) os_printf(LM_APP, LL_INFO, "rtc calculate start!\r\n");
#endif



    //user_app_entry();

//#ifndef TUYA_SDK_CLI_ADAPTER

    os_printf(LM_APP, LL_INFO, "start TUYA application\n");
    tuya_app_main();
//#endif


	vTaskStartScheduler();
	return 0;
}



