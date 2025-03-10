/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    event_default_handlers.c
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        lixiao
 * Date:          2018-12-21
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
#include "system_event.h"
#include "system_network.h"
#include <stddef.h>
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif
#include "hal_system.h"
#include "os_event.h"
//#include "system_network.h"

#ifdef CONFIG_VNET_SERVICE
#include "system_config.h"
void vnet_reg_wifilink_status(int link);
#endif
/****************************************************************************
*                                                Local Macros
****************************************************************************/

/****************************************************************************
*                                                Local Types
****************************************************************************/

/****************************************************************************
*                                                Local Constants
****************************************************************************/

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
/* 
 * Default event handler functions 
 * Any entry in this table which is disabled by config will have a NULL handler.
*/
static system_event_handler_t default_event_handlers[SYSTEM_EVENT_MAX] = { 0 };
#ifdef CONFIG_PSM_SURPORT
extern void ap_cap_record_interface();
#endif
extern EventGroupHandle_t os_event_handle;
static sys_err_t system_event_sta_got_ip_default(system_event_t *event)
{
    struct netif *netif_temp;

#if defined(CONFIG_SNTP)
    extern void sntp_start(void);
    sntp_start();
#endif

    netif_temp = get_netif_by_index(STATION_IF);
	#ifdef CONFIG_IPV6
	u32_t ip_addr = ntohl(ip4_addr_get_u32(&netif_temp->ip_addr.u_addr.ip4));
	#else
    u32_t ip_addr = ntohl(ip4_addr_get_u32(&netif_temp->ip_addr));
	#endif
    os_printf(LM_APP, LL_INFO, "ip addr:%d.%d.%d.%d\n", (ip_addr>>24)&0xff, (ip_addr>>16)&0xff,(ip_addr>>8)&0xff, ip_addr&0xff);
#ifdef CONFIG_PSM_SURPORT
	ap_cap_record_interface();
	psm_set_device_status(PSM_DEVICE_WIFI_STA,PSM_DEVICE_STATUS_IDLE);
	psm_set_wifi_status(PSM_CONNECTED);
#ifdef CONFIG_TWT_POWER_SAVE
	psm_twt_reconnect();
#endif
#endif

#ifdef CONFIG_VNET_SERVICE
    wifi_work_mode_e wifi_mode = WIFI_MODE_STA;
    vnet_reg_wifilink_status(1);
    hal_system_set_config(CUSTOMER_NV_WIFI_OP_MODE, &wifi_mode, sizeof(wifi_mode));
#endif

    discard_dhcp_wait_timer();

    xEventGroupSetBits(os_event_handle, OS_EVENT_STA_GOT_IP);

	return SYS_OK;
}

static sys_err_t system_event_sta_lost_ip_default(system_event_t *event)
{
#if defined(CONFIG_SNTP)
    extern void sntp_stop(void);
    sntp_stop();
#endif

#ifdef CONFIG_VNET_SERVICE
    vnet_reg_wifilink_status(0);
#endif

    if (event && (wifi_station_dhcpc_status(event->vif) == TCPIP_DHCP_STARTED))
    {
       wifi_request_disconnect(event->vif);
       wifi_config_commit(event->vif);
    }

    return SYS_OK;
}

sys_err_t system_event_ap_start_handle_default(system_event_t *event)
{
    wifi_set_status(event->vif, AP_STATUS_STARTED);
    wifi_softap_dhcps_start(event->vif);
#ifdef CONFIG_VNET_SERVICE
    wifi_ap_config_t softap_info = {0};
    wifi_work_mode_e wifi_mode = WIFI_MODE_AP;
    wpa_get_ap_info((char *)softap_info.ssid, softap_info.password, &softap_info.channel, &softap_info.authmode);
    wifi_save_ap_nv_info(&softap_info);
    hal_system_set_config(CUSTOMER_NV_WIFI_OP_MODE, &wifi_mode, sizeof(wifi_mode));
    vnet_reg_wifilink_status(1);
#endif

    return SYS_OK;
}

sys_err_t system_event_ap_stop_handle_default(system_event_t *event)
{
    wifi_set_status(event->vif, AP_STATUS_STOP);
    wifi_softap_dhcps_stop(event->vif);
#ifdef CONFIG_VNET_SERVICE
    vnet_reg_wifilink_status(0);
#endif

    return SYS_OK;
}

sys_err_t system_event_wifi_init_ready(system_event_t *event)
{
    wifi_set_ready_flag(true);
#if defined CONFIG_SPI_MASTER && CONFIG_SPI_REPEATER
        extern void vnet_reg_get_peer_value(void);
        vnet_reg_get_peer_value();
#endif

    return SYS_OK;
}

sys_err_t system_event_sta_start_handle_default(system_event_t *event)
{
    return SYS_OK;
}

sys_err_t system_event_sta_stop_handle_default(system_event_t *event)
{
    return SYS_OK;
}

sys_err_t system_event_sta_connected_handle_default(system_event_t *event)
{
    wifi_handle_sta_connect_event(event, true);
    return SYS_OK;
}

sys_err_t system_event_sta_disconnected_handle_default(system_event_t *event)
{
    wifi_handle_sta_connect_event(event, true);
    return SYS_OK;
}

sys_err_t system_event_scan_done_default(system_event_t *event)
{
    wifi_event_scan_handler(event->vif);
    return SYS_OK;
}

static sys_err_t sys_system_event_debug(system_event_t *event)
{
#if 0
    int vif;
    
    vif = event->vif;
    switch (event->event_id) {
    case SYSTEM_EVENT_WIFI_READY:
        SYS_LOGD("SYSTEM_EVENT_WIFI_READY");
        break;
    case SYSTEM_EVENT_SCAN_DONE:
    {
        system_event_sta_scan_done_t *scan_done __maybe_unused = &event->event_info.scan_done;
        SYS_LOGD("SYSTEM_EVENT_SCAN_DONE[%d], status:%lu, number:%d", vif, scan_done->status, scan_done->number);
        break;
    }
    case SYSTEM_EVENT_STA_START: 
        SYS_LOGD("SYSTEM_EVENT_STA_START[%d]", vif);
        break;
    case SYSTEM_EVENT_STA_STOP:
        SYS_LOGD("SYSTEM_EVENT_STA_STOP[%d]", vif);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
    {
        system_event_sta_connected_t *connected __maybe_unused = &event->event_info.connected;
        SYS_LOGD("SYSTEM_EVENT_STA_CONNECTED[%d], ssid:%s, ssid_len:%d, bssid:" MAC_STR ", channel:%d, authmode:%d", \
                vif, connected->ssid, connected->ssid_len, MAC_VALUE(connected->bssid), connected->channel, connected->authmode);
        break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
        system_event_sta_disconnected_t *disconnected __maybe_unused = &event->event_info.disconnected;
        SYS_LOGD("SYSTEM_EVENT_STA_DISCONNECTED[%d], ssid:%s, ssid_len:%d, bssid:" MAC_STR ", reason:%d", \
                vif, disconnected->ssid, disconnected->ssid_len, MAC_VALUE(disconnected->bssid), disconnected->reason);
        break;
    }
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    {
        system_event_sta_authmode_change_t *auth_change __maybe_unused = &event->event_info.auth_change;
        SYS_LOGD("SYSTEM_EVENT_STA_AUTHMODE_CHNAGE[%d], old_mode:%d, new_mode:%d", vif, auth_change->old_mode, auth_change->new_mode);
        break;
    }
    case SYSTEM_EVENT_STA_GOT_IP:
    {
        system_event_sta_got_ip_t *got_ip __maybe_unused = &event->event_info.got_ip;
        SYS_LOGD("SYSTEM_EVENT_STA_GOT_IP[%d], ip:" IPSTR ", mask:" IPSTR ", gw:" IPSTR,
                vif, IP2STR(&got_ip->ip_info.ip),
                IP2STR(&got_ip->ip_info.netmask),
                IP2STR(&got_ip->ip_info.gw));
        break;
    }
    case SYSTEM_EVENT_STA_LOST_IP:
        SYS_LOGD("SYSTEM_EVENT_STA_LOST_IP[%d]", vif);
        break;
    case SYSTEM_EVENT_AP_START:
        SYS_LOGD("SYSTEM_EVENT_AP_START[%d]", vif);
        break;
    case SYSTEM_EVENT_AP_STOP:
        SYS_LOGD("SYSTEM_EVENT_AP_STOP[%d]", vif);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
    {
        system_event_ap_staconnected_t *staconnected __maybe_unused = &event->event_info.sta_connected;
        SYS_LOGD("SYSTEM_EVENT_AP_STACONNECTED[%d], mac:" MAC_STR ", aid:%d", \
                vif, MAC_VALUE(staconnected->mac), staconnected->aid);
        break;
    }
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    {
        system_event_ap_stadisconnected_t *stadisconnected __maybe_unused = &event->event_info.sta_disconnected;
        SYS_LOGD("SYSTEM_EVENT_AP_STADISCONNECTED[%d], mac:" MAC_STR ", aid:%d", \
                vif, MAC_VALUE(stadisconnected->mac), stadisconnected->aid);
        break;
    }
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
        SYS_LOGD("SYSTEM_EVENT_AP_STAIPASSIGNED[%d]", vif);
        break;
    case SYSTEM_EVENT_STA_4WAY_HS_FAIL:
        SYS_LOGD("SYSTEM_EVENT_STA_4WAY_HS_FAIL[%d]", vif);
        break;
    default:
        SYS_LOGW("[%d]unexpected system event %d!", vif, event->event_id);
        break;
    }
#else
    //SYS_LOGD("vif[%d],event[%d]", event->vif, event->event_id);
#endif
    return SYS_OK;
}

sys_err_t sys_event_process_default(system_event_t *event)
{
    if (event == NULL) {
        //SYS_LOGE("Error: event is null!");
        return SYS_ERR;
    }
    if (!IS_VALID_VIF(event->vif)) {
        //SYS_LOGE("Error: event vif invalid!");
        return SYS_ERR;
    }

    sys_system_event_debug(event);
    if ((event->event_id < SYSTEM_EVENT_MAX)) {
        if (default_event_handlers[event->event_id] != NULL) {
            //SYS_LOGV("enter default callback");
            default_event_handlers[event->event_id](event);
            //SYS_LOGV("exit default callback");
        }
    } else {
        //SYS_LOGE("mismatch or invalid event, id=%d", event->event_id);
        return SYS_ERR;
    }
    return SYS_OK;
}

void sys_event_set_default_wifi_handlers()
{
#define REGISTER_DEFAULT_HANDLER(x,func)     default_event_handlers[x] = func;
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_WIFI_READY,       system_event_wifi_init_ready);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_START,        system_event_sta_start_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_STOP,         system_event_sta_stop_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_CONNECTED,    system_event_sta_connected_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_DISCONNECTED, system_event_sta_disconnected_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_GOT_IP,       system_event_sta_got_ip_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_STA_LOST_IP,      system_event_sta_lost_ip_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_AP_START,         system_event_ap_start_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_AP_STOP,          system_event_ap_stop_handle_default);
     REGISTER_DEFAULT_HANDLER(SYSTEM_EVENT_SCAN_DONE,        system_event_scan_done_default);
#undef REGISTER_DEFAULT_HANDLER
}

void sys_event_reset_wifi_handlers(system_event_id_t eventid, system_event_handler_t func)
{
    if (eventid < SYSTEM_EVENT_MAX) {
        default_event_handlers[eventid] = func;
    }
}
