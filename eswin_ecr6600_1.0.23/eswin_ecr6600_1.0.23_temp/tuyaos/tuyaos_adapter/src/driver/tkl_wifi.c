/**
 * @file tuya_os_adapt_wifi.c
 * @brief wifi操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#if 1
#include "tkl_wifi.h"
#include "tkl_output.h"
#include "tkl_semaphore.h"
#include "tkl_thread.h"
#include "tkl_queue.h"
#include "tkl_memory.h"
#if CONFIG_TUYA_HOSTAPD_SUPPORT
#include "tuya_wlan_auth.h"
#endif

#include "wifi_sniffer.h"
#include "hal_system.h"
#include "system_config.h"
#include "format_conversion.h"
#include "oshal.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

//#define  cli_printf tuya_cli_printf

/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************variable define********************
***********************************************************/
static WF_WK_MD_E g_wifi_work_mode = WWM_STATIONAP;
static SNIFFER_CALLBACK tuya_sniffer_cb = NULL;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;



extern void cli_printf(const char *f, ...);
extern int net_packet_send(int vif_idx, const uint8_t *data, int data_len, int raw_flag);
extern void get_channel(uint8_t *ch);
extern int fhost_set_channel(int ch);
extern int wpa_drv_set_country(const char *p_country_code);
extern void ap_cap_record_interface();
/***********************************************************
*************************function define********************
***********************************************************/
#if 0
/**
 * @brief scan current environment and obtain all the ap
 *        infos in current environment
 * 
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_all_ap_scan(AP_IF_S **ap_ary, unsigned int *num)
{
    // to do
}

/*
 * @param[in]       ssid        the specific ssid
 * @param[out]      ap          the ap info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_assign_ap_scan(const signed char *ssid, AP_IF_S **ap)
{
    // to do
}

/**
 * @brief release the memory malloced in <tuya_os_adapt_wifi_all_ap_scan>
 *        and <tuya_os_adapt_wifi_assign_ap_scan> if needed. tuya-sdk
 *        will call this function when the ap info is no use.
 * 
 * @param[in]       ap          the ap info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_release_ap(AP_IF_S *ap)
{
     // to do
    
    return OPRT_OK;
}
#endif
/**
 * @brief set wifi interface work channel
 * 
 * @param[in]       chan        the channel to set
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_cur_channel(CONST UCHAR_T chan)
{
    //Set chan when current channel and the requested channel are not equal
    if(0 != fhost_set_channel(chan))
    {
        return OPRT_OS_ADAPTER_CHAN_SET_FAILED;
    }

    return OPRT_OK;

}

/**
 * @brief get wifi interface work channel
 * 
 * @param[out]      chan        the channel wifi works
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T *chan)
{

    get_channel(chan); 
    
    return OPRT_OK;
}

static int wifi_sniffer_start(wifi_promiscuous_cb_t cb, wifi_promiscuous_filter_t *filter)
{
    //os_printf(LM_APP, LL_INFO, "wifi_sniffer_start\n");
    wifi_set_promiscuous_filter(filter);
    wifi_set_promiscuous_rx_cb(cb);
    wifi_set_promiscuous(true);

    return 0;
}

static int wifi_sniffer_stop(void)
{
    //os_printf(LM_APP, LL_INFO, "wifi_sniffer_stop\n");

    wifi_set_promiscuous(false);
    wifi_set_promiscuous_rx_cb(NULL);

    return 0;
}
static void wifi_promiscuous_mgnt_cb(uint8_t *data, uint16_t len, wifi_promiscuous_pkt_type_t type)
{
	if(data == NULL || len == 0){
        return;                                
    }
	
    if (mgnt_recv_cb == NULL) {
        return;
    }

    mgnt_recv_cb(data, len);
}

static void sniffer_local_cb(const uint8_t *buf, const uint16_t len, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *sniffer_pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *data = sniffer_pkt->payload;

    if (tuya_sniffer_cb) {
        tuya_sniffer_cb(data, len, sniffer_pkt->rx_ctrl.rssi);
    }

    if (type == WIFI_PKT_MGMT) {
	    wifi_promiscuous_mgnt_cb(data, len, type);
    }
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 * 
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_sniffer(CONST BOOL_T en, CONST SNIFFER_CALLBACK cb)
{
    #if 1
	//unsigned char chan;
	//int result;
	
    if((en) && (NULL == cb)) {
        return OPRT_INVALID_PARM;
    }
    //os_printf(LM_APP, LL_INFO, "%s: %d\n",__func__, en);

    wifi_promiscuous_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;

    if(en)
    {
        //os_printf(LM_APP, LL_INFO, "stop first\n");
        //wifi_sniffer_stop();
        tuya_sniffer_cb = cb;
        wifi_sniffer_start((wifi_promiscuous_cb_t)sniffer_local_cb, &filter);
		//result = tkl_wifi_get_cur_channel(&chan);
		//os_printf(LM_APP, LL_INFO, "cur_channel: ret:%d channel=%d\r\n", result, chan);
		//tkl_wifi_set_cur_channel(1);
    }
    else
    {
        if (!mgnt_recv_cb)
        {
            wifi_sniffer_stop();
        }

        tuya_sniffer_cb = NULL;
    }
#endif
    return OPRT_OK;
}
#if 0
/**
 * @brief get wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_ip(const WF_IF_E wf, NW_IP_S *ip)
{

}
#endif
/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return  OPRT_OK: success  Other: fail
 */
 
int tkl_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
{
    //char mac_str[18] = {0};
    int ret;
    //int i;
    
    if(NULL == mac)
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    if ((wf == WF_STATION) || (wf == WF_AP))
    {
        ret = hal_system_get_config(STA_MAC, mac, sizeof(mac));
        if (ret <= 0)
        {
            os_printf(LM_APP, LL_INFO, "read mac err!\r\n");
            return OPRT_OS_ADAPTER_COM_ERROR; 
        }
        #if 0
        for (i=0; i<6; i++)
        {
            mac->mac[i] = hex2num(mac_str[i*3]) * 0x10 + hex2num(mac_str[i*3+1]);
        }
        #endif
        if (wf == WF_AP)
        {
            mac->mac[5] ^= 0x01;
        }
#if 0
        for (int i = 0; i < 6; i++)
        {
            cli_printf("read MAC[%d]: =0x%2x\r\n", i, mac->mac[i]);
        }
#endif
        return OPRT_OK; 
    }
    else
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
}

/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac)
{
    int i;
    char string_mac[18] = {0};
    
    if(NULL == mac)
    {
        os_printf(LM_APP, LL_INFO, " mac is null!\r\n");
        return OPRT_INVALID_PARM;
    }

    for(i = 0; i < 6;i++ ) {
        sprintf(string_mac+3*i, "%02X", mac->mac[i]);
        string_mac[3*i + 2] = ':';
    }
    string_mac[17] = '\0';
    
    //os_printf(LM_APP, LL_INFO, "set mac %s\r\n", string_mac);
    //fhost_config_set_mac(string_mac);
    int ret = hal_system_set_mac(MAC_SAVE_TO_AMT, string_mac);
    if (ret != 0)
    {
        os_printf(LM_APP, LL_INFO, "set_mac return err %d!\r\n", ret);
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
#if 0 
    wifi_interface_e vif = tuya_get_transa_vif(wf);
   
    if(SYS_OK != wifi_set_mac_addr(vif, mac->mac))
    {
        return OPRT_OS_ADAPTER_MAC_SET_FAILED;
    }
#endif     
    //ef_set_env_blob(NV_WIFI_STA_MAC, string_mac, sizeof(string_mac));
//    amt_mac_write(NV_WIFI_STA_MAC, string_mac, sizeof(string_mac));
    return OPRT_OK;
}

//static inline wifi_work_mode_e tuya_get_transa_wm(WF_WK_MD_E mode)
//{
//    return (WWM_STATION == mode) ? WIFI_MODE_STA : (WWM_SOFTAP == mode ? WIFI_MODE_AP : WIFI_MODE_AP_STA);
//}

/**
 * @brief set wifi work mode
 * 
 * @param[in]       mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_work_mode(CONST WF_WK_MD_E mode)
{
    if(mode > WWM_STATIONAP)
    {
        return OPRT_INVALID_PARM;
    }

    //os_printf(LM_APP, LL_INFO, "--%s:%d--workmode %d\n", __func__, __LINE__, mode);
    if(WWM_POWERDOWN == mode)
    {
        //tuya_lowpower_set();        	
#ifdef CONFIG_PSM_SURPORT
		psm_set_lowpower();
		//os_printf(LM_APP, LL_INFO, "%s: lowpower enabled\n", __func__);
#endif
    }
    else if(WWM_SNIFFER == mode)
    {
        //os_printf(LM_APP, LL_INFO, "set sniffer mode\n");
		
#ifdef CONFIG_PSM_SURPORT
        psm_set_normal();
#endif
    }
    else
    {
#ifdef CONFIG_PSM_SURPORT
        psm_set_normal();
#endif
        #if 0
        tuya_os_adapt_wifi_sniffer_set(0, NULL);
        
        wifi_work_mode_e transa_wm = tuya_get_transa_wm(mode);        
        wifi_set_opmode(transa_wm);
        if(WWM_SOFTAP == mode || WWM_STATIONAP == mode)
        {
            /*if(SYS_OK != wifi_set_ap_sta_num(g_ap_sta_num))
            {
                cli_printf("set ap_sta_num fail\n");
                return OPRT_COM_ERROR;
            }*/
        }
        #endif
    }

    g_wifi_work_mode = mode;
    return OPRT_OK;
}

/**
 * @brief get wifi work mode
 * 
 * @param[out]      mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    //cli_printf("%s %d\r\n", __func__, g_wifi_work_mode);
    *mode = g_wifi_work_mode;
    return OPRT_OK;
}
#if 0
//#if CONFIG_TUYA_HOSTAPD_SUPPORT
/***********************************************************
*  Function: tuya_hal_wifi_sta_conn_prev
*  Input: params
*  Output:None
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_sta_conn_prev(const signed char *ssid, const signed char *passwd)
{

}

/***********************************************************
*  Function: tuya_hal_wifi_sta_conn_post
*  Input: params
*  Output:None
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_sta_conn_post(void)
{

}
//#endif /* CONFIG_TUYA_HOSTAPD_SUPPORT */
/**
 * @brief connect wifi with ssid and passwd
 * 
 * @param[in]       ssid
 * @param[in]       passwd
 * @attention only support wap/wap2 
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_connect(const signed char *ssid, const signed char *passwd)
{

}

//#if CONFIG_TUYA_HOSTAPD_SUPPORT
/***********************************************************
*  Function: tuya_hal_wifi_sta_disconn_prev
*  Input: params
*  Output: mode
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_sta_disconn_prev(void)
{

}

/***********************************************************
*  Function: tuya_hal_wifi_sta_disconn_post
*  Input: params
*  Output: mode
*  Return: int
***********************************************************/

int tuya_os_adapt_wifi_sta_disconn_post(void)
{

}
//#endif /* CONFIG_TUYA_HOSTAPD_SUPPORT */

/**
 * @brief disconnect wifi from connect ap
 * 
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_disconnect(void)
{

}

/**
 * @brief get wifi connect rssi
 * 
 * @param[out]      rssi        the return rssi
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_get_conn_ap_rssi(signed char *rssi)
{

}

/**
 * @brief get wifi bssid
 * 
 * @param[out]      mac         uplink mac
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_bssid(unsigned char *mac)
{
   
}


/**
 * @brief get wifi station work status
 * 
 * @param[out]      stat        the wifi station work status
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_get_status(WF_STATION_STAT_E *stat)
{

}

//#if CONFIG_TUYA_HOSTAPD_SUPPORT
/***********************************************************
*  Function: tuya_hal_wifi_ap_start_prev
*  Input: params
*  Output: None
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_ap_start_prev(const WF_AP_CFG_IF_S *cfg)
{

}

/***********************************************************
*  Function: tuya_hal_wifi_ap_start_post
*  Input: params
*  Output: None
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_ap_start_post(const WF_AP_CFG_IF_S *cfg)
{

}
//#endif /* CONFIG_TUYA_HOSTAPD_SUPPORT */

/**
 * @brief start a soft ap
 * 
 * @param[in]       cfg         the soft ap config
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_ap_start(const WF_AP_CFG_IF_S *cfg)
{

}

//#if CONFIG_TUYA_HOSTAPD_SUPPORT
/***********************************************************
*  Function: tuya_hal_wifi_ap_stop_prev
*  Input: params
*  Output: none
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_ap_stop_prev(void)
{

}

/***********************************************************
*  Function: tuya_hal_wifi_ap_stop_post
*  Input: params
*  Output: none
*  Return: int
***********************************************************/
int tuya_os_adapt_wifi_ap_stop_post(void)
{

}
//#endif /* CONFIG_TUYA_HOSTAPD_SUPPORT */

/**
 * @brief stop a soft ap
 * 
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_ap_stop(void)
{

}

/**
 * @brief 快连信息输出给SDK，SDK实现快连
 * 
 * @param[in] FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_connected_ap_info_v2(FAST_WF_CONNECTED_AP_INFO_V2_S **fast_ap_info)
{

}

/**
 * @brief : fast connect prev
 * @param[in]      fast_ap_info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_fast_station_connect_v2_prev(const FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info)
{

}


/**
 * @brief 快连信息SDK输出到开发环境中，开发环境负责保存
 * @param[in] FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_fast_station_connect_v2(const FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info)
{

}

/**
 * @brief : fast connect post
 * @param[in]      fast_ap_info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_fast_station_connect_v2_post(const FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info)
{
}
#endif
/**
 * @brief set wifi country code
 * 
 * @param[in]       ccode  country code
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_country_code(CONST COUNTRY_CODE_E ccode)
{
    char *str = NULL;

    //os_printf(LM_APP, LL_INFO, "--%s:%d--  [%d]\n", __func__, __LINE__, ccode);

    if(ccode == COUNTRY_CODE_CN)
    {
        str = "CN";
    }
    else if(ccode == COUNTRY_CODE_US)
    {
        str = "US";
    }
    else if(ccode == COUNTRY_CODE_JP)
    {
        str = "JP";
    }
    else if(ccode == COUNTRY_CODE_EU)
    {
        str = "AL";
    }
    else
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
	wpa_drv_set_country((const char *)str);

    return OPRT_OK;
}

/**
 * @brief send wifi management
 * 
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_send_mgnt(CONST UCHAR_T *buf, CONST UINT_T len)
{
    int ret = 0;
    //TODO: maybe sta or softap?
    if (0 == net_packet_send(0, buf, len, 1))
    {
        ret = OPRT_OK;
    }
    else
    {
        ret = OPRT_OS_ADAPTER_MGNT_SEND_FAILED;
    }
    
    return ret;
}

/**
 * @brief register receive wifi management callback
 * 
 * @param[in]       enable
 * @param[in]       recv_cb     receive callback
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb)
{
    if (enable)
    {
        mgnt_recv_cb = recv_cb;
		
        wifi_promiscuous_filter_t filter;
        memset(&filter, 0, sizeof(filter));
        filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;

        //wifi_sniffer_stop();
        wifi_sniffer_start((wifi_promiscuous_cb_t)sniffer_local_cb, &filter);

        //os_printf(LM_APP, LL_INFO, "enable mgnt cb succ\n");
    }
    else 
    {
        if (!tuya_sniffer_cb)
        {
            wifi_sniffer_stop();
        }
        
        mgnt_recv_cb = NULL;
        //os_printf(LM_APP, LL_INFO, "disable mgnt cb\n");
    }
    
    return OPRT_OK;
}

/**
 * @brief set wifi lowerpower mode
 * 
 * @param[in]       en
 * @param[in]       dtim
 * @return  OPRT_OK: success  Other: fail
 */
int tkl_wifi_set_lp_mode(const BOOL_T en, const unsigned char dtim)
{
    #ifdef CONFIG_PSM_SURPORT
	if (en)
	{
        ap_cap_record_interface();
        psm_set_device_status(PSM_DEVICE_WIFI_STA,PSM_DEVICE_STATUS_IDLE);
        
		psm_infs.wifi_infs.listen_interval_req = dtim;
		psm_set_psm_enable(SLEEP_ENABLE);
		PSM_SLEEP_SET(MODEM_SLEEP);
		
		os_printf(LM_APP, LL_INFO, "%s: lowerpower enable\n", __func__);
		
	}
	
	else
	{
		psm_infs.wifi_infs.listen_interval_req = 0;
		psm_set_psm_enable(SLEEP_DISABLE);
		PSM_SLEEP_CLEAR(MODEM_SLEEP);
        psm_pwr_mgt_ctrl(0);
	    os_printf(LM_APP, LL_INFO, "%s: lowerpower disable\n", __func__);
	}
    #endif
    return OPRT_OK;
}

/**
 * @brief get wifi rf param exist or not
 * 
 * @param[in]       none
 * @return  true: rf param exist  Other: fail
 */
BOOL_T tkl_wifi_set_rf_calibrated(VOID_T)
{
    int rfFinishFlag = 0;

    hal_system_get_config(AMT_FINISH_FLAG, &rfFinishFlag, sizeof(rfFinishFlag));

    return rfFinishFlag;
}

OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    return 0;
}

OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd,  VOID *args)
{
    return OPRT_NOT_SUPPORTED;
}

#if !defined(TUYA_HOSTAPD_SUPPORT) || (TUYA_HOSTAPD_SUPPORT == 0)
/**
 * @brief get wifi bssid
 * 
 * @param[out]      mac         uplink mac
 * @return  OPRT_OK: success  Other: fail
 */
int tkl_wifi_get_bssid(unsigned char *mac)
{
    return 0;
}

OPERATE_RET tkl_wifi_station_connect(CONST SCHAR_T *ssid, CONST SCHAR_T *passwd)
{
    return 0;
}

OPERATE_RET tkl_wifi_station_disconnect(VOID_T)
{
    return 0;
}

OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    return 0;
}

OPERATE_RET tkl_wifi_all_ap_scan(AP_IF_S **ap_ary, UINT_T *num)
{
    return 0;
}

OPERATE_RET tkl_wifi_get_ip(CONST WF_IF_E wf, NW_IP_S *ip)
{
    return 0;
}

OPERATE_RET tkl_wifi_release_ap(AP_IF_S *ap)
{
    return 0;
}

OPERATE_RET tkl_wifi_start_ap(CONST WF_AP_CFG_IF_S *cfg)
{
    return 0;
}

OPERATE_RET tkl_wifi_stop_ap(VOID_T)
{
    return 0;
}

OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info)
{
    return 0;
}

OPERATE_RET tkl_wifi_scan_ap(CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num)
{
    return 0;
}

//OPERATE_RET tkl_fast_station_connect_v2(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
//{
//    return 0;
//}

OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T *rssi)
{
    return 0;
}

OPERATE_RET tkl_wifi_station_fast_connect(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    return 0;
}
#endif /* TUYA_HOSTAPD_SUPPORT */

#endif