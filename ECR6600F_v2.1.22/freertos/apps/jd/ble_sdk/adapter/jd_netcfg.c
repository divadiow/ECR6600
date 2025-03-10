/**
 ****************************************************************************************
 *
 * @file test_netcfg.c
 *
 * @brief 
 *
 * Copyright (C) ESWIN 2015-2020
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "os_task_config.h"
#include <string.h>
#include "cli.h"//print

#include "system_wifi.h"
#include "system_network.h"
#include "oshal.h"

#if defined (CONFIG_NV)
//#include "nv_config.h"
#include "system_config.h"
#include "easyflash.h"
#endif

#include "joylink_sdk.h"
#include "jd_netcfg.h"

#include "joylink_extern.h"
#include "joylink_flash.h"
/*
 * DEFINES
 ****************************************************************************************
 */

#define NETCFG_BLE_DEV_NAME_LEN		(18)
#define ECR_BLE_GATT_SERVICE_MAX_NUM                                        (3)
#define ECR_BLE_GATT_CHAR_MAX_NUM                                           (3)

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
static jl_gatt_data_t g_jl_gatt_data;
int netcfg_task_handle;
uint8_t connect_id;
static char g_ble_dev_name[NETCFG_BLE_DEV_NAME_LEN];

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static ECR_BLE_GATTS_PARAMS_T		   ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T 	   ecr_ble_common_service[ECR_BLE_GATT_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T		   ecr_ble_common_char[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
ECR_BLE_GAP_CONN_PARAMS_T con_update_data;

#define  COMMON_SERVICE_MAX_NUM      (1)
#define  COMMON_CHAR_MAX_NUM         (2)
	 
#define  BLE_CMD_SERVICE_UUID                 (0xfe70)
#define  BLE_CMD_WRITE_CHAR_UUID              (0xfe71)
#define  BLE_CMD_NOTIFY_CHAR_UUID             (0xfe72)
static uint16_t con_handle;
static int32_t disc_reason;

static void jd_ble_netcfg_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_CONN_PARAM_UPDATE:
			con_update_data.conn_interval_min = 0x10;//Connection interval minimum, ms
			con_update_data.conn_interval_max = 0x20;//Connection interval maximum, ms
			con_update_data.conn_latency  = 0;//Latency
			con_update_data.conn_sup_timeout = 400;//Supervision timeout, ms
			ecr_ble_connect_param_update(p_event->conn_handle,&con_update_data);
			break;
		case ECR_BLE_GAP_EVT_CONNECT:
			con_handle=p_event->conn_handle;
			system_printf("msg_con_ind_handle %d\r\n",p_event->conn_handle);
			break;
		case ECR_BLE_GAP_EVT_DISCONNECT:
			con_handle=p_event->conn_handle;
			disc_reason=p_event->gap_event.disconnect.reason;
			jd_start_advertising();
			jl_disconnet_reset();
			break;
		default:
			break;

	}

}
static void jd_ble_netcfg_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
			system_printf("receive from app write data\r\n");
			jl_write_data(p_event->gatt_event.write_report.report.p_data,p_event->gatt_event.write_report.report.length);
			break;
		case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_TX:
			system_printf("indication success\r\n");
			break;
		default:
			break;
	}

}
static void jd_netcfg_task_init()
{
	ecr_ble_gap_callback_register(jd_ble_netcfg_gap_event_cb);
	ecr_ble_gatt_callback_register(jd_ble_netcfg_gatt_event_cb);
	ECR_BLE_GATTS_PARAMS_T *p_ble_service = &ecr_ble_gatt_service;
	p_ble_service->svc_num =  COMMON_SERVICE_MAX_NUM;
	p_ble_service->p_service = ecr_ble_common_service;

	ECR_BLE_SERVICE_PARAMS_T *p_common_service = ecr_ble_common_service;
	p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
	p_common_service->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID;
	p_common_service->type	   = ECR_BLE_UUID_SERVICE_PRIMARY;
	p_common_service->char_num = COMMON_CHAR_MAX_NUM;
	p_common_service->p_char   = ecr_ble_common_char;
	
	/*Add write characteristic*/
	ECR_BLE_CHAR_PARAMS_T *p_common_char = ecr_ble_common_char;
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_WRITE_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	p_common_char++;

	/*Add Notify characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_NOTIFY_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	p_common_char++;

	ecr_ble_reset();
	ecr_ble_gatts_service_add(p_ble_service, false);

}

void jd_set_adv_scanRsp_data(void)
{
  	uint8_t  i = 0;
	jl_gap_data_t gap_data;
	uint16_t index = 0;
	ECR_BLE_DATA_T p_adv_data;
	ECR_BLE_DATA_T p_scan_data;
	static uint8_t adv_data[31] = {0};
	static uint8_t scan_rsp_data[31]={0};
	//set adv mode --- flag
	adv_data[index++] = 0x02;
	adv_data[index++] = 0x01;
	adv_data[index++] = 0x06;
	jl_get_gap_config_data(&gap_data);
	uint8_t manufacture_data_lenght = sizeof(gap_data.manufacture_data); 
	uint8_t service_uuid_lenght = sizeof(gap_data.service_uuid16); 

	//set uuid
	adv_data[index++] = service_uuid_lenght + 1;
	adv_data[index++] = 0x03; /* 16-bit UUID */

	adv_data[index++] = gap_data.service_uuid16[0];
	adv_data[index++] = gap_data.service_uuid16[1];
	//set manufacture
	adv_data[index++] = manufacture_data_lenght + 1;
	adv_data[index++] = 0xff; /* GAP appearance */
	for(i = 0;i < manufacture_data_lenght;i++)
	    adv_data[index + i] = gap_data.manufacture_data[i];
  	index += manufacture_data_lenght;
	p_adv_data.p_data=adv_data;
	p_adv_data.length=sizeof(adv_data)/sizeof(adv_data[0]);
	//scan data
	scan_rsp_data[0] = strlen(g_ble_dev_name)+1;
	scan_rsp_data[1] = 0x09;
	memcpy(&scan_rsp_data[2],g_ble_dev_name, strlen(g_ble_dev_name));
	p_scan_data.p_data=scan_rsp_data;
	p_scan_data.length=sizeof(scan_rsp_data)/sizeof(scan_rsp_data[0]);
	ecr_ble_adv_param_set(&p_adv_data,&p_scan_data);
}


void jd_start_advertising(void)
{
	ECR_BLE_GAP_ADV_PARAMS_T adv_param;
	adv_param.adv_type=ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
	memset(&(adv_param.direct_addr),0,sizeof(ECR_BLE_GAP_ADDR_T));
	adv_param.adv_interval_max=64;
	adv_param.adv_interval_min=64;
	adv_param.adv_channel_map=0x07;
	ecr_ble_gap_adv_start(&adv_param);
}

void jd_stop_advertising(void)
{
	ecr_ble_gap_adv_stop();
}

void jd_ble_disconnect(void)
{
	ecr_ble_gap_disconnect(con_handle, disc_reason);
}
int32_t jl_adapter_send_data(uint8_t* data, uint32_t data_len)
{
    int ret = 0;
	ecr_ble_gatts_value_notify(con_handle,ecr_ble_common_char[1].handle,data,data_len);
    return ret;
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void  jd_netcfg_task(void *arg)
{
    int ret = 0;
    jl_dev_info_t dev_info;
    static char jlp_uuid[JLP_UUID_LEN] = {0};
    if( jl_flash_GetString("jlp_uuid", (char *)&jlp_uuid,  sizeof(jlp_uuid)) == 0){
        log_info("Get uuid String failed\n");
        return;
    }
	memcpy(dev_info.product_uuid, jlp_uuid, strlen(jlp_uuid));//SDK说明：系统分配的产品唯一标识码，在小京鱼平台注册设备后获取 
	wifi_get_mac_addr(STATION_IF, dev_info.mac);
	system_printf("dev_info mac%.2X%.2X%.2X%.2X%.2X%.2X", dev_info.mac[0], dev_info.mac[1], dev_info.mac[2], dev_info.mac[3], dev_info.mac[4], dev_info.mac[5]);
	ret = jl_init(&dev_info);
	if (ret != 0)
		 printf("j1_init fail\n");

		//SDK说明：获取gatt配置信息
	ret = jl_get_gatt_config_data(&g_jl_gatt_data); 
	if (ret != 0)
		 printf("j1 get gatt config data fail\n");
  #if defined(CONFIG_NV)
  	ecr_ble_get_device_name((uint8_t *)g_ble_dev_name,NETCFG_BLE_DEV_NAME_LEN);
  #endif
	jl_thread_t jl_send_pid;
	jl_send_pid.thread_task = (threadtask)jl_send_thread;
	jl_send_pid.parameter = NULL;//"ble_confirm";
	jl_send_pid.stackSize = 1024;
  	jl_send_pid.priority = BLE_APPS_PRIORITY; 
	jl_platform_thread_start(&jl_send_pid);
	jd_netcfg_task_init();
	jd_set_adv_scanRsp_data();
	jd_start_advertising();
}



