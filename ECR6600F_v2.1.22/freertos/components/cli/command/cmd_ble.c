/**
 * @file cmd_ble
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-8
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
	
#include "cli.h"
#include "chip_pinmux.h"
#include "format_conversion.h"
#include "bluetooth.h"
#include "bluetooth_err.h"
#if defined (CONFIG_NV)
#include "system_config.h"
#endif
#if defined(CFG_HOST) && defined(CONFIG_BLE_TUYA_ADAPTER_TEST)
#include "ble_tuya_adapter_test.h"
#endif
#include "ble_thread.h"
#define ECR_BLE_GATT_SERVICE_MAX_NUM                                        (3)
#define ECR_BLE_GATT_CHAR_MAX_NUM                                           (3)
#define ECR_BLE_INVALID_CONIDX                                              (0xff)

extern uint8_t gapc_get_conidx(uint16_t conhdl);
/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/
#if defined(CONFIG_TRC_ENABLE)
extern void print_trace_dirct(void);
static int cmd_ble_trace(cmd_tbl_t *h, int argc, char *argv[])
{
	print_trace_dirct();
	return CMD_RET_SUCCESS;
}

CLI_CMD(ble_trace, cmd_ble_trace,  "",	  "");
#endif

#if defined(CONFIG_MULTIPLEX_UART)
extern void uart_buff_cpy(char *cmd, int cmd_len);
static int cmd_ble_hci(cmd_tbl_t *h, int argc, char *argv[])
{
	uint8_t cmd_len = 0;
	if(argc==1)
	{
		os_printf(LM_CMD, LL_ERR, "input: hci [arg], such as: hci 101f2000\n");
		return CMD_RET_FAILURE;
	}

	if (strlen(argv[1]) % 2 != 0)
	{
		os_printf(LM_CMD, LL_ERR, "hci arg error\n");
		return CMD_RET_FAILURE;
	}

	cmd_len = strlen(argv[1])/2;
	
	uart_buff_cpy(argv[1], cmd_len);

	return CMD_RET_SUCCESS;
}

CLI_CMD(hci, cmd_ble_hci,  "ble hci cmd",	 "hci [arg]");
#endif //(CONFIG_MULTIPLEX_UART)

static int ble_writereg(cmd_tbl_t *t, int argc, char *argv[])
{
	uint32_t addr = 0, value = 0;
	
	if (argc < 3) {
		return 1;
	}
	
	addr = strtoul(argv[1], NULL, 0);
	value = strtoull(argv[2], NULL, 0);
	os_printf(LM_CMD, LL_INFO, "w [0x%x,0x%x]\n", addr, value);
	(*(volatile uint32_t *)(addr)) = (value);

	return 0;
}
CLI_CMD_F(bwrite, ble_writereg, "ble write register", "bwrite [reg]", false);

static int ble_readreg(cmd_tbl_t *t, int argc, char *argv[])
{
	uint32_t addr = 0,value = 0;
	int i, n;

	if (argc < 3) {
		return 1;
	}
	
	addr = strtoul(argv[1], NULL, 0);
	n = (int)atoi(argv[2])/4;
	if (n == 0) {
		n = 1;
	}
	
	for (i = 0; i < n; i++ ) {
		value = (*(volatile uint32_t *)(addr+i*4));
		os_printf(LM_CMD, LL_INFO, "%08x: %08x\n", addr + i * 4, value);
	}

	return 0;
}

CLI_CMD_F(bread, ble_readreg, "ble read register", "bread [reg]", false);

extern void ble_change_chanel(uint32_t ch, uint8_t bw_mode, uint8_t trx_mode);
static int ble_chanel(cmd_tbl_t *t, int argc, char *argv[])
{
	uint32_t ch = 0; 
	uint8_t bw_mode = 0; 
	uint8_t trx_mode = 0;
	if(argc<4)
	{
		os_printf(LM_CMD, LL_ERR, "arg error\n");
		return CMD_RET_FAILURE;
	}

	ch = atoi(argv[1]);
	bw_mode = atoi(argv[2]);
	trx_mode = atoi(argv[3]);

	ble_change_chanel(ch, bw_mode, trx_mode);
	
	return 0;
}

CLI_CMD(bch, ble_chanel, "ble change chanel", "bchanel [ch] [bw 0:1M,1:2M] [trx 1:tx,2:rx]");
extern bool rf_rfpll_channel_sel(int bw,int ch);
static int ble2wifi_chanel(cmd_tbl_t *t, int argc, char *argv[])
{
	int bw = 0;
	int ch = 0; 

	if(argc<3)
	{
		os_printf(LM_CMD, LL_ERR, "arg error\n");
		return CMD_RET_FAILURE;
	}
	bw = atoi(argv[1]);
	ch = atoi(argv[2]);

	rf_rfpll_channel_sel(bw,ch);
	return 0;
}

CLI_CMD(b2w_ch, ble2wifi_chanel, "ble us wifi chanel", "b2w_ch [bw] [ch]");

#define BLE_READ_REG(offset)        (*(volatile uint32_t*)(offset))
#define BLE_WRITE_REG(offset,value) (*(volatile uint32_t*)(offset) = (uint32_t)(value))
static int ble_rx_dump(cmd_tbl_t *t, int argc, char *argv[])
{
	uint32_t length = strtol(argv[1], NULL, 0);

	if(argc<2)
	{
		os_printf(LM_CMD, LL_ERR, "arg error\n");
		return CMD_RET_FAILURE;
	}
	
	//step1:set iram start address and max address
	BLE_WRITE_REG(0x20e014,0x0);			  //dump start address
	BLE_WRITE_REG(0x20e008,0x7fff); 		   //dump max address
	BLE_WRITE_REG(0x20e00c,length);
	//step2:debug mode select, 0:rx.1:tx,2:phy dump
	BLE_WRITE_REG(0x20e004,0);			 //0,rx.1 tx,2 phy dump
	//step3:debug mode enable
	BLE_WRITE_REG(0x20e000,1);		   // dump enable
	return 0;
}

CLI_CMD(rx_dump, ble_rx_dump, "ble rx dump", "brx_dump [max_addr]");

static int ble_auto_mode(cmd_tbl_t *t, int argc, char *argv[])
{	
	BLE_WRITE_REG(0X20208C, 0x1);//pta mode disable
	BLE_WRITE_REG(0X202054, 0x1);//wifi ble select, ble == 1
	BLE_WRITE_REG(0x47000c, 0x0);//auto hopping
	BLE_WRITE_REG(0x203290,0);//agc auto mode

	//BLE_WRITE_REG(0x203018, 0xa80000c8);//ble sx_enable init config
	//BLE_WRITE_REG(0x20311c, 0x4);
	//BLE_WRITE_REG(0x203018, 0xa80000b7);
	return 0;
}

CLI_CMD_F(ble_auto_mode, ble_auto_mode, "ble auto mode", "brx_dump", false);

static int ble_debug_mode(cmd_tbl_t *t, int argc, char *argv[])
{
	uint32_t code = 0;
	if(argc<2)
	{
		os_printf(LM_CMD, LL_ERR, "arg error\n");
		return CMD_RET_FAILURE;
	}

	code = strtoul(argv[1], NULL, 0);
	code = code & 0x7f;
	/**
	* @brief DIAGCNTL register definition
	* <pre>
	*	Bits		   Field Name	Reset Value
	*  -----   ------------------	-----------
	*	  31			 DIAG3_EN	0
	*  30:24				DIAG3	0x0
	*	  23			 DIAG2_EN	0
	*  22:16				DIAG2	0x0
	*	  15			 DIAG1_EN	0
	*  14:08				DIAG1	0x0
	*	  07			 DIAG0_EN	0
	*  06:00				DIAG0	0x0
	* </pre>
	*/
	//#define BLE_DIAGCNTL_ADDR   0x00460050

	BLE_WRITE_REG(0x00460050, /*diag3en*/ ((uint32_t)(0x00) << 31) |
							  /*diag3*/   ((uint32_t)(0x00) << 24) |
							  /*diag2en*/ ((uint32_t)(0x00) << 23) |
							  /*diag2*/   ((uint32_t)(0x00) << 16) |
							  /*diag1en*/ ((uint32_t)(0x00) << 15) |
							  /*diag1*/   ((uint32_t)(0x00) << 8) |
							  /*diag0en*/ ((uint32_t)(0x01) << 7) |
							  /*diag0*/   ((uint32_t)(code) << 0));

	PIN_FUNC_SET(IO_MUX_GPIO16, FUNC_GPIO16_PWM_CTRL2);
	PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO16_BLE_DEBUG_1);
	PIN_FUNC_SET(IO_MUX_GPIO17, FUNC_GPIO16_BLE_DEBUG_2);
	PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_BLE_DEBUG_3);
	PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO0_BLE_DEBUG_4);
	PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO0_BLE_DEBUG_5);
	PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO0_BLE_DEBUG_6);
	PIN_FUNC_SET(IO_MUX_GPIO4, FUNC_GPIO0_BLE_DEBUG_7);

	BLE_WRITE_REG(0x201214, 0);//disable reset
	return CMD_RET_SUCCESS;
}
CLI_CMD_F(ble_debug, ble_debug_mode,	"ble_debug",  "ble_debug [code]", false);

static int ble_read_rssi(cmd_tbl_t *t, int argc, char *argv[])
{
#if 0
		int8_t rssi_h, rssi_l, rssi_value;
		rssi_h = BLE_READ_REG(0x470254)&0x3f;
		rssi_l = (BLE_READ_REG(0x470258)&0xff)>>2;
		rssi_value = (rssi_h<<6) | (rssi_l);
		os_printf(LM_CMD, LL_ERR, "%d\n", rssi_value);
#else
		int8_t rssi_h, rssi_l, rssi_value, rssi_index, agc_interation, index_switch_cnt, tmp;
		rssi_h = BLE_READ_REG(0x470254)&0x3f;//0x470300
		rssi_l = (BLE_READ_REG(0x470258)&0xff)>>2;//0x470304
		rssi_value = (rssi_h<<6) | (rssi_l);
		tmp = BLE_READ_REG(0x47027c);
		rssi_index = tmp&0x70>>4;
		agc_interation = tmp&0xf;
		index_switch_cnt = BLE_READ_REG(0x47027c);
		os_printf(LM_CMD, LL_INFO, "rssi_index: %d, agc_interation: %d, index_switch_cnt: %d, rssi_value: %d\n",rssi_index, agc_interation, index_switch_cnt, rssi_value);
#endif
	return CMD_RET_SUCCESS;
}
CLI_CMD_F(ble_rssi, ble_read_rssi,  "ble_rssi",  "ble_rssi", false);

#if defined (CONFIG_WIFI_LMAC_TEST)
void wifi_pta_start();
void wifi_pta_stop();
void ble_pta_start(uint8_t act_id);
void ble_pta_stop(uint8_t act_id);
static int pta_test(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc<2)
	{
		os_printf(LM_CMD, LL_ERR, "arg error\n");
		return CMD_RET_FAILURE;
	}

	if(!strcmp(argv[1], "start"))
	{
		ble_pta_start(0);
		wifi_pta_start();
	}
	else if(!strcmp(argv[1], "stop"))
	{
		ble_pta_stop(0);
		wifi_pta_stop();
	}
	else
	{
		os_printf(LM_CMD, LL_ERR, "arg value error\n");
		return CMD_RET_FAILURE;
	}

	return 0;
}
CLI_CMD_F(pta, pta_test, "pta test", "pta [start/stop]", false);
#endif // CONFIG_ECR6600_WIFI

#if defined(CFG_HOST) && defined(CONFIG_NV)
//hongsai

#define BLE_DEVICE_NAME_MAX_LEN		(18)
#define BLE_BD_ADDR_LEN				(6)

static void ble_cli_set_adv_data(ECR_BLE_DATA_T *pdata)
{
	uint16_t index = 0;
	static uint8_t adv_data[31] = {0};
	char adv_mode[3] = {2,1,6};
	char service_uuid[4] = {3,2,1,0xa2};
	//static uint8_t adv_data[]      = {2,1,6,3,2,1,0xa2,4,0x16,1,0xa2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    //set adv mode --- flag
    memcpy(&adv_data[index], adv_mode, sizeof(adv_mode)/sizeof(adv_mode[0]));
    index += sizeof(adv_mode)/sizeof(adv_mode[0]);

    //set service uuid
    memcpy(&adv_data[index], service_uuid, sizeof(service_uuid)/sizeof(service_uuid[0]));
    index += sizeof(service_uuid)/sizeof(service_uuid[0]);

    pdata->length = index;
    pdata->p_data = adv_data;
}

static void ble_cli_set_scan_rsp_data(ECR_BLE_DATA_T *pdata)
{
	uint16_t index = 0;
	static uint8_t rsp_data[31] = {0};
	
	//static uint8_t scan_rsp_data[] = {4,9,'C','L','I',0x19,0xff,0xd0,7,0x80,3,0,0,0xc,0,0x5f,0x9f,0x6e,3,0x22,0,0x95,0xd8,0xa,0xf2,0x87,0x2e,0xda,0xac,0x83,0x7e};
    char temp_name[BLE_DEVICE_NAME_MAX_LEN] = {0};
    char default_name[5] = {4,0x09,'C','L','I'};
	
	memcpy(&rsp_data[index], temp_name, strlen(temp_name));
	int ret = hal_system_get_config(CUSTOMER_NV_BLE_DEVICE_NAME, temp_name, sizeof(temp_name));
	if (ret)
	{
		//set nv device name
		rsp_data[index++] = strlen(temp_name)+1;
		rsp_data[index++] = 0x09;
		memcpy(&rsp_data[index], temp_name, strlen(temp_name));
		index += strlen(temp_name);
	}
	else
	{
		//set default device name
		memcpy(&rsp_data[index], default_name, sizeof(default_name)/sizeof(default_name[0]));
		index += sizeof(default_name)/sizeof(default_name[0]);
	}
    pdata->length = index;
    pdata->p_data = rsp_data;
}

static int set_ble_dev_name(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_DATA_T	padv_data = {0};
	ECR_BLE_DATA_T	prsp_data = {0};
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "ARG NUM ERROR!!!\r\n");
		return CMD_RET_FAILURE;
	}

	if(0 == ecr_ble_set_device_name((uint8_t*)argv[1],strlen(argv[1])))
	{
		ble_cli_set_adv_data(&padv_data);
		ble_cli_set_scan_rsp_data(&prsp_data);
        ecr_ble_adv_param_update(&padv_data, &prsp_data);
		return CMD_RET_SUCCESS;
	}
	else
	{
		return CMD_RET_FAILURE;
	}
}
CLI_SUBCMD_F(ble, set_dev_name, set_ble_dev_name, "set device name", "ble set_dev_name", true);

static int get_ble_dev_name(cmd_tbl_t *t, int argc, char *argv[])
{
	char temp_name[BLE_DEVICE_NAME_MAX_LEN] = {'\0'};

	int ret = hal_system_get_config(CUSTOMER_NV_BLE_DEVICE_NAME, temp_name, sizeof(temp_name));
	if (ret)
	{
		os_printf(LM_CMD, LL_INFO, "ble_dev_name: %s\r\n", temp_name);
		return CMD_RET_SUCCESS;
	}
	else
	{
		return CMD_RET_FAILURE;
	}
}
CLI_SUBCMD_F(ble, get_dev_name, get_ble_dev_name, "get device name", "ble get_dev_name", true);
#endif

#if defined(CFG_HOST)

static ECR_BLE_GATTS_PARAMS_T          ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T        ecr_ble_common_service[ECR_BLE_GATT_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T           ecr_ble_common_char1[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T           ecr_ble_common_char2[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T           ecr_ble_common_char3[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
static unsigned char BLE_CMD_SERVICE_UUID2[]={0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x3e, 0x80, 0x00, 0x10, 0xad, 0x00, 0x00, 0xFE, 0x00, 0x00};





#define BLE_TMR_PRIV_ADDR_MIN                             (0x003C)
#define BLE_TMR_PRIV_ADDR_MAX                             (0x0384)

#define  COMMON_SERVICE_MAX_NUM      (3)
#define  COMMON_CHAR_MAX_NUM         (3)
 
#define  BLE_CMD_SERVICE_UUID1                 (0x180b)
#define  BLE_CMD_WRITE_CHAR_UUID              (0x2a2d)
#define  BLE_CMD_READ_CHAR_UUID               (0x2a2e)
#define  BLE_CMD_NOTIFY_CHAR_UUID             (0x2a2f)

//static uint8_t adv_data_update[]      = {0x02,0x01,0x06,0x03,0x02,0x01,0xa2,0x14,0x16,0x01,0xa2,0x01,0x6b,0x65,0x79,0x6d,0x35,0x35,0x37,0x6e,0x71,0x77,0x33,0x70,0x38,0x70,0x37,0x6d};
//static uint8_t scan_rsp_data_update[] = {0x03,0x09,0x54,0x59,0x19,0xff,0xd0,0x07,0x09,0x03,0x00,0x00,0x0c,0x00,0x1b,0xf2,0x87,0x8b,0x9c,0x3e,0xf7,0x4c,0x44,0x2b,0x95,0x5a,0x63,0x1d,0xd0,0xa8};

static void ecr_ble_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_RESET:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gap_event_reset\n");
			break;
		case ECR_BLE_GAP_EVT_CONNECT:
			{
				if(BT_ERROR_NO_ERROR == p_event->result)
				{
					os_printf(LM_CMD, LL_INFO, "Device connect succcess\n");
				}
				else
				{
					os_printf(LM_CMD, LL_INFO, "Device connect fail\n");
				}
			}
			break;
		case ECR_BLE_GAP_EVT_ADV_STATE:
			{
				if(ECR_BLE_GAP_ADV_STATE_ADVERTISING == p_event->result)
				{
					os_printf(LM_CMD, LL_INFO, "adv activity success\n");
				}
				if(ECR_BLE_GAP_ADV_STATE_IDLE == p_event->result)
				{
					os_printf(LM_CMD, LL_INFO, "stop adv success\n");
				}
			}
			break;
		case ECR_BLE_GAP_EVT_ADV_REPORT:
			{
				if((p_event->gap_event.adv_report.data.p_data == NULL)
					|| (p_event->gap_event.adv_report.data.length == 0))
				{
					//empty packet
					return;
				}
				int8_t i=0;
				os_printf(LM_CMD, LL_INFO, "adv_addr");
				
				for(i=5;i>=0;i--)
				{
					os_printf(LM_CMD, LL_INFO, ":%02X",p_event->gap_event.adv_report.peer_addr.addr[i]);
				}
				os_printf(LM_CMD, LL_INFO, "\n\r");
				os_printf(LM_CMD, LL_INFO, "rssi:%d dBm\n\r",p_event->gap_event.adv_report.rssi);
			}
			break;
		case ECR_BLE_GAP_EVT_CONN_PARAM_UPDATE:
			os_printf(LM_CMD, LL_INFO, "conn_interval=0x%04x\n",p_event->gap_event.conn_param.conn_interval_max);
			os_printf(LM_CMD, LL_INFO, "conn_latency=0x%04x\n",p_event->gap_event.conn_param.conn_latency);
			os_printf(LM_CMD, LL_INFO, "conn_sup_timeout=0x%04x\n\r",p_event->gap_event.conn_param.conn_sup_timeout);
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
				char *write_data = (char *)p_event->gatt_event.write_report.report.p_data;
				uint8_t len = p_event->gatt_event.write_report.report.length;
				write_data[len] = '\0';
				os_printf(LM_CMD, LL_INFO, "Char handle:%#02x, write_data:%s\n", p_event->gatt_event.write_report.char_handle,write_data);
			}
			break;
		
		case ECR_BLE_GATT_EVT_WRITE_CMP:
			{
				if(p_event->gatt_event.write_result.result == BT_ERROR_NO_ERROR)
				{
					os_printf(LM_BLE,LL_INFO,"The operation of write char handle %#02x is proceed\n",p_event->gatt_event.write_result.char_handle);
				}
				else
				{
					os_printf(LM_BLE,LL_INFO,"The operation of write char handle %#02x is failed,reason:%#02x\n",p_event->gatt_event.write_result.char_handle,
																					p_event->gatt_event.write_result.result);
				}
			}
			break;
		
		case ECR_BLE_GATT_EVT_READ_RX:
			{
				os_printf(LM_BLE,LL_INFO,"Char handle %#02x,data:%s\n",p_event->gatt_event.data_read.char_handle,
																	p_event->gatt_event.data_read.report.p_data);		
			}
			break;
		
		case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_RX:
			{
				os_printf(LM_CMD, LL_INFO, "notify/indicate char handle=%#02x\n",p_event->gatt_event.data_report.char_handle);
				os_printf(LM_CMD, LL_INFO, "notify/indicate char value length=%#02x\n",p_event->gatt_event.data_report.report.length);
				os_printf(LM_CMD, LL_INFO, "notify/indicate char value=");
				for(int i=0;i<p_event->gatt_event.data_report.report.length;i++)
				{
					os_printf(LM_BLE,LL_INFO,"%c",p_event->gatt_event.data_report.report.p_data[i]);
				}
				os_printf(LM_BLE,LL_INFO,"\n");
			}
			break;
		
		case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_TX:
			{
				if(p_event->gatt_event.notify_result.result == BT_ERROR_NO_ERROR)
				{
					os_printf(LM_BLE,LL_INFO,"The operation of notify/indicate char handle %#02x is proceed\n",p_event->gatt_event.notify_result.char_handle);
				}
				else
				{
					os_printf(LM_BLE,LL_INFO,"The operation of notify/indicate char handle %#02x is failed,reason:%#02x\n",p_event->gatt_event.notify_result.char_handle,
																					p_event->gatt_event.notify_result.result);
				}
			}
			break;
		
		case ECR_BLE_GATT_EVT_CHAR_DISCOVERY:
			{
				if(p_event->result != BT_ERROR_DISC_DONE)
				{
					os_printf(LM_CMD, LL_INFO, "char handle=%#02x\n",p_event->gatt_event.char_disc.handle);
					if(ECR_BLE_UUID_TYPE_16 == p_event->gatt_event.char_disc.uuid.uuid_type)
					os_printf(LM_CMD, LL_INFO, "char uuid=%#04x\n",p_event->gatt_event.char_disc.uuid.uuid.uuid16);
					else if(ECR_BLE_UUID_TYPE_32 == p_event->gatt_event.char_disc.uuid.uuid_type)
					os_printf(LM_CMD, LL_INFO, "char uuid=%#x\n",p_event->gatt_event.char_disc.uuid.uuid.uuid32);
					else if(ECR_BLE_UUID_TYPE_128 == p_event->gatt_event.char_disc.uuid.uuid_type)
					os_printf(LM_CMD, LL_INFO, "char uuid=%#x\n",p_event->gatt_event.char_disc.uuid.uuid.uuid128);
				}
			}
			break;
		
		case ECR_BLE_GATT_EVT_CHAR_DESC_DISCOVERY:
			{
				if(p_event->result != BT_ERROR_DISC_DONE)
				{
					os_printf(LM_CMD, LL_INFO, "char descriptors handle=%#02x\n",p_event->gatt_event.desc_disc.cccd_handle);
					os_printf(LM_CMD, LL_INFO, "char descriptors uuid16=%#04x\n",p_event->gatt_event.desc_disc.uuid16);
				}
			}
			break;
		
		case ECR_BLE_GATT_EVT_PRIM_SEV_DISCOVERY:
			{
				if(p_event->result != BT_ERROR_DISC_DONE)
				{
					os_printf(LM_CMD, LL_INFO, "services start handle:%#02x\n",p_event->gatt_event.svc_disc.start_handle);
					os_printf(LM_CMD, LL_INFO, "services end handle:%#02x\n",p_event->gatt_event.svc_disc.end_handle);
					os_printf(LM_CMD, LL_INFO, "services uuid:%#04x\n",p_event->gatt_event.svc_disc.uuid.uuid.uuid16);
				}
			}
			break;
		
		default:
			break;
	}

}

static int ble_set_addr(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_GAP_ADDR_T dev_addr = {0};
	dev_addr.type = strtoul(argv[1],NULL,0);

    if (dev_addr.type > 0)
    {
        os_printf(LM_CMD, LL_ERR, "Not supported address type\n");
        return CMD_RET_FAILURE;
    }
    else
    {
        if(argc != 3)
        {
            os_printf(LM_CMD, LL_ERR, "Input ERROR!!!, Please input: ble set_addr [addr_type] [address]\n");
            return CMD_RET_FAILURE;
        }

        for (int i = 0; i < 6; i++)
        {
            dev_addr.addr[i] = hex2num(*(argv[2]+i*3)) * 0x10 + hex2num(*(argv[2]+i*3+1));
        }
    }

    if(ecr_ble_set_device_addr(&dev_addr) != BT_ERROR_NO_ERROR)
    {
        os_printf(LM_CMD, LL_ERR, "set address fail\n");
        return CMD_RET_FAILURE;
    }
    return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, set_addr, ble_set_addr, "set own device addr", "ble set_addr", true);

static int ble_get_addr(cmd_tbl_t *t, int argc, char *argv[])
{
    ECR_BLE_GAP_ADDR_T dev_addr = {0};

    if (ecr_ble_get_device_addr(&dev_addr) == BT_ERROR_NO_ERROR)
    {
        os_printf(LM_CMD, LL_INFO, "addr_type: %x, ble_addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",dev_addr.type,
                    dev_addr.addr[0],dev_addr.addr[1],dev_addr.addr[2],dev_addr.addr[3],dev_addr.addr[4],dev_addr.addr[5]);
    }
    return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, get_addr, ble_get_addr, "get own device addr", "ble get_addr", true);

static int ble_set_dev_config(cmd_tbl_t *t, int argc, char *argv[])
{
    if(argc != 3)
    {
        os_printf(LM_CMD, LL_ERR, "Input ERROR!!!, Please input: ble set_dev_config [renew_dur] [privacy_cfg]\n");
        return CMD_RET_FAILURE;
    }

	ECR_BLE_DEV_CONFIG p_dev_config;
	
	p_dev_config.role = BLE_ROLE_ALL;
	
	p_dev_config.renew_dur = strtoul(argv[1],NULL,0);
    p_dev_config.privacy_cfg = strtoul(argv[2],NULL,0);

	memset((void *)&p_dev_config.irk[0], 0x00, 16);
	
	p_dev_config.pairing_mode = BLE_PAIRING_DISABLE;
	p_dev_config.max_mps = 512;
	p_dev_config.max_mtu = 512;

    if (p_dev_config.renew_dur < BLE_TMR_PRIV_ADDR_MIN 
        || p_dev_config.renew_dur > BLE_TMR_PRIV_ADDR_MAX)
    {
        os_printf(LM_BLE, LL_ERR, "error, renew_dur range: 0x003c - 0x284\n");
        return CMD_RET_FAILURE;
    }

    if (p_dev_config.privacy_cfg != 0 && p_dev_config.privacy_cfg != 1
        && p_dev_config.privacy_cfg != 4)
    {
        os_printf(LM_BLE, LL_ERR, "error, argv[2] mean: 0 - Pbulic addr, 1 - static addr, 4 - (resolvable | non-resolvable)\n");
        return CMD_RET_FAILURE;
    }
	ecr_ble_set_dev_config(&p_dev_config);
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, set_dev_config, ble_set_dev_config, "set device config", "ble set_dev_config", false);


static int ble_init(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: device_role\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		uint8_t device_role = strtoul(argv[1],NULL,0);
		if(device_role > 1)
		{
			os_printf(LM_APP, LL_INFO, "Invalid arguments\n");
			return CMD_RET_FAILURE;
		}
		ecr_ble_gap_callback_register(ecr_ble_gap_event_cb);
		ecr_ble_gatt_callback_register(ecr_ble_gatt_event_cb);
		ecr_ble_reset();
		memset(ecr_ble_common_service, 0x00, sizeof(ECR_BLE_SERVICE_PARAMS_T)*ECR_BLE_GATT_SERVICE_MAX_NUM);

		if(device_role == 0)
		{
			ECR_BLE_GATTS_PARAMS_T *p_ble_service = &ecr_ble_gatt_service;
			p_ble_service->svc_num =  COMMON_SERVICE_MAX_NUM;
			p_ble_service->p_service = ecr_ble_common_service;

			/*First service add*/
			ECR_BLE_SERVICE_PARAMS_T *p_common_service = ecr_ble_common_service;
			p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
			p_common_service->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID1;
			p_common_service->type     = ECR_BLE_UUID_SERVICE_PRIMARY;
			p_common_service->char_num = COMMON_CHAR_MAX_NUM;
			p_common_service->p_char   = ecr_ble_common_char1;
			
			/*Add write characteristic*/
			ECR_BLE_CHAR_PARAMS_T *p_common_char = ecr_ble_common_char1;
			p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_WRITE_CHAR_UUID;
			
			p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
			p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char->value_len = 252;
			p_common_char++;

			/*Add Notify characteristic*/
			p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_NOTIFY_CHAR_UUID;
			
			p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
			p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char->value_len = 252;
			p_common_char++;
			
			/*Add Read && write characteristic*/
			p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char->char_uuid.uuid.uuid16 = BLE_CMD_READ_CHAR_UUID;
			
			p_common_char->property = ECR_BLE_GATT_CHAR_PROP_READ | ECR_BLE_GATT_CHAR_PROP_WRITE;
			p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char->value_len = 252;

			/*Second service add*/
			ECR_BLE_SERVICE_PARAMS_T *p_common_service_2 = &ecr_ble_common_service[1];
			p_common_service_2->handle = ECR_BLE_GATT_INVALID_HANDLE;
			//p_common_service_2->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
			p_common_service_2->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_128;
			//p_common_service_2->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID;
			memcpy(&p_common_service_2->svc_uuid.uuid.uuid128,BLE_CMD_SERVICE_UUID2,sizeof(BLE_CMD_SERVICE_UUID2));
			p_common_service_2->type     = ECR_BLE_UUID_SERVICE_PRIMARY;
			p_common_service_2->char_num = COMMON_CHAR_MAX_NUM;
			p_common_service_2->p_char   = ecr_ble_common_char2;

			/*Add write characteristic*/
			ECR_BLE_CHAR_PARAMS_T *p_common_char2 = ecr_ble_common_char2;
			p_common_char2->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char2->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char2->char_uuid.uuid.uuid16 = BLE_CMD_WRITE_CHAR_UUID;
			
			p_common_char2->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
			p_common_char2->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char2->value_len = 252;
			p_common_char2++;

			/*Add Notify characteristic*/
			p_common_char2->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char2->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char2->char_uuid.uuid.uuid16 = BLE_CMD_NOTIFY_CHAR_UUID;
			
			p_common_char2->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
			p_common_char2->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char2->value_len = 252;
			p_common_char2++;
			
			/*Add Read && write characteristic*/
			p_common_char2->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char2->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char2->char_uuid.uuid.uuid16 = BLE_CMD_READ_CHAR_UUID;
			
			p_common_char2->property = ECR_BLE_GATT_CHAR_PROP_READ | ECR_BLE_GATT_CHAR_PROP_WRITE;
			p_common_char2->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char2->value_len = 252;

			/*Third service add*/
			ECR_BLE_SERVICE_PARAMS_T *p_common_service_3 = &ecr_ble_common_service[2];
			p_common_service_3->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_service_3->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
			p_common_service_3->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID1;
			p_common_service_3->type     = ECR_BLE_UUID_SERVICE_PRIMARY;
			p_common_service_3->char_num = COMMON_CHAR_MAX_NUM;
			p_common_service_3->p_char   = ecr_ble_common_char3;

			/*Add write characteristic*/
			ECR_BLE_CHAR_PARAMS_T *p_common_char3 = ecr_ble_common_char3;
			p_common_char3->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char3->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char3->char_uuid.uuid.uuid16 = BLE_CMD_WRITE_CHAR_UUID;
			
			p_common_char3->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
			p_common_char3->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char3->value_len = 252;
			p_common_char3++;

			/*Add Notify characteristic*/
			p_common_char3->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char3->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char3->char_uuid.uuid.uuid16 = BLE_CMD_NOTIFY_CHAR_UUID;
			
			p_common_char3->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
			p_common_char3->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char3->value_len = 252;
			p_common_char3++;
			
			/*Add Read && write characteristic*/
			p_common_char3->handle = ECR_BLE_GATT_INVALID_HANDLE;
			p_common_char3->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
			p_common_char3->char_uuid.uuid.uuid16 = BLE_CMD_READ_CHAR_UUID;
			
			p_common_char3->property = ECR_BLE_GATT_CHAR_PROP_READ | ECR_BLE_GATT_CHAR_PROP_WRITE;
			p_common_char3->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
			p_common_char3->value_len = 252;
			ecr_ble_gatts_service_add(p_ble_service, false);
		}
		return CMD_RET_SUCCESS;
	}
}
CLI_SUBCMD_F(ble, init, ble_init, "ble init", "ble init", false);

#endif

#if defined(CFG_HOST)
static int ble_adv_start(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: adv_type | adv_interval_min | adv_interval_max | adv_channel_map\r\n");
		return CMD_RET_FAILURE;
	}
	uint8_t status = BT_ERROR_NO_ERROR;
    ECR_BLE_DATA_T  padv_data = {0};
    ECR_BLE_DATA_T  prsp_data = {0};
	
	ECR_BLE_GAP_ADV_PARAMS_T adv_param = {0};
	adv_param.adv_type = strtoul(argv[1],NULL,0);
	adv_param.adv_interval_min = strtoul(argv[2],NULL,0);
	adv_param.adv_interval_max = strtoul(argv[3],NULL,0);
	adv_param.adv_channel_map  = strtoul(argv[4],NULL,0);

	ble_cli_set_adv_data(&padv_data);
	ble_cli_set_scan_rsp_data(&prsp_data);
	ecr_ble_adv_param_set(&padv_data, &prsp_data);

	status = ecr_ble_gap_adv_start(&adv_param);

	if(BT_ERROR_INVALID_PARAM == status)
	{
		os_printf(LM_APP, LL_INFO, "Invalid arguments\n");
		return CMD_RET_FAILURE;
	}
	else if(BT_ERROR_NO_ERROR != status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, start_adv, ble_adv_start, "slave start adv", "ble start_adv", true);

static int ble_adv_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	uint8_t status = BT_ERROR_NO_ERROR;
	status = ecr_ble_gap_adv_stop();

	if(BT_ERROR_NO_ERROR != status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, stop_adv, ble_adv_stop, "slave stop adv", "ble stop_adv", true);
#endif

#if defined(CFG_HOST)
/*GATT  Related*/

static int ble_gatts_set_value(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: connect_handle | char_handle | data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t connect_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0);

    if(gapc_get_conidx(connect_handle) == ECR_BLE_INVALID_CONIDX)
    {
        os_printf(LM_BLE, LL_INFO, "Device is not connected, connect_handle set to 0\n");
    }

	uint8_t str_len = strlen(argv[3]);
	uint8_t *data = (uint8_t *)argv[3];
	
	ecr_ble_gatts_value_set(connect_handle, char_handle, data, str_len);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, set_char_value, ble_gatts_set_value, "slave set own char value", "ble set_char_value", false);

static int ble_gatts_get_value(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: connect_handle | char_handle | data_len\n");
		return CMD_RET_FAILURE;
	}
	uint16_t connect_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0);
	uint16_t len  =  strtoul(argv[3],NULL,0); 

    if(gapc_get_conidx(connect_handle) == ECR_BLE_INVALID_CONIDX)
    {
        os_printf(LM_BLE, LL_INFO, "Device is not connected, connect_handle set to 0\n");
    }

	uint8_t * data = os_malloc(sizeof(uint8_t)*len+1);//+1 is for '\0'
	if (data)
	{
	    memset(data, 0x00, sizeof(uint8_t)*len+1);
	}
	else
	{
	    os_printf(LM_CMD, LL_ERR, "malloc fail\n");
	    return CMD_RET_FAILURE;
	}
	ecr_ble_gatts_value_get(connect_handle, char_handle, data, len);

	os_printf(LM_CMD, LL_INFO, "data: %s\n", data);
	os_free(data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, get_char_value, ble_gatts_get_value, "slave get own char value", "ble get_char_value", false);

static int ble_gatts_notify(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_handle | char_handle | data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0); 

	if(char_handle > ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "Char_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;	
	}
	if(gapc_get_conidx(conn_handle) == ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	ecr_ble_gatts_value_notify(conn_handle, char_handle, (uint8_t *)argv[3], strlen(argv[3]));

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, notify, ble_gatts_notify, "slave notify to master", "ble notify", false);

static int ble_gatts_indicate(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_handle | char_handle | data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0); 

	if(char_handle > ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "Char_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;	
	}
	
	if(gapc_get_conidx(conn_handle) == ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}

	ecr_ble_gatts_value_indicate(conn_handle, char_handle, (uint8_t *)argv[3], strlen(argv[3]));

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, indicate, ble_gatts_indicate, "slave indicate to master", "ble indicate", false);

static int ble_irk_set(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 3)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | irk_len | irk\n");
		return CMD_RET_FAILURE;
	}
	uint16_t irk_len = strtoul(argv[1],NULL,0);

	uint8_t * irk = os_malloc(sizeof(uint8_t)*irk_len);

	for (int i = 0; i < irk_len; i++)
    {
        irk[i] = hex2num(*(argv[4]+i*3)) * 0x10 + hex2num(*(argv[4]+i*3+1));
    }	

	ecr_ble_set_irk(irk);

	os_free(irk);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, set_irk, ble_irk_set, "set irk", "ble set_irk", false);


static int ble_reset_all(cmd_tbl_t *t, int argc, char *argv[])
{
	ecr_ble_reset();

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, reset_all, ble_reset_all, "reset all", "ble reset_all", false);


static int ble_char_handle_show(cmd_tbl_t *t, int argc, char *argv[])
{
	for(int svc_idx = 0; svc_idx < COMMON_SERVICE_MAX_NUM; svc_idx++)
	{
		os_printf(LM_BLE, LL_INFO, "=============service %d=============\n", svc_idx);
		os_printf(LM_BLE, LL_INFO, "service handle: %d\n", ecr_ble_common_service[svc_idx].handle);

		ECR_BLE_CHAR_PARAMS_T* p_char = ecr_ble_common_service[svc_idx].p_char;
		for(int char_idx = 0; char_idx < ecr_ble_common_service[svc_idx].char_num; char_idx++)
		{
			os_printf(LM_BLE, LL_INFO, "char %d, char handle: %d\n", char_idx, p_char[char_idx].handle);
		}
		os_printf(LM_BLE, LL_INFO, "====================================\n");
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, show_char_handle, ble_char_handle_show, "slave show char handle", "ble show_char_handle", false);


#endif //CFG_HOST


#if defined(CFG_HOST) && defined(CONFIG_BLE_TUYA_ADAPTER_TEST)
/**tuya bt adapter test */
static ty_bt_param_t bt_init_para = {0};

static uint8_t adv_msg[] = {2,1,6,3,2,1,0xa2,4,0x16,1,0xa2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t rsp_msg[] = {4,9,'C','L','I',0x19,0xff,0xd0,7,0x80,3,0,0,0xc,0,0x5f,0x9f,0x6e,3,0x22,0,0x95,0xd8,0xa,0xf2,0x87,0x2e,0xda,0xac,0x83,0x7e};

static uint8_t adv_msg_new[] = {0x02,0x01,0x06,0x03,0x02,0x01,0xa2,0x14,0x16,0x01,0xa2,0x01,0x6b,0x65,0x79,0x6d,0x35,0x35,0x37,0x6e,0x71,0x77,0x33,0x70,0x38,0x70,0x37,0x6d};
static uint8_t rsp_msg_new[] = {0x03,0x09,0x54,0x59,0x19,0xff,0xd0,0x07,0x09,0x03,0x00,0x00,0x0c,0x00,0x1b,0xf2,0x87,0x8b,0x9c,0x3e,0xf7,0x4c,0x44,0x2b,0x95,0x5a,0x63,0x1d,0xd0,0xa8};

void tuya_cli_debug_hex_dump(char *title, uint8_t width, uint8_t *buf, uint16_t size)
{
	int i = 0;

	if(width < 64)
		width = 64;

	os_printf(LM_CMD, LL_INFO, "%s %d\r\n", title, size);
	for (i = 0; i < size; i++) {
		os_printf(LM_CMD, LL_INFO, "%02x ", buf[i]&0xFF);
		if ((i+1)%width == 0) {
			os_printf(LM_CMD, LL_INFO,"\r\n");
		}
	}
	os_printf(LM_CMD, LL_INFO,"\r\n");
}

static void __ble_recv_data_proc(tuya_ble_data_buf_t *buf)
{
	if (!buf) {
		return;
	}
	if (!buf->data && !buf->len) {
		return;
	}
	tuya_cli_debug_hex_dump("ble_slave recv", 16, buf->data, buf->len);
}

static void ble_data_recv_cb(int id, ty_bt_cb_event_t event, tuya_ble_data_buf_t *buf)
{
	os_printf(LM_CMD, LL_INFO, "ble_data_recv_cb, id:%d, event:%d \n", id, event);
	switch (event) {
	case TY_BT_EVENT_DISCONNECTED: {
		break;
	}
	case TY_BT_EVENT_CONNECTED: {
		break;
	}
	case TY_BT_EVENT_RX_DATA: {
		__ble_recv_data_proc(buf);
		break;
	}
	case TY_BT_EVENT_ADV_READY: {
		os_printf(LM_CMD, LL_INFO,"RET_BLE_INIT_0_SUCCESS\r\n");
		break;
	}
	}

}

static int tuya_ble_init(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
	bt_init_para.mode = TY_BT_MODE_PERIPHERAL;

	snprintf(bt_init_para.name, DEVICE_NAME_LEN, "cli-%02x-%02x", 00, 00);
	os_printf(LM_CMD, LL_INFO, "bt apperance name:%s\n", bt_init_para.name);
	
	bt_init_para.link_num = 1;
	bt_init_para.cb = ble_data_recv_cb;

	bt_init_para.adv.data = adv_msg;
	bt_init_para.adv.len = sizeof(adv_msg)/sizeof(adv_msg[0]);
	bt_init_para.scan_rsp.data = rsp_msg;
	bt_init_para.scan_rsp.len = sizeof(rsp_msg)/sizeof(rsp_msg[0]);

	tuya_cli_debug_hex_dump("bt adv", 16, bt_init_para.adv.data, bt_init_para.adv.len);
	tuya_cli_debug_hex_dump("bt scan_rsp", 16, bt_init_para.scan_rsp.data, bt_init_para.scan_rsp.len);
	ret = tuya_os_adapt_bt_port_init(&bt_init_para);
	if(ret != 0){
		os_printf(LM_CMD, LL_INFO,"ble init fail!\r\n");
		return CMD_RET_FAILURE;
	} else {
		os_printf(LM_CMD, LL_INFO,"ble init success\r\n");
		return CMD_RET_SUCCESS;
	}
}
CLI_CMD(tuya_ble_init, tuya_ble_init,  "tuya_ble_init",  "");

static int tuya_ble_deinit(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;

	os_printf(LM_CMD, LL_INFO,"ble deinit start\r\n");
	ret = tuya_os_adapt_bt_port_deinit();
	if(ret != 0){
		os_printf(LM_CMD, LL_INFO,"ble deinit fail!\r\n");
		return CMD_RET_FAILURE;
	} else {
		os_printf(LM_CMD, LL_INFO,"ble deinit success\r\n");
		return CMD_RET_SUCCESS;
	}
}
CLI_CMD(tuya_ble_deinit, tuya_ble_deinit,  "tuya_ble_deinit",  "");

static int tuya_ble_send(cmd_tbl_t *t, int argc, char *argv[])
{
	uint8_t len = atoi(argv[1]);
	char * data = "tuya_ble_send_test";
	int count = atoi(argv[2]);
	size_t delay = atoi(argv[3]);

	for(int i = 0; i < count; i++)
	{
		tuya_os_adapt_bt_send((unsigned char *)data, len);
		os_msleep(delay);
	}

	return CMD_RET_SUCCESS;
}
CLI_CMD(tuya_ble_send, tuya_ble_send,  "tuya_ble_send [datalen] [num] [interval(ms)]",	"tuya_ble_send [datalen] [num] [interval(ms)]");

static int tuya_ble_scan(cmd_tbl_t *t, int argc, char *argv[])
{
	if (strcmp(argv[1], "target") == 0) {
		ty_bt_scan_info_t ble_scan;
		memset(&ble_scan, 0x00, sizeof(ble_scan));
		strncpy(ble_scan.name, argv[2], DEVICE_NAME_LEN);
		ble_scan.scan_type = TY_BT_SCAN_BY_NAME;
		ble_scan.timeout_s = 5;
		os_printf(LM_CMD, LL_INFO,"start bt scan:%s", ble_scan.name);
		int op_ret = tuya_os_adapt_bt_assign_scan(&ble_scan);
		if (0 != op_ret) {
			os_printf(LM_CMD, LL_INFO,"ble scan fail:%d\r\n", op_ret);
			return CMD_RET_FAILURE;
		} else {
			os_printf(LM_CMD, LL_INFO,"ble scan success:%d\r\n", ble_scan.rssi);
			return CMD_RET_SUCCESS;
		}
	}

	return CMD_RET_FAILURE;

}
CLI_CMD(tuya_ble_scan, tuya_ble_scan,  "tuya_ble_scan target [name]",  "tuya_ble_scan target [name]");

static int tuya_ble_reset_adv(cmd_tbl_t *t, int argc, char *argv[])
{
	tuya_ble_data_buf_t new_adv = {
		.data = adv_msg_new,
		.len = sizeof(adv_msg_new)/sizeof(adv_msg_new[0])
	};
	tuya_ble_data_buf_t new_scan_rsp = {
		.data = rsp_msg_new,
		.len = sizeof(rsp_msg_new)/sizeof(rsp_msg_new[0])
	};
	tuya_cli_debug_hex_dump("bt update adv", 16, new_adv.data, new_adv.len);
	tuya_cli_debug_hex_dump("bt update scan_rsp", 16, new_scan_rsp.data, new_scan_rsp.len);
	int ret = tuya_os_adapt_bt_reset_adv(&new_adv, &new_scan_rsp);
	if(ret)
	{
		os_printf(LM_CMD, LL_INFO,"ble reset adv error\r\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		os_printf(LM_CMD, LL_INFO,"ble reset adv success\r\n");
		return CMD_RET_SUCCESS;
	}

}
CLI_CMD(tuya_ble_reset_adv, tuya_ble_reset_adv,  "tuya_ble_reset_adv",	"tuya_ble_reset_adv");

static int tuya_ble_get_rssi(cmd_tbl_t *t, int argc, char *argv[])
{
	signed char rssi = 0;
	int ret = tuya_os_adapt_bt_get_rssi(&rssi);
	if(ret)
	{
		os_printf(LM_CMD, LL_INFO,"ble get rssi error\r\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		os_printf(LM_CMD, LL_INFO,"ble rssi is: %d\r\n", rssi);
		return CMD_RET_SUCCESS;
	}

}
CLI_CMD(tuya_ble_get_rssi, tuya_ble_get_rssi,  "tuya_ble_get_rssi",  "tuya_ble_get_rssi");

static int tuya_ble_start_adv(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = tuya_os_adapt_bt_start_adv();
	if(ret)
	{
		return CMD_RET_FAILURE;
	}
	else
	{
		return CMD_RET_SUCCESS;
	}

}
CLI_CMD(tuya_ble_start_adv, tuya_ble_start_adv,  "tuya_ble_start_adv",	"tuya_ble_start_adv");

static int tuya_ble_stop_adv(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = tuya_os_adapt_bt_stop_adv();
	if(ret)
	{
		return CMD_RET_FAILURE;
	}
	else
	{
		return CMD_RET_SUCCESS;
	}

}
CLI_CMD(tuya_ble_stop_adv, tuya_ble_stop_adv,  "tuya_ble_stop_adv",  "tuya_ble_stop_adv");

static int tuya_ble_disconnect(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = tuya_os_adapt_bt_gap_disconnect();
	if(ret)
	{
		return CMD_RET_FAILURE;
	}
	else
	{
		return CMD_RET_SUCCESS;
	}

}
CLI_CMD(tuya_ble_disconnect, tuya_ble_disconnect,  "tuya_ble_disconnect",  "tuya_ble_disconnect");

#endif	//(CFG_HOST) && (CONFIG_BLE_TUYA_ADAPTER)

#if defined(CONFIG_BLE_TUYA_HCI_ADAPTER_TEST) && defined(CONFIG_COMP_HCI_ADAPT)
#include "tuya_ble_hci.h"
#include "hci_interface.h"
int tuya_hci_rx_cmd_test(uint8_t *cmd, void *arg)
{
	os_printf(LM_CMD, LL_INFO,"%s\r\n", __func__);
	return 0;
}

int tuya_hci_rx_acl_test(void *acl_pkt, void *arg)
{
	os_printf(LM_CMD, LL_INFO,"%s\r\n", __func__);
	return 0;
}

static int tuya_hci_init(cmd_tbl_t *t, int argc, char *argv[])
{
	tuya_ble_hci_init();
	tuya_ble_hci_cfg_hs(tuya_hci_rx_cmd_test,NULL,tuya_hci_rx_acl_test,NULL);

	return CMD_RET_SUCCESS;
}
CLI_CMD(tuya_hci_init, tuya_hci_init,  "",  "");

#define PKT_RECV_SIZE 128
static uint8_t g_pkt[PKT_RECV_SIZE];
static int g_pkt_len;

static void save_hci_cmd(char * cmd, int cmd_len)
{
	int i = 0;

	char high = 0, low = 0;
	char *p = cmd;

	while(i < cmd_len)
	{
		high = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
    	low = (*(++ p) > '9' && ((*p <= 'F') || (*p <= 'f'))) ? *(p) - 48 - 7 : *(p) - 48;
		g_pkt[i] = ((high & 0x0f) << 4) | (low & 0x0f);
		i++;
		p++;
	}
}
static int tuya_hci_recv(cmd_tbl_t *t, int argc, char *argv[])
{
	uint8_t cmd_len = 0;
	if(argc==1)
	{
		os_printf(LM_CMD, LL_ERR, "input: hci [arg], such as: hci 101f2000\n");
		return CMD_RET_FAILURE;
	}

	if (strlen(argv[1]) % 2 != 0)
	{
		os_printf(LM_CMD, LL_ERR, "hci arg error\n");
		return CMD_RET_FAILURE;
	}

	cmd_len = strlen(argv[1])/2;

	memset(g_pkt,0,sizeof(g_pkt));

	//save cmd length
	g_pkt_len = cmd_len;

	save_hci_cmd(argv[1], cmd_len);

	os_printf(LM_CMD, LL_INFO,"--test hci type-- %d\r\n", g_pkt[0]);
	switch(g_pkt[0])
    {
    	case BLE_HCI_CMD_MSG_TYPE:
    	{
			tuya_ble_hci_hs_cmd_tx(&g_pkt[1],cmd_len-1);
		}
		break;
        case BLE_HCI_ACL_MSG_TYPE:
        case BLE_HCI_SYNC_MSG_TYPE:
        {
			tuya_ble_hci_hs_acl_tx(&g_pkt[1],cmd_len-1);
		}
		break;
        case BLE_HCI_EVT_MSG_TYPE:
        {
			
        }
        break;

        default:
        {
        
        }
        break;
    }
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(tuya_hci_rx, tuya_hci_recv,  "",  "");

#endif //

#if defined(CFG_HOST)
static int ble_start_scan(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: scan_type | scan_interval | scan_window | scan_duration\n");
		return CMD_RET_FAILURE;
	}


	uint8_t status = BT_ERROR_NO_ERROR;
	ECR_BLE_GAP_SCAN_PARAMS_T ECR_BLE_GAP_SCAN_PARAMS ={0};
	
	ECR_BLE_GAP_SCAN_PARAMS.scan_type= strtoul(argv[1],NULL,0);
	ECR_BLE_GAP_SCAN_PARAMS.dup_filt_pol = ECR_DUP_FILT_EN;
	ECR_BLE_GAP_SCAN_PARAMS.scan_prop= ECR_BLE_SCAN_PROP_PHY_1M_BIT|ECR_BLE_SCAN_PROP_ACTIVE_1M_BIT;

	ECR_BLE_GAP_SCAN_PARAMS.interval = strtoul(argv[2],NULL,0);
	ECR_BLE_GAP_SCAN_PARAMS.window = strtoul(argv[3],NULL,0);
	ECR_BLE_GAP_SCAN_PARAMS.duration = strtoul(argv[4],NULL,0)*100;
	ECR_BLE_GAP_SCAN_PARAMS.period = 0;
	ECR_BLE_GAP_SCAN_PARAMS.scan_channel_map = 0x07;

	status = ecr_ble_gap_scan_start(&ECR_BLE_GAP_SCAN_PARAMS);
	
	if(BT_ERROR_INVALID_PARAM == status)
	{
		os_printf(LM_APP, LL_INFO, "Invalid arguments\n");
		return CMD_RET_FAILURE;
	}
	else if(BT_ERROR_NO_ERROR != status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, start_scan, ble_start_scan, "master start scan", "ble start_scan", false);

static int ble_stop_scan(cmd_tbl_t *h, int argc, char *argv[])
{
	uint8_t status = BT_ERROR_NO_ERROR;
	status = ecr_ble_gap_scan_stop();

	if(BT_ERROR_NO_ERROR != status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, stop_scan, ble_stop_scan, "master stop scan", "ble stop_scan", false);


static int ble_start_connect(cmd_tbl_t *h, int argc, char *argv[])
{	
	ECR_BLE_GAP_CONN_PARAMS_T ECR_BLE_GAP_CONN_PARAMS = {0};
	ECR_BLE_GAP_ADDR_T p_peer_addr={0};
	if(argc!=8)
	{
		os_printf(LM_CMD,LL_INFO,"Input ERROR!!!,please input:type|addr|conn_interval_min|conn_interval_max|conn_latency|conn_sup_timeout|conn_establish_timeout\n");
		return CMD_RET_FAILURE;
	}
	
	uint8_t status = BT_ERROR_NO_ERROR;
	p_peer_addr.type = strtoul(argv[1],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_min = strtoul(argv[3],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_max = strtoul(argv[4],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_latency =strtoul(argv[5],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_sup_timeout = strtoul(argv[6],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_establish_timeout = strtoul(argv[7],NULL,0)*100;

	for(int i=0;i<=5;i++)
	{
		p_peer_addr.addr[5-i]=hex2num(*(argv[2]+i*3))*0x10+hex2num(*(argv[2]+i*3+1));
	}

	status = ecr_ble_gap_connect(&p_peer_addr, &ECR_BLE_GAP_CONN_PARAMS);
	
	if(BT_ERROR_INVALID_PARAM ==status)
	{
		os_printf(LM_APP, LL_INFO, "Invalid arguments\n");
		return CMD_RET_FAILURE;
	}
	else if(BT_ERROR_NO_ERROR != status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
	 
}
CLI_SUBCMD_F(ble, start_connect, ble_start_connect, "master start connect", "ble start_connect", false);

static int ble_cancel_connect(cmd_tbl_t *h, int argc, char *argv[])
{
	ecr_ble_gap_connect_cancel();

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, cancel_connect, ble_cancel_connect, "cancel connect", "ble cancel_connect", false);


static int ble_start_disconnect(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_handle\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint8_t conn_idx = gapc_get_conidx(conn_handle);

	if(conn_idx == 0xFF)
	{
		os_printf(LM_CMD,LL_INFO,"Input ERROR!!!\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gap_disconnect(conn_handle, BT_ERROR_CON_TERM_BY_LOCAL_HOST);
	}
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, start_disconnect, ble_start_disconnect, "disconnect link", "ble start_disconnect", false);

static int ble_connect_param_update(cmd_tbl_t *h, int argc, char *argv[])
{
	uint8_t status = BT_ERROR_NO_ERROR;
	
	if(argc != 6)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_handle|conn_interval_min|conn_interval_max|conn_latency|conn_sup_timeout\n");
		return CMD_RET_FAILURE;
	}
	ECR_BLE_GAP_CONN_PARAMS_T ECR_BLE_GAP_CONN_PARAMS ;
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_min = strtoul(argv[2],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_max = strtoul(argv[3],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_latency = strtoul(argv[4],NULL,0);
	ECR_BLE_GAP_CONN_PARAMS.conn_sup_timeout = strtoul(argv[5],NULL,0);
	
	status = ecr_ble_connect_param_update(conn_handle, &ECR_BLE_GAP_CONN_PARAMS);

	if(BT_ERROR_INVALID_PARAM == status)
	{
		os_printf(LM_APP, LL_INFO, "Invalid arguments\n");
		return CMD_RET_FAILURE;
	}
	else if(BT_ERROR_COMMAND_DISALLOWED == status)
	{
		os_printf(LM_APP, LL_INFO, "Request not allowed in current state\n");
		return CMD_RET_FAILURE;
	}
	else if(BT_ERROR_OPERATION_CANCELED_BY_HOST == status)
	{
		//BT_ERROR_OPERATION_CANCELED_BY_HOST
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, update_conn_params, ble_connect_param_update, "connect parameters update", "ble update_conn_params", false);


static int ble_get_conn_rssi(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_idx\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint8_t conn_idx = gapc_get_conidx(conn_handle);
	if(conn_idx==0xFF)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_connect_rssi_get(conn_handle);
	}
	return CMD_RET_SUCCESS;
}

CLI_SUBCMD_F(ble, get_conn_rssi, ble_get_conn_rssi, "get connection rssi", "ble get_conn_rssi", false);

static int ble_gattc_all_service_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=2)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:con_handle\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	
	uint8_t conn_idx = gapc_get_conidx(conn_handle);
	if(conn_idx==0xFF)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gattc_all_service_discovery(conn_handle);
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, discover_all_serv, ble_gattc_all_service_discovery, "master discover service", "ble discover_all_serv", false);

static int ble_gattc_all_char_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:conn_handle start_handle end_handle\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t start_handle = strtoul(argv[2],NULL,0);
	uint16_t end_handle = strtoul(argv[3],NULL,0);

	if(start_handle==0x00 || end_handle<start_handle)
	{
		os_printf(LM_OS, LM_BLE, "Start_handle or end_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;
	}
	
	uint8_t conn_idx = gapc_get_conidx(conn_handle);
	if(conn_idx==0xFF)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gattc_all_char_discovery(conn_handle, start_handle, end_handle);
	}
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, discover_all_char, ble_gattc_all_char_discovery, "master discover char", "ble discover_all_char", false);

static int ble_gattc_char_desc_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:conn_handle start_handle end_handle\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t start_handle = strtoul(argv[2],NULL,0);
	uint16_t end_handle = strtoul(argv[3],NULL,0);
	
	if(start_handle==0x00 || end_handle<start_handle)
	{
		os_printf(LM_OS, LM_BLE, "Start_handle or end_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;
	}
	
	uint8_t conn_idx = gapc_get_conidx(conn_handle);
	if(conn_idx==0xFF)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gattc_char_desc_discovery(conn_handle, start_handle, end_handle);
	}

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, discover_char_desc, ble_gattc_char_desc_discovery, "master discover char descriptor", "ble discover_char_desc", false);


static int ble_gattc_exchange_mtu_req(cmd_tbl_t *h, int argc, char *argv[])
{
	if(argc !=3)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:conn_handle client_mtu\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t client_mtu = strtoul(argv[2],NULL,0);
	
	uint8_t conn_idx = gapc_get_conidx(conn_handle);
	
	if(conn_idx == 0xFF)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else if(client_mtu > 512 || client_mtu < 23)
	{
		os_printf(LM_OS, LM_BLE, "The mtu size is not support\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gattc_exchange_mtu_req(conn_handle, client_mtu);
	}
	 
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble, exchange_mtu, ble_gattc_exchange_mtu_req, "master exchange mtu with slave", "ble exchange_mtu", false);


static int ble_gattc_read(cmd_tbl_t *h, int argc, char *argv[])
{
	if(argc !=3)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:conn_handle | char_handle\n");
		return CMD_RET_FAILURE;
	}
	
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0);
	
	if(char_handle > ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "Char_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;
	}
	
	if(gapc_get_conidx(conn_handle) == ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	else
	{
		ecr_ble_gattc_read(conn_handle, char_handle);
	}

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD_F(ble,read, ble_gattc_read, "master read data from slave", "ble read", false);


static int ble_gattc_write(cmd_tbl_t *h, int argc, char *argv[])
{		
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:con_handle | char_handle | char_value \n");
		return CMD_RET_FAILURE;
	}
	

	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0);
	
	if(char_handle > ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "Char_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;	
	}
	
	if(gapc_get_conidx(conn_handle) == ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}
	
	os_printf(LM_OS, LM_BLE, "Input Value=%s\n",argv[3]);
	ecr_ble_gattc_write(conn_handle, char_handle, (uint8_t *)argv[3], strlen(argv[3]));

	return CMD_RET_SUCCESS;
}

CLI_SUBCMD_F(ble,write_with_rsp, ble_gattc_write, "master write data with response", "ble write_with_rsp", false);

static int ble_gattc_write_without_rsp(cmd_tbl_t *h, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_OS, LM_BLE, "Input ERROR!!!, Please input:con_handle | char_handle | char_value\n");
		return CMD_RET_FAILURE;
	}
	
	uint16_t conn_handle = strtoul(argv[1],NULL,0);
	uint16_t char_handle = strtoul(argv[2],NULL,0);
	
	if(char_handle>ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "Char_handle input ERROR!!!\n");
		return CMD_RET_FAILURE;	
	}
	
	if(gapc_get_conidx(conn_handle) == ECR_BLE_INVALID_CONIDX)
	{
		os_printf(LM_OS, LM_BLE, "The device is not connected\n");
		return CMD_RET_FAILURE;
	}

	os_printf(LM_OS, LM_BLE, "Input Value=%s\n",argv[3]);
	
	ecr_ble_gattc_write_without_rsp(conn_handle, char_handle, (uint8_t *)argv[3], strlen(argv[3]));
	return CMD_RET_SUCCESS;
}

CLI_SUBCMD_F(ble,write_without_rsp, ble_gattc_write_without_rsp, "master write data without response", "ble write_without_rsp", false);

#if defined (CONFIG_BLE_EXAMPLES_NETCFG_SLAVE)
#define ECR_NETCFG_NAME ("ecr_netcfg")
#endif
#define ECR_BLE_CMD_NAME ("ecr_ble_cmd")
CLI_CMD(ble, NULL, "displays detail information of the ble option", "ble <XXX>"); 
static void ble_subcmd_flage_all_set(cmd_tbl_t *parent, bool f)
{
	cmd_tbl_t *t;

	for (t = cli_subcmd_start(ble); (t < cli_subcmd_end(ble)); t++)
	{
		t->f->used = f;
	}
}
static int ble_soft_switch_mode(cmd_tbl_t *h, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "Input ERROR!!!, Please input: ble_switch_mode [ecr_netcfg/ecr_ble_cmd]\n");
		return CMD_RET_FAILURE;
	}

	if(0 == strncmp(ECR_BLE_CMD_NAME, argv[1], strlen(ECR_BLE_CMD_NAME)))
	{
		os_printf(LM_CMD, LL_INFO, "BLE switch cmd mode success\n");
		cmd_tbl_t *t = CLI_CMD_GET(ble);
		ble_subcmd_flage_all_set(t, true);
		ecr_ble_reset();
		ecr_ble_gap_callback_register(ecr_ble_gap_event_cb);
		ecr_ble_gatt_callback_register(ecr_ble_gatt_event_cb);
		return CMD_RET_SUCCESS;
	}

	#if defined (CONFIG_BLE_EXAMPLES_NETCFG_SLAVE)
	if(0 == strncmp(ECR_NETCFG_NAME, argv[1], strlen(ECR_NETCFG_NAME)))
	{
		os_printf(LM_CMD, LL_INFO, "BLE switch netcfg mode success\n");
		cmd_tbl_t *t = CLI_CMD_GET(ble);
		cmd_tbl_t *sub_t = NULL;
		ble_subcmd_flage_all_set(t, false);
		sub_t = CLI_SUBCMD_GET(ble, start_adv);
		sub_t->f->used=true;
		sub_t = CLI_SUBCMD_GET(ble, stop_adv);
		sub_t->f->used=true;
		sub_t = CLI_SUBCMD_GET(ble, get_addr);
		sub_t->f->used=true;
		sub_t = CLI_SUBCMD_GET(ble, set_addr);
		sub_t->f->used=true;
		sub_t = CLI_SUBCMD_GET(ble, get_dev_name);
		sub_t->f->used=true;
		sub_t = CLI_SUBCMD_GET(ble, set_dev_name);
		sub_t->f->used=true;
		
		ecr_ble_reset();
		BLE_APPS_INIT();
		return CMD_RET_SUCCESS;
	}
	#endif

	os_printf(LM_CMD, LL_ERR, "Input ERROR!!!, Please input correct mode name\n");
	return CMD_RET_FAILURE;
}
CLI_CMD(ble_switch_mode, ble_soft_switch_mode, "ble software switch mode", "ble_switch_mode [ecr_netcfg/ecr_ble_cmd]");

#endif ///defined(CFG_HOST)
