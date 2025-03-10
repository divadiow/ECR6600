#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uart.h"
#include "cli.h"
#include "oshal.h"
#include "sdk_version.h"
#include "hal_system.h"

#if defined (CONFIG_OTA_LOCAL) || defined(CONFIG_OTA_SERVICE)
#include "ota.h"
#include "flash.h"
#endif

#if defined (CONFIG_PSM_SURPORT)
#include "psm_system.h"
#endif

#if defined (CONFIG_ECR6600_WIFI)
#include "rtos.h"
#endif

#if defined (CONFIG_ECR_BLE)
#include "ble_thread.h"
#endif




#if defined (CONFIG_CMD_SHOW_REL_VERSION)
int cmd_show_rel_verion(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD,LL_INFO,"Release version is %s\n",RELEASE_VERSION);
	return CMD_RET_SUCCESS;	
}
CLI_SUBCMD(show, rel_version, cmd_show_rel_verion, "displays Release version", "show rel_version");


int cmd_show_hw_version(cmd_tbl_t *t, int argc, char *argv[])
{
	char *hw_version = "HW1.0.0";
	os_printf(LM_CMD, LL_INFO, "Hardware version is %s\n",hw_version);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, hw_version, cmd_show_hw_version, "displays hardware version", "show hw_version");
#endif



#if defined (CONFIG_CMD_SHOW_SDK_VERSION)
int cmd_show_sdk_verion(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD,LL_INFO,"SDK version is %s\n",sdk_version);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, sdk_version, cmd_show_sdk_verion, "displays SDK version", "show sdk_version");

int cmd_show_wifi_verion(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD,LL_INFO,"Wi-Fi Component Firmware Version is %s\n", wifi_version);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, wifi_version, cmd_show_wifi_verion, "displays wifi version", "show wifi_version");
#endif



#if defined (CONFIG_CMD_SHOW_REL_VERSION)
int show_reset_type(cmd_tbl_t *t, int argc, char *argv[])
{
	hal_reset_type_init();
	RST_TYPE temp = hal_get_reset_type();
	os_printf(LM_CMD, LL_INFO, "show_reset_type\r\n");
	switch(temp)
	{
		case RST_TYPE_POWER_ON: os_printf(LM_CMD, LL_INFO, "RST_TYPE_POWER_ON\n"); break;
    	case RST_TYPE_FATAL_EXCEPTION: os_printf(LM_CMD, LL_INFO, "RST_TYPE_FATAL_EXCEPTION\n"); break;
    	case RST_TYPE_SOFTWARE_REBOOT: os_printf(LM_CMD, LL_INFO, "RST_TYPE_SOFTWARE_REBOOT\n"); break;
    	case RST_TYPE_HARDWARE_REBOOT: os_printf(LM_CMD, LL_INFO, "RST_TYPE_HARDWARE_REBOOT\n"); break;
    	case RST_TYPE_OTA: os_printf(LM_CMD, LL_INFO, "RST_TYPE_OTA\n"); break;
    	case RST_TYPE_WAKEUP: os_printf(LM_CMD, LL_INFO, "RST_TYPE_WAKEUP\n"); break;
    	case RST_TYPE_SOFTWARE: os_printf(LM_CMD, LL_INFO, "RST_TYPE_SOFTWARE\n"); break;
    	case RST_TYPE_HARDWARE_WDT_TIMEOUT: os_printf(LM_CMD, LL_INFO, "RST_TYPE_HARDWARE_WDT_TIMEOUT\n"); break;
    	case RST_TYPE_SOFTWARE_WDT_TIMEOUT: os_printf(LM_CMD, LL_INFO, "RST_TYPE_SOFTWARE_WDT_TIMEOUT\n"); break;
		case RST_TYPE_UNKOWN: os_printf(LM_CMD, LL_INFO, "RST_TYPE_UNKOWN\n"); break;
		default:
			os_printf(LM_CMD, LL_INFO, "RST_TYPE error\n"); break;
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, reset_type, show_reset_type, "show reset type", "show reset_type");
#endif



#if defined (CONFIG_OTA_LOCAL) || defined(CONFIG_OTA_SERVICE)
int show_ota_get_active_part(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int start_addr;
    unsigned int part_size;
    image_headinfo_t image_head;

    if (partion_info_get(PARTION_NAME_CPU, &start_addr, &part_size) != 0)
    {
        os_printf(LM_APP, LL_ERR, "can not get %s info\n", PARTION_NAME_CPU);
        return CMD_RET_FAILURE;
    }
    //os_printf(LM_APP, LL_INFO, "partition start addr 0x%x part size 0x%x\n", start_addr, part_size);
    drv_spiflash_read(start_addr, (unsigned char *)&image_head, sizeof(image_headinfo_t));
    switch (image_head.update_method)
    {
        case OTA_UPDATE_METHOD_AB:
            os_printf(LM_APP, LL_INFO, "update method is AB\n");
			extern void *__etext1;
    		unsigned int text_addr = ((unsigned int)&__etext1) - 0x40800000U;
			
//		    os_printf(LM_APP, LL_INFO, "start_addr=0x%08x\n", start_addr);
//		    os_printf(LM_APP, LL_INFO, "part_size=0x%08x\n", part_size);
//		    os_printf(LM_APP, LL_INFO, "__etext1=0x%08x\n", text_addr);
		
		    if (text_addr > start_addr + part_size / 2)
		    {
		        os_printf(LM_APP, LL_INFO, "active part B\n");
		    } 
			else 
			{
		    	os_printf(LM_APP, LL_INFO, "active part A\n");
		    }
			
            break;
        case OTA_UPDATE_METHOD_CZ:
            os_printf(LM_APP, LL_INFO, "update method is compress\n");
            break;
        case OTA_UPDATE_METHOD_DI:
            os_printf(LM_APP, LL_INFO, "update method is DIFF\n");
            break;
        default:
            os_printf(LM_APP, LL_ERR, "unkown method %d\n", image_head.update_method);
			break;
    }
    return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, ota, show_ota_get_active_part, "show ota mode and ab part", "show ota");

#ifdef CONFIG_MQTT
int subcmd_ota_mqtt_ping(cmd_tbl_t *h, int argc, char *argv[])
{	
	extern uint ping_resp;
	extern uint ping_num;

	os_printf(LM_CMD, LL_INFO, "PING num:%d   RESP num:%d\n", ping_num, ping_resp);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(show, ota_mqtt_ping, subcmd_ota_mqtt_ping, "show mqtt ping num", "show OTA_MQTT_PING");
#endif
#endif


extern int cmd_show_stack(cmd_tbl_t *t, int argc, char *argv[]);
CLI_SUBCMD(show, stack, cmd_show_stack, "displays stack status", "show stack");



int cmd_show_lib_compile_info(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_CMD, LL_INFO, "SDK version is %s\n",sdk_version);
	os_printf(LM_CMD, LL_INFO, "SDK version lib:%s %s\n", 
		sdk_version_lib_compile_date, sdk_version_lib_compile_time);
#if defined (CONFIG_PSM_SURPORT)
	os_printf(LM_CMD, LL_INFO, "psm lib:%s %s\n", 
		psm_lib_compile_date, psm_lib_compile_time);
#endif
#if defined (CONFIG_ECR6600_WIFI)
	os_printf(LM_CMD, LL_INFO, "wifi lib:%s %s\n", 
		wifi_lib_compile_date, wifi_lib_compile_time);
#endif
#if defined (CONFIG_ECR_BLE)
	os_printf(LM_CMD, LL_INFO, "ble lib:%s %s\n", 
		ble_lib_compile_date, ble_lib_compile_time);
#endif
	return CMD_RET_SUCCESS;	
}
CLI_SUBCMD(show, lib_compile_info, cmd_show_lib_compile_info, "displays lib compile info", "show lib_compile_info");






CLI_CMD(show, NULL, "displays detail information of the XXX option", "show <XXX>");



