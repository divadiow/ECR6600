#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_softap.h"
#include "joylink_thread.h"
#include "joylink_stdio.h"
#include "joylink_flash.h"
#include "joylink_extern.h"

#include  "system_wifi.h"
#include "system_network.h"

#define DHCP_START_IP 0x9B0A0A0A
#define DHCP_END_IP 0xFD0A0A0A

/**
 * @brief:set wifi mode to AP mode.
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_enter_ap_mode(void)
{
    user_dev_info_t dev_info;
    jl_platform_memset(&dev_info, 0, sizeof(dev_info));
    joylink_get_device_info(&dev_info);

    // open white wifi AP mode
    wifi_set_opmode(WIFI_MODE_AP);  /* set wifi mode */

    // 设置进入AP模式
    wifi_config_u wificonfig;
    char softap_pwd[64] = {0};
    char *ssid =dev_info.jlp_soft_ssid;
    char *pwd = softap_pwd;
    ip_info_t ip_info;

    jl_platform_memset((void *)&wificonfig, 0, sizeof(wifi_config_u));
    jl_platform_strcpy((char *)wificonfig.ap.ssid, ssid);
    wificonfig.ap.channel = 1;

    if (jl_platform_strlen((char *)pwd)) {
        jl_platform_strcpy((char *)wificonfig.ap.password, (char *)pwd);
        wificonfig.ap.authmode = AUTH_WPA_WPA2_PSK;
    } else {
        wificonfig.ap.authmode = AUTH_OPEN;
    }

    while (!wifi_is_ready()){
        jl_platform_printf("err! set AP/STA information must after wifi initialized!\n");
        jl_platform_msleep(3000); //延时5秒后在执行  
    }

    int ret = wifi_start_softap(&wificonfig);
	if (SYS_OK != ret) {
            jl_platform_printf("wifi_start_softap err:%d\n",ret);
        return ret;
    }

    jl_platform_memset(&ip_info, 0, sizeof(ip_info));
	IP4_ADDR(&ip_info.ip, 10, 11, 10, 6);
	IP4_ADDR(&ip_info.gw, 10, 11, 10, 6);
	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    set_softap_ipconfig(&ip_info);

    struct dhcps_lease dhcp_cfg_info;
    dhcp_cfg_info.enable = true;
    dhcp_cfg_info.start_ip.addr = DHCP_START_IP;
    dhcp_cfg_info.end_ip.addr = DHCP_END_IP;

    wifi_softap_set_dhcps_lease(&dhcp_cfg_info);

    return 0;
}

/**
 * @brief:System is expected to get product information that user regiested in Cloud platform.
 *
 * @param[out]: UUID , max length: 8
 * @param[out]: BRAND, max length: 8 
 * @param[out]: CID,   max length: 8
 * @param[out]: dev_soft_ssid max length: 32
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_get_product_info(char *uuid, char *brand, char *cid, char *dev_soft_ssid)
{
    user_dev_info_t dev_info;
    jl_platform_memset(&dev_info, 0, sizeof(dev_info));
    joylink_get_device_info(&dev_info);

    char *_uuid = dev_info.jlp_uuid;
    char *_dev_soft_ssid =dev_info.jlp_soft_ssid;
    char *_cid = dev_info.jlp_cid;
    char *_brand=dev_info.jlp_brand;

    jl_platform_strcpy(uuid, _uuid);
    jl_platform_strcpy(dev_soft_ssid, _dev_soft_ssid);
    jl_platform_strcpy(cid, _cid);
    jl_platform_strcpy(brand, _brand);
    
    return 0;
}

/**
 * @brief:System is expected to connect wifi router with the in parameters.
 *
 * @param[in]: ssid of wifi router
 * @param[in]: passwd  of wifi router
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_connect_wifi_router(char *ssid, char *passwd)
{
    jl_platform_printf("jl_softap_connect_wifi_router enter \n");   

    // step 1 close AP mode
    wifi_stop_softap();
    wifi_set_opmode(WIFI_MODE_STA);//切换至STA模式

    //config sta_mode info
    wifi_config_u sta_config;
    jl_platform_memset(&sta_config, 0, sizeof(sta_config));
    
    jl_platform_memcpy(sta_config.sta.ssid, ssid, jl_platform_strlen(ssid));
    sta_config.sta.channel = 1;
    if (passwd && jl_platform_strlen((const char *)passwd) >= 5) {
        jl_platform_printf("set pwd [%s]\n", passwd);
        jl_platform_memcpy(sta_config.sta.password, passwd, jl_platform_strlen(passwd));
    }
    // step 2 connect to the router
    wifi_stop_station();
    int ret = wifi_start_station(&sta_config);
	if (SYS_OK != ret) {
        jl_platform_printf("wifi_start_station err:%d\n",ret);
        return ret;
    }

     /* save it to flash */
    jl_flash_SetString("StaSSID", ssid, WIFI_SSID_MAX_LEN-1);
    jl_flash_SetString("StaPW", passwd, WIFI_PWD_MAX_LEN-1);
    return 0;
}

/**
 * @brief:set wifi mode to STA mode.
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_enter_sta_mode(char *ssid, char *passwd)
{
    wifi_set_opmode(WIFI_MODE_STA);//STA模式

    //config sta_mode info
    wifi_config_u sta_config;
    jl_platform_memset(&sta_config, 0, sizeof(sta_config));

    jl_platform_memcpy(sta_config.sta.ssid, ssid, WIFI_SSID_MAX_LEN-1);
    sta_config.sta.channel = 1;
    if (passwd && jl_platform_strlen((const char *)passwd) >= 5) {
        jl_platform_printf("set pwd [%s]\n", passwd);
        jl_platform_memcpy(sta_config.sta.password, passwd, WIFI_PWD_MAX_LEN-1);
    }
    // step 2 connect to the router
    wifi_stop_station();
    int ret = wifi_start_station(&sta_config);
	if (SYS_OK != ret) {
        jl_platform_printf("wifi_start_station err:%d\n",ret);
        return ret;
    }
    return 0;
}
