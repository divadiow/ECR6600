/*******************************************************************************
 * Copyright by eswin.
 *
 * File Name: wifi_command.c
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v1.0
 * Author:        zhangyunpeng
 * Date:         
 ********************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
//#include "util_cli_freertos.h"
//#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "utils/includes.h"
#include "utils/common.h"
//#include "hostapd.h"
//#include "wpa_supplicant_i.h"
//#include "sta_info.h"

#include "system_event.h"
#include "system_wifi.h"
#include "system_network.h"

//#include "umac_beacon.h"

#include "easyflash.h"
#include "system_config.h"
#include "hal_uart.h"

#include "smartconfig.h"
//#include "system_modem_api.h"
#include "smartconfig_notify.h"
//#include "dhcpserver.h"

#include "at_common.h"
#include "dce.h"
#include "dce_commands.h"
#include "at_def.h"
#include "wifi_command.h"
#include "hal_gpio.h"

#define MACSTR_LEN 17
#define SHOW_ECN              BIT0   
#define SHOW_SSID             BIT1
#define SHOW_RSSI             BIT2
#define SHOW_MAC              BIT3
#define SHOW_CHANNEL          BIT4
#define SHOW_FREQ_OFFSET      BIT5
#define SHOW_PAIRWISE_CIPHER  BIT6
#define SHOW_PWD			BIT7

#define DEF "_DEF"
#define CUR "_CUR"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define SSID_MAX_LEN        32
#define AT_CMD_MAX 64
wifi_ap_config_t softap_config;

static const int CONNECTED_BIT = BIT0;
static const int DISCONNECTED_BIT = BIT1;
static const int GOT_IP_BIT = BIT2;
// quxin static const int GOT_SSID_PWD = BIT3;
static EventGroupHandle_t at_wifi_event_group = NULL;
// quxin extern beacon_info g_beacon_info;
extern void dns_server_save_pre(void);
extern void dns_server_depend_ap(void);
// quxin extern bool scan_hit_multi;

typedef struct at_scan_config_t {
    uint32_t sort_flag;
    uint32_t scan_bit;
} scan_config;

static scan_config at_scan_config = {0,127};
typedef struct connect_staip_s
{
    ip_addr_t ip;
    uint8_t mac[6];
    struct connect_staip_s *next;
    struct connect_staip_s *prev;
} connect_staip_t;
connect_staip_t *connect_staip = NULL;

static void connect_list_init(void)
{
    connect_staip = malloc(sizeof(connect_staip_t));
    memset(connect_staip, '\0', sizeof(connect_staip_t));

    connect_staip->next = connect_staip->prev = connect_staip;
}

static void connect_list_add(ip_addr_t ip, uint8_t *mac)
{
    connect_staip_t *new_ip;
    new_ip = malloc(sizeof(connect_staip_t));
    new_ip->ip = ip;
    memcpy(new_ip->mac,mac,ETH_ALEN);

    connect_staip->next->prev = new_ip;
    new_ip->next = connect_staip->next;

    connect_staip->next = new_ip;
    new_ip->prev = connect_staip;
}

static void connect_list_remove(uint8_t *mac)
{
    connect_staip_t *cur = connect_staip->next;
    while(cur != connect_staip)
    {
        if(memcmp((char *)mac, (char *)cur->mac, 6) == 0)
        {
            cur->next->prev = cur->prev;
            cur->prev->next = cur->next;
            cur->next = cur->prev = cur;

            free(cur);
            return;
        }

        cur = cur->next;
    }
}
bool connect_check(ip_addr_t ip)
{
    connect_staip_t *cur = connect_staip->next;
    while(cur != connect_staip)
    {
        if(memcmp(&ip, &(cur->ip), sizeof(ip_addr_t)) == 0)
            return true;
        cur = cur->next;
    }

    return false;
}

extern uint8_t skylab_bleNet_flag;
extern wifi_config_u skylab_conf;
void Skylab_SaveWifi(wifi_config_u conf){
	wifi_nv_info_t *sta_conn_info = get_wifi_nv_info();
	wifi_info_t ap_info = {0};
	wifi_get_wifi_info(&ap_info);
	if(ap_info.ssid[0] == 0) {
	  os_printf(LM_APP, LL_INFO, "ap_info.ssid[0] == 0\n");
	  //return;
	}
    memcpy(sta_conn_info->sta_ssid, (char *)conf.sta.ssid,sizeof((conf.sta.ssid)));
    memcpy(sta_conn_info->sta_pwd,conf.sta.password,sizeof((conf.sta.password)));
    memcpy(sta_conn_info->sta_bssid,conf.sta.bssid,sizeof((conf.sta.bssid)));
    sta_conn_info->channel = ap_info.channel;
    wifi_save_nv_info(sta_conn_info);
}


static sys_err_t at_wifi_event_handler(void *ctx, system_event_t *event)
{
    dce_t* dce = (dce_t*) ctx;
    arg_t args;
    wifi_info_t info;
    unsigned int pass_through;

    char mac_str[MACSTR_LEN+1];
    switch (event->event_id) {
    case SYSTEM_EVENT_WIFI_READY:
        wifi_sta_auto_conn_start();
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("SYSTEM_EVENT_STA_CONNECTED\n");
        dns_server_save_pre();
         if(at_wifi_event_group) {
            xEventGroupClearBits(at_wifi_event_group, GOT_IP_BIT);
            xEventGroupSetBits(at_wifi_event_group, CONNECTED_BIT);
        }
        dce_emit_extended_result_code(dce, "WIFI CONNECTED", -1, 1);
        wifi_get_wifi_info(&info);
        os_printf(LM_WIFI, LL_ERR, "===info->channel=%d=\n", info.channel);
        if(AP_STATUS_STARTED == wifi_get_status(SOFTAP_IF) && softap_config.channel != info.channel)
        {
            softap_config.channel = info.channel;
            wifi_stop_softap();

            wifi_start_softap((wifi_config_u *)&softap_config);
        }
        break;

    case SYSTEM_EVENT_STA_START:
        printf("SYSTEM_EVENT_STA_START\n");
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        printf("test SYSTEM_EVENT_STA_GOT_IP\n");
        dns_server_depend_ap();
        dce_emit_extended_result_code(dce, "WIFI GOT IP", -1, 1);
        if(at_wifi_event_group) {
            xEventGroupSetBits(at_wifi_event_group, GOT_IP_BIT);
        }
        extern int at_net_client_auto_start(void);
        at_net_client_auto_start();
		if(skylab_bleNet_flag){
			Skylab_SaveWifi(skylab_conf);
			skylab_bleNet_flag = 0;
		}
		hal_gpio_write(GPIO_NUM_4,1);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        printf("SYSTEM_EVENT_STA_DISCONNECTED\n");
        dce_emit_extended_result_code(dce, "WIFI DISCONNECT", -1, 1);
        hal_system_get_config(CUSTOMER_NV_WIFI_IP_MODE, &(pass_through), sizeof(pass_through));
        extern int at_net_close(int link_id);
        if(pass_through == 0) {
            at_net_close(MAX_CONN_NUM);
        }
         if(at_wifi_event_group) {
             xEventGroupClearBits(at_wifi_event_group, CONNECTED_BIT);
             xEventGroupSetBits(at_wifi_event_group, DISCONNECTED_BIT);
         }
		hal_gpio_write(GPIO_NUM_4,0);
        break;
     case SYSTEM_EVENT_AP_STACONNECTED:
        printf("SYSTEM_EVENT_AP_STACONNECTED\n");
        sprintf(mac_str, MACSTR, MAC2STR(event->event_info.sta_connected.mac));
        args.type = ARG_TYPE_STRING;
        args.value.string = mac_str;
        
        // pos +=(snprintf(pos,sizeof("STA_CONNECTED: "),"%s","STA_CONNECTED: "));
        // pos +=snprintf(pos,sizeof(buf),MAC_STR,MAC_VALUE(event->event_info.sta_connected.mac));
           dce_emit_extended_result_code_with_args(dce, "STA_CONNECTED", -1, &args, 1, 1,false);
         break;
        
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        printf("SYSTEM_EVENT_AP_STADISCONNECTED\n");
        // pos +=(snprintf(pos,sizeof("STA_DISCONNECTED: "),"%s","STA_DISCONNECTED: "));
        // pos +=snprintf(pos,sizeof(buf),MAC_STR,MAC_VALUE(event->event_info.sta_connected.mac));

        sprintf(mac_str, MACSTR, MAC2STR(event->event_info.sta_connected.mac));
        args.type = ARG_TYPE_STRING;
        args.value.string = mac_str;
        connect_list_remove(event->event_info.sta_connected.mac);
           dce_emit_extended_result_code_with_args(dce, "STA_DISCONNECTED", -1, &args, 1, 1,false);
    break;
     case SYSTEM_EVENT_AP_STAIPASSIGNED:    
         printf("SYSTEM_EVENT_AP_STAIPASSIGNED\n");
        //  pos +=(snprintf(pos,sizeof("DIST_STA_IP: "),"%s","DIST_STA_IP: "));
        //  pos +=snprintf(pos,sizeof(buf),MAC_STR,MAC_VALUE(event->event_info.sta_connected.mac));
        //  pos +=snprintf(pos,3,"%s"," ");
        //  pos +=snprintf(pos,sizeof(buf),IP4_ADDR_STR,IP4_ADDR_VALUE(&event->event_info.sta_connected.ip_info.ip));
        char ip_str[17]; 
        sprintf(ip_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&event->event_info.sta_connected.ip_info.ip)));
        args.type = ARG_TYPE_STRING;
        args.value.string = ip_str;
        extern int at_net_client_auto_start(void);
        at_net_client_auto_start();
        connect_list_add(event->event_info.sta_connected.ip_info.ip, event->event_info.sta_connected.mac);
        dce_emit_extended_result_code_with_args(dce, "DIST_STA_IP", -1, &args, 1, 1,false);
        break;        

    default:
        break;
    }
    return 0;
}


bool is_valid_mac(const char *mac_addr)
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
void hexStr2MACAddr(uint8_t *hexStr, uint8_t* mac )
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

dce_result_t dce_handle_CWMODE_api(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv,uint8_t is_flash,char* mode_type)
{
    static const char valid_mode[] = {WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_AP_STA};
    static const int valid_mode_count = sizeof(valid_mode)/sizeof(char);
    wifi_work_mode_e wifi_mode;
    char handle[AT_CMD_MAX];
    int ret;

    if (dce_handle_flashEn_arg(kind, &argc, argv, (bool *)&is_flash) != DCE_RC_OK) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    if (kind == DCE_READ)
    {
        ret = hal_system_get_config(CUSTOMER_NV_WIFI_OP_MODE, &wifi_mode, sizeof(wifi_mode));
        if (ret == 0) {
            sprintf(handle, "%s%s", mode_type, ": FLASH IS NULL, PLEASE WRITE FIRST");
            dce_emit_extended_result_code(dce, handle, -1, 1);
        } else {
            arg_t result_def[] = {{ARG_TYPE_NUMBER, .value.number = wifi_mode + 1}};
            sprintf(handle, "%s%s", mode_type, DEF);
            dce_emit_extended_result_code_with_args(dce, handle, -1, result_def, 1, 1, false);
        }
        memset(handle, 0, AT_CMD_MAX);
        wifi_mode = wifi_get_opmode();
        arg_t result_cur[] = {{ARG_TYPE_NUMBER, .value.number = wifi_mode + 1}};
        sprintf(handle, "%s%s", mode_type, CUR);
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_cur, 1, 1, false);
    } else if(kind == DCE_TEST){
        char buf[30]={0};
        sprintf(buf,"%s%s",mode_type,":(1-3)");
        dce_emit_extended_result_code(dce, buf, -1, 1);
    } else {
        if (argc != 1 || argv[0].type != ARG_TYPE_NUMBER){
            os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        int i = 0;
        for (i = 0; i < valid_mode_count; ++i)
        {
            if (valid_mode[i] == (argv[0].value.number - 1))
                break;
        }
		
        if(i == valid_mode_count) {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        wifi_set_opmode(valid_mode[i]);
        if(valid_mode[i] == WIFI_MODE_STA)
            wifi_stop_softap();
            
        else if(valid_mode[i] == WIFI_MODE_AP)
            wifi_stop_station();
        
        if(is_flash == true)
        {
            wifi_mode = valid_mode[i];
            hal_system_set_config(CUSTOMER_NV_WIFI_OP_MODE, &wifi_mode, sizeof(wifi_mode));
        }
        dce_emit_basic_result_code(dce, DCE_RC_OK);
    }
	
	return DCE_OK;
}
dce_result_t dce_handle_CWMODE_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_handle_CWMODE_api(dce, group_ctx, kind, argc, argv,false,"CWMODE_CUR");
    return DCE_OK;
}

dce_result_t dce_handle_CWMODE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_handle_CWMODE_api(dce, group_ctx, kind, argc, argv,false,"CWMODE");
    return DCE_OK;
}


uint8_t pci_en = 0;
uint8_t chinese_ssid=0;
extern int TG_strtohex(const char * data,int data_len);
dce_result_t TG_dce_handle_CWJAP_api(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv,uint8_t is_flash)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    wifi_config_u conf;
    wifi_info_t ap_info = {0};
    char handle[AT_CMD_MAX];
	os_printf(LM_APP, LL_INFO, "canshu=%d\r\n",argc);
	os_printf(LM_APP, LL_INFO, "ssid=%s\r\n",argv[0].value.string);

    if (dce_handle_flashEn_arg(kind, &argc, argv, (bool *)&is_flash) != DCE_RC_OK) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

	
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    else if (kind == DCE_WRITE)
    {
        memset(&conf, 0, sizeof(wifi_config_u));
        if (argc < 2 || argc > 5 ||
            argv[0].type != ARG_TYPE_STRING ||
            argv[1].type != ARG_TYPE_STRING || (strlen(argv[1].value.string) < 8 && strlen(argv[1].value.string) > 0) || strlen(argv[1].value.string) > 64 ||
            strlen(argv[0].value.string) >= sizeof(conf.sta.ssid) ||
            strlen(argv[1].value.string) >= sizeof(conf.sta.password))
        {
            os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
			
			os_printf(LM_APP, LL_INFO, "argc=%d\r\n",argc);
        if(argc == 3)
        {
        	os_printf(LM_APP, LL_INFO, "argc LINE= %d\n",__LINE__);
            if(argv[2].type == ARG_TYPE_NUMBER && argv[2].value.number >= 0 && argv[2].value.number <= 1)
            {
            
                chinese_ssid = argv[2].value.number;
           
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_OK;
            }
        }
		os_printf(LM_APP, LL_INFO, "chinese_ssid= %d\n",chinese_ssid);
        if (0 == strlen(argv[0].value.string)) {
            os_printf(LM_APP, LL_INFO, "invalid ssid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }  

		if(chinese_ssid==0)
        	strcpy((char *)conf.sta.ssid, argv[0].value.string);
		else
			{
				TG_strtohex( argv[0].value.string ,  strlen(argv[0].value.string) );
				for(int i=0;i<strlen(argv[0].value.string)/2;i++)
					{
		
						os_printf(LM_APP, LL_INFO, " write_buffer=%X\n",write_buffer[i]);
					}		
				
				memcpy((char *)conf.sta.ssid,(char *)write_buffer,strlen(argv[0].value.string)/2);
			}
	

		strcpy(conf.sta.password, argv[1].value.string);
        //firstly disconnect from AP
        wifi_stop_station();
//        dce_emit_extended_result_code(dce, "WIFI DISCONNECT", -1, 1);
        sys_err_t ret = wifi_start_station(&conf);
        if (SYS_ERR_WIFI_MODE == ret || SYS_ERR_WIFI_BUSY == ret){
            wifi_stop_station();
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }

        EventBits_t uxBits;
        const TickType_t xTicksToWait = 30000 / portTICK_PERIOD_MS;

        uxBits = xEventGroupWaitBits(at_wifi_event_group, CONNECTED_BIT, true, true, xTicksToWait);
        if((uxBits & CONNECTED_BIT ) != 0){
            // dce_emit_extended_result_code(dce, "WIFI CONNECTED", -1, 1);
        } else {
            wifi_stop_station();
            dce_emit_extended_result_code(dce, is_flash ? "+CWJAP_DEF:4" : "+CWJAP_CUR:4", -1, 1);
            dce_emit_extended_result_code(dce, "FAIL", -1, 1);
            return DCE_RC_OK;
        }
        uxBits = xEventGroupWaitBits(at_wifi_event_group, GOT_IP_BIT, true, true, xTicksToWait);
        if((uxBits & GOT_IP_BIT ) != 0){
            // dce_emit_extended_result_code(dce, "WIFI GOT IP", -1, 1);
            /*\B1\A3\B4\E6\C1\AC\BD\D3\D0\C5息\B5\BDflash*/
            wifi_nv_info_t *sta_conn_info = get_wifi_nv_info();
            wifi_get_wifi_info(&ap_info);
            if(ap_info.ssid[0] == 0) {
                dce_emit_basic_result_code(dce, DCE_RC_OK);
                return DCE_RC_OK;
            }
            memcpy(sta_conn_info->sta_ssid, (char *)conf.sta.ssid,sizeof((conf.sta.ssid)));
            memcpy(sta_conn_info->sta_pwd,conf.sta.password,sizeof((conf.sta.password)));
            memcpy(sta_conn_info->sta_bssid,conf.sta.bssid,sizeof((conf.sta.bssid)));
            sta_conn_info->channel = ap_info.channel;
			os_printf(LM_APP, LL_INFO, "wifi_save_nv_info=%d %d\r\n",is_flash,argc);
            if(true == is_flash){
	            if(argc == 2){
	                wifi_get_wifi_info(&ap_info);
	                memcpy(sta_conn_info->sta_bssid,ap_info.bssid,sizeof((ap_info.bssid)));
	            }
				os_printf(LM_APP, LL_INFO, "enter wifi_save_nv_info\r\n");
	            wifi_save_nv_info(sta_conn_info);
            }
                
        } else {
            wifi_stop_station();
            dce_emit_extended_result_code(dce, is_flash ? "+CWJAP_DEF:4" : "+CWJAP_CUR:4", -1, 1);
            dce_emit_extended_result_code(dce, "FAIL", -1, 1);
            return DCE_RC_OK;
        }
        dce_emit_basic_result_code(dce, DCE_RC_OK);

        return DCE_RC_OK;
    }
    else
    {
        char mac_str[MACSTR_LEN+1];
        wifi_nv_info_t ap_info_nv;

        uint8_t ret = wifi_load_nv_info(&ap_info_nv);

        if (ret == SYS_ERR) {
            sprintf(handle, "%s", "CWJAP_DEF: FLASH IS NULL, PLEASE WRITE FIRST");
            dce_emit_extended_result_code(dce, handle, -1, 1);
        } else {
            sprintf(mac_str, MACSTR, MAC2STR(ap_info_nv.sta_bssid));

            arg_t args_def[] = {
                {ARG_TYPE_STRING, .value.string = (char *)&ap_info_nv.sta_ssid[0]},
                {ARG_TYPE_STRING, .value.string = (char *)&ap_info_nv.sta_pwd[0] }, // add 20240714
                {ARG_TYPE_STRING, .value.string = mac_str},
                {ARG_TYPE_NUMBER, .value.number = ap_info_nv.channel},
                {ARG_TYPE_NUMBER, .value.number = 0},
                
            };
            dce_emit_extended_result_code_with_args(dce, "CWJAP_DEF" , -1, args_def, 5, 1,false);//4
        }

        wifi_get_wifi_info(&ap_info);

        if(ap_info.ssid[0] == 0) {
            dce_emit_extended_result_code(dce, "CWJAP_CUR: NOT CONNECT", -1, 1);
            return DCE_RC_OK;
        }

        sprintf(mac_str, MACSTR, MAC2STR(ap_info.bssid));
		//system_printf("pwd=%s\r\n",(char *)&ap_info_nv.sta_pwd );
        arg_t args_cur[] = {
            {ARG_TYPE_STRING, .value.string = (char *)&ap_info.ssid[0]},
			{ARG_TYPE_STRING, .value.string = (char *)&ap_info.pwd[0] }, // add 20240714
            {ARG_TYPE_STRING, .value.string = mac_str},
            {ARG_TYPE_NUMBER, .value.number = ap_info.channel},
            {ARG_TYPE_NUMBER, .value.number = ap_info.rssi},

        };
        dce_emit_extended_result_code_with_args(dce, "CWJAP_CUR", -1, args_cur, 5, 1,false);

    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWJAP_api(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv,uint8_t is_flash)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    wifi_config_u conf;
    wifi_info_t ap_info = {0};
    char handle[AT_CMD_MAX];
	os_printf(LM_APP, LL_INFO, "canshu=%d\r\n",argc);
	os_printf(LM_APP, LL_INFO, "ssid=%s\r\n",argv[0].value.string);

    if (dce_handle_flashEn_arg(kind, &argc, argv, (bool *)&is_flash) != DCE_RC_OK) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    else if (kind == DCE_WRITE)
    {
        memset(&conf, 0, sizeof(wifi_config_u));
        if (argc < 2 || argc > 4 ||
            argv[0].type != ARG_TYPE_STRING ||
            argv[1].type != ARG_TYPE_STRING || (strlen(argv[1].value.string) < 8 && strlen(argv[1].value.string) > 0) || strlen(argv[1].value.string) > 64 ||
            strlen(argv[0].value.string) >= sizeof(conf.sta.ssid) ||
            strlen(argv[1].value.string) >= sizeof(conf.sta.password))
        {
            os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        if(argc == 3 || argc == 4){
            if(argv[2].type != ARG_TYPE_STRING){
                os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_OK;
            } else {
                if(true != is_valid_mac(argv[2].value.string)){
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_RC_OK;
                } else {
                    hexStr2MACAddr((uint8_t*)argv[2].value.string, conf.sta.bssid);
                }
            }
        }

        if(argc == 4)
        {
            if(argv[3].type == ARG_TYPE_NUMBER && argv[3].value.number >= 0 && argv[3].value.number <= 1)
            {
                pci_en = argv[3].value.number;
                //quxin if(is_flash == true)
                //quxin     ef_set_env_blob(NV_PCI, &pci_en, sizeof(pci_en));
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_OK;
            }
        }

        if (0 == strlen(argv[0].value.string)) {
            os_printf(LM_APP, LL_INFO, "invalid ssid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }  
        strcpy((char *)conf.sta.ssid, argv[0].value.string);
        strcpy(conf.sta.password, argv[1].value.string);
        //firstly disconnect from AP
        wifi_stop_station();
//        dce_emit_extended_result_code(dce, "WIFI DISCONNECT", -1, 1);
#if 0
        wifi_scan_config_t config = {0};
        wifi_info_t info;
        config.ssid = (uint8_t *)conf.sta.ssid;
        wifi_scan_start(1, &config);
        wifi_get_scan_result(0, &info);

        wifi_work_mode_e opmode = wifi_get_opmode();
        if (opmode == WIFI_MODE_AP_STA) {
            wifi_status_e ap_status = wifi_get_status(SOFTAP_IF);
            if (ap_status == AP_STATUS_STARTED) {
                uint16_t rf_ch = wifi_rf_get_channel();
                if (info.channel != rf_ch) {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_RC_ERROR;
                }
            }
        }
#endif
        sys_err_t ret = wifi_start_station(&conf);
        if (SYS_ERR_WIFI_MODE == ret || SYS_ERR_WIFI_BUSY == ret){
            wifi_stop_station();
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }

        EventBits_t uxBits;
        const TickType_t xTicksToWait = 30000 / portTICK_PERIOD_MS;

        uxBits = xEventGroupWaitBits(at_wifi_event_group, CONNECTED_BIT, true, true, xTicksToWait);
        if((uxBits & CONNECTED_BIT ) != 0){
            // dce_emit_extended_result_code(dce, "WIFI CONNECTED", -1, 1);
        } else {
            wifi_stop_station();
            dce_emit_extended_result_code(dce, is_flash ? "+CWJAP_DEF:4" : "+CWJAP_CUR:4", -1, 1);
            dce_emit_extended_result_code(dce, "FAIL", -1, 1);
            return DCE_RC_OK;
        }
        uxBits = xEventGroupWaitBits(at_wifi_event_group, GOT_IP_BIT, true, true, xTicksToWait);
        if((uxBits & GOT_IP_BIT ) != 0){
            // dce_emit_extended_result_code(dce, "WIFI GOT IP", -1, 1);
            /*\B1\A3\B4\E6\C1\AC\BD\D3\D0\C5息\B5\BDflash*/
            wifi_nv_info_t *sta_conn_info = get_wifi_nv_info();;
            wifi_get_wifi_info(&ap_info);
            if(ap_info.ssid[0] == 0) {
                dce_emit_basic_result_code(dce, DCE_RC_OK);
                return DCE_RC_OK;
            }
            memcpy(sta_conn_info->sta_ssid, (char *)conf.sta.ssid,sizeof((conf.sta.ssid)));
            memcpy(sta_conn_info->sta_pwd,conf.sta.password,sizeof((conf.sta.password)));
            memcpy(sta_conn_info->sta_bssid,conf.sta.bssid,sizeof((conf.sta.bssid)));
            sta_conn_info->channel = ap_info.channel;
            if(true == is_flash){
                if(argc == 2){
                    wifi_get_wifi_info(&ap_info);
                    memcpy(sta_conn_info->sta_bssid,ap_info.bssid,sizeof((ap_info.bssid)));
                }
                wifi_save_nv_info(sta_conn_info);
            }
                
        } else {
            wifi_stop_station();
            dce_emit_extended_result_code(dce, is_flash ? "+CWJAP_DEF:4" : "+CWJAP_CUR:4", -1, 1);
            dce_emit_extended_result_code(dce, "FAIL", -1, 1);
            return DCE_RC_OK;
        }
        dce_emit_basic_result_code(dce, DCE_RC_OK);

        return DCE_RC_OK;
    }
    else
    {
        char mac_str[MACSTR_LEN+1];
        wifi_nv_info_t ap_info_nv;

        uint8_t ret = wifi_load_nv_info(&ap_info_nv);

        if (ret == SYS_ERR) {
            sprintf(handle, "%s", "CWJAP_DEF: FLASH IS NULL, PLEASE WRITE FIRST");
            dce_emit_extended_result_code(dce, handle, -1, 1);
        } else {
            sprintf(mac_str, MACSTR, MAC2STR(ap_info_nv.sta_bssid));

            arg_t args_def[] = {
                {ARG_TYPE_STRING, .value.string = (char *)&ap_info_nv.sta_ssid[0]},
                {ARG_TYPE_STRING, .value.string = (char *)&ap_info_nv.sta_pwd[0] }, // add 20230714
                {ARG_TYPE_STRING, .value.string = mac_str},
                {ARG_TYPE_NUMBER, .value.number = ap_info_nv.channel},
                {ARG_TYPE_NUMBER, .value.number = 0},
                
            };
            dce_emit_extended_result_code_with_args(dce, "CWJAP_DEF" , -1, args_def, 5, 1,false);//4
        }

        wifi_get_wifi_info(&ap_info);

        if(ap_info.ssid[0] == 0) {
            dce_emit_extended_result_code(dce, "CWJAP_CUR: NOT CONNECT", -1, 1);
            return DCE_RC_OK;
        }

        sprintf(mac_str, MACSTR, MAC2STR(ap_info.bssid));

        arg_t args_cur[] = {
            {ARG_TYPE_STRING, .value.string = (char *)&ap_info.ssid[0]},
			{ARG_TYPE_STRING, .value.string = (char *)&ap_info_nv.sta_pwd[0] }, // add 20240714
            {ARG_TYPE_STRING, .value.string = mac_str},
            {ARG_TYPE_NUMBER, .value.number = ap_info.channel},
            {ARG_TYPE_NUMBER, .value.number = ap_info.rssi},

        };
        dce_emit_extended_result_code_with_args(dce, "CWJAP_CUR", -1, args_cur, 5, 1,false);

    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWJAP_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_handle_CWJAP_api(dce, group_ctx, kind, argc, argv,false);
    return DCE_OK;
}

dce_result_t dce_handle_CWJAP_DEF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    TG_dce_handle_CWJAP_api(dce, group_ctx, kind, argc, argv,false);
    return DCE_OK;
}

dce_result_t dce_handle_CWLAPOPT(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    // quxin wifi_config_u conf;
    if (kind != DCE_WRITE){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }

    if (argc != 2 || 
        argv[0].type != ARG_TYPE_NUMBER ||
        argv[1].type != ARG_TYPE_NUMBER ||
        argv[0].value.number < 0 ||
        argv[0].value.number > 1 ||
        argv[1].value.number > 127 ||
        argv[1].value.number < 0 ){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    memset(&at_scan_config, 0, sizeof(scan_config)); 
    at_scan_config.sort_flag = argv[0].value.number;
    at_scan_config.scan_bit = argv[1].value.number;
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

static void sort_rssi(wifi_info_t *ap_list,int ap_num)
{
    int i,j;
    wifi_info_t temp;
    for(i=0;i<ap_num;i++) {
       for(j=0;j<ap_num-i-1;j++) {
            if(ap_list[j].rssi<ap_list[j+1].rssi)
            {
                temp=ap_list[j+1];
                ap_list[j+1]=ap_list[j];
                ap_list[j]=temp;
            }
        }
    }
}

static int bitcount(unsigned int n)
{
    int count=0 ;
    while (n) {
        count++ ;
        n &= (n - 1) ;
    }
    return count ;
}

static void response_ap_info(dce_t* dce,int ap_num, wifi_info_t *ap_list)
{
    if(at_scan_config.sort_flag == 1){
        sort_rssi(ap_list, ap_num);
    }
    int argc = bitcount(at_scan_config.scan_bit);
    arg_t args[7] = {0};
    
    int i = 0;
    for(i = 0; i < ap_num; i++){
        int arg_index = 0;
        if(at_scan_config.scan_bit & SHOW_ECN){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = ap_list[i].auth;
            arg_index++;
        }
        if(at_scan_config.scan_bit & SHOW_SSID){
            args[arg_index].type = ARG_TYPE_STRING;
            args[arg_index].value.string = (const char *)ap_list[i].ssid;
            arg_index++;
        }
        if(at_scan_config.scan_bit & SHOW_RSSI){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = ap_list[i].rssi;
            arg_index++;
        }

        if(at_scan_config.scan_bit & SHOW_MAC){
            char mac_str[MACSTR_LEN+1];
            sprintf(mac_str, MACSTR, MAC2STR(ap_list[i].bssid));
            args[arg_index].type = ARG_TYPE_STRING;
            args[arg_index].value.string = mac_str;
            arg_index++;
        }
        if(at_scan_config.scan_bit & SHOW_CHANNEL){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = ap_list[i].channel;
            arg_index++;
        }
        if(at_scan_config.scan_bit & SHOW_FREQ_OFFSET){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = 0;    //todo
            arg_index++;
        }
        if(at_scan_config.scan_bit & SHOW_PAIRWISE_CIPHER){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = ap_list[i].cipher;
            arg_index++; 
        }
        dce_emit_extended_result_code_with_args(dce, "CWLAP", -1, args, argc, 0, true);
    }
}

static void response_softap_info(dce_t* dce,int ap_num, wifi_ap_config_t *ap_list)
{

    int argc = bitcount(at_scan_config.scan_bit);
    arg_t args[7] = {0};
    
    int i = 0;
    for(i = 0; i < ap_num; i++){
        int arg_index = 0;
        if(at_scan_config.scan_bit & SHOW_SSID){
            args[arg_index].type = ARG_TYPE_STRING;
            args[arg_index].value.string = (const char *)ap_list[i].ssid;
            arg_index++;
        }

        if(at_scan_config.scan_bit & SHOW_CHANNEL){
            args[arg_index].type = ARG_TYPE_NUMBER;
            args[arg_index].value.number = ap_list[i].channel;
            arg_index++;
        }

        dce_emit_extended_result_code_with_args(dce, "CWINFO", -1, args, argc, 0, true);
    }
}

dce_result_t dce_handle_CWLAP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_info_t wifi_info;
    wifi_work_mode_e mode = wifi_get_opmode();
    wifi_scan_config_t scan_conf = {0};
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }

    char ssid[SSID_MAX_LEN] = {0};
    
    
    if (kind == DCE_WRITE) {
        int i = 0;
        for(i = 0;i < argc; i++){
            if(i==0){
                if(argv[i].type == ARG_TYPE_STRING){
                    memcpy(ssid, argv[i].value.string, strlen(argv[i].value.string));
                    scan_conf.ssid = (uint8_t *)ssid;
                    // quxin scan_hit_multi = true;
                } else if (argv[i].type == ARG_NOT_SPECIFIED){
                    continue;
                } else{
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_RC_OK;
                }
            }
            if(i==1){
                if(argv[i].type == ARG_TYPE_STRING && 
                   true == is_valid_mac(argv[i].value.string)){
                    uint8_t bssid[6];
                    hexStr2MACAddr((uint8_t*)argv[i].value.string, bssid);
                    scan_conf.bssid = (uint8_t*)&bssid[0];
                } else if (argv[i].type == ARG_NOT_SPECIFIED){
                    continue;
                } else {
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                }
            }
            if(i==2){
                if(argv[i].type != ARG_TYPE_NUMBER || 
                      argv[i].value.number >= 14 ||
                      argv[i].value.number <= 0){
                    
                    if(argv[i].type == ARG_NOT_SPECIFIED){
                        continue;
                    }
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    //quxin if(scan_hit_multi == true)
                    //quxin     scan_hit_multi = false;
                    return DCE_RC_OK;
                } else {
                    scan_conf.channel = argv[i].value.number;
                }
            }
            if(i==3){
                if(argv[i].type != ARG_TYPE_NUMBER || 
                      argv[i].value.number > 1 ||
                      argv[i].value.number < 0){
                    
                    if(argv[i].type == ARG_NOT_SPECIFIED){
                        continue;
                    }
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    //quxin if(scan_hit_multi == true)
                    //quxin     scan_hit_multi = false;
                    return DCE_RC_OK;
                } else {
                    scan_conf.passive = argv[i].value.number;
                    scan_conf.passive == 1 ? (scan_conf.scan_time = 360) : (scan_conf.scan_time = 120);
                }
            }
            if(i==4){
                if(argv[i].type != ARG_TYPE_NUMBER || 
                   argv[i].value.number < 0){
                    if(argv[i].type == ARG_NOT_SPECIFIED){
                        continue;
                    }
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);

                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    //quxin if(scan_hit_multi == true)
                    //quxin     scan_hit_multi = false;
                    return DCE_RC_OK;
                } else {
                    scan_conf.max_item = argv[i].value.number;
                }
            }
            if(i==5){
                if(argv[i].type != ARG_TYPE_NUMBER || 
                   argv[i].value.number < 0 || 
                   argv[i].value.number > 1500){
                    if(argv[i].type == ARG_NOT_SPECIFIED){
                        continue;
                    }
                    os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    //quxin if(scan_hit_multi == true)
                    //quxin     scan_hit_multi = false;
                    return DCE_RC_OK;
                } else {
                    scan_conf.scan_time = argv[i].value.number;
                }
            }
        }
        sys_err_t ret = wifi_scan_start(true, &scan_conf);
        if(ret != SYS_OK){
            os_printf(LM_APP, LL_INFO, "scan ret %d\n",ret);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            //quxin if(scan_hit_multi == true)
            //quxin             scan_hit_multi = false;
            return DCE_RC_OK;
        }
        int ap_num = wpa_get_scan_num();
        // printf("scan ap_num %d\n",ap_num);
        wifi_get_wifi_info(&wifi_info);
        wifi_status_e sta_status = wifi_get_status(STATION_IF);
        if (sta_status == STA_STATUS_CONNECTED && ap_num == 1 &&
            strcmp((const char*)wifi_info.ssid, (const char*)scan_conf.ssid)) {
            dce_emit_extended_result_code(dce, "CWLAP_CUR: THERE IS NO SOFTAP", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        }
        if (ap_num > 0) {
            int ap_print = 0;
            wifi_info_t *ap_list = (wifi_info_t*)calloc(ap_num, sizeof(wifi_info_t));
            if(ap_list == NULL){
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                //quxin if(scan_hit_multi == true)
                //quxin         scan_hit_multi = false;
                return DCE_RC_OK;
            }
            for(i = 0; i < ap_num; i++){
                wifi_info_t ap_info;
                wifi_get_scan_result(i, &ap_info);
                int len = strlen((const char *)ap_info.ssid);
                if(scan_conf.passive == 1 && len == 0)
                {
                    continue;
                }
                if(strcmp((const char*)ap_info.ssid, (const char*)scan_conf.ssid))
                {
                    continue;
                }
                memcpy(&ap_list[ap_print++],&ap_info,sizeof(wifi_info_t));
            }
            response_ap_info(dce, ap_print, ap_list);
            free(ap_list);
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        } else {
            dce_emit_extended_result_code(dce, "CWLAP_CUR: THERE IS NO SOFTAP", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        }
    } else {
        sys_err_t ret;
        scan_conf.max_item = 128;
        scan_conf.scan_time = 300;

        ret = wifi_scan_start(true, &scan_conf);
        if(ret != SYS_OK){
            os_printf(LM_APP, LL_INFO, "scan ret %d\n",ret);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        int ap_num = wpa_get_scan_num();
        os_printf(LM_APP, LL_INFO, "ap_num %d\n",ap_num);
        wifi_info_t *ap_list = (wifi_info_t*)calloc(ap_num, sizeof(wifi_info_t));
        if(ap_list == NULL){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        int i = 0;
        for(i = 0; i < ap_num; i++){
            wifi_info_t ap_info;
            wpa_get_scan_result(i,&ap_info);
            memcpy(&ap_list[i],&ap_info,sizeof(wifi_info_t));
        }
        response_ap_info(dce, ap_num, ap_list);
        free(ap_list);
        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_RC_OK;
    }


    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWQAP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    if (kind != DCE_EXEC){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    wifi_stop_station();
    /*dce_emit_extended_result_code(dce, "WIFI DISCONNECT", -1, 1);*/
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWQIF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    if (kind != DCE_EXEC){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    wifi_stop_softap();
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

#if 0
dce_result_t dce_handle_CWSAP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_ap_config_t softap_config;
    memset(&softap_config,0,sizeof(softap_config));
    if(DCE_WRITE == kind)
    {
    #define ARGV_PARA_VALID (argc < 4        || \
                argc > 6                        ||\
                 argv[0].type != ARG_TYPE_STRING || \
                 argv[1].type != ARG_TYPE_STRING || \
                 argv[2].type != ARG_TYPE_NUMBER || \
                 argv[3].type != ARG_TYPE_NUMBER)
             
        if(ARGV_PARA_VALID)
        {
            os_printf(LM_APP, LL_INFO, "invalid num of param\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        
        memcpy((char *)softap_config.ssid,argv[0].value.string,strlen(argv[0].value.string));
        memcpy(softap_config.password,argv[1].value.string,strlen(argv[1].value.string));
        softap_config.channel = argv[2].value.number;
        softap_config.authmode = argv[3].value.number;
        softap_config.max_connect = 4;
        softap_config.hidden_ssid = 0;
        if(argc == 5 && argv[4].type == ARG_TYPE_NUMBER)
        {
            softap_config.max_connect = argv[4].value.number;
        }
        else if(argc == 6 && \
            argv[4].type == ARG_TYPE_NUMBER && \
            argv[5].type == ARG_TYPE_NUMBER)
        {
            softap_config.max_connect = argv[4].value.number;
            softap_config.hidden_ssid = argv[5].value.number;
        }
        else if(argc != 4)
        {
            os_printf(LM_APP, LL_INFO, "invalid param of input\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        if(SYS_OK != wifi_start_softap((wifi_config_u *)&softap_config))
        {
            os_printf(LM_APP, LL_INFO, "err set softap fail\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;

        }
        wifi_save_ap_nv_info(&softap_config);
        if(softap_config.hidden_ssid)
            g_beacon_info.hidden_ssid = true;
    }
    else if(DCE_READ == kind)
    {
        wifi_load_ap_nv_info(&softap_config);
        arg_t args[] = {
            {ARG_TYPE_STRING, .value.string = (char *)softap_config.ssid},
            {ARG_TYPE_STRING, .value.string = softap_config.password},
            {ARG_TYPE_NUMBER, .value.number = softap_config.channel},
            {ARG_TYPE_NUMBER, .value.number = softap_config.authmode},
            {ARG_TYPE_NUMBER, .value.number = softap_config.max_connect},
            {ARG_TYPE_NUMBER, .value.number = softap_config.hidden_ssid},
        };
        dce_emit_extended_result_code_with_args(dce, "CWSAP", -1, args, 6, 1,false);
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}
#endif
extern uint8_t g_sta_addr[CFG_STA_MAX][ETH_ALEN];
extern uint8_t g_sta_index;
dce_result_t dce_handle_CWSAP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv, bool is_flash)
{
    wifi_config_u config;
    wifi_ap_config_t softap_config_tmp;

    if (dce_handle_flashEn_arg(kind, &argc, argv, &is_flash) != DCE_RC_OK) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    memset(&softap_config_tmp,0,sizeof(softap_config_tmp));
    if(DCE_WRITE == kind)
    {
    #define ARGV_PARA_VALID ( \
        argc >= 4 && argc <= 6 && \
        argv[0].type == ARG_TYPE_STRING && strlen(argv[0].value.string) <= WIFI_SSID_MAX_LEN && \
        argv[1].type == ARG_TYPE_STRING && ((strlen(argv[1].value.string) >= 8 && strlen(argv[1].value.string) <= WIFI_PWD_MAX_LEN) || argv[3].value.number == 0) && \
        argv[2].type == ARG_TYPE_NUMBER && \
        argv[3].type == ARG_TYPE_NUMBER && argv[3].value.number >= 0 && argv[3].value.number <= 4 && argv[3].value.number != 1 )

        if(!ARGV_PARA_VALID)
        {
            os_printf(LM_APP, LL_INFO, "invalid num of param\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        memset(&softap_config_tmp,0,sizeof(softap_config_tmp));
        memcpy((char *)softap_config_tmp.ssid,argv[0].value.string,strlen(argv[0].value.string));
        memcpy(softap_config_tmp.password,argv[1].value.string,strlen(argv[1].value.string));
        softap_config_tmp.channel = argv[2].value.number;
        wifi_work_mode_e opmode = wifi_get_opmode();
        if (opmode == WIFI_MODE_AP_STA) {
            wifi_status_e sta_status = wifi_get_status(STATION_IF);
            if (sta_status == STA_STATUS_CONNECTED) {
                uint8_t rf_channel = wifi_rf_get_channel();
                if (softap_config_tmp.channel != rf_channel) {
                    softap_config_tmp.channel = rf_channel;
                }
            }
        }
        softap_config_tmp.authmode = argv[3].value.number;
        softap_config_tmp.max_connect = 8;
        softap_config_tmp.hidden_ssid = 0;
        if(argc == 5)
        {
            if(argv[4].type == ARG_TYPE_NUMBER && argv[4].value.number >= 0 && argv[4].value.number <= 8)
            {
                softap_config_tmp.max_connect = argv[4].value.number;
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "invalid num of param 4\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_OK;
            }
            
            softap_config_tmp.max_connect = argv[4].value.number;
        }
        else if(argc == 6)
        {
            if (argv[4].type == ARG_TYPE_NUMBER && argv[4].value.number >= 0 && argv[4].value.number <= 8 &&
                argv[5].type == ARG_TYPE_NUMBER && argv[5].value.number >= 0 && argv[5].value.number <= 1)
            {
                softap_config_tmp.max_connect = argv[4].value.number;
                softap_config_tmp.hidden_ssid = argv[5].value.number;
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "invalid num of param 5\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_OK;
            }
            

            
        }
        else if(argc != 4)
        {
            os_printf(LM_APP, LL_INFO, "invalid param of input\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        memcpy(&config.ap, &softap_config_tmp, sizeof(wifi_ap_config_t));
        if(SYS_OK != wifi_start_softap(&config))
        {
            os_printf(LM_APP, LL_INFO, "err set softap fail\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;

        }
        if(is_flash){
            wifi_save_ap_nv_info(&softap_config_tmp);
        }

//        softap_config = softap_config_tmp;
    }
    else if(DCE_READ == kind)
    {
        memset(&softap_config_tmp,0,sizeof(softap_config_tmp));
        wifi_load_ap_nv_info(&softap_config_tmp);
        arg_t args_DFE[] = {
            {ARG_TYPE_STRING, .value.string = (char *)softap_config_tmp.ssid},
            {ARG_TYPE_STRING, .value.string = (char *)softap_config_tmp.password},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.channel},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.authmode},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.max_connect},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.hidden_ssid},
        };
        dce_emit_extended_result_code_with_args(dce, "CWSAP_DEF", -1, args_DFE, 6, 1,false);

        wifi_status_e ap_status = wifi_get_status(SOFTAP_IF);
        os_printf(LM_APP, LL_INFO, "======ap_status=%d====\n", ap_status);
        if (ap_status == AP_STATUS_STOP) {
            dce_emit_extended_result_code(dce, "CWSAP_CUR: NOT CREATE SOFTAP", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        memset(&softap_config_tmp, 0, sizeof(softap_config_tmp));
        memcpy(&softap_config_tmp, &softap_config, sizeof(softap_config_tmp));
        arg_t args_CUR[] = {
            {ARG_TYPE_STRING, .value.string = (char *)softap_config_tmp.ssid},
            {ARG_TYPE_STRING, .value.string = (char *)softap_config_tmp.password},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.channel},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.authmode},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.max_connect},
            {ARG_TYPE_NUMBER, .value.number = softap_config_tmp.hidden_ssid},
        };
        dce_emit_extended_result_code_with_args(dce, "CWSAP_CUR", -1, args_CUR, 6, 1,false);

        for (int i = 0; i < g_sta_index; i++) {
            char mac_str[MACSTR_LEN + 1];
            sprintf(mac_str, MACSTR, MAC2STR(g_sta_addr[i]));
            arg_t args_sta[] = {
                {ARG_TYPE_NUMBER, .value.number = i},
                {ARG_TYPE_STRING, .value.string = mac_str},
            };
            dce_emit_extended_result_code_with_args(dce, "STA_INFO", -1, args_sta, 2, 1,false);
        }
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}
dce_result_t dce_handle_CWSAP_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return dce_handle_CWSAP(dce,group_ctx,kind,argc,argv,false);
}
dce_result_t dce_handle_CWSAP_DEF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return dce_handle_CWSAP(dce,group_ctx,kind,argc,argv,false);
}

dce_result_t dce_handle_CWLIF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{

    if(DCE_EXEC == kind)
    {
#if 0 //quxin
        char ip_str[16], mac_str[18];
        struct sta_info *sta;
        struct hostapd_data *hostapd;

        if (AP_STATUS_STARTED == wifi_get_ap_status()) {
            extern struct hostapd_data * wifi_ap_get_apdata(int vif_id);
            extern unsigned int wifi_get_sta_ip_from_mac(unsigned char *mac);

            hostapd = wifi_ap_get_apdata(1);
            for(sta=hostapd->sta_list; sta!=NULL; sta=sta->next)
            {
                ip_addr_t ip_info;
                ip_info.addr = wifi_get_sta_ip_from_mac(sta->addr);
                
                arg_t args[] = {
                    {ARG_TYPE_STRING, .value.string = ip_str},
                    {ARG_TYPE_STRING, .value.string = mac_str},
                };
                memset(ip_str, 0, sizeof(ip_str));
                memset(mac_str, 0, sizeof(mac_str));

                snprintf(ip_str, sizeof(ip_str), IP4_ADDR_STR, IP4_ADDR_VALUE(&ip_info));
                snprintf(mac_str, sizeof(mac_str), MAC_STR, MAC_VALUE(sta->addr));
                dce_emit_extended_result_code_with_args(dce, "CWLIF", -1, args, 2, 1,false);

            }
        }
#endif
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWDHCP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    char handle[] = "CWDHCP_DEF";
    bool dhcp_ap_status  = true;
    bool dhcp_sta_status = true;
    bool flash_en        = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "dhcp argc:%d, flash:%d\n", argc, flash_en);

    if (kind & DCE_READ)
    {
        dhcp_ap_status = get_wifi_dhcp_use_status(SOFTAP_IF);
        dhcp_sta_status = get_wifi_dhcp_use_status(STATION_IF);
        memcpy(handle,"CWDHCP_CUR",sizeof("CWDHCP_CUR"));
        arg_t result_cur[1] = {
            {ARG_TYPE_NUMBER, .value.number = dhcp_sta_status << 1 | dhcp_ap_status},
        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_cur, 1, 1, false);
        // check flash
        if(!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)) || 
           !hal_system_get_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status)))
        {
            dce_emit_extended_result_code(dce, "FLASH IS NULL, NEED WRITE FIRST", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        
        memcpy(handle,"CWDHCP_DEF",sizeof("CWDHCP_CUR"));
        arg_t result_def[1] = {
            {ARG_TYPE_NUMBER, .value.number = dhcp_sta_status << 1 | dhcp_ap_status},
        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_def, 1, 1, false);

    }
    else if((kind & DCE_WRITE) && 
            argc == 2 && argv[0].type == ARG_TYPE_NUMBER && argv[1].type == ARG_TYPE_NUMBER &&
            argv[0].value.number >= 0 && argv[0].value.number <= 2 &&
            argv[1].value.number >= 0 && argv[1].value.number <=1 )
    {
        // close
        if(argv[1].value.number == 0)
        {
            switch (argv[0].value.number)
            {
            case 0: 
                wifi_dhcp_close(SOFTAP_IF);
                dhcp_ap_status = 0; 
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)) : 0;
                break;
            case 1: 
                wifi_dhcp_close(STATION_IF); 
                dhcp_sta_status = 0; 
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status)) : 0;
                break;            
            default:
                wifi_dhcp_close(SOFTAP_IF);
                wifi_dhcp_close(STATION_IF); 
                dhcp_ap_status = 0; 
                dhcp_sta_status = 0; 
                if(flash_en)
                {
                    hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)) ; 
                    hal_system_set_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status));
                }
                break;
            }
        }
        // open 
        else
        {
            switch (argv[0].value.number)
            {
            case 0: 
                wifi_dhcp_open(SOFTAP_IF); 
                dhcp_ap_status = 1; 
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)) : 0;
                break;
            case 1: 
                wifi_dhcp_open(STATION_IF);
                dhcp_sta_status = 1; 
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status)) : 0;
                break;            
            default:
                wifi_dhcp_open(SOFTAP_IF);
                wifi_dhcp_open(STATION_IF);
                dhcp_ap_status = 1; 
                dhcp_sta_status = 1; 
                if(flash_en)
                {
                    hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)) ; 
                    hal_system_set_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status));
                }
                break;
            }
        }
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t at_softap_set_dhcps_lease(dce_t* dce, struct dhcps_lease *please, uint32_t lease_time, bool reset)
{
    if(reset)
    {
        u32_t ip;
        u32_t local_ip = 0;
        u32_t softap_ip = 0;

        please->enable = 1;

        netif_db_t *netif_db;

        netif_db = get_netdb_by_index(SOFTAP_IF);

        ip = ip_2_ip4(&netif_db->ipconfig.ip)->addr;

        local_ip = softap_ip = htonl(ip);
        softap_ip &= 0xFFFFFF00;
        local_ip &= 0xFF;
 
        if (local_ip >= 0x80) 
        {
            local_ip -= DHCPS_MAX_LEASE;
        } 
        else 
        {
            local_ip ++;
        }

        ip_2_ip4(&please->start_ip)->addr = softap_ip | local_ip;
        ip_2_ip4(&please->end_ip)->addr = softap_ip | (local_ip + DHCPS_MAX_LEASE - 1);
        ip_2_ip4(&please->start_ip)->addr = htonl(ip_2_ip4(&please->start_ip)->addr);
        ip_2_ip4(&please->end_ip)->addr = htonl(ip_2_ip4(&please->end_ip)->addr);

		set_softap_dhcps_cfg(please);
    
        if(!wifi_softap_set_dhcps_lease(please))
        {
            os_printf(LM_APP, LL_INFO, "reset DHCP SERVER ERROR \r\n");
            // dce_emit_extended_result_code(dce, "RESET TCPIP_DHCP_STARTED", -1, 1);
            return DCE_RC_ERROR;
        }
        wifi_softap_reset_dhcps_lease_time();
        return DCE_OK;
    }

	set_softap_dhcps_cfg(please);
    
    if(!wifi_softap_set_dhcps_lease(please))
    {
        os_printf(LM_APP, LL_INFO, "set DHCP SERVER ERROR \r\n");
        // dce_emit_extended_result_code(dce, "TCPIP_DHCP_STARTED", -1, 1);
        return DCE_RC_ERROR;
    }
    wifi_softap_set_dhcps_lease_time(lease_time);

    return DCE_OK;
}
dce_result_t dce_handle_CWDHCPS(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    bool flash_en = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "dhcps argc:%d, flash:%d\n", argc, flash_en);

    if(!get_wifi_dhcp_use_status(SOFTAP_IF))
    {
        os_printf(LM_APP, LL_INFO, "dhcp not started\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    struct dhcps_lease please = {0};
    char start_ip[17], end_ip[17];
    char handle[] = "CWDHCPS_DEF";
    uint32_t lease_time = 0;
    dce_result_t ret;

    if(kind & DCE_READ)
    {
        lease_time = wifi_softap_get_dhcps_lease_time();

        if(!wifi_softap_get_dhcps_lease(&please))
        {
            os_printf(LM_APP, LL_INFO, "dhcp is stopped\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        sprintf(start_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&please.start_ip)));
        sprintf(end_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&please.end_ip)));
        memcpy(handle,"CWDHCPS_CUR",sizeof("CWDHCPS_CUR"));
        
        arg_t result_cur[] = {
            {ARG_TYPE_NUMBER, .value.number = lease_time},
            {ARG_TYPE_STRING, .value.string = start_ip},
            {ARG_TYPE_STRING, .value.string = end_ip},
        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_cur, 3, 1, false);

        // check flash dhcps_lease
        if(!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_IP, &(please), sizeof(please)) ||
           !hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME, &(lease_time), sizeof(lease_time)))
        {
            dce_emit_extended_result_code(dce, "FLASH IS NULL, NEED WRITE FIRST", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        sprintf(start_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&please.start_ip)));
        sprintf(end_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&please.end_ip)));
        memcpy(handle,"CWDHCPS_DEF",sizeof("CWDHCPS_DEF"));
        
        arg_t result[] = {
            {ARG_TYPE_NUMBER, .value.number = lease_time},
            {ARG_TYPE_STRING, .value.string = start_ip},
            {ARG_TYPE_STRING, .value.string = end_ip},
        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result, 3, 1, false);
    }
    else if(kind & DCE_WRITE) 
    {
        // Clear Setting Restore default values
        if( argc == 1 && argv[0].value.number == 0 )
        {
        	lease_time = wifi_softap_get_dhcps_lease_time();
			please.dhcps_lease = lease_time;
			
            ret = at_softap_set_dhcps_lease(dce, &please, 0, true);
            if(ret != DCE_OK)
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return ret;
            }

            flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_IP, &(please), sizeof(please)) : 0;
            flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME, &(lease_time), sizeof(lease_time)) : 0;

        }
        else if (argc == 4 && 
            argv[0].type == ARG_TYPE_NUMBER && 
            argv[1].type == ARG_TYPE_NUMBER &&
            argv[2].type == ARG_TYPE_STRING && 
            argv[3].type == ARG_TYPE_STRING &&
            argv[0].value.number >= 0 && argv[0].value.number <= 1 &&
            argv[1].value.number >= 1 && argv[1].value.number <= 2880)
        {
            // Clear Setting Restore default values
            if(argv[0].value.number == 0)
            {
            	lease_time = wifi_softap_get_dhcps_lease_time();
				please.dhcps_lease = lease_time;
				
                ret = at_softap_set_dhcps_lease(dce, &please, 0, true);
                if(ret != DCE_OK)
                {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return ret;
                }
                
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_IP, &(please), sizeof(please)) : 0;
                flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME, &(lease_time), sizeof(lease_time)) : 0;
            }

            // set DHCPS
            lease_time = argv[1].value.number;
            please.enable = 1;
			please.dhcps_lease = lease_time;
            if (!ipaddr_aton(argv[2].value.string, &(please.start_ip)) ||
                !ipaddr_aton(argv[3].value.string, &(please.end_ip)) )
            {
                os_printf(LM_APP, LL_INFO, "ip format err \r\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }

            
            ret = at_softap_set_dhcps_lease(dce, &please, lease_time, false);
            if(ret != DCE_OK)
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return ret;
            }

            flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_IP, &(please), sizeof(please)) : 0;
            flash_en == true ? hal_system_set_config(CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME, &(lease_time), sizeof(lease_time)) : 0;
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CWAUTOCONN(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    uint8_t auto_conn = 1;
    if(kind == DCE_WRITE)
    {
        if (argc != 1 || argv[0].type != ARG_TYPE_NUMBER){
            os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        auto_conn = argv[0].value.number;
        if(auto_conn > 1)
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        wifi_nv_info_t *wifi_nv_info = get_wifi_nv_info();
        wifi_nv_info->auto_conn = auto_conn;
        if(hal_system_set_config(CUSTOMER_NV_WIFI_AUTO_CONN, &wifi_nv_info->auto_conn, sizeof(wifi_nv_info->auto_conn)))
        {
            os_printf(LM_APP, LL_INFO, "save nv fail %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;

        }
        dce_emit_basic_result_code(dce, DCE_RC_OK);
    }
    return DCE_OK;
}

extern uint8_t g_balconn;
dce_result_t dce_handle_CWBALCONN(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    uint8_t bal_conn = 1;
    if(kind == DCE_WRITE)
    {
        if (argc != 1 || argv[0].type != ARG_TYPE_NUMBER){
            os_printf(LM_APP, LL_INFO, "invalid arguments %d\n",__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        bal_conn = argv[0].value.number;
        if(bal_conn > 1)
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
         g_balconn = bal_conn;
         dce_emit_basic_result_code(dce, DCE_RC_OK);
    }
    return DCE_OK;
}


#if 0 //quxin
static sc_result_t result;
static void sc_callback(smartconfig_status_t status, void *pdata)
{ 
    int ret = 0;
    if(status == SC_STATUS_GOT_SSID_PSWD) 
    {
        memcpy(&result,pdata,sizeof(sc_result_t));
        smartconfig_stop();
    } else if(status == SC_STATUS_STOP){
        os_printf(LM_APP, LL_INFO, "GOT SSID %s len %d\n",result.ssid,result.ssid_length);
        if(result.ssid[0] != 0 && result.ssid_length != 0) {
            wifi_config_u conf;
            memset(&conf, 0, sizeof(wifi_config_u));
            strcpy((char *)conf.sta.ssid, result.ssid);
            strcpy(conf.sta.password, result.pwd);
            ret |= wifi_stop_station();
            ret |= wifi_start_station(&conf);
            if(ret != 0){
                os_printf(LM_APP, LL_INFO, "CONNECT AP ERROR %d\n",ret);
            }
        }
        memset(&result, 0 ,sizeof(sc_result_t));
    } else if(status == SC_STATUS_TIMEOUT){
        memset(&result, 0 ,sizeof(sc_result_t));
        smartconfig_stop();
    }
}
#endif

dce_result_t dce_handle_CWSTARTSMART(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    if (kind & DCE_EXEC)
    {
        if(-1 ==smartconfig_start(sc_callback))
        {
            dce_emit_extended_result_code(dce, "SMARTCONFIG IS ALREADY STARTED", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        }
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    return DCE_OK;
}

dce_result_t dce_handle_CWSTOPSMART(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_work_mode_e mode = wifi_get_opmode();
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
    {
        os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_OK;
    }
    if (kind & DCE_EXEC)
    {
        if(-1 == smartconfig_stop())
        {
            dce_emit_extended_result_code(dce, "SMARTCONFIG IS ALREADY STOPPED", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

static void set_netif_hostname(const char *hostname)
{
    if(hostname == NULL)
        return;

    int length = strlen(hostname)+1;
    struct netif *netif_temp = get_netif_by_index(STATION_IF);
    free((void *)netif_temp->hostname);
    netif_temp->hostname = malloc(sizeof(char) * length);
    memset((char *)netif_temp->hostname,0,length);
    memcpy((char *)netif_temp->hostname,hostname,length);
}
dce_result_t dce_handle_CWHOSTNAME(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    #define MAX_HOSTNAME_LEN 32
    wifi_work_mode_e mode = wifi_get_opmode();
    char hostname[MAX_HOSTNAME_LEN+1]={0};
    static int hostname_isset=0;
    int i=0;
    int pos=0;

    if (kind & DCE_READ)
    {
        if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
        {
            arg_t result[] = {{ARG_TYPE_STRING, .value.string = "null"}};
            dce_emit_extended_result_code_with_args(dce, "CWHOSTNAME", -1, result, 1, 1, false);
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        }

        struct netif *netif_temp = get_netif_by_index(STATION_IF);

        if(hostname_isset)
        {
            memcpy(hostname,netif_temp->hostname,strlen(netif_temp->hostname));
        }
        else
        {
            for (i=0;i<NETIF_MAX_HWADDR_LEN;i++)
            {
                pos+=sprintf(hostname+pos,"%02x",netif_temp->hwaddr[i]);
            }
        }

        arg_t result[] = {{ARG_TYPE_STRING, .value.string = hostname}};

        dce_emit_extended_result_code_with_args(dce, "CWHOSTNAME", -1, result, 1, 1, false);

        dce_emit_basic_result_code(dce, DCE_RC_OK);

        return DCE_RC_OK;
    }
    else if ((kind & DCE_WRITE))
    {
        if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

        if (argc == 1 && argv[0].type == ARG_TYPE_STRING && strlen(argv[0].value.string)<=MAX_HOSTNAME_LEN)
        {

            // struct netif *netif_temp = get_netif_by_index(STATION_IF);
            // netif_temp->hostname = argv[0].value.string;
            set_netif_hostname(argv[0].value.string);
            hostname_isset=1;

            dce_emit_basic_result_code(dce, DCE_RC_OK);

            return DCE_RC_OK;
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

#if 0 //quxin
static const char *const us_op_class_cc[] = {
    "US", "CA", NULL
};

static const char *const eu_op_class_cc[] = {
    "AL", "AM", "AT", "AZ", "BA", "BE", "BG", "BY", "CH", "CY", "CZ", "DE",
    "DK", "EE", "EL", "ES", "FI", "FR", "GE", "HR", "HU", "IE", "IS", "IT",
    "LI", "LT", "LU", "LV", "MD", "ME", "MK", "MT", "NL", "NO", "PL", "PT",
    "RO", "RS", "RU", "SE", "SI", "SK", "TR", "UA", "UK", NULL
};

static const char *const jp_op_class_cc[] = {
    "JP", NULL
};

static const char *const cn_op_class_cc[] = {
    "CN", NULL
};
static int country_match(const char *const cc[], const char *const country)
{
    int i;

    if (country == NULL)
        return 0;
    for (i = 0; cc[i]; i++) {
        if (cc[i][0] == country[0] && cc[i][1] == country[1])
            return 1;
    }

    return 0;
}
static int wifi_driver_nrc_set_country(const char *country)
{
   wifi_country_t info;

   if (!country) 
       return -1;
   
   memset(&info, 0, sizeof(info));
   memcpy(info.cc, country, 2);

   info.schan = 1;
   if (country_match(us_op_class_cc, country)) {
       info.nchan = 11;
   } else if (country_match(eu_op_class_cc, country)) {
       info.nchan = 13;
   } else if (country_match(jp_op_class_cc, country)) {
       info.nchan = 14;
   } else if (country_match(cn_op_class_cc, country)) {
       info.nchan = 13;
   } else {
       info.nchan = 13;
   }

   system_modem_api_set_country_info(&info);
   return 0;
}
#endif

uint8_t g_auto_set_country = 0;
static void country_paramm_init(void)
{
    //quxin if(!ef_get_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &g_auto_set_country, sizeof(g_auto_set_country), NULL)) 
    {
        os_printf(LM_APP, LL_INFO, "flash auto set country read null, set 0\r\n");

        //quxin ef_set_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &g_auto_set_country, sizeof(g_auto_set_country));
        //quxin ef_get_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &g_auto_set_country, sizeof(g_auto_set_country), NULL);
    }
}

#if 0
static dce_result_t dce_handle_CWCOUNTRY(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv, bool is_flash)
{
#if 0 //quxin
    wifi_country_t wifi_country_tmp;
    uint8_t auto_set_country = 0;

    if (kind & DCE_READ)
    {
        if(is_flash){

            //if(!ef_get_env_blob(NV_WIFI_COUNTRY, wifi_country_tmp.cc, sizeof(wifi_country_tmp.cc), NULL)) 
            {
                char cc[] = "CN";
                os_printf(LM_APP, LL_INFO, "flash country read null, set china\r\n");

                //ef_set_env_blob(NV_WIFI_COUNTRY, cc, sizeof(cc));
                //ef_get_env_blob(NV_WIFI_COUNTRY, &(wifi_country_tmp.cc), sizeof(wifi_country_tmp.cc), NULL);
            }

            //if(!ef_get_env_blob(NV_WIFI_START_CHANNEL, &(wifi_country_tmp.schan), sizeof(wifi_country_tmp.schan), NULL)) 
            {
                uint8_t schan = 1;
                os_printf(LM_APP, LL_INFO, "flash start channel read null, set 1\r\n");

                //ef_set_env_blob(NV_WIFI_START_CHANNEL, &schan, sizeof(schan));
                //ef_get_env_blob(NV_WIFI_START_CHANNEL, &(wifi_country_tmp.schan), sizeof(wifi_country_tmp.schan), NULL);
            }

            //if(!ef_get_env_blob(NV_WIFI_TOTAL_CHANNEL_NUMBEL, &(wifi_country_tmp.nchan), sizeof(wifi_country_tmp.nchan), NULL)) 
            {
                uint8_t nchan = 13;
                os_printf(LM_APP, LL_INFO, "flash total channel read null, set 13\r\n");

                //ef_set_env_blob(NV_WIFI_TOTAL_CHANNEL_NUMBEL, &nchan, sizeof(nchan));
                //ef_get_env_blob(NV_WIFI_TOTAL_CHANNEL_NUMBEL, &(wifi_country_tmp.nchan), sizeof(wifi_country_tmp.nchan), NULL);
            }

            //if(!ef_get_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &auto_set_country, sizeof(auto_set_country), NULL)) 
            {
                os_printf(LM_APP, LL_INFO, "flash auto set country read null, set 0\r\n");

                //ef_set_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &auto_set_country, sizeof(auto_set_country));
                //ef_get_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &auto_set_country, sizeof(auto_set_country), NULL);
            }
        }else{
            system_modem_api_get_country_info(&wifi_country_tmp);
            auto_set_country = g_auto_set_country;
        }
        

        arg_t result[] = {
            {ARG_TYPE_NUMBER, .value.number = auto_set_country},
            {ARG_TYPE_STRING, .value.string = wifi_country_tmp.cc},
            {ARG_TYPE_NUMBER, .value.number = wifi_country_tmp.schan},
            {ARG_TYPE_NUMBER, .value.number = wifi_country_tmp.nchan}
        };

        dce_emit_extended_result_code_with_args(dce, "CWCOUNTRY", -1, result, 4, 1, false);

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_OK;
    }
    else if(kind & DCE_WRITE)
    {
        if ((argc == 4 &&
            argv[0].type == ARG_TYPE_NUMBER &&
            argv[1].type == ARG_TYPE_STRING &&
            argv[2].type == ARG_TYPE_NUMBER &&
            argv[3].type == ARG_TYPE_NUMBER && 
            (argv[0].value.number == 0 || argv[0].value.number == 1) &&
            2 == strlen(argv[1].value.string)) ||  
            (argc == 1 && argv[0].value.number == 0))
        {
            auto_set_country = argv[0].value.number;

            if(auto_set_country == 0){
                //if(!ef_get_env_blob(NV_WIFI_COUNTRY, wifi_country_tmp.cc, sizeof(wifi_country_tmp.cc), NULL)) 
                {
                    char cc[] = "CN";
                    os_printf(LM_APP, LL_INFO, "flash country read null, set china\r\n");

                    //ef_set_env_blob(NV_WIFI_COUNTRY, cc, sizeof(cc));
                    //ef_get_env_blob(NV_WIFI_COUNTRY, &(wifi_country_tmp.cc), sizeof(wifi_country_tmp.cc), NULL);
                }
                wifi_driver_nrc_set_country(wifi_country_tmp.cc);
                g_auto_set_country = auto_set_country;

                if(is_flash){
                    system_modem_api_get_country_info(&wifi_country_tmp);
                    
                    //ef_set_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &auto_set_country, sizeof(auto_set_country));
                    //ef_set_env_blob(NV_WIFI_COUNTRY, wifi_country_tmp.cc, sizeof(wifi_country_tmp.cc));
                    //ef_set_env_blob(NV_WIFI_START_CHANNEL, &(wifi_country_tmp.schan), sizeof(wifi_country_tmp.schan));
                    //ef_set_env_blob(NV_WIFI_TOTAL_CHANNEL_NUMBEL, &(wifi_country_tmp.nchan), sizeof(wifi_country_tmp.nchan));   
                }
            } 
            else{
                memcpy(wifi_country_tmp.cc, argv[1].value.string, sizeof(wifi_country_tmp.cc));
                wifi_country_tmp.schan = argv[2].value.number;
                wifi_country_tmp.nchan = argv[3].value.number;
                if(system_modem_api_set_country_info(&wifi_country_tmp))
                {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_INVALID_INPUT;
                }
                g_auto_set_country = auto_set_country;
                
                if(is_flash){
                    //ef_set_env_blob(NV_WIFI_AUTO_SET_COUNTRY, &auto_set_country, sizeof(auto_set_country));
                    //ef_set_env_blob(NV_WIFI_COUNTRY, wifi_country_tmp.cc, sizeof(wifi_country_tmp.cc));
                    //ef_set_env_blob(NV_WIFI_START_CHANNEL, &(wifi_country_tmp.schan), sizeof(wifi_country_tmp.schan));
                    //ef_set_env_blob(NV_WIFI_TOTAL_CHANNEL_NUMBEL, &(wifi_country_tmp.nchan), sizeof(wifi_country_tmp.nchan));
                }
            }

            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_OK;
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_INVALID_INPUT;
        }
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_INVALID_INPUT;
    }
    

    dce_emit_basic_result_code(dce, DCE_RC_OK);
#endif
    return DCE_OK;
}


static dce_result_t dce_handle_CWCOUNTRY_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return dce_handle_CWCOUNTRY(dce,group_ctx,kind,argc,argv,false);
}
static dce_result_t dce_handle_CWCOUNTRY_DEF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return dce_handle_CWCOUNTRY(dce,group_ctx,kind,argc,argv,true);
}
#endif


dce_result_t dce_handle_CWINFO(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_info_t *wifi_info;
    wifi_ap_config_t *softap_info;

    wifi_work_mode_e mode = wifi_get_opmode();



    wifi_info = (wifi_info_t*)calloc(1, sizeof(wifi_info_t));
    softap_info = (wifi_ap_config_t*)calloc(1, sizeof(wifi_info_t));

    if(kind == DCE_EXEC) {
        if (mode == WIFI_MODE_STA) {
            wifi_get_wifi_info(wifi_info);
            response_ap_info(dce, 1, wifi_info);
        } else if (mode == WIFI_MODE_AP) {
            wifi_get_softap_info(softap_info);
            response_softap_info(dce, 1, softap_info);
        } else if (mode == WIFI_MODE_AP_STA) {
            wifi_get_wifi_info(wifi_info);
            wifi_get_softap_info(softap_info);
            response_ap_info(dce, 1, wifi_info);
            response_softap_info(dce, 1, softap_info);
        } else {
            os_printf(LM_APP, LL_INFO, "Invilad para\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        }
    }
    free(wifi_info);
    free(softap_info);
    dce_emit_basic_result_code(dce, DCE_RC_OK);

    return DCE_OK;
#if 0
    if(kind == DCE_EXEC) {
        os_printf(LM_APP, LL_INFO, "=====kind == DCE_EXEC====\n");
        sys_err_t ret;
        scan_conf.max_item = 128;
        scan_conf.scan_time = 300;

        ret = wifi_scan_start(true, &scan_conf);
        if(ret != SYS_OK){
            os_printf(LM_APP, LL_INFO, "scan ret %d\n",ret);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        int ap_num = wpa_get_scan_num();
        os_printf(LM_APP, LL_INFO, "ap_num %d\n",ap_num);
        wifi_info_t *ap_list = (wifi_info_t*)calloc(ap_num, sizeof(wifi_info_t));
        if(ap_list == NULL){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        int i = 0;
        for(i = 0; i < ap_num; i++){
            wifi_info_t ap_info;
            wpa_get_scan_result(i,&ap_info);
            memcpy(&ap_list[i],&ap_info,sizeof(wifi_info_t));
        }
        response_ap_info(dce, ap_num, ap_list);
        free(ap_list);
        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_RC_OK;
    }


    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
#endif
}

static const command_desc_t CW_commands[] = {
//    {"CWMODE_CUR"   , &dce_handle_CWMODE_CUR,    DCE_TEST| DCE_WRITE | DCE_READ},
    {"CWMODE"       , &dce_handle_CWMODE,    	 DCE_TEST| DCE_WRITE | DCE_READ},
//    {"CWJAP_CUR"    , &dce_handle_CWJAP_CUR,     DCE_WRITE | DCE_READ},
    {"CWJAP"        , &dce_handle_CWJAP_DEF,     DCE_WRITE | DCE_READ},
//    {"CWLAPOPT"     , &dce_handle_CWLAPOPT,      DCE_WRITE},
    {"CWLAP"        , &dce_handle_CWLAP,         DCE_WRITE | DCE_READ},
    {"CWQAP"        , &dce_handle_CWQAP,         DCE_EXEC},
    {"CWQIF"        , &dce_handle_CWQIF,         DCE_EXEC},
//    {"CWSAP_CUR"    , &dce_handle_CWSAP_CUR,     DCE_WRITE | DCE_READ},
    {"CWSAP"        , &dce_handle_CWSAP_DEF,     DCE_WRITE | DCE_READ},
//    {"CWLIF"        , &dce_handle_CWLIF,         DCE_EXEC},
    {"CWDHCP"       , &dce_handle_CWDHCP,        DCE_WRITE | DCE_READ},
    {"CWDHCPS"      , &dce_handle_CWDHCPS,       DCE_WRITE | DCE_READ},
    {"CWAUTOCONN"   , &dce_handle_CWAUTOCONN,    DCE_WRITE},
    {"CWBALCONN"   , &dce_handle_CWBALCONN,    DCE_WRITE},
//    {"CWSTARTSMART" , &dce_handle_CWSTARTSMART,  DCE_EXEC},
//    {"CWSTOPSMART"  , &dce_handle_CWSTOPSMART,   DCE_EXEC},
//    {"CWHOSTNAME"   , &dce_handle_CWHOSTNAME,    DCE_WRITE | DCE_READ},
//    {"CWCOUNTRY_CUR", &dce_handle_CWCOUNTRY_CUR, DCE_WRITE | DCE_READ},
//    {"CWCOUNTRY_DEF", &dce_handle_CWCOUNTRY_DEF, DCE_WRITE | DCE_READ},
//    {"CWINFO"       , &dce_handle_CWINFO,         DCE_WRITE | DCE_EXEC},
};

void at_init_wifi_event_group(void)
{   
    if(at_wifi_event_group) {
        return;
    }
    at_wifi_event_group = xEventGroupCreate();
}

void dce_register_wifi_commands(dce_t* dce)
{
    dce_register_command_group(dce, "WIFI", CW_commands, sizeof(CW_commands) / sizeof(command_desc_t), 0);
    sys_event_loop_init(at_wifi_event_handler, dce);
    at_init_wifi_event_group();
    country_paramm_init();
    connect_list_init();
}
