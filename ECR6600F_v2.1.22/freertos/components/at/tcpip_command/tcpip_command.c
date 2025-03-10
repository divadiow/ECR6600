#include "uart.h"
#include "rtc.h"
#include "at_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
//#include "util_cli_freertos.h"
#include "system_event.h"
//#include "system.h"
#include "system_wifi.h"
#include "dce.h"
#include "dce_commands.h"
#include "tcpip_command.h"
#include "system_network.h"
#include "system_config.h"
#include "easyflash.h"
//#include "trs_tls.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "tcpip_func.h"
#include "lwip/apps/sntp.h"
#include "apps/ping/ping.h"
#include "lwip/netdb.h"

#include "at_def.h"

/************************quxin************************************/
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_LEN 6
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif
/************************quxin************************************/
extern wifi_ap_config_t softap_config;
extern bool is_valid_mac(const char *mac_addr);
extern bool connect_check(ip_addr_t ip);
extern void wifi_sta_connnect(const char *ssid, const char *pwd);

uint8_t g_temporary_mac[MAC_LEN] = {0};
uint8_t g_temporary_mac_flag = 0;
uint32_t ssl_client_num = 0;
uint32_t g_at_port = AT_UDP_MIN_PORT;
extern int8_t fhost_cntrl_map_check(uint8_t vif_type);
dce_result_t dce_handle_CIPMAC_api(dce_t* dce, void* group_ctx, int kind, size_t argc,
        										arg_t* argv,int vif_id,uint8_t is_flash)
{
	uint8_t mac[6] = {0}; 
	struct netif *netif_temp;
    wifi_info_t info;
    int auto_sta = 0;
    int auto_ap = 0;
//	int ret = 0;
    wifi_config_u config={0};


	if(DCE_WRITE == kind)
	{
		if (argc != 1
			|| argv[0].type != ARG_TYPE_STRING 
			|| (true != is_valid_mac(argv[0].value.string)))
		{
			os_printf(LM_APP, LL_INFO, "invalid arguments %s %d \n",__func__,__LINE__);
			dce_emit_basic_result_code(dce, DCE_RC_ERROR);
			return DCE_RC_OK;
		}
		hexStr2MACAddr((uint8_t*)argv[0].value.string,mac);
        if(mac[0]&0x1)
        {
            os_printf(LM_APP, LL_INFO, "not unicast mac %s %d\n",__func__,__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        
        if(STATION_IF == vif_id)
        {
            wifi_status_e ap_status = wifi_get_status(SOFTAP_IF);
            wifi_status_e sta_status = wifi_get_status(STATION_IF);
            wifi_work_mode_e opmode = wifi_get_opmode();

            if (sta_status == STA_STATUS_CONNECTED) {
                auto_sta = 1;
                wifi_get_wifi_info(&info);
            }
            wifi_remove_config_all(STATION_IF);
            if (ap_status == AP_STATUS_STARTED) {
                auto_ap = 1;
            }
            wifi_stop_softap();

            while (1) {
                if (fhost_cntrl_map_check(0) != 1) {
                    os_msleep(1);
                    continue;
                }
                if (fhost_cntrl_map_check(2) != 1) {
                    os_msleep(1);
                    continue;
                }
                ap_status = wifi_get_status(SOFTAP_IF);
                sta_status = wifi_get_status(STATION_IF);
                if (sta_status == 0) {
                    sta_status = STA_STATUS_STOP;
                }
                if (opmode == WIFI_MODE_AP_STA) {
                    if ((sta_status == STA_STATUS_STOP || sta_status == STA_STATUS_DISCON) && (ap_status == AP_STATUS_STOP)) {
                        break;
                    }
                } else if (opmode == WIFI_MODE_AP) {
                    if (ap_status == AP_STATUS_STOP) {
                        break;
                    }
                } else if (opmode == WIFI_MODE_STA) {
                    if (sta_status == STA_STATUS_STOP || sta_status == STA_STATUS_DISCON) {
                        break;
                    }
                } else {
                    os_printf(LM_APP, LL_ERR, "error opmode = WIFI_MODE_MAX\n");
                    break;
                }
                os_msleep(1);
            }
#if 0
            if (wifi_change_mac_addr(mac) != SYS_OK) {
                os_printf(LM_APP, LL_INFO, "wifi_change_mac_addr error\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
#endif
            if (is_flash) {
                g_temporary_mac_flag = 0;
                memset(g_temporary_mac, 0, MAC_LEN);
                if(STATION_IF == vif_id)
                    hal_system_set_mac(MAC_SAVE_TO_AMT, (void*)argv[0].value.string);
            } else {
                g_temporary_mac_flag = 1;
                memcpy(g_temporary_mac, mac, MAC_LEN);
            }

            wifi_set_mac_addr(STATION_IF,mac);
            mac[5] ^= 0x1;
            wifi_set_mac_addr(SOFTAP_IF,mac);

            if (auto_sta) {
                memcpy(config.sta.ssid, info.ssid, info.ssid_len);
                memcpy(config.sta.password, info.pwd, info.pwd_len);
                wifi_set_status(STATION_IF, STA_STATUS_STOP);
                wifi_start_station(&config);
            }
            if (auto_ap) {
                if (auto_sta) {
                    while (1) {
                        sta_status = wifi_get_status(STATION_IF);
                        if (sta_status == STA_STATUS_CONNECTED) {
                            break;
                        } else if (sta_status == STATUS_ERROR){
                            os_printf(LM_APP, LL_ERR, "error sta_status = STATUS_ERROR\n");
                            break;
                        }
                        os_msleep(1);
                    }
                    softap_config.channel = info.channel;
                }
                wifi_start_softap((wifi_config_u *)&softap_config);
            }
        }
        
	}
	else if (DCE_READ == kind)
	{
		char mac_str[18] ={0};
        char handle[18]  = "CIPSTAMAC_CUR";

        if(STATION_IF == vif_id)
        {
            strcpy(handle, "CIPSTAMAC_CUR");
        }
        else if(SOFTAP_IF == vif_id)
        {
            strcpy(handle, "CIPAPMAC_CUR");
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        netif_temp = get_netif_by_index(vif_id);
        sprintf(mac_str, MACSTR, MAC2STR(netif_temp->hwaddr));
		arg_t result_cur[] = {{ARG_TYPE_STRING,.value.string=mac_str}};
		dce_emit_extended_result_code_with_args(dce, handle, -1, result_cur, 1, 1,false);
		
        memset(mac_str, 0, 18);
        if(STATION_IF == vif_id)
        {
            strcpy(handle, "CIPSTAMAC_DEF");
            amt_get_env_blob(STA_MAC, mac_str, 18, NULL);
//            ret = wifi_load_mac_addr(STATION_IF, (uint8_t *)mac_str);
        }
        else if(SOFTAP_IF == vif_id)
        {
            strcpy(handle, "CIPAPMAC_DEF");
//            ret = wifi_load_mac_addr(SOFTAP_IF, (uint8_t *)mac_str);
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
        if(mac_str == NULL)
        {
            os_printf(LM_APP, LL_INFO, "get mac fail from flash %s %d\n",__func__,__LINE__);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_OK;
        }
		arg_t result[] = {{ARG_TYPE_STRING,.value.string=mac_str}};
		dce_emit_extended_result_code_with_args(dce, handle, -1, result, 1, 1,false);
	}
	dce_emit_basic_result_code(dce, DCE_RC_OK);
	return DCE_OK;
}

dce_result_t dce_handle_CIPSTAMAC(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    bool flash_en = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "CIPSTAMAC argc:%d, flash:%d\n", argc, flash_en);

    dce_handle_CIPMAC_api(dce, group_ctx, kind, argc,argv,STATION_IF,flash_en);
    return DCE_OK;
}

#if 0 // quxin 先注释，因为6600上也是根据STA自动生成（最低位取反）�?
dce_result_t dce_handle_CIPAPMAC(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    bool flash_en = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "CIPAPMAC argc:%d, flash:%d\n", argc, flash_en);

    dce_handle_CIPMAC_api(dce, group_ctx, kind, argc,argv,SOFTAP_IF,flash_en);
    return DCE_OK;
}
#endif

dce_result_t dce_handle_CIPSTA(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    bool             flash_en = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "CIPSTA argc:%d, flash:%d\n", argc, flash_en);

    if (kind & DCE_READ)
    {
		if(STA_STATUS_CONNECTED != wifi_get_sta_status())
        {
            os_printf(LM_APP, LL_INFO, "Not Connected AP\r\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_INVALID_INPUT;
        }

        struct netif netif_temp;
        char handle[] = "CIPSTA_DEF";

        struct netif *n = get_netif_by_index(STATION_IF);
        memcpy(&netif_temp,n,sizeof(struct netif));

        if (!netif_is_up(&netif_temp))
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

        char ip_cur[16], gateway_cur[16], netmask_cur[16];
        sprintf(ip_cur, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.ip_addr))));
        sprintf(netmask_cur, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.netmask))));
        sprintf(gateway_cur, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.gw))));

        arg_t result_cur[] = {
            {ARG_TYPE_STRING, .value.string = ip_cur},
            {ARG_TYPE_STRING, .value.string = gateway_cur},
            {ARG_TYPE_STRING, .value.string = netmask_cur},
        };
        memcpy(handle,"CIPSTA_CUR",sizeof("CIPSTA_CUR"));
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[0], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[1], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[2], 1, 1, false);

        // 检查flash中的Ip
        if(!hal_system_get_config(CUSTOMER_NV_WIFI_STA_IP, &(netif_temp.ip_addr), sizeof(netif_temp.ip_addr)) || 
           !hal_system_get_config(CUSTOMER_NV_WIFI_STA_GATEWAY, &(netif_temp.gw), sizeof(netif_temp.gw)) || 
           !hal_system_get_config(CUSTOMER_NV_WIFI_STA_NETMASK, &(netif_temp.netmask), sizeof(netif_temp.netmask)))
		{
            dce_emit_extended_result_code(dce, "FLASH IS NULL, NEED WRITE FIRST", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        

        char ip[16], gateway[16], netmask[16];
        sprintf(ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.ip_addr))));
        sprintf(netmask, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.netmask))));
        sprintf(gateway, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(netif_temp.gw))));

        arg_t result[] = {
            {ARG_TYPE_STRING, .value.string = ip},
            {ARG_TYPE_STRING, .value.string = gateway},
            {ARG_TYPE_STRING, .value.string = netmask},
        };
        memcpy(handle,"CIPSTA_DEF",sizeof("CIPSTA_DEF"));
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[0], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[1], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[2], 1, 1, false);
        
    }
    else if ((kind & DCE_WRITE))
    {
        if (argc == 3 &&
            argv[0].type == ARG_TYPE_STRING &&
            argv[1].type == ARG_TYPE_STRING &&
            argv[2].type == ARG_TYPE_STRING)
        {    
            ip_info_t ip_info;
            //struct netif *netif_tmp = get_netif_by_index(STATION_IF);

            if (ipaddr_aton(argv[0].value.string, &(ip_info.ip)) &&
                ipaddr_aton(argv[1].value.string, &(ip_info.gw)) &&
                ipaddr_aton(argv[2].value.string, &(ip_info.netmask)))
            {
                if(TCPIP_DHCP_STATIC != wifi_station_dhcpc_status(STATION_IF))
                {
                    wifi_dhcp_close(STATION_IF); 
                }
                set_sta_ipconfig(&ip_info);

                if(flash_en)
                {
                    bool dhcp_sta_status = false;
                    hal_system_set_config(CUSTOMER_NV_WIFI_STA_IP, &(ip_info.ip), sizeof(ip_info.ip));
                    hal_system_set_config(CUSTOMER_NV_WIFI_STA_GATEWAY, &(ip_info.gw), sizeof(ip_info.gw));
                    hal_system_set_config(CUSTOMER_NV_WIFI_STA_NETMASK, &(ip_info.netmask), sizeof(ip_info.netmask));
                    hal_system_set_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status));
                }

            }
            else
            {
                os_printf(LM_APP, LL_INFO, "ip invalid\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
        }
        else
        {
            os_printf(LM_APP, LL_INFO, "ip invalid\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPAP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    bool             flash_en = false;

    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &flash_en))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    //os_printf(LM_APP, LL_INFO, "CIPAP argc:%d, flash:%d\n", argc, flash_en);

    if(kind & DCE_READ)
    {
        char ip[24], gateway[24], netmask[24];
        ip_info_t ip_info;
        char handle[] = "CIPAP_CUR";

        wifi_get_ip_info(SOFTAP_IF, &ip_info);
        sprintf(ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.ip))));
        sprintf(gateway, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.gw))));
        sprintf(netmask, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.netmask))));

        arg_t result_cur[] = {
            {ARG_TYPE_STRING, .value.string = ip},
            {ARG_TYPE_STRING, .value.string = gateway},
            {ARG_TYPE_STRING, .value.string = netmask},
        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[0], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[1], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result_cur[2], 1, 1, false);

        hal_system_get_config(CUSTOMER_NV_WIFI_AP_IP, &(ip_info.ip), sizeof(ip_info.ip));
        hal_system_get_config(CUSTOMER_NV_WIFI_AP_GATEWAY, &(ip_info.gw), sizeof(ip_info.gw));
        hal_system_get_config(CUSTOMER_NV_WIFI_AP_NETMASK, &(ip_info.netmask), sizeof(ip_info.netmask));
        sprintf(ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.ip))));
        sprintf(gateway, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.gw))));
        sprintf(netmask, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&(ip_info.netmask))));

        arg_t result[] = {
            {ARG_TYPE_STRING, .value.string = ip},
            {ARG_TYPE_STRING, .value.string = gateway},
            {ARG_TYPE_STRING, .value.string = netmask},
        };
        memcpy(handle,"CIPAP_DEF",sizeof("CIPAP_DEF"));
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[0], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[1], 1, 1, false);
        dce_emit_extended_result_code_with_args(dce, handle, -1, &result[2], 1, 1, false);

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_RC_OK;
    }
    else if ((kind & DCE_WRITE))
    {
        if (argc == 3 &&
            argv[0].type == ARG_TYPE_STRING &&
            argv[1].type == ARG_TYPE_STRING &&
            argv[2].type == ARG_TYPE_STRING)
        {
            ip_info_t ip_info;
            if (ipaddr_aton(argv[0].value.string, &(ip_info.ip)) &&
                ipaddr_aton(argv[1].value.string, &(ip_info.gw)) &&
                ipaddr_aton(argv[2].value.string, &(ip_info.netmask)))
            {
                if(!wifi_set_ip_info(SOFTAP_IF,&ip_info))
                {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_RC_ERROR;
                }
                set_softap_ipconfig(&ip_info);

                if(flash_en){
                    hal_system_set_config(CUSTOMER_NV_WIFI_AP_IP, &(ip_info.ip), sizeof(ip_info.ip));
                    hal_system_set_config(CUSTOMER_NV_WIFI_AP_GATEWAY, &(ip_info.gw), sizeof(ip_info.gw));
                    hal_system_set_config(CUSTOMER_NV_WIFI_AP_NETMASK, &(ip_info.netmask), sizeof(ip_info.netmask));
                }
                

                dce_emit_basic_result_code(dce, DCE_RC_OK);
                return DCE_RC_OK;
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
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

int wifi_status_to_at_state(wifi_status_e status)
{
    switch (status) {
        case AP_STATUS_STOP:
            return 5;
        case AP_STATUS_STARTED:
            return 2;
        case STA_STATUS_STOP:
        case STA_STATUS_DISCON:
        case STA_STATUS_START:
            return 5;
        case STA_STATUS_CONNECTED:
            return 2;
        default:
            return 5;
    }
    
    return 5;
}

const char * conn_type_2_str(conn_type_e type)
{
    switch (type) {
        case conn_type_udp:
            return "UDP";
        case conn_type_tcp:
            return "TCP";
        case conn_type_ssl:
            return "SSL";
        default:
            return "";
    }

    return "";
}

int response_conn_status(client_db_t *client, void* dce_param, void *para)
{
    dce_t *dce = (dce_t *)dce_param;
    
    arg_t results[6];
    char ip_str[16] = {'\0'};
    
    results[0].type = ARG_TYPE_NUMBER;
    results[0].value.number = client->id;

    results[1].type = ARG_TYPE_STRING;
    results[1].value.string = conn_type_2_str(client->type);

    if (client->father == NULL)
	{
		if (client->ip_info.src_port == 0) {
	        struct sockaddr_in conn_addr;
	        uint32_t len = sizeof(conn_addr);
	        if (0 == getsockname(client->fd, (struct sockaddr *)&conn_addr, &len)) {
	            client->ip_info.src_port = ntohs(conn_addr.sin_port);
	        }
	    }

	    sprintf(ip_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&client->ip_info.dst_ip)));
	    results[2].type = ARG_TYPE_STRING;
	    results[2].value.string = ip_str;

	    results[3].type = ARG_TYPE_NUMBER;
	    results[3].value.number = client->ip_info.dst_port;

	    results[4].type = ARG_TYPE_NUMBER;
	    results[4].value.number = client->ip_info.src_port;
	}
	else
	{
        if (client->type == conn_type_udp)
        {
            return 0;
        }
	    sprintf(ip_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&client->ip_info.src_ip)));
	    results[2].type = ARG_TYPE_STRING;
	    results[2].value.string = ip_str;

	    results[3].type = ARG_TYPE_NUMBER;
	    results[3].value.number = client->ip_info.src_port;

	    results[4].type = ARG_TYPE_NUMBER;
	    results[4].value.number = client->ip_info.dst_port;
	}

    results[5].type = ARG_TYPE_NUMBER;
    results[5].value.number = client->father ? 1 : 0;
    
    dce_emit_extended_result_code_with_args(dce, "CIPSTATUS", -1, results, ARRAY_SIZE(results), 1, false);

    return 0;
}

dce_result_t dce_handle_CIPSTATUS(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_status_e status = STATUS_ERROR;
    int at_status;
    char str[10] = {0};
    client_db_t *client = NULL;
    wifi_work_mode_e wifi_mode = wifi_get_opmode();

    if(wifi_mode == WIFI_MODE_AP){
        status = wifi_get_ap_status();
    } else {
        status = wifi_get_sta_status();
    }

    at_status = wifi_status_to_at_state(status);
    if (2 == at_status) {
        if ((client = at_net_find_client_by_status(conn_stat_connected))) {
            at_status = 3;
        }
    }
    sprintf(str, "STATUS:%1d", at_status);
    dce_emit_information_response(dce, str, -1);

    at_net_iterate_client(response_conn_status, (void *)dce, NULL);
        
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPDOMAIN(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    struct sockaddr_in sock_addr;
    ip_addr_t ip;
    char  ip_str[18] = {0};
    
    AT_CHECK_ERROR_RETURN(argc < 1 || argv[0].type != ARG_TYPE_STRING);
    
    memset(&sock_addr, 0, sizeof(sock_addr));
    if (at_net_resolve_dns(argv[0].value.string, &sock_addr) < 0) {
        dce_emit_information_response(dce, "DNS Fail", -1);
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    ip_2_ip4(&ip)->addr = sock_addr.sin_addr.s_addr;
    sprintf(ip_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&ip)));
    arg_t result = {ARG_TYPE_STRING, .value.string = ip_str};
    dce_emit_extended_result_code_with_args(dce, "CIPDOMAIN", -1, &result, 1, 1, false);
    
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSTART(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t client;
    int i = 0, ret;
    
    wifi_work_mode_e mode = wifi_get_opmode();

    AT_CHECK_ERROR_RETURN(!(kind & DCE_WRITE));
    
    AT_CHECK_ERROR_RETURN(argc < (3 + cfg->ipmux) ||  argc > (4 + cfg->ipmux));

    memset(&client, 0, sizeof(client));
    //link id
    if (cfg->ipmux) {
        AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER || argv[i].value.number < 0 ||
            argv[i].value.number >= MAX_CONN_NUM);
        
        client.id = argv[i].value.number;
        if (at_net_find_client(client.id)) {
            dce_emit_information_response(dce, "ALREADY	CONNECTED", -1);
            return DCE_RC_ERROR;
        }
        ++i;
    }

    //type
    AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_STRING);
    
    if (!strncmp(argv[i].value.string, "SSL", 3)) {
        client.type = conn_type_ssl;
    } else if (!strncmp(argv[i].value.string, "TCP", 3)) {
        client.type = conn_type_tcp;
    } else if (!strncmp(argv[i].value.string, "UDP", 3)) {
        client.type = conn_type_udp;
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    ++i;
    if((client.type == conn_type_ssl) && (ssl_client_num != 0)) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        os_printf(LM_APP, LL_INFO, "Only one ssl client can be established at most \r\n");
        return DCE_RC_ERROR;
    }
    //remote ip
    AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_STRING);
    
    if(!ipaddr_aton(argv[i].value.string, &client.ip_info.dst_ip)) {
        strlcpy(client.ip_info.domain_str, argv[i].value.string, sizeof(client.ip_info.domain_str));
    }
    ++i;

    if((mode == WIFI_MODE_STA || mode == WIFI_MODE_AP_STA) && wifi_get_status(STATION_IF) != STA_STATUS_CONNECTED)
    {
        dce_emit_extended_result_code(dce, "no ip", -1, 1);
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    else if(mode == WIFI_MODE_AP && connect_check(client.ip_info.dst_ip) == false)
    {
        char buf[16];
        memset(buf, 0x0, 10);
        if (cfg->ipmux)
            sprintf(buf,"%d, CLOSED", client.id);
        else
            sprintf(buf, "CLOSED");
        
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        dce_emit_extended_result_code(dce, buf, -1, 1);
        return DCE_RC_ERROR;
    }
    else if(mode == WIFI_MODE_AP_STA)
    {
        char buf[20];
        ip_info_t sta_ip, ap_ip;
        uint32_t  sta_addr,ap_addr, remote_addr;
        wifi_get_ip_info(STATION_IF, &sta_ip);
        
        memset(&ap_ip, 0, sizeof(ap_ip));
        sta_addr = htonl(ip_2_ip4(&sta_ip.ip)->addr);
        ap_addr = htonl(ip_2_ip4(&ap_ip.ip)->addr);
        remote_addr = htonl(ip_2_ip4(&client.ip_info.dst_ip)->addr);

        memset(buf, 0x0, 20);
        if (cfg->ipmux)
            sprintf(buf,"%d, CONNECTE FAIL", client.id);
        else
            sprintf(buf, "CONNECTE FAIL");

        // STA
        if(sta_addr >> 8 == remote_addr >> 8 && wifi_get_status(STATION_IF) != STA_STATUS_CONNECTED)
        {
            dce_emit_extended_result_code(dce, buf, -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        // AP
        else if(ap_addr >> 8 == remote_addr >> 8 && connect_check(client.ip_info.dst_ip) == false && wifi_get_status(SOFTAP_IF) != AP_STATUS_STARTED)
        {
            dce_emit_extended_result_code(dce, buf, -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

    }

    //remote port
    AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
    client.ip_info.dst_port = argv[i].value.number;
    ++i;

    //keepalive or udp local port
    if (i < argc) {
        AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
        
        if (client.type != conn_type_udp) {
            client.priv.tcp.keep_alive = argv[i].value.number;
        } else {
            client.ip_info.src_port = argv[i].value.number;
        }
        ++i;
    }

	// udp src port
	if (client.type == conn_type_udp && client.ip_info.src_port == 0)
	{
		client.ip_info.src_port = g_at_port++;
		if (g_at_port >= AT_UDP_MAX_PORT)
			g_at_port = AT_UDP_MIN_PORT;
	}
    
    ret = at_net_client_start(&client);
    if (ret) {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    } else {
        if(client.type == conn_type_ssl) {
            ssl_client_num++;
        }
    }
    

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSSLSIZE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->ssl_buff_size};
        dce_emit_extended_result_code_with_args(dce, "CIPSSLSIZE", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        cfg->ssl_buff_size = argv[0].value.number;
        cfg->ssl_buff_size = MAX(cfg->ssl_buff_size, 2048);
        cfg->ssl_buff_size = MIN(cfg->ssl_buff_size, 4096);
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSSLCCONF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->ssl_mode};
        dce_emit_extended_result_code_with_args(dce, "CIPSSLCCONF", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        cfg->ssl_mode = argv[0].value.number;
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSEND_api(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv, bool exit)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t    *client;
    unsigned char   link_id = 0;
    int i = 0;

	int at_status;
	wifi_status_e status = STATUS_ERROR;
    wifi_work_mode_e wifi_mode = wifi_get_opmode();

    if(wifi_mode == WIFI_MODE_AP){
        status = wifi_get_ap_status();
    } else {
        status = wifi_get_sta_status();
    }

    at_status = wifi_status_to_at_state(status);
    if (5 == at_status) {
		dce_emit_extended_result_code(dce, "WIFI DISCONNECT", -1, 1);
        return DCE_RC_ERROR;
    }

    if (0 == argc) {
        AT_CHECK_ERROR_RETURN(!cfg->pass_through || cfg->ipmux);
        client = at_net_find_client(link_id);
        AT_CHECK_ERROR_RETURN(!client);
    }else{
        AT_CHECK_ERROR_RETURN(cfg->pass_through);
        AT_CHECK_ERROR_RETURN(argc == 2 && cfg->ipmux == 0);
        AT_CHECK_ERROR_RETURN(argc == 1 && cfg->ipmux == 1);

        if (cfg->ipmux) {
            AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
            link_id = argv[i].value.number;
            ++i;

            if (i >= argc) {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
        }

        if (i < argc) {
            AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER \
                                 || argv[i].value.number > 2048 \
                                 || argv[i].value.number <= 0);
            cfg->send_buff.threshold = argv[i].value.number;
            ++i;
        }

        client = at_net_find_client(link_id);
        AT_CHECK_ERROR_RETURN(!client);

        if (client->type == conn_type_udp) {
			//udp server remote ip + remote port
			if (client->father && client->father->type == conn_type_udp && client->state == conn_stat_connected)
			{
				AT_CHECK_ERROR_RETURN(argc < 3 && cfg->ipmux == 0);
				AT_CHECK_ERROR_RETURN(argc < 4 && cfg->ipmux == 1);
			}
			
            if (i < argc) {
                //remote ip
                AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_STRING);
                AT_CHECK_ERROR_RETURN(!ipaddr_aton(argv[i].value.string, &client->ip_info.dst_ip));
                ++i;
            }

            if (i < argc) {
                //remote port
                AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
                client->ip_info.dst_port = argv[i].value.number;
                ++i;
            }
        }
    }

    if(exit)
        cfg->use_end_flag = 1;
    else
    {
        cfg->use_end_flag = 0;
    }
    

    AT_CHECK_ERROR_RETURN(0 != at_net_client_prepare_tx(client));
    
    dce_emit_information_response(dce, ">", -1);
    return DCE_OK;
}
dce_result_t dce_handle_CIPSEND(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return dce_handle_CIPSEND_api(dce, group_ctx, kind, argc, argv, false);
}
dce_result_t dce_handle_CIPSENDEX(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    AT_CHECK_ERROR_RETURN(0 == argc);
    
    return dce_handle_CIPSEND_api(dce, group_ctx, kind, argc, argv, true);
}

dce_result_t dce_handle_CIPSENDBUF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	#if 0
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    unsigned char   link_id = 0;
    client_db_t    *client;
    int i = 0;

    if (cfg->ipmux) {
        AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER || argv[i].value.number < 0 ||
            argv[i].value.number >= MAX_CONN_NUM);
        
        link_id = argv[i].value.number;
        ++i;
    }

    client = at_net_find_client(link_id);
    AT_CHECK_ERROR_RETURN(!client);
    AT_CHECK_ERROR_RETURN(conn_type_tcp != client->type);
    
    if (i < argc) {
        AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
        cfg->send_buff.threshold = argv[i].value.number;
        ++i;
    }

    client->u.tcp.tcp_buf_flag = 1;
    AT_CHECK_ERROR_RETURN(0 != at_net_client_prepare_tx(client));
    
    dce_emit_information_response(dce, ">", -1);
	#else
    dce_emit_basic_result_code(dce, DCE_RC_OK);
	#endif
	
    return DCE_OK;
}

dce_result_t dce_handle_CIPBUFRESET(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPBUFSTATUS(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPCHECKSEQ(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPCLOSE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    unsigned char   link_id = MAX_CONN_NUM;
    
    AT_CHECK_ERROR_RETURN(0 == argc && cfg->ipmux);
    AT_CHECK_ERROR_RETURN(1 == argc && !cfg->ipmux);
    if (1 == argc) {
        AT_CHECK_ERROR_RETURN(argv[0].type != ARG_TYPE_NUMBER);
        link_id = argv[0].value.number;
    }

    if (MAX_CONN_NUM == link_id) {
        AT_CHECK_ERROR_RETURN(!list_empty(&cfg->server_list));
    }

    if(link_id < 0 || link_id > MAX_CONN_NUM)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    if(!at_net_close(link_id))
    {
        if(cfg->ipmux != 0)
        {
            char results[11];
            sprintf(results,"%d,CLOSED",link_id);
            dce_emit_extended_result_code(dce, results, -1, 1);
        }
        else
        {
            dce_emit_extended_result_code(dce, "CLOSED", -1, 1);
        }
        

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_OK;
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
}

dce_result_t dce_handle_CIFSR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    // int i = 0;
    char str_ip[16] = {0};
    char str_mac[18] = {0};
    ip_addr_t ip;
    unsigned char mac[6] = {0};
    arg_t results_ip[2], results_mac[2];
    extern network_db_t network_db;
    wifi_work_mode_e mode = WIFI_MODE_STA;

    results_ip[0].type = ARG_TYPE_STRING;
    results_mac[0].type = ARG_TYPE_STRING;
    switch (network_db.mode) {
        case WIFI_MODE_AP:
            results_ip[0].value.string = "APIP";
            results_mac[0].value.string = "APMAC";
            mode = WIFI_MODE_AP;
            break;
        case WIFI_MODE_STA:
            results_ip[0].value.string = "STAIP";
            results_mac[0].value.string = "STAMAC";
            mode = WIFI_MODE_STA;
            break;
        case WIFI_MODE_AP_STA:
            results_ip[0].value.string = "APIP";
            results_mac[0].value.string = "APMAC";
            mode = WIFI_MODE_AP;
            wifi_get_ip_addr(mode, (unsigned int *)&(ip_2_ip4(&ip)->addr));
            sprintf(str_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&ip)));
            results_ip[1].type = ARG_TYPE_STRING;
            results_ip[1].value.string = str_ip;
            dce_emit_extended_result_code_with_args(dce, "CIFSR", -1, results_ip, ARRAY_SIZE(results_ip), 1, false);
            wifi_get_mac_addr(mode, mac);
            sprintf(str_mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            results_mac[1].type = ARG_TYPE_STRING;
            results_mac[1].value.string = str_mac;     
            dce_emit_extended_result_code_with_args(dce, "CIFSR", -1, results_mac, ARRAY_SIZE(results_mac), 1, false);
            results_ip[0].value.string = "STAIP";
            results_mac[0].value.string = "STAMAC";
            mode = WIFI_MODE_STA;
            break;
        default:
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }

    wifi_get_ip_addr(mode, (unsigned int *)&(ip_2_ip4(&ip)->addr));
    sprintf(str_ip, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&ip)));
    results_ip[1].type = ARG_TYPE_STRING;
    results_ip[1].value.string = str_ip;
    dce_emit_extended_result_code_with_args(dce, "CIFSR", -1, results_ip, ARRAY_SIZE(results_ip), 1, false);

    wifi_get_mac_addr(mode, mac);
    sprintf(str_mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    results_mac[1].type = ARG_TYPE_STRING;
    results_mac[1].value.string = str_mac;     
    dce_emit_extended_result_code_with_args(dce, "CIFSR", -1, results_mac, ARRAY_SIZE(results_mac), 1, false);

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

// 设置单连接还是多连接
// 0：单连接
// 1：多连接
dce_result_t dce_handle_CIPMUX(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) 
    {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->ipmux};
        dce_emit_extended_result_code_with_args(dce, "CIPMUX", -1, &result, 1, 1, false);
    } 
    else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) 
    {
        // 必须在没有建立连接的情况下设置连接模�?
        int client_id = 0;
        for(client_id = 0; client_id <= 4; client_id++)
        {
            if(at_net_find_client(client_id))
            {
                os_printf(LM_APP, LL_INFO, "cannot set in connected mode \r\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
        }

        // 如果是多连接，不允许在透传模式下进行设�?
        if((argv[0].value.number & cfg->pass_through) != 0)
        {
            os_printf(LM_APP, LL_INFO, "cannot set in CIPMODE=%d \r\n",cfg->pass_through);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

        cfg->ipmux = argv[0].value.number ? 1 : 0;

    } 
    else 
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSERVER(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    wifi_work_mode_e wifi_mode;
    wifi_mode = wifi_get_opmode();

    if (kind != DCE_WRITE){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
    os_printf(LM_APP, LL_INFO, "dce_handle_CIPSERVER argc %ld\n",argc);
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    if(cfg && cfg->ipmux == 0 ){
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }

    if((wifi_mode == WIFI_MODE_STA) || (wifi_mode == WIFI_MODE_AP))
    {
        if(wifi_get_status(STATION_IF) != STA_STATUS_CONNECTED && wifi_get_status(SOFTAP_IF) != AP_STATUS_STARTED)
        {
            os_printf(LM_APP, LL_INFO, "not connect wifi or set ap \r\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
    }
    else if (wifi_mode == WIFI_MODE_AP_STA)
    {
        if(wifi_get_status(STATION_IF) != STA_STATUS_CONNECTED)
        {
            os_printf(LM_APP, LL_INFO, "not connect wifi in sta+ap mode\r\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
    }
	int index = 0;
    server_db_t server;
    memset(&server, 0, sizeof(server));
    if ((argc == 3 &&
        	argv[0].type == ARG_TYPE_NUMBER &&
        	argv[1].type == ARG_TYPE_NUMBER &&
        	argv[2].type == ARG_TYPE_STRING) || 
        (argc == 2 && 
        	argv[0].type == ARG_TYPE_NUMBER &&
        	argv[1].type == ARG_TYPE_STRING)) {
           int mode = argv[0].value.number;

		   if(argc == 2){
		   		index = 1;
				server.ip_info.src_port = 333;
		   } else {
		   		index = 2;
				server.ip_info.src_port = argv[1].value.number;
		   }
    
			if (!strncmp(argv[index].value.string, "SSL", 3)) {
				server.type = conn_type_ssl;
			} else if (!strncmp(argv[index].value.string, "TCP", 3)) {
				server.type = conn_type_tcp;
			} else if (!strncmp(argv[index].value.string, "UDP", 3)) {
				server.type = conn_type_udp;
			} else {
				dce_emit_basic_result_code(dce, DCE_RC_ERROR);
				return DCE_RC_ERROR;
			}
		   
           if(mode == 0){
               at_net_server_stop(&server);
               dce_emit_basic_result_code(dce, DCE_RC_OK);
               return DCE_OK;
           } 
		   else if(mode == 1 && 
                     argv[1].value.number < 65535 && 
                     argv[1].value.number > 0){
                if(0 != at_net_server_start(&server)){
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_OK;
                }
           } 
		   else {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_OK;
           }
    } 
	else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSERVERMAXCONN(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->server_max_conn};
        dce_emit_extended_result_code_with_args(dce, "CIPSERVERMAXCONN", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        cfg->server_max_conn = argv[0].value.number;
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPMODE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->pass_through};
        dce_emit_extended_result_code_with_args(dce, "CIPMODE", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        if(cfg->ipmux & argv[0].value.number){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        cfg->pass_through = argv[0].value.number ? 1 : 0;
		hal_system_set_config(CUSTOMER_NV_WIFI_IP_MODE, (void*)&cfg->pass_through, sizeof(cfg->pass_through));
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSTO(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
	arg_t result = {ARG_TYPE_NUMBER, .value.number = pdTICKS_TO_MS(cfg->server_timeout / 1000)};
        dce_emit_extended_result_code_with_args(dce, "CIPSTO", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        if(argv[0].value.number > 7200 || argv[0].value.number < 0){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        if(cfg->server_timeout != argv[0].value.number){
            cfg->server_timeout = pdMS_TO_TICKS(argv[0].value.number * 1000);
        }
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPDINFO(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        if(argv[0].value.number != 1 && argv[0].value.number != 0){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
        cfg->remote_visible = argv[0].value.number ? true : false;
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPRECVMODE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
        arg_t result = {ARG_TYPE_NUMBER, .value.number = cfg->recv_mode};
        dce_emit_extended_result_code_with_args(dce, "CIPRECVMODE", -1, &result, 1, 1, false);

    } else if ((kind & DCE_WRITE) && (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)) {
        cfg->recv_mode = argv[0].value.number ? 1 : 0;
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPRECVDATA(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    char IPD_BUF[AT_MAX_RECV_LEN] = {0};
    int pos = 0;
    
    if (kind & DCE_WRITE) {
	    int num = 0, length = 0;
		unsigned char link_id = 0;
	    
	    AT_CHECK_ERROR_RETURN(argc < (1 + cfg->ipmux));
		
	    //link id
	    if (cfg->ipmux) {
	        AT_CHECK_ERROR_RETURN(argv[num].type != ARG_TYPE_NUMBER || argv[num].value.number < 0 || argv[num].value.number > 2048);
	        
	        link_id = argv[num].value.number;
	        if (at_net_find_client(link_id) == NULL) {
	            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	            return DCE_RC_ERROR;
	        }
	        ++num;
	    } else {
                if (at_net_find_client_by_status(conn_stat_connected) == NULL) {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    return DCE_RC_ERROR;
		}
            }
		
	    //len
	    AT_CHECK_ERROR_RETURN(argv[num].type != ARG_TYPE_NUMBER || argv[num].value.number < 0 || argv[num].value.number > 2048);
	    length = argv[num].value.number;

		//ip normal mode
		if (cfg->pass_through == 0 && cfg->recv_mode == 1)
		{
			int temp_len = 0;
			unsigned char dst_buf[2048];
			client_db_t *client_tcp, *client_tmp;
			server_db_t *server_tcp, *server_tmp;
			
		    // set local client nonblock
		    list_for_each_entry_safe(client_tcp, client_tmp, &cfg->client_list, list) 
		    {
		    	//tcp protocol
		        if (conn_type_tcp == client_tcp->type && conn_stat_connected == client_tcp->state) 
				{
					if ((cfg->ipmux == 1 && link_id == client_tcp->id) || (cfg->ipmux == 0))
					{
						temp_len = at_net_read_buff(cfg->recv_buff.recv_data[client_tcp->id], dst_buf, length);
						if (temp_len < 0)
						{
							continue;
						}
						dst_buf[temp_len] = '\0';
						
	            		arg_t result[] = {
	            			{ARG_TYPE_NUMBER, .value.number = temp_len},
	            			{ARG_TYPE_STRING, .value.string = (const char*)&dst_buf[0]},
	            		};
						setsockopt(client_tcp->fd, SOL_SOCKET, SO_RCVBUF, &client_tcp->recv_win, sizeof(int));

						dce_emit_extended_result_code_with_args(dce, "CIPRECVDATA", -1, (const arg_t*)&result, 2, 1, true);
						break;
					}
		        }
		    }

			list_for_each_entry_safe(server_tcp, server_tmp, &cfg->server_list, list) 
			{
				if (server_tcp->type == conn_type_tcp)
				{
					list_for_each_entry_safe(client_tcp, client_tmp, &server_tcp->client_list, list) 
					{
						if (conn_stat_connected == client_tcp->state) 
						{
							if (link_id == client_tcp->id)
							{
								temp_len = at_net_read_buff(cfg->recv_buff.recv_data[client_tcp->id], dst_buf, length);
								if (temp_len < 0)
								{
									continue;
								}
								dst_buf[temp_len] = '\0';
								
			            		arg_t result[] = {
			            			{ARG_TYPE_NUMBER, .value.number = temp_len},
			            			{ARG_TYPE_STRING, .value.string = (const char*)&dst_buf[0]},
			            		};
								setsockopt(client_tcp->fd, SOL_SOCKET, SO_RCVBUF, &client_tcp->recv_win, sizeof(int));

								dce_emit_extended_result_code_with_args(dce, "CIPRECVDATA", -1, (const arg_t*)&result, 2, 1, true);
								break;
							}
						}
					}
				}
                if (link_id == client_tcp->id)
                {
                    break;
                }
			}
            if (cfg->ipmux == 1) {
                if (temp_len > 0)
                    pos = sprintf(IPD_BUF,"+IPD,%d,%d\r\n",link_id, cfg->recv_buff.recv_data[link_id]->read_len);
                else
                    pos = sprintf(IPD_BUF,"+IPD,%d,%d\r\n", link_id, 0);
            } else {
                if (temp_len > 0)
                    pos = sprintf(IPD_BUF,"+IPD,%d\r\n", cfg->recv_buff.recv_data[client_tcp->id]->read_len);
                else
                    pos = sprintf(IPD_BUF,"+IPD,%d\r\n", 0);
            }
            if(pos != 0)
                dce_emit_pure_response(dce, IPD_BUF, pos);

            dce_emit_basic_result_code(dce, DCE_RC_OK);
			return DCE_OK;
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

dce_result_t dce_handle_CIPRECVLEN(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    if (kind & DCE_READ) {
		int num = 0, length = 0;
		arg_t result[MAX_CONN_NUM];
			
		//ip normal mode
		if (cfg->pass_through == 0 && cfg->recv_mode == 1)
		{
			client_db_t *client, *client_tmp;
			server_db_t *server, *server_tmp;
			
		    // set local client nonblock
		    list_for_each_entry_safe(client, client_tmp, &cfg->client_list, list) 
		    {
		    	//tcp protocol
		        if (conn_stat_connected == client->state && conn_type_tcp == client->type) 
				{
					length = cfg->recv_buff.recv_data[client->id]->read_len;
		            if (length > 0)
	            	{
	            		result[num].type = ARG_TYPE_NUMBER;
						result[num].value.number = length;
						num++;
	            	}
		        }
		    }

			list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) 
			{
				if (server->type == conn_type_tcp)
				{
					list_for_each_entry_safe(client, client_tmp, &server->client_list, list) 
					{
						if (conn_stat_connected == client->state) 
						{
							length = cfg->recv_buff.recv_data[client->id]->read_len;
				            if (length > 0)
			            	{
			            		result[num].type = ARG_TYPE_NUMBER;
								result[num].value.number = length;
								num++;
			            	}
						}
					}
				}
			}
		}
		
        dce_emit_extended_result_code_with_args(dce, "CIPRECVLEN", -1, (const arg_t*)&result, num, 1, true);
    } 
	else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPSNTPCFG(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
 #if 0 ///quxin
    char tz_str[16];

    if (kind & DCE_READ)
    {
        int i = 0, j = 2;

        arg_t result[] = {
            {ARG_TYPE_NUMBER, .value.number = sntp_enabled()},
            {ARG_TYPE_NUMBER, .value.number = get_timezone()},
            {ARG_TYPE_STRING, .value.string = sntp_getservername(0)},
            {ARG_TYPE_STRING, .value.string = sntp_getservername(1)},
            {ARG_TYPE_STRING, .value.string = sntp_getservername(2)},
        };

        for(i = 0; i < 3; i++)
        {
            if(sntp_getservername(i))
            {
                result[j++].value.string = sntp_getservername(i);
            }
        }
        dce_emit_extended_result_code_with_args(dce, "CIPSNTPCFG", -1, result, j, 1, false);
    }
    else if(kind & DCE_WRITE)
    {
        if( argc == 1 &&
            argv[0].type == ARG_TYPE_NUMBER &&
            (argv[0].value.number == 0 || argv[0].value.number == 1))
        {
            if(argv[0].value.number == 0 && sntp_enabled())
            {
                sntp_stop();
            }
            else if(argv[0].value.number == 1)
            {
                if (sntp_enabled()) {
                    sntp_stop();
                }
                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_setservername(0, "cn.ntp.org.cn");
                sntp_setservername(1, "ntp.sjtu.edu.cn");
                sntp_setservername(2, "us.pool.ntp.org");
                sntp_init();
            }
        }
        else if(argc == 2 &&
                argv[0].type == ARG_TYPE_NUMBER &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) &&
                argv[1].type == ARG_TYPE_NUMBER &&
                argv[1].value.number >= -11 && argv[1].value.number <= 13)
        {
            if(argv[0].value.number == 0 && sntp_enabled())
            {
                sntp_stop();
            }
            else if(argv[0].value.number == 1)
            {
                if (sntp_enabled()) {
                    sntp_stop();
                }
                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_setservername(0, "cn.ntp.org.cn");
                sntp_setservername(1, "ntp.sjtu.edu.cn");
                sntp_setservername(2, "us.pool.ntp.org");
                sntp_init();
            }

            set_timezone((char)argv[1].value.number);
        }
        else if((argc == 3 || argc == 4 || argc == 5) &&
                argv[0].type == ARG_TYPE_NUMBER &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) &&
                argv[1].type == ARG_TYPE_NUMBER &&
                argv[1].value.number >= -11 && argv[1].value.number <= 13)
        {
            int i = 3;
            if(argv[0].value.number == 0 && sntp_enabled())
            {
                sntp_stop();
            }
            else if(argv[0].value.number == 1)
            {
                if (sntp_enabled()) {
                    sntp_stop();
                }
                sntp_setoperatingmode(SNTP_OPMODE_POLL);

                for(i = 3;i <= argc; i++){
                    if(argv[i-1].type == ARG_TYPE_STRING){
                        sntp_setservername(i-3,argv[i-1].value.string);
                    } else if (argv[i-1].type == ARG_NOT_SPECIFIED){
                        continue;
                    } else{
                        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                        return DCE_RC_OK;
                    }
                }
                
                sntp_init();
            }
            set_timezone((char)argv[1].value.number);
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
#endif
    return DCE_OK;
}
#define NTP_PORT    123  
typedef struct    
{  
    unsigned char LiVnMode;  
    unsigned char Stratum;  
    unsigned char Poll;  
    unsigned char Precision;  
    long int RootDelay;  
    long int RootDispersion;  
    char RefID[4];  
    long int RefTimeInt;  
    long int RefTimeFraction;  
    long int OriTimeInt;  
    long int OriTimeFraction;  
    long int RecvTimeInt;  
    long int RecvTimeFraction;  
    long int TranTimeInt;  
    long int TranTimeFraction;  
}STNP_Header;  

bool get_ntp_time(STNP_Header *H_SNTP,uint8_t num)  
{  
    int sockfd=0;  
    struct sockaddr_in server;  
    fd_set set;    
    struct timeval timeout;  
    ip4_addr_t ping_addr;
    bzero((void*)H_SNTP, sizeof(STNP_Header));  //清零
    H_SNTP->LiVnMode = 0x23;  
		
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;    
    server.sin_port = htons(NTP_PORT);  
    struct hostent * host=NULL;//quxin  = gethostbyname(sntp_getservername(num));

    if(!host)
    {
        system_printf("gethostbyname error\r\n");
        return false;
    }
    
    ping_addr = *(ip4_addr_t*)host->h_addr_list[0];
    server.sin_addr.s_addr = ping_addr.addr;
	
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if(sockfd<0)  
    {  
        system_printf("sockfd error!\r\n");  
        return false;  
    }  
    
    if(sendto(sockfd, (void*)H_SNTP, sizeof(STNP_Header), 0, (struct sockaddr*)&server, sizeof(server))<0)  
    {  
        system_printf("sendto error!\r\n");  
        close(sockfd);
        return false;  
    }  
      
    FD_ZERO(&set);  
    FD_SET(sockfd, &set);
    timeout.tv_sec = 5;  
    timeout.tv_usec = 0;  
    
    if( select(sockfd+1, &set, NULL, NULL, &timeout) <= 0 )  
    {
        system_printf("select wait timeout!\r\n");  
        close(sockfd);
        return false;  
    }
    if(recv(sockfd, (void*)H_SNTP, sizeof(STNP_Header), 0)<0)  
    {
        system_printf("recv error!\r\n"); 
        close(sockfd);
        return false;  
    }
    
    close(sockfd);  
    return true;  
}

#if 0 //quxin 
static struct tm * my_localtime_r(const time_t *srctime,struct tm *tm_time)
{
    long int n32_Pass4year,n32_hpery;

    // 每个月的天数  非闰�?
    const static char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 一年的小时�?
    const static int ONE_YEAR_HOURS = 8760; // 365 * 24 (非闰�?

    //计算时差8*60*60 固定北京时间
    time_t time = *srctime;
    time=time+28800;
    if(time < 0)
    {
    time = 0;
    }

    //取秒时间
    tm_time->tm_sec=(int)(time % 60);
    time /= 60;

    //取分钟时�?
    tm_time->tm_min=(int)(time % 60);
    time /= 60;

    //计算星期
    tm_time->tm_wday=(time/24+4)%7;

    //取过去多少个四年，每四年�?1461*24 小时
    n32_Pass4year=((unsigned int)time / (1461L * 24L));

    //计算年份
    tm_time->tm_year=(n32_Pass4year << 2)+70;

    //四年中剩下的小时�?
    time %= 1461L * 24L;

    //校正闰年影响的年份，计算一年中剩下的小时数
    for (;;)
    {
        //一年的小时�?
        n32_hpery = ONE_YEAR_HOURS;

        //判断闰年
        if ((tm_time->tm_year & 3) == 0)
        {
            //是闰年，一年则�?4小时，即一�?
            n32_hpery += 24;
        }

        if (time < n32_hpery)
        {
            break;
        }

        tm_time->tm_year++;
        time -= n32_hpery;
    }

    //小时�?
    tm_time->tm_hour=(int)(time % 24);

    //一年中剩下的天�?
    time /= 24;

    //假定为闰�?
    time++;

    //校正润年的误差，计算月份，日�?
    if ((tm_time->tm_year & 3) == 0)
    {
        if (time > 60)
        {
            time--;
        }
        else
        {
            if (time == 60)
            {
                tm_time->tm_mon = 1;
                tm_time->tm_mday = 29;
                return tm_time;
            }
        }
    }

    //计算月日
    for (tm_time->tm_mon = 0;Days[tm_time->tm_mon] < time;tm_time->tm_mon++)
    {
        time -= Days[tm_time->tm_mon];
    }

    tm_time->tm_mday = (int)(time);
    return tm_time;
}

extern const char *week[7];
static int show_time(char strftime_buf[])
{
    struct tm cur_time = rtc_get_system_time();
    
    if (cur_time.tm_year < (2016 - 1900)) 
    {
        return -1;
    }

    sprintf(strftime_buf,"%s %4d-%02d-%02d %02d:%02d:%02d", week[cur_time.tm_wday],
                                                            cur_time.tm_year,
                                                            cur_time.tm_mon,
                                                            cur_time.tm_mday,
                                                            cur_time.tm_hour,
                                                            cur_time.tm_min,
                                                            cur_time.tm_sec);

    return 1;
}
#endif
dce_result_t dce_handle_CIPSNTPTIME(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    #if 0
    // 获取网络时间
    if (kind & DCE_READ)
    {
        STNP_Header HeaderSNTP;  
        if(!get_ntp_time(&HeaderSNTP,0) && !get_ntp_time(&HeaderSNTP,1) && !get_ntp_time(&HeaderSNTP,2))
        {
            dce_emit_extended_result_code(dce, "Time is not set yet. Connecting to WiFi and getting time over NTP.", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        time_t t1 = ntohl(HeaderSNTP.TranTimeInt); 
        struct timeval tv;
        struct tm time;
        t1=t1-2208988800L - 3600*8 + get_timezone()*3600; 

        my_localtime_r(&t1, &time);
		printf("sntp:%u-%u-%u,%u:%u:%u,%d\r\n",(time.tm_year)+1900,(time.tm_mon)+1,time.tm_mday,
                            time.tm_hour,time.tm_min,time.tm_sec,time.tm_wday);
		
        char strftime_buf[64];
        memset(strftime_buf,'\0',sizeof(strftime_buf));
        sprintf(strftime_buf,"%s %4d-%02d-%02d %02d:%02d:%02d", week[time.tm_wday],
                                                            time.tm_year + 1900,
                                                            time.tm_mon + 1,
                                                            time.tm_mday,
                                                            time.tm_hour,
                                                            time.tm_min,
                                                            time.tm_sec);
 
        arg_t result[] = {
            {ARG_TYPE_STRING, .value.string = strftime_buf},
        };

        dce_emit_extended_result_code_with_args(dce, "CIPSNTPTIME", -1, result, 1, 1, false);

    }
    #endif

    // 获取本地时间
    if (kind & DCE_READ)
    {
        char strftime_buf[64];
        memset(strftime_buf,'\0',sizeof(strftime_buf));

        // quxin if( -1 == show_time(strftime_buf) )
        if(0)
        {
            dce_emit_extended_result_code(dce, "Time is not set yet. Connecting to WiFi and getting time over NTP.", -1, 1);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        else
        {
            arg_t result[] = {
                {ARG_TYPE_STRING, .value.string = strftime_buf},
            };

            dce_emit_extended_result_code_with_args(dce, "CIPSNTPTIME", -1, result, 1, 1, false);
        }
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

bool dns_auto_change = true;
static ip_addr_t dns_server_save0, dns_server_save1;

void dns_server_save_pre(void)
{
    if(dns_auto_change)
        return;
    
    const ip_addr_t *server0 = dns_getserver(0);
    const ip_addr_t *server1 = dns_getserver(1);

    ip_2_ip4(&dns_server_save0)->addr = ip_2_ip4(server0)->addr;
    ip_2_ip4(&dns_server_save1)->addr = ip_2_ip4(server1)->addr;
}

void dns_server_depend_ap(void)
{
    if(dns_auto_change)
        return;

    dns_setserver(0,&dns_server_save0);
    dns_setserver(1,&dns_server_save1);
}

dce_result_t dce_handle_CIPDNS_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (kind & DCE_READ)
    {
        const ip_addr_t *server0 = dns_getserver(0);
        const ip_addr_t *server1 = dns_getserver(1);

        char server0_str[24], server1_str[24];
        sprintf(server0_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&server0)));
        sprintf(server1_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&server1)));

        arg_t result[] = {
            {ARG_TYPE_STRING, .value.string = server0_str},
            {ARG_TYPE_STRING, .value.string = server1_str},
        };

        if(0 != ip_2_ip4(&server0)->addr)
            dce_emit_extended_result_code_with_args(dce, "CIPDNS_CUR", -1, &result[0], 1, 1, false);
        if(0 != ip_2_ip4(&server1)->addr)
            dce_emit_extended_result_code_with_args(dce, "CIPDNS_CUR", -1, &result[1], 1, 1, false);
        
        dce_emit_basic_result_code(dce, DCE_RC_OK);

        return DCE_RC_OK;
    }
    else if ((kind & DCE_WRITE))
    {
        ip_addr_t server0, server1;
        ip_2_ip4(&server0)->addr = 0;
        ip_2_ip4(&server1)->addr = 0;
        if (argc == 1 && 
            argv[0].type == ARG_TYPE_NUMBER &&
            (argv[0].value.number == 0 || argv[0].value.number == 1))
        {
            const char *serverstr = "208.67.222.222";

            if(!ipaddr_aton(serverstr,&server0))
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);

                return DCE_RC_ERROR;
            }

            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else if(argc == 2 &&
                argv[0].type == ARG_TYPE_NUMBER &&
                argv[1].type == ARG_TYPE_STRING &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) )
        {
            if(argv[0].value.number == 0)
            {
                const char *serverstr = "208.67.222.222";
                ipaddr_aton(serverstr,&server0);
            }
            else if(!ipaddr_aton(argv[1].value.string, &server0))
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }

            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else if(argc == 3 &&
                argv[0].type == ARG_TYPE_NUMBER &&
                argv[1].type == ARG_TYPE_STRING &&
                argv[2].type == ARG_TYPE_STRING &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) )
        {
            if(argv[0].value.number == 0)
            {
                // quxin ip_addr_t server0, server1;
                const char *serverstr = "208.67.222.222";
                ipaddr_aton(serverstr,&server0);
            }
            else if (!ipaddr_aton(argv[1].value.string, &server0) || 
                     !ipaddr_aton(argv[2].value.string, &server1) || 
                    ip_2_ip4(&server0)->addr == ip_2_ip4(&server1)->addr)
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
            
            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_OK;
        
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CIPDNS(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (kind & DCE_READ)
    {
        ip_addr_t server0,server1;
        ip_2_ip4(&server0)->addr = 0;
        ip_2_ip4(&server1)->addr = 0;

		hal_system_get_config(CUSTOMER_NV_TCPIP_DNS_SERVER0, &(server0), sizeof(server0));
		hal_system_get_config(CUSTOMER_NV_TCPIP_DNS_SERVER1, &(server1), sizeof(server1));

        char server0_str[16], server1_str[16];
        sprintf(server0_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&server0)));
        sprintf(server1_str, IP4_ADDR_STR, IP4_ADDR_VALUE(ip_2_ip4(&server1)));

         arg_t result[] = {
            {ARG_TYPE_STRING, .value.string = server0_str},
            {ARG_TYPE_STRING, .value.string = server1_str},
        };

        if(0 != ip_2_ip4(&server0)->addr)
            dce_emit_extended_result_code_with_args(dce, "CIPDNS_DEF", -1, &result[0], 1, 1, false);
        if(0 != ip_2_ip4(&server1)->addr)
            dce_emit_extended_result_code_with_args(dce, "CIPDNS_DEF", -1, &result[1], 1, 1, false);
        
        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_RC_OK;
    }
    else if ((kind & DCE_WRITE))
    {
        ip_addr_t server0, server1;
        ip_2_ip4(&server0)->addr = 0;
        ip_2_ip4(&server1)->addr = 0;
        if (argc == 1 && 
            argv[0].type == ARG_TYPE_NUMBER &&
            (argv[0].value.number == 0 || argv[0].value.number == 1))
        {
            const char *serverstr = "208.67.222.222";

            if(!ipaddr_aton(serverstr,&server0))
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);

                return DCE_RC_ERROR;
            }

            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else if(argc == 2 &&
                argv[0].type == ARG_TYPE_NUMBER &&
                argv[1].type == ARG_TYPE_STRING &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) )
        {
            if(argv[0].value.number == 0)
            {
                const char *serverstr = "208.67.222.222";
                ipaddr_aton(serverstr,&server0);
            }
            else if(!ipaddr_aton(argv[1].value.string, &server0))
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }

            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else if(argc == 3 &&
                argv[0].type == ARG_TYPE_NUMBER &&
                argv[1].type == ARG_TYPE_STRING &&
                argv[2].type == ARG_TYPE_STRING &&
                (argv[0].value.number == 0 || argv[0].value.number == 1) )
        {
            if(argv[0].value.number == 0)
            {
                // quxin ip_addr_t server0, server1;
                const char *serverstr = "208.67.222.222";
                ipaddr_aton(serverstr,&server0);
            }
            else if (!ipaddr_aton(argv[1].value.string, &server0) || 
                     !ipaddr_aton(argv[2].value.string, &server1) || 
                    ip_2_ip4(&server0)->addr == ip_2_ip4(&server1)->addr)
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_RC_ERROR;
            }
            
            dns_setserver(0,&server0);
            dns_setserver(1,&server1);

            argv[0].value.number == 0 ? (dns_auto_change = true) : (dns_auto_change = false);
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

		hal_system_set_config(CUSTOMER_NV_TCPIP_DNS_SERVER0, (void*)&(server0), sizeof(server0));
		hal_system_set_config(CUSTOMER_NV_TCPIP_DNS_SERVER1, (void*)&(server0), sizeof(server0));
		hal_system_set_config(CUSTOMER_NV_DNS_AUTO_CHANGE, (void*)&(dns_auto_change), sizeof(dns_auto_change));

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        return DCE_OK;
        
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SAVETRANSLINK(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t client = {0};
    int i = 0, ret;
    uint8_t transLinkEn = 0;
    char linktype[] = "TCP";
    char content[32] = {0};
    
    AT_CHECK_ERROR_RETURN(!(kind & DCE_WRITE));

    //link id
    if (cfg->ipmux) {
        // not support for mul connect
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

    AT_CHECK_ERROR_RETURN(argv[0].type != ARG_TYPE_NUMBER);
    
    if(argv[0].value.number == 0) {
        if (argc == 1) {
            hal_system_set_config(CUSTOMER_NV_TRANSLINK_EN, (void*)&(transLinkEn), sizeof(transLinkEn));
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_RC_OK;
        } else {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    } else if(argv[0].value.number == 1) {
        if(argc >= 3) {
            transLinkEn = argv[i].value.number;
            hal_system_set_config(CUSTOMER_NV_TRANSLINK_EN, (void*)&(transLinkEn), sizeof(transLinkEn));
            i++;
            
            memset(&client, 0, sizeof(client));
            client.type = conn_type_tcp;

    AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_STRING);
    if(!ipaddr_aton(argv[i].value.string, &client.ip_info.dst_ip)) {
        strlcpy(client.ip_info.domain_str, argv[i].value.string, sizeof(client.ip_info.domain_str));
    }
    strcpy(content, argv[i].value.string);
	hal_system_set_config(CUSTOMER_NV_TRANSLINK_IP, (void*)&content[0], sizeof(content) - 1);
    i++;

    AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
    client.ip_info.dst_port = argv[i].value.number;
	hal_system_set_config(CUSTOMER_NV_TRANSLINK_PORT, (void*)&argv[i].value.number, sizeof(argv[i].value.number));
    i++;

    if (argc > i) {
        //type
        AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_STRING);

                if (!strncmp(argv[i].value.string, "UDP", 3)) {
                    client.type = conn_type_udp;
                    strcpy(linktype, "UDP");
                } else {
                    client.type = conn_type_tcp;
                }
                i++;
            }
            hal_system_set_config(CUSTOMER_NV_TRANSLINK_TYPE, (void*)&linktype, sizeof(linktype) - 1);

            //keepalive or udp local port
            if (argc > i) {
                AT_CHECK_ERROR_RETURN(argv[i].type != ARG_TYPE_NUMBER);
                
                if (client.type != conn_type_udp) {
                    client.priv.tcp.keep_alive = argv[i].value.number;
                } else {
                    client.ip_info.src_port = argv[i].value.number;
                    hal_system_set_config(CUSTOMER_NV_TRANSLINK_UDPPORT, (void*)&argv[i].value.number, sizeof(argv[i].value.number));
                }
                i++;
            }
            
            ret = at_net_client_start(&client);
            if (ret) {
        		dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        		return DCE_RC_ERROR;
        	}

        } else {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    } 
    
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}


static uint8_t get_vif_by_dst_ip(ip4_addr_t* addr)
{
	uint8_t vif_id = STATION_IF;
	int i;
    struct netif *itf = ip4_route_src(NULL, addr);

    if (!itf)
        return vif_id;
    for(i = 0; i < MAX_IF; i++){
		if(itf == get_netif_by_index(i)){
			vif_id = i;
			break;
		}
	}
	return vif_id;
}

dce_result_t dce_handle_PING(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if(!(kind & DCE_WRITE) || argc != 1 || argv[0].type != ARG_TYPE_STRING)
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }

	ip4_addr_t ping_addr;

    if(*(argv[0].value.string) >= 0 && *(argv[0].value.string) <= 9)
    {
        if(!ip4addr_aton(argv[0].value.string, &ping_addr))
        {
            os_printf(LM_APP, LL_INFO, "ip address aton error\r\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }
    }
    else
    {
        struct hostent * host = gethostbyname(argv[0].value.string);

        if(!host)
        {
            os_printf(LM_APP, LL_INFO, "Get Ip Address fail\r\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_RC_ERROR;
        }

        ping_addr = *(ip4_addr_t*)host->h_addr_list[0];
    }
    
    if(ip4_addr_isany_val(ping_addr))
    {
		os_printf(LM_APP, LL_INFO, "Error!! There is no target address. run help ping\r\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		return DCE_RC_ERROR;
	}

    ping_parm_t ping_conn;

    ip4_addr_copy(*(ip_2_ip4(&ping_conn.addr)), ping_addr);
	ping_conn.packet_size = (u32_t)PING_DATA_SIZE;
	ping_conn.vif_id  = get_vif_by_dst_ip(&ping_addr);

    // 等到Ping结束
    if(ping_once(&ping_conn) == 1)
    {
        arg_t result[] = {
            {ARG_TYPE_NUMBER, .value.number = ping_conn.time_delay},
        };

        dce_emit_extended_result_code_with_args(dce, "", -1, result, 1, 1, false);
    }
    else
    {
        dce_emit_extended_result_code(dce, "+timeout", -1, 1);

        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		return DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

static const command_desc_t CIP_commands[] = {
    {"CIPSTAMAC"         , &dce_handle_CIPSTAMAC       , DCE_WRITE | DCE_READ},
// quxin 先注释，因为6600上也是根据STA自动生成（最低位取反）�?
//    {"CIPAPMAC"          , &dce_handle_CIPAPMAC        , DCE_WRITE | DCE_READ},
    {"CIPSTA"            , &dce_handle_CIPSTA          , DCE_WRITE | DCE_READ},
    {"CIPAP"             , &dce_handle_CIPAP           , DCE_WRITE | DCE_READ},
    {"CIPSTATUS"         , &dce_handle_CIPSTATUS       , DCE_EXEC},
//    {"CIPDOMAIN"         , &dce_handle_CIPDOMAIN       , DCE_EXEC},
    {"CIPSTART"          , &dce_handle_CIPSTART        , DCE_WRITE | DCE_READ},
//    {"CIPSSLSIZE"        , &dce_handle_CIPSSLSIZE      , DCE_WRITE | DCE_READ},
//    {"CIPSSLCCONF"       , &dce_handle_CIPSSLCCONF     , DCE_WRITE | DCE_READ},
    {"CIPSEND"           , &dce_handle_CIPSEND         , DCE_WRITE | DCE_EXEC},
//    {"CIPSENDEX"         , &dce_handle_CIPSENDEX       , DCE_WRITE},
//    {"CIPSENDBUF"        , &dce_handle_CIPSENDBUF      , DCE_WRITE},
//    {"CIPBUFRESET"       , &dce_handle_CIPBUFRESET     , DCE_WRITE},
//    {"CIPBUFSTATUS"      , &dce_handle_CIPBUFSTATUS    , DCE_WRITE},
//    {"CIPCHECKSEQ"       , &dce_handle_CIPCHECKSEQ     , DCE_WRITE},
    {"CIPCLOSE"          , &dce_handle_CIPCLOSE        , DCE_WRITE | DCE_EXEC},
//    {"CIFSR"             , &dce_handle_CIFSR           , DCE_EXEC},
    {"CIPMUX"            , &dce_handle_CIPMUX          , DCE_WRITE | DCE_READ},
    {"CIPSERVER"         , &dce_handle_CIPSERVER       , DCE_WRITE},
//    {"CIPSERVERMAXCONN"  , &dce_handle_CIPSERVERMAXCONN, DCE_WRITE | DCE_READ},
    {"CIPMODE"           , &dce_handle_CIPMODE         , DCE_WRITE | DCE_READ},
    {"CIPSTO"            , &dce_handle_CIPSTO          , DCE_WRITE | DCE_READ},
//    {"CIPDINFO"          , &dce_handle_CIPDINFO        , DCE_WRITE},
    {"CIPRECVMODE"       , &dce_handle_CIPRECVMODE     , DCE_WRITE | DCE_READ},
    {"CIPRECVDATA"       , &dce_handle_CIPRECVDATA     , DCE_WRITE | DCE_READ},
    {"CIPRECVLEN"        , &dce_handle_CIPRECVLEN      , DCE_WRITE | DCE_READ},
//    {"CIPSNTPCFG"        , &dce_handle_CIPSNTPCFG      , DCE_WRITE | DCE_READ},
//    {"CIPSNTPTIME"       , &dce_handle_CIPSNTPTIME     , DCE_READ},
//    {"CIPDNS"            , &dce_handle_CIPDNS          , DCE_WRITE | DCE_READ},
//    {"CIPDNS_CUR"        , &dce_handle_CIPDNS_CUR      , DCE_WRITE | DCE_READ},
//    {"CIPDNS_DEF"        , &dce_handle_CIPDNS          , DCE_WRITE | DCE_READ},
    {"SAVETRANSLINK"     , &dce_handle_SAVETRANSLINK   , DCE_WRITE},
    {"PING"              , &dce_handle_PING            , DCE_WRITE},
};

void dce_register_tcpip_commands(dce_t* dce)
{
    dce_register_command_group(dce, "TCPIP", CIP_commands, sizeof(CIP_commands) / sizeof(command_desc_t), 0);
    at_init_net_func(dce);
}

