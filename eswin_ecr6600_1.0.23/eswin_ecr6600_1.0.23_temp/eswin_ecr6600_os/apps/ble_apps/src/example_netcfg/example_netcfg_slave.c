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
static int ble_apps_task_handle = 0;

uint8_t ssid_length;
static char g_ssid[64];
static uint8_t password_length;
static char g_password[64];


device_rsp_ssid_issue *ssid_issue;
device_rsp_ssid_issue *ssid_issue;
device_rsp_pwd_issue *pwd_issue;
device_rsp_start_dis_network *start_dis_netwok;
decive_rsp_net_status *net_status;
device_rsp_ssid_issue *ssid_issue;
device_rsp_pwd_issue *pwd_issue;
device_rsp_start_dis_network *start_dis_netwok;
device_rsp_stop_dis_network	*stop_dis_network;

static ECR_BLE_GATTS_PARAMS_T		   ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T 	   ecr_ble_common_service[ECR_BLE_GATT_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T		   ecr_ble_common_char[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
#define  COMMON_SERVICE_MAX_NUM      (1)
#define  COMMON_CHAR_MAX_NUM         (3)
	 
#define  BLE_CMD_SERVICE_UUID                 (0x180b)
#define  BLE_CMD_WRITE_CHAR_UUID              (0x2a2d)
#define  BLE_CMD_READ_CHAR_UUID               (0x2a2e)
#define  BLE_CMD_NOTIFY_CHAR_UUID             (0x2a2f)
static uint8_t adv_data[]	   = {2,1,6,3,2,1,0xa2,4,0x16,1,0xa2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t scan_rsp_data[] = {4,9,'N','E','T',0x19,0xff,0xd0,7,0x80,3,0,0,0xc,0,0x5f,0x9f,0x6e,3,0x22,0,0x95,0xd8,0xa,0xf2,0x87,0x2e,0xda,0xac,0x83,0x7e};

extern int8_t hex2num(char c);
extern wifi_status_e wifi_get_status(int vif);
void ecr_ble_gatts_value_notify(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);

void ecr_ble_gatts_service_add(ECR_BLE_GATTS_PARAMS_T *p_service, bool is_ues_deatult);
extern bool wifi_get_ip_info(wifi_interface_e if_index, struct ip_info *info);
extern sys_err_t wifi_get_state(void);
extern BT_RET_T ecr_ble_adv_param_set(ECR_BLE_DATA_T CONST *p_adv, ECR_BLE_DATA_T CONST *p_scan_rsp);
extern BT_RET_T ecr_ble_gap_adv_start(ECR_BLE_GAP_ADV_PARAMS_T CONST *p_adv_params);
extern sys_err_t wifi_stop_station(void);
extern sys_err_t wifi_start_station(wifi_config_u *config);
sys_err_t wpa_get_wifi_info(wifi_info_t *info);

enum netcfg_mode
{
	M_TRANSPORT = 0xE1,
	M_NETCFG = 0xE2,
};
enum wifi_netcfg_type
{
	WIFI_TYPE_WPA = 0xF1,
	WIFI_TYPE_WEP = 0xF2,
	WIFI_TYPE_OPEN = 0xF3,
};

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

enum netcfg_req_pkt
{
	NETCFG_MODE_SWTICH_REQ = 0x00,
	NETCFG_NET_STATUS_REQ = 0x01,
	NETCFG_SSID_REQ = 0x02,
	NETCFG_PWD_REQ = 0x03,
	NETCFG_START_NETCFG_REQ = 0x04,
	NETCFG_STOP_NETCFG_REQ = 0x05,
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
#if 0
static void set_net_status(void)
{
	switch(wifi_get_status(STATION_IF))
	{
		case  STA_STATUS_CONNECTED:
			memcpy(g_net_status, NET_STATUS_VALUE_2, strlen(NET_STATUS_VALUE_2)+1);
			break;
		case  STA_STATUS_START:
		case  STA_STATUS_STOP:
		case  STA_STATUS_DISCON:
		default:
			memcpy(g_net_status, NET_STATUS_VALUE_1, strlen(NET_STATUS_VALUE_1)+1);
			break;
	}
}		
#endif
static void ecr_ble_netcfg_handler(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	//int8_t i=hex2num(p_event->gatt_event.write_report.report.p_data[1]);
	switch(p_event->gatt_event.write_report.report.p_data[0])
	{
		case NETCFG_DISTRIBUTE_NETWORK_STATUS:
		{
				wifi_status_e status;
				status=wifi_get_status(STATION_IF);
				if(status==STA_STATUS_CONNECTED)
				{
					os_printf(LM_CMD, LL_INFO, "already distribute network!\n");
					net_status->type=NETCFG_NET_STATUS_RSP;
					net_status->state=STA_CONNECTED;
					net_status->ssid_length=0;
					net_status->length=sizeof(net_status)+1;
					ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)net_status,sizeof(net_status));
					break;
				}
				else 
				{
					os_printf(LM_CMD, LL_INFO, "start distribute network……!\n");
					//netcfg_flag=true;
					net_status->type=NETCFG_NET_STATUS_RSP;
					net_status->state=STA_NOT_CONNECTED;
					net_status->ssid_length=0;
					net_status->length=sizeof(net_status)+1;
					os_printf(LM_CMD, LL_INFO, "net_status->length=%d\n",net_status->length);
					ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)net_status,sizeof(net_status));
				}
		};break;
		
		case NETCFG_SSID_ISSUE:
		{

			if((p_event->gatt_event.write_report.report.length-2>=64)||(p_event->gatt_event.write_report.report.length-2<=0)||(p_event->gatt_event.write_report.report.length-2!=p_event->gatt_event.write_report.report.p_data[1]))
			{
				
				os_printf(LM_CMD, LL_INFO, "the length of ssid error!\n");
				ssid_issue->type=NETCFG_SSID_RSP;
				ssid_issue->rsp_result=NETCFG_SSID_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)net_status,sizeof(net_status));
				break;
			}
			else
			{
				memcpy(g_ssid,&p_event->gatt_event.write_report.report.p_data[2],p_event->gatt_event.write_report.report.length-2);
				ssid_length = p_event->gatt_event.write_report.report.p_data[1];
				ssid_issue->type=NETCFG_SSID_RSP;
				ssid_issue->rsp_result=NETCFG_NO_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t*)ssid_issue,sizeof(ssid_issue));
			}
		};break;
		
		case NETCFG_PASSWORD_ISSUE:
		{
			if((p_event->gatt_event.write_report.report.length-2>=64)||(p_event->gatt_event.write_report.report.length-2<0)||(p_event->gatt_event.write_report.report.length-2!=p_event->gatt_event.write_report.report.p_data[1]))
			{
				os_printf(LM_CMD, LL_INFO, "the length of password error!\n");
				pwd_issue->type=NETCFG_PWD_RSP;
				pwd_issue->rsp_result=NETCFG_PASSWORD_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)pwd_issue,sizeof(pwd_issue));
			}
			else
			{
				os_printf(LM_CMD, LL_INFO, "this is password!\n");
				memcpy(g_password,&p_event->gatt_event.write_report.report.p_data[2],p_event->gatt_event.write_report.report.length-2);
				password_length = p_event->gatt_event.write_report.report.p_data[1];
				pwd_issue->type=NETCFG_PWD_RSP;
				pwd_issue->rsp_result=NETCFG_NO_ERROR;
				ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)pwd_issue,sizeof(pwd_issue));
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
			if(STA_STATUS_CONNECTED==wifi_get_status(STATION_IF)&&!ip4_addr_isany(if_ip.ip))
			#endif
			{
				os_printf(LM_APP, LL_INFO, "net config start3\n");
				
			}
			if(STA_STATUS_STOP==wifi_get_status(STATION_IF))
			{
				os_printf(LM_APP, LL_ERR, ",wifi active stop station\n");
				
			}
			start_dis_netwok->type=NETCFG_START_NETCFG_RSP;
			start_dis_netwok->state=STA_CONNECTED;
			start_dis_netwok->ssid_length=p_event->gatt_event.write_report.report.p_data[1];
			start_dis_netwok->length=p_event->gatt_event.write_report.report.p_data[1]+2;
			//memcpy(start_dis_netwok->ssid,&p_event->gatt_event.write_report.report.p_data[2],p_event->gatt_event.write_report.report.length-2);
			ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)start_dis_netwok,sizeof(start_dis_netwok));
		};break;
		
		case NETCFG_STOP_DISTRIBUTE_NETWORK:
		{
			
			wifi_stop_station();
			stop_dis_network->type=NETCFG_STOP_NETCFG_RSP;
			stop_dis_network->rsp_result=NETCFG_STOP_NETCFG_ERROR;
			ecr_ble_gatts_value_notify(p_event->conn_handle,p_event->gatt_event.write_report.char_handle,(uint8_t *)stop_dis_network,sizeof(stop_dis_network));
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
		case ECR_BLE_GAP_EVT_RESET:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_reset\n");
			
			break;
		case ECR_BLE_GAP_EVT_CONNECT:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_connect\n");
			break;
		case ECR_BLE_GAP_EVT_ADV_STATE:
			os_printf(LM_CMD, LL_INFO, "adv state report type %d\n", p_event->gap_event.adv_report.adv_type);
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
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gaTT_event_write\n");
			ecr_ble_netcfg_handler(p_event);
			break;
		case ECR_BLE_GATT_EVT_NOTIFY_TX:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gaTT_event_notity\n");
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
	
	/*Add Read && write characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_READ_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_READ | ECR_BLE_GATT_CHAR_PROP_WRITE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	ecr_ble_reset();
	ecr_ble_gatts_service_add(p_ble_service, false);

}
static void ecr_ble_adv_start(void)
{
	ECR_BLE_DATA_T p_adv_data;
	ECR_BLE_DATA_T p_scan_rsp_data;
	p_adv_data.length = sizeof(adv_data)/sizeof(adv_data[0]);
	p_adv_data.p_data = adv_data;
	p_scan_rsp_data.length = sizeof(scan_rsp_data)/sizeof(scan_rsp_data[0]);
	p_scan_rsp_data.p_data = scan_rsp_data;
	ecr_ble_adv_param_set(&p_adv_data, &p_scan_rsp_data);
	ECR_BLE_GAP_ADV_PARAMS_T adv_param;
	adv_param.adv_type=ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
	memset(&(adv_param.direct_addr),0,sizeof(ECR_BLE_GAP_ADDR_T));
	adv_param.adv_interval_max=64;
	adv_param.adv_interval_min=64;
	adv_param.adv_channel_map=0x07;
	ecr_ble_gap_adv_start(&adv_param);
}

static void ecr_netcfg_slave_init(void *arg)
{
	ecr_netcfg_task_init(); 
	ecr_ble_adv_start();
	os_task_delete(ble_apps_task_handle);
	ble_apps_task_handle = 0;
}

void ble_apps_init(void)
{
	ble_apps_task_handle=os_task_create( "ble_apps_task", BLE_APPS_PRIORITY, BLE_APPS_STACK_SIZE, (task_entry_t)ecr_netcfg_slave_init, NULL);
}


