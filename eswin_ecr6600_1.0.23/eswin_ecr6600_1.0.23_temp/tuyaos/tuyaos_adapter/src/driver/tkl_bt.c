/**
 * @file tuya_os_adapt_bt.c
 * @brief bt操作接口
 * 
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 * 
 */
#include "bluetooth.h"
#include "oshal.h"
#include "os_task_config.h"
#include <stdint.h>
#include <string.h>           // in order to use memset
#include "hci_interface.h"
#include "tkl_hci.h"

#if 0
/***********************************************************
*************************micro define***********************
***********************************************************/
#define BLE_CTRL_SEND_QUEUE_NUM (10)


/***********************************************************
*************************variable define********************
***********************************************************/
static const TUYA_OS_BT_INTF m_tuya_os_bt_intfs = {
    .port_init      = tuya_os_adapt_bt_port_init,
    .port_deinit    = tuya_os_adapt_bt_port_deinit,
    .gap_disconnect = tuya_os_adapt_bt_gap_disconnect,
    .send           = tuya_os_adapt_bt_send,
    .reset_adv      = tuya_os_adapt_bt_reset_adv,
    .get_rssi       = tuya_os_adapt_bt_get_rssi,
    .start_adv      = tuya_os_adapt_bt_start_adv,
    .stop_adv       = tuya_os_adapt_bt_stop_adv,
    .assign_scan    = tuya_os_adapt_bt_assign_scan,
    .scan_init      = tuya_os_adapt_bt_scan_init,
    .start_scan     = tuya_os_adapt_bt_start_scan,
    .stop_scan      = tuya_os_adapt_bt_stop_scan,
    .get_ability    = tuya_os_adapt_bt_get_ability,
};

static ty_bt_param_t bt_init_p = {0};
static ty_bt_scan_info_t bt_scan_info_s;
static int ble_ctrl_task_handle = 0;
static uint8_t connect_id;
static os_sem_handle_t sync_scan_sem;
static int connect_status = 0;
static int adv_status = 0;
static tuya_ble_data_buf_t bt_adv_data = {0};
static tuya_ble_data_buf_t bt_scan_rsp_data = {0};


/***********************************************************
*************************function define********************
***********************************************************/

extern void cli_printf(const char *f, ...);
static char* mystrstr(const char* dest, int dest_len, const char* src)
{
	char* tdest = dest;
	char* tsrc = src;
	int i = 0;
	int j = 0;
	while (i < dest_len && j < strlen(tsrc))
	{
		if (tdest[i] == tsrc[j])//字符相等，则继续匹配下一个字符
		{
			i++;
			j++;
		}
		else
		{
			i = i - j + 1;
			j = 0;
		}
	}
	if (j == strlen(tsrc))
	{
		return tdest + i - strlen(tsrc);
	}
 
	return NULL;
}
static void ble_scan_report_handle(struct s_send_msg msg)
{
	struct ble_scan_report_ind *scan_report_info = (struct ble_scan_report_ind *)msg.param;

	switch (bt_scan_info_s.scan_type)
	{
		case TY_BT_SCAN_BY_NAME:
			if (NULL != mystrstr((char *)scan_report_info->data, scan_report_info->length, bt_scan_info_s.name))
			{
				bt_scan_info_s.rssi = scan_report_info->rssi;
				memcpy(bt_scan_info_s.bd_addr, scan_report_info->addr, 6);
				if(NULL != sync_scan_sem)
				{
					os_sem_post(sync_scan_sem);
				}
				os_printf(LM_APP, LL_INFO, "====by name rssi: %d ====\r\n", bt_scan_info_s.rssi);
			}
			break;
		case TY_BT_SCAN_BY_MAC:
			if (0 == memcmp(bt_scan_info_s.bd_addr, scan_report_info->addr, 6))
			{
				bt_scan_info_s.rssi = scan_report_info->rssi;
				if(NULL != sync_scan_sem)
				{
					os_sem_post(sync_scan_sem);
				}
				os_printf(LM_APP, LL_INFO, "====by mac rssi: %d ====\r\n", bt_scan_info_s.rssi);
			}
			break;
		default:
			break;
	}
}

static void hal_process_msg(struct s_send_msg msg)
{
	uint8_t msg_id = msg.msg_id;

	switch(msg_id)
	{
		case MSG_ID_WRITE_REQ:
			{
				struct s_recv_msg recv_msg;

				recv_msg.msg_id = MSG_ID_WRITE_RSP;
				recv_msg.connect_id = msg.connect_id;
				recv_msg.handle = msg.handle;
				recv_msg.param_len = msg.param_len;
				recv_msg.param = msg.param;

				if((recv_msg.handle == NETCFG_IDX_WRITE_VAL) && (bt_init_p.cb != NULL))
				{
					tuya_ble_data_buf_t cb_buf;

					cb_buf.len = msg.param_len;
					cb_buf.data = msg.param;
					bt_init_p.cb(msg.connect_id,TY_BT_EVENT_RX_DATA,&cb_buf);
				}
				ble_app_send_msg(&recv_msg);
			}break;
		case MSG_ID_DISCON_RSP:
			connect_status = 0;
			if(bt_init_p.cb != NULL)
			{
				bt_init_p.cb(msg.connect_id,TY_BT_EVENT_DISCONNECTED,NULL);
			}
			break;
		case MSG_ID_CON_IND:
			connect_id = msg.connect_id;
			adv_status = 0;
			connect_status = 1;
			if (bt_init_p.cb != NULL)
			{
				bt_init_p.cb(msg.connect_id,TY_BT_EVENT_CONNECTED,NULL);
			}
			break;
		case MSG_ID_SCAN_REPORT_IND:
			ble_scan_report_handle(msg);
			break;
		default:
			break;
	}
}

static void ble_ctrl_loop()
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
		hal_process_msg(s_new_msg);
	}
}

static void ble_ctrl_task(void *arg)
{
    connect_id = 0;
    connect_status = 0;
    adv_status = 0;

	task1_queue = os_queue_create("ble_send_queue", BLE_CTRL_SEND_QUEUE_NUM, sizeof(struct s_send_msg), 0);

	ble_ctrl_loop();
}

static void ble_set_adv_data(tuya_ble_data_buf_t *adv)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_SET_ADV_DATA;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = adv->data;
	recv_msg.param_len = adv->len;

	ble_app_send_msg(&recv_msg);
}

static void ble_set_scan_rsp_data(tuya_ble_data_buf_t *scan_rsp)
{
	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_SET_SCAN_RSP_DATA;
	recv_msg.connect_id = 0;
	recv_msg.handle = 0;
	recv_msg.param = scan_rsp->data;
	recv_msg.param_len = scan_rsp->len;

	ble_app_send_msg(&recv_msg);
}

/**
 * @brief tuya_os_adapt_bt 蓝牙初始化
 * @return int 
 */
 int tuya_os_adapt_bt_port_init(ty_bt_param_t *p)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_port_init\n");

    if((NULL == p)&&(NULL == p->cb)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if(ble_ctrl_task_handle != 0)
    {
        os_printf(LM_APP, LL_INFO, "bt port already init, do not repetitive init!!!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    memcpy(&bt_init_p, p, sizeof(ty_bt_param_t));
	
	ble_set_device_name((uint8_t *)bt_init_p.name, strlen(bt_init_p.name));
	ble_main();

	ble_ctrl_task_handle=os_task_create( "ble_ctrl-task", BLE_CTRL_PRIORITY, BLE_CTRL_STACK_SIZE, (task_entry_t)ble_ctrl_task, NULL);

	memcpy(&bt_adv_data, &bt_init_p.adv, sizeof(tuya_ble_data_buf_t));
	memcpy(&bt_scan_rsp_data, &bt_init_p.scan_rsp, sizeof(tuya_ble_data_buf_t));

	tuya_os_adapt_bt_start_adv();
	if(bt_init_p.cb != NULL)
	{
		//inform ble ready, ty will call reset adv
		bt_init_p.cb(0,TY_BT_EVENT_ADV_READY,NULL);
	}
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 蓝牙断开关闭
 * @return int 
 */
int tuya_os_adapt_bt_port_deinit(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_port_deinit\n");
    if(ble_ctrl_task_handle == 0)
    {
        os_printf(LM_APP, LL_INFO, "bt port is not init, please do not deinit!!!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    os_task_delete(ble_ctrl_task_handle);
    ble_ctrl_task_handle = 0;
    os_queue_destory(task1_queue);
    ble_destory();

    return OPRT_OS_ADAPTER_OK;

}


/**
 * @brief tuya_os_adapt_bt 蓝牙断开
 * @return int 
 */
int tuya_os_adapt_bt_gap_disconnect(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_gap_disconnect\n");
    if (!connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is not connected, can't disconnect!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_DISCON_REQ;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief tuya_os_adapt_bt 蓝牙发送
 * @return int 
 */
int tuya_os_adapt_bt_send(const unsigned char *data, const unsigned char len)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_send\n");
    if (!connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is not connected, can't send data!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    if (0 == len || NULL == data) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_NOTIFICATION;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = NETCFG_IDX_NOTIFY_VAL;
	recv_msg.param = (unsigned char *)data;
	recv_msg.param_len = len;

	ble_app_send_msg(&recv_msg);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 广播包重置
 * @return int 
 */
int tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_reset_adv\n");

    memcpy(&bt_adv_data, adv, sizeof(tuya_ble_data_buf_t));
    memcpy(&bt_scan_rsp_data, scan_resp, sizeof(tuya_ble_data_buf_t));

    if (connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is connected, can't reset adv!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

	//reset adv data and re-start adv
	tuya_os_adapt_bt_stop_adv();

	tuya_os_adapt_bt_start_adv();

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 获取rssi信号值
 * @return int 
 */
int tuya_os_adapt_bt_get_rssi(signed char *rssi)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_get_rssi\n");
    if(connect_status != 0)
    {

        struct s_recv_msg recv_msg;
        void *rsp;

        recv_msg.msg_id = MSG_ID_GET_PEER_DEV_INFO;
        recv_msg.connect_id = connect_id;
        recv_msg.handle = 0;
        recv_msg.param = (void *)GET_CON_RSSI;

        ble_app_send_msg_sync(&recv_msg, &rsp);

        *rssi = (signed char)(int)rsp;
        return OPRT_OS_ADAPTER_OK;
    }
    return OPRT_OS_ADAPTER_COM_ERROR;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int 
 */
int tuya_os_adapt_bt_start_adv(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_start_adv\n");
    if (connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is connected, can't start adv!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    if (adv_status)
    {
        os_printf(LM_APP, LL_INFO, "err: already advertising, can't start adv again!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    ble_set_adv_data(&bt_adv_data);
    ble_set_scan_rsp_data(&bt_scan_rsp_data);

	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_START_ADV;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
    adv_status = 1;

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int 
 */
int tuya_os_adapt_bt_stop_adv(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_stop_adv\n");
    if (connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is connected, can't stop adv!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_ADV;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    adv_status = 0;
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 主动扫描
 * @return int 
 */
int tuya_os_adapt_bt_assign_scan(IN OUT ty_bt_scan_info_t *info)
{
    if (connect_status)
    {
        os_printf(LM_APP, LL_INFO, "err: status is connected, can't scan!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    if (NULL == info) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if(ble_ctrl_task_handle == 0)
    {
        os_printf(LM_APP, LL_INFO, "bt is not init! please init bt first!\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    int ret = 0;
    memcpy(&bt_scan_info_s, info, sizeof(ty_bt_scan_info_t));
    struct s_recv_msg recv_msg;
	static struct ble_scan_param scan_param;

	scan_param.type = SCAN_TYPE_CONN_DISC;//SCAN_TYPE_GEN_DISC;
	scan_param.prop = SCAN_PROP_PHY_1M_BIT | SCAN_PROP_ACTIVE_1M_BIT;
	scan_param.scan_intv = 48;//30ms - 48 slots
	scan_param.scan_wd = 48;
	scan_param.duration = bt_scan_info_s.timeout_s * 100;// Scan duration (in unit of 10ms).
	scan_param.period = 0;

	recv_msg.msg_id = MSG_ID_START_SCAN;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = (void *)&scan_param;
	recv_msg.param_len = sizeof(struct ble_scan_param);

	sync_scan_sem = os_sem_create(1, 0);

	ble_app_send_msg(&recv_msg);

	ret = os_sem_wait(sync_scan_sem, bt_scan_info_s.timeout_s * 1000);
	os_sem_destroy(sync_scan_sem);
	sync_scan_sem = NULL;

	if (0 == ret)
	{
		tuya_os_adapt_bt_stop_scan();
		memcpy(info, &bt_scan_info_s, sizeof(ty_bt_scan_info_t));
		return OPRT_OS_ADAPTER_OK;
	}
	else
	{
		tuya_os_adapt_bt_stop_scan();
		return OPRT_OS_ADAPTER_COM_ERROR;
	}
}

/**
 * @brief tuya_os_adapt_bt 开始scan接收
 * @return int 
 */
int tuya_os_adapt_bt_start_scan(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_start_scan\n");

    //todo

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 停止scan接收
 * @return int 
 */
int tuya_os_adapt_bt_stop_scan(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_stop_scan\n");

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_SCAN;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt scan初始化
 * @return int 
 */
int tuya_os_adapt_bt_scan_init(IN TY_BT_SCAN_ADV_CB scan_adv_cb)
{
    os_printf(LM_APP, LL_INFO, "tuya_os_adapt_bt_scan_init\n");

    //todo
    return OPRT_OS_ADAPTER_OK;
}

int tuya_os_adapt_bt_get_ability(void)
{
    return BT_ABI_NEED_RESET_STACK;
}

/**
 * @brief tuya_os_adapt_reg_bt_intf 接口注册
 * @return int
 */
int tuya_os_adapt_reg_bt_intf(void)
{
    return tuya_os_adapt_reg_intf(INTF_BT, (void *)&m_tuya_os_bt_intfs);
}
#endif


#if 0
#include "system_config.h"
#include "tkl_bluetooth.h"

static TKL_BLE_GAP_EVT_FUNC_CB g_gap_evt;
static TKL_BLE_GATT_EVT_FUNC_CB g_gatt_evt;

static void ecr_ble_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_CONNECT:
		{
			TKL_BLE_GAP_PARAMS_EVT_T event;
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_connect\n");
			
			event.result = OPRT_OK;
			event.conn_handle = p_event->conn_handle;
			event.type = TKL_BLE_GAP_EVT_CONNECT;
			event.gap_event.connect.role = p_event->gap_event.connect.role;
			event.gap_event.connect.peer_addr.type = p_event->gap_event.connect.peer_addr.type;
			memcpy(&event.gap_event.connect.peer_addr.addr[0],&p_event->gap_event.connect.peer_addr.addr[0],6);
			memcpy(&event.gap_event.connect.conn_params,&p_event->gap_event.connect.conn_params,sizeof(TKL_BLE_GAP_CONN_PARAMS_T));
			g_gap_evt(&event);
		}
		break;
		
		case ECR_BLE_GAP_EVT_DISCONNECT:
		{
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_disconnect:0x %x\n",p_event->gap_event.disconnect.reason);
			
			TKL_BLE_GAP_PARAMS_EVT_T event;
			event.type = TKL_BLE_GAP_EVT_DISCONNECT;
			event.conn_handle = p_event->conn_handle;
			event.result = p_event->result;
			event.gap_event.disconnect.reason = p_event->gap_event.disconnect.reason;
			event.gap_event.disconnect.role = TKL_BLE_ROLE_SERVER;
			g_gap_evt(&event);
		}
		break;
		case ECR_BLE_GAP_EVT_ADV_STATE:
		{
			TKL_BLE_GAP_PARAMS_EVT_T event;
			event.type = TKL_BLE_GAP_EVT_ADV_STATE;
			event.conn_handle = p_event->conn_handle;
			event.result = p_event->result;

			event.gap_event.adv_report.adv_type = p_event->gap_event.adv_report.adv_type;
			os_printf(LM_CMD, LL_INFO, "adv state report type %d\n", p_event->gap_event.adv_report.adv_type);
			g_gap_evt(&event);
		}
		break;
		case ECR_BLE_GAP_EVT_ADV_REPORT:
		{

			TKL_BLE_GAP_PARAMS_EVT_T event;
			event.type = TKL_BLE_GAP_EVT_ADV_REPORT;
			event.conn_handle = p_event->conn_handle;
			event.result = p_event->result;
			
			event.gap_event.adv_report.adv_type = p_event->gap_event.adv_report.adv_type;
			event.gap_event.adv_report.peer_addr.type = p_event->gap_event.adv_report.peer_addr.type;
			memcpy(event.gap_event.adv_report.peer_addr.addr, p_event->gap_event.adv_report.peer_addr.addr, 6);
			event.gap_event.adv_report.rssi = p_event->gap_event.adv_report.rssi;
			event.gap_event.adv_report.channel_index = p_event->gap_event.adv_report.channel_index;
			event.gap_event.adv_report.data.length = p_event->gap_event.adv_report.data.length;
			event.gap_event.adv_report.data.p_data = p_event->gap_event.adv_report.data.p_data;

			g_gap_evt(&event);				
		 }	
		break;

		default:
			break;
	}
	
}

static void ecr_ble_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
		{
		
			TKL_BLE_GATT_PARAMS_EVT_T event;
			
			event.type = TKL_BLE_GATT_EVT_WRITE_REQ;
			event.result = p_event->result;
			event.conn_handle = p_event->conn_handle;
		
			event.gatt_event.write_report.char_handle = p_event->gatt_event.write_report.char_handle;
			event.gatt_event.write_report.report.length = p_event->gatt_event.write_report.report.length;
			event.gatt_event.write_report.report.p_data = p_event->gatt_event.write_report.report.p_data;

			g_gatt_evt(&event);
		}
		break;

		case ECR_BLE_GATT_EVT_NOTIFY_TX:
		{
			// os_printf(LM_BLE, LL_INFO, "TKL_EVT_NOTIFY_SUCCESEE\r\n");
			
			TKL_BLE_GATT_PARAMS_EVT_T event;
			event.type = TKL_BLE_GATT_EVT_NOTIFY_TX;
			event.result = p_event->result;
			
			if(0 == event.result)
			{
				g_gatt_evt(&event);
			}
		}
		break;

		case ECR_BLE_GATT_EVT_READ_RX:
		{
			TKL_BLE_GATT_PARAMS_EVT_T event;
			event.type = p_event->type;

			g_gatt_evt(&event);
		}
		break;
		default:
			break;
	}

}
/**
 * @brief   Function for initializing the ble stack
 * @param   role                Indicate the role for ble stack.
 *                              role = 0: ble peripheral    @TKL_BLE_ROLE_SERVER
 *                              role = 1: ble central       @TKL_BLE_ROLE_CLIENT
 * @return  SUCCESS             Initialized successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_stack_init(UCHAR_T role)
{	
	os_printf(LM_BLE,LL_INFO,"tkl_ble_stack_init\n");
	ecr_ble_gap_callback_register(ecr_ble_gap_event_cb);
	ecr_ble_gatt_callback_register(ecr_ble_gatt_event_cb);
	return OPRT_OK;
}


/**
 * @brief   Function for de-initializing the ble stack features
 * @param   role                 Indicate the role for ble stack.
 *                               role = 0: ble peripheral
 *                               role = 1: ble central
 * @return  SUCCESS             Deinitialized successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_stack_deinit(UCHAR_T role)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_stack_deinit\n");
	ecr_ble_reset();
	return OPRT_OK;
}


/**
 * @brief   Function for getting the GATT Link-Support.
 * @param   p_link              return gatt link                 
 * @return  SUCCESS             Support Gatt Link
 *          ERROR               Only Beacon or Mesh Beacon, Not Support Gatt Link.
 * */
OPERATE_RET tkl_ble_stack_gatt_link(USHORT_T *p_link)
{
	return OPRT_OK;
}
/**
 * @brief   Register GAP Event Callback
 * @param   TKL_BLE_GAP_EVT_FUNC_CB Refer to @TKL_BLE_GAP_EVT_FUNC_CB
 * @return  SUCCESS         Register successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_gap_callback_register(CONST TKL_BLE_GAP_EVT_FUNC_CB gap_evt)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_callback_register\n");
	if(gap_evt != NULL)
		g_gap_evt = gap_evt;
	return OPRT_OK;
}

/**
 * @brief   Register GATT Event Callback
 * @param   TKL_BLE_GATT_EVT_FUNC_CB Refer to @TKL_BLE_GATT_EVT_FUNC_CB
 * @return  SUCCESS         Register successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_gatt_callback_register(CONST TKL_BLE_GATT_EVT_FUNC_CB gatt_evt)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gatt_callback_register\n");
	if(gatt_evt != NULL)
		g_gatt_evt = gatt_evt;
	return OPRT_OK;
}

/******************************************************************************************************************************/
/** @brief Define All GAP Interface
 */
/**
 * @brief   Set the local Bluetooth identity address.
 *          The local Bluetooth identity address is the address that identifies this device to other peers.
 *          The address type must be either @ref TKL_BLE_GAP_ADDR_TYPE_PUBLIC or @ref TKL_BLE_GAP_ADDR_TYPE_RANDOM.
 * @param   [in] p_peer_addr:   pointer to local address parameters 
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_addr_set(TKL_BLE_GAP_ADDR_T CONST *p_peer_addr)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_addr_set\n");

	ECR_BLE_GAP_ADDR_T device_addr;
	memcpy(device_addr.addr, p_peer_addr->addr, 6);
	device_addr.type = p_peer_addr->type;

	ecr_ble_set_device_addr(&device_addr);
    return OPRT_OK;
}
 
/**
 * @brief   Get the local Bluetooth identity address.
 * @param   [out] p_peer_addr:  pointer to local address
 * @return  SUCCESS             Set Address successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_gap_address_get(TKL_BLE_GAP_ADDR_T *p_peer_addr)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_address_get\n");

	ECR_BLE_GAP_ADDR_T device_addr;
	
	ecr_ble_get_device_addr(&device_addr);

	memcpy(p_peer_addr->addr, device_addr.addr, 6);
	p_peer_addr->type = device_addr.type;
	return OPRT_OK;
}

/**
 * @brief   Start advertising
 * @param   [in] p_adv_params : pointer to advertising parameters 
 * @return  SUCCESS
 *  ERROR
 * */ 
OPERATE_RET tkl_ble_gap_adv_start(TKL_BLE_GAP_ADV_PARAMS_T          CONST *p_adv_params)
{	
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_adv_start\n");
	ECR_BLE_GAP_ADV_PARAMS_T  adv_params = {0};
	BT_RET_T ret = BT_ERROR_NO_ERROR;
	
	if(p_adv_params->adv_interval_max > 32 && p_adv_params->adv_interval_max < 64)
	{
		 adv_params.adv_interval_max = p_adv_params->adv_interval_max;   /// Advertising maximum interval - 100ms (160*0.625ms)
	}
	else
	{
		 adv_params.adv_interval_max = 32;
	}

	if(p_adv_params->adv_interval_min > 32 && p_adv_params->adv_interval_min < 64)
	{
		 adv_params.adv_interval_min = p_adv_params->adv_interval_min;    /// Advertising minimum interval - 100ms (160*0.625ms)
	}
	else
	{
		 adv_params.adv_interval_min = 32;
	}

	if(p_adv_params->adv_channel_map != 37 || p_adv_params->adv_channel_map != 38 || \
		p_adv_params->adv_channel_map != 39 || p_adv_params->adv_channel_map != 0x07)
	{
		adv_params.adv_channel_map = 0x07;
	}

	ret = ecr_ble_gap_adv_start(&adv_params);
	if(ret == BT_ERROR_NO_ERROR)
	{
		return OPRT_OK;
	}else
	{
		return OPRT_COM_ERROR;
	}
}

/**
 * @brief   Stop advertising
 * @param   VOID
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_adv_stop(VOID)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_adv_stop\n");
	ecr_ble_gap_adv_stop();
	return OPRT_OK;
}

/**
 * @brief   Setting advertising data
 * @param   [in] p_adv:         Data to be used in advertisement packets, and include adv data len
 *          [in] p_scan_rsp:    Data to be used in advertisement respond packets, and include rsp data len
 * @Note    Please Check p_adv and p_scan_rsp, if data->p_data == NULL or data->length == 0, we will not update these values.
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_adv_rsp_data_set(TKL_BLE_DATA_T CONST *p_adv, TKL_BLE_DATA_T CONST *p_scan_rsp)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_adv_rsp_data_set\n");

	ECR_BLE_DATA_T adv_data;
	ECR_BLE_DATA_T scan_rsp_data;
	memcpy(&adv_data, p_adv, sizeof(TKL_BLE_DATA_T));
	memcpy(&scan_rsp_data, p_scan_rsp, sizeof(TKL_BLE_DATA_T));
	
	ecr_ble_adv_param_set(&adv_data, &scan_rsp_data);

	return OPRT_OK;
}

/**
 * @brief   Update advertising data
 * @param   [in] p_adv: Data    to be used in advertisement packets, and include adv data len
 *          [in] p_scan_rsp:    Data to be used in advertisement respond packets, and include rsp data len
 * @Note    Please Check p_adv and p_scan_rsp, if data->p_data == NULL or data->length == 0, we will not update these values.
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_adv_rsp_data_update(TKL_BLE_DATA_T CONST *p_adv, TKL_BLE_DATA_T CONST *p_scan_rsp)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_adv_rsp_data_update\n");

	ECR_BLE_DATA_T adv_data, scan_rsp_data;
	memcpy(&adv_data, p_adv, sizeof(ECR_BLE_DATA_T));
	memcpy(&scan_rsp_data, p_scan_rsp, sizeof(ECR_BLE_DATA_T));
	
	ecr_ble_adv_param_update(&adv_data, &scan_rsp_data);

	return OPRT_OK;
}

/**
 * @brief   Start scanning
 * @param   [in] scan_param:    scan parameters including interval, windows
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_scan_start(TKL_BLE_GAP_SCAN_PARAMS_T CONST *p_scan_params)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_scan_start\n");
	
	ECR_BLE_GAP_SCAN_PARAMS_T ecr_ble_gap_scan_params;
	ecr_ble_gap_scan_params.scan_type = ECR_SCAN_TYPE_GEN_DISC;
	ecr_ble_gap_scan_params.dup_filt_pol = ECR_DUP_FILT_EN;
	ecr_ble_gap_scan_params.scan_prop= ECR_BLE_SCAN_PROP_PHY_1M_BIT | ECR_BLE_SCAN_PROP_ACTIVE_1M_BIT | ECR_BLE_SCAN_PROP_FILT_TRUNC_BIT;
	
	ecr_ble_gap_scan_params.interval = p_scan_params->interval;
	ecr_ble_gap_scan_params.window = p_scan_params->window;
	ecr_ble_gap_scan_params.duration = p_scan_params->timeout;
	ecr_ble_gap_scan_params.period = 0;
	ecr_ble_gap_scan_params.scan_channel_map = p_scan_params->scan_channel_map;
	
	ecr_ble_gap_scan_start(&ecr_ble_gap_scan_params);
	return OPRT_OK;
}

/**
 * @brief   Stop scanning
 * @param   VOID
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_scan_stop(VOID)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gap_scan_stop\n");
	ecr_ble_gap_scan_stop();
	return OPRT_OK;
}

/**
 * @brief   Start connecting one peer
 * @param   [in] p_peer_addr:   include address and address type
 *          [in] p_scan_params: scan parameters
 *          [in] p_conn_params: connection  parameters
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_connect(TKL_BLE_GAP_ADDR_T CONST *p_peer_addr, TKL_BLE_GAP_SCAN_PARAMS_T CONST *p_scan_params, TKL_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params)
{
	return OPRT_OK;
}

/**
 * @brief   Disconnect from peer
 * @param   [in] conn_handle:   the connection handle
 *          [in] hci_reason:    terminate reason
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_disconnect(USHORT_T conn_handle, UCHAR_T hci_reason)
{
	return OPRT_OK;
}

/**
 * @brief   Start to update connection parameters
 * @param   [in] conn_handle:   connection handle
 *          [in] p_conn_params: connection  parameters
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_ble_gap_conn_param_update(USHORT_T conn_handle, TKL_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params)
{
	return OPRT_OK;
}

/**
 * @brief   Set the radio's transmit power.
 * @param   [in] role:          0: Advertising Tx Power; 1: Scan Tx Power; 2: Connection Power
 *          [in] tx_power:      tx power:This value will be magnified 10 times. 
 *                              If the tx_power value is -75, the real power is -7.5dB.(or 40 = 4dB)
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gap_tx_power_set(UCHAR_T role, INT_T tx_power)
{
	return OPRT_OK;
}

/**
 * @brief   Get the received signal strength for the last connection event.
 * @param   [in]conn_handle:    connection handle
 * @return  SUCCESS             Successfully read the RSSI.
 *          ERROR               No sample is available.
 * */
OPERATE_RET tkl_ble_gap_rssi_get(USHORT_T conn_handle)
{
    return OPRT_OS_ADAPTER_COM_ERROR;
}

/******************************************************************************************************************************/
/** @brief Define All Gatt Server Interface
 *
 *  Notes: notice the handle will be the one of signed point.
 */
/**
 * @brief   Add Ble Gatt Service
 * @param   [in] p_service: define the ble service
 *  For Example:/
 *  static TKL_BLE_GATTS_PARAMS_T       tkl_ble_gatt_service;
 *  static TKL_BLE_SERVICE_PARAMS_T     tkl_ble_common_service[TKL_BLE_GATT_SERVICE_MAX_NUM];
 *  static TKL_BLE_CHAR_PARAMS_T        tkl_ble_common_char[TKL_BLE_GATT_CHAR_MAX_NUM];
 *  
 *  static TAL_BLE_EVT_FUNC_CB          tkl_tal_ble_event_callback;
 *  
 *  static VOID tkl_ble_kernel_gap_event_callback(TKL_BLE_GAP_PARAMS_EVT_T *p_event)
 *  {
 *  }
 *  
 *  static VOID tkl_ble_kernel_gatt_event_callback(TKL_BLE_GATT_PARAMS_EVT_T *p_event)
 *  {
 *  }
 *  
    
 *  
     OPERATE_RET tal_ble_bt_init(TAL_BLE_ROLE_E role, CONST TAL_BLE_EVT_FUNC_CB ble_event)
     {
         UCHAR_T ble_stack_role = TKL_BLE_ROLE_SERVER;
     
         // Init Bluetooth Stack Role For Ble.
         if((role&TAL_BLE_ROLE_PERIPERAL) == TAL_BLE_ROLE_PERIPERAL || (role&TAL_BLE_ROLE_BEACON) == TAL_BLE_ROLE_BEACON) {
             ble_stack_role |= TKL_BLE_ROLE_SERVER;
         }
         if((role&TAL_BLE_ROLE_CENTRAL) == TAL_BLE_ROLE_CENTRAL) {
             ble_stack_role |= TKL_BLE_ROLE_CLIENT;
         }
     
         tkl_ble_stack_init(ble_stack_role);
         
         if(role == TAL_BLE_ROLE_PERIPERAL) {
             TKL_BLE_GATTS_PARAMS_T *p_ble_services = &tkl_ble_gatt_service;
             *p_ble_services = (TKL_BLE_GATTS_PARAMS_T) {
                 .svc_num    = TAL_COMMON_SERVICE_MAX_NUM,
                 .p_service  = tkl_ble_common_service,
             };
             
             // Add Service
             TKL_BLE_SERVICE_PARAMS_T *p_ble_common_service = tkl_ble_common_service;
             *(p_ble_common_service + TAL_COMMON_SERVICE_INDEX) = (TKL_BLE_SERVICE_PARAMS_T){
                 .handle     = TKL_BLE_GATT_INVALID_HANDLE,
                 .svc_uuid   = {
                     .uuid_type   = TKL_BLE_UUID_TYPE_16,
                     .uuid.uuid16 = TAL_BLE_CMD_SERVICE_UUID_V2,
                 },
                 .type       = TKL_BLE_UUID_SERVICE_PRIMARY,
                 .char_num   = TAL_COMMON_CHAR_MAX_NUM,
                 .p_char     = tkl_ble_common_char,
             };
             
             // Add Write Characteristic
             TKL_BLE_CHAR_PARAMS_T *p_ble_common_char = tkl_ble_common_char;
             
             *(p_ble_common_char + TAL_COMMON_WRITE_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T){
                 .handle = TKL_BLE_GATT_INVALID_HANDLE,
                 .char_uuid  = {
                     .uuid_type   = TKL_BLE_UUID_TYPE_16,
                     .uuid.uuid16 = TAL_BLE_CMD_WRITE_CHAR_UUID_V2,
                 },
                 .property   = TKL_BLE_GATT_CHAR_PROP_WRITE | TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP,
                 .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
                 .value_len  = 244,
             };
             
             // Add Notify Characteristic
             *(p_ble_common_char + TAL_COMMON_NOTIFY_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T){
                 .handle = TKL_BLE_GATT_INVALID_HANDLE,
                 .char_uuid  = {
                     .uuid_type   = TKL_BLE_UUID_TYPE_16,
                     .uuid.uuid16 = TAL_BLE_CMD_NOTIFY_CHAR_UUID_V2,
                 },
                 .property   = TKL_BLE_GATT_CHAR_PROP_NOTIFY,
                 .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
                 .value_len  = 244,
             };
     
             // Add Read Characteristic
             *(p_ble_common_char + TAL_COMMON_READ_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T){
                 .handle = TKL_BLE_GATT_INVALID_HANDLE,
                 .char_uuid  = {
                     .uuid_type   = TKL_BLE_UUID_TYPE_16,
                     .uuid.uuid16 = TAL_BLE_CMD_READ_CHAR_UUID_V2,
                 },
                 .property   = TKL_BLE_GATT_CHAR_PROP_READ,
                 .permission = TKL_BLE_GATT_PERM_READ,
                 .value_len  = 244,
             };
             
             if(tkl_ble_gatts_service_add(p_ble_services) != 0) {
                 return -1; // Invalid Paramters.
             }
         }
     
         // Get the TAL Event Callback.
         tal_ble_event_callback = ble_event;
     
         // Register GAP And GATT Callback
         tkl_ble_gap_callback_register(&tkl_ble_kernel_gap_event_callback);
         tkl_ble_gatt_callback_register(&tkl_ble_kernel_gatt_event_callback);
     
         return 0;
     }
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gatts_service_add(TKL_BLE_GATTS_PARAMS_T *p_service)
{	
	ECR_BLE_GATTS_PARAMS_T service;

	memcpy(&service, p_service, sizeof(ECR_BLE_GATTS_PARAMS_T));
	ecr_ble_gatts_service_add(&service, 0);
	
	return OPRT_OK;
}

/**
 * @brief   Set the value of a given attribute. After Config Tuya Read-Char, we can update read-value at any time.
 * @param   [in] conn_handle    Connection handle.
 *          [in] char_handle    Attribute handle.
 *          [in,out] p_value    Attribute value information.
 * @return  SUCCESS
 *          ERROR
 *
 * @note Values other than system attributes can be set at any time, regardless of whether any active connections exist. 
 * */ 
OPERATE_RET tkl_ble_gatts_value_set(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
	os_printf(LM_BLE,LL_INFO,"%s\n",__func__);
	ecr_ble_gatts_value_set(conn_handle, char_handle, p_data, length);
	return OPRT_OK;
}

/**
 * @brief   Get the value of a given attribute.
 * @param   [in] conn_handle    Connection handle. Ignored if the value does not belong to a system attribute.
 * @param   [in] char_handle    Attribute handle.
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gatts_value_get(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
	os_printf(LM_BLE,LL_INFO,"%s\n",__func__);
	ecr_ble_gatts_value_get(conn_handle, char_handle, p_data, length);
	return OPRT_OK;
}

/**
 * @brief   Notify an attribute value.
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 *          [in] p_data         Notify Values
 *          [in] length         Value Length
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gatts_value_notify(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{	
	os_printf(LM_BLE,LL_INFO," %s %d %d\n",__func__, char_handle,conn_handle);
	ecr_ble_gatts_value_notify(conn_handle, char_handle, p_data, length);
	return OPRT_OK;
}

/**
 * @brief   Indicate an attribute value.
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 *          [in] p_data         Notify Values
 *          [in] length         Value Length
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gatts_value_indicate(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
	os_printf(LM_BLE,LL_INFO,"tkl_ble_gatts_value_indicate\n");
	ecr_ble_gatts_value_indicate(conn_handle, char_handle, p_data, length);
	return OPRT_OK;
}

/**
 * @brief   Reply to an ATT_MTU exchange request by sending an Exchange MTU Response to the client.
 * @param   [in] conn_handle    Connection handle.
 *          [in] server_rx_mtu  mtu size.
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gatts_exchange_mtu_reply(USHORT_T conn_handle, USHORT_T server_rx_mtu)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gatts_exchange_mtu_reply\n");
	return OPRT_OK;
}

/******************************************************************************************************************************/
/** @brief Define All Gatt Client Interface, Refer to current ble gw and ble stack.
 *
 *  Notes: notice the handle will be the one of signed point.
 *  Discovery Operations belongs to GAP Interface, But declear here, because it will be used for the gatt client.
 */
 
/**
 * @brief   [Ble Central] Will Discovery All Service
 * @param   [in] conn_handle    Connection handle.
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gattc_all_service_discovery(USHORT_T conn_handle)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_all_service_discovery\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   [Ble Central] Will Discovery All Characteristic
 * @param   [in] conn_handle    Connection handle.
 *          [in] start_handle   Handle of start
 *          [in] end_handle     Handle of End
 * @return  SUCCESS
 *          ERROR
 * @Note:   For Tuya Service, it may contains more optional service, it is more better to find all Characteristic 
 *          instead of find specific uuid.
 * */  
OPERATE_RET tkl_ble_gattc_all_char_discovery(USHORT_T conn_handle, USHORT_T start_handle, USHORT_T end_handle)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_all_char_discovery\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   [Ble Central] Will Discovery All Descriptor of Characteristic
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] conn_handle    Connection handle.
 *          [in] start_handle   Handle of start
 *          [in] end_handle     Handle of End
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gattc_char_desc_discovery(USHORT_T conn_handle, USHORT_T start_handle, USHORT_T end_handle)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_char_desc_discovery\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   [Ble Central] Write Data without Response
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 *          [in] p_data         Write Values
 *          [in] length         Value Length
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_gattc_write_without_rsp(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_write_without_rsp\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   [Ble Central] Write Data with response
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 *          [in] p_data         Write Values
 *          [in] length         Value Length
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_ble_gattc_write(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_write\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   [Ble Central] Read Data
 * @param   [in] conn_handle    Connection handle.
 * @param   [in] char_handle    Attribute handle.
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_ble_gattc_read(USHORT_T conn_handle, USHORT_T char_handle)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_read\n");
	//master, to do
	return OPRT_OK;
}

/**
 * @brief   Start an ATT_MTU exchange by sending an Exchange MTU Request to the server.
 * @param   [in] conn_handle    Connection handle.
 *          [in] client_rx_mtu  mtu size.
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_ble_gattc_exchange_mtu_request(USHORT_T conn_handle, USHORT_T client_rx_mtu)
{
	os_printf(LM_APP, LL_DBG, "tkl_ble_gattc_exchange_mtu_request\n");
	//master, to do
	return OPRT_OK;
}
#endif

/* HCI packet types */
static TKL_HCI_FUNC_CB tuya_ble_hci_rx_cmd_hs_cb;
static TKL_HCI_FUNC_CB tuya_ble_hci_rx_acl_hs_cb;

#if 0
void TRACE_PACKET_DEBUG(uint8_t type, CONST UCHAR_T *buf, uint16_t len)
{
	os_printf(LM_BLE, LL_INFO, " ");

	for(uint16_t i = 0; i < len; i++)
	{
		os_printf(LM_BLE, LL_INFO, "%02x ", *(buf+i));
	}
	os_printf(LM_BLE, LL_INFO, "\r\n");
}
#else
#define TRACE_PACKET_DEBUG(t, buf, len)
#endif

/**
 * Sends an HCI event from the controller to the host.
 *
 * @param cmd                   The HCI event to send.  This buffer must be
 *                                  allocated via tuya_ble_hci_buf_alloc().
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_ll_evt_tx(uint8_t *hci_ev, uint16_t len)
{
	int rc = -1;
	uint16_t pkt_len = len;
	uint8_t *pkt = (uint8_t*) os_malloc(pkt_len);
	if(pkt == NULL)
	{
		return -1;
	}
	memcpy(pkt, hci_ev, pkt_len);
	// os_printf(LM_BLE, LL_INFO, "ctrl->host EVT");
	// TRACE_PACKET_DEBUG(BLE_HCI_EVT_MSG_TYPE, hci_ev, len);

	if(tuya_ble_hci_rx_cmd_hs_cb != NULL)
		rc = tuya_ble_hci_rx_cmd_hs_cb(pkt, (USHORT_T)pkt_len);

	if (pkt != NULL)
	{
		os_free(pkt);
		pkt = NULL;
	}
	return rc;
}

/**
 * Sends ACL data from controller to host.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_ll_acl_tx(uint8_t *acl_pkt,uint16_t len)
{
	int rc = -1;
	uint16_t pkt_len = len;
	uint8_t *pkt = (uint8_t*) os_malloc(pkt_len);
	if(pkt == NULL)
	{
		return -1;
	}
	memcpy(pkt, acl_pkt, pkt_len);
	// os_printf(LM_BLE, LL_INFO, "ctrl->host ACL");
	// TRACE_PACKET_DEBUG(BLE_HCI_ACL_MSG_TYPE, acl_pkt, len);

	if(tuya_ble_hci_rx_acl_hs_cb != NULL)
		rc = tuya_ble_hci_rx_acl_hs_cb(pkt, (USHORT_T)pkt_len);

	if (pkt != NULL)
	{
		os_free(pkt);
		pkt = NULL;
	}

	return rc;
}


void tuya_ble_hci_ll_tx(uint8_t type, uint8_t *pkt, uint16_t pkt_len)
{
	if(type == BLE_HCI_EVT_MSG_TYPE)
	{
		tuya_ble_hci_ll_evt_tx(pkt, pkt_len);

	}
	else if(type == BLE_HCI_ACL_MSG_TYPE)
	{
		tuya_ble_hci_ll_acl_tx(pkt, pkt_len);
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "%s [hci type error]\r\n", __func__);
		return;
	}
}

/**
 * Sends an HCI command from the host to the controller.
 *
 * @param cmd                   The HCI command to send.  This buffer must be
 *                                  allocated via tuya_ble_hci_buf_alloc().
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_cmd_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
	//os_printf(LM_BLE, LL_INFO, "host->ctrl CMD");
	TRACE_PACKET_DEBUG(BLE_HCI_CMD_MSG_TYPE, p_buf, buf_len);

	return hci_adapt_ll_rx(BLE_HCI_CMD_MSG_TYPE, (uint8_t *)p_buf, buf_len);
}

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_acl_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
	// os_printf(LM_BLE, LL_INFO, "host->ctrl ACL");
	// TRACE_PACKET_DEBUG(BLE_HCI_ACL_MSG_TYPE, p_buf, buf_len);

	return hci_adapt_ll_rx(BLE_HCI_ACL_MSG_TYPE,  (uint8_t *)p_buf, buf_len);
}

// /* Controller Use */
// void tuya_ble_hci_cfg_ll(tuya_ble_hci_rx_cmd_fn *cmd_cb, void *cmd_arg,
//                          tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg)
// {
//     os_printf(LM_APP, LL_INFO, "tuya_ble_hci_cfg_ll\n");
// }

// /* Host Use */
// void tuya_ble_hci_cfg_hs(tuya_ble_hci_rx_cmd_fn *cmd_cb, void *cmd_arg,
//                          tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg)
// {
//     os_printf(LM_APP, LL_INFO, "tuya_ble_hci_cfg_hs\n");

//     tuya_ble_hci_rx_cmd_hs_cb   = cmd_cb;
//     tuya_ble_hci_rx_cmd_hs_arg  = cmd_arg;
//     tuya_ble_hci_rx_acl_hs_cb   = acl_cb;
//     tuya_ble_hci_rx_acl_hs_arg  = acl_arg;
// }

OPERATE_RET tkl_hci_callback_register(CONST TKL_HCI_FUNC_CB hci_evt_cb, CONST TKL_HCI_FUNC_CB acl_pkt_cb)
{
    tuya_ble_hci_rx_cmd_hs_cb   = hci_evt_cb;
    tuya_ble_hci_rx_acl_hs_cb   = acl_pkt_cb;
	return 0;
}

/**
 * Resets the HCI module to a clean state.  Frees all buffers and reinitializes
 * the underlying transport.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_reset(void)
{
    //os_printf(LM_APP, LL_INFO, "tuya_ble_hci_reset\n");
	hci_adapt_reset();
	return 0;
}

/**
 * Init the HCI module
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_init(void)
{
    // Initialize the controller
    //os_printf(LM_APP, LL_INFO, "tuya_ble_hci_init\n");
	hci_adapt_register_ll_tx_cb(tuya_ble_hci_ll_tx);
	return 0;
}

/* Deinit the controller and hci interface*/
/* Host Use */
OPERATE_RET tkl_hci_deinit(void)
{
    // DeInitialize the controller
    //os_printf(LM_APP, LL_INFO, "tuya_ble_hci_deinit\n");
    return 0;
}

