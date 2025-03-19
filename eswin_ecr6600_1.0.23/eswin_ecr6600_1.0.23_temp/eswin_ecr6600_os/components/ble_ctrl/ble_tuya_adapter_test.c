#include <string.h>

#include "ble_tuya_adapter_test.h"

#include "app_api.h"
#include "defaults_profile.h"
#include "ble_thread.h"
#include "oshal.h"
#include "os_task_config.h"


/***********************************************************
*************************micro define***********************
***********************************************************/
#define BLE_CTRL_SEND_QUEUE_NUM (10)


/***********************************************************
*************************variable define********************
***********************************************************/
/**
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
};
*/
static ty_bt_param_t bt_init_p = {0};
static ty_bt_scan_info_t bt_scan_info_s;
static int ble_ctrl_task_handle = 0;
static uint8_t connect_id;
static os_sem_handle_t sync_scan_sem;
static int connect_status = 0;
static int adv_status = 0;

/***********************************************************
*************************function define********************
***********************************************************/
static char* mystrstr(const char* dest, int dest_len, const char* src)
{
	char* tdest = (char *)dest;
	char* tsrc = (char *)src;
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
				os_printf(LM_BLE, LL_INFO, "====by name rssi: %d ====\r\n", bt_scan_info_s.rssi);
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
				os_printf(LM_BLE, LL_INFO, "====by mac rssi: %d ====\r\n", bt_scan_info_s.rssi);
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

				tuya_ble_data_buf_t cb_buf;

				if(bt_init_p.cb != NULL)
				{
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
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_port_init\n");

    if((NULL == p)&&(NULL == p->cb)) {
        return -2;
    }

    if(ble_ctrl_task_handle != 0)
    {
        os_printf(LM_BLE, LL_ERR, "bt port already init, do not repetitive init!!!\n");
        return -1;
    }

    memcpy(&bt_init_p, p, sizeof(ty_bt_param_t));
	
	ble_set_device_name((uint8_t *)bt_init_p.name, strlen(bt_init_p.name));
	ble_main();

	ble_ctrl_task_handle=os_task_create( "ble_ctrl-task", BLE_CTRL_PRIORITY, BLE_CTRL_STACK_SIZE, (task_entry_t)ble_ctrl_task, NULL);
	ble_set_adv_data(&bt_init_p.adv);
	ble_set_scan_rsp_data(&bt_init_p.scan_rsp);
	tuya_os_adapt_bt_start_adv();
	if(bt_init_p.cb != NULL)
	{
		//inform ble ready, ty will call reset adv
		bt_init_p.cb(0,TY_BT_EVENT_ADV_READY,NULL);
	}
    return 0;
}

/**
 * @brief tuya_os_adapt_bt 蓝牙断开关闭
 * @return int 
 */
int tuya_os_adapt_bt_port_deinit(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_port_deinit\n");

    if(ble_ctrl_task_handle == 0)
    {
        os_printf(LM_BLE, LL_ERR, "bt port is not init, please do not deinit!!!\n");
        return -1;
    }

    os_task_delete(ble_ctrl_task_handle);
    ble_ctrl_task_handle = 0;
    os_queue_destory(task1_queue);

    ble_destory();

    return 0;
}

/**
 * @brief tuya_os_adapt_bt 蓝牙断开
 * @return int 
 */
int tuya_os_adapt_bt_gap_disconnect(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_gap_disconnect\n");
    if (!connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is not connected, can't disconnect!\n");
        return -1;
    }

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_DISCON_REQ;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    return 0;
}


/**
 * @brief tuya_os_adapt_bt 蓝牙发送
 * @return int 
 */
int tuya_os_adapt_bt_send(const unsigned char *data, const unsigned char len)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_send\n");
    if (!connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is not connected, can't send data!\n");
        return -1;
    }

    if (0 == len || NULL == data) {
        return -2;
    }

	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_NOTIFICATION;
	recv_msg.connect_id = connect_id;
	recv_msg.handle = NETCFG_IDX_NOTIFY_VAL;
	recv_msg.param = (unsigned char *)data;
	recv_msg.param_len = len;

	ble_app_send_msg(&recv_msg);

    return 0;
}

/**
 * @brief tuya_os_adapt_bt 广播包重置
 * @return int 
 */
int tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_reset_adv\n");
    if (connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is connected, can't reset adv!\n");
        return -1;
    }

	//reset adv data and re-start adv
	tuya_os_adapt_bt_stop_adv();
	ble_set_adv_data(adv);
	ble_set_scan_rsp_data(scan_resp);

	tuya_os_adapt_bt_start_adv();

    return 0;
}

/**
 * @brief tuya_os_adapt_bt 获取rssi信号值
 * @return int 
 */
int tuya_os_adapt_bt_get_rssi(signed char *rssi)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_get_rssi\n");

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
        return 0;
	}
	return -1;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int 
 */
int tuya_os_adapt_bt_start_adv(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_start_adv\n");
    if (connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is connected, can't start adv!\n");
        return -1;
    }
    if (adv_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: already advertising, can't start adv again!\n");
        return -1;
    }

	struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_START_ADV;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    adv_status = 1;
    return 0;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int 
 */
int tuya_os_adapt_bt_stop_adv(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_stop_adv\n");
    if (connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is connected, can't stop adv!\n");
        return -1;
    }

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_ADV;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);
    adv_status = 0;
    return 0;
}

/**
 * @brief tuya_os_adapt_bt 主动扫描
 * @return int 
 */
int tuya_os_adapt_bt_assign_scan(ty_bt_scan_info_t *info)
{
    //os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_assign_scan\n");
    if (connect_status)
    {
        os_printf(LM_BLE, LL_INFO, "err: status is connected, can't scan!\n");
        return -1;
    }

    if (NULL == info) {
        return -2;
    }
    if(ble_ctrl_task_handle == 0)
    {
        os_printf(LM_BLE, LL_ERR, "bt is not init! please init bt first!\n");
        return -1;
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
		return 0;
	}
	else
	{
		tuya_os_adapt_bt_stop_scan();
		return -1;
	}
}

/**
 * @brief tuya_os_adapt_bt 开始scan接收
 * @return int 
 */
int tuya_os_adapt_bt_start_scan(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_start_scan\n");

    //todo

    return 0;
}

/**
 * @brief tuya_os_adapt_bt 停止scan接收
 * @return int 
 */


int tuya_os_adapt_bt_stop_scan(void)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_stop_scan\n");

    struct s_recv_msg recv_msg;

	recv_msg.msg_id = MSG_ID_STOP_SCAN;
	recv_msg.task_id = ble_ctrl_task_handle;
	recv_msg.handle = 0;
	recv_msg.param = NULL;
	recv_msg.param_len = 0;

	ble_app_send_msg(&recv_msg);

    return 0;
}

/**
 * @brief tuya_os_adapt_bt scan初始化
 * @return int 
 */
int tuya_os_adapt_bt_scan_init(TY_BT_SCAN_ADV_CB scan_adv_cb)
{
    os_printf(LM_BLE, LL_INFO, "tuya_os_adapt_bt_scan_init\n");

    //todo
    return 0;
}
/**
 * @brief tuya_os_adapt_reg_bt_intf 接口注册
 * @return int
 */
//int tuya_os_adapt_reg_bt_intf(void)
//{
//    return tuya_os_adapt_reg_intf(INTF_BT, (void *)&m_tuya_os_bt_intfs);
//}

//@ble ctrl interface
