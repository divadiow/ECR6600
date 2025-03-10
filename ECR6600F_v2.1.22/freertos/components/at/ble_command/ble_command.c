/**
 ****************************************************************************************
 *
 * @file ble_command.c
 *
 * @brief BLE AT Command Source Code
 *
 * @par Copyright (C):
 *      ESWIN 2015-2020
 *
 ****************************************************************************************
 */

/****************************************************************************************
 * 
 * INCLUDE FILES
 *
 ****************************************************************************************/
#include "ble_command.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"
#include "task.h"
#include "at_def.h"
#include "at_common.h"
#include "at_customer_wrapper.h"
#include "system_config.h"
#include "bluetooth.h"
#include "arch_irq.h"
#include "ble_thread.h"

/****************************************************************************************
 * 
 * Local Macros 
 *
 ****************************************************************************************/
#define BLE_DEVICE_NAME_MAX_LEN (18)

/** connectable undirected adv */
#define  ADV_MODE                        "\x02\x01\x06"
#define  ADV_MODE_LEN                    (3)

#define  COMMON_CHAR_MAX_NUM            (2)
#define  EUS_TX_CHAR_UUID                 (0xffe2)
#define  EUS_RX_CHAR_UUID                 (0xffe1)
#define  EUS_CCCD_UUID                 (0x2902)
#define  EUS_SERVICE_UUID                  "\x12\xa2\x4d\x2e\xfe\x14\x48\x8e\x93\xd2\x17\x3c\xff\xe0\x00\x00"
#define  EUS_SERVICE_UUID_LEN             (16)
#define  DEFAULT_DEV_NAME              "Eswin"
#define  DEFAULT_ADV_INTERVAL          (160)  /* default 160*0.625=100ms */
#define  DEFAULT_SCAN_INTERVAL          (160)  /* default 160*0.625=100ms */
#define  DEFAULT_SCAN_WINDOW          (48)  /* default 48*0.625=30ms */

#define  UART_DEFAULT_RECV_TIMEOUT     (5) /* 5ms 4*(8+1+1)bit=40 40*1/9600=4.17ms~5ms */
#define  BLE_CONNECT_TIMEOUT           (10000) /* 10000ms */

#define  RX_BUFFER_LEN         (BLE_ATT_MTU_DEF-3)  //(gattc_get_mtu(conidx)-(ATT_CODE_LEN + ATT_HANDLE_LEN))
#define  UART_BUFFER_LEN       (2048+1)
#define  MAX_CON_HANDLER_NUM   (10)

#define  AUTO_CONN_TIME                 (0x02)
#define  AUTO_EST_CONN_TIME             (0x03)

/****************************************************************************************
 * 
 * TYPE DEFINITIONS
 *
 ****************************************************************************************/

typedef enum {
    BLE_ROLE_SLAVE,                         /* slave */
    BLE_ROLE_MASTER,                        /* master */
    BLE_ROLE_BEACON,                        /* beacon */
    BLE_ROLE_SM                             /* slave & master */
} ECR_BLE_ROLE_E;

typedef enum {
    BLE_ADV_MANUAL,                         /* manual advertising*/
    BLE_ADV_AUTO                            /* auto advertising */
} ECR_BLE_ADV_MOD_E;

typedef enum {
    BLE_ADV_OFF,                            /* turn off advertising */
    BLE_ADV_ON                              /* turn on advertising */
} ECR_BLE_ADV_SW_E;

typedef enum {
    BLE_NON_TRANSPARENT_MODE,               /*non transparent mode or command mode*/
    BLE_TRANSPARENT_MODE                    /*transparent mode*/
} ECR_BLE_TTMOD_E;


typedef enum {
    BLE_CMD_MSG_RECV_UART = 10,
    BLE_CMD_MSG_SRV_DISC,
    BLE_CMD_MSG_CHAR_DISC,
    BLE_CMD_MSG_DESC_DISC,
    BLE_CMD_MSG_CCCD_ENABLE,
    BLE_CMD_MSG_STOP_SCAN,
    BLE_CMD_MSG_EXH_MTU,
    BLE_CMD_MSG_DEV_RECON,
    BLE_CMD_MSG_DEV_REEST_CON,
} BLE_CMD_MSG_E;


typedef struct 
{
    BLE_CMD_MSG_E msg_type;
    int      conn_handle;
} BLE_CMD_MSG_ST;


struct ble_peer_info_t {
    char     peer_name[BLE_DEVICE_NAME_MAX_LEN];
    ECR_BLE_GAP_ADDR_T peer_addr;
    struct ble_peer_info_t *next;
    int8_t index;
};

typedef  struct ble_peer_info_t BLE_PEER_INFO_T;

typedef struct {
    BLE_PEER_INFO_T *head;
    uint8_t num;
} BLE_SCAN_DEV_INFO_T;

typedef struct {
    uint16_t  scan_during;
}BLE_SCAN_INFO_T;

typedef struct {
    uint16_t conn_handle;
    uint16_t tx_handle;
    uint16_t rx_handle;
    uint16_t gatt_mtu;
} BLE_CON_INFO_T;

struct ble_con_list_t{
    BLE_CON_INFO_T conn_info;
    ECR_BLE_GAP_ADDR_T peer_addr;
    uint16_t start_handle;        /*ens service start handle*/
    uint16_t end_handle;          /*ens service end handle*/
    uint16_t cccd_num;            /*ens client characteristic configuration num*/
    uint16_t cccd_handle[5];         /*ens client characteristic configuration*/
    struct ble_con_list_t *next;
};
typedef struct ble_con_list_t BLE_CON_LIST_T;

typedef struct {
    BLE_CON_LIST_T *head;
    uint8_t num;
} BLE_MASTER_CONS_T;

typedef struct {
    TimerHandle_t     uart_rx_timeout;
    os_queue_handle_t msg_queue;
    char      uart_buffer[UART_BUFFER_LEN];
    uint16_t  uart_buf_rd;
    uint16_t  uart_buf_wr;
    char    rx_buffer[RX_BUFFER_LEN];
    size_t  rx_buffer_pos;
    size_t  sent_pos;    
    bool    sending;
    BLE_SCAN_INFO_T   scan_info;
    TimerHandle_t     conn_timeout;
    bool     connecting;
    bool     connected;
    uint8_t reconn_time;
    uint8_t reest_conn_time;
	bool     active_dis;
	bool     start_recon;
    int      now_conn_idx;
    ECR_BLE_TTMOD_E   ttmod;
    ECR_BLE_ROLE_E    dev_role;
    ECR_BLE_ADV_MOD_E adv_mod;
    uint16_t          adv_interval;
    char dev_name[BLE_DEVICE_NAME_MAX_LEN];

    dce_t* pdce;    
}BLE_CMD_CXT_T;


/****************************************************************************************
 * 
 * Global Variables DEFINITIONS
 *
 ****************************************************************************************/
static ECR_BLE_GATTS_PARAMS_T		   eus_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T 	   eus_common_service[1] = {0};
static ECR_BLE_CHAR_PARAMS_T		   eus_common_char[2] = {0};


static BLE_SCAN_DEV_INFO_T scan_devs = {0};

static BLE_CON_INFO_T cmd_slave_con = {ECR_BLE_GATT_INVALID_HANDLE, 0,0,0};

static BLE_MASTER_CONS_T cmd_master_con = {0};

static BLE_CMD_CXT_T ble_cmd_cxt = {0};

static int uart_handle = -1;

/****************************************************************************************
 * 
 * LOCAL FUNCTION DEFINITIONS
 *
 ****************************************************************************************/
void _ble_handle_data_from_uart(void *param, const char *data, size_t size);
static void _ble_process_uart_data_msg(int handle);

static void _ble_set_adv_data(ECR_BLE_DATA_T *pdata)
{
	uint16_t index = 0;
	static uint8_t adv_data[31] = {0};
    
    //set adv mode --- flag
    memcpy(&adv_data[index], ADV_MODE, ADV_MODE_LEN);
    index += ADV_MODE_LEN;

    //set service uuid
    adv_data[index++] = EUS_SERVICE_UUID_LEN + 1;
    adv_data[index++] = 0x07; /* GAP_AD_TYPE_COMPLETE_LIST_128_BIT_UUID */
	memcpy(&adv_data[index], EUS_SERVICE_UUID, EUS_SERVICE_UUID_LEN);
	index += EUS_SERVICE_UUID_LEN;
#if 0
    //set device name
	adv_data[index++] = strlen(ble_cmd_cxt.dev_name)+1;
	adv_data[index++] = 0x09; /* GAP_AD_TYPE_COMPLETE_NAME */
	memcpy(&adv_data[index], ble_cmd_cxt.dev_name, strlen(ble_cmd_cxt.dev_name));
	index += strlen(ble_cmd_cxt.dev_name);
#endif
    pdata->length = index;
    pdata->p_data = adv_data;
}

static void _ble_set_scan_rsp_data(ECR_BLE_DATA_T *pdata)
{
	uint16_t index = 0;
	static uint8_t rsp_data[31] = {0};    
    
    //set device name
	rsp_data[index++] = strlen(ble_cmd_cxt.dev_name)+1;
	rsp_data[index++] = 0x09; /* Complete name */
	memcpy(&rsp_data[index], ble_cmd_cxt.dev_name, strlen(ble_cmd_cxt.dev_name));
	index += strlen(ble_cmd_cxt.dev_name);

    pdata->length = index;
    pdata->p_data = rsp_data;
}

static BT_RET_T _ble_adv_start(ECR_BLE_ADV_MOD_E adv_mode)
{
    ECR_BLE_DATA_T  padv_data = {0};
    ECR_BLE_DATA_T  prsp_data = {0};
    ECR_BLE_GAP_ADV_PARAMS_T adv_param = {0};

    _ble_set_adv_data(&padv_data);
    _ble_set_scan_rsp_data(&prsp_data);
	ecr_ble_adv_param_set(&padv_data, &prsp_data);

    adv_param.adv_type = ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
    adv_param.adv_channel_map = 0x07;
    if(adv_mode == BLE_ADV_AUTO)
    {
        adv_param.adv_interval_min = DEFAULT_ADV_INTERVAL;
        adv_param.adv_interval_max = DEFAULT_ADV_INTERVAL;
    }
    else
    {
        adv_param.adv_interval_min = ble_cmd_cxt.adv_interval;
        adv_param.adv_interval_max = ble_cmd_cxt.adv_interval;
    }

    return ecr_ble_gap_adv_start(&adv_param);	
}

static void _ble_switch_ttmod(ECR_BLE_TTMOD_E tsmod)
{
    if(ble_cmd_cxt.ttmod == tsmod)
        return;

    if(tsmod == BLE_TRANSPARENT_MODE)
    {
		ble_cmd_cxt.sending = false;
        memset(ble_cmd_cxt.rx_buffer, 0x00, sizeof(ble_cmd_cxt.rx_buffer));
        ble_cmd_cxt.rx_buffer_pos = 0;
        ble_cmd_cxt.sent_pos = 0;
        memset(ble_cmd_cxt.uart_buffer, 0x00, sizeof(ble_cmd_cxt.uart_buffer));
        ble_cmd_cxt.uart_buf_rd = ble_cmd_cxt.uart_buf_wr = 0;
        dce_register_data_input_cb(_ble_handle_data_from_uart);        
        target_dec_switch_input_state(ONLINE_DATA_STATE);
    }
    else
    {
        os_timer_stop(ble_cmd_cxt.uart_rx_timeout);
        dce_register_data_input_cb(NULL);        
        target_dec_switch_input_state(COMMAND_STATE);
    }

    ble_cmd_cxt.ttmod = tsmod;
   
}


static  BT_RET_T _ble_gap_connect(int idx)
{
	ECR_BLE_GAP_CONN_PARAMS_T ECR_BLE_GAP_CONN_PARAMS = {0};
    BLE_PEER_INFO_T *p = scan_devs.head;

	while(p != NULL)
	{
		if(p->index == idx)
		{
			break;
		}
		p = p->next;
	}

    if(p == NULL)
    {
        os_printf(LM_APP, LL_INFO, "exception\n");
        return BT_ERROR_UNDEFINED;
    }
    //conn_interval = power and data interaction
    ECR_BLE_GAP_CONN_PARAMS.conn_interval_min = BLE_CON_INTERVAL_MIN;
    ECR_BLE_GAP_CONN_PARAMS.conn_interval_max = BLE_CON_INTERVAL_MIN;
    ECR_BLE_GAP_CONN_PARAMS.conn_latency = BLE_CON_LATENCY_MIN;
    ECR_BLE_GAP_CONN_PARAMS.conn_sup_timeout = BLE_CON_SUP_TO_MIN*30;
	ECR_BLE_GAP_CONN_PARAMS.conn_establish_timeout = 0;
    return ecr_ble_gap_connect(&p->peer_addr, &ECR_BLE_GAP_CONN_PARAMS);
}

void _ble_handle_data_from_uart(void *param, const char *data, size_t size)
{
    int i;
    static int state = 0;
    static int link_stat = 0, link_id = 0;
	//&& ((ble_cmd_cxt.uart_buf_wr+1)%UART_BUFFER_LEN != ble_cmd_cxt.uart_buf_rd)
    if (NULL != ble_cmd_cxt.uart_rx_timeout) 
    {
		os_timer_start(ble_cmd_cxt.uart_rx_timeout);
    }
	
    for (i = 0; i < size; ++i)
    {
        char c = data[i];

		//queue_is_full?
		if((ble_cmd_cxt.uart_buf_wr+1)%UART_BUFFER_LEN != ble_cmd_cxt.uart_buf_rd)
		{
			//push queue
	        ble_cmd_cxt.uart_buffer[ble_cmd_cxt.uart_buf_wr++] = c;
			
	        ble_cmd_cxt.uart_buf_wr = (ble_cmd_cxt.uart_buf_wr)%UART_BUFFER_LEN;
		}
		
        if(CONFIG_LE_CONN > 1)
        {
            if(uart_handle < 0)
            {
                if(c == '<')
                {
                    if(link_stat == 0)
                        link_stat = 1;
                    else
                        link_stat = 0;
                }
                if((c) >= '0' && (c) <= '9')
                {
                    if(link_stat == 1)
                    {
                        link_stat = 2;
                        link_id = c - '0';
                    }
                    else
                    {
                        link_stat = 0;
                    }
                }
                if(c == '>')
                {
                    if(link_stat == 2)
                    {
                        int temp = 0;
                        uart_handle = link_id;
                        temp = ble_cmd_cxt.uart_buf_wr - 3;
                        ble_cmd_cxt.uart_buf_wr = (temp >= 0) ? temp : (temp+UART_BUFFER_LEN);
                        link_stat = 0;
                    }
                    else
                    {
                        link_stat = 0;
                    }
                }
            }
        }

        if(c == '+')
        {
            if(state == 0)
            {
                state = 1;
            }
            else if(state == 1)
            {
                state = 2;
            }
            else if(state == 2)
            {
                state = 0;
                _ble_switch_ttmod(BLE_NON_TRANSPARENT_MODE);
                return; 
            }
        }
        else
        {
            state = 0;
        }
    }
}

static void _ble_uart_rx_timeout(os_timer_handle_t xTimer)
{
    if(ble_cmd_cxt.sending == false && ble_cmd_cxt.uart_buf_wr != ble_cmd_cxt.uart_buf_rd)
    {
        ble_cmd_cxt.sending = true;
         _ble_process_uart_data_msg(uart_handle);
    }
}


static void _ble_connect_timeout(os_timer_handle_t xTimer)
{
    int ret = 0;
    ret = ecr_ble_gap_connect_cancel();
	os_printf(LM_CMD, LL_INFO, "ret = %d\n", ret);

	BLE_PEER_INFO_T *p = scan_devs.head;
	BLE_CON_LIST_T *q = cmd_master_con.head;

	while(p != NULL)
	{
		if(p->index == ble_cmd_cxt.now_conn_idx)
		{
			break;
		}
		p = p->next;
	}

	while(q != NULL)
	{
		if(0 == memcmp(p->peer_addr.addr, q->peer_addr.addr, GAP_BD_ADDR_LEN))
		{
			break;
		}
		q = q->next;
	}
	
	if( q == NULL)
	{
		if (ble_cmd_cxt.start_recon)
		{
			BLE_CMD_MSG_ST msg_send = {0};
            msg_send.msg_type = BLE_CMD_MSG_DEV_RECON;
            msg_send.conn_handle = 0;
            os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
		}else
		{
			char resp_buf[50];
			sprintf(resp_buf,"+CONNECTION TIME OUT");
			dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
			ble_cmd_cxt.connecting = false;
		}
	}
}


static void _master_con_add_list(BLE_CON_LIST_T * conn)
{
    if(cmd_master_con.head == NULL)
    {
        cmd_master_con.head = conn;        
    }
    else
    {
        BLE_CON_LIST_T *p = cmd_master_con.head;
        
        while(p->next != NULL)
            p = p->next;
        p->next = conn;            
    }
    cmd_master_con.num++;
}


static void _master_con_del_list(uint16_t conn_handle)
{
    BLE_CON_LIST_T *p = NULL, *prev = NULL;
    bool find = false;

    if(cmd_master_con.head->conn_info.conn_handle == conn_handle)
    {
        p = cmd_master_con.head;
        cmd_master_con.head = NULL;
        os_free(p);
        cmd_master_con.num--;
        find = true;
    }
    else
    {
        prev = cmd_master_con.head;
        p = prev->next;
        while(p != NULL)
        {
            if(p->conn_info.conn_handle == conn_handle)
            {
                prev->next = p->next;
                os_free(p);
                cmd_master_con.num--;
                find = true;
                break;
            }
			p = p->next;
			prev = prev->next;
        }
    }
    
    if(find)
        os_printf(LM_CMD, LL_INFO, "_master_con_del_list ok num=%d\n", cmd_master_con.num);

}


static BLE_CON_LIST_T * _master_con_get_by_conn(uint16_t conn_handle)
{
    BLE_CON_LIST_T *p = cmd_master_con.head;    

    while(p != NULL)
    {
        if(p->conn_info.conn_handle == conn_handle)
        {
            break;
        }
        p = p->next;
    }

    return p;    
}



static void _ble_master_connect_cmp(int32_t conn_hdl, ECR_BLE_GAP_CONNECT_EVT_T *con_evt)
{
    BLE_CON_LIST_T *con = NULL;

    con = (BLE_CON_LIST_T *)os_malloc(sizeof(BLE_CON_LIST_T));
    if(con == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "malloc fail\n");
        return;
    }
    memset(con, 0x00, sizeof(BLE_CON_LIST_T));
    memcpy(&con->peer_addr, &con_evt->peer_addr, sizeof(ECR_BLE_GAP_ADDR_T));
    con->conn_info.conn_handle = conn_hdl;
    con->conn_info.rx_handle = ECR_BLE_GATT_INVALID_HANDLE;
    con->conn_info.tx_handle = ECR_BLE_GATT_INVALID_HANDLE;
    
    _master_con_add_list(con);

    //start service discovery
    {        
        BLE_CMD_MSG_ST msg_send = {0};
        msg_send.msg_type = BLE_CMD_MSG_SRV_DISC;
        msg_send.conn_handle = conn_hdl;
        os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
    }
}

static void _ble_proc_connect_evt(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
    char resp_buf[50];
	ble_cmd_cxt.active_dis = false;
    ECR_BLE_GAP_ADDR_T  *peer_addr = &p_event->gap_event.connect.peer_addr;

    if(p_event->result != BT_ERROR_NO_ERROR)
    {
        if(ble_cmd_cxt.connecting)
        {        
            os_timer_stop(ble_cmd_cxt.conn_timeout);
            ble_cmd_cxt.connecting = false;
            dce_emit_basic_result_code(ble_cmd_cxt.pdce, DCE_RC_ERROR);
        }
        return;
    }

    if(p_event->gap_event.connect.role == ECR_BLE_ROLE_SLAVE)
    {
        cmd_slave_con.conn_handle = p_event->conn_handle;
        cmd_slave_con.tx_handle = eus_common_char[0].handle;
        cmd_slave_con.rx_handle = eus_common_char[1].handle;

        sprintf(resp_buf,"+CONNECTED[%d],%02X:%02X:%02X:%02X:%02X:%02X",p_event->conn_handle,peer_addr->addr[5],peer_addr->addr[4],peer_addr->addr[3],
            peer_addr->addr[2],peer_addr->addr[1],peer_addr->addr[0]);
        ble_cmd_cxt.connected = true;
        
        dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);

        _ble_switch_ttmod(BLE_TRANSPARENT_MODE);
    }
    else
    {
        _ble_master_connect_cmp(p_event->conn_handle, &p_event->gap_event.connect);
    }

}

static void _ble_proc_disconnect_evt(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
    char resp_buf[50];

    if(p_event->gap_event.disconnect.role == ECR_BLE_ROLE_SLAVE)
    {
        if(cmd_slave_con.conn_handle != p_event->conn_handle)
        {
            os_printf(LM_CMD, LL_ERR, "unknown slave conn disconnect\n");
            return;
        }
        cmd_slave_con.conn_handle = ECR_BLE_GATT_INVALID_HANDLE;
        ble_cmd_cxt.connected = false;
        sprintf(resp_buf,"+DISCONNECT[%d]",p_event->conn_handle);
        dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
    }
    else
    {
        BLE_CON_LIST_T *con_info = _master_con_get_by_conn(p_event->conn_handle);
        if(con_info == NULL)
        {
            os_printf(LM_CMD, LL_ERR, "unknown master conn disconnect\n");
            return;
        }

        if((con_info->conn_info.rx_handle != ECR_BLE_GATT_INVALID_HANDLE)
        && (con_info->conn_info.tx_handle != ECR_BLE_GATT_INVALID_HANDLE)
        && (!ble_cmd_cxt.start_recon)
        && ((ble_cmd_cxt.active_dis)
        ||(p_event->gap_event.disconnect.reason == BT_ERROR_CON_TERM_BY_LOCAL_HOST)))
        {
	        #ifdef CONFIG_AT_UART_0
	        arch_irq_unmask(VECTOR_NUM_UART0);
	        #endif
	        #ifdef CONFIG_AT_UART_1
	        arch_irq_unmask(VECTOR_NUM_UART1);
	        #endif
	        #ifdef CONFIG_AT_UART_2
	        arch_irq_unmask(VECTOR_NUM_UART2);
	        #endif
			ble_cmd_cxt.connected = false;
            sprintf(resp_buf,"+DISCONNECT[%d]",p_event->conn_handle);
            dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
        }
        else
        {
			// abnormal disconnect 
            os_timer_stop(ble_cmd_cxt.conn_timeout);
		    if((!ble_cmd_cxt.active_dis) && (ble_cmd_cxt.connected))
			{
			    BLE_CMD_MSG_ST msg_send = {0};
	            msg_send.msg_type = BLE_CMD_MSG_DEV_RECON;
	            msg_send.conn_handle = p_event->conn_handle;
	            os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
			}
			// disconnect reason = 0x3E
			else if(ble_cmd_cxt.connecting && ble_cmd_cxt.reest_conn_time < AUTO_EST_CONN_TIME)
			{
			    BLE_CMD_MSG_ST msg_send = {0};
	            msg_send.msg_type = BLE_CMD_MSG_DEV_REEST_CON;
	          //  msg_send.conn_handle = p_event->conn_handle;
	            os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
			}
			else
			{
				ble_cmd_cxt.reest_conn_time = 0;
				ble_cmd_cxt.connected = false;
				ble_cmd_cxt.connecting = false;
                sprintf(resp_buf,"+DISCONNECT[%d]",p_event->conn_handle);
                dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
			}
        }
        if(con_info)_master_con_del_list(p_event->conn_handle);
    }

    if(cmd_slave_con.conn_handle == ECR_BLE_GATT_INVALID_HANDLE
        && cmd_master_con.num == 0)
    {
        _ble_switch_ttmod(BLE_NON_TRANSPARENT_MODE);
    }

    if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE || ble_cmd_cxt.dev_role == BLE_ROLE_SM)
    {
        if(ble_cmd_cxt.adv_mod == BLE_ADV_AUTO)
        {
            _ble_adv_start(BLE_ADV_AUTO);
        }
    }
}

static bool _ble_conn_handle_valid(uint16_t conn_handle)
{
    bool valid = false;

    if(cmd_slave_con.conn_handle == conn_handle)
    {
        valid = true;
    }
    else
    {
        BLE_CON_LIST_T * con_info = _master_con_get_by_conn(conn_handle);

        if(con_info != NULL)
        {
            valid = true;
        }
    }
    return valid;
}


/*
adv data format
len(1 octet)+ad_type(1 octet)+ad_data(len-1)
*/
static void _get_dev_name_from_adv_report(ECR_BLE_DATA_T *adv_data, char *dev_name, uint8_t name_len)
{
    uint8_t *data = adv_data->p_data;
    int i = 0;
    uint8_t ad_len = 0;
    uint8_t ad_type = 0;
    uint8_t len = 0;

    memset(dev_name, 0x00, name_len);

    do{
        ad_len = *(data+i);
        i++;
		if(i >= adv_data->length) return;
        ad_type = *(data+i);
        i++;
		if(i+ad_len-1 > adv_data->length) return;
        //GAP_AD_TYPE_SHORTENED_NAME ||GAP_AD_TYPE_COMPLETE_NAME
        if(ad_type == 0x08 || ad_type == 0x09)
        {
            len = (ad_len-1 > name_len) ? (name_len) : (ad_len-1);
            memcpy(dev_name, data+i, len);
			if(BLE_DEVICE_NAME_MAX_LEN == len)
				dev_name[BLE_DEVICE_NAME_MAX_LEN-1]='\0';
            return;
        }
        i += (ad_len-1);
    }while(i < adv_data->length);
}

static void _scan_dev_add_list(BLE_PEER_INFO_T * peer_dev)
{
    if(scan_devs.head == NULL)
    {
        scan_devs.head = peer_dev;        
    }
    else
    {
        BLE_PEER_INFO_T *p = scan_devs.head;
        
        while(p->next != NULL)
            p = p->next;
        p->next = peer_dev;            
    }
}

static bool _scan_dev_has_existed(ECR_BLE_GAP_ADV_REPORT_T *adv_report)
{
    char resp_buf[60]={0};  
    uint8_t *mac_adr = adv_report->peer_addr.addr;
    BLE_PEER_INFO_T *p = scan_devs.head;    
    bool exist = false;

    while(p != NULL)
    {
        if(0 == memcmp(p->peer_addr.addr, mac_adr, GAP_BD_ADDR_LEN))
        {
            if(strlen(p->peer_name) == 0)
            {
                _get_dev_name_from_adv_report(&adv_report->data, p->peer_name, BLE_DEVICE_NAME_MAX_LEN);
                if(strlen(p->peer_name) > 0)
                {                
					scan_devs.num++;
			        if(scan_devs.num == 10)
				    {
				        BLE_CMD_MSG_ST msg = {0};
				        msg.msg_type = BLE_CMD_MSG_STOP_SCAN;
				        os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg, sizeof(BLE_CMD_MSG_ST), 0);
				    }
					p->index = scan_devs.num-1;
					sprintf(resp_buf,"+BLESCAN:%d,\"%s\",%02X:%02X:%02X:%02X:%02X:%02X",p->index, p->peer_name, 
								p->peer_addr.addr[5], p->peer_addr.addr[4], p->peer_addr.addr[3],
								p->peer_addr.addr[2], p->peer_addr.addr[1], p->peer_addr.addr[0]);
                    dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
                }
            }
            exist = true;
        }
        p = p->next;
    }
    
    return exist;    
}


static bool _scan_dev_has_ens_service(ECR_BLE_GAP_ADV_REPORT_T *adv_report)
{
    uint8_t *data = adv_report->data.p_data;
    uint8_t uuid16[EUS_SERVICE_UUID_LEN] = {0};
    int i = 0;
    uint8_t ad_len = 0,ad_type = 0, len = 0;
    bool find = false;

    do{
        ad_len = *(data+i);
        i++;
		if(i >= adv_report->data.length) break;
        ad_type = *(data+i);
        i++;
		if(i+ad_len-1 > adv_report->data.length) break;
        //GAP_AD_TYPE_COMPLETE_LIST_128_BIT_UUID
        if(ad_type == 0x07)
        {
            len = (ad_len-1 > EUS_SERVICE_UUID_LEN) ? EUS_SERVICE_UUID_LEN : (ad_len-1);
            memcpy(uuid16, data+i, len);
            find = true;
            break;
        }
        i += (ad_len-1);
    }while(i < adv_report->data.length);

    if(find && 0 == memcmp(uuid16, EUS_SERVICE_UUID, EUS_SERVICE_UUID_LEN))
    {
        return true;
    }
    
    return false;   
}


static void _scan_dev_clear_list(void)
{
    BLE_PEER_INFO_T *p = scan_devs.head;
    BLE_PEER_INFO_T *del = NULL;

    while(p != NULL){
        del = p;
        p = p->next;        
        os_free(del);
    }
    scan_devs.head = NULL;
    scan_devs.num = 0;
}

static void _ble_proc_adv_report_evt(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
    char resp_buf[60]={0};
    BLE_PEER_INFO_T * peer_dev = NULL;

    if(p_event->result != BT_ERROR_NO_ERROR)
    {
        return;
    }

	if((p_event->gap_event.adv_report.data.p_data == NULL)
		|| (p_event->gap_event.adv_report.data.length == 0))
	{
		//empty packet
		return;
	}

    if(_scan_dev_has_existed(&p_event->gap_event.adv_report))
    {   
        return;
    }
    if(!_scan_dev_has_ens_service(&p_event->gap_event.adv_report))
    {
        return;
    }
    peer_dev = (BLE_PEER_INFO_T *)os_malloc(sizeof(BLE_PEER_INFO_T));
    if(peer_dev == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "malloc fail\n");
        return;
    }
    memset(peer_dev, 0x00, sizeof(BLE_PEER_INFO_T));
	peer_dev->index = -1;
    memcpy(&peer_dev->peer_addr, &p_event->gap_event.adv_report.peer_addr, sizeof(ECR_BLE_GAP_ADDR_T));
    _get_dev_name_from_adv_report(&p_event->gap_event.adv_report.data, peer_dev->peer_name, BLE_DEVICE_NAME_MAX_LEN);
    _scan_dev_add_list(peer_dev);
    if(strlen(peer_dev->peer_name) > 0)
    {
    	scan_devs.num++;
        if(scan_devs.num == 10)
	    {
	        BLE_CMD_MSG_ST msg = {0};
	        msg.msg_type = BLE_CMD_MSG_STOP_SCAN;
	        os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg, sizeof(BLE_CMD_MSG_ST), 0);
	    }
		peer_dev->index = scan_devs.num-1;
		sprintf(resp_buf,"+BLESCAN:%d,\"%s\",%02X:%02X:%02X:%02X:%02X:%02X",peer_dev->index, peer_dev->peer_name, 
								peer_dev->peer_addr.addr[5], peer_dev->peer_addr.addr[4], peer_dev->peer_addr.addr[3],
								peer_dev->peer_addr.addr[2], peer_dev->peer_addr.addr[1], peer_dev->peer_addr.addr[0]);
        dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
    }
}



static void _ble_proc_svc_discovery_evt(uint16_t conn_handle, ECR_BLE_GATT_SVC_DISC_TYPE_T *svr_disc)
{
    BLE_CON_LIST_T * con_info = NULL;
    if(svr_disc->uuid.uuid_type == ECR_BLE_UUID_TYPE_128
        && 0 == memcmp(svr_disc->uuid.uuid.uuid128, EUS_SERVICE_UUID, EUS_SERVICE_UUID_LEN))
    {
        
        os_printf(LM_CMD, LL_INFO, "discovery eus service\n");
        con_info = _master_con_get_by_conn(conn_handle);
        con_info->start_handle = svr_disc->start_handle;
        con_info->end_handle = svr_disc->end_handle;
    }
}




static void _ble_proc_char_discovery_evt(uint16_t conn_handle, ECR_BLE_GATT_CHAR_DISC_TYPE_T *char_disc)
{
    BLE_CON_LIST_T * con_info = NULL;

    con_info = _master_con_get_by_conn(conn_handle);
    if(con_info == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "con_info is NULL\n");
        return;
    }        

    if(char_disc->uuid.uuid_type == ECR_BLE_UUID_TYPE_16)
    {
        if(char_disc->uuid.uuid.uuid16 == EUS_TX_CHAR_UUID)
        {       
            con_info->conn_info.tx_handle = char_disc->handle;
        }
        else if(char_disc->uuid.uuid.uuid16 == EUS_RX_CHAR_UUID)
        {
            con_info->conn_info.rx_handle = char_disc->handle;
        }
        
    }
}

static void _ble_proc_disc_discovery_evt(uint16_t conn_handle, ECR_BLE_GATT_DESC_DISC_TYPE_T *desc_disc)
{
    BLE_CON_LIST_T * con_info = NULL;
    con_info = _master_con_get_by_conn(conn_handle);
    if(con_info == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "con_info is NULL\n");
        return;
    }        

    if(desc_disc->uuid_len == 2)
    {
        if(desc_disc->uuid16 == EUS_CCCD_UUID)
        {  
            con_info->cccd_handle[con_info->cccd_num++] = desc_disc->cccd_handle;
        }
    }
}

static void _ble_eus_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_RESET:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_reset\n");
			break;

		case ECR_BLE_GAP_EVT_CONNECT:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_connect\n");
            _ble_proc_connect_evt(p_event);
			break;

        case ECR_BLE_GAP_EVT_DISCONNECT:
            os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_disconnect\n");
            _ble_proc_disconnect_evt(p_event);
            break;

		case ECR_BLE_GAP_EVT_ADV_REPORT:
            _ble_proc_adv_report_evt(p_event);
			break;
		default:
			break;

	}

}


static void _ble_eus_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
            if(p_event->conn_handle == cmd_slave_con.conn_handle && ble_cmd_cxt.ttmod == BLE_TRANSPARENT_MODE)
            {
                if(p_event->gatt_event.write_report.char_handle == cmd_slave_con.rx_handle)
                {     
                    dce_emit_pure_response(ble_cmd_cxt.pdce, (char*)p_event->gatt_event.write_report.report.p_data, 
                    p_event->gatt_event.write_report.report.length);
                }
            }
			break;
		case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_TX:
        case ECR_BLE_GATT_EVT_WRITE_CMP:    
            {
                BLE_CON_LIST_T * con_info = NULL;
                con_info = _master_con_get_by_conn(p_event->conn_handle);
				if(ble_cmd_cxt.ttmod == BLE_NON_TRANSPARENT_MODE )
				{
					if(con_info->cccd_num>0)
					{
                      //continue enable cccd
                       BLE_CMD_MSG_ST msg_send = {0};
                       msg_send.msg_type = BLE_CMD_MSG_CCCD_ENABLE;
                       msg_send.conn_handle = p_event->conn_handle;
                       os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
					}
                    else
                    {
  					  BLE_CMD_MSG_ST msg_send = {0};
                      msg_send.msg_type = BLE_CMD_MSG_EXH_MTU;
                      msg_send.conn_handle = p_event->conn_handle;
                      os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
					}
				}
				else 
				{
				  	if(ble_cmd_cxt.sent_pos == ble_cmd_cxt.rx_buffer_pos)
					{
						//to do
						memset(ble_cmd_cxt.rx_buffer, 0x00, RX_BUFFER_LEN);
						ble_cmd_cxt.sending = false;
						if(ble_cmd_cxt.uart_buf_wr == ble_cmd_cxt.uart_buf_rd)
						{
							os_printf(LM_CMD, LL_INFO, "send finish\n");
							os_timer_stop(ble_cmd_cxt.uart_rx_timeout);
						}
						else
						{
							os_timer_start(ble_cmd_cxt.uart_rx_timeout);
						}
					}
				}
            }
			break;
        case ECR_BLE_GATT_EVT_PRIM_SEV_DISCOVERY:
            {
                if(p_event->result == BT_ERROR_NO_ERROR)
                {
                    _ble_proc_svc_discovery_evt(p_event->conn_handle, &p_event->gatt_event.svc_disc);
                }
                else if(p_event->result == BT_ERROR_DISC_DONE)
                { 
                    //start service char discovery
                    BLE_CMD_MSG_ST msg_send = {0};
                    msg_send.msg_type = BLE_CMD_MSG_CHAR_DISC;
                    msg_send.conn_handle = p_event->conn_handle;
                    os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
                }                        
                else
                {
                    os_printf(LM_CMD, LL_INFO, "service discovery unkown error\n");
                }
                
            }
			break;
        case ECR_BLE_GATT_EVT_CHAR_DISCOVERY:
            {
                if(p_event->result == BT_ERROR_NO_ERROR)
                {
                    _ble_proc_char_discovery_evt(p_event->conn_handle, &p_event->gatt_event.char_disc);
                }
                //Exanchge Client MTU
                else if(p_event->result == BT_ERROR_DISC_DONE)
                { 
                    //start discovery char disc
                    BLE_CMD_MSG_ST msg_send = {0};
                    msg_send.msg_type = BLE_CMD_MSG_DESC_DISC;
                    msg_send.conn_handle = p_event->conn_handle;
                    os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
                }
                else
                {
                    os_printf(LM_CMD, LL_INFO, "char discovery unkown error\n");
                }                
            }
            break;
		case ECR_BLE_GATT_EVT_CHAR_DESC_DISCOVERY:
			{
                BLE_CON_LIST_T * con_info = NULL;
                if(p_event->result == BT_ERROR_NO_ERROR)
                {
                    _ble_proc_disc_discovery_evt(p_event->conn_handle, &p_event->gatt_event.desc_disc);
                }
                else if(p_event->result == BT_ERROR_DISC_DONE)
                {
                    con_info = _master_con_get_by_conn(p_event->conn_handle);
                    if(con_info == NULL)
                    {
                        os_printf(LM_CMD, LL_ERR, "con_info is NULL\n");
                        return;
                    }
                    else if(con_info->cccd_num > 0)
                    {
                        os_timer_stop(ble_cmd_cxt.conn_timeout);
                        //start enable cccd
                        BLE_CMD_MSG_ST msg_send = {0};
                        msg_send.msg_type = BLE_CMD_MSG_CCCD_ENABLE;
                        msg_send.conn_handle = p_event->conn_handle;
                        os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
                    }
                    else
                    {
                        BLE_CMD_MSG_ST msg_send = {0};
                        msg_send.msg_type = BLE_CMD_MSG_EXH_MTU;
                        msg_send.conn_handle = p_event->conn_handle;
                        os_queue_send(ble_cmd_cxt.msg_queue, (char*)&msg_send, sizeof(BLE_CMD_MSG_ST), 0);
                    }
				}
				else
				{
					os_printf(LM_CMD, LL_INFO, "desc discovery unkown error\n");
				}
			}
			break;
        case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_RX:
            if(ble_cmd_cxt.ttmod == BLE_TRANSPARENT_MODE)
            {
                BLE_CON_LIST_T * con_info = _master_con_get_by_conn(p_event->conn_handle);
                
                if(p_event->gatt_event.data_report.char_handle == con_info->conn_info.tx_handle)
                {      
                    os_printf(LM_BLE, LL_INFO, "master received slave data=%s\n",p_event->gatt_event.data_report.report.p_data);                
                    dce_emit_pure_response(ble_cmd_cxt.pdce, (char*)p_event->gatt_event.data_report.report.p_data, 
                    p_event->gatt_event.data_report.report.length);
                }
            } 
            break;
		case ECR_BLE_GATT_EVT_MTU_RSP:
			{
                BLE_CON_LIST_T * con_info = NULL;
                con_info = _master_con_get_by_conn(p_event->conn_handle);
                if(con_info != NULL)
        		 {
        		      if((ble_cmd_cxt.connected == false)||(ble_cmd_cxt.start_recon))
                      {
                           if(!ble_cmd_cxt.start_recon)
                           {
                              char resp_buf[50];
                              sprintf(resp_buf,"+CONNECTED[%d],%02X:%02X:%02X:%02X:%02X:%02X",p_event->conn_handle,con_info->peer_addr.addr[5],con_info->peer_addr.addr[4],con_info->peer_addr.addr[3],
                                  con_info->peer_addr.addr[2], con_info->peer_addr.addr[1], con_info->peer_addr.addr[0]);                
                              dce_emit_basic_result_code(ble_cmd_cxt.pdce,DCE_RC_OK);
                			  dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
                           }
                      }      
                      ble_cmd_cxt.connecting = false;
                      ble_cmd_cxt.connected = true;
                      ble_cmd_cxt.reest_conn_time = 0;
                	  ble_cmd_cxt.reconn_time = 0;
                	  ble_cmd_cxt.start_recon=false;
                      os_timer_changeperiod(ble_cmd_cxt.conn_timeout, BLE_CONNECT_TIMEOUT);
                	  os_timer_stop(ble_cmd_cxt.conn_timeout);
            		 _ble_switch_ttmod(BLE_TRANSPARENT_MODE);
        		 }
			}
			break;
		default:
            break;
    }
}


static void _ble_eus_service_init(void)
{
	ECR_BLE_GATTS_PARAMS_T *p_ble_service = &eus_gatt_service;
	p_ble_service->svc_num =  1;
	p_ble_service->p_service = eus_common_service;

	ECR_BLE_SERVICE_PARAMS_T *p_common_service = eus_common_service;
	p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_128;
    memcpy(p_common_service->svc_uuid.uuid.uuid128, EUS_SERVICE_UUID, 16*sizeof(uint8_t));
	p_common_service->type	   = ECR_BLE_UUID_SERVICE_PRIMARY;
	p_common_service->char_num = COMMON_CHAR_MAX_NUM;
	p_common_service->p_char   =eus_common_char;

	/*Add TX characteristic*/
    ECR_BLE_CHAR_PARAMS_T *p_common_char = eus_common_char;
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = EUS_TX_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = BLE_ATT_MTU_DEF;
	p_common_char++;

    /*Add RX characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = EUS_RX_CHAR_UUID;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = BLE_ATT_MTU_DEF;
	p_common_char++;		
	
	ecr_ble_gatts_service_add(p_ble_service, false);
}



static int _get_cur_conn_handle(void)
{
    if(cmd_slave_con.conn_handle != ECR_BLE_GATT_INVALID_HANDLE)
    {
        return cmd_slave_con.conn_handle;
    }
    else if(cmd_master_con.head->conn_info.conn_handle != ECR_BLE_GATT_INVALID_HANDLE)
    {
        return cmd_master_con.head->conn_info.conn_handle;
    }
    else
    {
        os_printf(LM_APP, LL_ERR, "unkown handle\n");
        return -1;
    }
}



static bool _ble_read_data_from_buf(int *handle)
{
    uint16_t wrrd_len = 0,len = 0;
    bool ret = false;
	uint16_t wr = ble_cmd_cxt.uart_buf_wr;
	uint16_t rd = ble_cmd_cxt.uart_buf_rd;
	uint16_t mtu = 0;

    if(CONFIG_LE_CONN > 1)
    {
        if(*handle < 0)
        {
            ble_cmd_cxt.rx_buffer_pos = 0;
            os_printf(LM_APP, LL_ERR, "Uart send missing con handle, discard data\n");
        }
    }
    else
    {
        *handle = _get_cur_conn_handle();
    }

	if (*handle < 0)
	{
		return ret;
	}
	else
	{
		mtu = ecr_ble_gattc_get_mtu(*handle)-3;
	}
	
	if(wr > rd)
    {
		//distance betweed wr and rd
        wrrd_len = wr - rd;
		if(wrrd_len >= mtu)
		{
			memcpy(ble_cmd_cxt.rx_buffer, ble_cmd_cxt.uart_buffer+rd, mtu);
			len = mtu;
		}
		else
		{
			memcpy(ble_cmd_cxt.rx_buffer, ble_cmd_cxt.uart_buffer+rd, wrrd_len);
			len = wrrd_len;
		}
    }
    else
    {
		//distance betweed wr and rd
        wrrd_len = wr + (UART_BUFFER_LEN-rd);
		if(wrrd_len >= mtu)
		{
			if(rd+mtu<=UART_BUFFER_LEN)
			{
				memcpy(ble_cmd_cxt.rx_buffer, ble_cmd_cxt.uart_buffer+rd, mtu);
			}
			else
			{
				memcpy(ble_cmd_cxt.rx_buffer, ble_cmd_cxt.uart_buffer+rd, UART_BUFFER_LEN-rd);
        		memcpy(ble_cmd_cxt.rx_buffer+(UART_BUFFER_LEN-rd), ble_cmd_cxt.uart_buffer, mtu-(UART_BUFFER_LEN-rd));
			}
    		len = mtu;
		}
		else
		{
			memcpy(ble_cmd_cxt.rx_buffer, ble_cmd_cxt.uart_buffer+rd, UART_BUFFER_LEN-rd);
        	memcpy(ble_cmd_cxt.rx_buffer+(UART_BUFFER_LEN-rd), ble_cmd_cxt.uart_buffer, wr);
	        len = UART_BUFFER_LEN-rd+wr;
		}
    }

    ble_cmd_cxt.rx_buffer_pos = len;

    ble_cmd_cxt.uart_buf_rd = (ble_cmd_cxt.uart_buf_rd+len)%UART_BUFFER_LEN;

    ret = true;    
    return ret;
}



static void _ble_process_uart_data_msg(int handle)
{
    int con_handle = handle;
    if(!_ble_read_data_from_buf(&con_handle))
    {
        return;
    }

    if(con_handle == cmd_slave_con.conn_handle)
    {        
        if(cmd_slave_con.tx_handle == ECR_BLE_GATT_INVALID_HANDLE)
        {
            os_printf(LM_APP, LL_ERR, "slave invalid char handler\n");                
            return;
        }
        cmd_slave_con.gatt_mtu = ecr_ble_gattc_get_mtu(con_handle)-3;
        
        if(cmd_slave_con.gatt_mtu >= ble_cmd_cxt.rx_buffer_pos)
		{
            ecr_ble_gatts_value_notify(con_handle, cmd_slave_con.tx_handle, (uint8_t*)ble_cmd_cxt.rx_buffer, ble_cmd_cxt.rx_buffer_pos);
            ble_cmd_cxt.sent_pos = ble_cmd_cxt.rx_buffer_pos;
        }
        else
        {
            ecr_ble_gatts_value_notify(con_handle, cmd_slave_con.tx_handle, (uint8_t*)ble_cmd_cxt.rx_buffer, cmd_slave_con.gatt_mtu);
            ble_cmd_cxt.sent_pos = cmd_slave_con.gatt_mtu;
        }
    }
    else
    {
        BLE_CON_LIST_T * con_info = _master_con_get_by_conn(con_handle);

        if(con_info == NULL || con_info->conn_info.rx_handle == ECR_BLE_GATT_INVALID_HANDLE)
        {
            os_printf(LM_APP, LL_ERR, "master invalid char handler\n");                
            return;
        }
        con_info->conn_info.gatt_mtu = ecr_ble_gattc_get_mtu(con_handle)-3;
        if(con_info->conn_info.gatt_mtu >= ble_cmd_cxt.rx_buffer_pos)
		{
            ecr_ble_gattc_write_without_rsp(con_info->conn_info.conn_handle, con_info->conn_info.rx_handle, (uint8_t*)ble_cmd_cxt.rx_buffer, ble_cmd_cxt.rx_buffer_pos);
            ble_cmd_cxt.sent_pos = ble_cmd_cxt.rx_buffer_pos;
        }
        else
        {
            ecr_ble_gattc_write_without_rsp(con_info->conn_info.conn_handle, con_info->conn_info.rx_handle, (uint8_t*)ble_cmd_cxt.rx_buffer, con_info->conn_info.gatt_mtu);
            ble_cmd_cxt.sent_pos = con_info->conn_info.gatt_mtu;
        }
    }    
}

static void _ble_app_init(void)
{
    ecr_ble_get_device_name((uint8_t*)ble_cmd_cxt.dev_name, sizeof(ble_cmd_cxt.dev_name));
    if(0 == strlen(ble_cmd_cxt.dev_name))
    {
        memcpy(&ble_cmd_cxt.dev_name, DEFAULT_DEV_NAME, strlen(DEFAULT_DEV_NAME));
        ecr_ble_set_device_name((uint8_t*)ble_cmd_cxt.dev_name, sizeof(ble_cmd_cxt.dev_name));
    }

    #if defined(CONFIG_NV)
	int status1 = hal_system_get_config(CUSTOMER_NV_BLE_ROLE, &ble_cmd_cxt.dev_role, sizeof(ble_cmd_cxt.dev_role));
	if((0 == status1) || (0xffffffff == status1))
	#endif
	{
	    ble_cmd_cxt.dev_role = BLE_ROLE_SLAVE;
	}

    #if defined(CONFIG_NV)
	//int status2 = hal_system_get_config(CUSTOMER_NV_BLE_ADVMODE, &ble_cmd_cxt.adv_mod, sizeof(ble_cmd_cxt.adv_mod));
	int status2 = develop_get_env_blob("BleAdvMode", &ble_cmd_cxt.adv_mod, sizeof(ble_cmd_cxt.adv_mod),NULL);
	if((0 == status2) || (0xffffffff == status2))
	#endif
	{
	    ble_cmd_cxt.adv_mod = BLE_ADV_MANUAL;
	}

    ecr_ble_reset();
    ecr_ble_gap_callback_register(_ble_eus_gap_event_cb);
    ecr_ble_gatt_callback_register(_ble_eus_gatt_event_cb);

    if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE || ble_cmd_cxt.dev_role == BLE_ROLE_SM)
    {
        _ble_eus_service_init();
        if(ble_cmd_cxt.adv_mod == BLE_ADV_AUTO)
        {
            _ble_adv_start(BLE_ADV_AUTO);
        }
    }
}

void  _ble_process_msg_task(void* param)
{
    BLE_CMD_MSG_ST msg_recv = {0};

    _ble_app_init();    
    
    while(1){          
        if(0 == os_queue_receive(ble_cmd_cxt.msg_queue, (char *)&msg_recv, sizeof(BLE_CMD_MSG_ST), WAIT_FOREVER))
        {
            switch(msg_recv.msg_type)
            {
                case BLE_CMD_MSG_RECV_UART:
                {
                    _ble_process_uart_data_msg(msg_recv.conn_handle);
                }
                break;
				
                case BLE_CMD_MSG_SRV_DISC:
                {
                    ECR_BLE_UUID_T uuid;
                    uuid.uuid_type = ECR_BLE_UUID_TYPE_128;
                    memcpy(uuid.uuid.uuid128, EUS_SERVICE_UUID, EUS_SERVICE_UUID_LEN);
                    ecr_ble_gattc_service_discovery_by_uuid(msg_recv.conn_handle, uuid);
                }
                break;

                case BLE_CMD_MSG_CHAR_DISC:
                {
                    BLE_CON_LIST_T * con_info = NULL;
                    con_info = _master_con_get_by_conn(msg_recv.conn_handle);

                    if(con_info != NULL)
                    {
                        ecr_ble_gattc_all_char_discovery(msg_recv.conn_handle, con_info->start_handle, con_info->end_handle);
                    }
                }
                break;

                case BLE_CMD_MSG_DESC_DISC:
                {
                    BLE_CON_LIST_T * con_info = NULL;
                    con_info = _master_con_get_by_conn(msg_recv.conn_handle);
					
                    if(con_info != NULL)
                    {
                        ecr_ble_gattc_char_desc_discovery(msg_recv.conn_handle, con_info->start_handle, con_info->end_handle);
                    }
                }
                break;

                case BLE_CMD_MSG_CCCD_ENABLE:
                {
                    BLE_CON_LIST_T * con_info = NULL;
                    con_info = _master_con_get_by_conn(msg_recv.conn_handle);
					
					uint8_t cccd_enable = 1; 
                    if(con_info != NULL)
                    {
                       con_info->cccd_num-=1;
                       ecr_ble_gattc_write(con_info->conn_info.conn_handle, con_info->cccd_handle[con_info->cccd_num], &cccd_enable, 1);
                    }
                }
				break;
				
                case BLE_CMD_MSG_STOP_SCAN:
                {
                    os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_scan_stop\n");
                    ecr_ble_gap_scan_stop();
                }
                break;

                case BLE_CMD_MSG_EXH_MTU:
                {
                    ecr_ble_gattc_exchange_mtu_req(msg_recv.conn_handle, BLE_ATT_MTU_DEF);
                }
                break;

				case BLE_CMD_MSG_DEV_RECON:
				{
					char resp_buf[50];
					if(ble_cmd_cxt.reconn_time<AUTO_CONN_TIME)
					{
						ble_cmd_cxt.reconn_time++;
						ble_cmd_cxt.start_recon=true;
						os_timer_changeperiod(ble_cmd_cxt.conn_timeout, BLE_CONNECT_TIMEOUT/2);
						os_timer_start(ble_cmd_cxt.conn_timeout);
						_ble_gap_connect(ble_cmd_cxt.now_conn_idx);
					}
					else if(ble_cmd_cxt.reconn_time>=AUTO_CONN_TIME)
					{
						ble_cmd_cxt.reconn_time = 0;
						ble_cmd_cxt.start_recon=false;
						ble_cmd_cxt.connected = false;
						sprintf(resp_buf,"+DISCONNECT[1]");
						dce_emit_information_response(ble_cmd_cxt.pdce, resp_buf, -1);
					}
				}
				break;

				case BLE_CMD_MSG_DEV_REEST_CON:
				{
					os_timer_start(ble_cmd_cxt.conn_timeout);
					ble_cmd_cxt.reest_conn_time++;
					_ble_gap_connect(ble_cmd_cxt.now_conn_idx);
				}
				break;
				
                default:
                {

                }
                break;

            }//switch
        }//if
        
    }//while
}



dce_result_t at_handle_LADDR_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_ERROR;

	if(DCE_EXEC != kind)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	}
    else
    {  
        ECR_BLE_GAP_ADDR_T dev_addr = {0};

    	if(ecr_ble_get_device_addr(&dev_addr))
    	{
    		dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    	}
        else
        {
            char resp_buf[50];        
            sprintf(resp_buf,"+BLEADDR:%02X:%02X:%02X:%02X:%02X:%02X",dev_addr.addr[0], dev_addr.addr[1], dev_addr.addr[2],
                dev_addr.addr[3], dev_addr.addr[4], dev_addr.addr[5]);
            dce_emit_information_response(dce, resp_buf, -1);
            dce_emit_basic_result_code(dce,DCE_RC_OK);
            ret = DCE_RC_OK;
        }        
    }
    return ret;
}


dce_result_t at_handle_ADVMOD_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_OK;

    if(DCE_EXEC == kind)
    {
        sprintf(resp_buf,"+BLEADVMOD:%d", ble_cmd_cxt.adv_mod);
        
        dce_emit_information_response(dce, resp_buf, -1);
        dce_emit_basic_result_code(dce,DCE_RC_OK);
    }
    else if(DCE_WRITE == kind)
    {  
        if(argc !=1)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else if(argv[0].value.number != BLE_ADV_AUTO && argv[0].value.number != BLE_ADV_MANUAL)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else if(ble_cmd_cxt.dev_role == BLE_ROLE_MASTER)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else
        {
            ble_cmd_cxt.adv_mod = argv[0].value.number;
#if defined(CONFIG_NV)
           // hal_system_set_config(CUSTOMER_NV_BLE_ADVMODE, &ble_cmd_cxt.adv_mod, sizeof(ble_cmd_cxt.adv_mod));
			develop_set_env_blob("BleAdvMode",&ble_cmd_cxt.adv_mod,sizeof(ble_cmd_cxt.adv_mod));
#endif
            sprintf(resp_buf,"+BLEADVMOD:%d", ble_cmd_cxt.adv_mod);        
            dce_emit_information_response(dce, resp_buf, -1);
            dce_emit_basic_result_code(dce,DCE_RC_OK);
        
            if(ble_cmd_cxt.adv_mod == BLE_ADV_AUTO)
            {
                _ble_adv_start(BLE_ADV_AUTO);
            }
            else
            {
                ecr_ble_gap_adv_stop();
            }
        }
        
    }
    return ret;
}


dce_result_t at_handle_ADVINT_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_OK;

    if(DCE_EXEC == kind)
    {
        sprintf(resp_buf,"+BLEADVINT:%d", ble_cmd_cxt.adv_interval);        
        dce_emit_information_response(dce, resp_buf, -1);
        dce_emit_basic_result_code(dce,DCE_RC_OK);
    }
    else if(DCE_WRITE == kind)
    {  
        if(argc !=1)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else if(argv[0].value.number < BLE_ADV_INTERVAL_MIN || argv[0].value.number > BLE_ADV_INTERVAL_MAX)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else
        {
            ble_cmd_cxt.adv_interval = argv[0].value.number;
            
            sprintf(resp_buf,"+BLEADVINT:%d", ble_cmd_cxt.adv_interval);        
            dce_emit_information_response(dce, resp_buf, -1);
            dce_emit_basic_result_code(dce,DCE_RC_OK);
        }
    }
    return ret;
}


dce_result_t at_handle_ADVEN_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_RC_ERROR;
    BT_RET_T   bt_ret = BT_ERROR_NO_ERROR;

	if(DCE_WRITE != kind)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	}
    else
    {  
		if(argc !=1)
		{
			dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		}
        else if(argv[0].value.number != BLE_ADV_ON && argv[0].value.number != BLE_ADV_OFF)
        {
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else if(ble_cmd_cxt.adv_mod == BLE_ADV_AUTO || ble_cmd_cxt.dev_role == BLE_ROLE_MASTER)
        {
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else 
        {   
            if(argv[0].value.number == BLE_ADV_OFF)
            {
                bt_ret = ecr_ble_gap_adv_stop();
            }
            else
            {
                bt_ret = _ble_adv_start(BLE_ADV_MANUAL);
            }
            if(bt_ret == BT_ERROR_NO_ERROR)
            {
        	    sprintf(resp_buf,"+BLEADVEN:%d", argv[0].value.number);        
                dce_emit_information_response(dce, resp_buf, -1);
                dce_emit_basic_result_code(dce,DCE_RC_OK);
                ret = DCE_RC_OK;
            }
            else
            {
                dce_emit_basic_result_code(dce,DCE_RC_ERROR);
            }
        }        
    }
    return ret;
}


dce_result_t at_handle_ROLE_command(dce_t *dce,void *group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_OK;

	if(DCE_EXEC == kind)
	{
	    sprintf(resp_buf,"+BLEROLE:%d", ble_cmd_cxt.dev_role);
        
        dce_emit_information_response(dce, resp_buf, -1);
        dce_emit_basic_result_code(dce,DCE_RC_OK);
	}
    else if(DCE_WRITE == kind)
    {  
		if(argc !=1)
		{
		    ret = DCE_RC_ERROR;
			dce_emit_basic_result_code(dce,DCE_RC_ERROR);
		}
        else if(argv[0].value.number < BLE_ROLE_SLAVE || argv[0].value.number > BLE_ROLE_MASTER)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
		
        else if(ble_cmd_cxt.connected)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else
        {
            ble_cmd_cxt.dev_role = argv[0].value.number;
#if defined(CONFIG_NV)
            hal_system_set_config(CUSTOMER_NV_BLE_ROLE, &ble_cmd_cxt.dev_role, sizeof(ble_cmd_cxt.dev_role));
#endif
            sprintf(resp_buf,"+BLEROLE:%d", ble_cmd_cxt.dev_role);        
            dce_emit_information_response(dce, resp_buf, -1);
            dce_emit_basic_result_code(dce,DCE_RC_OK);

            ecr_ble_reset();
            if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE || ble_cmd_cxt.dev_role == BLE_ROLE_SM)
            {      
                _ble_eus_service_init();
                if(ble_cmd_cxt.adv_mod == BLE_ADV_AUTO)
                {
                    _ble_adv_start(BLE_ADV_AUTO);
                }
            }
		  ret = DCE_RC_OK;
        }
    }
    return ret;
}


dce_result_t at_handle_NAME_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_RC_OK;
	ECR_BLE_DATA_T	padv_data = {0};
	ECR_BLE_DATA_T	prsp_data = {0};

	if(DCE_EXEC == kind)
	{
	    sprintf(resp_buf,"+BLENAME:\"%s\"", ble_cmd_cxt.dev_name);
        
        dce_emit_information_response(dce, resp_buf, -1);
        dce_emit_basic_result_code(dce,DCE_RC_OK);
	}
    else if(DCE_WRITE == kind)
    {  
		if(argc !=1)
		{
			dce_emit_basic_result_code(dce, DCE_RC_ERROR);
			ret = DCE_RC_ERROR;
		}
        else
        {   
            if(0 == ecr_ble_set_device_name((uint8_t*)argv[0].value.string,strlen(argv[0].value.string)))
            {
                uint16_t len = (strlen(argv[0].value.string) < BLE_DEVICE_NAME_MAX_LEN-1) ? (strlen(argv[0].value.string)) : (BLE_DEVICE_NAME_MAX_LEN-1);
                memset(ble_cmd_cxt.dev_name, 0x00, sizeof(ble_cmd_cxt.dev_name));
                memcpy(ble_cmd_cxt.dev_name, argv[0].value.string, len);
				_ble_set_adv_data(&padv_data);
				_ble_set_scan_rsp_data(&prsp_data);
                ecr_ble_adv_param_update(&padv_data, &prsp_data);

                sprintf(resp_buf,"+BLENAME:\"%s\"", ble_cmd_cxt.dev_name);        
                dce_emit_information_response(dce, resp_buf, -1);
        	    dce_emit_basic_result_code(dce,DCE_RC_OK);
            }
            else
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                ret = DCE_RC_ERROR;
            }
        }
        
    }
    return ret;
}

dce_result_t at_handle_TTMOD_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    char resp_buf[50];
    dce_result_t ret = DCE_RC_OK;

     if(ble_cmd_cxt.connected == 0)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        ret = DCE_RC_ERROR;
    }
	else
	{
    	if(DCE_EXEC == kind)
    	{
    	    sprintf(resp_buf,"+TTMOD:%d", ble_cmd_cxt.ttmod);        
            dce_emit_information_response(dce, resp_buf, -1);
            dce_emit_basic_result_code(dce, DCE_RC_OK);
    	}
        else if(DCE_WRITE == kind)
        {  
    		if(argc !=1 || argv[0].value.number != BLE_TRANSPARENT_MODE)
    		{
    			dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    			ret = DCE_RC_ERROR;
    		}        
            else
            {
                sprintf(resp_buf,"+TTMOD:%d", BLE_TRANSPARENT_MODE);        
                dce_emit_information_response(dce, resp_buf, -1);
                dce_emit_basic_result_code(dce,DCE_RC_OK);
    
                _ble_switch_ttmod(BLE_TRANSPARENT_MODE);
            }
            
        }
	}
    return ret;
}


dce_result_t at_handle_DISC_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_OK;

	if(DCE_WRITE != kind)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		ret = DCE_RC_ERROR;
	}
    else
    {  
		if(argc !=1)
		{
			dce_emit_basic_result_code(dce, DCE_RC_ERROR);
			ret = DCE_RC_ERROR;
		}
        else if(!_ble_conn_handle_valid(argv[0].value.number))
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            ret = DCE_RC_ERROR;
        }
        else
        {            
    	    dce_emit_basic_result_code(dce,DCE_RC_OK);
            //dce_emit_information_response(dce, "+DISCS", -1);
			ble_cmd_cxt.active_dis = true;
            ecr_ble_gap_disconnect(argv[0].value.number, BT_ERROR_CON_TERM_BY_LOCAL_HOST);
        }
        
    }
    return ret;
}


dce_result_t at_handle_SCAN_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    ECR_BLE_GAP_SCAN_PARAMS_T ECR_BLE_GAP_SCAN_PARAMS = {0};
    dce_result_t ret = DCE_RC_OK;
	
	ECR_GAP_ACT_STATE_T act_state = ECR_BLE_GAP_ACT_STATE_INVALID;

	act_state = ecr_ble_actv_state_get(ECR_BT_SCAN_IDX);

	if(act_state == ECR_BLE_GAP_ACT_STATE_STARTED)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		return DCE_RC_ERROR;
	}else
	{
		_scan_dev_clear_list();
	}

	if(DCE_EXEC != kind)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		ret = DCE_RC_ERROR;
	}
    else if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        ret = DCE_RC_ERROR;
    }
	else if(ble_cmd_cxt.connected)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        ret = DCE_RC_ERROR;
    }
	else
	{
        ECR_BLE_GAP_SCAN_PARAMS.scan_type = ECR_SCAN_TYPE_CONN_DISC;
    	ECR_BLE_GAP_SCAN_PARAMS.dup_filt_pol = ECR_DUP_FILT_DIS;
    	ECR_BLE_GAP_SCAN_PARAMS.scan_prop = ECR_BLE_SCAN_PROP_PHY_1M_BIT|ECR_BLE_SCAN_PROP_ACTIVE_1M_BIT;
    	ECR_BLE_GAP_SCAN_PARAMS.interval = DEFAULT_SCAN_INTERVAL;
    	ECR_BLE_GAP_SCAN_PARAMS.window = DEFAULT_SCAN_WINDOW;
    	ECR_BLE_GAP_SCAN_PARAMS.duration = ble_cmd_cxt.scan_info.scan_during*100;
    	ECR_BLE_GAP_SCAN_PARAMS.period = 0;
    	ECR_BLE_GAP_SCAN_PARAMS.scan_channel_map = 0x07;

        if(BT_ERROR_NO_ERROR == ecr_ble_gap_scan_start(&ECR_BLE_GAP_SCAN_PARAMS))
        {
            dce_emit_basic_result_code(dce,DCE_RC_OK);
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            ret = DCE_RC_ERROR;
        }       
    }
    return ret;
}

dce_result_t at_handle_SCANPARAM_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_OK;
    char resp_buf[50];
	if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE)
	{
		dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	}
	else
	{
	    if(DCE_EXEC == kind)
    	{
             sprintf(resp_buf,"+BLESCANPARAM:<%d>",ble_cmd_cxt.scan_info.scan_during);
             dce_emit_information_response(dce, resp_buf, -1);
             dce_emit_basic_result_code(dce, DCE_RC_OK);
    	}
    	else if(DCE_WRITE == kind)
    	{
       	  if(argc!= 1)
       	  {
               dce_emit_basic_result_code(dce, DCE_RC_ERROR);   
               ret = DCE_RC_ERROR;
       	  }
   		  else if(argv->value.number<0)
   		  {
               dce_emit_basic_result_code(dce, DCE_RC_ERROR);
               ret = DCE_RC_ERROR;
   		  }
   		  else
   		  {
               ble_cmd_cxt.scan_info.scan_during = argv->value.number;
               sprintf(resp_buf,"+BLESCANPARAM:<%d>",ble_cmd_cxt.scan_info.scan_during);
               dce_emit_information_response(dce, resp_buf, -1);
               dce_emit_basic_result_code(dce, DCE_RC_OK);
          }
    	}
		else
		{
               dce_emit_basic_result_code(dce, DCE_RC_ERROR);
               ret = DCE_RC_ERROR;
		}
	}
	return ret;
}

dce_result_t at_handle_SCANSTOP_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_ERROR;

    if(DCE_EXEC != kind)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }
    else if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        ret = DCE_RC_ERROR;
    }
    else
    {  
        if(BT_ERROR_NO_ERROR != ecr_ble_gap_scan_stop())
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        }
        else
        {
            dce_emit_basic_result_code(dce,DCE_RC_OK);
            ret = DCE_RC_OK;
        }        
    }
    return ret;
}


dce_result_t at_handle_SCANRLT_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_ERROR;

    if(DCE_WRITE != kind)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }
    else if(ble_cmd_cxt.dev_role == BLE_ROLE_SLAVE)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        ret = DCE_RC_ERROR;
    }
    else
    {            
		if(argc !=1)
		{
		    ret = DCE_RC_ERROR;
			dce_emit_basic_result_code(dce,DCE_RC_ERROR);
		}
        else if(argv[0].value.number < 0)
        {
            ret = DCE_RC_ERROR;
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else
        {
            BLE_PEER_INFO_T *p = scan_devs.head;
            char resp_buf[50];

			while(p != NULL)
			{
				if(p->index == argv[0].value.number)
				{
					break;
				}
				p = p->next;
			}

			if(p != NULL)
			{
	            sprintf(resp_buf,"+SCANLT:%d,%02X:%02X:%02X:%02X:%02X:%02X", p->index, p->peer_addr.addr[5], p->peer_addr.addr[4], p->peer_addr.addr[3],
	                    p->peer_addr.addr[2], p->peer_addr.addr[1], p->peer_addr.addr[0]);
	            dce_emit_information_response(dce, resp_buf, -1);
	            dce_emit_basic_result_code(dce, DCE_RC_OK);
			}
			else
			{
				dce_emit_basic_result_code(dce, DCE_RC_ERROR);
			}
        }
    }
    return ret;
}


dce_result_t at_handle_CONN_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
    dce_result_t ret = DCE_RC_ERROR;

    if(DCE_WRITE != kind)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }
    else
    {  
        if(argc !=1)
        {            
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
        else if(argv[0].value.number < 0 || argv[0].value.number > scan_devs.num-1)
        {            
            dce_emit_basic_result_code(dce,DCE_RC_ERROR);
        }
		else if(ble_cmd_cxt.dev_role != BLE_ROLE_MASTER)
		{
			dce_emit_basic_result_code(dce,DCE_RC_ERROR);
		}
        else if(ble_cmd_cxt.connecting)
        {
            dce_emit_information_response(dce, "INVALID CONNECTION COMMAND", -1);
        }
		else if(ble_cmd_cxt.connected)
		{
            dce_emit_information_response(dce, "ALREADY CONNECTED", -1);
		}
        else
        {
            os_timer_start(ble_cmd_cxt.conn_timeout);
			//stop scan activity
			ecr_ble_gap_scan_stop();
			ble_cmd_cxt.connecting = true;
			ble_cmd_cxt.now_conn_idx = argv[0].value.number;
            if(BT_ERROR_NO_ERROR == _ble_gap_connect(argv[0].value.number))
            {                
                ret = DCE_RC_OK;
            }
        }      
    }
    return ret;
}



void ble_at_init_func(dce_t* dce)
{
    T_DRV_UART_CONFIG config;
    uint32_t uart_recv_timeout = UART_DEFAULT_RECV_TIMEOUT;
    memset(&ble_cmd_cxt, 0x00, sizeof(ble_cmd_cxt));
    ble_cmd_cxt.pdce = dce;
    ble_cmd_cxt.msg_queue = os_queue_create("ble_at_msg_queue", 10, sizeof(BLE_CMD_MSG_ST), 0);

    #if defined(CONFIG_NV)
    if(hal_system_get_config(CUSTOMER_NV_UART_CONFIG, &(config), sizeof(config)))
    {
       if(config.uart_baud_rate != BAUD_RATE_9600)
       {
         //Calculate timeout 40bit 40*1/baud_rate*1000=x.xx ms =x+1 
         uart_recv_timeout = (uint32_t)(4*10*(1/config.uart_baud_rate)*1000)+1;
       }
    }
    #endif
	if(uart_recv_timeout <= portTICK_PERIOD_MS)
	{
        uart_recv_timeout = 2*portTICK_PERIOD_MS;
	}
    ble_cmd_cxt.uart_rx_timeout = os_timer_create("uart_rx_timeout", uart_recv_timeout, 0, _ble_uart_rx_timeout, NULL);

    ble_cmd_cxt.ttmod = BLE_NON_TRANSPARENT_MODE;
    ble_cmd_cxt.dev_role = BLE_ROLE_SLAVE;
    ble_cmd_cxt.adv_mod = BLE_ADV_AUTO;
    ble_cmd_cxt.adv_interval = DEFAULT_ADV_INTERVAL;
    memset(ble_cmd_cxt.dev_name, 0x00, sizeof(ble_cmd_cxt.dev_name));

    ble_cmd_cxt.conn_timeout = os_timer_create("connect_timeout", BLE_CONNECT_TIMEOUT, 0, _ble_connect_timeout, NULL);
    os_task_create("BLE_PROCESS_MSG", 8, 4096, (task_entry_t)_ble_process_msg_task, NULL);
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

dce_result_t at_handle_apps_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv)
{
	dce_result_t ret = DCE_RC_OK;
	if(DCE_EXEC != kind)
	{
	    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		ret = DCE_RC_ERROR;
	}
	ecr_ble_reset();
	BLE_APPS_INIT();
	ecr_ble_adv_start();
    dce_emit_basic_result_code(dce, DCE_RC_OK);
	ret = DCE_RC_OK;
			
		
	return  ret;


}

static const command_desc_t BLE_commands[]={
   
	{ "BLEADDR",            &at_handle_LADDR_command,             DCE_EXEC},
    { "BLEADVMOD",           &at_handle_ADVMOD_command,            DCE_EXEC|DCE_WRITE},    
    { "BLEADVINT",           &at_handle_ADVINT_command,            DCE_EXEC|DCE_WRITE},
    { "BLEADVEN",            &at_handle_ADVEN_command,             DCE_WRITE},
    { "BLEROLE",             &at_handle_ROLE_command,              DCE_EXEC|DCE_WRITE},
    { "BLENAME" ,            &at_handle_NAME_command,              DCE_EXEC|DCE_WRITE},
    { "BLETTMOD" ,           &at_handle_TTMOD_command,             DCE_EXEC|DCE_WRITE},
    { "BLEDISC" ,             &at_handle_DISC_command,              DCE_WRITE},
    { "BLESCANPARAM" ,        &at_handle_SCANPARAM_command,         DCE_EXEC|DCE_WRITE},
    { "BLESCAN" ,             &at_handle_SCAN_command,               DCE_EXEC},    
    { "BLESCANSTOP" ,         &at_handle_SCANSTOP_command,           DCE_EXEC},    
    { "BLESCANRLT" ,          &at_handle_SCANRLT_command,           DCE_WRITE},
    { "BLECONN" ,             &at_handle_CONN_command,           DCE_WRITE},
	{ "BLEAPPS",			&at_handle_apps_command,				DCE_EXEC}
//	{ "BLEAPPS",			&at_handle_apps_command,				DCE_EXEC}
};


void dce_register_ble_commands(dce_t* dce)
{
    dce_register_command_group(dce,"BLE",BLE_commands,sizeof(BLE_commands)/sizeof(command_desc_t),0);
    
    ble_at_init_func(dce);

}
