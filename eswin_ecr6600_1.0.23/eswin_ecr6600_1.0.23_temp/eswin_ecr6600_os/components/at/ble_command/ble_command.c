#include "ble_command.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cli.h"
#include "task.h"
#include "event_groups.h"
#include "dce_commands.h"
#include "at_def.h"
#include "at_common.h"
#include <string.h>
//#include "test_netcfg.h"
#include "defaults_profile.h"
#include "at_customer_wrapper.h"
#include "app_api.h"
#include "string.h"
#include "stdint.h"
#include "stddef.h"
#include "ble_command.h"
//#include "gap.h"
//#include <gapm_task.h>
//#include "gapc_task.h"
//#include "co_math.h"
//#include "app.h"
#include "os.h"
//#include "dbg_cli.h"
#define BLE_SEND_QUEUE_NUM (10)

static EventGroupHandle_t at_ble_event_group = NULL;
/**
 ****************************************************************************************
 *
 * @file test_netcfg.c
 *
 * @brief BLE network config demo application
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

//#include <string.h>
//#include "defaults_profile.h"
#if defined (CONFIG_SYSTEM_WIFI_CTRL)
#include "system_wifi.h"
#include "system_network.h"
#include "system_config.h"
#endif
#include "oshal.h"
#if defined (CONFIG_NV)
//#include "nv_config.h"
#include "easyflash.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
#define BLE_NOTIFY_TEST 0
#define BLE_SEND_QUEUE_NUM (10)
#define NETCFG_IND_CFG_LEN	((uint16_t)(0x0002))
#define NETCFG_SSID_LEN		((uint16_t)(0x0040))
#define NETCFG_PASSWORD_LEN	((uint16_t)(0x0040))
#define NETCFG_NET_STATUS_LEN	((uint16_t)(0x040))
#define NETCFG_BLE_DEV_NAME_LEN		(18)

#define TEST_DEVICE_NAME ("eswin")
#define TEST_DEVICE_NAME_2 ("eswin_test")
#define SSID_INIT_VALUE ("ESWIN")
#define PWD_INIT_VALUE	("123456#$%")

#define NET_STATUS_VALUE_1 ("link off")
#define NET_STATUS_VALUE_2 ("connect success")
#define NET_STATUS_VALUE_3 ("connecting...")

/** connectable undirected adv */
#define ADV_MODE                        "\x02\x01\x06"
#define ADV_MODE_LEN                    (3)
#define MACSTR    "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x18\x0b - NetCfg Service UUID
 * --------------------------------------------------------------------------------------
 */
#define APP_NETCFG_ADV_DATA_UUID        "\x03\x03\x18\x0b"
#define APP_NETCFG_ADV_DATA_UUID_LEN    (4)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - ble device
 * --------------------------------------------------------------------------------------
 */
#define APP_NETCFG_ADV_DATA_APPEARANCE           "\x03\x19\x03\x00"
#define APP_NETCFG_ADV_DATA_APPEARANCE_LEN       (4)

#define BLE_DEVICE_NAME_MAX_LEN (18)
#define BLE_DEVIVE_NAME         "lillBLE"
#define MACSTR_LEN  17

#define BLE_BD_ADDR						"BleMacAddr"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
int netcfg_task_handle;

uint8_t connect_id;

static char g_ssid[NETCFG_SSID_LEN];
static char g_password[NETCFG_PASSWORD_LEN];
static char g_net_status[NETCFG_NET_STATUS_LEN];
static char g_ble_dev_name[NETCFG_BLE_DEV_NAME_LEN];

static uint16_t g_ssid_cfg;
static uint16_t g_password_cfg;
static uint16_t g_net_status_cfg;
static os_timer_handle_t ind_timer;
typedef struct dce_ dce_t;
enum netconfig_status_type
{
	NETCFG_READY = 0,
	NETCFG_DUR = 1,
	NETCFG_END = 2,
};

static uint8_t netconfig_status;
static bool is_ssid_change;
static bool is_pwd_change;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

bool AT_is_valid_mac(const char *mac_addr)
{
  if(strlen(mac_addr) != MACSTR_LEN){
      return false;
  }
  int i = 0;
  for(i = 0; i < MACSTR_LEN;i++){
      if((i+1)%3 == 0){
          if(mac_addr[i] != ':')
            return false;
          else
            continue;
      } else if((mac_addr[i]>='0' && mac_addr[i]<='9') || 
                (mac_addr[i]>='a' && mac_addr[i]<='f') || 
                (mac_addr[i]>='A' && mac_addr[i]<='F')){
            continue;
      } else {
          return false;
      }
  }
  return true;
}

void AT_hex2MACAddr(uint8_t *hexStr, uint8_t* mac )
{
	uint8_t i;
	uint8_t *currHexStrPos = hexStr;
	uint8_t tempValue;
	/* convert hex string to lower */
	for ( i = 0; i < strlen((char*)hexStr); i++ )
	{
    	hexStr[i] = tolower(hexStr[i]);
	}
	/* convert to numbers */
	for (i = 0; i < 6; i++)
	{
    	tempValue = 0;
    	if ( *currHexStrPos >= 'a' )
        		tempValue = *currHexStrPos - 'a' + 10;
    	else
        		tempValue = *currHexStrPos - '0';
    	
		currHexStrPos++;
    	tempValue <<= 4;

    	if ( *currHexStrPos >= 'a' )
        		tempValue += *currHexStrPos - 'a' + 10;
    	else
        		tempValue += *currHexStrPos - '0';

    	currHexStrPos += 2;
      mac[i] = tempValue;
	}
}

#if 1
static uint16_t AT_test_read16p(void const *ptr16)
{
	uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
	return value;
}
#endif
/**
* @brief Handles reception of the attribute info request message.
* @param[in] the message received.
*/
static void AT_msg_att_info_req_handle(struct s_send_msg msg)
{
	struct s_recv_msg recv_msg;
	uint16_t length = 0;

	recv_msg.msg_id = MSG_ID_ATT_INFO_RSP;
	recv_msg.connect_id = msg.connect_id;
	recv_msg.handle = msg.handle;
	recv_msg.param_len = sizeof(uint16_t);

	switch(msg.handle)
	{
		case NETCFG_IDX_SSID_IND_CFG:
		case NETCFG_IDX_PASSWORD_IND_CFG:
		case NETCFG_IDX_NET_STATUS_IND_CFG:
			length = NETCFG_IND_CFG_LEN;
			recv_msg.param = &length;
			break;
		case NETCFG_IDX_SSID_VAL:
			length = NETCFG_SSID_LEN;
			recv_msg.param = &length;
			break;
		case NETCFG_IDX_PASSWORD_VAL:
			length = NETCFG_PASSWORD_LEN;
			recv_msg.param = &length;
			break;
		default:
			recv_msg.param_len = 0;
			recv_msg.param = NULL;
			break;
	}

	ble_app_send_msg(&recv_msg);
}

/**
* @brief Handles reception of the attribute write request message.
* @param[in] the message received.
*/
static void AT_msg_write_req_handle(struct s_send_msg msg)
{
	os_printf(LM_APP, LL_INFO, "msg_write_req_handle start\r\n");
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_WRITE_RSP;
	recv_msg.connect_id = msg.connect_id;
	recv_msg.handle = msg.handle;
	recv_msg.param_len = msg.param_len;
	recv_msg.param = msg.param;

	switch(msg.handle)
	{
		case NETCFG_IDX_SSID_IND_CFG:	
			if(msg.param_len == sizeof(uint16_t))
			{
				g_ssid_cfg = AT_test_read16p(msg.param);
			}
			break;
		case NETCFG_IDX_PASSWORD_IND_CFG:
			if(msg.param_len == sizeof(uint16_t))
			{
				g_password_cfg = AT_test_read16p(msg.param);
			}
			break;
		case NETCFG_IDX_NET_STATUS_IND_CFG:
			if(msg.param_len == sizeof(uint16_t))
			{
				g_net_status_cfg = AT_test_read16p(msg.param);
			}
			os_printf(LM_APP, LL_INFO, "g_net_status_cfg: %d\r\n",g_net_status_cfg);
			break;
		case NETCFG_IDX_SSID_VAL:
			memset(g_ssid,0,NETCFG_SSID_LEN);
			memcpy(g_ssid, msg.param, msg.param_len);
			os_printf(LM_APP, LL_INFO, "g_ssid: %s, connect_id: %d\r\n",g_ssid, msg.connect_id);
			is_ssid_change = true;
			break;
		case NETCFG_IDX_PASSWORD_VAL:
			memset(g_password,0,NETCFG_PASSWORD_LEN);
			memcpy(g_password, msg.param, msg.param_len);
			os_printf(LM_APP, LL_INFO, "g_password: %s\r\n",g_password);
			is_pwd_change = true;
			break;
		default:
			break;
	}
	ble_app_send_msg(&recv_msg);
}

/**
* @brief Handles reception of the attribute read request message.
* @param[in] the message received.
*/
static void AT_msg_read_req_handle(struct s_send_msg msg)
{
	os_printf(LM_APP, LL_INFO, "msg_read_req_handle start\r\n");
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_READ_RSP;
	recv_msg.connect_id = msg.connect_id;
	recv_msg.handle = msg.handle;

	switch(msg.handle)
	{
		case NETCFG_IDX_SSID_IND_CFG:
			recv_msg.param = &g_ssid_cfg;
			recv_msg.param_len = sizeof(uint16_t);
			break;
		case NETCFG_IDX_PASSWORD_IND_CFG:	
			recv_msg.param = &g_ssid_cfg;
			recv_msg.param_len = sizeof(uint16_t);
			break;
		case NETCFG_IDX_SSID_VAL:
			recv_msg.param = g_ssid;
			recv_msg.param_len = strlen(g_ssid);
			break;
		case NETCFG_IDX_PASSWORD_VAL:
			recv_msg.param = g_password;
			recv_msg.param_len = strlen(g_password);
			break;
		case NETCFG_IDX_NET_STATUS_VAL:
			recv_msg.param = g_net_status;
			recv_msg.param_len = strlen(g_net_status);
			break;
		default:
			recv_msg.param = NULL;
			recv_msg.param_len = 0;
			break;
	}

	ble_app_send_msg(&recv_msg);
}

/**
* @brief Handles disconnection complete event.
* @param[in] the message received.
*/
static void AT_msg_discon_rsp_handle(struct s_send_msg msg)
{
	switch (netconfig_status)
	{
		case (NETCFG_READY):
		{
			memcpy(g_ssid, SSID_INIT_VALUE, strlen(SSID_INIT_VALUE)+1);
			memcpy(g_password, PWD_INIT_VALUE, strlen(PWD_INIT_VALUE)+1);
			
			netconfig_status = NETCFG_READY;
		}break;

		case (NETCFG_DUR):
		{
			os_timer_stop(ind_timer);

			netconfig_status = NETCFG_END;
		}break;

		case (NETCFG_END):
		{

		}break;

		default:
		{

		}break;
	}
	g_net_status_cfg = 0;
	AT_start_advertising();
}

/**
* @brief Handles connection complete event.
* @param[in] the message received.
*/
static void AT_msg_con_ind_handle(struct s_send_msg msg)
{
	connect_id = msg.connect_id;
	os_printf(LM_APP, LL_INFO, "msg_con_ind_handle %d\r\n",connect_id);
}

/**
* @brief Handles connection rssi msg.
* @param[in] the message received.
*/

/**
* @brief Handles scan report msg in scan phase.
* @param[in] the message received.
*/

static void AT_process_msg(struct s_send_msg msg)
{
	uint8_t msg_id = msg.msg_id;

	switch(msg_id)
	{
		case MSG_ID_ATT_INFO_REQ:
			AT_msg_att_info_req_handle(msg);
			break;
		case MSG_ID_WRITE_REQ:
			AT_msg_write_req_handle(msg);
			break;
		case MSG_ID_READ_REQ:
			AT_msg_read_req_handle(msg);
			break;
		case MSG_ID_DISCON_RSP:
			AT_msg_discon_rsp_handle(msg);
			break;
		case MSG_ID_CON_IND:
			AT_msg_con_ind_handle(msg);
			break;
		case MSG_ID_CON_RSSI_IND:
			//AT_msg_con_rssi_ind_handle(msg);
			break;
		case MSG_ID_SCAN_REPORT_IND:
			//AT_msg_scan_report_ind_handle(msg);
			break;
		case MSG_ID_INDICATION_RSP:
			{
				os_printf(LM_APP, LL_INFO, "MSG_ID_INDICATION_RSP\r\n");
				uint8_t status = (uint8_t)(uint32_t)msg.param;
				if ((status == APP_ERR_NO_ERROR) && (msg.handle = NETCFG_IDX_NET_STATUS_VAL))
				{
					os_printf(LM_APP, LL_INFO, "indication success\r\n");
				}
				break;
			}
		default:
			break;
	}
}
#if 1
static void AT_ind_timeout(os_timer_handle_t timer)
{
	struct s_send_msg send_msg;

	send_msg.msg_id = 0xff;
	send_msg.connect_id = connect_id;
	send_msg.handle = 0;
	send_msg.param_len = 0;
	send_msg.param = NULL;
	os_queue_send(task1_queue, (char *)&send_msg, sizeof(struct s_send_msg),0);
}

static void AT_run_loop()
{
	int ret = 0;
	struct s_send_msg s_new_msg;

	while(1)
	{	

		memset(&s_new_msg, 0, sizeof(struct s_send_msg));
		ret = os_queue_receive(task1_queue, (char *)&s_new_msg, sizeof(struct s_send_msg), 0xFFFFFFFF);
		if(ret)
		{
			//receive ERROR
			continue;
		}
		AT_process_msg(s_new_msg);

		#if BLE_NOTIFY_TEST
		notify_net_status();
		#endif
	}
}
#endif 
/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void AT_reset_adv_data(void)
{
	struct s_recv_msg recv_msg;

	uint16_t index = 0;
	static uint8_t adv_data[31] = {0};

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
	adv_data[index++] = sizeof(TEST_DEVICE_NAME_2)+1;
	adv_data[index++] = 0x09; /* Complete name */
	memcpy(&adv_data[index],TEST_DEVICE_NAME_2, sizeof(TEST_DEVICE_NAME_2));
	index += sizeof(TEST_DEVICE_NAME_2);

	recv_msg.msg_id = MSG_ID_SET_ADV_DATA;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = &adv_data;
	recv_msg.param_len = index;

	ble_app_send_msg(&recv_msg);
}

void AT_set_adv_data(void)
{
	struct s_recv_msg recv_msg;

	uint16_t index = 0;
	static uint8_t adv_data[31] = {0};

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
	adv_data[index++] = strlen(g_ble_dev_name)+1;
	adv_data[index++] = 0x09; /* Complete name */
	memcpy(&adv_data[index],g_ble_dev_name, strlen(g_ble_dev_name));
	index += strlen(g_ble_dev_name);

	recv_msg.msg_id = MSG_ID_SET_ADV_DATA;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = &adv_data;
	recv_msg.param_len = index;

	ble_app_send_msg(&recv_msg);
}

void AT_set_scan_rsp_data(void)
{
	struct s_recv_msg recv_msg;
	static uint8_t scan_rsp_data[31] = {0};

	scan_rsp_data[0] = strlen(g_ble_dev_name)+1;
	scan_rsp_data[1] = 0x09;
	memcpy(&scan_rsp_data[2],g_ble_dev_name, strlen(g_ble_dev_name));
	recv_msg.param_len = strlen(g_ble_dev_name)+2;

	recv_msg.msg_id = MSG_ID_SET_SCAN_RSP_DATA;
	recv_msg.connect_id = 0;
	recv_msg.handle = 0;
	recv_msg.param = &scan_rsp_data[0];

	ble_app_send_msg(&recv_msg);
}

void AT_start_advertising(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_START_ADV;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
}

void AT_stop_advertising(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_ADV;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
}

void AT_start_scan(void)
{
	struct s_recv_msg recv_msg;
	static struct ble_scan_param scan_param;

	scan_param.type = SCAN_TYPE_GEN_DISC;
	scan_param.prop = SCAN_PROP_PHY_1M_BIT | SCAN_PROP_ACTIVE_1M_BIT;
	scan_param.scan_intv = 48;//30ms - 48 slots
	scan_param.scan_wd = 48;
	scan_param.duration = 500;// Scan duration (in unit of 10ms). 500*10ms
	scan_param.period = 0;

	recv_msg.msg_id = MSG_ID_START_SCAN;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = (void *)&scan_param;
	recv_msg.param_len = sizeof(struct ble_scan_param);

	ble_app_send_msg(&recv_msg);



}

void AT_stop_scan(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_SCAN;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

}

void AT_ble_disconnect(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_DISCON_REQ;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
}

/** @brief update the connect param of current connection.
 *  @param[in] intv_min  Connection interval minimum(ms)
 *  @param[in] intv_max  Connection interval maximum(ms)
 *  @param[in] latency
 *  @param[in] time_out  Supervision timeout(ms)
 */
void AT_connect_param_update(uint16_t intv_min, uint16_t intv_max, uint16_t latency, uint16_t time_out)
{
	struct s_recv_msg recv_msg;
	static uint16_t con_param_data[4] = {0};

	con_param_data[0] = intv_min;//Connection interval minimum, ms
	con_param_data[1] = intv_max;//Connection interval maximum, ms
	con_param_data[2] = latency;//Latency
	con_param_data[3] = time_out;//Supervision timeout, ms

	recv_msg.msg_id = MSG_ID_CON_PARAM_UPDATE_REQ;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = 0;
	recv_msg.param = &con_param_data;
	recv_msg.param_len = sizeof(con_param_data);

	ble_app_send_msg(&recv_msg);
}

void AT_ble_get_con_rssi(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_GET_PEER_DEV_INFO;
	recv_msg.connect_id = 0;
	recv_msg.handle = 0;
	recv_msg.param = (void *)GET_CON_RSSI;

	ble_app_send_msg(&recv_msg);
}

void AT_ble_get_con_rssi_sync(void)
{
	struct s_recv_msg recv_msg;
	int32_t rssi = 0;
	void *rsp;

	recv_msg.msg_id = MSG_ID_GET_PEER_DEV_INFO;
	recv_msg.connect_id = 0;
	recv_msg.handle = 0;
	recv_msg.param = (void *)GET_CON_RSSI;

	ble_app_send_msg_sync(&recv_msg, &rsp);

	rssi = (int32_t)rsp;

	os_printf(LM_APP, LL_INFO, "ble_get_con_rssi_sync rssi =  %d\r\n", rssi);
}
#if 1
void AT_test_netcfg_task(void *arg)
{
	task1_queue = os_queue_create("ble_send_queue", BLE_SEND_QUEUE_NUM, sizeof(struct s_send_msg), 0);

	g_ssid_cfg = 1;
	g_password_cfg = 1;
	g_net_status_cfg = 0;
	is_ssid_change = false;
	is_pwd_change = false;
	
	netconfig_status = NETCFG_READY;
	memcpy(g_ssid, SSID_INIT_VALUE, strlen(SSID_INIT_VALUE)+1);
	memcpy(g_password, PWD_INIT_VALUE, strlen(PWD_INIT_VALUE)+1);
	#if defined (CONFIG_SYSTEM_WIFI_CTRL1111)
	AT_set_net_status();
	#endif

	#if defined(CONFIG_NV1)
	int status = customer_get_env_blob(BLE_DEVIVE_NAME, g_ble_dev_name, NETCFG_BLE_DEV_NAME_LEN, NULL);
	if((0 == status) || (0xffffffff == status))
	#endif
	{
		memcpy(g_ble_dev_name, TEST_DEVICE_NAME, sizeof(TEST_DEVICE_NAME));
	}

	ind_timer = os_timer_create("auto indication", 1000, 1, AT_ind_timeout, NULL);

	AT_set_adv_data();
	AT_set_scan_rsp_data();
	//start_advertising();

	AT_run_loop();

	return;
}
#endif
static void start_adv_ble(void)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_START_ADV;
	recv_msg.task_id = netcfg_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
}
 
dce_result_t dce_handle_BLEADV_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    
	start_adv_ble();
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}
dce_result_t dce_handle_SETADDR_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    AT_stop_advertising();
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;  
}
dce_result_t dce_handle_BLEDISCON_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
    
	AT_ble_disconnect();
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}
dce_result_t dce_handle_BLEGETMAC_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv,char* mac_type)
{
	
    int ret;
	//DCE_DEBUG("argc=%d",argc);
    unsigned char decv_temp_mac[6]={'\0'};
    char mac[6]={0};
	//DCE_DEBUG("rx_buffer=%s\r\n",group_ctx->rx_buffer);
    #if 1
	if(DCE_WRITE==kind)
	{
		//DCE_DEBUG("argc=%d\r\n",argc);
        if(argc!=1||argv[0].type!=ARG_TYPE_STRING||(true!=AT_is_valid_mac(argv[0].value.string)))
		{
			//DCE_DEBUG("invalid arguents %s %d",__func__,__LINE__);
			dce_emit_basic_result_code(dce,DCE_RC_ERROR);
			return DCE_RC_ERROR;
		}
        //DCE_DEBUG("mac1=%02x:%02x:%02x:%02x:%02x:%02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		//AT_hex2MACAddr((uint8_t*)argv[0].value.string,mac);
        memcpy(mac,argv[0].value.string,strlen(argv[0].value.string));
        //DCE_DEBUG("******************111\r\n");
        //os_printf(LM_CMD,LL_INFO,"mac=%02x:%02x:%02x:%02x:%02x:%02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        //customer_set_env_blob(BLE_BD_ADDR,argv[0].value.string,strlen(argv[0].value.string));
        hal_system_set_mac(MAC_SAVE_TO_AMT,mac);
        //DCE_DEBUG("BLE_BD_ADDR=%s\r\n",BLE_BD_ADDR);
	}
    #endif
    else  if(DCE_READ==kind)
    {
	    char mac_str[18]={0};
        if(1)
        {
			if(strstr(mac_type,"BLEGETMAC"))
			{
            	ret=hal_system_get_config(STA_MAC,decv_temp_mac,sizeof(decv_temp_mac));
				sprintf(mac_str, MACSTR, MAC2STR(decv_temp_mac));
				arg_t result[] = {{ARG_TYPE_STRING,.value.string=mac_str}};
				//DCE_DEBUG(".value.string=%s,%s,%d\r\n",mac_str,__func__,__LINE__);
				dce_emit_extended_result_code_with_args(dce, mac_type, -1, result, 1, 1,false);
			}
			 else
            {
                dce_emit_basic_result_code(dce,DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
            if(ret)
            {
               // DCE_DEBUG("ble_bd_addr:%02x:%02x:%02x:%02x:%02x:%02x:\n\r",decv_temp_mac[0],decv_temp_mac[1],decv_temp_mac[2],decv_temp_mac[3],decv_temp_mac[4],decv_temp_mac[5]);
                dce_emit_basic_result_code(dce,DCE_RC_OK);
                return DCE_RC_OK;
            }
            else
            {
                dce_emit_basic_result_code(dce,DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
            dce_emit_basic_result_code(dce,DCE_RC_OK);
            return DCE_RC_OK;
            
        }
        else
        {
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
		
		
    }
    dce_emit_basic_result_code(dce,DCE_RC_OK);
    return DCE_OK;

}
dce_result_t dce_handle_BLEADDR_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
   //dce_emit_extended_result_code(dce,"BLE get mac:",-1,1);
   dce_handle_BLEGETMAC_command(dce,group_ctx,kind,argc,argv,"BLEGETMAC");
    
   return DCE_OK;
}
dce_result_t dce_hanle_SETNAME_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
	if(DCE_READ==kind)
	{
		char temp_name[BLE_DEVICE_NAME_MAX_LEN]={'\0'};
		int ret=hal_system_get_config(CUSTOMER_NV_BLE_DEVIVE_NAME,temp_name,sizeof(temp_name));
		arg_t result[]={{ARG_TYPE_STRING,.value.string=temp_name}};
		dce_emit_extended_result_code_with_args(dce, "BLENAME", -1, result, 1, 1, false);
			
		if(ret)
		{
			dce_emit_basic_result_code(dce,DCE_RC_OK);
			return DCE_RC_OK;
		}
		else
		{
		    dce_emit_basic_result_code(dce,DCE_RC_ERROR);
			return DCE_RC_ERROR;
		}
			
	}
    else if(DCE_WRITE==kind)
    {
        
        char decv_temp_name[BLE_DEVICE_NAME_MAX_LEN]={'\0'};
        memcpy(decv_temp_name,argv[0].value.string,strlen(argv[0].value.string));
        if(EF_NO_ERR==hal_system_set_config(CUSTOMER_NV_BLE_DEVIVE_NAME,decv_temp_name,sizeof(decv_temp_name)))
        {
            // os_printf(LM_APP, LL_INFO, "SET ble name\r\n");
            //dce_emit_pure_response(dce,argv[0].value.string,strlen(argv[0].value.string));
            dce_emit_basic_result_code(dce,DCE_RC_OK);
            return DCE_RC_OK;
        }
        else
        {
            //os_printf(LM_APP, LL_INFO, "SET ble name1\r\n");
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        //os_printf(LM_APP, LL_INFO, "SET ble name2\r\n");
        
    }
    dce_emit_basic_result_code(dce,DCE_RC_OK);
    return DCE_OK;
}
#if 0
dce_result_t dce_handle_SETADVDATA_command(dce_t* dce,void*group_ctx,int kind,size_t argc,arg_t *argv)
{
	///DCE_DEBUG("set ble adv data:%s,%d\r\n",__FILE__,__LINE__);

	if(DCE_WRITE==kind){
		struct s_recv_msg recv_msg;
	    uint16_t index = 0;
	    static uint8_t adv_data[31]={0};
	    index += ADV_MODE_LEN;
		memcpy(&adv_data[index],argv[0].value.string,ADV_MODE_LEN);
		index += ADV_MODE_LEN;
		memcpy(&adv_data[index],argv[1].value.string,APP_NETCFG_ADV_DATA_UUID_LEN);
		index +=APP_NETCFG_ADV_DATA_UUID_LEN;
		memcpy(&adv_data[index],argv[2].value.string,APP_NETCFG_ADV_DATA_APPEARANCE_LEN);
		index += APP_NETCFG_ADV_DATA_APPEARANCE_LEN;
		adv_data[index++]=strlen(argv[3].value.string)+1 ;
		adv_data[index++]=0x09;
		memcpy(&adv_data[index],argv[3].value.string,strlen(argv[3].value.string));
		index += strlen(argv[3].value.string);
		recv_msg.msg_id = MSG_ID_SET_ADV_DATA;
		recv_msg.task_id = netcfg_task_handle;//name change
		recv_msg.handle = 0;
		recv_msg.param=&adv_data;
		recv_msg.param_len = index;
		ble_app_send_msg(&recv_msg);

		static uint8_t scan_rsp_data[31]={0};
		scan_rsp_data[0]=sizeof(argv[3].value.string);
		scan_rsp_data[1]=0x09;
		memcpy(&scan_rsp_data,argv[3].value.string,strlen(argv[3].value.string));
		recv_msg.param_len=sizeof(argv[3].value.string)+2;
		recv_msg.msg_id=MSG_ID_SET_SCAN_RSP_DATA;
		recv_msg.connect_id=0;
		recv_msg.handle=0;
		recv_msg.param=&scan_rsp_data[0];
		ble_app_send_msg(&recv_msg);
	}
	dce_emit_basic_result_code(dce,DCE_RC_OK);
	return DCE_OK;
}
dce_result_t dce_handle_SETADVPARAM_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
	//uint8_t flag[3]={0};
	struct gapm_activity_create_adv_cmd *p_cmd=KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,TASK_GAPM,TASK_APP,gapm_activity_create_adv_cmd);
	if(DCE_WRITE==kind)
	{
		p_cmd->operation=GAPM_CREATE_ADV_ACTIVITY;
		p_cmd->own_addr_type = (unsigned int )argv[2].value.string;
        //DCE_DEBUG("OWN ADDR TYPE=%d\r\n",p_cmd->own_addr_type);
		p_cmd->adv_param.type = (unsigned int)argv[6].value.string;
		p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
		p_cmd->adv_param.filter_pol = (unsigned int)argv[4].value.string;
       /// DCE_DEBUG("p_cmd->adv_param.filter_pol =%d\n\r",p_cmd->adv_param.filter_pol);
		p_cmd->adv_param.prim_cfg.chnl_map = (unsigned int)argv[3].value.string;
        //DCE_DEBUG("p_cmd->adv_param.prim_cfg.chnl_map =%d\r\n",p_cmd->adv_param.prim_cfg.chnl_map);
		p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;
		p_cmd->adv_param.max_tx_pwr = 10;
		p_cmd->adv_param.disc_mode = (unsigned int)argv[5].value.string;
        //DCE_DEBUG("p_cmd->adv_param.disc_mode =%d\r\n",p_cmd->adv_param.disc_mode);
		p_cmd->adv_param.prim_cfg.adv_intv_min= (unsigned int)argv[0].value.string;
        //DCE_DEBUG("p_cmd->adv_param.prim_cfg.adv_intv_min =%d\r\n",p_cmd->adv_param.prim_cfg.adv_intv_min);
		p_cmd->adv_param.prim_cfg.adv_intv_max= (unsigned int)argv[1].value.string;
        //DCE_DEBUG("p_cmd->adv_param.prim_cfg.adv_intv_max =%d\r\n",p_cmd->adv_param.prim_cfg.adv_intv_max);
		ke_msg_send(p_cmd); 
	}
    dce_emit_basic_result_code(dce,DCE_RC_OK);
	return DCE_OK;
	

}
void AT_default_app_set_adv_data(uint8_t *data, uint16_t data_len)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);
	
    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_ADV_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;

    p_cmd->length = 0;
    
	// GAP will use 3 bytes for the AD Type
	if (data_len>(ADV_DATA_LEN - 3))
	{
		p_cmd->length = ADV_DATA_LEN - 3;
	}else
	{
		p_cmd->length = data_len;
	}
	memcpy(&p_cmd->data[0], data, data_len);

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    //app_env.adv_state = APP_ADV_STATE_SETTING_ADV_DATA;
    // And the next expected operation code for the command completed event
    //app_env.adv_op = GAPM_SET_ADV_DATA;
}
void AT_default_app_set_scan_rsp_data(uint8_t *data, uint16_t data_len)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_SCAN_RSP_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;

    p_cmd->length = data_len;
    memcpy(&p_cmd->data[0], data, data_len);

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    //app_env.adv_state = APP_ADV_STATE_SETTING_SCAN_RSP_DATA;
    // And the next expected operation code for the command completed event
    //app_env.adv_op = GAPM_SET_SCAN_RSP_DATA;
}

dce_result_t dce_handle_BLESETADVDATA_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t* argv)
{
    
    if(DCE_WRITE==kind)
    {
      // AT_default_app_set_adv_data(argv[0].value.string,(unsigned int)argv[1].value.string);
	  // AT_default_app_set_scan_rsp_data(argv[0].value.string,(unsigned int)argv[1].value.string);
	   #if 1
	   struct gapm_set_adv_data_cmd* p_cmd=KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,TASK_GAPM,TASK_APP,gapm_set_adv_data_cmd,ADV_DATA_LEN);
        uint8_t data[]={'\0'};
        uint16_t data_len=0;
        p_cmd->operation=GAPM_SET_ADV_DATA;
        p_cmd->actv_idx=app_env.adv_actv_idx;
        p_cmd->length=0;
        if(data_len>(ADV_DATA_LEN-3))
        {
            p_cmd->length=ADV_DATA_LEN-3;
        }
        else
        {
            p_cmd->length=data_len;
        }
        
        //data =argv[0].value.string;
        data_len = (unsigned int)argv[1].value.string;
		// memcpy(data,argv[0].value.string,data_len);
        //DCE_DEBUG("argv[0].value.string=%s\r\n",argv[0].value.string);
        //DCE_DEBUG("data_len=%d\r\n",data_len);
        //DCE_DEBUG("data=%s\r\n",data);
        memcpy(&p_cmd->data[0],argv[0].value.string,strlen(argv[0].value.string));
        //int size=strlen(& p_cmd->data[0]);
        //for(int i=0;i<data_len;i++){
       // DCE_DEBUG("ble_set_adv_data:P_CMD->data=%c\r\n",p_cmd->data[i]);}
        ke_msg_send(p_cmd);
		p_cmd=KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,TASK_GAPM,TASK_APP,gapm_set_adv_data_cmd,ADV_DATA_LEN);
		p_cmd->operation =GAPM_SET_SCAN_RSP_DATA;
		p_cmd->actv_idx=app_env.adv_actv_idx;
		p_cmd->length=data_len;
		memcpy(&p_cmd->data[0],argv[0].value.string,data_len);
		 for(int i=0;i<data_len;i++){
        //DCE_DEBUG("ble_set_adv_data1111:P_CMD->data=%c\r\n",p_cmd->data[i]);}
		ke_msg_send(p_cmd);
		#endif
    }
	
    dce_emit_basic_result_code(dce,DCE_RC_OK);
    return DCE_OK;
}
#endif
dce_result_t dce_handle_BLESTARTSCAN_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
    //struct s_send_msg msg={0};
	AT_start_scan();
	//AT_msg_scan_report_ind_handle(&msg);
	//struct ble_scan_report_ind *scan_report = (struct ble_scan_report_ind *)msg.param;
	//DCE_DEBUG("****************RSSI=%d,%s,%d\r\n",scan_report->rssi,__func__,__LINE__);
	//while(1);
	//DCE_DEBUG("****************LENGTH=%d,%s,%d\r\n",scan_report->length,__func__,__LINE__);
	//while(1);
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}



static const command_desc_t BLE_commands[]={
   
    { "BLEADDR",      &dce_handle_BLEADDR_command,        DCE_READ|DCE_WRITE},
	{ "BLEADVSTART",  &dce_handle_BLEADV_command,         DCE_EXEC},
    { "BLEADVSTOP",   &dce_handle_SETADDR_command,        DCE_EXEC},
    { "BLEDISCONN",   &dce_handle_BLEDISCON_command,      DCE_EXEC},
    { "BLENAME" ,     &dce_hanle_SETNAME_command,         DCE_READ|DCE_WRITE},
    { "BLESCAN",      &dce_handle_BLESTARTSCAN_command,   DCE_EXEC},
};

void at_init_ble_event_group(void)
{
    if(at_ble_event_group){
        return;
    }
    at_ble_event_group= xEventGroupCreate();
}

void dce_register_ble_commands(dce_t* dce)
{
    dce_register_command_group(dce,"BLE",BLE_commands,sizeof(BLE_commands)/sizeof(command_desc_t),0);
    at_init_ble_event_group();

}