#include "easyflash.h"
#include "hal_system.h"
#include "stdio.h"
#include "system_def.h"
#include "system_wifi.h"
#include "os_event.h"

#include <string.h>
#ifdef CONFIG_APP_AT_COMMAND
#include "at.h"
#endif

#define CONFIG_WIFI_CONN_CONNET_TIMEOUT 20000 /* 20s */
#define MS_TO_TICKS( ms )         ( TickType_t )( ( TickType_t ) ( ms ) / portTICK_PERIOD_MS )


bool is_wifi_conn = false;

#if defined(CONFIG_OTA_LOCAL)
bool is_local_ota_conn = false;

static char CONFIG_WIFI_CONN_SSID[32] = {0};
static char CONFIG_WIFI_CONN_PASSWD[32] = {0};
static char CONFIG_WIFI_CONN_CHANNEL = 0;

static int wifi_conn_scan(void)
{
    wifi_scan_config_t scan_cfg;
    wifi_info_t ap_info;
    char ssid_buf[32] = {0};
    int retry;

    snprintf(ssid_buf, sizeof(ssid_buf), "%s", CONFIG_WIFI_CONN_SSID);
    memset(&scan_cfg, 0, sizeof(wifi_scan_config_t));
    scan_cfg.ssid = (unsigned char *)ssid_buf;
    scan_cfg.channel = CONFIG_WIFI_CONN_CHANNEL;

    for(retry = 0; retry < 2; retry++) {
        if(wifi_scan_start(1, &scan_cfg) != 0) {
            os_printf(LM_APP, LL_ERR, "can not scan fac wifi\r\n");
            continue;
        }

        if (wpa_get_scan_num() == 0) {
            os_printf(LM_APP, LL_ERR, "ap num is 0\r\n");
            continue;
        }

        memset((void *)&ap_info, 0, sizeof(wifi_info_t));
        wifi_get_scan_result(0, &ap_info);

        if (strcmp((char *)ap_info.ssid, (char *)ssid_buf) == 0) {
            if (CONFIG_WIFI_CONN_CHANNEL == 0 || ap_info.channel == CONFIG_WIFI_CONN_CHANNEL)
                break;
        }
    }

    if(retry == 2) {
        os_printf(LM_APP, LL_ERR, "enter normal mode \r\n");
        return -1;
    }

    return 0;
}

static int wifi_conn_set_sta(void)
{
    wifi_config_u sta_cfg;

    wifi_sniffer_stop();
    memset(&sta_cfg, 0, sizeof(sta_cfg));

    strlcpy(sta_cfg.sta.password, CONFIG_WIFI_CONN_PASSWD , WIFI_PWD_MAX_LEN);
    strlcpy((char *)sta_cfg.sta.ssid, CONFIG_WIFI_CONN_SSID , WIFI_SSID_MAX_LEN);

    if (wifi_start_station(&sta_cfg) != 0) {
        return -1;
    }

    return 0;
}
#endif

#if 0
static int wifi_conn_assoc(void)
{
    struct ip_info if_ip;
    int wifi_status = 0;
    int connect_cnt = 0;

    //is_remove_network = false;

    wifi_status = wifi_get_status(STATION_IF);
    wifi_get_ip_info(STATION_IF, &if_ip);

#ifdef CONFIG_IPV6
    while (((wifi_status !=  STA_STATUS_CONNECTED) || ip4_addr_isany_val(if_ip.ip.u_addr.ip4)) && (connect_cnt != CONFIG_WIFI_CONN_CONNET_TIMEOUT))
#else
    while (((wifi_status !=  STA_STATUS_CONNECTED) || ip4_addr_isany_val(if_ip.ip)) && (connect_cnt != CONFIG_WIFI_CONN_CONNET_TIMEOUT))
#endif
    {
       wifi_status = wifi_get_status(STATION_IF);
       wifi_get_ip_info(STATION_IF, &if_ip);
       vTaskDelay(pdMS_TO_TICKS(1000));
       connect_cnt++;
    }

    if (connect_cnt == CONFIG_WIFI_CONN_CONNET_TIMEOUT)
    {
        return -1;
    }

    return 0;
}
#endif

static int g_wifi_conn_task_handle;
extern EventGroupHandle_t os_event_handle;
static void wifi_conn_task(void *arg)
{
    EventBits_t r_event;

    while (!wifi_is_ready()) {
        os_msleep(50);
    }

#ifdef CONFIG_APP_AT_COMMAND
    AT_Net_Info_Init();
#endif

#if defined(CONFIG_OTA_LOCAL)
    char channel_str[2] = {0};

    if ((develop_get_env_blob("local_ssid", CONFIG_WIFI_CONN_SSID, 32, NULL) <= 0) ||
        (develop_get_env_blob("local_password", CONFIG_WIFI_CONN_PASSWD, 32, NULL) <= 0) ||
        (develop_get_env_blob("local_channel", channel_str, 2, NULL) <= 0))  {
        os_printf(LM_APP, LL_ERR, "Can not find local_ssid or local_password or local_channel in dnv.\n");
        memcpy(CONFIG_WIFI_CONN_SSID, "local_ota_test", 14);
        memcpy(CONFIG_WIFI_CONN_PASSWD, "12345678", 8);
        CONFIG_WIFI_CONN_CHANNEL = 7;
    }
    else {
        CONFIG_WIFI_CONN_CHANNEL = atoi(channel_str);
    }
    os_printf(LM_APP, LL_INFO, "local ota: ssid:%s  password:%s  channel:%d\n", CONFIG_WIFI_CONN_SSID, CONFIG_WIFI_CONN_PASSWD, CONFIG_WIFI_CONN_CHANNEL);
    if (wifi_conn_scan() == 0) {
        if (wifi_conn_set_sta() == 0) {
            r_event = xEventGroupClearBits(os_event_handle, OS_EVENT_STA_REMOVE_NETWORK);
            r_event = xEventGroupWaitBits(os_event_handle, OS_EVENT_STA_GOT_IP | OS_EVENT_STA_REMOVE_NETWORK,
                pdTRUE, pdFALSE, MS_TO_TICKS(CONFIG_WIFI_CONN_CONNET_TIMEOUT));
            if ((r_event & OS_EVENT_STA_GOT_IP) == OS_EVENT_STA_GOT_IP) {
                os_printf(LM_APP, LL_INFO, "local ota conn OK.\n");
                is_local_ota_conn = true; //may use os_event to replace global variable later
                os_task_delete(g_wifi_conn_task_handle);
                return;
            }
            else if ((r_event & OS_EVENT_STA_REMOVE_NETWORK) == OS_EVENT_STA_REMOVE_NETWORK) {
                os_printf(LM_APP, LL_INFO, "local ota conn , remove_network by other task.\n");
            }
            else { //after 20s, still can't get ip, and it's not disconnect from other task
                os_printf(LM_APP, LL_INFO, "local ota conn , get ip fail.\n");
                wifi_remove_config_all(STATION_IF);
            }
        }
        else {
            os_printf(LM_APP, LL_INFO, "local ota conn, set_sta fail.\n");
        }
    }
    else {
        os_printf(LM_APP, LL_INFO, "local ota scan fail.\n");
    }
#endif

    if(wifi_load_nv_info(NULL) == SYS_OK) {
        wifi_nv_info_t *wifi_nv_info = get_wifi_nv_info();
        os_printf(LM_APP, LL_INFO, "load NV ok, start auto:%d.\n", wifi_nv_info->auto_conn);
        if(1 == wifi_nv_info->auto_conn)
        {
            if (wifi_set_sta_by_nv(NULL) == 0) {
                r_event = xEventGroupClearBits(os_event_handle, OS_EVENT_STA_REMOVE_NETWORK);
                r_event = xEventGroupWaitBits(os_event_handle, OS_EVENT_STA_GOT_IP | OS_EVENT_STA_REMOVE_NETWORK,
                    pdTRUE, pdFALSE, MS_TO_TICKS(CONFIG_WIFI_CONN_CONNET_TIMEOUT));
                if ((r_event & OS_EVENT_STA_GOT_IP) == OS_EVENT_STA_GOT_IP) {
                    os_printf(LM_APP, LL_INFO, "auto conn , get ip success.\n");
                    is_wifi_conn = true; //may use os_event to replace global variable later
                }
                else if ((r_event & OS_EVENT_STA_REMOVE_NETWORK) == OS_EVENT_STA_REMOVE_NETWORK) {
                    os_printf(LM_APP, LL_INFO, "auto conn , remove_network by other task.\n");
                }
                else { //after 20s, still can't get ip, and it's not disconnect from other task
                    os_printf(LM_APP, LL_INFO, "auto conn , get ip fail.\n");
                    wifi_remove_config_all(STATION_IF);
                }
            }
            else {
                os_printf(LM_APP, LL_INFO, "auto conn , set_sta fail.\n");
            }
        }
    }
#ifdef CONFIG_VNET_SERVICE
    else
    {
        wifi_config_u wificonfig;
        int ret;

        if (wifi_load_ap_nv_info(&wificonfig.ap) == SYS_OK) {
            ret = wifi_start_softap(&wificonfig);
            if (ret != SYS_OK) {
                os_printf(LM_APP, LL_INFO, "auto conn ap failed (0x%x)\n", ret);
            }
        }
    }
#endif

    os_task_delete(g_wifi_conn_task_handle);
}


void wifi_conn_main()
{
    g_wifi_conn_task_handle = os_task_create("wifi_conn-task", SYSTEM_EVENT_LOOP_PRIORITY - 1, (4 * 1024), (task_entry_t)wifi_conn_task, NULL);
    return;
}

