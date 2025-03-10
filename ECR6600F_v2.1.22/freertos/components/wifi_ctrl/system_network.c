/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        lixiao
 * Date:          2018-12-12
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

/****************************************************************************
*                                                Include files
****************************************************************************/
#include "lwip/apps/lwiperf.h"
#include "apps/ping/ping.h"
#include "lwip/err.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "lwip/ip_addr.h"
#include "system_network.h"
#include "system_wifi.h"
#include "system_log.h"
#include "wifi_common.h"

#include "lwip/icmp.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_ble.h"
#endif
#ifdef CONFIG_CUSTOM_FHOSTAPD
#include "ethernetif.h"
#endif

/****************************************************************************
*                                                Local Macros
****************************************************************************/


/****************************************************************************
*                                                Local Types
****************************************************************************/
static bool use_dhcp_server = true;
static bool use_dhcp_client = true;
static os_timer_handle_t dhcp_wait_timer = NULL;
static uint8_t dhcp_wait_vif = STATION_IF;
/****************************************************************************
*                                                Local Constants
****************************************************************************/
#ifdef LWIP_PING
const char ping_usage_str[] = "Usage: ping [-c count] [-i interval] [-s packetsize] destination\n"
                           "      `ping destination stop' for stopping\n"
                           "      `ping -st' for checking current ping applicatino status\n";
#endif
/****************************************************************************
*                                                Local Function Prototypes
****************************************************************************/

/****************************************************************************
*                                               Global Constants
****************************************************************************/

/****************************************************************************
*                                               Global Variables
****************************************************************************/

/****************************************************************************
*                                               Global Function Prototypes
****************************************************************************/

/****************************************************************************
*                                               Function Definitions
****************************************************************************/

#ifdef LWIP_PING
u8_t get_vif_by_dstip(ip_addr_t *addr)
{
    u8_t vif_id = STATION_IF;
    int i;
    struct netif *itf = ip_route(NULL, addr);

    if (!itf) {
        return vif_id;
    }
    for (i = STATION_IF; i < MAX_IF; i++) {
        if (itf == get_netif_by_index(i)) {
            vif_id = i;
            break;
        }
    }
    //return vif_id;
    return fhost_get_idx_by_netif(itf);//itf->num;
}

void ping_run(char *cmd)
{
    char *str = NULL;
    char *pcmdStr_log = NULL;
    char *pcmdStr = (char *)cmd;
    double interval = PING_DELAY_DEFAULT;
    u32_t packet_size= PING_DATA_SIZE;
    u32_t count = 0;
    ip_addr_t ping_addr;
    u8_t vif_id = 0xFF;
    u8_t rec_timeout = RCV_TIMEO;
    ping_parm_t* ping_conn = NULL;
    ping_parm_t* ping_stop_conn = NULL;
    int ip4addr_aton_is_valid = false;
	int ip6addr_aton_is_valid = false;

	#ifdef CONFIG_IPV6
	ip6_addr_set_any(&ping_addr.u_addr.ip6);
	ip4_addr_set_any(&ping_addr.u_addr.ip4);
	#else
    ip4_addr_set_any(&ping_addr);
	#endif

    if (pcmdStr != NULL) {
        memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));
    }

    ping_conn = (ping_parm_t *)os_zalloc(sizeof(ping_parm_t));
    if (ping_conn == NULL) {
        os_printf(LM_APP, LL_INFO, "memory allocation fail! [%s]\n", __func__);
        return;
    }
    ping_mutex_init();
    str = strtok_r(NULL, " ", &pcmdStr_log);

	#ifdef CONFIG_IPV6
	if (strcmp(str , "-6") == 0)
	{
		str = strtok_r(NULL, " ", &pcmdStr_log);
	}
	#endif

	//ip6addr_aton_is_valid = ip6addr_aton(str, &ping_addr.u_addr.ip6);
	//ip4addr_aton_is_valid = ip4addr_aton(str, &ping_addr);
	//support ping www.baidu.com
    int retry = 3;
    struct hostent *host = NULL;
    do {
        host = gethostbyname(str);
        if (host)
        {
        	#ifdef CONFIG_IPV6
			if (IP_IS_V6((ip_addr_t*)host->h_addr))
			{
				ip6addr_aton_is_valid = true;
				ip6_addr_copy(ping_addr.u_addr.ip6, ((ip_addr_t*)host->h_addr)->u_addr.ip6);
				//cli_printf("%s, %d: %s\r\n", __func__, __LINE__, ip6addr_ntoa(&ping_addr.u_addr.ip6));	
			}
			else
			{
				ip4addr_aton_is_valid = true;
				memcpy((char *)&ping_addr.u_addr.ip4, host->h_addr, host->h_length);	
			}
			#else
			ip4addr_aton_is_valid = true;
            memcpy((char *)&ping_addr, host->h_addr, host->h_length);			
			#endif
			
            break;
        }

        os_msleep(1);
    } while (--retry);

    if (ip4addr_aton_is_valid == true || ip6addr_aton_is_valid == true) 
	{
        vif_id = get_vif_by_dstip(&ping_addr);
        for (;;) {
            str = strtok_r(NULL, " ", &pcmdStr_log);			
            if (str == NULL || str[0] == '\0') {
                break;
            } 
			else if (strcmp(str , "stop") == 0) {
            	#ifdef CONFIG_IPV6
				if (ip6addr_aton_is_valid == true)
				{
					ping_stop_conn = ping6_get_session(&(ping_addr.u_addr.ip6));
				}
				else
				{
					ping_stop_conn = ping_get_session(&ping_addr.u_addr.ip4);
				}
				#else
                ping_stop_conn = ping_get_session((ip4_addr_t*)&ping_addr);
				#endif
				
                if (ping_stop_conn != NULL) {
                    ping_stop_conn->force_stop = 1;
                } else {
                    os_printf(LM_APP, LL_INFO, "Nothing to stop : wlan%d \n", vif_id);
                }
                os_free(ping_conn);
                return;
            } 
			else if (strcmp(str, "-i") == 0) {
                str = strtok_r(NULL, " ", &pcmdStr_log);
                interval = PING_DELAY_UNIT * atof(str);
            } 
			else if (strcmp(str, "-c") == 0) {
                str = strtok_r(NULL, " ", &pcmdStr_log);
                count = atoi(str);
            } 
			else if (strcmp(str, "-s") == 0) {
                str = strtok_r(NULL, " ", &pcmdStr_log);
                packet_size = atoi(str);
                if(packet_size > 12000) {
                    os_printf(LM_APP, LL_INFO, "input -s parameter Error, please input pkg size value at 0-12000 !!\n");
                    os_free(ping_conn);
                    return;
                }
            } 
			else if (strcmp(str, "-w") == 0) {
                str = strtok_r(NULL, " ", &pcmdStr_log);
                rec_timeout = atoi(str);
                if(rec_timeout > 100) {
                    os_printf(LM_APP, LL_INFO, "input -w parameter Error, please rec timeout value at 0-100 !!\n");
                    os_free(ping_conn);
                    return;
                }
            } 
			else {
                os_printf(LM_APP, LL_INFO, "Error!! '%s' is unknown option. Run help ping\n", str);
                os_free(ping_conn);
                return;
            }
        }
    } 
	else 
	{
        if (strcmp(str, "-st") == 0) {
			#ifdef CONFIG_IPV6
			if (ip6addr_aton_is_valid == true)
				ping6_list_display();
			else
			#endif
            	ping_list_display();
        } else if (strcmp(str, "-h") == 0) {
            os_printf(LM_APP, LL_INFO, "%s\r\n", ping_usage_str); //ping_usage();
        } else {
            os_printf(LM_APP, LL_INFO, "Error!! There is no target address. run help ping\n");
        }
        os_free(ping_conn);
        return;
    }

	#ifdef CONFIG_IPV6
	if ((ip6addr_aton_is_valid == true 
			&& ip6_addr_isany(&ping_addr.u_addr.ip6))
		|| (ip4_addr_isany_val(ping_addr.u_addr.ip4)))
	#else
    if (ip4_addr_isany_val(ping_addr))
	#endif
	{
        os_printf(LM_APP, LL_INFO, "Error!! There is no target address. run help ping\n");
        os_free(ping_conn);
        return;
    }

	#ifdef CONFIG_IPV6
	if (ip6addr_aton_is_valid == true)
	{
		ip6_addr_copy(ping_conn->addr.u_addr.ip6, ping_addr.u_addr.ip6);
	}
	else
	{
		ip4_addr_copy(ping_conn->addr.u_addr.ip4, ping_addr.u_addr.ip4);
	}
	#else
    ip4_addr_copy((ping_conn->addr), ping_addr);
	#endif
    ping_conn->interval = (u32_t)interval;
    ping_conn->target_count = (u32_t)count;
    ping_conn->packet_size = (u32_t)packet_size;
    ping_conn->vif_id  = vif_id;
    ping_conn->rec_timeout  = rec_timeout * 1000;

	#ifdef CONFIG_IPV6
	if ((ip6addr_aton_is_valid == true 
			&& ping6_get_session(&ping_conn->addr.u_addr.ip6) != NULL)
		|| (ping_get_session(&ping_conn->addr.u_addr.ip4) != NULL))
	#else
    if (ping_get_session((ip4_addr_t*)&ping_conn->addr) != NULL)
	#endif
	{
        os_printf(LM_APP, LL_INFO, "Ping application is already running\n");
        os_free(ping_conn);
        return;
    }

	#ifdef CONFIG_IPV6
	if (ip6addr_aton_is_valid == true)
	{
		ping_conn->task_handle = (xTaskHandle)ping6_init((void*)ping_conn);
	}
	else
	{
		ping_conn->task_handle = (xTaskHandle)ping_init((void*)ping_conn);
	}
	#else
    ping_conn->task_handle = (xTaskHandle)ping_init((void*)ping_conn);
	#endif
    
    if (ping_conn->task_handle == NULL) {
        os_free(ping_conn);
        return;
    }
}
#endif /* LWIP_PING */

static void set_netdb_ipconfig(int vif, struct ip_info *ipconfig)
{
    netif_db_t *netif_db = NULL;

    if (!IS_VALID_VIF(vif)) {
        return;
    }

    netif_db = get_netdb_by_index(vif);
	memcpy(&netif_db->ipconfig, ipconfig, sizeof(struct ip_info));
}

static void set_netdb_dhcps_cfg(int vif, void *dhcps_cfg)
{
    netif_db_t *netif_db = NULL;

    if (!IS_VALID_VIF(vif)) {
        return;
    }

    netif_db = get_netdb_by_index(vif);
	memcpy(&netif_db->dhcps_cfg, dhcps_cfg, sizeof(struct dhcps_lease));
}

void set_softap_ipconfig(struct ip_info *ipconfig)
{
    set_netdb_ipconfig(SOFTAP_IF, ipconfig);
}

void set_softap_dhcps_cfg(void *dhcps_cfg)
{
    set_netdb_dhcps_cfg(SOFTAP_IF, dhcps_cfg);
}

void set_sta_ipconfig(struct ip_info *ipconfig)
{
    netif_db_t *netif_db;

    netif_db = get_netdb_by_index(STATION_IF);
    netif_db->dhcp_stat = TCPIP_DHCP_STATIC;
    set_netdb_ipconfig(STATION_IF, ipconfig);
}

void wifi_dhcp_open(int vif)
{
    if (!IS_VALID_VIF(vif)) {
        return;
    }
	
	if(SOFTAP_IF == vif)
	{
		use_dhcp_server = true;
	}
	else
	{
		netif_db_t *netif_db;

		netif_db = get_netdb_by_index(STATION_IF);
		if(TCPIP_DHCP_STATIC == netif_db->dhcp_stat)
		{
			netif_db->dhcp_stat = TCPIP_DHCP_INIT;
		}
		use_dhcp_client = true;

	}
}

// close dhcp
void wifi_dhcp_close(int vif)
{
    if (!IS_VALID_VIF(vif)) {
        return;
    }

    (vif == SOFTAP_IF) ? (use_dhcp_server = false) : (use_dhcp_client = false);
} 

bool get_wifi_dhcp_use_status(int vif)
{
    bool ret;

    if (!IS_VALID_VIF(vif)) {
        return false;
    }
	
    (vif == SOFTAP_IF) ? (ret = use_dhcp_server) : (ret = use_dhcp_client);

    return ret;
}

int wifi_station_dhcpc_start(int vif)
{
    int8_t ret;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(vif)) {
        return -1;
    }

    netif_db = get_netdb_by_index(vif);

    if (netif_db->dhcp_stat == TCPIP_DHCP_STATIC) {
        wifi_set_ip_info(vif, &netif_db->ipconfig);
        return 0;
    }

    if (!use_dhcp_client) {
        return -1;
    }
    
#ifdef CONFIG_PSM_SUPER_LOWPOWER
	if(netif_db->dhcp_stat == TCPIP_DHCP_STARTED)
	{
		netifapi_dhcp_fast_select(netif_db->net_if);
		return 0;
	}
#endif

#ifdef CONFIG_IPV6
	netifapi_dhcp6_stop(netif_db->net_if);
#endif
    netifapi_dhcp_stop(netif_db->net_if);

    ip_addr_set_zero(&netif_db->net_if->ip_addr);
    ip_addr_set_zero(&netif_db->net_if->netmask);
    ip_addr_set_zero(&netif_db->net_if->gw);
    netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;

    ret = netifapi_dhcp_start(netif_db->net_if);	
    if (!ret) {
        netif_db->dhcp_stat = TCPIP_DHCP_STARTED;

        if (!dhcp_wait_timer)
        {
            dhcp_wait_vif = vif;
            dhcp_wait_timer = os_timer_create("DhcpWaitTimer", DHCP_MAX_WAIT_TIMEOUT, false, dhcp_wait_timeout_func, &dhcp_wait_vif);
        }
        if (dhcp_wait_timer)
        {
            os_timer_start(dhcp_wait_timer);
        }
    }
    SYS_LOGD("dhcpc start %s!", ret ? "fail" : "ok");

    return 0;
}
#ifdef CONFIG_PSM_SURPORT_close
extern void ap_cap_clear_interface();
extern void rf_ble_restore();

#endif
#ifdef CONFIG_PSM_SUPER_LOWPOWER
bool sta_remove_network_flag;
#endif
int wifi_station_dhcpc_stop(int vif)
{
    netif_db_t *netif_db = NULL;

    if (!IS_VALID_VIF(vif)) {
        return -1;
    }

    netif_db = get_netdb_by_index(vif);

    if (netif_db->dhcp_stat == TCPIP_DHCP_STATIC) {
        netif_set_addr(netif_db->net_if, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        //if (wifi_get_status(vif) == STA_STATUS_STOP) { //clear static dhcpc info
        //    netif_db->dhcp_stat = TCPIP_DHCP_INIT;
        //    memset(&netif_db->ipconfig, 0, sizeof(netif_db->ipconfig));
        //}
        return 0;
    }
#ifdef CONFIG_PSM_SUPER_LOWPOWER
    if (sta_remove_network_flag && (netif_db->dhcp_stat == TCPIP_DHCP_STARTED)) {
#else
    if (netif_db->dhcp_stat == TCPIP_DHCP_STARTED) {
#endif
		#ifdef CONFIG_IPV6
		netifapi_dhcp6_stop(netif_db->net_if);
		#endif
        netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;
        netifapi_dhcp_stop(netif_db->net_if);

#ifdef CONFIG_PSM_SUPER_LOWPOWER
		sta_remove_network_flag = false;
#endif
        SYS_LOGV("stop dhcp client");
    }
#ifdef CONFIG_PSM_SUPER_LOWPOWER
    else if(netif_db->dhcp_stat == TCPIP_DHCP_STARTED){
        netif_set_addr(netif_db->net_if, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
    }
#endif

#ifdef CONFIG_PSM_SURPORT_close
	psm_set_device_status(PSM_DEVICE_WIFI_STA,PSM_DEVICE_STATUS_ACTIVE);
	psm_pwr_mgt_ctrl(0);
	ap_cap_clear_interface();
	rf_ble_restore();
	psm_ble_psm_store_flag_op(true, false);
#endif

    discard_dhcp_wait_timer();
    return 0;
}

void dhcp_wait_timeout_func(os_timer_handle_t timer)
{
    uint8_t *vif = NULL;

    if (timer)
    {
        vif = os_timer_get_arg(timer);
        if (vif)
        {
           wifi_request_disconnect(*vif);
           wifi_config_commit(*vif);
        }
    }

    discard_dhcp_wait_timer();
}

void discard_dhcp_wait_timer()
{
    if (dhcp_wait_timer)
    {
        os_timer_stop(dhcp_wait_timer);
        os_timer_delete(dhcp_wait_timer);
        dhcp_wait_timer = NULL;
        dhcp_wait_vif = STATION_IF;
    }
}

/*get dhcpc/dhcpserver status*/
dhcp_status_t wifi_station_dhcpc_status(int vif)
{
    netif_db_t *netif_db = NULL;

    if (!IS_VALID_VIF(vif)) {
        return TCPIP_DHCP_STATUS_MAX;
    }
    netif_db = get_netdb_by_index(vif);
    return netif_db->dhcp_stat;
}

void wifi_softap_dhcps_start(int intf)
{
    //struct ip_info ipinfo_tmp;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(intf)) {
        return;
    }

    netif_db = get_netdb_by_index(intf);
    if (netif_db->dhcp_stat == TCPIP_DHCP_STARTED) {
        SYS_LOGD("start dhcps, dhcps already started.");
        return;
    }

	#ifdef CONFIG_IPV6
    if (ip4_addr_isany(&netif_db->ipconfig.ip.u_addr.ip4)) 
	{
        IP4_ADDR(&netif_db->ipconfig.ip.u_addr.ip4, 10, 10, 10, 1);
        IP4_ADDR(&netif_db->ipconfig.gw.u_addr.ip4, 10, 10, 10, 1);
        IP4_ADDR(&netif_db->ipconfig.netmask.u_addr.ip4, 255, 255, 255, 0);
    }
	#else
    if (ip4_addr_isany(&netif_db->ipconfig.ip)) 
	{
        IP4_ADDR(&netif_db->ipconfig.ip, 10, 10, 10, 1);
        IP4_ADDR(&netif_db->ipconfig.gw, 10, 10, 10, 1);
        IP4_ADDR(&netif_db->ipconfig.netmask, 255, 255, 255, 0);
    }
	#endif
#if defined ENABLE_LWIP_NAPT || (CONFIG_SPI_REPEATER && CONFIG_SPI_MASTER)
#ifdef CONFIG_IPV6
    if (ip4_addr_isany(&netif_db->ipconfig.dns1.u_addr.ip4)) {
        netif_db->ipconfig.dns1 = (IP_ADDR_ANY != dns_getserver(0)) ? *dns_getserver(0) : netif_db->ipconfig.ip;
        netif_db->ipconfig.dns2 = (IP_ADDR_ANY != dns_getserver(1)) ? *dns_getserver(1) : netif_db->ipconfig.ip;
    }
    netif_db->ipconfig.dns1.u_addr.ip4.addr = 0x8080808;
#else
    if (ip4_addr_isany(&netif_db->ipconfig.dns1)) {
        netif_db->ipconfig.dns1 = (IP_ADDR_ANY != dns_getserver(0)) ? *dns_getserver(0) : netif_db->ipconfig.ip;
        netif_db->ipconfig.dns2 = (IP_ADDR_ANY != dns_getserver(1)) ? *dns_getserver(1) : netif_db->ipconfig.ip;
    }
#endif
#endif
    wifi_set_ip_info(intf, &netif_db->ipconfig);

	
	wifi_softap_set_dhcps_lease(&netif_db->dhcps_cfg);
    wifi_softap_set_dhcps_lease_time(netif_db->dhcps_cfg.dhcps_lease);
	
    if (!use_dhcp_server) {
        return;
    }

    dhcps_start(netif_db->net_if , &netif_db->ipconfig);
    netif_db->dhcp_stat = TCPIP_DHCP_STARTED;
}

void wifi_softap_dhcps_stop(int intf)
{
    struct ip_info ipinfo_tmp;
    netif_db_t *netif_db;

    if (!IS_VALID_VIF(intf)) {
        return;
    }
    netif_db = get_netdb_by_index(intf);
    if (netif_db->dhcp_stat != TCPIP_DHCP_STARTED) {
        SYS_LOGD("stop dhcps, but dhcps not started.");
        return;
    }

    memset(&ipinfo_tmp, 0, sizeof(ipinfo_tmp));
    wifi_set_ip_info(intf, &ipinfo_tmp);

    dhcps_stop();
    netif_db->dhcp_stat = TCPIP_DHCP_STOPPED;
}

dhcp_status_t wifi_softap_dhcps_status(int vif)
{
    netif_db_t *netif_db;
    dhcp_status_t ret;

    if (!IS_VALID_VIF(vif)) {
        return TCPIP_DHCP_STATUS_MAX;
    }

    netif_db = get_netdb_by_index(vif);
    ret = netif_db->dhcp_stat;

    return ret;
}

void wifi_ifconfig_help_display(void)
{
    os_printf(LM_APP, LL_INFO, "Usage:\n");
    os_printf(LM_APP, LL_INFO, "    ifconfig <interface> [<address>] | [-6 <address>]\n");
    os_printf(LM_APP, LL_INFO, "    ifconfig <interface> [mtu <NN>]\n");
	#ifdef CONFIG_IPV6
	os_printf(LM_APP, LL_INFO, "    ifconfig <interface> [dhcp6 <N>]\n");
	#endif
}

void wifi_ifconfig_display(wifi_interface_e if_index)
{
    struct netif *netif_temp = get_netif_by_index(if_index);

    if (!netif_is_up(netif_temp)) {
        return;
    }

    os_printf(LM_APP, LL_INFO, "\nName:[wlan%d]     \n", if_index);
    os_printf(LM_APP, LL_INFO, "HWaddr:["MAC_STR"]\n", MAC_VALUE(netif_temp->hwaddr));
    os_printf(LM_APP, LL_INFO, "MTU:[%d]\n", netif_temp->mtu);
	#ifdef CONFIG_IPV6
    os_printf(LM_APP, LL_INFO, "inet addr:["IP4_ADDR_STR"]\n", IP4_ADDR_VALUE(&(netif_temp->ip_addr.u_addr.ip4)));
    os_printf(LM_APP, LL_INFO, "Mask:["IP4_ADDR_STR"]\n", IP4_ADDR_VALUE(&(netif_temp->netmask.u_addr.ip4)));
    os_printf(LM_APP, LL_INFO, "Gw:["IP4_ADDR_STR"]\n",IP4_ADDR_VALUE(&(netif_temp->gw.u_addr.ip4)));
	#else
    os_printf(LM_APP, LL_INFO, "inet addr:["IP4_ADDR_STR"]\n", IP4_ADDR_VALUE(&(netif_temp->ip_addr)));
    os_printf(LM_APP, LL_INFO, "Mask:["IP4_ADDR_STR"]\n", IP4_ADDR_VALUE(&(netif_temp->netmask)));
    os_printf(LM_APP, LL_INFO, "Gw:["IP4_ADDR_STR"]\n",IP4_ADDR_VALUE(&(netif_temp->gw)));
	#endif

	#ifdef CONFIG_IPV6
	os_printf(LM_APP, LL_INFO, "inet6 addr:[%s]\n", ip6addr_ntoa(netif_ip6_addr(netif_temp, 0)));	
    os_printf(LM_APP, LL_INFO, "inet6 addr:[%s]\n", ip6addr_ntoa(netif_ip6_addr(netif_temp, 1)));
	os_printf(LM_APP, LL_INFO, "inet6 addr:[%s]\n", ip6addr_ntoa(netif_ip6_addr(netif_temp, 2)));
	#endif
}

void wifi_ifconfig(char *cmd)
{
    char *str = NULL;
    char *pcmdStr_log = NULL;
    char *pcmdStr = cmd;
    struct ip_info info;
    struct netif *netif_temp;
    u8_t vif;

    if (pcmdStr != NULL) {
        memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));
    }

    str = strtok_r(NULL, " ", &pcmdStr_log);
    if (str == NULL || str[0] == '\0') {
        show_netif_status();
    } else {
        if (strcmp(str, "wl0") == 0) {
            netif_temp = get_netif_by_index(STATION_IF);
            vif = STATION_IF;
        } else if(strcmp(str, "wl1") == 0) {
            netif_temp = get_netif_by_index(SOFTAP_IF);
            vif = SOFTAP_IF;
        } else if(strcmp(str, "-h") == 0) {
            wifi_ifconfig_help_display();
            return;
        } else {
            os_printf(LM_APP, LL_INFO, "%s is unsupported. run ifconfig -h\n", str);
            return;
        }

        str = strtok_r(NULL, " ", &pcmdStr_log);
        if (str == NULL || str[0] == '\0') 
		{
            wifi_ifconfig_display(vif);
        } 
		else if (strcmp(str, "mtu") == 0)
		{
            str = strtok_r(NULL, " ", &pcmdStr_log);
            netif_temp->mtu = atoi(str);
		}
		#ifdef CONFIG_IPV6
		else if (strcmp(str, "dhcp6") == 0)
		{
            str = strtok_r(NULL, " ", &pcmdStr_log);
            netif_temp->dhcp6_state = atoi(str);	/*0-diable,1-stateless,2-stateful*/

			if (netif_temp->dhcp6_state == NETIF_DHCP6_STATE_LESS)
				netifapi_dhcp6_stateless_start(netif_temp);
			else if (netif_temp->dhcp6_state == NETIF_DHCP6_STATE_FULL)
				netifapi_dhcp6_stateful_start(netif_temp);
			else if (netif_temp->dhcp6_state == NETIF_DHCP6_STATE_DISABLE)
				netifapi_dhcp6_stop(netif_temp);
        }
		else if (strcmp(str, "-6") == 0)
		{
			str = strtok_r(NULL, " ", &pcmdStr_log);
			IP_SET_TYPE(&info.ip, IPADDR_TYPE_V6);
			ip6addr_aton(str, &info.ip.u_addr.ip6);
            netif_ip6_addr_set(netif_temp, 1, &info.ip.u_addr.ip6);
		}
		#endif
		else 
		{
            if (STATION_IF == vif)
            {
                if (TCPIP_DHCP_STATIC != wifi_station_dhcpc_status(vif))
                {
                    wifi_station_dhcpc_stop(vif);
                }
            }

			#ifdef CONFIG_IPV6
			IP_SET_TYPE(&info.ip, IPADDR_TYPE_V4);
			IP_SET_TYPE(&info.gw, IPADDR_TYPE_V4);
			IP_SET_TYPE(&info.netmask, IPADDR_TYPE_V4);

			ip4addr_aton(str, &info.ip.u_addr.ip4);
            IP4_ADDR(&info.gw.u_addr.ip4, ip4_addr1_16(&info.ip.u_addr.ip4), ip4_addr2_16(&info.ip.u_addr.ip4), ip4_addr3_16(&info.ip.u_addr.ip4), 1);
            IP4_ADDR(&info.netmask.u_addr.ip4, 255, 255, 255, 0);
            netif_set_addr(netif_temp, &info.ip.u_addr.ip4, &info.netmask.u_addr.ip4, &info.gw.u_addr.ip4);
			#else
            ip4addr_aton(str, &info.ip);
            IP4_ADDR(&info.gw, ip4_addr1_16(&info.ip), ip4_addr2_16(&info.ip), ip4_addr3_16(&info.ip), 1);
            IP4_ADDR(&info.netmask, 255, 255, 255, 0);
            netif_set_addr(netif_temp, &info.ip, &info.netmask, &info.gw);
			#endif

            if (SOFTAP_IF == vif)
            {
                set_softap_ipconfig(&info);
                
                if (TCPIP_DHCP_STARTED == wifi_softap_dhcps_status(vif))
                {
                    wifi_softap_dhcps_stop(vif);
                    wifi_softap_dhcps_start(vif);
                }
            }
        }
    }
}

bool wifi_set_ip_info(wifi_interface_e if_index, ip_info_t *info)
{
	#ifdef CONFIG_IPV6
	netif_set_addr(get_netif_by_index(if_index), &info->ip.u_addr.ip4, &info->netmask.u_addr.ip4, &info->gw.u_addr.ip4);

    if (!ip4_addr_isany(&info->dns1.u_addr.ip4) || !ip4_addr_isany(&info->dns1.u_addr.ip4)) {
        dns_setserver(0, &info->dns1);
        dns_setserver(1, &info->dns2);
    }
	#else
    netif_set_addr(get_netif_by_index(if_index), &info->ip, &info->netmask, &info->gw);

    if (!ip4_addr_isany(&info->dns1) || !ip4_addr_isany(&info->dns1)) {
        dns_setserver(0, &info->dns1);
        dns_setserver(1, &info->dns2);
    }
	#endif

    return true;
}

bool wifi_get_ip_info(wifi_interface_e if_index, struct ip_info *info)
{
    struct netif *nif = NULL;

    nif = get_netif_by_index(if_index);
	#ifdef CONFIG_IPV6
    info->ip.u_addr.ip4.addr = nif->ip_addr.u_addr.ip4.addr;
    info->netmask.u_addr.ip4.addr = nif->netmask.u_addr.ip4.addr;
    info->gw.u_addr.ip4.addr = nif->gw.u_addr.ip4.addr;
    info->dns1.u_addr.ip4.addr = dns_getserver(0)->u_addr.ip4.addr;
    info->dns1.u_addr.ip4.addr = dns_getserver(1)->u_addr.ip4.addr;
	#else
    info->ip = nif->ip_addr;
    info->netmask = nif->netmask;
    info->gw = nif->gw;
    info->dns1 = *dns_getserver(0);
    info->dns1 = *dns_getserver(1);
	#endif

    return true;
}

void wifi_send_heartbeat_pkg(void)
{
    int socket_tmp;
    uint16_t pkg_size;
    net_if_t *net_if;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    if ((socket_tmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        //W_DEBUG_D("ping: socket create fail\n");
        return;
    }

    pkg_size = sizeof(struct icmp_echo_hdr) + 32;
    iecho = (struct icmp_echo_hdr *)malloc(pkg_size);
    if (!iecho) {
        return;
    }

#ifndef CONFIG_CUSTOM_FHOSTAPD
    net_if = get_netif_by_index(STATION_IF);
#else
    net_if = tuya_ethernetif_get_netif_by_index(0);
#endif

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = WIFI_HEARTBEAT_PING_ID;
    iecho->seqno  = htons(1);

    /* fill the additional data buffer with some data */
    for(int i = 0; i < 32; i++) {
        ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }
    iecho->chksum = inet_chksum(iecho, pkg_size);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;

#ifndef CONFIG_CUSTOM_FHOSTAPD
#if LWIP_IPV4 && LWIP_IPV6
    to.sin_addr.s_addr = netif_ip_gw4(net_if)->u_addr.ip4.addr;
#elif LWIP_IPV4
    to.sin_addr.s_addr = netif_ip_gw4(net_if)->addr;
#endif
#else
    to.sin_addr.s_addr = netif_ip_gw4(net_if)->addr;
#endif


    sendto(socket_tmp, iecho, pkg_size, 0, (struct sockaddr*)&to, sizeof(to));

    os_free(iecho);
    lwip_close(socket_tmp);
}

