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
//#include "ecr_bt_task.h"
#include "bluetooth_err.h"
#if defined (CONFIG_NV)
#include "system_config.h"
#endif

#if defined(CFG_HOST) && defined(CONFIG_BLE_TUYA_ADAPTER_TEST)
#include "ble_tuya_adapter_test.h"
#endif

#define peer_device_addr             "\xaa\xff\x33\x22\x11\x00"
#define peer_device_addr_len          (6)


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
CLI_CMD(bwrite, ble_writereg, "ble write register", "bwrite [reg]");

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

CLI_CMD(bread, ble_readreg, "ble read register", "bread [reg]");

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

CLI_CMD(ble_auto_mode, ble_auto_mode, "ble auto mode", "brx_dump");

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
CLI_CMD(ble_debug, ble_debug_mode,	"ble_debug",  "ble_debug [code]");

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
CLI_CMD(ble_rssi, ble_read_rssi,  "ble_rssi",  "ble_rssi");


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
CLI_CMD(pta, pta_test, "pta test", "pta [start/stop]");
#endif // CONFIG_ECR6600_WIFI

#if defined(CFG_HOST) && defined(CONFIG_NV)
//hongsai

#define BLE_DEVICE_NAME_MAX_LEN		(18)
#define BLE_BD_ADDR_LEN				(6)

static int set_ble_dev_name(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "ARG NUM ERROR!!!\r\n");
		return CMD_RET_FAILURE;
	}

	char temp_name[BLE_DEVICE_NAME_MAX_LEN] = {'\0'};
	memcpy(temp_name, argv[1], strlen(argv[1]));

	if(EF_NO_ERR == hal_system_set_config(CUSTOMER_NV_BLE_DEVIVE_NAME, temp_name, sizeof(temp_name)))
	{
		return CMD_RET_SUCCESS;
	}
	else
	{
		return CMD_RET_FAILURE;
	}

	return 0;
}
CLI_CMD(set_ble_dev_name, set_ble_dev_name, "set_ble_dev_name", "set_ble_dev_name [dev_name](<=18)");

static int get_ble_dev_name(cmd_tbl_t *t, int argc, char *argv[])
{
	char temp_name[BLE_DEVICE_NAME_MAX_LEN] = {'\0'};

	int ret = hal_system_get_config(CUSTOMER_NV_BLE_DEVIVE_NAME, temp_name, sizeof(temp_name));
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
CLI_CMD(get_ble_dev_name, get_ble_dev_name, "get_ble_dev_name", "get_ble_dev_name");

/**
static int set_ble_mac_addr(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "ARG NUM ERROR!!!\r\n");
		return CMD_RET_FAILURE;
	}
	
	char * macstr = argv[1];
	uint8_t temp_mac[BLE_BD_ADDR_LEN] = {0};
	macstr = argv[1];
	
	for (int i=0; i<BLE_BD_ADDR_LEN; i++)
	{
		temp_mac[i] = hex2num(macstr[(BLE_BD_ADDR_LEN - 1 - i) * 3]) * 0x10 + hex2num(macstr[(BLE_BD_ADDR_LEN - 1 - i) * 3 + 1]);
	}
	os_printf(LM_CMD, LL_INFO, "mac = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
		temp_mac[5],temp_mac[4],temp_mac[3],temp_mac[2],temp_mac[1],temp_mac[0]);
	
	if(EF_NO_ERR == hal_system_set_config(CUSTOMER_NV_BLE_BD_ADDR, temp_mac, sizeof(temp_mac)))
	{
		return CMD_RET_SUCCESS;
	}
	else
	{
		return CMD_RET_FAILURE;
	}
}
CLI_CMD(set_ble_mac_addr, set_ble_mac_addr, "set_ble_mac_addr", "set_ble_mac_addr [mac_addr]");

static int get_ble_mac_addr(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned char temp_mac[BLE_BD_ADDR_LEN] = {'\0'};

	int ret = hal_system_get_config(CUSTOMER_NV_BLE_BD_ADDR, temp_mac, sizeof(temp_mac));
	if (ret)
	{
		os_printf(LM_CMD, LL_INFO, "ble_bd_addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
		temp_mac[5],temp_mac[4],temp_mac[3],temp_mac[2],temp_mac[1],temp_mac[0]);
		return CMD_RET_SUCCESS;
	}
	else
	{
		return CMD_RET_FAILURE;
	}
}
CLI_CMD(get_ble_mac_addr, get_ble_mac_addr, "get_ble_mac_addr", "get_ble_mac_addr");
*/

#endif

#if defined(CFG_HOST) && defined(CONFIG_BLE_TEST_NETCFG)

static ECR_BLE_GATTS_PARAMS_T          ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T        ecr_ble_common_service[ECR_BLE_GATT_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T           ecr_ble_common_char[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};

#define  COMMON_SERVICE_MAX_NUM      (2)
#define  COMMON_CHAR_MAX_NUM         (3)
 
#define  BLE_CMD_SERVICE_UUID                 (0x180b)
#define  BLE_CMD_WRITE_CHAR_UUID              (0x2a2d)
#define  BLE_CMD_READ_CHAR_UUID               (0x2a2e)
#define  BLE_CMD_NOTIFY_CHAR_UUID             (0x2a2f)

static uint8_t adv_data[]      = {2,1,6,3,2,1,0xa2,4,0x16,1,0xa2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t scan_rsp_data[] = {4,9,'C','L','I',0x19,0xff,0xd0,7,0x80,3,0,0,0xc,0,0x5f,0x9f,0x6e,3,0x22,0,0x95,0xd8,0xa,0xf2,0x87,0x2e,0xda,0xac,0x83,0x7e};
static uint8_t adv_data_update[]      = {0x02,0x01,0x06,0x03,0x02,0x01,0xa2,0x14,0x16,0x01,0xa2,0x01,0x6b,0x65,0x79,0x6d,0x35,0x35,0x37,0x6e,0x71,0x77,0x33,0x70,0x38,0x70,0x37,0x6d};
static uint8_t scan_rsp_data_update[] = {0x03,0x09,0x54,0x59,0x19,0xff,0xd0,0x07,0x09,0x03,0x00,0x00,0x0c,0x00,0x1b,0xf2,0x87,0x8b,0x9c,0x3e,0xf7,0x4c,0x44,0x2b,0x95,0x5a,0x63,0x1d,0xd0,0xa8};

static void ecr_ble_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
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

static void ecr_ble_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gatt_event_write\n");
			break;
		case ECR_BLE_GATT_EVT_READ_RX:
			os_printf(LM_CMD, LL_INFO, "ecr_ble_gatt_event_read_rx\n");
			break;
		default:
			break;
	}

}

static int ecr_ble_set_addr(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 3)
	{
		os_printf(LM_APP, LL_INFO, "Input ERROR!!!, Please input: Func | addr type | set addr");
		return CMD_RET_FAILURE;
	}
		
	ECR_BLE_GAP_ADDR_T dev_addr = {0};
	dev_addr.type = atoi(argv[1]);

	for (int i = 0; i < 6; i++)
    {
        dev_addr.addr[i] = hex2num(*(argv[2]+i*3)) * 0x10 + hex2num(*(argv[2]+i*3+1));
    }
	
	if(0 != ecr_ble_set_device_addr(&dev_addr))
	{
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_set_addr, ecr_ble_set_addr,  "ble_set_addr 0 AA:11:22:33:44:55",  "ble_set_addr 0 AA:11:22:33:44:55");

static int ecr_ble_get_addr(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_GAP_ADDR_T dev_addr = {0};

	if(ecr_ble_get_device_addr(&dev_addr))
	{
		return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_get_addr, ecr_ble_get_addr,  "ble_get_addr",  "ble_get_addr");

static int ecr_set_dev_config(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_DEV_CONFIG p_dev_config;

	#if (NVDS_SUPPORT)
	nvds_tag_len_t len;
	#endif //(NVDS_SUPPORT)
	
	p_dev_config.role = BLE_ROLE_ALL;
	
	/*Not sure data*/
	p_dev_config.renew_dur = 0x0096;
    p_dev_config.privacy_cfg = 0;

	#if (NVDS_SUPPORT)
	//uint ret = nvds_get(NVDS_TAG_BD_ADDRESS, &p_dev_config.addr.addr[0], &len, NULL);
	if(nvds_get(ECR_NVDS_TAG_BD_ADDRESS, &len, &p_dev_config->addr.addr[0]))
	{
		 if (p_dev_config.addr.addr[5] & 0xC0)
        {
            // Host privacy enabled by default
            p_dev_config.privacy_cfg |= GAPM_PRIV_CFG_PRIV_ADDR_BIT;
        }
	}
	#endif //(NVDS_SUPPORT)

	memset((void *)&p_dev_config.irk[0], 0x00, KEY_LEN);
	
	p_dev_config.pairing_mode = GAPM_PAIRING_DISABLE;
	p_dev_config.max_mps = 512;
	p_dev_config.max_mtu = 512;
	
	ecr_ble_set_dev_config(&p_dev_config);
	
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_set_dev_config, ecr_set_dev_config,  "set_dev_config",  "set_dev_config");

static int ecr_ble_init(cmd_tbl_t *t, int argc, char *argv[])
{
	ecr_ble_gap_callback_register(ecr_ble_gap_event_cb);
	ecr_ble_gatt_callback_register(ecr_ble_gatt_event_cb);

	ECR_BLE_GATTS_PARAMS_T *p_ble_service = &ecr_ble_gatt_service;
	p_ble_service->svc_num =  COMMON_SERVICE_MAX_NUM;
	p_ble_service->p_service = ecr_ble_common_service;

	/*First service add*/
	ECR_BLE_SERVICE_PARAMS_T *p_common_service = ecr_ble_common_service;
	p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
	p_common_service->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID;
	p_common_service->type     = ECR_BLE_UUID_SERVICE_PRIMARY;
	p_common_service->char_num = COMMON_CHAR_MAX_NUM;
	p_common_service->p_char   = ecr_ble_common_char;
	
	/*Add write characteristic*/
	ECR_BLE_CHAR_PARAMS_T *p_common_char = ecr_ble_common_char;
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
	p_common_service = &ecr_ble_common_service[1];
	p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_16;
	p_common_service->svc_uuid.uuid.uuid16 =  BLE_CMD_SERVICE_UUID;
	p_common_service->type     = ECR_BLE_UUID_SERVICE_PRIMARY;
	p_common_service->char_num = 2;
	p_common_service->p_char   = ecr_ble_common_char+3;

	/*Add write characteristic*/
	p_common_char = ecr_ble_common_char+3;
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = 0x2345;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE | ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	p_common_char++;

	/*Add Notify characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type   = ECR_BLE_UUID_TYPE_16;
	p_common_char->char_uuid.uuid.uuid16 = 0x1234;
	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	
	ecr_ble_gatts_service_add(p_ble_service, false);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_init, ecr_ble_init,  "ecr_ble_init",  "ecr_ble_init");

static int ecr_set_adv_scan_rsp_data(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_DATA_T p_adv_data, p_scan_rsp_data;
	p_adv_data.length = sizeof(adv_data)/sizeof(adv_data[0]);
	p_adv_data.p_data = adv_data;

	p_scan_rsp_data.length = sizeof(scan_rsp_data)/sizeof(scan_rsp_data[0]);
	p_scan_rsp_data.p_data = scan_rsp_data;
	
	ecr_ble_adv_param_set(&p_adv_data, &p_scan_rsp_data);

	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_set_adv_scan_rsp_data, ecr_set_adv_scan_rsp_data,  "set_adv_scan_rsp_data",  "set_adv_scan_rsp_data");

static int ecr_adv_scan_rsp_data_update(cmd_tbl_t *t, int argc, char *argv[])
{
	ECR_BLE_DATA_T p_adv, p_scan_rsp;
	
	p_adv.length = sizeof(adv_data_update)/sizeof(adv_data_update[0]);
	p_adv.p_data = adv_data_update;

	p_scan_rsp.length = sizeof(scan_rsp_data_update)/sizeof(scan_rsp_data_update[0]);
	p_scan_rsp.p_data = scan_rsp_data_update;
	
	ecr_ble_adv_param_update(&p_adv, &p_scan_rsp);
	
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_adv_scan_rsp_data_update, ecr_adv_scan_rsp_data_update,  "adv_scan_rsp_data_update",  "adv_scan_rsp_data_update");

static int ecr_ble_adv_start(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | adv_type | adv_interval_min | adv_interval_max | adv_channel_map\r\n");
		return CMD_RET_FAILURE;
	}
	ECR_BLE_GAP_ADV_PARAMS_T adv_param = {0};
	
	adv_param.adv_type = atoi(argv[1]);
	adv_param.adv_interval_min = atoi(argv[2]);
	adv_param.adv_interval_max = atoi(argv[3]);
	adv_param.adv_channel_map  = atoi(argv[4]);

	if(0 != ecr_ble_gap_adv_start(&adv_param))
	{
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_adv_start, ecr_ble_adv_start,  "ble_adv_start",  "ble_adv_start");

static int ecr_ble_adv_stop(cmd_tbl_t *t, int argc, char *argv[])
{
	ecr_ble_gap_adv_stop();
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_adv_stop, ecr_ble_adv_stop,  "ble_adv_stop",  "ble_adv_stop");

/*GATT  Related*/

static int ecr_ble_gatts_set_value(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | conn_idx | char_handle | data_len | data,data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_idx = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	uint16_t len  =  atoi(argv[3]); 

	uint8_t indx = 0;
	uint8_t *data = os_malloc(sizeof(uint8_t *)*len);
	uint8_t str_len = strlen(argv[4]);

	char value[str_len+1];
	memcpy(value, argv[4], str_len);
	value[str_len] = '\0';
	
	char *back_value = strtok(value,",");
    while(back_value != NULL)
    {
    	uint8_t in_len = strlen(back_value);
        data[indx] = hexToDec(back_value,in_len);
        back_value = strtok(NULL,",");
        indx++;
    }

	ecr_ble_gatts_value_set(conn_idx, char_handle, data, len);
	
	os_free(data);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_gatts_set_value, ecr_ble_gatts_set_value,  "gatts_set_value [char_handle]",  "gatts_set_value [char_handle]");

static int ecr_ble_gatts_get_value(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 4)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | conn_idx | char_handle | data_len\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_idx = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	uint16_t len  =  atoi(argv[3]); 

	uint8_t * data = os_malloc(sizeof(uint8_t)*len);

	ecr_ble_gatts_value_get(conn_idx, char_handle, data, len);
	os_free(data);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_gatts_value_get, ecr_ble_gatts_get_value,  "gatts_get_value [char_handle]",  "gatts_get_value [char_handle]");

static int ecr_ble_notify(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | conn_handle | char_handle | data_len | data,data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_idx = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	uint16_t len  =  atoi(argv[3]); 
	
	uint8_t indx = 0;
	uint8_t *data = os_malloc(sizeof(uint8_t *)*len);
	uint8_t str_len = strlen(argv[4]);

	char value[str_len+1];
	memcpy(value, argv[4], str_len);
	value[str_len] = '\0';
	
	char *back_value = strtok(value,",");
    while(back_value != NULL)
    {
    	uint8_t in_len = strlen(back_value);
        data[indx] = hexToDec(back_value,in_len);
        back_value = strtok(NULL,",");
        indx++;
    }

	ecr_ble_gatts_value_notify(conn_idx, char_handle, data, len);

	os_free(data);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_notify, ecr_ble_notify,  "ecr_ble_notify",  "ecr_ble_notify");

static int ecr_ble_indicate(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 5)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | conn_handle | char_handle | data_len | data,data\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_idx = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	uint16_t len  =  atoi(argv[3]); 
	
	uint8_t indx = 0;
	uint8_t *data = os_malloc(sizeof(uint8_t *)*len);
	uint8_t str_len = strlen(argv[4]);

	char value[str_len+1];
	memcpy(value, argv[4], str_len);
	value[str_len] = '\0';
	
	char *back_value = strtok(value,",");

	while(back_value != NULL)
	{
		uint8_t in_len = strlen(back_value);
		data[indx] = hexToDec(back_value,in_len);
		back_value = strtok(NULL,",");
		indx++;
	}
	
	ecr_ble_gatts_value_indicate(conn_idx, char_handle, data, len);

	os_free(data);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_indicate, ecr_ble_indicate,  "ecr_ble_indicate",  "ecr_ble_indicate");

static int ecr_ble_irk_set(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 3)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | irk_len | irk\n");
		return CMD_RET_FAILURE;
	}
	uint16_t irk_len = atoi(argv[1]);

	uint8_t * irk = os_malloc(sizeof(uint8_t)*irk_len);

	for (int i = 0; i < irk_len; i++)
    {
        irk[i] = hex2num(*(argv[4]+i*3)) * 0x10 + hex2num(*(argv[4]+i*3+1));
    }	

	ecr_ble_set_irk(irk);

	os_free(irk);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_irk_set, ecr_ble_irk_set,  "ecr_ble_irk_set",  "ecr_ble_irk_set");

static int ecr_ble_reset_all(cmd_tbl_t *t, int argc, char *argv[])
{
	ecr_ble_reset();

	return CMD_RET_SUCCESS;
}

CLI_CMD(ecr_ble_reset_all, ecr_ble_reset_all,  "ecr_ble_reset",  "ecr_ble_reset");


#endif //CFG_HOST && CONFIG_BLE_TEST_NETCFG


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
		os_printf(LM_CMD, LL_INFO,"TY_BT_EVENT_CONNECTED\n");
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

static int ble_start_scan(cmd_tbl_t *h, int argc, char *argv[])
{	
	ECR_BLE_GAP_SCAN_PARAMS_T ECR_BLE_GAP_SCAN_PARAMS ;
	
	ECR_BLE_GAP_SCAN_PARAMS.extended = 1;
	ECR_BLE_GAP_SCAN_PARAMS.active = 1;
	ECR_BLE_GAP_SCAN_PARAMS.scan_phys = 1;
	ECR_BLE_GAP_SCAN_PARAMS.interval = 30;
	ECR_BLE_GAP_SCAN_PARAMS.window = 30;
	ECR_BLE_GAP_SCAN_PARAMS.timeout = 0;
	ECR_BLE_GAP_SCAN_PARAMS.scan_channel_map = 0x07;
	
	ecr_ble_gap_scan_start(&ECR_BLE_GAP_SCAN_PARAMS);
	return CMD_RET_SUCCESS;
}
CLI_CMD(ble_start_scan, ble_start_scan,  "ble_start_scan",	  "ble_start_scan");

static int ble_scan_stop(cmd_tbl_t *h, int argc, char *argv[])
{
	ecr_ble_gap_scan_stop();
	return CMD_RET_SUCCESS;
}
CLI_CMD(ble_scan_stop, ble_scan_stop,  "ble_scan_stop",	  "ble_scan_stop");

static int ble_start_connect(cmd_tbl_t *h, int argc, char *argv[])
{	
	ECR_BLE_GAP_CONN_PARAMS_T ECR_BLE_GAP_CONN_PARAMS ;
	ECR_BLE_GAP_ADDR_T p_peer_addr;
	p_peer_addr.type = ECR_BLE_STATIC_ADDR;
	memcpy(p_peer_addr.addr, peer_device_addr, peer_device_addr_len);

	ECR_BLE_GAP_CONN_PARAMS.conn_interval_min = 12;    ////´óÓÚ0x0006
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_max = 12;    //x*0.625ms,<0X0C80£¬32s
	ECR_BLE_GAP_CONN_PARAMS.conn_latency = 0x0000;         //x*0.625 
	ECR_BLE_GAP_CONN_PARAMS.conn_sup_timeout = 500;     
	
	ecr_ble_gap_connect(&p_peer_addr, &ECR_BLE_GAP_CONN_PARAMS);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ble_start_connect, ble_start_connect,  "ble_start_connect",	  "ble_start_connect");

static int ble_gap_disconnect(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc != 3)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: conn_idx  reason\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_idx = atoi(argv[1]);
	uint8_t reason = atoi(argv[2]);
	ecr_ble_gap_disconnect(conn_idx, reason);
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(ble_gap_disconnect, ble_gap_disconnect,  "ble_gap_disconnect",	  "ble_gap_disconnect");

static int ble_connect_param_update(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc != 6)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: interval_min interval_max latency timeout conn_idx\n");
		return CMD_RET_FAILURE;
	}
	ECR_BLE_GAP_CONN_PARAMS_T ECR_BLE_GAP_CONN_PARAMS ;
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_min = atoi(argv[1]);;
	ECR_BLE_GAP_CONN_PARAMS.conn_interval_max = atoi(argv[2]);;
	ECR_BLE_GAP_CONN_PARAMS.conn_latency = atoi(argv[3]);;
	ECR_BLE_GAP_CONN_PARAMS.conn_sup_timeout = atoi(argv[4]);;
	uint16_t conn_idx = atoi(argv[5]);

	ecr_ble_connect_param_update(conn_idx, &ECR_BLE_GAP_CONN_PARAMS);
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(ble_param_update, ble_connect_param_update,  "ble_connect_param_update",	  "ble_connect_param_update");


static int ecr_ble_get_conn_rssi(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_INFO, "Input ERROR!!!, Please input: Func | conn_idx\n");
		return CMD_RET_FAILURE;
	}
	uint16_t conn_handle = atoi(argv[1]);
	ecr_ble_connect_rssi_get(conn_handle);
	return CMD_RET_SUCCESS;
}

CLI_CMD(ble_get_conn_rssi, ecr_ble_get_conn_rssi,  "get_conn_rssi",  "get_conn_rssi");

static int ble_gattc_all_service_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=2)
	{
		os_printf(LM_OS, LM_BLE, "Please input:con_handle");
		return CMD_RET_FAILURE;
	}
	uint16_t con_handle = atoi(argv[1]);
	ecr_ble_gattc_all_service_discovery(con_handle);

	return CMD_RET_SUCCESS;
}
CLI_CMD(discovery_all_service, ble_gattc_all_service_discovery,  "ble_gattc_all_service_discovery",	  "ble_gattc_all_service_discovery");

static int ble_gattc_all_char_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Please input:con_handle start_handle end_handle");
		return CMD_RET_FAILURE;
	}
	uint16_t con_handle = atoi(argv[1]);
	uint16_t start_handle = atoi(argv[2]);
	uint16_t end_handle = atoi(argv[3]);
	ecr_ble_gattc_all_char_discovery(con_handle, start_handle, end_handle);
	
	return CMD_RET_SUCCESS;
}
CLI_CMD(discovery_all_char, ble_gattc_all_char_discovery,  "ble_gattc_all_char_discovery",	  "ble_gattc_all_char_discovery");

static int ble_gattc_char_desc_discovery(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Please input:con_handle start_handle end_handle");
		return CMD_RET_FAILURE;
	}
	uint16_t con_handle = atoi(argv[1]);
	uint16_t start_handle = atoi(argv[2]);
	uint16_t end_handle = atoi(argv[3]);
 	ecr_ble_gattc_char_desc_discovery(con_handle, start_handle, end_handle);

	return CMD_RET_SUCCESS;
}
CLI_CMD(discovery_all_desc, ble_gattc_char_desc_discovery,  "ble_gattc_char_desc_discovery",	  "ble_gattc_char_desc_discovery");

static int ble_gattc_exchange_mtu_req(cmd_tbl_t *h, int argc, char *argv[])
{		
	if(argc !=3)
	{
		os_printf(LM_OS, LM_BLE, "Please input:con_handle client_mtu ");
		return CMD_RET_FAILURE;
	}
	uint16_t con_handle = atoi(argv[1]);
	uint16_t client_mtu = atoi(argv[2]);
	
	ecr_ble_gattc_exchange_mtu_req(con_handle, client_mtu);
	 
	return CMD_RET_SUCCESS;
}
CLI_CMD(gattc_exchange_mtu, ble_gattc_exchange_mtu_req,  "ble_gattc_exchange_mtu_req",	  "ble_gattc_exchange_mtu_req");

static int ble_gattc_read(cmd_tbl_t *h, int argc, char *argv[])
{	
	if(argc !=3)
	{
		os_printf(LM_OS, LM_BLE, "Please input:con_handle char_handle ");
		return CMD_RET_FAILURE;
	}
	
	uint16_t con_handle = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	
	ecr_ble_gattc_read(con_handle, char_handle);

	return CMD_RET_SUCCESS;
}
CLI_CMD(gattc_read_char, ble_gattc_read,  "ble_gattc_read",	  "ble_gattc_read");

static int ble_gattc_write(cmd_tbl_t *h, int argc, char *argv[])
{		
	if(argc !=4)
	{
		os_printf(LM_OS, LM_BLE, "Please input:Con_handle Char_handle Char_value \n");
		return CMD_RET_FAILURE;
	}

	uint16_t con_handle = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);
	
	uint8_t char_value[10] = {'\0'}; 
	
	if(strlen(argv[3])<10)
	{
		char * tmp=argv[3];
		os_printf(LM_OS, LM_BLE, "input value=");
		for(uint8_t i=0;i<strlen(argv[3]);i++)
		{	
			//hex2num
			char_value[i] = *tmp++;
			os_printf(LM_OS, LM_BLE, "%c", char_value[i]);
		}	
		os_printf(LM_OS, LM_BLE, "\n");
		ecr_ble_gattc_write(con_handle, char_handle, char_value, strlen(argv[3]));
		return CMD_RET_SUCCESS;
	}
	else 
	{
		return CMD_RET_FAILURE;
	}
}
CLI_CMD(gattc_write_char, ble_gattc_write,  "ble_gattc_write",	  "ble_gattc_write");

static int ble_gattc_write_without_rsp(cmd_tbl_t *h, int argc, char *argv[])
{
	
	if(argc != 4)
	{
		os_printf(LM_OS, LM_BLE, "Please input:Con_handle Char_handle Char_value \n");
		return CMD_RET_FAILURE;
	}

	uint16_t con_handle = atoi(argv[1]);
	uint16_t char_handle = atoi(argv[2]);

	uint8_t char_value[10] = {'\0'};

	if(strlen(argv[3])<10)
	{
		char * tmp=argv[3];
		os_printf(LM_OS, LM_BLE, "input value=");
		for(uint8_t i=0;i<strlen(argv[3]);i++)
		{	
			//hex2num
			char_value[i] = *tmp++;
			os_printf(LM_OS, LM_BLE, "%c", char_value[i]);
		}	
		os_printf(LM_OS, LM_BLE, "\n");
		ecr_ble_gattc_write_without_rsp(con_handle, char_handle, char_value, strlen(argv[3]));
		return CMD_RET_SUCCESS;
	}
	else 
	{
		return CMD_RET_FAILURE;
	}
}
CLI_CMD(gattc_write_char_norsp, ble_gattc_write_without_rsp,  "ble_gattc_write_without_rsp",  "ble_gattc_write_without_rsp");
