/****************************************************************************
  * apps/tencent/ble_sdk/adapter/src/qiot_netcfg.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  ZhengdongXia <xiazhengdong@eswin.com>
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

#include <string.h>
#include "cli.h"//print
#include "system_wifi.h"
#include "system_network.h"
#include "oshal.h"
#include "os_task_config.h"
#include "ble_qiot_llsync_device.h"
#if defined (CONFIG_NV)
#include "system_config.h"
#include "easyflash.h"
#endif

#include "system_wifi.h"
#include "system_network.h"
#include "qiot_netcfg.h"
#include "ecr_qcloud_iot.h"
#include "ble_qiot_log.h"
#include "ble_qiot_export.h"
#include "ble_qiot_import.h"
#include "ble_qiot_config.h"
#include "ble_qiot_service.h"
#include "qiot_adapter.h"
#include "qcloud_iot_import.h"
#include "qcloud_iot_export.h"
#define ECR_BLE_GATT_SERVICE_MAX_NUM                                        (3)
#define ECR_BLE_GATT_CHAR_MAX_NUM                                           (3)
static ECR_BLE_GATTS_PARAMS_T		   ecr_ble_gatt_service = {0};
static ECR_BLE_SERVICE_PARAMS_T 	   ecr_ble_common_service[ECR_BLE_GATT_SERVICE_MAX_NUM] = {0};
static ECR_BLE_CHAR_PARAMS_T		   ecr_ble_common_char[ECR_BLE_GATT_CHAR_MAX_NUM] = {0};
#define  COMMON_SERVICE_MAX_NUM      (1)
#define  COMMON_CHAR_MAX_NUM         (2)	 
uint8_t tx_1[]={0xe2,0xa4 ,0x1b, 0x54, 0x93, 0xe4, 0x6a, 0xb5, 0x20, 0x4e, 0xd0, 0x65, 0xe3, 0xff, 0x00, 0x00};
uint8_t rx_1[]={0xe2,0xa4 ,0x1b, 0x54, 0x93, 0xe4, 0x6a, 0xb5, 0x20, 0x4e, 0xd0, 0x65, 0xe1, 0xff, 0x00, 0x00};
uint8_t serv_1[]={0xe2,0xa4 ,0x1b, 0x54, 0x93, 0xe4, 0x6a, 0xb5, 0x20, 0x4e, 0xd0, 0x65, 0xf0, 0xff, 0x00, 0x00};
uint16_t ind_handle;
uint16_t connt_handle;
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NETCFG_BLE_DEV_NAME_LEN     (29)

/****************************************************************************
 * Public Data
 ****************************************************************************/

DeviceInfo qiot_device_info = {0};  /* device information */
int netcfg_task_handle;


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ble_combo_wifi_mode_set
 * Description:
 * @brief       set wifi mode
 * @param[in]   mode: select STA mode or AP mode
 * @return  0
 ****************************************************************************/
static void ble_qiot_netcfg_gap_event_cb(ECR_BLE_GAP_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GAP_EVT_RESET:	
			break;
		case ECR_BLE_GAP_EVT_CONNECT:
			ble_qiot_log_i("qiot_msg_con_ind_handle %d",p_event->conn_handle);
			break;
		case ECR_BLE_GAP_EVT_DISCONNECT:
			ble_gap_disconnect_cb();
			ble_qiot_advertising_start();
			qiot_start_advertising();
			break;
		case ECR_BLE_GAP_EVT_ADV_REPORT:	
			break;
		case ECR_BLE_GAP_EVT_ADV_STATE:
			break;
		default:
			break;
	}

}
static void ble_qiot_netcfg_gatt_event_cb(ECR_BLE_GATT_PARAMS_EVT_T *p_event)
{
	switch(p_event->type)
	{
		case ECR_BLE_GATT_EVT_WRITE_REQ:
			connt_handle=p_event->conn_handle;
			ind_handle=ecr_ble_common_char[1].handle;
			ble_device_info_write_cb(p_event->gatt_event.write_report.report.p_data,p_event->gatt_event.write_report.report.length);
			break;
		case ECR_BLE_GATT_EVT_NOTIFY_INDICATE_TX:
			break;
		default:
			break;
	}

}
static void ble_qiot_netcfg_init(void)
{
	ecr_ble_gap_callback_register(ble_qiot_netcfg_gap_event_cb);
	ecr_ble_gatt_callback_register(ble_qiot_netcfg_gatt_event_cb);
	ECR_BLE_GATTS_PARAMS_T *p_ble_service=&ecr_ble_gatt_service;
	p_ble_service->svc_num=COMMON_SERVICE_MAX_NUM;
	p_ble_service->p_service=ecr_ble_common_service;
	ECR_BLE_SERVICE_PARAMS_T *p_common_service = ecr_ble_common_service;
	p_common_service->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_service->svc_uuid.uuid_type   =  ECR_BLE_UUID_TYPE_128;
	memcpy(&p_common_service->svc_uuid.uuid.uuid16,serv_1,16);
	p_common_service->type	   = ECR_BLE_UUID_SERVICE_PRIMARY;
	p_common_service->char_num = COMMON_CHAR_MAX_NUM;
	p_common_service->p_char   = ecr_ble_common_char;
	
	/*Add write characteristic*/
	ECR_BLE_CHAR_PARAMS_T *p_common_char = ecr_ble_common_char;
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_128;
	memcpy(&p_common_char->char_uuid.uuid.uuid128,rx_1,16);	
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_WRITE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	p_common_char++;
	/*Add Notify characteristic*/
	p_common_char->handle = ECR_BLE_GATT_INVALID_HANDLE;
	p_common_char->char_uuid.uuid_type	 = ECR_BLE_UUID_TYPE_128;
	memcpy(&p_common_char->char_uuid.uuid.uuid128,tx_1,16);
	p_common_char->property = ECR_BLE_GATT_CHAR_PROP_NOTIFY | ECR_BLE_GATT_CHAR_PROP_INDICATE;
	p_common_char->permission = ECR_BLE_GATT_PERM_READ | ECR_BLE_GATT_PERM_WRITE;
	p_common_char->value_len = 252;
	p_common_char++;
	ecr_ble_reset();
	ecr_ble_gatts_service_add(p_ble_service, false);

}

ble_qiot_ret_status_t ble_combo_wifi_mode_set(BLE_WIFI_MODE mode)
{
    static uint8_t reponse_data[4] = {0};
    int index= 0;
    if(BLE_WIFI_MODE_STA == mode)//station
    {
        ble_qiot_log_i("ble combo wifi mode set BLE_WIFI_MODE_STA, mode = 0x%02x",mode);
        wifi_set_opmode(WIFI_MODE_STA);//切换至STA模式
    }
    else if(BLE_WIFI_MODE_AP == mode)//ap
    {
        ble_qiot_log_i("ble combo wifi mode set BLE_WIFI_MODE_AP, mode = 0x%02x",mode);
        wifi_set_opmode(WIFI_MODE_AP);
    }
    int now_mode = wifi_get_opmode();

    //type
   reponse_data[index++] = 0xE0;
    //2 Bytes length
   reponse_data[index++] = 0x00;
   reponse_data[index++] = 0x01;
    //value
   if(now_mode == WIFI_MODE_STA)
   {
       reponse_data[index++] = 0x00;
   }
   else
   {
      reponse_data[index++] = 0x01;
   }
   ecr_ble_gatts_value_notify(connt_handle,ind_handle,reponse_data,sizeof(reponse_data)/sizeof(reponse_data[0]));
   return 0;
}

/****************************************************************************
 * Name:  ble_combo_wifi_info_set
 * Description:
 * @brief       config wifi sta information
 * @param[in]   ssid: device access point ssid
 * @param[in]   ssid_len: length of ssid
 * @param[in]   passwd: device access point password
 * @param[in]   passwd_len: length of passwd
 * @return  0
 ****************************************************************************/

ble_qiot_ret_status_t ble_combo_wifi_info_set(const char *ssid, uint8_t ssid_len, const char *passwd, uint8_t passwd_len)
{
    static uint8_t reponse_data[4] = {0};
    int index = 0;
    int ret = -1;
    char     sta_ssid[WIFI_SSID_MAX_LEN]={'\0'};    /**< SSID of target AP*/
    char     sta_passwd[WIFI_PWD_MAX_LEN]={'\0'};   /**< password of target AP*/
   //type
    reponse_data[index++] = 0xE1;
    //2 Bytes length
    reponse_data[index++] = 0x00;
    reponse_data[index++] = 0x01;

    wifi_set_opmode(WIFI_MODE_STA);//STA模式

    memcpy(sta_ssid, ssid, ssid_len);

    ble_qiot_log_i("ssid [%s]", sta_ssid);

    if (passwd && (passwd_len >= 5)) {
        ble_qiot_log_i("set pwd [%s]", passwd);
        memcpy(sta_passwd, passwd, passwd_len);
    }
     /* save it to flash */
    ret = qiot_flash_SetString("StaSSID", sta_ssid, WIFI_SSID_MAX_LEN-1);
    if(0 != ret)
    {
        ble_qiot_log_i("save StaSSID failed !!!");
        reponse_data[index++] = 0x01;
    }
    else
    {
        ret = qiot_flash_SetString("StaPW", sta_passwd, WIFI_PWD_MAX_LEN-1);
        if(0 != ret)
        {
            ble_qiot_log_i("save StaPW failed !!!");
            reponse_data[index++] = 0x01;
        }
        else
        {
            //success value
            reponse_data[index++] = 0x00;
        }
    }
	ecr_ble_gatts_value_notify(connt_handle,ind_handle,reponse_data,sizeof(reponse_data)/sizeof(reponse_data[0]));

    return 0;
}

/****************************************************************************
 * Name:  ble_report_set_token_state
 * Description:
 * @brief      app report set token status
 ****************************************************************************/

void ble_report_set_token_state()
{
    static uint8_t reponse_data[4] = {0};
    int index = 0;
    //type
    reponse_data[index++] = 0xE3;
    //2 Bytes length
    reponse_data[index++] = 0x00;
    reponse_data[index++] = 0x01;
    //token设置result
    reponse_data[index++] = 0x00;
	ecr_ble_gatts_value_notify(connt_handle,ind_handle,reponse_data,sizeof(reponse_data)/sizeof(reponse_data[0]));
}

/****************************************************************************
 * Name:  ble_report_wifi_connect_state
 * Description:
 * @brief       report wifi connect state
 * @param[in]   mode: wifi STA mode
 * @param[in]   station_state: is connect STA
 * @param[in]   softAp_state: is connect AP
 * @param[in]   ssid: wifi router ssid
 * @param[in]   ssid_len: length of ssid
 ****************************************************************************/

void ble_report_wifi_connect_state(uint8_t mode, uint8_t station_state, uint8_t softAp_state, char *ssid, uint8_t ssid_len)
{
    static uint8_t reponse_data[7+WIFI_SSID_MAX_LEN] = {0};
    int index = 0;
    int len = 0;
   //type
    reponse_data[index++] = 0xE2;
    //2 Bytes length
    len = 4 + ssid_len;//4:wifi_mode:1Byte,station state:1Byte,softAp:1Byte,SSID_len:1byte
    memcpy(&reponse_data[index], &len, 2);
    index += 2;
    //WIFI MODE 当前仅支持 STA
    reponse_data[index++] = mode;
    //Station 状态表示 WIFI 是否连接，0x00 表示已连接，0x01 表示未连接
    reponse_data[index++] = station_state;
    //SoftAp 状态当前不使用
    reponse_data[index++] = softAp_state;
    reponse_data[index++] = ssid_len;
    //已连接时需要将当前的 SSID 上报，未连接时 SSID Length 设置为 0
    memcpy(&reponse_data[index], ssid, ssid_len);
    index += ssid_len;
	ecr_ble_gatts_value_notify(connt_handle,ind_handle,reponse_data,sizeof(reponse_data)/sizeof(reponse_data[0]));
}

/****************************************************************************
 * Name:  ble_combo_wifi_connect
 * Description:
 * @brief       stop AP mode and connect STA
 * @return  0 when success, or err code for failure
 ****************************************************************************/

ble_qiot_ret_status_t ble_combo_wifi_connect()
{
	char ssid[WIFI_SSID_MAX_LEN] = {0};
    char passwd[WIFI_PWD_MAX_LEN] = {0};
    int g_net_status = -1;
    // step 1 close AP mode
    wifi_stop_softap();
    wifi_set_opmode(WIFI_MODE_STA);//切换至STA模式
    //config sta_mode info
    wifi_config_u sta_config;
    memset(&sta_config, 0, sizeof(sta_config));
    qiot_flash_GetString("StaSSID", ssid, WIFI_SSID_MAX_LEN-1);
    qiot_flash_GetString("StaPW", passwd, WIFI_PWD_MAX_LEN-1);
    ble_qiot_log_i("get StaSSID [%s]", ssid);
    ble_qiot_log_i("get StaPW [%s]", passwd);
    memcpy(sta_config.sta.ssid, ssid, strlen(ssid));
    memcpy(sta_config.sta.password, passwd, strlen(passwd));
    //sta_config.sta.channel = 1;
    // step 2 connect to the router
    int ret =  wifi_stop_station();
    if (0 == ret)
    {
        ble_qiot_log_i("net stop start");
        int wait_cnt = 5;
        do{
            ble_qiot_log_i("waiting for wifi config stop result...");
            os_msleep(1000);
            g_net_status = is_wifi_connect_successful();
        } while ((g_net_status == SYS_OK) && wait_cnt--);
    }
    else
    {
        ble_qiot_log_i("net config stop error, %d", ret);
    }

    ret = wifi_start_station(&sta_config);

    if (0 == ret)
    {
        ble_qiot_log_i("net config start");
        int wait_cnt = 5;
        do{
            ble_qiot_log_i("waiting for wifi config result...\n");
            os_msleep(1000);
            g_net_status = is_wifi_connect_successful();
        } while ((g_net_status != SYS_OK) && wait_cnt--);
    }
    else
    {
        ble_qiot_log_i("net config coneect error, %d", ret);
    }

    if (0 != g_net_status) {
        ble_qiot_log_i("wifi_start_station err:%d",g_net_status);
        ble_report_wifi_connect_state(WIFI_MODE_STA, 0x01, 0x01, ssid, strlen(ssid));
        return ret;
    }
    ble_report_wifi_connect_state(WIFI_MODE_STA, 0x00, 0x01, ssid, strlen(ssid));
    return 0;
}

/****************************************************************************
 * Name:  qiot_set_adv_data
 * Description:
 * @brief       set advertising data
 * @param[in]   my_adv_info: advertising information
 ****************************************************************************/

void qiot_set_adv_scanRes_data(adv_info_s *my_adv_info)
{
	ECR_BLE_DATA_T p_adv_data;
	ECR_BLE_DATA_T p_scan_rsp_data;
    uint16_t index = 0;
    //set adv mode --- flag
	static uint8_t scan_rsp_data[31]={0};
	static uint8_t adv_data[31]={0};
    adv_data[index++] = 0x02;
    adv_data[index++] = 0x01;
    adv_data[index++] = 0x06;
    uint8_t manufacture_data_lenght = my_adv_info->manufacturer_info.adv_data_len;
    uint8_t service_uuid_lenght = 2;
    //set uuid
    adv_data[index++] = service_uuid_lenght + 1;
    adv_data[index++] = 0x03; //16-bit UUID
    adv_data[index++] = 0xF0;//service uuid:0xFFE0
    adv_data[index++] = 0xFF;
    //set manufacture
    adv_data[index++] = manufacture_data_lenght + 3;
    adv_data[index++] = 0xff; // GAP appearance
    adv_data[index++] = 0xE7;//manufacture id 0xFEE7
    adv_data[index++] = 0xFE;//index=11
    memcpy(&adv_data[index], my_adv_info->manufacturer_info.adv_data , manufacture_data_lenght);
    index += manufacture_data_lenght;//index=11+17=28
    p_adv_data.length=sizeof(adv_data)/sizeof(adv_data[0]);
	p_adv_data.p_data=adv_data;
	//p_scan_rsp_data.length= strlen(qiot_device_info.device_name) + 1;
	scan_rsp_data[0]=strlen(qiot_device_info.device_name) + 1;
    scan_rsp_data[1] = 0x09;
    if(strlen(qiot_device_info.device_name)<= NETCFG_BLE_DEV_NAME_LEN)
    {
        memcpy(&scan_rsp_data[2],qiot_device_info.device_name, strlen(qiot_device_info.device_name));
        //recv_msg.param_len = strlen(qiot_device_info.device_name) + 2;
    }
    else
    {
        memcpy(&scan_rsp_data[2],qiot_device_info.device_name, NETCFG_BLE_DEV_NAME_LEN);
        //recv_msg.param_len = NETCFG_BLE_DEV_NAME_LEN + 2;
    }
	p_scan_rsp_data.p_data = scan_rsp_data;
	p_scan_rsp_data.length = sizeof(scan_rsp_data)/sizeof(scan_rsp_data[0]);
	ecr_ble_adv_param_set(&p_adv_data, &p_scan_rsp_data);
}

/****************************************************************************
 * Name:  qiot_start_advertising
 * Description:
 * @brief        start advertising
 ****************************************************************************/

void qiot_start_advertising(void)
{
	ECR_BLE_GAP_ADV_PARAMS_T adv_param;
	adv_param.adv_type=ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
	memset(&(adv_param.direct_addr),0,sizeof(ECR_BLE_GAP_ADDR_T));
	adv_param.adv_interval_min=64;//64*0.625=40ms;
	adv_param.adv_interval_max=64;
	adv_param.adv_channel_map=0x07;//Default Value: 0x07;37,38,39
	ecr_ble_gap_adv_start(&adv_param);
}

/****************************************************************************
 * Name:  ble_advertising_stop
 * Description:
 * @brief       stop advertising
 * @return  0
 ****************************************************************************/

ble_qiot_ret_status_t ble_advertising_stop(void)
{
	ecr_ble_gap_adv_stop();
    return 0;
}




/****************************************************************************
 * Name:  ble_send_notify
 * Description:
 * @brief       send notify message
 * @param[in]   buf: send buff
 * @param[in]   len: send length
 * @return  0
 ****************************************************************************/

ble_qiot_ret_status_t ble_send_notify(uint8_t *buf, uint8_t len)
{
	ecr_ble_gatts_value_notify(connt_handle,ind_handle,buf,len);
    return 0;
}

/****************************************************************************
 * Name:  ble_advertising_start
 * Description:
 * @brief       advertising start
 * @param[in]   adv: advertising information
 * @return  0
 ****************************************************************************/

ble_qiot_ret_status_t ble_advertising_start(adv_info_s *adv)
{
    qiot_set_adv_scanRes_data(adv);
    return 0;
}

/****************************************************************************
 * Name:  qiot_process_msg
 * Description:
 * @brief       deal with process message
 * @param[in]   msg: process message
 ****************************************************************************/


/****************************************************************************
 * Name:  qiot_run_loop
 * Description:
 * @brief       receive message
 ****************************************************************************/

void  qiot_netcfg_task(void *arg)
{
    uint8_t  mac[6];
    int ret = HAL_GetDevInfo(&qiot_device_info);
    if (ret) {
        Log_e("[eswin] load device info failed: %d", ret);
    }

    wifi_get_mac_addr(STATION_IF, mac);
    ble_qiot_log_i("device mac%.2X%.2X%.2X%.2X%.2X%.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ble_qiot_explorer_init();
	ble_qiot_netcfg_init();
    ble_qiot_advertising_start();
    qiot_start_advertising();
	//os_task_delete(netcfg_task_handle);
	//netcfg_task_handle = 0;
}

/****************************************************************************
 * Name:  start_ble_netcfg_task
 * Description:
 * @brief       create ble netcfg task
 * @param[in]   flag: true or false
 ****************************************************************************/

void start_ble_netcfg_task(bool flag)
{
    if(flag)
    {
        ble_qiot_log_i("ble netcfg task enable !");
        netcfg_task_handle=os_task_create( "qiot_netcfg-task", BLE_APPS_PRIORITY, BLE_NETCFG_STACK_SIZE, (task_entry_t)qiot_netcfg_task, NULL);
    }
    else
    {
        ble_qiot_log_i("ble netcfg task disable !");
    }

}

