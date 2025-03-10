/**
 ****************************************************************************************
 *
 * @file example_netcfg_slave.c
 *
 * @brief slave network config example
 *
 * @par Copyright (C):
 *      ESWIN 2015-2020
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "example_netcfg_slave.h"
#include "bluetooth.h"
#include "system_wifi.h"
#include "system_network.h"
#include "system_config.h"
#include "ble_thread.h"
#include "oshal.h"
#include "os_task_config.h"
/*
 * DEFINES
 ****************************************************************************************
 */
#define NETCFG_SSID_LEN			((uint16_t)(0x0040))
#define NETCFG_PASSWORD_LEN		((uint16_t)(0x0040))
#define HEAD_LEN              (2)
#define DEVICE_NAME_LEN          (18)
#define GATT_VALUE_MAX_LEN    	 (252)
#define BLE_CMD_SERVICE_UUID                 (0x180b)
#define BLE_CMD_WRITE_CHAR_UUID              (0x2a2d)
#define BLE_CMD_READ_CHAR_UUID               (0x2a2e)
#define BLE_CMD_NOTIFY_CHAR_UUID             (0x2a2f)
#define COMMON_SERVICE_MAX_NUM      (1)
#define COMMON_CHAR_MAX_NUM         (3)
#define ADV_MODE_LEN               (3)
#define ADV_DATA_LEN              (31)
#define SCAN_RSP_DATA_LEN         (31)
#define ADV_MODE                        "\x02\x01\x06"
#define APP_NETCFG_ADV_DATA_UUID        "\x03\x03\x18\x0b"
#define APP_NETCFG_ADV_DATA_UUID_LEN    (4)
#define APP_NETCFG_ADV_DATA_APPEARANCE           "\x03\x19\x03\x00"
#define APP_NETCFG_ADV_DATA_APPEARANCE_LEN       (4)


static int ble_apps_task_handle = 0;
static ECR_BLE_GATTS_PARAMS_T		   ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T 	   ecr_ble_common_service[COMMON_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T		   ecr_ble_common_char[COMMON_CHAR_MAX_NUM] = {0}; 
static os_timer_handle_t ind_timer=NULL;
static uint16_t con_handle=0;

enum wifi_sta_status
{
	STA_CONNECTED = 0x00,
	STA_NOT_CONNECTED = 0x01,
};

enum netcfg_result_info
{
	NETCFG_NO_ERROR = 0x00,
	NETCFG_MODE_SWITCH_ERROR = 0x01,
	NETCFG_WIFI_INFO_ERROR = 0x02,
	NETCFG_STOP_NETCFG_ERROR = 0x03,
	NETCFG_SSID_ERROR=0x04,
	NETCFG_PASSWORD_ERROR=0x05,
};

enum netcfg_rsp_pkt
{
	NETCFG_MODE_SWTICH_RSP = 0xF0,
	NETCFG_NET_STATUS_RSP = 0xF1,
	NETCFG_SSID_RSP = 0xF2,
	NETCFG_PWD_RSP = 0xF3,
	NETCFG_START_NETCFG_RSP = 0xF4,
	NETCFG_STOP_NETCFG_RSP = 0xF5,
};
	
enum netcfg_evt_list
{
	NETCFG_DISTRIBUTE_NETWORK_STATUS=0x01,
	NETCFG_SSID_ISSUE=0x02,
	NETCFG_PASSWORD_ISSUE=0x03,
	NETCFG_START_DISTRIBUTE_NETWORK=0x04,
	NETCFG_STOP_DISTRIBUTE_NETWORK=0x05,
};
	
static void ecr_ble_adv_scanRsp_param_set(void )
{
	char dev_name[DEVICE_NAME_LEN]={0};
	uint8_t index=0;
	ECR_BLE_DATA_T p_adv_data;
	ECR_BLE_DATA_T p_scan_rsp_data;
	ecr_ble_get_device_name((uint8_t*)dev_name, DEVICE_NAME_LEN);
	
	static uint8_t scan_rsp_data[SCAN_RSP_DATA_LEN]={0};
	scan_rsp_data[0]=strlen(dev_name)+1;
	scan_rsp_data[1]=0x09; /* Complete name */
	memcpy(&scan_rsp_data[2],dev_name,sizeof(dev_name));
	p_scan_rsp_data.length = sizeof(scan_rsp_data)/sizeof(scan_rsp_data[0]);
	p_scan_rsp_data.p_data = scan_rsp_data;

	static uint8_t adv_data[ADV_DATA_LEN]={0}; 
	//set adv mode --- flag
	memcpy(&adv_data[index], ADV_MODE, ADV_MODE_LEN);
	index += ADV_MODE_LEN;
	//set uuid
	memcpy(&adv_data[index], APP_NETCFG_ADV_DATA_UUID, APP_NETCFG_ADV_DATA_UUID_LEN);
	index += APP_NETCFG_ADV_DATA_UUID_LEN;
	//set Appearance
	memcpy(&adv_data[index], APP_NETCFG_ADV_DATA_APPEARANCE, APP_NETCFG_ADV_DATA_APPEARANCE_LEN);
	index += APP_NETCFG_ADV_DATA_APPEARANCE_LEN;
	//set device name
	adv_data[index] = strlen(dev_name)+1;
	index++;
	adv_data[index++] = 0x09; /* Complete name */
	memcpy(&adv_data[index],dev_name, strlen(dev_name));
	p_adv_data.length = sizeof(adv_data)/sizeof(adv_data[0]);
	p_adv_data.p_data = adv_data;

	ecr_ble_adv_param_set(&p_adv_data, &p_scan_rsp_data);
}

static void ecr_ble_adv_start(void)
{
	ECR_BLE_GAP_ADV_PARAMS_T adv_param;
	adv_param.adv_type=ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
	memset(&(adv_param.direct_addr),0,sizeof(ECR_BLE_GAP_ADDR_T));
	adv_param.adv_interval_max=64;
	adv_param.adv_interval_min=64;
	adv_param.adv_channel_map=0x07;
	ecr_ble_gap_adv_start(&adv_param);
}

static void  get_wifi_status_timer(os_timer_handle_t timer)
{
	static int get_wifi_status_count=0;
	get_wifi_status_count++;
	device_rsp_start_dis_network start_dis_netwok={0};
	wifi_info_t info;
	wifi_get_wifi_info(&info);
	if((wifi_get_status(STATION_IF)!=STA_STATUS_CONNECTED)&&(get_wifi_status_count>=100))
	{
		os_printf(LM_APP, LL_INFO, "Wifi Donot Distribute Network!\n");
		start_dis_netwok.type=NETCFG_START_NETCFG_RSP;
		start_dis_netwok.state=STA_NOT_CONNECTED;
		os_timer_stop(ind_timer);
		get_wifi_status_count=0;
		ecr_ble_gatts_value_notify(con_handle,ecr_ble_common_char[1].handle,(uint8_t *)&start_dis_netwok,start_dis_netwok.ssid_length+3);
	}
	else if(wifi_get_status(STATION_IF)==STA_STATUS_CONNECTED)
	{
		os_printf(LM_APP, LL_INFO, "Wifi  Distribute Network!\n");
		start_dis_netwok.type=NETCFG_START_NETCFG_RSP;
		start_dis_netwok.state=STA_CONNECTED;
		start_dis_netwok.ssid_length=info.ssid_len;
		memcpy(start_dis_netwok.ssid,info.ssid,start_dis_netwok.ssid_length);
		os_timer_stop(ind_timer);
		get_wifi_status_count=0;
		ecr_ble_gatts_value_notify(con_handle,ecr_ble_common_char[1].handle,(uint8_t *)&start_dis_netwok,start_dis_netwok.ssid_length+3);
	}

}

uint8_t skylab_bleNet_flag=0;
wifi_config_u skylab_conf={0};

static void ecr_ble_netcfg_handler(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	static char g_ssid[NETCFG_SSID_LEN];
	static char g_password[NETCFG_PASSWORD_LEN];
	os_printf(LM_APP, LL_INFO, "ecr_ble_netcfg_handler=%d\n",p_event->gatt_event.write_report.report.p_data[0]);
	switch(p_event->gatt_event.write_report.report.p_data[0])
	{
		case NETCFG_DISTRIBUTE_NETWORK_STATUS:
		{
				wifi_status_e status;
				status=wifi_get_status(STATION_IF);
				decive_rsp_net_status net_status;
				memset(g_password,0,NETCFG_PASSWORD_LEN);
				memset(g_ssid,0,NETCFG_SSID_LEN);	
				if(status==STA_STATUS_CONNECTED)
				{
					os_printf(LM_APP, LL_INFO, "Already Distribute Network!\n");
					net_status.type=NETCFG_NET_STATUS_RSP;
					net_status.state=STA_CONNECTED;
					ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&net_status,sizeof(net_status));
					break;
				}
				else 
				{
					os_printf(LM_APP, LL_INFO, "Start Distribute Network!\n");
					net_status.type=NETCFG_NET_STATUS_RSP;
					net_status.state=STA_NOT_CONNECTED;
					ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&net_status,sizeof(net_status));
				}
		};break;
		
		case NETCFG_SSID_ISSUE:
		{
			device_rsp_ssid_issue ssid_issue;
			if((p_event->gatt_event.write_report.report.length-HEAD_LEN>=NETCFG_SSID_LEN)||(p_event->gatt_event.write_report.report.length-HEAD_LEN<=0)||(p_event->gatt_event.write_report.report.length-HEAD_LEN!=p_event->gatt_event.write_report.report.p_data[1]))
			{
				os_printf(LM_APP, LL_INFO, "The Length of Ssid Error!\n");
				ssid_issue.type=NETCFG_SSID_RSP;
				ssid_issue.rsp_result=NETCFG_SSID_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&ssid_issue,sizeof(ssid_issue));
				break;
			}
			else
			{
				os_printf(LM_APP, LL_INFO, "Get Ssid!\n");
				memcpy(g_ssid,&p_event->gatt_event.write_report.report.p_data[2],p_event->gatt_event.write_report.report.length-HEAD_LEN);
				ssid_issue.type=NETCFG_SSID_RSP;
				ssid_issue.rsp_result=NETCFG_NO_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&ssid_issue,sizeof(ssid_issue));
			}
		};break;
		
		case NETCFG_PASSWORD_ISSUE:
		{
			device_rsp_pwd_issue pwd_issue;
			if((p_event->gatt_event.write_report.report.length-HEAD_LEN>=NETCFG_PASSWORD_LEN)||(p_event->gatt_event.write_report.report.length-HEAD_LEN<0)||(p_event->gatt_event.write_report.report.length-HEAD_LEN!=p_event->gatt_event.write_report.report.p_data[1]))
			{
				os_printf(LM_APP, LL_INFO, "The Length of Password Error!\n");
				pwd_issue.type=NETCFG_PWD_RSP;
				pwd_issue.rsp_result=NETCFG_PASSWORD_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&pwd_issue,sizeof(pwd_issue));
			}
			else
			{
				os_printf(LM_APP, LL_INFO, "Get Password!\n");
				memcpy(g_password,&p_event->gatt_event.write_report.report.p_data[2],p_event->gatt_event.write_report.report.length-HEAD_LEN);
				pwd_issue.type=NETCFG_PWD_RSP;
				pwd_issue.rsp_result=NETCFG_NO_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&pwd_issue,sizeof(pwd_issue));
			}
		};break;
		
		case NETCFG_START_DISTRIBUTE_NETWORK:
		{
			wifi_config_u sta_cfg;
			sys_err_t ret =0;
			memset(&sta_cfg,0,sizeof(sta_cfg));
			strlcpy((char *)sta_cfg.sta.ssid, g_ssid , strlen(g_ssid)+1);
			strlcpy(sta_cfg.sta.password, g_password, strlen(g_password)+1);
			wifi_stop_station();
			ret = wifi_start_station(&sta_cfg);
			if (SYS_OK == ret)
			{
				os_printf(LM_APP, LL_INFO, "net config start\n");
				memcpy((void *)&skylab_conf,(void *)&sta_cfg,sizeof(wifi_config_u));
				skylab_bleNet_flag = 1;
			} 
			else 
			{
				os_printf(LM_APP, LL_ERR, "net config error, %d\n", ret);
				
			}
			struct ip_info if_ip;
			wifi_get_ip_info(STATION_IF,&if_ip);
			#ifdef CONFIG_IPV6
			if(STA_STATUS_CONNECTED==wifi_get_status(STATION_IF)&& ! ip4_addr_isany_val(if_ip.ip.u_addr.ip4))
			#else
			if(STA_STATUS_CONNECTED==wifi_get_status(STATION_IF)&&!ip4_addr_isany(&(if_ip.ip)))
			#endif
			{
				os_printf(LM_APP, LL_INFO, "net config start\n");
				
			}
			if(STA_STATUS_STOP==wifi_get_status(STATION_IF))
			{
				os_printf(LM_APP, LL_ERR, "wifi active stop station\n");
				
			}
			
			
			os_timer_start(ind_timer);
			
		};break;
		
		case NETCFG_STOP_DISTRIBUTE_NETWORK:
		{
			os_printf(LM_APP, LL_INFO, "disable distribute network!\n");
			device_rsp_stop_dis_network	stop_dis_network;
			wifi_stop_station();
			stop_dis_network.type=NETCFG_STOP_NETCFG_RSP;
			stop_dis_network.rsp_result=NETCFG_STOP_NETCFG_ERROR;
			ecr_ble_gatts_value_notify(p_event->conn_handle,ecr_ble_common_char[1].handle,(uint8_t *)&stop_dis_network,sizeof(stop_dis_network));
		};break;
		
		default:
		{
			
		//nothing to do
		};
		break;
	}
}

static void ecr_ble_netcfg_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_CONNECT:
			con_handle=p_event->conn_handle;
			break;
		case ECR_BLE_GAP_EVT_DISCONNECT:
			con_handle=0;
			os_timer_stop(ind_timer);
			ecr_ble_adv_start();
			break;
		default:
			break;

	}

}

static void ecr_ble_netcfg_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
			ecr_ble_netcfg_handler(p_event);
			break;
		default:
			break;
	}
}

static void ecr_netcfg_task_init()
{
	ecr_ble_gap_callback_register(ecr_ble_netcfg_gap_event_cb);
	ecr_ble_gatt_callback_register(ecr_ble_netcfg_gatt_event_cb);
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
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = GATT_VALUE_MAX_LEN;
	p_common_char++;

	/*Add Notify characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_NOTIFY_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = GATT_VALUE_MAX_LEN;
	p_common_char++;
	
	/*Add Read && write characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_READ_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_READ | ECR_BLE_GATT_CHAR_PROP_WRITE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = BLE_ATT_MTU_DEF;
	ecr_ble_reset();
	ecr_ble_gatts_service_add(p_ble_service, false);

}

static void ecr_netcfg_slave_init(void *arg)
{
	ecr_netcfg_task_init(); 		
	ecr_ble_adv_scanRsp_param_set();				
	ecr_ble_adv_start();		
	if(ind_timer==NULL) 			   
		ind_timer = os_timer_create("netcfg_timer", 100, 1, get_wifi_status_timer, NULL);		
	ble_apps_task_handle = 0;				 
	os_task_delete(ble_apps_task_handle);

}

int ble_apps_init(void)
{                        
	if(ble_apps_task_handle==0){
		ble_apps_task_handle=os_task_create( "ble_apps_task", BLE_APPS_PRIORITY, BLE_APPS_STACK_SIZE, (task_entry_t)ecr_netcfg_slave_init, NULL);
		return 0;        
	}        
	return 0;
}



