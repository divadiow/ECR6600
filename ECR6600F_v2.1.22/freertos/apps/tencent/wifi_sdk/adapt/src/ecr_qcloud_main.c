/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_main.c
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

#include "qcloud_iot_import.h"
#include "qcloud_wifi_config.h"
#include "data_template_client.h"
#include "ecr_qcloud_softap.h"
#include "ecr_qcloud_led.h"
#include "ecr_qcloud_led_driver.h"
#include "ecr_qcloud_ota.h"
#include "ecr_qcloud_iot.h"
#include "service_mqtt.h"
#include "hal/hal_system.h"
#include "easyflash.h"
#include "system_wifi.h"
#include "oshal.h"
#include "hal_rtc.h"

#if defined(CONFIG_BLE_TENCENT_ADAPTER)
#include "qiot_adapter.h"
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

DeviceInfo sg_devInfo;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int _close_r()
{
    return 0;
}

/****************************************************************************
 * Name:  ecr_device_unbind_callback
 * Description:
 * @brief       callback when MQTT unbind_device_msg arrives, save propery to NV and restart
 * @param[in]   pContext: service event context
 * @param[in]   msg: replay data from Wechat mini programe
 * @param[in]   msgLen: length of msg
 ****************************************************************************/

void ecr_device_unbind_callback(void *pContext, const char *msg, uint32_t msgLen)
{
    if (msgLen == 0) {
        Log_e("msg is null");
        return;
    }

    Log_i(" >>msgLen is %d", msgLen);
    Log_i(" >>msg is %s", msg);
    Log_i(" >>pContext is %s", pContext);

    customer_del_env("StaSSID");
    customer_del_env("StaPW");
    customer_del_env("wait_bind");

    hal_reset_type_init();
    hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
    Log_i(" system will be reset, and enter to softap config");
}

/****************************************************************************
 * Name:  ecr_start_dev_ctrl
 * Description:
 * @brief       start ota and led control
 * @param[in]   client: mqtt data template client
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_start_dev_ctrl(Qcloud_IoT_Template *client)
{
    int rc = 0;
    rc = ecr_qcloud_ota_ctrl(client);
    if (rc != QCLOUD_RET_SUCCESS) {
        Log_e("enable ota task fail\n");
        return QCLOUD_ERR_FAILURE;
    }
    rc = ecr_qcloud_led_ctrl(client);
    if (rc != QCLOUD_RET_SUCCESS) {
        Log_e("start led control fail\n");
        return QCLOUD_ERR_FAILURE;
    }
    Log_i("If run here, open Tencent cloud control failed\n");
    return 0;
}

/****************************************************************************
 * Name:  ecr_start_mqtt_client
 * Description:
 * @brief       start mqtt client, and start led control
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_start_mqtt_client(void)
{
    int rc = 0;
    /* init connection */
    TemplateInitParams init_params = DEFAULT_TEMPLATE_INIT_PARAMS;
    rc = _setup_connect_init_params(&init_params); /* initialize device information */
    if (rc != QCLOUD_RET_SUCCESS) {
        Log_e("init params err,rc=%d\n", rc);
        return rc;
    }

    /* construct data template client and connect to the mqtt cloud service */
    void *client = IOT_Template_Construct(&init_params, NULL);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success\n");
    } else {
        Log_e("Cloud Device Construct Failed\n");
        return QCLOUD_ERR_FAILURE;
    }
#ifdef MULTITHREAD_ENABLED /* multi-threading */
    rc = IOT_Template_Start_Yield_Thread(client);
    if (rc != QCLOUD_RET_SUCCESS)
    {
        Log_e("create yield thread fail\n");
        return QCLOUD_ERR_FAILURE;
    }
#endif /* MULTITHERAD_ENABLED */

    HAL_GetDevInfo((void *)&sg_devInfo);  /* 获取预设 设备信息，产品类型，id，秘钥，地区 */
    /* 订阅mqtt_service主题消息 */
    rc =  qcloud_service_mqtt_init(sg_devInfo.product_id, sg_devInfo.device_name, IOT_Template_Get_MQTT_Client(client));
    if (rc < 0) {
        Log_e("service init failed: %d", rc);
        return rc;
    }

    IOT_Unbind_Device_Register(ecr_device_unbind_callback, NULL); /* 注册设备解绑功能 */

    ecr_led_diver_init(); /* pwm初始化 */
    rc = ecr_start_dev_ctrl(client);
    if (rc != QCLOUD_RET_SUCCESS)
    {
        Log_e("start device control fail\n");
        return QCLOUD_ERR_FAILURE;
    }
    return rc;
}

/****************************************************************************
 * Name:  user_main
 * Description:
 * @brief       connect network and start qcloud mqtt service
 ****************************************************************************/

void user_main()
{
	char ap_station_dev[48 + 1]={0};
    char value = 0;
    wifi_config_u wifi_config = {0};
    wifi_bind_u wifi_bind ={-1};
    int wifi_connected, wifi_ret;
    int app_bind_state = -1, chanel = 1;
	int32_t read_len;
	int softap_ret=0;
    ecr_sntp();                                                         /* sntp时间同步 */
    value = ecr_system_reset_count(value);                              /* 计数器 */
    wifi_ret = ecr_get_stored_wifi_config(&wifi_config);                /* 获得NV存储WiFi SSID、PSW信息 */
    wifi_bind = ecr_get_app_bind_status(wifi_bind, chanel,wifi_config); /* 获取NV 绑定状态信息 */
    app_bind_state = wifi_bind.app_bind_state;
    wifi_connected = wifi_bind.wifi_connected;

    /* 从NV里读取到ssid和passwd（已完成绑定），且启动次数小于3次进入sta模式； */
    if (value < 3 && wifi_ret == 0 && app_bind_state == 0) {
        Log_i("enter to STA mode !\n");

        while (!wifi_is_ready()) { //等待WiFi就绪
            // system_printf(">>>>>>>wifi is not ready !\n");
            os_msleep(50);
        }

        int ret = 0;
        ret = ecr_sta_connect_router((char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password, chanel);
        if (ret == 0) {
            Log_i("config STA mode OK !\n");
        }else{
            Log_e("config STA mode failed !\n");
            return ;
        }

        HAL_SleepMs(5000); /* 等待5秒后再去查询wifi状态 */
        wifi_connected = ecr_is_wifi_config_successful();
    } else {
        if (wifi_ret == 0) {
            customer_del_env("StaSSID");
            customer_del_env("StaPW");
        }
        int wifi_config_state = ESWIN_WIFI_CONFIG_GOING_ON;  /* 初始化 */	
	if((read_len=(customer_get_env_blob("ap_station_dev",ap_station_dev,sizeof(ap_station_dev),NULL)))>0)
	{
		
		os_printf(LM_CMD,LL_INFO,"ap_station_dev=%s,len=%d\n",ap_station_dev,read_len);
		softap_ret = ecr_start_softap(ap_station_dev, NULL, 0);
	}
	else
	{
		softap_ret = ecr_start_softap(AP_STATION, NULL, 0);
	}
        
#if defined(CONFIG_BLE_TENCENT_ADAPTER)
        /*创建ble netcfg task*/
        start_ble_netcfg_task(true);
#endif
        /* 开启配网进度状态循环监控，因为在配网中从UDP MQTT线程创建到配网成功需要一段时间 */
        if (softap_ret != 0) {
            Log_e("start wifi config failed !\n");
        } else {
            /* max waiting: 150 * 2000ms */
            int wait_cnt = 150;
            do {
                Log_i("waiting for wifi config result...\n");
                HAL_SleepMs(2000);
                wifi_config_state = ecr_query_wifi_config_state();
            } while (wifi_config_state == ESWIN_WIFI_CONFIG_GOING_ON && wait_cnt--);
        }
        /* 退出配网状态时，检测网络状态若成功往下走，否则打印log退出 */
        wifi_connected = ecr_is_wifi_config_successful();
        if ( wifi_config_state != 0 || wifi_config_state != ESWIN_WIFI_CONFIG_SUCCESS){
            Log_e("wifi config failed!\n");
            // setup a softAP to upload log to mini program
            ecr_start_softap_log();
            return ;
        }
    }

    if (0 == wifi_connected) {
        Log_i("WiFi is ready, to do Qcloud IoT demo\n");
        DATE_RTC cur_time = {0};
        int time_ret = hal_rtc_utctime_to_date(hal_rtc_get_time(), &cur_time);
        if (time_ret == 0)
        {
            Log_i("CurrentTime: week[%d] %4d-%02d-%02d %02d:%02d:%02d\n",
               cur_time.wday, cur_time.year, cur_time.month,  cur_time.day, cur_time.hour, cur_time.min, cur_time.sec);
        }
        else
        {
            Log_e("hal_rtc_utctime_to_date() return err[%d]!\r\n", time_ret);
        }

        ecr_start_mqtt_client(); //构建MQTT客户端，起相关控制线程OTA和灯控
    } else {
        Log_e("WiFi is not ready, please check configuration\n");
    }

    // Log_i("qcloud main Current Free Heap Size = %10d\n", xPortGetFreeHeapSize() ); //剩余的可用字节数
    // Log_i("qcloud main Minumum Free Heap Size = %10d\n", xPortGetMinimumEverFreeHeapSize() ); //剩余的最小可用字节数

    Log_w("finish enter qcloud main");
}
