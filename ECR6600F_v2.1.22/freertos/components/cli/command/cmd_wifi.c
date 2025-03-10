/**
 * @file cmd_wifi.c
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
#include "system_def.h"
#include "system_config.h"
#include "format_conversion.h"
#ifdef CONFIG_WIFI_FHOST
#include "system_wifi.h"
#include "common.h"
#endif
#ifdef ENABLE_LWIP_NAPT
#include "system_lwip.h"
#endif

#define MAC_LEN 17
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

#ifdef CONFIG_WIRELESS_WPA_SUPPLICANT
extern sys_err_t wifi_stop_station(void);
static int cmd_wifi_stop_station(cmd_tbl_t *t, int argc, char *argv[])
{
	wifi_stop_station();
	return CMD_RET_SUCCESS;
}
CLI_CMD(wifi_stop_sta, cmd_wifi_stop_station,  "wifi_stop_sta", "wifi_stop_sta");

static int cmd_wifi_sta(cmd_tbl_t *t, int argc, char *argv[])
{
    wifi_config_u config;

    if (argc < 2 || argc > 3)
    {
        return CMD_RET_USAGE;
    }

    memset(&config, 0, sizeof(config));
    strcpy((char *)config.sta.ssid, argv[1]);
    if (argc > 2)
    {
        strncpy(config.sta.password, argv[2], sizeof(config.sta.password) - 1);
    }

    wifi_stop_station();
    os_msleep(500);
    return wifi_start_station(&config) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}
CLI_CMD(wifi_sta, cmd_wifi_sta,  "connect wifi as station", "wifi_sta <ssid> [password]");


static int cmd_chineses_wifi_sta(cmd_tbl_t * t, int argc, char * argv[])
{

	wifi_config_u config;

	memset(&config,0,sizeof(config));

	if(0== strcmp(argv[1],"test1"))
		strcpy((char *)config.sta.ssid,"!@#￥%*\\");
	else if(0== strcmp(argv[1],"test2"))
		
		strcpy((char *)config.sta.ssid,"中文测试SSID");

	strncpy(config.sta.password,argv[2],sizeof(config.sta.password)-1);


	wifi_stop_station();
	os_msleep(500);
	return wifi_start_station(&config) ? CMD_RET_FAILURE:CMD_RET_SUCCESS;
}
CLI_CMD(wifi_chinenese_sta, cmd_chineses_wifi_sta,  "connect wifi as station", "wifi_sta <ssid> [password]");
extern uint16_t get_macsw_sta_max_cnt(void);
static int cmd_wifi_softap(cmd_tbl_t * t, int argc, char * argv[])
{
    wifi_config_u   config;
    int             i, j;

    if (argc == 1)
    {
        return CMD_RET_USAGE;
    }

    memset(&config, 0, sizeof(config));
    config.ap.authmode  = AUTH_OPEN;
    config.ap.channel   = 6;
    config.ap.max_connect = get_macsw_sta_max_cnt();
    config.ap.hidden_ssid = 0;

    for (i = 1; i <= argc; i += 2)
    {
        if ((strcmp(argv[i], "--ssid") == 0) || (strcmp(argv[i], "-s") == 0))
        {
            strcpy((char *) config.ap.ssid, argv[i + 1]);
            continue;
        }

        if ((strcmp(argv[i], "--password") == 0) || (strcmp(argv[i], "-p") == 0))
        {
            strncpy(config.ap.password, argv[i + 1], sizeof(config.ap.password) - 1);
            config.ap.authmode  = AUTH_WPA_WPA2_PSK;

            for (j = 1; j <= argc; j += 2)
            {
                if ((strcmp(argv[j], "--authmode") == 0) || (strcmp(argv[j], "-a") == 0))
                {
                    if (strcmp(argv[j + 1], "WEP") == 0)
                    {
                        config.ap.authmode  = AUTH_WEP;
                        break;
                    }
                    else if (strcmp(argv[j + 1], "WPA") == 0)
                    {
                        config.ap.authmode  = AUTH_WPA_PSK;
                        break;
                    }
                    else if (strcmp(argv[j + 1], "WPA2") == 0)
                    {
                        config.ap.authmode  = AUTH_WPA2_PSK;
                        break;
                    }
                    else if (strcmp(argv[j + 1], "WPA/WPA2") == 0)
                    {
                        config.ap.authmode  = AUTH_WPA_WPA2_PSK;
                        break;
                    }
                    else if (strcmp(argv[j + 1], "OPEN") == 0)
                    {
                        return CMD_RET_FAILURE;
                    }
                }
            }
        }

        if ((strcmp(argv[i], "--channel") == 0) || (strcmp(argv[i], "-c") == 0))
        {
            config.ap.channel   = atoi(argv[i + 1]);
            continue;
        }

        if ((strcmp(argv[i], "--max_connect") == 0) || (strcmp(argv[i], "-m") == 0))
        {
            config.ap.max_connect = atoi(argv[i + 1]);
            continue;
        }

        if ((strcmp(argv[i], "--hidden_ssid") == 0) || (strcmp(argv[i], "-h") == 0))
        {
            config.ap.hidden_ssid = atoi(argv[i + 1]);
            continue;
        }
    }

    wifi_stop_softap();
    os_msleep(500);
    return wifi_start_softap(&config) ? CMD_RET_FAILURE: CMD_RET_SUCCESS;
}
CLI_CMD(wifi_softap, cmd_wifi_softap, "start a wifi softap", 
"wifi_softap <--ssid or -s> [--password or -p] [--channel or -c] [--max_connect or -m] [--hidden_ssid-or -h] [--authmode or -a]");

static int cmd_wifi_stop_softap(cmd_tbl_t * t, int argc, char * argv[])
{
    wifi_stop_softap();
    return CMD_RET_SUCCESS;
}
CLI_CMD(wifi_stop_softap, cmd_wifi_stop_softap, "stop a wifi softap", "wifi_stop_softap");

#if 0
static int cmd_wifi_scan(cmd_tbl_t *t, int argc, char *argv[])
{
    int i;
    wifi_scan_config_t config;
    wifi_info_t info;
    wifi_scan_config_t *pcfg = NULL;
    char *auth_str[] = {"open", "wep", "wpa", "wpa2", "wpa/wpa2", "wpa3", "wpa2/wpa3"};
    char *cipher_str[] = {"", "", "", "-tkip", "-ccmp"};

    if (argc > 2)
    {
        return CMD_RET_USAGE;
    }

    if (argc > 1)
    {
        memset(&config, 0, sizeof(config));
        config.channel = atoi(argv[1]);
        pcfg = &config;
    }

    if (!wifi_scan_start(1, pcfg))
    {
        if (wpa_get_scan_num())
        {
            os_printf(LM_CMD, LL_INFO, "bssid               channel   rssi   encrypt          ssid\n");

            for (i = 0; i < wpa_get_scan_num(); i++)
            {
                memset(&info, 0, sizeof(info));
                if (wpa_get_scan_result(i, &info))
                    break;

                os_printf(LM_CMD, LL_INFO, MACSTR"   %7d   %4d   %9s%-5s   %s\n", 
                          MAC2STR(info.bssid), info.channel, info.rssi, auth_str[info.auth], cipher_str[info.cipher], info.ssid);
            }
        }
        else
        {
            os_printf(LM_CMD, LL_INFO, "not found bss.\n");
        }

        return CMD_RET_SUCCESS;
    }
    else
    {
	    return CMD_RET_FAILURE;
	}
}
CLI_CMD(wifi_scan, cmd_wifi_scan,  "scan wifi as station", "wifi_scan [channel]");
#endif

extern int wpa_cmd_receive(int vif_id, int argc, char *argv[]);
static int cmd_wpa(cmd_tbl_t *t, int argc, char *argv[])
{
	//os_printf(LM_CMD, LL_INFO, "--wpa cli argc %d, %s %s\n", argc, argv[0] , argc > 1 ? argv[1] : "");
	if (wpa_cmd_receive(0, argc, argv) == 0)
		return CMD_RET_SUCCESS;

	return CMD_RET_FAILURE;
}

static int cmd_wpb(cmd_tbl_t *t, int argc, char *argv[])
{
	//os_printf(LM_CMD, LL_INFO, "--wpb cli argc %d, %s %s\n", argc, argv[0] , argc > 1 ? argv[1] : "");
	if (wpa_cmd_receive(1, argc, argv) == 0)
		return CMD_RET_SUCCESS;

	return CMD_RET_FAILURE;
}

CLI_CMD(wpa, cmd_wpa,  "wpa cli",	 "wpa [command]");
CLI_CMD(wpb, cmd_wpb,  "wpb cli",	 "wpb [command]");
#endif  //CONFIG_WIRELESS_WPA_SUPPLICANT

#if CONFIG_WIFI_PRODUCT_FHOST
static os_timer_handle_t g_wifi_mac_check_timer = NULL;

static void wifi_check_mac_timeout(os_timer_handle_t timer)
{
    if (g_wifi_mac_check_timer)
        os_timer_stop(g_wifi_mac_check_timer);

    wifi_set_promiscuous(false);
}

static void wifi_check_mac_rx(void *buf, int len, wifi_promiscuous_pkt_type_t type)
{
    uint8_t *sa;
    uint8_t *payload;
    uint8_t sta_mac[6];
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    if (type != WIFI_PKT_NULL)
    {
        payload = pkt->payload;

        if ((len >= 24) && (payload[1] & 0x1))
        {
            if (payload[1] & 0x2)
            {
                if (len >= 30)
                    sa = &payload[24];
                else
                    return;
            }
            else
            {
                sa = &payload[10];
            }

            hal_system_get_sta_mac(sta_mac);

            if (!memcmp(sta_mac, sa, 6))
            {
                os_printf(LM_CMD, LL_INFO, "\n\nsta mac address may be duplicate\n\n");
            }
        }
    }
}

static int cmd_wifi_mac_check(cmd_tbl_t *t, int argc, char *argv[])
{
    int v;

    if (argc < 2)
        return CMD_RET_FAILURE;

    v = atoi(argv[1]);
    os_printf(LM_CMD, LL_INFO, "%s check mac is duplicate\n", v ? "enable" : "disable");

    if (v)
    {
        if (!g_wifi_mac_check_timer)
            g_wifi_mac_check_timer = os_timer_create("mac_check", 60 * 1000, 1, wifi_check_mac_timeout, NULL);

        os_timer_start(g_wifi_mac_check_timer);

        wifi_promiscuous_filter_t filter;
        filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
        wifi_set_promiscuous_filter(&filter);
        wifi_set_promiscuous_rx_cb(wifi_check_mac_rx);
        wifi_set_promiscuous(true);
    }
    else
    {
        if (g_wifi_mac_check_timer)
        {
            os_timer_delete(g_wifi_mac_check_timer);
            g_wifi_mac_check_timer = NULL;
        }

        wifi_set_promiscuous(false);
    }

    if (argc > 2)
    {
        v = atoi(argv[2]);
        os_printf(LM_CMD, LL_INFO, "switch channel %d\n", v);
        wifi_rf_set_channel(v);
    }

    return CMD_RET_SUCCESS;
}
CLI_CMD(wifi_check_mac, cmd_wifi_mac_check,  "check mac is duplicate", "wifi_check_mac <en> [ch]");
#endif

#ifdef CONFIG_LWIP
extern void ping_run(char *cmd);
int cmd_ping(cmd_tbl_t *t, int argc, char *argv[])
{
    int i, len = 0;
    char buf[512] = {0};

    for (i = 1; i < argc; i++)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "%s ", argv[i]);
    }

    ping_run(buf);
    return CMD_RET_SUCCESS;
}
CLI_CMD(ping, cmd_ping, "ping command", "ping [IP address]");

extern void wifi_ifconfig(char* cmd);
int cmd_ifconfig(cmd_tbl_t *t, int argc, char *argv[])
{
	int i, len = 0;
	char buf[64] = {0};

    for (i = 1; i < argc; i++)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "%s ", argv[i]);
    }

	wifi_ifconfig(buf);
	return CMD_RET_SUCCESS;
}
CLI_CMD(ifconfig, cmd_ifconfig, "ifconfig command", "ifconfig -h");
#endif  //CONFIG_LWIP



#ifdef CONFIG_WIFI_PRODUCT_FHOST

int set_mac_func(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}	
	
	if(hal_system_set_mac(MAC_SAVE_TO_AMT,argv[1]))
	{
		return CMD_RET_FAILURE;
	}	
	return CMD_RET_SUCCESS;
}
CLI_CMD(set_mac, set_mac_func, "set_mac 00:11:22:33:44:55", "set_mac 00:11:22:33:44:55");


int get_mac_func(cmd_tbl_t *t, int argc, char *argv[])
{
	//int i = 0;
	//char tmp_mac[18] = {0};
	unsigned char mac[6] = {0};
	if(!hal_system_get_config(STA_MAC,mac,sizeof(mac)))
	{
		return CMD_RET_FAILURE;
	}
	#if 0
	for (i=0; i<6; i++)
	{
		mac[i] = hex2num(tmp_mac[i*3]) * 0x10 + hex2num(tmp_mac[i*3+1]);
	}
	#endif
	os_printf(LM_CMD, LL_ERR, "sta mac = %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	memset(mac,0,6);
	hal_system_get_sta_mac(mac);
	os_printf(LM_CMD, LL_INFO, "sta %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memset(mac,0,6);
	hal_system_get_ap_mac(mac);
	os_printf(LM_CMD, LL_INFO, "ap %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memset(mac,0,6);
	hal_system_get_ble_mac(mac);
	os_printf(LM_CMD, LL_INFO, "ble %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return CMD_RET_SUCCESS;
}
CLI_CMD(get_mac, get_mac_func, "get_mac", "get_mac");

typedef struct sc_head
{
    unsigned char dummyap[26];
    unsigned char dummy[32];
} sc_head_t;

static sc_head_t sc_head;
static unsigned char source_mac[6];
static unsigned char destination_mac[6];

static int process_frame(void *packet, int size, wifi_promiscuous_pkt_type_t type)
{
    int isvalid = 0;
    int seq_num = 0;
    int fromDS_flag = 0;
    int toDS_flag = 0;
    int i;
    unsigned char ch;
    char frameType[5];
    int cts_flag = 0;
    // after discover
    uint16_t duration = 0;

    for (i = 0; i < 24; i++) {
        ch = *((unsigned char*)packet + i);
        sc_head.dummy[i] = ch;
    }

    if ((type == WIFI_PKT_CTRL) && ((sc_head.dummy[0] & 0xf0) == 0xc0)) {
        cts_flag = 1;
    }

    if (!cts_flag) {
        if (memcmp(source_mac, &sc_head.dummy[10], 6) != 0) {
            goto invalid;
        }
    }
    if (memcmp(destination_mac, &sc_head.dummy[4], 6) != 0) {
        goto invalid;
    }

    if((sc_head.dummy[1] & 0x02) == 0x02){
        fromDS_flag = 1;
    }

    if ((sc_head.dummy[1] & 0x01) == 0x01) {
        toDS_flag = 1;
    }

    seq_num = (sc_head.dummy[22] + sc_head.dummy[23] * 256) >> 4;

    isvalid = 1;

    if (type == WIFI_PKT_MGMT) {
        memcpy(frameType, "MGMT", 5);
    } else if (type == WIFI_PKT_CTRL) {
        memcpy(frameType, "CTRL", 5);
    } else if (type == WIFI_PKT_DATA) {
        memcpy(frameType, "DATA", 5);
    } else {
        goto invalid;
    }

    if (type == WIFI_PKT_CTRL) {
        duration = sc_head.dummy[3];
        duration = (duration << 8) | sc_head.dummy[2];
        if ((sc_head.dummy[0] & 0xf0) == 0xc0) {
            os_printf(LM_WIFI, LL_ERR, "[%s]-[CTS]-[Duration-%d]\n", frameType, duration);
        } else if ((sc_head.dummy[0] & 0xf0) == 0xb0) {
            os_printf(LM_WIFI, LL_ERR, "[%s]-[RTS]-[Duration-%d]\n", frameType, duration);
        } else {
            os_printf(LM_WIFI, LL_ERR, "[%s]-[seq_num-%d]-[FromDS-%d]-[ToDS-%d]\n", frameType, seq_num, fromDS_flag, toDS_flag);
        }
    } else {
        os_printf(LM_WIFI, LL_ERR, "[%s]-[seq_num-%d]-[FromDS-%d]-[ToDS-%d]\n", frameType, seq_num, fromDS_flag, toDS_flag);
    }
invalid:
    return isvalid;
}

static void SnifferCallback(void *buf, int len, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf; 

    if (type != WIFI_PKT_NULL) {
        process_frame((char *)pkt->payload, len, type);
    }
}

int set_sniffer_start(cmd_tbl_t *t, int argc, char *argv[])
{
    int ch;
    int mgmt, ctrl, data;
    char *source_macstr;
    char *des_macstr;

    wifi_remove_config_all(STATION_IF);
    wifi_remove_config_all(SOFTAP_IF);

    ch = atoi(argv[1]);
    wifi_rf_set_channel(ch);

    mgmt = atoi(argv[2]);
    ctrl = atoi(argv[3]);
    data = atoi(argv[4]);
    source_macstr = argv[5];
    des_macstr = argv[6];

    system_printf("==source_mac=");
    for (int i = 0; i < 6 ; i++) {
        source_mac[i] = hex2num(source_macstr[i*3]) * 0x10 + hex2num(source_macstr[i*3+1]);
        system_printf("=%x=", source_mac[i]);
    }

    system_printf("\n==destination_mac=");
    for (int i = 0; i < 6 ; i++) {
        destination_mac[i] = hex2num(des_macstr[i*3]) * 0x10 + hex2num(des_macstr[i*3+1]);
        system_printf("=%x=", destination_mac[i]);
    }
    system_printf("\n");
    if (mgmt == 1) {
        mgmt = WIFI_PROMIS_FILTER_MASK_MGMT;
    } else {
        mgmt = 0;
    }
    if (ctrl == 1) {
        ctrl = WIFI_PROMIS_FILTER_MASK_CTRL;
    } else {
        ctrl = 0;
    }
    if (data == 1) {
        data = WIFI_PROMIS_FILTER_MASK_DATA;
    } else {
        data = 0;
    }
    wifi_promiscuous_filter_t filter = {0};
    filter.filter_mask = mgmt | ctrl | data;
    wifi_set_promiscuous_filter(&filter);
    wifi_set_promiscuous_rx_cb(SnifferCallback);
    wifi_set_promiscuous(true);

    return 0;
}
CLI_CMD(sniffer_start, set_sniffer_start, "set_sniffer_start", "set_sniffer_start");

int set_sniffer_stop(cmd_tbl_t *t, int argc, char *argv[])
{
    wifi_set_promiscuous(false);

    return 0;
}
CLI_CMD(sniffer_stop, set_sniffer_stop, "set_sniffer_stop", "set_sniffer_stop");

#endif // CONFIG_WIFI_PRODUCT_FHOST

#ifdef ENABLE_LWIP_NAPT
extern void napt_debug_print();
int lwip_napt_set(cmd_tbl_t *t, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    if (0 == strcmp("enable", argv[1])) {
        enable_lwip_napt(SOFTAP_IF, 1);
    } else if (0 == strcmp("disable", argv[1])) {
        enable_lwip_napt(SOFTAP_IF, 0);
    } else if (0 == strcmp("show", argv[1])) {
        napt_debug_print();
    } else {
        return CMD_RET_USAGE;
    }

    return CMD_RET_SUCCESS;
}
CLI_CMD(napt, lwip_napt_set, "lwip napt", "napt enable/disable/show");
#endif

extern int call_wifi_dbg_func(int argc, char *argv[]);
int wifi_dbg_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
    return call_wifi_dbg_func(argc - 1, argv + 1) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}
CLI_CMD(wd, wifi_dbg_cmd, "wifi debug", "wd [para]");

#ifdef NX_ESWIN_LMAC_TEST
	int modem_cmd(cmd_tbl_t *t, int argc, char *argv[]);
	CLI_CMD(lmt, modem_cmd, "", "");
	CLI_CMD(rf, modem_cmd, "", "");
	CLI_CMD(macbyp, modem_cmd, "", "");
#endif



