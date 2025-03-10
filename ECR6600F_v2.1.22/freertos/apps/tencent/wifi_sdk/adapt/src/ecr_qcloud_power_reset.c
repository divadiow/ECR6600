/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_power_reset.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  MengCai <caimeng@eswin.com>
 *   Revision history:
 *   2021.08.25 - Initial version.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "system_wifi.h"
#include "system_wifi_def.h"
#include "easyflash.h"
#include "ecr_qcloud_iot.h"
#include "qcloud_iot_export.h"
#include "ecr_qcloud_softap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define QCLOUD_SNTP_TIME_ZONE      1
#define TIME_ZONE_IN_NV_STRING_LEN 6

/****************************************************************************
 * Private Data
 ****************************************************************************/

static os_timer_handle_t ap_timer;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_get_stored_wifi_config
 * Description:
 * @brief       get wifi configure information from NV flash
 * @param[in]   wifi_config: softAP mode or STA mode
 * @return  0: success, -1: no value in NV
 ****************************************************************************/

int ecr_get_stored_wifi_config(wifi_config_u *wifi_config)
{
    int ssid_len, passwd_len;
    ssid_len = customer_get_env_blob("StaSSID",wifi_config->sta.ssid, WIFI_SSID_MAX_LEN-1, NULL);
    if (ssid_len == 0) {
        Log_e("fread failed! \n");
    } else {
        Log_i("fread succeed, StaSSID : %s, read_len = %d\n\r", wifi_config->sta.ssid, ssid_len);
    }

    passwd_len = customer_get_env_blob("StaPW",wifi_config->sta.password, WIFI_PWD_MAX_LEN-1, NULL);
    if (passwd_len == 0) {
        Log_e("fread failed! \n");
    } else {
        Log_i("fread succeed, StaPW : %s, read_len = %d\n\r", wifi_config->sta.password, passwd_len);
    }

    if (ssid_len == 0 || passwd_len == 0) {
        return -1;
    }
    return 0;
}

/****************************************************************************
 * Name:  ecr_sntp
 * Description:
 * @brief       sntp network time sync
 * this code section is used for sntp sync service. it involves the confirmation of time information in the NV.
 * there is attempts read NV to prevent the time information from being inaccurate.
 * if read fails,the NV does not contain time information.
 * different NV file field has different values, the wifi sntp service is based on the client
 * the NV of the development environment does not have this field. If it can be read later, this code section can be deleted
 ****************************************************************************/
#if QCLOUD_SNTP_TIME_ZONE
#include "system_config.h"
#endif
void ecr_sntp(void)
{
#if QCLOUD_SNTP_TIME_ZONE
    int flag;
    char time_zone_nv[TIME_ZONE_IN_NV_STRING_LEN] = {0};
    //若单板时间不准，则有可能是之前代码问题导致的NV字段异常，打开注释将字段删除，下次烧录运行时关闭。
    //customer_get_env_blob(CUSTOMER_NV_SNTP_TIMEZONE, time_zone_nv, sizeof(time_zone_nv), NULL);
    flag = hal_system_get_config(CUSTOMER_NV_SNTP_TIMEZONE, time_zone_nv, sizeof(time_zone_nv));
    if (flag == 0)
    {
        Log_e("fread  failed \n");
        time_zone_nv[0] = '8';
        flag = hal_system_set_config(CUSTOMER_NV_SNTP_TIMEZONE, time_zone_nv, sizeof(time_zone_nv)); 
        if (flag == 0)
        {
             Log_i("set  ok \n");
        }
    } else {
	int timezone = atoi(time_zone_nv);
        Log_i("fread  succeed , time_zone is  %d\n", timezone);
    }
#endif  /* QCLOUD_SNTP_TIME_ZONE */
    return;
}

/****************************************************************************
 * Name:  ecr_system_timeout_func
 * Description:
 * @brief       deal with time out, and restore the StartCount of the system.
 * @param[out]  param: start count
 ****************************************************************************/
int StartCountFlag = 0;
void ecr_system_timeout_func(void* param)
{
    StartCountFlag = 1;
    os_timer_stop(ap_timer);
    os_timer_delete(ap_timer);
}

/****************************************************************************
 * Name:  start_count_set
 * Description:
 * @brief       create task to reset StartCount.
 * @param[out]  param: arg
 ****************************************************************************/
int reset_count_task_handle;
void start_count_set(void *arg)
{
    int value = 0;
    Log_i("start_count_set task\n");
    int count = 10;
    while(count--){
        if(1 == StartCountFlag){
            value = 0;
            customer_set_env_blob("StartCount", (char *)&value, sizeof(value)); 
            StartCountFlag = 0;
            Log_i("eswin  StartCount reset to:%d\n",value);
            break;
        }
        customer_get_env_blob("StartCount", (char *)&value,  sizeof(value),NULL);
        Log_i("eswin  StartCount:%d\n",value);
        os_msleep(2000);
    }
    os_task_delete(reset_count_task_handle);
}
/****************************************************************************
 * Name:  ecr_system_reset_count
 * Description:
 * @brief       count the number of restart with in 10s
 * @param[in]   value: number of restarts within 10s
 * @return  number of restarts
 ****************************************************************************/

int ecr_system_reset_count(char value)
{
    int StartCount_ret = customer_get_env_blob("StartCount", (char *)&value,  sizeof(value), NULL);
    if (StartCount_ret == 0) {
        Log_e("fread  failed \n");
        value = 1;
        customer_set_env_blob("StartCount", (char *)&value, sizeof(value)); //初始值为1
    } else {
        value += 1;
        customer_set_env_blob("StartCount", (char *)&value, sizeof(value));
    }

    /*创建task,用于重置重启次数*/
    reset_count_task_handle = os_task_create( "tencent_reset_StartCount", 1, 2*1024, (task_entry_t)start_count_set, NULL);

    // create a timeout timer
    ap_timer = os_timer_create("APModeTimer", 10000, 1, ecr_system_timeout_func, NULL);
    os_timer_start(ap_timer);
    Log_i("******APModeTimer start******** \n");

    return value;
}

/****************************************************************************
 * Name:  ecr_get_app_bind_status
 * Description:
 * @brief       get app bind status and wifi config status from qcloud
 * @param[out]  wifi_bind: wifi config status and app bind status
 * @param[in]   chanel: wifi router channel
 * @param[in]   wifi_config: configrue information of device connect to wifi router
 * @return  wifi bind status
 ****************************************************************************/

wifi_bind_u ecr_get_app_bind_status(wifi_bind_u wifi_bind, int chanel, wifi_config_u wifi_config)
{
    #if WIFI_CONFIG_WAIT_APP_BIND_STATE
        // 重启配置wifi前先做app侧绑定信息的查询，看是否绑定,若没有绑定直接进AP，否则进STA
        char wait_app_bind_device_state = 0x00;
        // 配网起MQTT时写wait_bind为true，绑定完成会设false。此处进行读取，若读取成功，进一步根据值判断是否绑定完成
        if ( 0 != customer_get_env_blob("wait_bind",&wait_app_bind_device_state, sizeof(wait_app_bind_device_state), NULL)) {
            if ( wait_app_bind_device_state == WAIT_APP_BIND_TRUE ) { //说明配网拿到ssid 但mqtt绑定上报token失败，这种情况下需要重启配网流程
                ecr_sta_connect_router((char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password, chanel);
                HAL_SleepMs(5000); //等待5秒后再去查询wifi状态
                wifi_bind.wifi_connected = ecr_is_wifi_config_successful();
                Log_i("wifi_connected result %d\n" , wifi_bind.wifi_connected);
                if (0 == wifi_bind.wifi_connected) {
                    wifi_bind.app_bind_state = mqtt_query_app_bind_result();
                    Log_i("query_bind_result:%d\n", wifi_bind.app_bind_state);
                }
            } else { //WAIT_APP_BIND_FALSE
                wifi_bind.app_bind_state = 0;
            }
        } else {  //读取失败说明没有进行过配网
        wifi_bind.app_bind_state = -1;
        }
    #else
        customer_get_env_blob("bind_state",&wifi_bind.app_bind_state, sizeof(app_bind_state), NULL); //配网绑定完成需要存储该信息
        Log_i("fread bind_state is %d\n\r", wifi_bind.app_bind_state);
    #endif  /* WIFI_CONFIG_WAIT_APP_BIND_STATE */

    return wifi_bind;
}
